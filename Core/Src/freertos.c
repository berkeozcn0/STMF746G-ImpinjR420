/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : freertos.c
 * @brief          : Impinj R420 LLRP minimal EPC inventory (STM32F746G)
 *
 * Değişiklikler (analiz raporuna göre):
 *  1. DELETE_ROSPEC (ID=0) eklendi — yeniden bağlanmada eski ROSpec temizlenir
 *  2. ROReportSpec eklendi — reader'a "ROSpec bitince rapor gönder" söylenir
 *  3. ADD_ROSPEC frame 62 → 75 byte oldu (ROReportSpec +
 * TagReportContentSelector)
 *  4. LLRP_PARAM_ROREPORTSPEC / TAGREPORTCONTENTSELECTOR define'ları eklendi
 *  5. parse_tag_report_data: bilinmeyen TV tipi sz==0 olduğunda güvenli çıkış
 ******************************************************************************
 */
/* USER CODE END Header */

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "main.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/* -------------------------------------------------------------------------- */
/*                                 AYARLAR                                    */
/* -------------------------------------------------------------------------- */

#define READER_IP_ADDR "169.254.1.1"
#define READER_PORT 5084

#define RX_BUF_SIZE 4096
#define LLRP_HEADER_LEN 10

#define SOCKET_TIMEOUT_MS 10000
#define LISTEN_TIMEOUT_MS 2000 /* Ana döngü: CMD_STOP kontrolü için kısa */

/* Komut sabitleri (View butonlarından) */
#define LLRP_CMD_CONNECT 1u
#define LLRP_CMD_START 2u
#define LLRP_CMD_STOP 3u

/* -------------------------------------------------------------------------- */
/*                          MESSAGE TYPE SABİTLERİ                            */
/* -------------------------------------------------------------------------- */

#define LLRP_MSG_ADD_ROSPEC 20
#define LLRP_MSG_ADD_ROSPEC_RESPONSE 30
#define LLRP_MSG_DELETE_ROSPEC 21          /* YENİ: yeniden bağlanma için */
#define LLRP_MSG_DELETE_ROSPEC_RESPONSE 31 /* YENİ */
#define LLRP_MSG_START_ROSPEC 22
#define LLRP_MSG_START_ROSPEC_RESPONSE 32
#define LLRP_MSG_ENABLE_ROSPEC 24
#define LLRP_MSG_ENABLE_ROSPEC_RESPONSE 34
#define LLRP_MSG_RO_ACCESS_REPORT 61
#define LLRP_MSG_KEEPALIVE 62
#define LLRP_MSG_KEEPALIVE_ACK 72
#define LLRP_MSG_READER_EVENT_NOTIFICATION 63
#define LLRP_MSG_SET_READER_CONFIG 9
#define LLRP_MSG_SET_READER_CONFIG_RESPONSE 19
#define LLRP_MSG_ERROR_MESSAGE 100

/* -------------------------------------------------------------------------- */
/*                         PARAMETER TYPE SABİTLERİ                           */
/* -------------------------------------------------------------------------- */

#define LLRP_PARAM_ROSPEC 177
#define LLRP_PARAM_ROBOUNDARYSPEC 178
#define LLRP_PARAM_ROSPECSTARTTRIGGER 179
#define LLRP_PARAM_ROSPECSTOPTRIGGER 182
#define LLRP_PARAM_AISPEC 183
#define LLRP_PARAM_AISPECSTOPTRIGGER 184
#define LLRP_PARAM_INVENTORYPARAMETERSPEC 186
#define LLRP_PARAM_ROREPORTSPEC 237 /* YENİ: rapor tetikleyicisi */
#define LLRP_PARAM_TAGREPORTCONTENTSELECTOR                                    \
  238 /* YENİ: ROReportSpec içinde zorunlu */
#define LLRP_PARAM_TAGREPORTDATA 240
#define LLRP_PARAM_EPCDATA 241
#define LLRP_PARAM_LLRPSTATUS 287

/* TV parametreler (typeNum < 128) */
#define LLRP_TV_EPC_96 13

/* -------------------------------------------------------------------------- */
/*                                 VERİ TİPLERİ                               */
/* -------------------------------------------------------------------------- */

/* EpcData: 64 byte (Model.cpp tanımıyla aynı layout olmalı) */
typedef struct {
  char hexString[60];
  uint8_t antennaId;   /* 1 veya 2 */
  int8_t peakRssi;     /* YENI: Anten rssi gucu */
  uint16_t phaseAngle; /* YENI: Faz acisi */
} EpcData;

typedef struct {
  char text[64];
} StatusMsg;

osMessageQueueId_t epcQueueHandle;
osMessageQueueId_t statusQueueHandle;
osMessageQueueId_t cmdQueueHandle;

static uint8_t rx_global[RX_BUF_SIZE];
static uint32_t g_msg_id = 1;
static int g_recv_timeout_ms = SOCKET_TIMEOUT_MS;

/* -------------------------------------------------------------------------- */
/*                            YARDIMCI MAKROLAR                               */
/* -------------------------------------------------------------------------- */

#define WRITE_U8(buf, val) (*(buf) = (uint8_t)(val))

#define WRITE_U16_BE(buf, val)                                                 \
  do {                                                                         \
    (buf)[0] = (uint8_t)(((val) >> 8) & 0xFF);                                 \
    (buf)[1] = (uint8_t)((val) & 0xFF);                                        \
  } while (0)

#define WRITE_U32_BE(buf, val)                                                 \
  do {                                                                         \
    (buf)[0] = (uint8_t)(((val) >> 24) & 0xFF);                                \
    (buf)[1] = (uint8_t)(((val) >> 16) & 0xFF);                                \
    (buf)[2] = (uint8_t)(((val) >> 8) & 0xFF);                                 \
    (buf)[3] = (uint8_t)((val) & 0xFF);                                        \
  } while (0)

#define READ_U16_BE(buf) (uint16_t)((((uint16_t)(buf)[0]) << 8) | (buf)[1])

#define READ_U32_BE(buf)                                                       \
  (uint32_t)((((uint32_t)(buf)[0]) << 24) | (((uint32_t)(buf)[1]) << 16) |     \
             (((uint32_t)(buf)[2]) << 8) | ((uint32_t)(buf)[3]))

/* -------------------------------------------------------------------------- */
/*                              UI YARDIMCILARI                               */
/* -------------------------------------------------------------------------- */

/* Durum mesajlarını statusQueueHandle'a gönderir */
static void send_status_ui(const char *m) {
  StatusMsg s;
  memset(&s, 0, sizeof(s));
  strncpy(s.text, m, sizeof(s.text) - 1);
  if (statusQueueHandle != NULL)
    osMessageQueuePut(statusQueueHandle, &s, 0, 0);
}

/* EPC verisini antenna ID ve RSSI ile epcQueueHandle'a gönderir */
static void send_epc_with_ant(const uint8_t *epc, uint8_t len, uint8_t antId,
                              int8_t rssi, uint16_t phase) {
  EpcData d;
  uint8_t i;
  uint8_t max_bytes = (len > 24) ? 24 : len;

  memset(&d, 0, sizeof(d));
  strcpy(d.hexString, "EPC:");
  for (i = 0; i < max_bytes; i++)
    snprintf(&d.hexString[4 + (i * 2)], 3, "%02X", epc[i]);

  d.antennaId = antId;
  d.peakRssi = rssi;
  d.phaseAngle = phase;

  if (epcQueueHandle != NULL)
    osMessageQueuePut(epcQueueHandle, &d, 0, 0);
}

/* -------------------------------------------------------------------------- */
/*                         LLRP HEADER / FRAME BUILD                          */
/* -------------------------------------------------------------------------- */

static void write_msg_header(uint8_t *buf, uint16_t type, uint32_t length,
                             uint32_t id) {
  /* version=1 → bits[12:10]=001, typeNum→bits[9:0] */
  WRITE_U16_BE(buf, (uint16_t)((1U << 10) | (type & 0x03FFU)));
  WRITE_U32_BE(buf + 2, length);
  WRITE_U32_BE(buf + 6, id);
}

static void write_tlv_header(uint8_t *buf, uint16_t type, uint16_t length) {
  /* TLV: top 2 bit = 00, sonra 14 bit typeNum */
  WRITE_U16_BE(buf, (uint16_t)(type & 0x3FFFU));
  WRITE_U16_BE(buf + 2, length);
}

/* -------------------------------------------------------------------------
 * ADD_ROSPEC — 91 byte
 *
 * Offset  Size  İçerik
 * ------  ----  -------
 *  0       10   Message header (type=20, len=91, id)
 * 10        4   ROSpec TLV header (177, 81)
 * 14        4   ROSpecID
 * 18        1   Priority = 0
 * 19        1   CurrentState = 0  ← KRİTİK, mutlaka 0
 * 20        4   ROBoundarySpec TLV header (178, 18)
 * 24        4   ROSpecStartTrigger TLV header (179, 5)
 * 28        1   StartTriggerType = 0 (Null)
 * 29        4   ROSpecStopTrigger TLV header (182, 9)
 * 33        1   StopTriggerType = 0 (Null)
 * 34        4   DurationTriggerValue = 0
 * 38        4   AISpec TLV header (183, 24)
 * 42        2   AntennaCount = 1  (u16v: önce count)
 * 44        2   AntennaID[0] = 0  (0 = tüm antenler)
 * 46        4   AISpecStopTrigger TLV header (184, 9)
 * 50        1   AIStopTriggerType = 0 (Null)
 * 51        4   DurationTrigger = 0
 * 55        4   InventoryParameterSpec TLV header (186, 7)
 * 59        2   InventoryParameterSpecID = 1
 * 61        1   ProtocolID = 1 (C1G2)
 * 62        4   ROReportSpec TLV header (237, 29)
 * 66        1   ROReportTrigger = 2 (End_Of_ROSpec)
 * 67        2   N = 0
 * 69        4   TagReportContentSelector TLV header(238,22)
 * 73        2   Selector bits = 0x1400 (EnableAntennaID | EnablePeakRSSI)
 * ------
 * Toplam: 75 byte
 * ------------------------------------------------------------------------- */
static int build_add_rospec(uint8_t *frame, uint32_t msg_id,
                            uint32_t rospec_id) {
  /* --- Message header --- */
  write_msg_header(frame, LLRP_MSG_ADD_ROSPEC, 75, msg_id);

  /* --- ROSpec TLV @10, len=65 --- */
  write_tlv_header(frame + 10, LLRP_PARAM_ROSPEC, 65);
  WRITE_U32_BE(frame + 14, rospec_id);
  WRITE_U8(frame + 18, 0x00);
  WRITE_U8(frame + 19, 0x00);

  /* --- ROBoundarySpec @20, len=18 --- */
  write_tlv_header(frame + 20, LLRP_PARAM_ROBOUNDARYSPEC, 18);

  /* ROSpecStartTrigger @24, len=5 */
  write_tlv_header(frame + 24, LLRP_PARAM_ROSPECSTARTTRIGGER, 5);
  WRITE_U8(frame + 28, 0x00);

  /* ROSpecStopTrigger @29, len=9 */
  write_tlv_header(frame + 29, LLRP_PARAM_ROSPECSTOPTRIGGER, 9);
  WRITE_U8(frame + 33, 0x00);
  WRITE_U32_BE(frame + 34, 0x00000000);

  /* --- AISpec @38, len=24 --- */
  write_tlv_header(frame + 38, LLRP_PARAM_AISPEC, 24);
  WRITE_U16_BE(frame + 42, 0x0001);
  WRITE_U16_BE(frame + 44, 0x0000);

  /* AISpecStopTrigger @46, len=9 */
  write_tlv_header(frame + 46, LLRP_PARAM_AISPECSTOPTRIGGER, 9);
  WRITE_U8(frame + 50, 0x00);
  WRITE_U32_BE(frame + 51, 0x00000000);

  /* InventoryParameterSpec @55, len=7 */
  write_tlv_header(frame + 55, LLRP_PARAM_INVENTORYPARAMETERSPEC, 7);
  WRITE_U16_BE(frame + 59, 0x0001);
  WRITE_U8(frame + 61, 0x01);

  /* --- ROReportSpec @62, len=13 --- */
  write_tlv_header(frame + 62, LLRP_PARAM_ROREPORTSPEC, 13);
  WRITE_U8(frame + 66, 0x01);       /* Upon_N_Tags_Or_End_Of_ROSpec */
  WRITE_U16_BE(frame + 67, 0x0001); /* N=1 */

  /* TagReportContentSelector @69, len=6 */
  write_tlv_header(frame + 69, LLRP_PARAM_TAGREPORTCONTENTSELECTOR, 6);
  WRITE_U16_BE(frame + 73, 0x1400); /* EnableAntennaID | EnablePeakRSSI */

  return 75;
}

/* -------------------------------------------------------------------------
 * SET_READER_CONFIG — Impinj Phase Angle etkinleştirme
 *
 * Offset  Size  İçerik
 * ------  ----  -------
 *  0       10   Message header (type=9, len=37, id)
 * 10        1   ResetToFactoryDefault = 0
 * 11        4   Custom TLV (1023, 26) ImpinjTagReportContentSelector
 * 15        4   VendorID = 25882
 * 19        4   Subtype = 50
 * 23        4   Custom TLV (1023, 14) ImpinjEnableRFPhaseAngle
 * 27        4   VendorID = 25882
 * 31        4   Subtype = 52
 * 35        2   RFPhaseAngleMode = 1 (Enabled)
 * ------
 * Toplam: 37 byte
 * ------------------------------------------------------------------------- */
static int build_set_reader_config_phase(uint8_t *frame, uint32_t msg_id) {
  write_msg_header(frame, LLRP_MSG_SET_READER_CONFIG, 37, msg_id);
  WRITE_U8(frame + 10, 0x00); /* ResetToFactoryDefault = 0 */

  /* ImpinjTagReportContentSelector @11, len=26 */
  write_tlv_header(frame + 11, 1023, 26);
  WRITE_U32_BE(frame + 15, 25882); /* Impinj VendorID */
  WRITE_U32_BE(frame + 19, 50);    /* Subtype=50 */

  /* ImpinjEnableRFPhaseAngle @23, len=14 */
  write_tlv_header(frame + 23, 1023, 14);
  WRITE_U32_BE(frame + 27, 25882);  /* Impinj VendorID */
  WRITE_U32_BE(frame + 31, 52);     /* Subtype=52 */
  WRITE_U16_BE(frame + 35, 0x0001); /* RFPhaseAngleMode = Enabled */

  return 37;
}

/* DELETE_ROSPEC — 14 byte (ROSpecID=0 → tümünü sil) */
static int build_delete_rospec(uint8_t *frame, uint32_t msg_id,
                               uint32_t rospec_id) {
  write_msg_header(frame, LLRP_MSG_DELETE_ROSPEC, 14, msg_id);
  WRITE_U32_BE(frame + 10, rospec_id); /* 0 = tüm ROSpec'leri sil */
  return 14;
}

/* ENABLE_ROSPEC — 14 byte */
static int build_enable_rospec(uint8_t *frame, uint32_t msg_id,
                               uint32_t rospec_id) {
  write_msg_header(frame, LLRP_MSG_ENABLE_ROSPEC, 14, msg_id);
  WRITE_U32_BE(frame + 10, rospec_id);
  return 14;
}

/* START_ROSPEC — 14 byte */
static int build_start_rospec(uint8_t *frame, uint32_t msg_id,
                              uint32_t rospec_id) {
  write_msg_header(frame, LLRP_MSG_START_ROSPEC, 14, msg_id);
  WRITE_U32_BE(frame + 10, rospec_id);
  return 14;
}

/* KEEPALIVE_ACK — 10 byte */
static int build_keepalive_ack(uint8_t *frame, uint32_t msg_id) {
  write_msg_header(frame, LLRP_MSG_KEEPALIVE_ACK, 10, msg_id);
  return 10;
}

/* -------------------------------------------------------------------------- */
/*                            SOCKET YARDIMCILARI                             */
/* -------------------------------------------------------------------------- */

static int wait_socket_readable(int sock, int timeout_ms) {
  fd_set readfds;
  struct timeval tv;
  int ret;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  ret = select(sock + 1, &readfds, NULL, NULL, &tv);

  if (ret > 0 && FD_ISSET(sock, &readfds))
    return 1;
  if (ret == 0)
    return 0;
  return -1;
}

static int recv_exact_timeout(int sock, uint8_t *buf, int len, int timeout_ms) {
  int total = 0;
  int n;

  while (total < len) {
    int rdy = wait_socket_readable(sock, timeout_ms);
    if (rdy <= 0)
      return -1;

    n = recv(sock, (char *)(buf + total), len - total, 0);
    if (n <= 0)
      return -1;

    total += n;
  }
  return total;
}

/* Tam bir LLRP mesajını buffer'a alır. Dönen değer toplam byte sayısı veya
 * negatif hata. */
static int recv_llrp_message(int sock, uint8_t *buf, int buf_size) {
  int r;
  uint32_t total_len;

  if (buf_size < LLRP_HEADER_LEN)
    return -1;

  /* 1. Header al (g_recv_timeout_ms ile) */
  r = recv_exact_timeout(sock, buf, LLRP_HEADER_LEN, g_recv_timeout_ms);
  if (r != LLRP_HEADER_LEN)
    return -10;

  /* 2. Toplam uzunluk */
  total_len = READ_U32_BE(buf + 2);
  if (total_len < (uint32_t)LLRP_HEADER_LEN || total_len > (uint32_t)buf_size)
    return -11;

  /* 3. Kalan payload */
  if (total_len > (uint32_t)LLRP_HEADER_LEN) {
    r = recv_exact_timeout(sock, buf + LLRP_HEADER_LEN,
                           (int)(total_len - LLRP_HEADER_LEN),
                           g_recv_timeout_ms);
    if (r != (int)(total_len - LLRP_HEADER_LEN))
      return -12;
  }
  return (int)total_len;
}

static int send_all(int sock, const uint8_t *buf, int len) {
  int sent = 0;
  int n;

  while (sent < len) {
    n = send(sock, (const char *)(buf + sent), len - sent, 0);
    if (n <= 0)
      return -1;
    sent += n;
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
/*                              LLRP PARSE                                    */
/* -------------------------------------------------------------------------- */

static uint16_t llrp_get_msg_type(const uint8_t *buf) {
  return (uint16_t)(READ_U16_BE(buf) & 0x03FFU);
}

static uint32_t llrp_get_msg_id(const uint8_t *buf) {
  return READ_U32_BE(buf + 6);
}

/* Response mesajlarındaki LLRPStatus.StatusCode'u döndürür (0=Başarı) */
static uint16_t llrp_get_status_code(const uint8_t *response_buf) {
  const uint8_t *ptr = response_buf + 10;
  uint16_t ptype = (uint16_t)(READ_U16_BE(ptr) & 0x3FFFU);

  if (ptype == LLRP_PARAM_LLRPSTATUS)
    return READ_U16_BE(ptr + 4);

  return 0xFFFFU; /* Parse hatası */
}

/*
 * TV format parametre boyları (header byte dahil):
 * TV header = 1 byte (bit7=1, bits[6:0]=typeNum)
 * Veri hemen arkasından gelir.
 */
static uint8_t tv_param_size(uint8_t typeNum) {
  switch (typeNum) {
  case 1:
    return 3; /* AntennaID: 1+2 */
  case 2:
    return 9; /* FirstSeenTimestampUTC: 1+8 */
  case 3:
    return 9; /* FirstSeenTimestampUptime: 1+8 */
  case 4:
    return 9; /* LastSeenTimestampUTC: 1+8 */
  case 5:
    return 9; /* LastSeenTimestampUptime: 1+8 */
  case 6:
    return 2; /* PeakRSSI: 1+1 */
  case 7:
    return 3; /* ChannelIndex: 1+2 */
  case 8:
    return 3; /* TagSeenCount: 1+2 */
  case 9:
    return 5; /* ROSpecID: 1+4 */
  case 10:
    return 3; /* InventoryParameterSpecID: 1+2 */
  case 13:
    return 13; /* EPC_96: 1+12 (96 bit = 12 byte) */
  case 14:
    return 3; /* SpecIndex: 1+2 */
  case 15:
    return 3; /* ClientRequestOpSpecResult: 1+2 */
  case 16:
    return 5; /* AccessSpecID: 1+4 */
  default:
    return 0; /* Bilinmiyor — çağıran çıkış yapmalı */
  }
}

/*
 * Tek bir TagReportData parametresinin içini parse eder.
 * İçinde EPCParameter bulursa send_epc_hex() çağırır.
 *
 * TagReportData yapısı:
 *   EPCParameter (choice, repeat=1): EPC_96 [TV,13] veya EPCData [TLV,241]
 *   Opsiyonel TV parametreler: AntennaID, PeakRSSI, vb.
 */
/*
 * TagReportData parse: AntennaID (TV type=1) ve EPC'yi birlikte çıkarır.
 * Tam bir pass yapar, sonra send_epc_with_ant() çağırır.
 */
static void parse_tag_report_data(const uint8_t *data, uint32_t len) {
  const uint8_t *ptr = data;
  const uint8_t *end = data + len;
  uint8_t antenna_id = 0;
  int8_t peak_rssi = 0;
  uint8_t epc_buf[24];
  uint8_t epc_len = 0;
  uint8_t has_epc = 0;
  uint16_t phase_angle = 0;

  while (ptr < end) {
    if (*ptr & 0x80U) {
      uint8_t tv_type = (uint8_t)(*ptr & 0x7FU);

      if (tv_type == 1) /* AntennaID: 1+2=3 byte */
      {
        if ((ptr + 3) <= end)
          antenna_id = (uint8_t)(READ_U16_BE(ptr + 1) & 0xFF);
        ptr += 3;
      } else if (tv_type == 6) /* PeakRSSI: 1+1=2 byte */
      {
        if ((ptr + 2) <= end)
          peak_rssi = (int8_t)(ptr[1]);
        ptr += 2;
      } else if (tv_type == LLRP_TV_EPC_96) /* EPC_96: 1+12=13 byte */
      {
        if ((ptr + 13) <= end && !has_epc) {
          memcpy(epc_buf, ptr + 1, 12);
          epc_len = 12;
          has_epc = 1;
        }
        ptr += 13;
      } else {
        uint8_t sz = tv_param_size(tv_type);
        if (sz == 0 || (ptr + sz) > end)
          break;
        ptr += sz;
      }
    } else {
      if ((ptr + 4) > end)
        break;
      uint16_t tlv_type = (uint16_t)(READ_U16_BE(ptr) & 0x3FFFU);
      uint16_t tlv_len = READ_U16_BE(ptr + 2);
      if (tlv_len < 4 || (ptr + tlv_len) > end)
        break;

      if (tlv_type == LLRP_PARAM_EPCDATA && !has_epc) {
        if (tlv_len >= 6) {
          uint16_t bit_count = READ_U16_BE(ptr + 4);
          uint8_t byte_count = (uint8_t)((bit_count + 7U) / 8U);
          if (byte_count > 24)
            byte_count = 24;
          if ((uint16_t)(6 + byte_count) <= tlv_len) {
            memcpy(epc_buf, ptr + 6, byte_count);
            epc_len = byte_count;
            has_epc = 1;
          }
        }
      } else if (tlv_type == 1023) /* Custom Parameter */
      {
        if (tlv_len >= 12) {
          uint32_t vendor = READ_U32_BE(ptr + 4);
          uint32_t subtype = READ_U32_BE(ptr + 8);

          /* ImpinjRFPhaseAngle: vendor=25882, subtype=56 (SDK), len=14 */
          if (vendor == 25882 && subtype == 56 && tlv_len >= 14) {
            phase_angle = READ_U16_BE(ptr + 12);
          }
        }
      }

      ptr += tlv_len;
    }
  }

  if (has_epc)
    send_epc_with_ant(epc_buf, epc_len, antenna_id, peak_rssi, phase_angle);
}

/*
 * RO_ACCESS_REPORT mesajını parse eder.
 * Mesaj: [10 byte header] [TagReportData TLV...] [RFSurveyReportData TLV...]
 * [Custom...] Her TagReportData için parse_tag_report_data() çağırır.
 */
static void llrp_parse_ro_access_report(const uint8_t *buf, uint32_t len) {
  uint32_t msg_len;
  const uint8_t *ptr;
  const uint8_t *end;

  (void)len;

  msg_len = READ_U32_BE(buf + 2);
  if (msg_len < (uint32_t)LLRP_HEADER_LEN)
    return;

  ptr = buf + LLRP_HEADER_LEN;
  end = buf + msg_len;

  while (ptr < end) {
    uint16_t ptype;
    uint16_t plen;

    /* RO_ACCESS_REPORT yalnızca TLV parametreler içerir (top 2 bit = 00) */
    if ((*ptr & 0xC0U) != 0)
      break;

    if ((ptr + 4) > end)
      break;

    ptype = (uint16_t)(READ_U16_BE(ptr) & 0x3FFFU);
    plen = READ_U16_BE(ptr + 2);

    if (plen < 4 || (ptr + plen) > end)
      break;

    if (ptype == LLRP_PARAM_TAGREPORTDATA) {
      parse_tag_report_data(ptr + 4, (uint32_t)(plen - 4));
    }
    /* RFSurveyReportData (242) ve Custom (1023) şimdilik atlanır */

    ptr += plen;
  }
}

/* -------------------------------------------------------------------------- */
/*                      ASYNC MESAJLAR / RESPONSE BEKLEME                     */
/* -------------------------------------------------------------------------- */

/*
 * Asenkron gelen mesajları işler (KEEPALIVE, EVENT_NOTIFICATION,
 * RO_ACCESS_REPORT). İşlendiyse 1, tanınmayan mesajsa 0 döner.
 */
static int handle_async_message(int sock, const uint8_t *buf, int len) {
  uint16_t mt = llrp_get_msg_type(buf);

  (void)len;

  if (mt == LLRP_MSG_KEEPALIVE) {
    uint8_t ack[10];
    int ack_len = build_keepalive_ack(ack, llrp_get_msg_id(buf));
    if (send_all(sock, ack, ack_len) == 0)
      send_status_ui("KA_ACK");
    else
      send_status_ui("KA_ACK_ERR");
    return 1;
  }

  if (mt == LLRP_MSG_READER_EVENT_NOTIFICATION) {
    send_status_ui("EVENT");
    return 1;
  }

  if (mt == LLRP_MSG_RO_ACCESS_REPORT) {
    llrp_parse_ro_access_report(buf, (uint32_t)len);
    return 1;
  }

  return 0;
}

/*
 * Beklenen mesaj tipini alana kadar döngüde okur.
 * Asenkron mesajlar handle_async_message() ile işlenir.
 * Başarıda 0, hata/timeout'ta negatif değer döner.
 */
static int wait_for_message_type(int sock, uint16_t expected_type,
                                 const char *label) {
  while (1) {
    int len = recv_llrp_message(sock, rx_global, RX_BUF_SIZE);
    if (len <= 0) {
      char msg[64];
      snprintf(msg, sizeof(msg), "%s TMOUT:%d", label, len);
      send_status_ui(msg);
      return -1;
    }

    uint16_t mt = llrp_get_msg_type(rx_global);

    /* Asenkron mesaj mı? */
    if (handle_async_message(sock, rx_global, len))
      continue;

    /* Beklenen response mu? */
    if (mt == expected_type) {
      uint16_t status = llrp_get_status_code(rx_global);
      if (status == 0) {
        char ok[48];
        snprintf(ok, sizeof(ok), "%s OK", label);
        send_status_ui(ok);
        return 0;
      } else {
        char err[64];
        snprintf(err, sizeof(err), "%s ERR:%u", label, status);
        send_status_ui(err);
        return -2;
      }
    }

    if (mt == LLRP_MSG_ERROR_MESSAGE) {
      uint16_t status = llrp_get_status_code(rx_global);
      char err[64];
      snprintf(err, sizeof(err), "%s ERRMSG:%u", label, status);
      send_status_ui(err);
      return -3;
    }

    /* Beklenmeyen ama tanınmayan mesaj — logla, döngüye devam et */
    {
      char msg[64];
      snprintf(msg, sizeof(msg), "%s SKIP MT:%u", label, mt);
      send_status_ui(msg);
    }
  }
}

/* Bağlantı kurulduğunda reader'ın gönderdiği ilk EVENT_NOTIFICATION beklenir */
static int wait_initial_reader_event(int sock) {
  while (1) {
    int len = recv_llrp_message(sock, rx_global, RX_BUF_SIZE);
    if (len <= 0) {
      char msg[48];
      snprintf(msg, sizeof(msg), "EVT TMOUT:%d", len);
      send_status_ui(msg);
      return -1;
    }

    uint16_t mt = llrp_get_msg_type(rx_global);

    if (mt == LLRP_MSG_READER_EVENT_NOTIFICATION) {
      send_status_ui("BAGLANTI EVT OK");
      return 0;
    }

    /* Başka bir şey gelirse asenkron handler'a ver, döngüye devam */
    if (handle_async_message(sock, rx_global, len))
      continue;

    /* Tanınmayan mesaj — yine de devam et (strict olmayalım) */
    {
      char msg[48];
      snprintf(msg, sizeof(msg), "ILK MT:%u",
               (unsigned)llrp_get_msg_type(rx_global));
      send_status_ui(msg);
    }
  }
}

/* -------------------------------------------------------------------------- */
/*                           LLRP KOMUT GÖNDERME                              */
/* -------------------------------------------------------------------------- */

static int send_delete_rospec_and_wait(int sock, uint32_t rospec_id) {
  uint8_t frame[14];
  int flen = build_delete_rospec(frame, g_msg_id++, rospec_id);

  if (send_all(sock, frame, flen) < 0) {
    send_status_ui("DEL SEND ERR");
    return -1;
  }
  /* DELETE_ROSPEC_RESPONSE (31) beklenir */
  return wait_for_message_type(sock, LLRP_MSG_DELETE_ROSPEC_RESPONSE, "DEL_RO");
}

static int send_add_rospec_and_wait(int sock, uint32_t rospec_id) {
  uint8_t frame[75];
  int flen = build_add_rospec(frame, g_msg_id++, rospec_id);

  if (send_all(sock, frame, flen) < 0) {
    send_status_ui("ADD SEND ERR");
    return -1;
  }
  return wait_for_message_type(sock, LLRP_MSG_ADD_ROSPEC_RESPONSE, "ADD_RO");
}

static int send_enable_rospec_and_wait(int sock, uint32_t rospec_id) {
  uint8_t frame[14];
  int flen = build_enable_rospec(frame, g_msg_id++, rospec_id);

  if (send_all(sock, frame, flen) < 0) {
    send_status_ui("ENA SEND ERR");
    return -1;
  }
  return wait_for_message_type(sock, LLRP_MSG_ENABLE_ROSPEC_RESPONSE, "ENA_RO");
}

static int send_start_rospec_and_wait(int sock, uint32_t rospec_id) {
  uint8_t frame[14];
  int flen = build_start_rospec(frame, g_msg_id++, rospec_id);

  if (send_all(sock, frame, flen) < 0) {
    send_status_ui("STA SEND ERR");
    return -1;
  }
  return wait_for_message_type(sock, LLRP_MSG_START_ROSPEC_RESPONSE, "STA_RO");
}

/* -------------------------------------------------------------------------- */
/*                                ANA GÖREV                                   */
/* -------------------------------------------------------------------------- */

void StartLlrpTask(void *argument) {
  int s;
  struct sockaddr_in addr;
  uint8_t cmd;
  int consec_timeout;

  (void)argument;

  /* Kuyrukları oluştur */
  if (epcQueueHandle == NULL)
    epcQueueHandle = osMessageQueueNew(10, sizeof(EpcData), NULL);
  if (statusQueueHandle == NULL)
    statusQueueHandle = osMessageQueueNew(10, sizeof(StatusMsg), NULL);
  if (cmdQueueHandle == NULL)
    cmdQueueHandle = osMessageQueueNew(5, sizeof(uint8_t), NULL);

  osDelay(3000); /* lwIP başlamasını bekle */
  send_status_ui("HAZIR: BAGLAN TUSUNA BASIN");

  while (1) {
    /* --- BAGLAN komutunu bekle --- */
    cmd = 0;
    while (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, osWaitForever) == osOK)
      if (cmd == LLRP_CMD_CONNECT)
        break;

    send_status_ui("BAGLANIYOR...");
    g_recv_timeout_ms = SOCKET_TIMEOUT_MS;

    /* TCP soket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
      send_status_ui("SOKET HATASI");
      osDelay(1000);
      send_status_ui("HAZIR: BAGLAN TUSUNA BASIN");
      continue;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)READER_PORT);
    addr.sin_addr.s_addr = inet_addr(READER_IP_ADDR);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      closesocket(s);
      send_status_ui("BAGLANTI HATASI");
      osDelay(2000);
      send_status_ui("HAZIR: BAGLAN TUSUNA BASIN");
      continue;
    }
    send_status_ui("TCP BAGLANDI");

    if (wait_initial_reader_event(s) < 0)
      goto reconnect;

    if (send_delete_rospec_and_wait(s, 0) < 0)
      send_status_ui("DEL ATLANDI");

    /* Impinj Phase Angle etkinleştir (SET_READER_CONFIG) */
    {
      uint8_t cfgframe[37];
      int cfglen = build_set_reader_config_phase(cfgframe, g_msg_id++);
      if (send_all(s, cfgframe, cfglen) < 0) {
        send_status_ui("CFG SEND ERR");
        goto reconnect;
      }
      if (wait_for_message_type(s, LLRP_MSG_SET_READER_CONFIG_RESPONSE,
                                "CFG_PH") < 0)
        send_status_ui("CFG_PH ATLANDI"); /* Hata olsa bile devam et */
    }

    if (send_add_rospec_and_wait(s, 1) < 0)
      goto reconnect;
    if (send_enable_rospec_and_wait(s, 1) < 0)
      goto reconnect;

    send_status_ui("HAZIR: BASLAT TUSUNA BASIN");

    /* --- BASLAT veya DURDUR komutunu bekle --- */
    cmd = 0;
    while (1) {
      if (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, 200) == osOK) {
        if (cmd == LLRP_CMD_START)
          break;
        if (cmd == LLRP_CMD_STOP)
          goto reconnect;
      }
    }

    if (send_start_rospec_and_wait(s, 1) < 0)
      goto reconnect;
    send_status_ui("OKUMA AKTIF");

    /* --- Ana dinleme döngüsü --- */
    g_recv_timeout_ms = LISTEN_TIMEOUT_MS;
    consec_timeout = 0;

    while (1) {
      /* DURDUR komutunu kontrol et (bloklamadan) */
      if (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, 0) == osOK) {
        if (cmd == LLRP_CMD_STOP) {
          send_status_ui("DURDURULDU");
          goto reconnect;
        }
      }

      int len = recv_llrp_message(s, rx_global, RX_BUF_SIZE);
      if (len <= 0) {
        /* -10 = sadece timeout → komut kontrolüne dön */
        if (len == -10) {
          consec_timeout++;
        } else {
          consec_timeout += 5;
        }

        if (consec_timeout > 15) /* ~30 sn veri yok = ger­çek hata */
        {
          send_status_ui("BAGLANTI KOPTU");
          break;
        }
        continue;
      }
      consec_timeout = 0;

      {
        uint16_t mt = llrp_get_msg_type(rx_global);
        if (mt == LLRP_MSG_RO_ACCESS_REPORT)
          llrp_parse_ro_access_report(rx_global, (uint32_t)len);
        else if (mt == LLRP_MSG_KEEPALIVE) {
          uint8_t ack[10];
          int ack_len = build_keepalive_ack(ack, llrp_get_msg_id(rx_global));
          send_all(s, ack, ack_len);
        }
        /* READER_EVENT_NOTIFICATION: yoksay */
      }
      osDelay(1);
    }

  reconnect:
    closesocket(s);
    g_recv_timeout_ms = SOCKET_TIMEOUT_MS;
    send_status_ui("SOKET KAPANDI");
    osDelay(1000);
    send_status_ui("HAZIR: BAGLAN TUSUNA BASIN");
  }
}

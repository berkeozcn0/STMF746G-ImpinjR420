#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "cmsis_os.h"

extern "C" {
    extern osMessageQueueId_t epcQueueHandle;
    extern osMessageQueueId_t statusQueueHandle;
    extern osMessageQueueId_t cmdQueueHandle;
}

#include <string.h>

/* Must match C-side EpcData exactly (64 bytes) */
struct EpcData {
    char    hexString[60];
    uint8_t antennaId;
    int8_t  peakRssi;
    uint8_t _pad[2];
};

/* Must match C-side StatusMsg exactly (64 bytes) */
struct StatusMsg {
    char text[64];
};

Model::Model() : modelListener(0), ant1Count(0) {}

void Model::tick()
{
    EpcData epc;
    if (epcQueueHandle != NULL)
    {
        while (osMessageQueueGet(epcQueueHandle, &epc, NULL, 0U) == osOK)
        {
            if (epc.antennaId == 2)
            {
                if (modelListener != 0)
                    modelListener->updateAntenna2Epc(epc.hexString);
            }
            else
            {
                if (modelListener != 0)
                    modelListener->updateAntenna1Epc(epc.hexString);

                // Anten 1 için listeye ekle / güncelle
                bool found = false;
                for (uint8_t i = 0; i < ant1Count; i++)
                {
                    if (strcmp(ant1List[i].epc, epc.hexString) == 0)
                    {
                        found = true;
                        if (epc.peakRssi > ant1List[i].maxRssi)
                            ant1List[i].maxRssi = epc.peakRssi;
                        break;
                    }
                }

                if (!found && ant1Count < MAX_EPC_COUNT)
                {
                    strncpy(ant1List[ant1Count].epc, epc.hexString, sizeof(ant1List[ant1Count].epc));
                    ant1List[ant1Count].maxRssi = epc.peakRssi;
                    ant1Count++;
                }

                // Basit Insertion Sort (Büyükten küçüğe sıralama)
                for (int i = 1; i < ant1Count; i++)
                {
                    Ant1EpcRecord key = ant1List[i];
                    int j = i - 1;
                    while (j >= 0 && ant1List[j].maxRssi < key.maxRssi)
                    {
                        ant1List[j + 1] = ant1List[j];
                        j = j - 1;
                    }
                    ant1List[j + 1] = key;
                }

                // Ekranı güncellemek için dinleyiciyi tetikle
                if (modelListener != 0)
                    modelListener->updateAnt1List();
            }
        }
    }

    StatusMsg st;
    if (statusQueueHandle != NULL)
    {
        if (osMessageQueueGet(statusQueueHandle, &st, NULL, 0U) == osOK)
        {
            if (modelListener != 0)
                modelListener->updateStatusText(st.text);
        }
    }
}

void Model::sendCommand(unsigned char cmd)
{
    if (cmdQueueHandle != NULL)
        osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
}

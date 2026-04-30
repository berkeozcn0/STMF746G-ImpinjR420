#include <gui/containers/EpcListItem.hpp>

EpcListItem::EpcListItem()
{

}

void EpcListItem::initialize()
{
    EpcListItemBase::initialize();
}

void EpcListItem::setEpcDetails(const char* epc, int8_t rssi, uint16_t count, int16_t phase)
{
    // EPC metnini ayarla
    touchgfx::Unicode::strncpy(textEpcBuffer, epc, TEXTEPC_SIZE);
    
    // RSSI metnini ayarla (örn: "-45 dBm")
    touchgfx::Unicode::snprintf(textRssiBuffer, TEXTRSSI_SIZE, "%d dBm", rssi);

    // Count metnini ayarla
    touchgfx::Unicode::snprintf(textCountBuffer, TEXTCOUNT_SIZE, "%d", count);

    // Phase metnini ayarla
    touchgfx::Unicode::snprintf(textPhaseBuffer, TEXTPHASE_SIZE, "%d", phase);
    
    // Değişikliklerin çizilmesi için invalidate et
    textEpc.invalidate();
    textRssi.invalidate();
    textCount.invalidate();
    textPhase.invalidate();
}

#include <gui/containers/EpcListItem.hpp>

EpcListItem::EpcListItem()
{

}

void EpcListItem::initialize()
{
    EpcListItemBase::initialize();
}

void EpcListItem::setEpcAndRssi(const char* epc, int8_t rssi)
{
    // EPC metnini ayarla
    touchgfx::Unicode::strncpy(textEpcBuffer, epc, TEXTEPC_SIZE);
    
    // RSSI metnini ayarla (örn: "-45 dBm")
    touchgfx::Unicode::snprintf(textRssiBuffer, TEXTRSSI_SIZE, "%d dBm", rssi);
    
    // Değişikliklerin çizilmesi için invalidate et
    textEpc.invalidate();
    textRssi.invalidate();
}

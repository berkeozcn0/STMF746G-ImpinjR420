#ifndef EPCLISTITEM_HPP
#define EPCLISTITEM_HPP

#include <gui_generated/containers/EpcListItemBase.hpp>

class EpcListItem : public EpcListItemBase
{
public:
    EpcListItem();
    virtual ~EpcListItem() {}

    virtual void initialize();

    void setEpcDetails(const char* epc, int8_t rssi, uint16_t count, int16_t phase);
protected:
};

#endif // EPCLISTITEM_HPP

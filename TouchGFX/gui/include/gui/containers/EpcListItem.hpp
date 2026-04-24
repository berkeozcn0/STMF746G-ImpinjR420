#ifndef EPCLISTITEM_HPP
#define EPCLISTITEM_HPP

#include <gui_generated/containers/EpcListItemBase.hpp>

class EpcListItem : public EpcListItemBase
{
public:
    EpcListItem();
    virtual ~EpcListItem() {}

    virtual void initialize();

    void setEpcAndRssi(const char* epc, int8_t rssi);
protected:
};

#endif // EPCLISTITEM_HPP

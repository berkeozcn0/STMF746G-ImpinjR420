#include <gui/screen3_screen/Screen3View.hpp>

Screen3View::Screen3View() :
    clearButtonCallback(this, &Screen3View::clearButtonClicked)
{

}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    onClearButtonClicked.setAction(clearButtonCallback);
    updateAnt2List();
}

void Screen3View::tearDownScreen()
{
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::updateAnt2List()
{
    uint8_t count = presenter->getAnt2EpcCount();
    scrollListAnt2.setNumberOfItems(count);
    scrollListAnt2.invalidate();
}

void Screen3View::scrollListAnt2UpdateItem(EpcListItem& item, int16_t itemIndex)
{
    uint8_t count = presenter->getAnt2EpcCount();
    if (itemIndex >= 0 && itemIndex < count)
    {
        const Ant1EpcRecord* list = presenter->getAnt2EpcList();
        item.setEpcDetails(list[itemIndex].epc, list[itemIndex].maxRssi, list[itemIndex].seenCount, list[itemIndex].phaseAngle);
    }
}

void Screen3View::clearButtonClicked(const touchgfx::AbstractButton& src)
{
    if (&src == &onClearButtonClicked)
    {
        presenter->clearList();
    }
}

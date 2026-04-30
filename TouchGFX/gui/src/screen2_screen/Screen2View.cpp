#include <gui/screen2_screen/Screen2View.hpp>

Screen2View::Screen2View() :
    clearButtonCallback(this, &Screen2View::clearButtonClicked)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    onClearButtonClicked.setAction(clearButtonCallback);
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::updateAnt1List()
{
    uint8_t count = presenter->getAnt1EpcCount();
    scrollListAnt1.setNumberOfItems(count);
    scrollListAnt1.invalidate();
}

void Screen2View::scrollListAnt1UpdateItem(EpcListItem& item, int16_t itemIndex)
{
    uint8_t count = presenter->getAnt1EpcCount();
    if (itemIndex >= 0 && itemIndex < count)
    {
        const Ant1EpcRecord* list = presenter->getAnt1EpcList();
        item.setEpcDetails(list[itemIndex].epc, list[itemIndex].maxRssi, list[itemIndex].seenCount, list[itemIndex].phaseAngle);
    }
}

void Screen2View::clearButtonClicked(const touchgfx::AbstractButton& src)
{
    if (&src == &onClearButtonClicked)
    {
        presenter->clearList();
    }
}

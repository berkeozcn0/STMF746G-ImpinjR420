#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{

}

void Screen2Presenter::deactivate()
{

}

void Screen2Presenter::updateAnt1List()
{
    view.updateAnt1List();
}

const Ant1EpcRecord* Screen2Presenter::getAnt1EpcList() const
{
    return model->getAnt1EpcList();
}

uint8_t Screen2Presenter::getAnt1EpcCount() const
{
    return model->getAnt1EpcCount();
}

void Screen2Presenter::clearList()
{
    model->clearAnt1List();
}

#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v)
    : view(v)
{

}

void Screen3Presenter::activate()
{

}

void Screen3Presenter::deactivate()
{

}

void Screen3Presenter::updateAnt2List()
{
    view.updateAnt2List();
}

const Ant1EpcRecord* Screen3Presenter::getAnt2EpcList() const
{
    return model->getAnt2EpcList();
}

uint8_t Screen3Presenter::getAnt2EpcCount() const
{
    return model->getAnt2EpcCount();
}

void Screen3Presenter::clearList()
{
    model->clearAnt2List();
}

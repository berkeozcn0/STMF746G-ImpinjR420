#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{
}

void Screen1Presenter::activate()
{
}

void Screen1Presenter::deactivate()
{
}

/**
 * Model'den gelen veriyi View'a ileten köprü fonksiyon
 */
void Screen1Presenter::updateEpcText(char* epc) {
    view.updateEpcText(epc);
}

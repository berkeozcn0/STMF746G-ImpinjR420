#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void updateAntenna1Epc(char* epc);
    virtual void updateAntenna2Epc(char* epc);
    virtual void updateStatusText(char* status);
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);
};

#endif // SCREEN1VIEW_HPP

#ifndef SCREEN1PRESENTER_HPP
#define SCREEN1PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen1View;

class Screen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1Presenter(Screen1View& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~Screen1Presenter() {};

    virtual void updateAntenna1Epc(char* epc) override;
    virtual void updateAntenna2Epc(char* epc) override;
    virtual void updateStatusText(char* status) override;

    void onConnectClicked();
    void onStartClicked();
    void onStopClicked();

private:
    Screen1Presenter();
    Screen1View& view;
};

#endif // SCREEN1PRESENTER_HPP

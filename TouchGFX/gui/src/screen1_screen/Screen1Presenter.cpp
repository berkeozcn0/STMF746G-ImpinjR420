#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v) : view(v) {}

void Screen1Presenter::activate() {}
void Screen1Presenter::deactivate() {}

void Screen1Presenter::updateAntenna1Epc(char* epc) { view.updateAntenna1Epc(epc); }
void Screen1Presenter::updateAntenna2Epc(char* epc) { view.updateAntenna2Epc(epc); }
void Screen1Presenter::updateStatusText(char* status) { view.updateStatusText(status); }

void Screen1Presenter::onConnectClicked() { model->sendCommand(1); }
void Screen1Presenter::onStartClicked()   { model->sendCommand(2); }
void Screen1Presenter::onStopClicked()    { model->sendCommand(3); }

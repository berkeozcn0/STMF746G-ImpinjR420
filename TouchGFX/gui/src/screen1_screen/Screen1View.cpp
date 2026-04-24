#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/events/ClickEvent.hpp>

Screen1View::Screen1View() {}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::updateAntenna1Epc(char* epc)
{
    if (!epc || !epc[0]) return;
    touchgfx::Unicode::strncpy(taAnt1Buffer, epc, TAANT1_SIZE);
    taAnt1.invalidate();
}

void Screen1View::updateAntenna2Epc(char* epc)
{
    if (!epc || !epc[0]) return;
    touchgfx::Unicode::strncpy(taAnt2Buffer, epc, TAANT2_SIZE);
    taAnt2.invalidate();
}

void Screen1View::updateStatusText(char* status)
{
    if (!status || !status[0]) return;
    touchgfx::Unicode::strncpy(taStatusBuffer, status, TASTATUS_SIZE);
    taStatus.invalidate();
}

void Screen1View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    // Orijinal TouchGFX widget (buton vs) tıklamalarının çalışması için 
    // base fonksiyonu HER DURUMDA çağırmalıyız:
    Screen1ViewBase::handleClickEvent(evt);

    if (evt.getType() != touchgfx::ClickEvent::RELEASED) return;

    int x = evt.getX(), y = evt.getY();
    touchgfx::Rect r;

    r = bgBtnConnect.getRect();
    if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
    { presenter->onConnectClicked(); return; }

    r = bgBtnStart.getRect();
    if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
    { presenter->onStartClicked(); return; }

    r = bgBtnStop.getRect();
    if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
    { presenter->onStopClicked(); return; }
}

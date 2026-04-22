#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /**
     * Presenter'dan gelen EPC verisini almak için bu fonksiyonu ekliyoruz.
     * Bu sayede Presenter'daki "view.updateEpcText(epc)" çağrısı derlenebilecek.
     */
    virtual void updateEpcText(char* epc);

protected:
};

#endif // SCREEN1VIEW_HPP

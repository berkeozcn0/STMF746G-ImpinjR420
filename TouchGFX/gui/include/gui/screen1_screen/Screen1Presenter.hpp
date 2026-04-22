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

    /**
     * ModelListener'daki fonksiyonu burada 'override' ediyoruz.
     * Model.cpp'den gelen veri buraya düşer.
     */
    virtual void updateEpcText(char* epc) override;

private:
    Screen1Presenter();

    Screen1View& view;
};

#endif // SCREEN1PRESENTER_HPP

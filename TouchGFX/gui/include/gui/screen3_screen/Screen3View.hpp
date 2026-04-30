#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void scrollListAnt2UpdateItem(EpcListItem& item, int16_t itemIndex);
    void updateAnt2List();

protected:
    touchgfx::Callback<Screen3View, const touchgfx::AbstractButton&> clearButtonCallback;
    void clearButtonClicked(const touchgfx::AbstractButton& src);
};

#endif // SCREEN3VIEW_HPP

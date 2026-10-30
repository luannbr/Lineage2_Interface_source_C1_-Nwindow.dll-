#pragma once
// NCHtmlSliderBar - a scroll bar thumb inside an HTML page (UClass "CHtmlSliderBar", 0x1CC):
// NCSliderBar clipped to its first ancestor that is not a control (the scroll bar of an HTML
// multi-line edit box).

#include "nwindow/NCSliderBar.h"

class NCHtmlSliderBar : public NCSliderBar
{
    NW_DECLARE_CLASS(NCHtmlSliderBar, NCSliderBar, 0, 0x10129900)

    NCHtmlSliderBar();                                               // 0x10032fc0
    NCHtmlSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName,
                    const TCHAR* BottomTexName);                     // 0x10033060
    // ~NCHtmlSliderBar                                              // 0x10033010

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100301f0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100330e0
};
NW_ASSERT_SIZE(NCHtmlSliderBar, 0x1CC);

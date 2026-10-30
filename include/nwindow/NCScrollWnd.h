#pragma once
// NCScrollWnd - window whose content scrolls vertically (UClass "CScrollWnd", 0xCC): base of
// the HTML viewers, item windows and quest/notice info windows. ScrollY is the content offset
// (0 at the top, down to MinScrollY = Height - content height); Delta* is how much the last
// change moved it, for the subclasses that repaint incrementally. Scroll up / down buttons and
// a slider sit on the right edge; the keyboard (PageUp/PageDown/arrows) and the wheel scroll.

#include "nwindow/NCWnd.h"

class NCPushButton;
class NCSliderBar;

#if NW_HOOK_MODE
#define GNextScrollCtrlID NW_GLOBAL(INT, 0x10109de8)   // ids of the scroll controls, from 0x1200
#else
extern INT GNextScrollCtrlID;
#endif

class NCScrollWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCScrollWnd, NCWnd, 0, 0x10122dd8)

    INT ScrollX;                     // +0x94 never changed here
    INT ScrollY;                     // +0x98 <= 0
    INT DeltaX;                      // +0x9C previous - current, after each scroll
    INT DeltaY;                      // +0xA0
    INT PageStep;                    // +0xA4 (100) PageUp / PageDown
    INT LineStep;                    // +0xA8 (15) arrows, buttons, wheel (3 per notch)
    INT MinScrollY;                  // +0xAC Height - content height (SetContentHeight)
    NCPushButton* ScrollUpButton;    // +0xB0
    NCPushButton* ScrollDownButton;  // +0xB4
    INT ScrollUpID;                  // +0xB8
    INT ScrollDownID;                // +0xBC
    NCSliderBar* ScrollBar;          // +0xC0
    INT ScrollBarID;                 // +0xC4
    INT Unknown0C8;                  // +0xC8 (12)

    NCScrollWnd();                                                   // 0x100210a0
    // ~NCScrollWnd                                                  // 0x100168e0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10020f40
    virtual INT OnCreate();                                          // 35 0x10021120
    virtual INT OnDestroy();                                         // 36 0x10021380
    virtual void SetSize(INT W, INT H);                              // 41 0x10020ff0
    virtual INT OnPaint(NCGDevice* Device) { return 1; }             // 50 0x10002830 (folded)
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10021420
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x10021690
    // names ours
    virtual INT ScrollUp();                                          // 62 0x10021560
    virtual INT ScrollDown();                                        // 63 0x100215f0
    virtual INT OnScrollBar(INT Value);                              // 64 0x100a0900

    void SetContentHeight(INT ContentHeight);                        // 0x10021050
    void SetRepeatDelay(INT Delay);                                  // 0x100210f0
};
NW_ASSERT_OFFSET(NCScrollWnd, ScrollY, 0x98);
NW_ASSERT_OFFSET(NCScrollWnd, MinScrollY, 0xAC);
NW_ASSERT_OFFSET(NCScrollWnd, ScrollBar, 0xC0);
NW_ASSERT_SIZE(NCScrollWnd, 0xCC);

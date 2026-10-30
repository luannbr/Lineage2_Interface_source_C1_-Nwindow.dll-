#pragma once
// NCSliderBar - vertical scroll bar thumb (UClass "CSliderBar", 0x1CC), used by the lists,
// edit boxes and scroll windows. SetRange(Total, Page) sizes the thumb (Height * Page /
// Total, at least 15 px); dragging it sets Value (first visible item) and reports it to the
// parent (NWM_COMMAND, lParam = Value).

#include "nwindow/NControl.h"

class NCSliderBar : public NControl
{
    NW_DECLARE_CLASS(NCSliderBar, NControl, 0, 0x10123300)

    INT ThumbSize;             // +0x1A0 px
    INT ThumbPos;              // +0x1A4 px from the top
    INT Total;                 // +0x1A8 items
    INT TrackLen;              // +0x1AC px the thumb can travel (Height - ThumbSize)
    INT Value;                 // +0x1B0 first visible item
    INT Page;                  // +0x1B4 visible items
    UBOOL bDragging;           // +0x1B8
    INT LastY;                 // +0x1BC mouse Y of the drag, relative
    UTexture* ThumbTopTex;     // +0x1C0 5x5
    UTexture* ThumbMidTex;     // +0x1C4 stretched
    UTexture* ThumbBottomTex;  // +0x1C8 5x5

    NCSliderBar();                                                   // 0x100219d0
    NCSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName,
                const TCHAR* BottomTexName);                         // 0x10021ac0
    // ~NCSliderBar                                                  // 0x10021a70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10021b80
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100218e0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10021870
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100218c0

    // names ours
    void SetRange(INT InTotal, INT InPage);                          // 0x100217a0
    void SetValue(INT NewValue);                                     // 0x10021800
    void Reset();                                                    // 0x100219b0
    void SetScrollRange(INT InTotal, INT InPage);                    // 0x10021cc0 (jmp SetRange)
    void SetScrollPos(INT NewValue);                                 // 0x10021cd0 (jmp SetValue)
    // an empty body the multi-line edit boxes call on their bar (name ours; the "ret 4" many
    // empty one-argument methods were folded into, too small to hook)
    void DoNothing(INT A) {}                                         // 0x10001700 (folded)
};
NW_ASSERT_OFFSET(NCSliderBar, TrackLen, 0x1AC);
NW_ASSERT_OFFSET(NCSliderBar, LastY, 0x1BC);
NW_ASSERT_SIZE(NCSliderBar, 0x1CC);

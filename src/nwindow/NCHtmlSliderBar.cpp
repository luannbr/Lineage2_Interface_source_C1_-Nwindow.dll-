/*=============================================================================
    NCHtmlSliderBar.cpp - the slider of the HTML pages. Stock: 0x10032fc0 .. 0x1003305f,
    0x10033060 .. 0x1003321e (WindowProc is a body the linker folded with the buttons').
=============================================================================*/
#include "nwindow/NCHtmlSliderBar.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"

// 0x10032fc0
NCHtmlSliderBar::NCHtmlSliderBar()
{
}
NW_HOOK_CTOR(0x10032fc0, NCHtmlSliderBar)

// 0x10033060 - NCSliderBar's default constructor, then the three textures
NCHtmlSliderBar::NCHtmlSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName,
                                 const TCHAR* BottomTexName)
{
    ThumbTopTex = FindTexture(TopTexName);
    ThumbMidTex = FindTexture(MidTexName);
    ThumbBottomTex = FindTexture(BottomTexName);
}
NW_HOOK_CTOR_ARGS(0x10033060, NCHtmlSliderBar, (const TCHAR* A, const TCHAR* B, const TCHAR* C),
                  (A, B, C))

// 0x10033010
NW_HOOK_DTOR(0x10033010, NCHtmlSliderBar)

// stock body shared with NCExButton's (0x100301f0)
INT NCHtmlSliderBar::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCSliderBar::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100330e0 (vslot 50) - NCSliderBar::OnPaint with the clip of the page; the origin goes on
// before the clip
INT NCHtmlSliderBar::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlSliderBar::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(Owner->ScreenX, Owner->ScreenY, Owner->Width, Owner->Height);
    if (ThumbSize > 0)
    {
        if (ThumbTopTex)
            Device->DrawTexture(3, ThumbPos, 5, 5, 0, 0, 5, 5, ThumbTopTex);
        if (ThumbBottomTex)
            Device->DrawTexture(3, ThumbPos + ThumbSize - 5, 5, 5, 0, 0, 5, 5, ThumbBottomTex);
        if (ThumbMidTex)
            Device->DrawStretchedTex(3, ThumbPos + 5, 5, ThumbSize - 10, 0, 0, 5, 4, ThumbMidTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100330e0, NCHtmlSliderBar, OnPaint)

NW_CHECK_VTABLE(NCHtmlSliderBar, 0x100df5b0, 63)

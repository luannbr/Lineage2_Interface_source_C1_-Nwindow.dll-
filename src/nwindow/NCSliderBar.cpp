/*=============================================================================
    NCSliderBar.cpp - scroll bar thumb. Stock: 0x100217a0 .. 0x10021cd5.
=============================================================================*/
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"

// 0x100219d0
NCSliderBar::NCSliderBar()
{
    ThumbSize = 0;
    ThumbPos = 0;
    Value = 0;
    Page = 0;
    bDragging = 0;
    ThumbTopTex = NULL;
    ThumbMidTex = NULL;
    ThumbBottomTex = NULL;
}
NW_HOOK_CTOR(0x100219d0, NCSliderBar)

// 0x10021ac0
NCSliderBar::NCSliderBar(const TCHAR* TopTexName, const TCHAR* MidTexName,
                         const TCHAR* BottomTexName)
{
    ThumbSize = 0;
    ThumbPos = 0;
    Value = 0;
    Page = 0;
    bDragging = 0;
    ThumbTopTex = FindTexture(TopTexName);
    ThumbMidTex = FindTexture(MidTexName);
    ThumbBottomTex = FindTexture(BottomTexName);
}
NW_HOOK_CTOR_ARGS(0x10021ac0, NCSliderBar, (const TCHAR* A, const TCHAR* B, const TCHAR* C),
                  (A, B, C))

// 0x10021a70
NW_HOOK_DTOR(0x10021a70, NCSliderBar)

// stock body shared with NCButton's (0x100051f0)
INT NCSliderBar::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10021b80 (vslot 50) - thumb only: top cap, bottom cap, stretched middle
INT NCSliderBar::OnPaint(NCGDevice* Device)
{
    guard(NCSliderBar::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
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
NW_HOOK_METHOD(0x10021b80, NCSliderBar, OnPaint)

// 0x100218e0 (vslot 55)
INT NCSliderBar::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & MK_LBUTTON) && bDragging)
    {
        INT Y = (SWORD)HIWORD(lParam) - ScreenY;
        INT Delta = Y - LastY;
        if (Delta > 0)
        {
            if (ThumbPos >= TrackLen)
                return 1;
            ThumbPos += Delta;
            if (ThumbPos > TrackLen)
                ThumbPos = TrackLen;
        }
        else
        {
            if (Delta >= 0 || ThumbPos <= 0)
                return 1;
            ThumbPos += Delta;
            if (ThumbPos < 0)
                ThumbPos = 0;
        }
        Value = (Total - Page) * ThumbPos / TrackLen;
        LastY = Y;
        NotifyParent(Value);
    }
    return 1;
}
NW_HOOK_METHOD(0x100218e0, NCSliderBar, OnMouseMove)

// 0x10021870 (vslot 56) - grabs the thumb; only a bar with something to scroll drags
INT NCSliderBar::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Y = (SWORD)HIWORD(lParam) - ScreenY;
    if (ThumbPos <= Y && Y <= ThumbPos + ThumbSize)
    {
        if (Total > Page)
            bDragging = 1;
        LastY = Y;
    }
    return 1;
}
NW_HOOK_METHOD(0x10021870, NCSliderBar, OnLButtonDown)

// 0x100218c0 (vslot 57)
INT NCSliderBar::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bDragging = 0;
    ReleaseFocus();
    return 1;
}
NW_HOOK_METHOD(0x100218c0, NCSliderBar, OnLButtonUp)

// 0x100217a0 - the thumb size is computed unsigned, as in the stock code
void NCSliderBar::SetRange(INT InTotal, INT InPage)
{
    INT H = Height;
    Total = InTotal;
    Page = InPage;
    if (InTotal > InPage)
        ThumbSize = (DWORD)(H * InPage) / (DWORD)InTotal;
    else
        ThumbSize = H;
    if (ThumbSize < 15)
        ThumbSize = 15;
    TrackLen = H - ThumbSize;
}
NW_HOOK_METHOD(0x100217a0, NCSliderBar, SetRange)

// 0x10021800 - the range test looks at the old Value, not the new one (stock behaviour)
void NCSliderBar::SetValue(INT NewValue)
{
    if (Value > Total || Value < 0)
    {
        Value = 0;
        return;
    }
    Value = NewValue;
    if (Total > Page)
        ThumbPos = TrackLen * NewValue / (Total - Page);
    else
        ThumbPos = 0;
}
NW_HOOK_METHOD(0x10021800, NCSliderBar, SetValue)

// 0x100219b0
void NCSliderBar::Reset()
{
    ThumbPos = 0;
    Value = 0;
    Page = 0;
}
NW_HOOK_METHOD(0x100219b0, NCSliderBar, Reset)

// 0x10021cc0
void NCSliderBar::SetScrollRange(INT InTotal, INT InPage)
{
    SetRange(InTotal, InPage);
}
NW_HOOK_METHOD(0x10021cc0, NCSliderBar, SetScrollRange)

// 0x10021cd0
void NCSliderBar::SetScrollPos(INT NewValue)
{
    SetValue(NewValue);
}
NW_HOOK_METHOD(0x10021cd0, NCSliderBar, SetScrollPos)

NW_CHECK_VTABLE(NCSliderBar, 0x100dd1f8, 63)

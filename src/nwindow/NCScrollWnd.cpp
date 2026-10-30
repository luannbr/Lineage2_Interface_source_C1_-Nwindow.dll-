/*=============================================================================
    NCScrollWnd.cpp - vertically scrolling window. Stock: 0x10020f40 .. 0x10021790,
    plus the destructor (0x100168e0), OnPaint (0x10002830, folded) and OnScrollBar
    (0x100a0900).
=============================================================================*/
#include "nwindow/NCScrollWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"

// 0x100210a0 - the controls and MinScrollY come with OnCreate / SetContentHeight
NCScrollWnd::NCScrollWnd()
{
    ScrollY = 0;
    ScrollX = 0;
    DeltaY = 0;
    DeltaX = 0;
    PageStep = 100;
    LineStep = 15;
    Unknown0C8 = 12;
}
NW_HOOK_CTOR(0x100210a0, NCScrollWnd)

// 0x100168e0
NW_HOOK_DTOR(0x100168e0, NCScrollWnd)

// 0x10020f40 (vslot 24) - its own controls report here
INT NCScrollWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((WPARAM)ScrollUpID == wParam)
            return ScrollUp() != 0;
        if ((WPARAM)ScrollDownID == wParam)
            return ScrollDown() != 0;
        if ((WPARAM)ScrollBarID == wParam)
            return OnScrollBar((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10020f40, NCScrollWnd, WindowProc)

// 0x10021120 (vslot 35) - up / down buttons and the slider along the right edge
INT NCScrollWnd::OnCreate()
{
    NCWnd::OnCreate();
    INT H = LocalH;
    INT W = LocalW;

    ScrollUpButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollUp"),
                                      TEXT("L2UI.NWindow.WindowScrollUp"));
    ScrollUpID = GNextScrollCtrlID++;
    ScrollUpButton->ID = ScrollUpID;
    ScrollUpButton->SetWindowRect(W - 12, 0, 12, 12);
    CreateChild(ScrollUpButton, NWS_VISIBLE);

    ScrollDownButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollDown"),
                                        TEXT("L2UI.NWindow.WindowScrollDown"));
    ScrollDownID = GNextScrollCtrlID++;
    ScrollDownButton->ID = ScrollDownID;
    ScrollDownButton->SetWindowRect(W - 12, H - 12, 12, 12);
    CreateChild(ScrollDownButton, NWS_VISIBLE);

    ScrollBar = new NCSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                TEXT("L2UI.NWindow.WindowScrollBar2"),
                                TEXT("L2UI.NWindow.WindowScrollBar3"));
    ScrollBarID = GNextScrollCtrlID++;
    ScrollBar->ID = ScrollBarID;
    ScrollBar->SetWindowRect(W - 12, 12, 12, H - 24);
    CreateChild(ScrollBar, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10021120, NCScrollWnd, OnCreate)

// 0x10021380 (vslot 36)
INT NCScrollWnd::OnDestroy()
{
    if (ScrollUpButton)
    {
        RemoveChildWindow(ScrollUpButton->Entry);
        delete ScrollUpButton;
        ScrollUpButton = NULL;
    }
    if (ScrollDownButton)
    {
        RemoveChildWindow(ScrollDownButton->Entry);
        delete ScrollDownButton;
        ScrollDownButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        delete ScrollBar;
        ScrollBar = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10021380, NCScrollWnd, OnDestroy)

// 0x10020ff0 (vslot 41) - the controls follow the right edge (size 0 x 0 keeps theirs)
void NCScrollWnd::SetSize(INT W, INT H)
{
    NCWnd::SetSize(W, H);
    ScrollUpButton->SetWindowRect(W - 12, 0, 0, 0);
    ScrollDownButton->SetWindowRect(W - 12, H - 12, 0, 0);
    ScrollBar->SetWindowRect(W - 12, 12, 12, H - 24);
}
NW_HOOK_METHOD(0x10020ff0, NCScrollWnd, SetSize)

// 0x10002830 (vslot 50) - `return 1` shared with NCCandidateBox and others
NW_HOOK_METHOD(0x10002830, NCScrollWnd, OnPaint)

// 0x10021420 (vslot 53) - the base handler runs twice, as in the stock code
INT NCScrollWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    DeltaX = ScrollX;
    INT OldY = ScrollY;
    DeltaY = OldY;
    UBOOL bHandled = 0;
    switch (wParam)
    {
    case VK_PRIOR:
        if (ScrollY < 0)
        {
            ScrollY += PageStep;
            if (ScrollY > 0)
                ScrollY = 0;
        }
        bHandled = 1;
        break;
    case VK_NEXT:
        if (ScrollY > MinScrollY)
        {
            ScrollY -= PageStep;
            if (ScrollY < MinScrollY)
                ScrollY = MinScrollY;
        }
        bHandled = 1;
        break;
    case VK_UP:
        if (ScrollY < 0)
        {
            ScrollY += LineStep;
            if (ScrollY > 0)
                ScrollY = 0;
        }
        bHandled = 1;
        break;
    case VK_DOWN:
        if (ScrollY > MinScrollY)
        {
            ScrollY -= LineStep;
            if (ScrollY < MinScrollY)
                ScrollY = MinScrollY;
        }
        bHandled = 1;
        break;
    }
    if (ScrollY != OldY)
        ScrollBar->SetScrollPos(-ScrollY);
    DeltaY -= ScrollY;
    DeltaX -= ScrollX;
    NCWnd::OnKeyDown(wParam, lParam);
    return NCWnd::OnKeyDown(wParam, lParam) ? 1 : bHandled;
}
NW_HOOK_METHOD(0x10021420, NCScrollWnd, OnKeyDown)

// 0x10021690 (vslot 61) - three lines per wheel notch (120)
INT NCScrollWnd::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    if (Notches > 0)
    {
        if (ScrollY < 0)
        {
            ScrollY += LineStep * Notches * 3;
            if (ScrollY > 0)
                ScrollY = 0;
            ScrollBar->SetScrollPos(-ScrollY);
        }
    }
    else if (ScrollY > MinScrollY)
    {
        ScrollY += LineStep * Notches * 3;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
    }
    DeltaX -= ScrollX;
    DeltaY -= ScrollY;
    return 1;
}
NW_HOOK_METHOD(0x10021690, NCScrollWnd, OnMouseWheel)

// 0x10021560 (vslot 62)
INT NCScrollWnd::ScrollUp()
{
    if (ScrollY < 0)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY += LineStep;
        if (ScrollY > 0)
            ScrollY = 0;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
        return 1;
    }
    DeltaY = 0;
    return 1;
}
NW_HOOK_METHOD(0x10021560, NCScrollWnd, ScrollUp)

// 0x100215f0 (vslot 63)
INT NCScrollWnd::ScrollDown()
{
    if (ScrollY > MinScrollY)
    {
        DeltaX = ScrollX;
        DeltaY = ScrollY;
        ScrollY -= LineStep;
        if (ScrollY < MinScrollY)
            ScrollY = MinScrollY;
        ScrollBar->SetScrollPos(-ScrollY);
        DeltaX -= ScrollX;
        DeltaY -= ScrollY;
        return 1;
    }
    DeltaY = 0;
    return 1;
}
NW_HOOK_METHOD(0x100215f0, NCScrollWnd, ScrollDown)

// 0x100a0900 (vslot 64) - the slider's value is the scroll position
INT NCScrollWnd::OnScrollBar(INT Value)
{
    DeltaX = ScrollX;
    DeltaY = ScrollY;
    ScrollY = -Value;
    DeltaX -= ScrollX;
    DeltaY -= ScrollY;
    return 1;
}
NW_HOOK_METHOD(0x100a0900, NCScrollWnd, OnScrollBar)

// 0x10021050 - MinScrollY = Height - content height (0 when it all fits)
void NCScrollWnd::SetContentHeight(INT ContentHeight)
{
    if (ContentHeight > Height)
    {
        MinScrollY = Height - ContentHeight;
        ScrollBar->SetScrollRange(ContentHeight, Height);
    }
    else
    {
        MinScrollY = 0;
        ScrollBar->SetScrollRange(0, Height);
    }
}
NW_HOOK_METHOD(0x10021050, NCScrollWnd, SetContentHeight)

// 0x100210f0 - the scroll buttons are created without auto-repeat (as NCListBox's)
void NCScrollWnd::SetRepeatDelay(INT Delay)
{
    if (ScrollUpButton && ScrollDownButton)
    {
        ScrollUpButton->RepeatDelay = Delay;
        ScrollDownButton->RepeatDelay = Delay;
    }
}
NW_HOOK_METHOD(0x100210f0, NCScrollWnd, SetRepeatDelay)

NW_CHECK_VTABLE(NCScrollWnd, 0x100dc7e0, 65)

/*=============================================================================
    NCListBox.cpp - scrolling list of text lines. Stock: 0x10017890 .. 0x100186e0.
=============================================================================*/
#include "nwindow/NCListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// 0x100184f0
NCListBox::NCListBox()
{
    BackTex = NULL;
    HighlightTex = NULL;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopIndex = 0;
    NumLines = 0;
    LineGap = 0;
    bAutoScroll = 0;
    TextX = 0;
    TextY = 0;
    MaxLines = 100;
    VisibleLines = 10;
    Unknown1D4 = 12;
    Selected = -1;
}
NW_HOOK_CTOR(0x100184f0, NCListBox)

// 0x10018630 - the third argument is never read (NCDebugWnd passes 16)
NCListBox::NCListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
{
    BackTex = NULL;
    HighlightTex = NULL;
    MaxLines = InMaxLines;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopIndex = 0;
    NumLines = 0;
    LineGap = 0;
    bAutoScroll = 0;
    TextX = 0;
    TextY = 0;
    VisibleLines = InVisibleLines;
    Unknown1D4 = 12;
    Selected = -1;
}
NW_HOOK_CTOR_ARGS(0x10018630, NCListBox, (INT A, INT B, INT C), (A, B, C))

// 0x100185d0
NW_HOOK_DTOR(0x100185d0, NCListBox)

// 0x10017890 (vslot 24) - its own buttons and slider report here
INT NCListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDOWN)
        return OnLButtonDown(wParam, lParam) != 0;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_LIST_SCROLLUP)
            return ScrollUp() != 0;
        if ((INT)wParam == NID_LIST_SCROLLDOWN)
            return ScrollDown() != 0;
        if ((INT)wParam == NID_LIST_SCROLLBAR)
            return SetTopIndex((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10017890, NCListBox, WindowProc)

// 0x10017f30 (vslot 35) - up / down buttons and the slider along the right edge
INT NCListBox::OnCreate()
{
    NControl::OnCreate();
    HighlightTex = FindTexture(TEXT("NWindow.MenuHighlight"));
    INT W = LocalW;
    INT H = LocalH;

    ScrollUpButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollUp"),
                                      TEXT("L2UI.NWindow.WindowScrollUp"));
    ScrollUpButton->ID = NID_LIST_SCROLLUP;
    ScrollUpButton->SetWindowRect(W - 12, 0, 12, 12);
    CreateChild(ScrollUpButton, NWS_VISIBLE);

    ScrollDownButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollDown"),
                                        TEXT("L2UI.NWindow.WindowScrollDown"));
    ScrollDownButton->ID = NID_LIST_SCROLLDOWN;
    ScrollDownButton->SetWindowRect(W - 12, H - 12, 12, 12);
    CreateChild(ScrollDownButton, NWS_VISIBLE);

    ScrollBar = new NCSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                TEXT("L2UI.NWindow.WindowScrollBar2"),
                                TEXT("L2UI.NWindow.WindowScrollBar3"));
    ScrollBar->ID = NID_LIST_SCROLLBAR;
    ScrollBar->SetWindowRect(W - 12, 12, 12, H - 24);
    CreateChild(ScrollBar, NWS_VISIBLE);

    InitScrollBar();
    return 1;
}
NW_HOOK_METHOD(0x10017f30, NCListBox, OnCreate)

// 0x10018170 (vslot 36)
INT NCListBox::OnDestroy()
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
    for (INT i = 0; i < Items.Num(); i++)
    {
        NListItem Item = Items(i);
        if (Item.Text)
            delete Item.Text;
    }
    Items.Empty();
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10018170, NCListBox, OnDestroy)

// 0x10017c30 (vslot 50) - the selected line gets a grey bar and white text
INT NCListBox::OnPaint(NCGDevice* Device)
{
    guard(NCListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Items.Num();
         i++, Line++)
    {
        NListItem Item = Items(i);
        if (i == Selected)
        {
            Device->DrawRect(0, (LineGap + 12) * Line, Width - 18, LineGap + 12, 0xFF646464);
            Device->DrawText(TextX, (LineGap + 12) * Line + TextY, 0xFFFFFFFF, Item.Text->GetAt(0),
                             0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
        else
            Device->DrawText(TextX, (LineGap + 12) * Line + TextY, Item.Text->Color,
                             Item.Text->GetAt(0), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10017c30, NCListBox, OnPaint)

// 0x10017a50 (vslot 56)
INT NCListBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
        NotifyParent(Selected);
    return 1;
}
NW_HOOK_METHOD(0x10017a50, NCListBox, OnLButtonDown)

// 0x10017a90 (vslot 61) - one line per wheel notch (120)
INT NCListBox::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    INT Delta = Notches;
    if (Notches > 0)
    {
        if (TopIndex > Delta)
            TopIndex -= Delta;
        else
            TopIndex = 0;
    }
    else if (NumLines >= VisibleLines)
    {
        if (NumLines - TopIndex + Delta > VisibleLines)
            TopIndex -= Delta;
        else
            TopIndex = NumLines - VisibleLines;
    }
    else
    {
        if (NumLines - TopIndex + Delta > VisibleLines)
            TopIndex -= Delta;
        else
            TopIndex = 0;
    }
    ScrollBar->SetValue(TopIndex);
    return 1;
}
NW_HOOK_METHOD(0x10017a90, NCListBox, OnMouseWheel)

// 0x10017980 (vslot 63) - selects the line under the point; returns Selected (unchanged when
// no line is hit) or -1 outside the box
INT NCListBox::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    if (RX < 0 || RX > Width || RY < 0 || RY > Height)
        return -1;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex; i++, Line++)
    {
        if (i >= NumLines)
            break;
        if (RX < Width && RY >= (LineGap + 12) * Line + TextY &&
            RY < (Line + 1) * (LineGap + 12) + TextY)
        {
            Selected = i;
            break;
        }
    }
    return Selected;
}
NW_HOOK_METHOD(0x10017980, NCListBox, HitTest)

// 0x10018310 (vslot 64) - when full, the oldest line is dropped
void NCListBox::AddString(const TCHAR* Text, DWORD Color, INT Param)
{
    NString* Str = new NString(Text);
    Str->Color = Color;
    if (NumLines < MaxLines)
    {
        NumLines++;
        if (bAutoScroll && NumLines > VisibleLines)
            TopIndex++;
    }
    else
    {
        NString* Oldest = Items(0).Text;
        if (Oldest)
        {
            delete Oldest;
            Items(0).Text = NULL;
        }
        Items.Remove(0, 1);
    }
    INT i = Items.Add();
    Items(i).Text = Str;
    Items(i).Param = Param;
    UpdateScrollBar();
}
NW_HOOK_METHOD(0x10018310, NCListBox, AddString)

// 0x10018270 (vslot 65)
void NCListBox::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Text = Items(i).Text;
        if (Text)
            delete Text;
    }
    Items.Empty();
    ScrollBar->Reset();
    TopIndex = 0;
    NumLines = 0;
    Selected = -1;
}
NW_HOOK_METHOD(0x10018270, NCListBox, Clear)

// 0x100098f0 (vslot 66) - folded `return 1`, hooked as NCIcon::OnLButtonDblClick

// 0x10017b40 (vslot 67)
INT NCListBox::ScrollUp()
{
    if (NumLines > VisibleLines)
    {
        if (--TopIndex < 0)
            TopIndex = 0;
        ScrollBar->SetValue(TopIndex);
    }
    return 1;
}
NW_HOOK_METHOD(0x10017b40, NCListBox, ScrollUp)

// 0x10017b80 (vslot 68)
INT NCListBox::ScrollDown()
{
    if (NumLines > VisibleLines)
    {
        INT Last = NumLines - VisibleLines;
        if (++TopIndex > Last)
            TopIndex = Last;
        ScrollBar->SetValue(TopIndex);
    }
    return 1;
}
NW_HOOK_METHOD(0x10017b80, NCListBox, ScrollDown)

// 0x10017bd0 (vslot 69) - from the slider
INT NCListBox::SetTopIndex(INT Index)
{
    TopIndex = Index;
    return 1;
}
NW_HOOK_METHOD(0x10017bd0, NCListBox, SetTopIndex)

// 0x10017bf0
void NCListBox::UpdateScrollBar()
{
    NumLines = Items.Num();
    ScrollBar->SetRange(Items.Num(), VisibleLines);
    ScrollBar->SetValue(TopIndex);
}
NW_HOOK_METHOD(0x10017bf0, NCListBox, UpdateScrollBar)

// 0x10017e20 - the scroll buttons are created without auto-repeat, so this has no effect
void NCListBox::SetRepeatDelay(INT Delay)
{
    if (ScrollUpButton && ScrollDownButton)
    {
        ScrollUpButton->RepeatDelay = Delay;
        ScrollDownButton->RepeatDelay = Delay;
    }
}
NW_HOOK_METHOD(0x10017e20, NCListBox, SetRepeatDelay)

// 0x10017e50 - the range uses TopIndex as the total (0 at creation), as in the stock code
void NCListBox::InitScrollBar()
{
    SetRepeatDelay(200);
    ScrollBar->SetRange(TopIndex, VisibleLines);
    ScrollBar->SetValue(TopIndex);
}
NW_HOOK_METHOD(0x10017e50, NCListBox, InitScrollBar)

// 0x10017e90
INT NCListBox::GetItemParam(INT Index)
{
    if (Index >= Items.Num())
        return -1;
    return Items(Index).Param;
}
NW_HOOK_METHOD(0x10017e90, NCListBox, GetItemParam)

NW_CHECK_VTABLE(NCListBox, 0x100dcc68, 70)

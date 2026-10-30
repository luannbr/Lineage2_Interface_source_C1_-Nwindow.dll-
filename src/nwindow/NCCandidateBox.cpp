/*=============================================================================
    NCCandidateBox.cpp - IME candidate list. Stock: 0x100025a0 .. 0x100029d2,
    0x10002ac0 .. 0x1000336f, 0x10003a50 .. 0x10003afd, 0x10003b00 .. 0x10003d61 (the
    TArray<NImeText> destructor at 0x10003bb0 is the template's).
=============================================================================*/
#include "nwindow/NCCandidateBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"

// 0x10003c50
NCCandidateBox::NCCandidateBox(INT InPageSize, INT InRowHeight)
{
    Selected = 0;
    PageStart = 0;
    UnknownE0 = 0;
    UnknownE8 = 0;
    Mode = 0;
    PageSize = InPageSize;
    RowHeight = InRowHeight;
    TextColor = 0xFF000000;
}
NW_HOOK_CTOR_ARGS(0x10003c50, NCCandidateBox, (INT A, INT B), (A, B))

// 0x10003cf0
NW_HOOK_DTOR(0x10003cf0, NCCandidateBox)

// 0x100025a0 (vslot 24)
INT NCCandidateBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDOWN)
        return OnLButtonDown(wParam, lParam) != 0;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_CANDIDATE_SCROLLBAR)
            return OnScrollBar((INT)lParam) != 0;
        if ((INT)wParam == NID_CANDIDATE_UP)
            return OnUpButton() != 0;
        if ((INT)wParam == NID_CANDIDATE_DOWN)
            return OnDownButton() != 0;
        return 0;
    }
    if (Msg == WM_LBUTTONUP)
        return OnLButtonUp(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100025a0, NCCandidateBox, WindowProc)

// 0x10002ac0 (vslot 35) - scroll controls just outside the right edge (hidden by ShowWindow)
INT NCCandidateBox::OnCreate()
{
    NCWnd::OnCreate();
    LeftEnable1 = FindTexture(TEXT("L2UI.CandidateWnd.left_enable1"));
    LeftDisable1 = FindTexture(TEXT("L2UI.CandidateWnd.left_disable1"));
    LeftEnable2 = FindTexture(TEXT("L2UI.CandidateWnd.left_enable2"));
    LeftDisable2 = FindTexture(TEXT("L2UI.CandidateWnd.left_disable2"));
    Center1 = FindTexture(TEXT("L2UI.CandidateWnd.center1"));
    Center2 = FindTexture(TEXT("L2UI.CandidateWnd.center2"));
    Right1 = FindTexture(TEXT("L2UI.CandidateWnd.right1"));
    Right2 = FindTexture(TEXT("L2UI.CandidateWnd.right2"));
    WhiteTex = FindTexture(TEXT("NWindow.WhiteTexture"));

    UpButton = new NCPushButton(TEXT("NWindow.MenuHighlight"), TEXT("NWindow.BlackTexture"));
    UpButton->ID = NID_CANDIDATE_UP;
    UpButton->SetWindowRect(Width, 0, 15, 15);
    CreateChild(UpButton, NWS_VISIBLE);
    UpButton->RepeatDelay = 180;

    DownButton = new NCPushButton(TEXT("NWindow.MenuHighlight"), TEXT("NWindow.BlackTexture"));
    DownButton->ID = NID_CANDIDATE_DOWN;
    DownButton->SetWindowRect(Width, Height - 15, 15, 15);
    CreateChild(DownButton, NWS_VISIBLE);
    DownButton->RepeatDelay = 180;

    ScrollBar = new NCSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                TEXT("L2UI.NWindow.WindowScrollBar2"),
                                TEXT("L2UI.NWindow.WindowScrollBar3"));
    ScrollBar->ID = NID_CANDIDATE_SCROLLBAR;
    ScrollBar->SetWindowRect(Width, 15, 15, Height - 30);
    CreateChild(ScrollBar, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10002ac0, NCCandidateBox, OnCreate)

// 0x10003b00 (vslot 36)
INT NCCandidateBox::OnDestroy()
{
    if (DownButton)
    {
        RemoveChildWindow(DownButton->Entry);
        delete DownButton;
        DownButton = NULL;
    }
    if (UpButton)
    {
        RemoveChildWindow(UpButton->Entry);
        delete UpButton;
        UpButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        delete ScrollBar;
        ScrollBar = NULL;
    }
    Items.Empty();
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10003b00, NCCandidateBox, OnDestroy)

// 0x10002840 (vslot 40) - NCWnd::PtInRect with the far edges compared unsigned
UBOOL NCCandidateBox::PtInRect(INT X, INT Y, INT RectX, INT RectY, INT RectW, INT RectH)
{
    if (X < RectX || (DWORD)X > (DWORD)(RectW + RectX))
        return 0;
    if (Y < RectY || (DWORD)Y > (DWORD)(RectH + RectY))
        return 0;
    return 1;
}
NW_HOOK_METHOD(0x10002840, NCCandidateBox, PtInRect)

// 0x10002880 (vslot 41) - at least 70 px wide (unsigned test); LocalW keeps the width asked
// for. The scroll controls follow the right edge.
void NCCandidateBox::SetSize(INT W, INT H)
{
    INT OldWidth = Width;
    if ((DWORD)W < 70)
        Width = 70;
    else
        Width = W;
    LocalW = W;
    Height = H;
    LocalH = H;
    ScrollBar->SetSize(15, H - 30);
    ScrollBar->MoveWindow(Width - OldWidth, 0);
    UpButton->MoveWindow(Width - OldWidth, 0);
    DownButton->MoveWindow(Width - OldWidth, 0);
}
NW_HOOK_METHOD(0x10002880, NCCandidateBox, SetSize)

// 0x100029a0 (vslot 42) - the scroll controls are never shown
void NCCandidateBox::ShowWindow()
{
    NCWnd::ShowWindow();
    UpButton->HideWindow();
    DownButton->HideWindow();
    ScrollBar->HideWindow();
}
NW_HOOK_METHOD(0x100029a0, NCCandidateBox, ShowWindow)

// 0x10002d90 (vslot 50) - frame for the mode (rows past Count get the "disable" strip), the
// selection bar, then the page. The text of row i is Items(PageStart + i) with i already
// counted from PageStart, as in the stock code, so pages after the first show the wrong
// candidates (or none).
INT NCCandidateBox::OnPaint(NCGDevice* Device)
{
    guard(NCCandidateBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    switch (Mode)
    {
    case 0:
    case 1:
    case 2:
        if (LeftEnable1)
            Device->DrawTexture(0, 0, 11, 136, 0, 0, 11, 136, LeftEnable1);
        if (Count < PageSize && LeftDisable1)
            Device->DrawTexture(1, RowHeight * Count + 1, 9, (PageSize - Count) * RowHeight, 0,
                                RowHeight * Count, 9, (PageSize - Count) * RowHeight, LeftDisable1);
        if (Center1)
            Device->DrawTexture(11, 0, Width - 15, 136, 0, 0, 12, 136, Center1);
        if (Right1)
            Device->DrawTexture(Width - 4, 0, 4, 136, 0, 0, 4, 136, Right1);
        break;
    case 3:
    case 4:
        if (LeftEnable2)
            Device->DrawTexture(0, 0, 11, 151, 0, 0, 11, 151, LeftEnable2);
        if (Count < PageSize && LeftDisable2)
            Device->DrawTexture(1, RowHeight * Count + 1, 9, (PageSize - Count) * RowHeight, 0,
                                RowHeight * Count, 9, (PageSize - Count) * RowHeight, LeftDisable2);
        if (Center2)
            Device->DrawTexture(11, 0, Width - 15, 151, 0, 0, 12, 136, Center2);
        if (Right2)
            Device->DrawTexture(Width - 4, 0, 4, 151, 0, 0, 4, 136, Right2);
        break;
    case 5:
        if (LeftEnable2)
        {
            Device->DrawTexture(0, 0, 11, 1, 0, 0, 11, 1, LeftEnable2);
            Device->DrawTexture(0, 1, 11, 135, 0, 16, 11, 135, LeftEnable2);
            Device->DrawTexture(0, 135, 11, 15, 0, 1, 11, 15, LeftEnable2);
        }
        if (Count < PageSize && LeftDisable2)
        {
            Device->DrawTexture(1, RowHeight * Count + 1, 9, (PageSize - Count - 1) * RowHeight, 0,
                                (Count + 1) * RowHeight, 9, (PageSize - Count - 1) * RowHeight,
                                LeftDisable2);
            Device->DrawTexture(1, (PageSize - 1) * RowHeight + 1, 9, RowHeight, 0, 0, 9, RowHeight,
                                LeftDisable2);
        }
        if (Center2)
            Device->DrawTexture(11, 0, Width - 15, 151, 0, 0, 12, 151, Center2);
        if (Right2)
            Device->DrawTexture(Width - 4, 0, 4, 151, 0, 0, 4, 151, Right2);
        break;
    }

    INT Line = 0;
    for (INT i = PageStart; i < PageStart + PageSize && i <= Count; i++, Line++)
    {
        INT Index = PageStart + i;
        if (i == Selected)
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Items(Index).Text, 0, 0, 0, 0, 0);
            Device->DrawTexture(12, Selected % PageSize * RowHeight, Size.cx + 3, RowHeight, 0, 0,
                                Size.cx, RowHeight, WhiteTex);
            if (Items.Num() > PageStart + i)
                Device->DrawText(14, RowHeight * Line + 2, TextColor, Items(PageStart + i).Text, 0,
                                 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
        else if (Items.Num() > Index)
        {
            Device->DrawText(14, RowHeight * Line + 2, 0xFFFFFFFF, Items(Index).Text, 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10002d90, NCCandidateBox, OnPaint)

// 0x10002710 (vslot 53)
INT NCCandidateBox::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case VK_NEXT:
        if (IsShowWindow())
            HideWindow();
        else
            ShowWindow();
        break;
    case VK_PRIOR:
        if (IsShowWindow())
            HideWindow();
        else
            ShowWindow();
        break;
    case VK_UP:
        SelectPrev();
        break;
    case VK_DOWN:
        SelectNext();
        break;
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10002710, NCCandidateBox, OnKeyDown)

// 0x10003270 (vslot 56) - selects the row under the cursor (16-bit coordinates as in the
// stock code); the click is never passed on
INT NCCandidateBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    _WORD X = (_WORD)(LOWORD(lParam) - (_WORD)ScreenX);
    _WORD Y = (_WORD)(HIWORD(lParam) - (_WORD)ScreenY);
    INT Row = 0;
    for (INT i = PageStart; i < PageStart + PageSize; i++, Row++)
    {
        if (i >= Count)
            break;
        if (PtInRect(X, Y, MarginX, RowHeight * Row + MarginY, Width - 2 * MarginX, RowHeight))
        {
            Selected = i;
            break;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10003270, NCCandidateBox, OnLButtonDown)

// 0x10002800 (vslot 57)
INT NCCandidateBox::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsShowWindow())
        HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x10002800, NCCandidateBox, OnLButtonUp)

// 0x100027c0 (vslot 62)
INT NCCandidateBox::OnUpButton()
{
    SelectPrev();
    ScrollBar->SetValue(Selected);
    return 1;
}
NW_HOOK_METHOD(0x100027c0, NCCandidateBox, OnUpButton)

// 0x100027e0 (vslot 63)
INT NCCandidateBox::OnDownButton()
{
    SelectNext();
    ScrollBar->SetValue(Selected);
    return 1;
}
NW_HOOK_METHOD(0x100027e0, NCCandidateBox, OnDownButton)

// 0x10002690 - moving up past the top of a page shows the previous page
void NCCandidateBox::SelectPrev()
{
    if (Selected <= 0 || --Selected == 0)
        PageStart = 0;
    else if (Selected % PageSize == PageSize - 1)
        PageStart = Selected - PageSize + 1;
}
NW_HOOK_METHOD(0x10002690, NCCandidateBox, SelectPrev)

// 0x100026e0 - moving down past the bottom of a page shows the next one
void NCCandidateBox::SelectNext()
{
    INT Next = Selected + 1;
    if (Next < Count)
    {
        Selected = Next;
        if (Next % PageSize == 0)
            PageStart = Next;
    }
}
NW_HOOK_METHOD(0x100026e0, NCCandidateBox, SelectNext)

// 0x10002930 - the scroll bar over Total candidates, a page of 9, at Value
void NCCandidateBox::SetScroll(INT Total, INT Value)
{
    ScrollBar->SetRange(Total, 9);
    ScrollBar->SetValue(Value);
}
NW_HOOK_METHOD(0x10002930, NCCandidateBox, SetScroll)

// 0x10002960 - the frame layout; modes 2..4 make the box 150 pixels high, the others 135
void NCCandidateBox::SetMode(INT InMode)
{
    Mode = InMode;
    if (InMode >= 2 && InMode <= 4)
        SetSize(100, 150);
    else
        SetSize(100, 135);
}
NW_HOOK_METHOD(0x10002960, NCCandidateBox, SetMode)

// 0x10003a50 - a copy of Text as the last candidate; the flag given is not stored (the entry's
// is 0)
void NCCandidateBox::AddItem(const TCHAR* Text, INT Flag)
{
    NImeText Item;
    Item.Text[0] = 0;
    Item.Flag = 0;
    if (Text)
        wcscpy(Item.Text, Text);
    Items(Items.Add()) = Item;
    Count++;
}
NW_HOOK_METHOD(0x10003a50, NCCandidateBox, AddItem)

// 0x10003ad0
void NCCandidateBox::Clear()
{
    Items.Empty();
    Count = 0;
}
NW_HOOK_METHOD(0x10003ad0, NCCandidateBox, Clear)

NW_CHECK_VTABLE(NCCandidateBox, 0x100d8938, 65)

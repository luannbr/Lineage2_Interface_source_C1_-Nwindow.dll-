/*=============================================================================
    NCChatListBox.cpp - chat message list. Stock: 0x10037630 .. 0x100376bf,
    0x10038580 .. 0x100387b5, 0x1003aa80 .. 0x1003acdd, 0x1003cb50 .. 0x1003cc14.
=============================================================================*/
#include "nwindow/NCChatListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"
#include "nwindow/NListWrap.h"

// 0x1003cb50
NCChatListBox::NCChatListBox()
{
}
NW_HOOK_CTOR(0x1003cb50, NCChatListBox)

// 0x1003cbf0 - the one NCChatWnd creates (200 lines, 10 visible)
NCChatListBox::NCChatListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x1003cbf0, NCChatListBox, (INT A, INT B, INT C), (A, B, C))

// 0x1003cba0
NW_HOOK_DTOR(0x1003cba0, NCChatListBox)

// stock body shared with NCDebugWnd's (0x100576d0)
INT NCChatListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10038580 (vslot 35) - scroll controls on the left, text after them
INT NCChatListBox::OnCreate()
{
    NCListBox::OnCreate();
    ScrollUpButton->SetWindowRect(0, 0, 12, 12);
    ScrollDownButton->SetWindowRect(0, Height - 12, 12, 12);
    ScrollBar->SetWindowRect(0, 12, 12, Height - 24);
    TextX = 23;
    TextY = 4;
    return 1;
}
NW_HOOK_METHOD(0x10038580, NCChatListBox, OnCreate)

// 0x100385f0 (vslot 50) - the visible line count follows the height; each line is drawn at
// TextX + its indent. GetAt(0) is called twice per line, as in the stock code.
INT NCChatListBox::OnPaint(NCGDevice* Device)
{
    guard(NCChatListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    INT Visible = (DWORD)Height / 12;
    if (Visible != VisibleLines)
    {
        VisibleLines = Visible;
        if (Items.Num() >= Visible)
            TopIndex = Items.Num() - Visible;
        ScrollBar->SetRange(NumLines, Visible);
        ScrollBar->SetValue(TopIndex);
    }
    ScrollBar->SetRange(NumLines, VisibleLines);
    ScrollBar->SetValue(TopIndex);
    if (Items.Num() < VisibleLines)
        TopIndex = 0;
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Items.Num();
         i++, Line++)
    {
        NString* Text = Items(i).Text;
        Text->GetAt(0);
        DWORD Color = Text->Color;
        INT Indent = Text->Indent;
        Device->DrawText(TextX + Indent, (LineGap + 12) * Line + TextY, Color, Text->GetAt(0), 0, 0,
                         0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100385f0, NCChatListBox, OnPaint)

// 0x1003aa80 (vslot 64) - wrapped, the view follows the last line (see NListWrap.h)
void NCChatListBox::AddString(const TCHAR* Text, DWORD Color, INT Param)
{
    NAddWrappedString(this, Text, Color, Param);
}
NW_HOOK_METHOD(0x1003aa80, NCChatListBox, AddString)

// 0x10037630 (vslot 70) - petitions and announcements always show; system messages have their
// own switches
UBOOL NCChatListBox::IsChannelShown(INT Type, INT SubType)
{
    UBOOL bShow;
    switch (Type)
    {
    case 0:  bShow = Filter.bNormal; break;
    case 1:  bShow = Filter.bShout;  break;
    case 2:  bShow = Filter.bTell;   break;
    case 3:  bShow = Filter.bParty;  break;
    case 4:  bShow = Filter.bPledge; break;
    case 8:  bShow = Filter.bMarket; break;
    case 9:  bShow = Filter.bAlly;   break;
    case 6:
    case 7:
    case 10:
        return 1;
    case 5:
        if (SubType == 3)
            return Filter.bDamage && Filter.bSystem;
        if (SubType == 6)
            return 1;
        return Filter.bSystem != 0;
    default:
        return 0;
    }
    return bShow && Filter.bChat;
}
NW_HOOK_METHOD(0x10037630, NCChatListBox, IsChannelShown)

NW_CHECK_VTABLE(NCChatListBox, 0x100e0520, 71)

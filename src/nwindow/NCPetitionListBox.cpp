/*=============================================================================
    NCPetitionListBox.cpp - petition conversation list. Stock: 0x100947e0 .. 0x10094eef.
=============================================================================*/
#include "nwindow/NCPetitionListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"
#include "nwindow/NListWrap.h"

// 0x10094e40
NCPetitionListBox::NCPetitionListBox()
{
}
NW_HOOK_CTOR(0x10094e40, NCPetitionListBox)

// 0x10094ee0 - the petition windows' (200 lines kept, 16 seen)
NCPetitionListBox::NCPetitionListBox(INT InMaxLines, INT InVisibleLines, INT Unused)
    : NCListBox(InMaxLines, InVisibleLines, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x10094ee0, NCPetitionListBox, (INT A, INT B, INT C), (A, B, C))

// 0x10094e90
NW_HOOK_DTOR(0x10094e90, NCPetitionListBox)

// stock body shared with NCDebugWnd's (0x100576d0)
INT NCPetitionListBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCListBox::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100947e0 (vslot 35) - scroll controls on the right, text from the top left corner
INT NCPetitionListBox::OnCreate()
{
    NCListBox::OnCreate();
    ScrollUpButton->SetWindowRect(Width - 12, 0, 12, 12);
    ScrollDownButton->SetWindowRect(Width - 12, Height - 12, 12, 12);
    ScrollBar->SetWindowRect(Width - 12, 12, 12, Height - 24);
    TextX = 0;
    TextY = 0;
    return 1;
}
NW_HOOK_METHOD(0x100947e0, NCPetitionListBox, OnCreate)

// 0x10094860 (vslot 50) - NCChatListBox::OnPaint without the height-driven line count; the
// guard keeps the chat's name, as in the stock code
INT NCPetitionListBox::OnPaint(NCGDevice* Device)
{
    guard(NCChatListBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    INT Line = 0;
    for (INT i = TopIndex; i < VisibleLines + TopIndex && i <= NumLines && i < Items.Num();
         i++, Line++)
    {
        NListItem Item = Items(i);
        DWORD Color = Item.Text->Color;
        INT Indent = Item.Text->Indent;
        Device->DrawText(TextX + Indent, (LineGap + 12) * Line + TextY, Color, Item.Text->GetAt(0),
                         0, 0, 0, L2FONT_Normal, 1, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10094860, NCPetitionListBox, OnPaint)

// 0x10094be0 (vslot 64) - same body as NCChatListBox::AddString (see NListWrap.h)
void NCPetitionListBox::AddString(const TCHAR* Text, DWORD Color, INT Param)
{
    NAddWrappedString(this, Text, Color, Param);
}
NW_HOOK_METHOD(0x10094be0, NCPetitionListBox, AddString)

NW_CHECK_VTABLE(NCPetitionListBox, 0x100ec718, 70)

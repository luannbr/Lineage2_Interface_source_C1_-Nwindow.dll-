/*=============================================================================
    NCComboBox.cpp - drop-down list. Stock: 0x10004300 .. 0x10004e8d.
=============================================================================*/
#include "nwindow/NCComboBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// 0x10004d70 - the textures come in OnCreate
NCComboBox::NCComboBox()
{
    TextX = 5;
    TextY = 4;
    HighlightTex = NULL;
    DropButton = NULL;
    bDropped = 0;
    Selected = -1;
    Hover = -1;
}
NW_HOOK_CTOR(0x10004d70, NCComboBox)

// 0x10004e20
NW_HOOK_DTOR(0x10004e20, NCComboBox)

// 0x10004300 (vslot 24) - the drop button reports here
INT NCComboBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_COMBO_DROP)
        return DropDown() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10004300, NCComboBox, WindowProc)

// 0x10004450 (vslot 29) - closes the list
void NCComboBox::KillFocus()
{
    if (bDropped)
    {
        SetSize(ClosedW, ClosedH);
        bDropped = 0;
    }
    NCWnd::KillFocus();
}
NW_HOOK_METHOD(0x10004450, NCComboBox, KillFocus)

// 0x10004500 (vslot 35)
INT NCComboBox::OnCreate()
{
    NControl::OnCreate();
    if (!HighlightTex)
        HighlightTex = FindTexture(TEXT("NWindow.MenuHighlight"));
    LeftTex = FindTexture(TEXT("L2UI.Control.Combobox1"));
    MidTex = FindTexture(TEXT("L2UI.Control.Combobox2"));
    LeftDisabledTex = FindTexture(TEXT("L2UI.Control.Combobox1_unable"));
    MidDisabledTex = FindTexture(TEXT("L2UI.Control.Combobox2_unable"));
    ListLeftTex = FindTexture(TEXT("L2UI.Control.Combobox_under1_1"));
    ListMidTex = FindTexture(TEXT("L2UI.Control.Combobox_under1_2"));
    ListRightTex = FindTexture(TEXT("L2UI.Control.Combobox_under1_3"));
    ListBottomTex = FindTexture(TEXT("L2UI.Control.Combobox_under2"));
    NCGDevice::GetTextExtent(&RowSize, TEXT("\xAC00"), 0, 0, 0, 0, 0);
    ClosedW = LocalW;
    ClosedH = LocalH;
    DropButton = new NCButton(TEXT("L2UI.Control.ComboBox3"), TEXT("L2UI.Control.ComboBox3"), 0);
    DropButton->ID = NID_COMBO_DROP;
    DropButton->SetWindowRect(ClosedW - 15, 0, 15, 15);
    CreateChild(DropButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10004500, NCComboBox, OnCreate)

// 0x10004a80 (vslot 36)
INT NCComboBox::OnDestroy()
{
    if (DropButton)
    {
        RemoveChildWindow(DropButton->Entry);
        delete DropButton;
        DropButton = NULL;
    }
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Item = Items(i);
        if (Item)
            delete Item;
    }
    Items.Empty();
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10004a80, NCComboBox, OnDestroy)

// 0x100046a0 (vslot 50) - clip first, origin after; the open list is drawn only while the
// control has the focus
INT NCComboBox::OnPaint(NCGDevice* Device)
{
    guard(NCComboBox::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (IsEnabled())
    {
        if (LeftTex)
            Device->DrawTexture(0, 0, 15, 15, 0, 0, 15, 15, LeftTex);
        if (MidTex)
            Device->DrawTexture(1, 0, Width, 15, 0, 0, 15, 15, MidTex);
        if (Items.Num() > Selected && Selected >= 0)
            Device->DrawText(TextX, TextY - 2, 0xFFFFFFFF, Items(Selected)->GetAt(0), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        if (LeftDisabledTex)
            Device->DrawTexture(0, 0, 15, 15, 0, 0, 15, 15, LeftDisabledTex);
        if (MidDisabledTex)
            Device->DrawTexture(1, 0, Width, 15, 0, 0, 15, 15, MidDisabledTex);
        if (Items.Num() > Selected && Selected >= 0)
            Device->DrawText(TextX, TextY - 2, 0xFF7E7E7E, Items(Selected)->GetAt(0), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    INT BottomY = 0;
    if (bDropped && IsFocused())
    {
        for (INT i = 0; i < Items.Num() + 1; i++)
        {
            if (ListLeftTex)
                Device->DrawTexture(0, RowSize.cy * i + 15, 8, 12, 0, 0, 8, 12, ListLeftTex);
            if (ListMidTex)
                Device->DrawTexture(8, RowSize.cy * i + 15, Width - 16, 12, 0, 0, 8, 12,
                                    ListMidTex);
            if (ListRightTex)
                Device->DrawTexture(Width - 8, RowSize.cy * i + 15, 8, 12, 0, 0, 8, 12,
                                    ListRightTex);
            BottomY = RowSize.cy * i + ClosedH - 3;
        }
        for (INT i = 0; i < Items.Num(); i++)
        {
            if (Hover == i)
                Device->DrawTexture(3, RowSize.cy * i + ClosedH, ClosedW - 6, 12, 0, 0,
                                    ClosedW - 6, 12, HighlightTex);
            Device->DrawText(TextX, RowSize.cy * i + ClosedH, 0xFFFFFFFF, Items(i)->GetAt(0), 0, 0,
                             0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        }
        if (ListBottomTex)
            Device->DrawTexture(0, BottomY + 12, ClosedW, 1, 0, 0, ClosedW, 1, ListBottomTex);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100046a0, NCComboBox, OnPaint)

// 0x10004370 (vslot 55) - the row under the mouse in the open list; the arithmetic is
// 16-bit and unsigned where the stock code has it so
INT NCComboBox::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    _WORD RX = (_WORD)(LOWORD(lParam) - (_WORD)ScreenX);
    _WORD RY = (_WORD)(HIWORD(lParam) - (_WORD)ScreenY);
    if (RX != 0 && (DWORD)RX < (DWORD)Width)
    {
        if ((INT)RY > ClosedH && (DWORD)RY < (DWORD)Height)
        {
            Hover = ((INT)RY - ClosedH) / RowSize.cy;
            if (Hover >= Items.Num())
                Hover = Items.Num() - 1;
            return 1;
        }
    }
    Hover = -1;
    return 1;
}
NW_HOOK_METHOD(0x10004370, NCComboBox, OnMouseMove)

// 0x100043e0 (vslot 56) - open it, or pick the row under the mouse
INT NCComboBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsEnabled())
    {
        if (!bDropped)
        {
            DropDown();
            return 1;
        }
        if (Hover >= 0)
        {
            Selected = Hover;
            NotifyParent(Hover);
            bDropped = 0;
            DropDown();
        }
        KillFocus();
    }
    return 1;
}
NW_HOOK_METHOD(0x100043e0, NCComboBox, OnLButtonDown)

// 0x10004a50 (vslot 63) - only opens (KillFocus closes)
INT NCComboBox::DropDown()
{
    if (IsEnabled())
    {
        if (!bDropped)
            bDropped = 1;
        UpdateSize();
    }
    return 1;
}
NW_HOOK_METHOD(0x10004a50, NCComboBox, DropDown)

// 0x10004490
void NCComboBox::SetSelected(INT Index)
{
    Selected = Index;
}
NW_HOOK_METHOD(0x10004490, NCComboBox, SetSelected)

// 0x100044a0 - the open list is part of the control: it grows to hold every row
void NCComboBox::UpdateSize()
{
    if (bDropped)
    {
        SetSize(ClosedW, (RowSize.cy + TextY) * Items.Num() + ClosedH + TextY + 4);
        SetFocus();
    }
    else
        SetSize(ClosedW, ClosedH);
}
NW_HOOK_METHOD(0x100044a0, NCComboBox, UpdateSize)

// 0x10004a20
const TCHAR* NCComboBox::GetItemText(INT Index)
{
    if (Index >= 0 && Index < Items.Num())
        return Items(Index)->GetAt(0);
    return NULL;
}
NW_HOOK_METHOD(0x10004a20, NCComboBox, GetItemText)

// 0x10004b20
void NCComboBox::ClearItems()
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Item = Items(i);
        if (Item)
            delete Item;
    }
    Items.Empty();
    Selected = -1;
    Hover = -1;
}
NW_HOOK_METHOD(0x10004b20, NCComboBox, ClearItems)

// 0x10004b90
void NCComboBox::AddItem(INT SysStringID)
{
    if (*GL2GameData.SysString[SysStringID])
        Items.AddItem(new NString(*GL2GameData.SysString[SysStringID]));
}
NW_HOOK_METHOD_SIG(0x10004b90, NCComboBox, AddItem, void (NCComboBox::*)(INT))

// 0x10004c50
void NCComboBox::AddItem(const TCHAR* Text)
{
    if (Text && *Text)
        Items.AddItem(new NString(Text));
}
NW_HOOK_METHOD_SIG(0x10004c50, NCComboBox, AddItem, void (NCComboBox::*)(const TCHAR*))

NW_CHECK_VTABLE(NCComboBox, 0x100d8f18, 64)

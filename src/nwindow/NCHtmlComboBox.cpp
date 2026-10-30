/*=============================================================================
    NCHtmlComboBox.cpp - the combo box of the HTML pages. Stock: 0x10028960 .. 0x100289ff,
    0x1002fcf0 .. 0x1002fe43, 0x10030230 .. 0x100305e5, 0x10033300 .. 0x10033538 (ShowWindow
    and HideWindow are bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCHtmlComboBox.h"
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"

// ';'-separated tokens of a list (the stock helper, name ours): *Token = the next one, cut out
// of Str in place; returns where the following one starts
static TCHAR* NextListToken(TCHAR* Str, TCHAR** Token)
{
    while (*Str == ';')
        Str++;
    if (*Str)
    {
        *Token = Str;
        while (*Str && *Str != ';')
            Str++;
        if (*Str)
        {
            *Str = 0;
            Str++;
        }
    }
    return Str;
}
NW_HOOK_FUNC(0x1002fd30, NextListToken)

// 0x10028960
NCHtmlComboBox::NCHtmlComboBox()
{
}
NW_HOOK_CTOR(0x10028960, NCHtmlComboBox)

// 0x100289b0
NW_HOOK_DTOR(0x100289b0, NCHtmlComboBox)

// 0x1002fcf0 (vslot 24)
INT NCHtmlComboBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCComboBox::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1002fcf0, NCHtmlComboBox, WindowProc)

// 0x10033300 (vslot 35) - NCComboBox::OnCreate with the drop button an NCHtmlButton in its own
// field
INT NCHtmlComboBox::OnCreate()
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
    HtmlDropButton = new NCHtmlButton(TEXT("L2UI.Control.ComboBox3"), TEXT("L2UI.Control.ComboBox3"));
    HtmlDropButton->ID = NID_COMBO_DROP;
    HtmlDropButton->SetWindowRect(ClosedW - 15, 0, 15, 15);
    CreateChild(HtmlDropButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10033300, NCHtmlComboBox, OnCreate)

// 0x100334a0 (vslot 36)
INT NCHtmlComboBox::OnDestroy()
{
    if (HtmlDropButton)
    {
        RemoveChildWindow(HtmlDropButton->Entry);
        delete HtmlDropButton;
        HtmlDropButton = NULL;
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
NW_HOOK_METHOD(0x100334a0, NCHtmlComboBox, OnDestroy)

// stock body shared with NCEditBox's (0x100a5d70)
void NCHtmlComboBox::ShowWindow()
{
    NCWnd::ShowWindow();
}

// stock body shared with NCChatWnd's (0x1003dab0)
void NCHtmlComboBox::HideWindow()
{
    NCWnd::HideWindow();
}

// 0x10030240 (vslot 50) - NCComboBox::OnPaint clipped to the viewer while closed (to itself
// while open, so the list shows past the viewer's edge); the origin goes on before the clip.
// Disabled, it tests the disabled textures but draws the normal ones (sic). The guard names
// NCComboBox, as in the stock code.
INT NCHtmlComboBox::OnPaint(NCGDevice* Device)
{
    guard(NCComboBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    if (bDropped)
        Device->SetClip(ScreenX, ScreenY, Width, Height);
    else
    {
        NCWnd* Viewer = EnclosingHtmlViewer(this);
        if (Viewer)
            Device->SetClip(Viewer->ScreenX, Viewer->ScreenY, Viewer->Width, Viewer->Height);
    }
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
            Device->DrawTexture(0, 0, 15, 15, 0, 0, 15, 15, LeftTex);
        if (MidDisabledTex)
            Device->DrawTexture(1, 0, Width, 15, 0, 0, 15, 15, MidTex);
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
NW_HOOK_METHOD(0x10030240, NCHtmlComboBox, OnPaint)

// 0x10030230 (vslot 62) - the chosen item
const TCHAR* NCHtmlComboBox::GetValue()
{
    return GetItemText(Selected);
}
NW_HOOK_METHOD(0x10030230, NCHtmlComboBox, GetValue)

// 0x1002fd80 - the items of a ';'-separated list (cut in place), the one equal to Sel chosen
// (the first when Sel is empty). The last item is added after the loop; Sel is read without
// a NULL test there. The token lives in List's own slot, so a list with no token (empty or
// only ';') adds List itself.
INT NCHtmlComboBox::SetList(TCHAR* List, TCHAR* Sel)
{
    INT Index = 0;
    TCHAR* Token = List;
    TCHAR* Next = NextListToken(List, &Token);
    while (Token && *Next)
    {
        AddItem(Token);
        if (Sel && !appStrcmp(Token, Sel))
            SetSelected(Index);
        Index++;
        Next = NextListToken(Next, &Token);
    }
    AddItem(Token);
    if (*Sel)
    {
        if (!appStrcmp(Token, Sel))
            SetSelected(Index);
    }
    else
        SetSelected(0);
    return 1;
}
NW_HOOK_METHOD(0x1002fd80, NCHtmlComboBox, SetList)

NW_CHECK_VTABLE(NCHtmlComboBox, 0x100de728, 64)

/*=============================================================================
    NCCheckBox.cpp - check box. Stock: 0x10003d70 .. 0x10004266.
=============================================================================*/
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCGDevice.h"

#include <wchar.h>

// 0x10003df0 - the textures come in OnCreate
NCCheckBox::NCCheckBox(const TCHAR* InLabel, UBOOL bInChecked, UBOOL bInLabelLeft)
{
    if (InLabel && *InLabel)
        wcscpy(Label, InLabel);
    CheckedTex = NULL;
    UncheckedTex = NULL;
    bDisabled = 0;
    BoxW = 12;
    BoxH = 12;
    bChecked = bInChecked;
    bLabelLeft = bInLabelLeft;
}
NW_HOOK_CTOR_ARGS(0x10003df0, NCCheckBox, (const TCHAR* A, UBOOL B, UBOOL C), (A, B, C))

// 0x10003f00 - label from the SysString table, "Check Box" when missing
NCCheckBox::NCCheckBox(INT LabelID, UBOOL bInChecked, UBOOL bInLabelLeft)
{
    if (*GL2GameData.SysString[LabelID])
        appSprintf(Label, TEXT("%s"), *GL2GameData.SysString[LabelID]);
    else
        appSprintf(Label, TEXT("%s"), TEXT("Check Box"));
    BoxW = 12;
    BoxH = 12;
    CheckedTex = NULL;
    UncheckedTex = NULL;
    bChecked = bInChecked;
    bDisabled = 0;
    bLabelLeft = bInLabelLeft;
}
NW_HOOK_CTOR_ARGS(0x10003f00, NCCheckBox, (INT A, UBOOL B, UBOOL C), (A, B, C))

// 0x10003eb0
NW_HOOK_DTOR(0x10003eb0, NCCheckBox)

// stock body shared with NCButton's (0x100051f0)
INT NCCheckBox::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10003d70 (vslot 35) - NControl's is not called: no Disable texture
INT NCCheckBox::OnCreate()
{
    CheckedTex = FindTexture(TEXT("L2UI.Control.CheckBox_Checked"));
    UncheckedTex = FindTexture(TEXT("L2UI.Control.CheckBox"));
    UncheckedDisabledTex = FindTexture(TEXT("L2UI.Control.CheckBox_Unable"));
    CheckedDisabledTex = FindTexture(TEXT("L2UI.Control.CheckBox_Checked_unable"));
    return 1;
}
NW_HOOK_METHOD(0x10003d70, NCCheckBox, OnCreate)

// stock body shared with NCButton's (0x100189e0)
INT NCCheckBox::OnDestroy()
{
    NControl::OnDestroy();
    return 1;
}

// 0x10004000 (vslot 50)
INT NCCheckBox::OnPaint(NCGDevice* Device)
{
    guard(NCCheckBox::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    UTexture* Box;
    if (bDisabled)
        Box = bChecked ? CheckedDisabledTex : UncheckedDisabledTex;
    else
        Box = bChecked ? CheckedTex : UncheckedTex;
    DWORD Color = bDisabled ? 0xFF808080 : 0xFFFFFFFF;
    if (bLabelLeft)
    {
        const TCHAR* Text = Label;
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawTexture(Size.cx + 4, 0, BoxW, BoxH, 0, 0, BoxW, BoxH, Box);
        Device->DrawText(0, 0, Color, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    else
    {
        Device->DrawTexture(0, 0, BoxW, BoxH, 0, 0, BoxW, BoxH, Box);
        Device->DrawText(BoxW + 4, 0, Color, Label, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10004000, NCCheckBox, OnPaint)

// 0x10003dc0 (vslot 56)
INT NCCheckBox::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (!bDisabled)
    {
        bChecked = !bChecked;
        NotifyParent(bChecked);
    }
    return 1;
}
NW_HOOK_METHOD(0x10003dc0, NCCheckBox, OnLButtonDown)

NW_CHECK_VTABLE(NCCheckBox, 0x100d8b70, 63)

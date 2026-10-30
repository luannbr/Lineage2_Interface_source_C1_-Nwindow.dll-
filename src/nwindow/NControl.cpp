/*=============================================================================
    NControl.cpp - base control. Stock: 0x10001810 .. 0x100018a0, 0x10004e90 .. 0x10005080.
=============================================================================*/
#include "nwindow/NControl.h"

#include <wchar.h>

// 0x10001860
NControl::NControl()
{
    ID = 0;
    DisableTexture = NULL;
    Caption[0] = 0;
    NotifyID = -1;
}
NW_HOOK_CTOR(0x10001860, NControl)

// 0x10001810 - NW_DECLARE_CLASS destructor
NW_HOOK_DTOR(0x10001810, NControl)

// 0x10004e90 (vslot 24) - repeats NCWnd's enabled test before calling it
INT NControl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10004e90, NControl, WindowProc)

// 0x10004ff0 (vslot 35)
INT NControl::OnCreate()
{
    NCWnd::OnCreate();
    if (!DisableTexture)
        DisableTexture = FindTexture(TEXT("NWindow.Disable"));
    return 1;
}
NW_HOOK_METHOD(0x10004ff0, NControl, OnCreate)

// 0x10005020 (vslot 36) - destroys the shared "NWindow.Disable" texture object itself, as the
// stock code does (UTexture::Destroy through the vtable)
INT NControl::OnDestroy()
{
    if (DisableTexture)
    {
        DisableTexture->Destroy();
        DisableTexture = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10005020, NControl, OnDestroy)

/*-----------------------------------------------------------------------------
    Parent communication (names ours).
-----------------------------------------------------------------------------*/

// 0x10004ed0 - NWM_COMMAND(ID, Code) to the parent
UBOOL NControl::NotifyParent(LPARAM Code)
{
    NCWnd* Parent = ParentWnd();
    if (!Parent)
        return 0;
    Parent->WindowProc(NWM_COMMAND, ID, Code);
    return 1;
}
NW_HOOK_METHOD(0x10004ed0, NControl, NotifyParent)

// 0x10004f10 - the parent takes the focus back
void NControl::ReleaseFocus()
{
    if (NCWnd* Parent = ParentWnd())
        Parent->SetFocus();
    KillFocus();
}
NW_HOOK_METHOD(0x10004f10, NControl, ReleaseFocus)

// 0x10004f40
void NControl::HideParent()
{
    if (NCWnd* Parent = ParentWnd())
        Parent->HideWindow();
    KillFocus();
}
NW_HOOK_METHOD(0x10004f40, NControl, HideParent)

// 0x10004f70 - does not use this
UBOOL NControl::SendTo(NWndEntry* Target, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (!Target || !Target->Wnd)
        return 0;
    return Target->Wnd->WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x10004f70, NControl, SendTo)

// 0x10004fb0 - the parent's answer is dropped
UBOOL NControl::SendToParent(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    NCWnd* Parent = ParentWnd();
    if (!Parent)
        return 0;
    Parent->WindowProc(Msg, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10004fb0, NControl, SendToParent)

// 0x10005050 - no length check, as in the stock code (Caption holds 128)
void NControl::SetCaption(const TCHAR* Text)
{
    if (Text)
        wcscpy(Caption, Text);
}
NW_HOOK_METHOD(0x10005050, NControl, SetCaption)

// 0x10005070 - NULL when empty
const TCHAR* NControl::GetCaption()
{
    const TCHAR* Text = Caption;
    if (!Text || !*Text)
        return NULL;
    return Text;
}
NW_HOOK_METHOD(0x10005070, NControl, GetCaption)

NW_CHECK_VTABLE(NControl, 0x100d7ea8, 63)

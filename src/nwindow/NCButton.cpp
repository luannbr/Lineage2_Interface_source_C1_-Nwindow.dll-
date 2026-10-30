/*=============================================================================
    NCButton.cpp - NCButton, NCExButton, NCPushButton, NCRadioButton.
    Stock: 0x10001000 .. 0x10002550, plus WindowProc 0x100051f0 / 0x100301f0 and
    OnDestroy 0x100189e0 (bodies the linker folded with other classes).
=============================================================================*/
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0, SOUND_ClickDisabled = 2 };

// timer of the push button auto-repeat
enum { TIMER_PushRepeat = 0xFF000000 };

/*-----------------------------------------------------------------------------
    NCButton.
-----------------------------------------------------------------------------*/

// 0x10001950
NCButton::NCButton()
{
    TextID = -1;
    ToolTipID = -1;
    bReleased = 1;
    NormalTex = NULL;
    PushedTex = NULL;
    bAcceptKey = 0;
    NormalTexH = 0;
    NormalTexW = 0;
    PushedTexH = 0;
    PushedTexW = 0;
    bMouseOver = 0;
    bToolTipAbove = 1;
    Text[0] = 0;
    TextOffsetX = 0;
    TextOffsetY = 0;
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
}
NW_HOOK_CTOR(0x10001950, NCButton)

// 0x10001a20 - the textures are not checked: FindTexture falls back to NWindow.BlackTexture
NCButton::NCButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey)
{
    bReleased = 1;
    NormalTex = FindTexture(NormalTexName);
    NormalTexW = NormalTex->USize;
    NormalTexH = NormalTex->VSize;
    PushedTex = FindTexture(PushedTexName);
    PushedTexW = PushedTex->USize;
    PushedTexH = PushedTex->VSize;
    bAcceptKey = bInAcceptKey;
    bMouseOver = 0;
    bToolTipAbove = 1;
    TextID = -1;
    ToolTipID = -1;
    Text[0] = 0;
    TextOffsetX = 0;
    TextOffsetY = 0;
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
}
NW_HOOK_CTOR_ARGS(0x10001a20, NCButton, (const TCHAR* A, const TCHAR* B, UBOOL C), (A, B, C))

// 0x100018d0
NW_HOOK_DTOR(0x100018d0, NCButton)

// 0x100051f0 (vslot 24)
INT NCButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100051f0, NCButton, WindowProc)

// 0x100189e0 (vslot 36)
INT NCButton::OnDestroy()
{
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100189e0, NCButton, OnDestroy)

// 0x10001b30 (vslot 50) - texture stretched over the button, label centered (grey when
// disabled). Width and Height are halved unsigned, as in the stock code.
INT NCButton::OnPaint(NCGDevice* Device)
{
    guard(NCButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (bReleased)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, NormalTex);
    else
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, PushedTex);

    SIZE Size;
    const TCHAR* Label = NULL;
    if (TextID >= 0)
    {
        if (*GL2GameData.SysString[TextID])
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[TextID], 0, 0, 0, 0, 0);
            Label = *GL2GameData.SysString[TextID];
        }
    }
    else
    {
        const TCHAR* Str = Text;
        if (Str && *Str)
        {
            NCGDevice::GetTextExtent(&Size, Str, 0, 0, 0, 0, 0);
            Label = Str;
        }
    }
    if (Label)
        Device->DrawText(((DWORD)Width >> 1) + TextOffsetX - Size.cx / 2,
                         ((DWORD)Height >> 1) + TextOffsetY - Size.cy / 2,
                         IsEnabled() ? 0xFFFFFFFF : 0xFF7E7E7E, Label, 0, 0, 0, L2FONT_Normal, 0, 0,
                         0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10001b30, NCButton, OnPaint)

// 0x10001dd0 (vslot 51) - above or below the button (NCGDevice::DrawToolTipBox). Draws even
// with ToolTipID -1, like the stock code (SysString[-1] is whatever precedes the table:
// normally an empty string).
INT NCButton::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (*GL2GameData.SysString[ToolTipID] && bMouseOver)
        Device->DrawToolTipBox(this, *GL2GameData.SysString[ToolTipID], ToolTipTex, bToolTipAbove);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10001dd0, NCButton, OnDrawToolTip)

// 0x10001150 (vslot 53)
INT NCButton::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if ((wParam == VK_RETURN || wParam == VK_SPACE) && bAcceptKey == 1)
    {
        GConsole->Engine->OnInterfacePlaySound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
        NotifyParent(0);
    }
    return 1;
}
NW_HOOK_METHOD(0x10001150, NCButton, OnKeyDown)

// 0x100010c0 (vslot 55) - dragging out of the button releases it without a click
INT NCButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & MK_LBUTTON) && IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    else
        bReleased = 1;
    bMouseOver = PtInRect(LOWORD(lParam), HIWORD(lParam), ScreenX, ScreenY, Width, Height);
    return 1;
}
NW_HOOK_METHOD(0x100010c0, NCButton, OnMouseMove)

// 0x10001000 (vslot 56)
INT NCButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
    {
        bReleased = 0;
        GConsole->Engine->OnInterfacePlaySound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    }
    return 1;
}
NW_HOOK_METHOD(0x10001000, NCButton, OnLButtonDown)

// 0x10001070 (vslot 57) - the click: released inside, enabled
INT NCButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)) && IsEnabled())
        NotifyParent(NotifyID == -1 ? ID : NotifyID);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10001070, NCButton, OnLButtonUp)

// 0x100011b0 - kept only if the string exists (FString::operator* never returns NULL, so
// always, in practice)
void NCButton::SetTextID(INT InID)
{
    if (*GL2GameData.SysString[InID])
        TextID = InID;
}
NW_HOOK_METHOD(0x100011b0, NCButton, SetTextID)

// 0x100011e0 - no length check, as in the stock code (Text holds 32)
void NCButton::SetText(const TCHAR* InText)
{
    if (InText && *InText)
    {
        TextID = -1;
        wcscpy(Text, InText);
    }
}
NW_HOOK_METHOD(0x100011e0, NCButton, SetText)

// 0x10001210
UBOOL NCButton::SetNormalTex(const TCHAR* Name)
{
    if (Name && *Name)
    {
        NormalTex = FindTexture(Name);
        NormalTexW = NormalTex->USize;
        NormalTexH = NormalTex->VSize;
        return 1;
    }
    NormalTex = NULL;
    return 1;
}
NW_HOOK_METHOD(0x10001210, NCButton, SetNormalTex)

// 0x10001260 - unlike SetNormalTex, an empty name still looks the texture up
UBOOL NCButton::SetPushedTex(const TCHAR* Name)
{
    if (Name)
    {
        PushedTex = FindTexture(Name);
        PushedTexW = PushedTex->USize;
        PushedTexH = PushedTex->VSize;
        return 1;
    }
    PushedTex = NULL;
    return 1;
}
NW_HOOK_METHOD(0x10001260, NCButton, SetPushedTex)

// 0x100012b0
void NCButton::SetToolTip(INT InID, UBOOL bAbove)
{
    if (*GL2GameData.SysString[InID])
    {
        ToolTipID = InID;
        bToolTipAbove = bAbove;
    }
}
NW_HOOK_METHOD(0x100012b0, NCButton, SetToolTip)

NW_CHECK_VTABLE(NCButton, 0x100d7fa8, 63)

/*-----------------------------------------------------------------------------
    NCExButton: reports press (1) and release (0).
-----------------------------------------------------------------------------*/

// 0x100020e0
NCExButton::NCExButton()
{
}
NW_HOOK_CTOR(0x100020e0, NCExButton)

// 0x10002180
NCExButton::NCExButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, UBOOL bInAcceptKey)
    : NCButton(NormalTexName, PushedTexName, bInAcceptKey)
{
}
NW_HOOK_CTOR_ARGS(0x10002180, NCExButton, (const TCHAR* A, const TCHAR* B, UBOOL C), (A, B, C))

// 0x10002130
NW_HOOK_DTOR(0x10002130, NCExButton)

// 0x100301f0 (vslot 24) - one body for NCExButton, NCPushButton and NCRadioButton
INT NCExButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x100301f0, NCExButton, WindowProc)

// 0x100012f0 (vslot 56) - reports even when the press is outside the button
INT NCExButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    NotifyParent(1);
    return 1;
}
NW_HOOK_METHOD(0x100012f0, NCExButton, OnLButtonDown)

// 0x10001330 (vslot 57)
INT NCExButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    NotifyParent(0);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10001330, NCExButton, OnLButtonUp)

NW_CHECK_VTABLE(NCExButton, 0x100d8108, 63)

/*-----------------------------------------------------------------------------
    NCPushButton: reports on press, then every RepeatDelay ms while held.
-----------------------------------------------------------------------------*/

// 0x100021b0
NCPushButton::NCPushButton()
{
    bHeld = 0;
    RepeatDelay = 100;
    bAutoRepeat = 1;
}
NW_HOOK_CTOR(0x100021b0, NCPushButton)

// 0x10002260 - RepeatDelay and bAutoRepeat stay as allocated (zero): no auto-repeat
NCPushButton::NCPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
{
    bHeld = 0;
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10002260, NCPushButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10002210
NW_HOOK_DTOR(0x10002210, NCPushButton)

INT NCPushButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10001480 (vslot 34)
INT NCPushButton::OnTimer(INT TimerID)
{
    if (TimerID == (INT)TIMER_PushRepeat && bHeld)
        NotifyParent(0);
    return 1;
}
NW_HOOK_METHOD(0x10001480, NCPushButton, OnTimer)

// 0x100015f0 (vslot 36) - also NCRadioButton's
INT NCPushButton::OnDestroy()
{
    NCButton::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100015f0, NCPushButton, OnDestroy)

// 0x10001410 (vslot 55)
INT NCPushButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & MK_LBUTTON) && IsFocused() && IsInWindow(LOWORD(lParam), HIWORD(lParam)))
    {
        bHeld = 1;
        bReleased = 0;
        return 1;
    }
    bHeld = 0;
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10001410, NCPushButton, OnMouseMove)

// 0x10001350 (vslot 56) - no hit test; NotifyID is sent as is (-1 is not replaced by ID)
INT NCPushButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    bReleased = 0;
    bHeld = 1;
    GConsole->Engine->OnInterfacePlaySound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    NotifyParent(NotifyID);
    if (bAutoRepeat)
        SetTimer(TIMER_PushRepeat, RepeatDelay, NULL);
    return 1;
}
NW_HOOK_METHOD(0x10001350, NCPushButton, OnLButtonDown)

// 0x100013d0 (vslot 57)
INT NCPushButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    bReleased = 1;
    bHeld = 0;
    if (bAutoRepeat)
        KillTimer(TIMER_PushRepeat);
    ReleaseFocus();
    return 1;
}
NW_HOOK_METHOD(0x100013d0, NCPushButton, OnLButtonUp)

NW_CHECK_VTABLE(NCPushButton, 0x100d8208, 63)

/*-----------------------------------------------------------------------------
    NCRadioButton: stays down once clicked and reports Value.
-----------------------------------------------------------------------------*/

// 0x100022c0
NCRadioButton::NCRadioButton()
{
    CreateHeight = 0;
    CreateWidth = 0;
    bReleased = 1;
    Value = -1;
}
NW_HOOK_CTOR(0x100022c0, NCRadioButton)

// 0x10002380 - sets the textures twice (base constructor, then the setters), as the stock code
NCRadioButton::NCRadioButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
    : NCButton(NormalTexName, PushedTexName, 0)
{
    Value = -1;
    CreateHeight = 0;
    CreateWidth = 0;
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10002380, NCRadioButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10002330
NW_HOOK_DTOR(0x10002330, NCRadioButton)

INT NCRadioButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100015c0 (vslot 35)
INT NCRadioButton::OnCreate()
{
    NCButton::OnCreate();
    CreateWidth = Width;
    CreateHeight = Height;
    return 1;
}
NW_HOOK_METHOD(0x100015c0, NCRadioButton, OnCreate)

INT NCRadioButton::OnDestroy()
{
    NCButton::OnDestroy();
    return 1;
}

// 0x10002410 (vslot 51) - the tip covers the button itself
INT NCRadioButton::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (*GL2GameData.SysString[ToolTipID] && bMouseOver)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[ToolTipID], 0, 0, 0, 0, 0);
        Device->SetClip(ScreenX, ScreenY, Size.cx, Size.cy);
        if (ToolTipTex)
            Device->DrawStretchedTex(0, 0, Size.cx, Size.cy, ToolTipTex);
        Device->DrawText(0, 0, 0xFFFFFFFF, *GL2GameData.SysString[ToolTipID], 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        Device->ReleaseClip();
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10002410, NCRadioButton, OnDrawToolTip)

// 0x10001570 (vslot 55)
INT NCRadioButton::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    bMouseOver = PtInRect(LOWORD(lParam), HIWORD(lParam), ScreenX, ScreenY, Width, Height);
    return 1;
}
NW_HOOK_METHOD(0x10001570, NCRadioButton, OnMouseMove)

// 0x100014b0 (vslot 56) - a button without a label never reports
INT NCRadioButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (!bReleased)
    {
        if (bToggle)
            bReleased = 1;
        return 1;
    }
    if (TextID >= 0)
    {
        if (*GL2GameData.SysString[TextID] && !bToggle)
            NotifyParent(Value);
    }
    else if (Text[0] && !bToggle)
        NotifyParent(Value);
    bReleased = 0;
    return 1;
}
NW_HOOK_METHOD(0x100014b0, NCRadioButton, OnLButtonDown)

// 0x10001550 (vslot 57)
INT NCRadioButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (bToggle)
        ReleaseFocus();
    return 1;
}
NW_HOOK_METHOD(0x10001550, NCRadioButton, OnLButtonUp)

NW_CHECK_VTABLE(NCRadioButton, 0x100d8308, 63)

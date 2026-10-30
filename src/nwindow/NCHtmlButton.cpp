/*=============================================================================
    NCHtmlButton.cpp - NCHtmlButton and NCHtmlPushButton, the buttons of the HTML pages.
    Stock: 0x1002fee0 .. 0x1002ff16, 0x10030840 .. 0x10030fb9 (WindowProc is a body the linker
    folded with the other buttons').
=============================================================================*/
#include "nwindow/NCHtmlButton.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCGDevice.h"

// the face and the label as NCButton::OnPaint draws them, the enabled state read from the
// field (the same code in both classes of the stock build)
static FORCEINLINE void DrawHtmlButton(NCButton* Button, NCGDevice* Device)
{
    if (Button->bReleased)
        Device->DrawTexture(0, 0, Button->Width, Button->Height, 0, 0, Button->Width,
                            Button->Height, Button->NormalTex);
    else
        Device->DrawTexture(0, 0, Button->Width, Button->Height, 0, 0, Button->Width,
                            Button->Height, Button->PushedTex);

    SIZE Size;
    const TCHAR* Label = NULL;
    if (Button->TextID >= 0)
    {
        if (*GL2GameData.SysString[Button->TextID])
        {
            NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[Button->TextID], 0, 0, 0, 0, 0);
            Label = *GL2GameData.SysString[Button->TextID];
        }
    }
    else
    {
        const TCHAR* Str = Button->Text;
        if (Str && *Str)
        {
            NCGDevice::GetTextExtent(&Size, Str, 0, 0, 0, 0, 0);
            Label = Str;
        }
    }
    if (Label)
        Device->DrawText(((DWORD)Button->Width >> 1) + Button->TextOffsetX - Size.cx / 2,
                         ((DWORD)Button->Height >> 1) + Button->TextOffsetY - Size.cy / 2,
                         Button->bEnabled ? 0xFFFFFFFF : 0xFF7E7E7E, Label, 0, 0, 0,
                         L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

/*-----------------------------------------------------------------------------
    NCHtmlButton.
-----------------------------------------------------------------------------*/

// 0x10030840
NCHtmlButton::NCHtmlButton()
{
}
NW_HOOK_CTOR(0x10030840, NCHtmlButton)

// 0x100308e0
NCHtmlButton::NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName,
                           const TCHAR* InCommand, UBOOL bInAcceptKey)
    : NCButton(NormalTexName, PushedTexName, bInAcceptKey)
{
    Command = InCommand;
}
NW_HOOK_CTOR_ARGS(0x100308e0, NCHtmlButton,
                  (const TCHAR* A, const TCHAR* B, const TCHAR* C, UBOOL D), (A, B, C, D))

// 0x10030910 - Command is left as allocated (the combo box's drop button)
NCHtmlButton::NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
    : NCButton(NormalTexName, PushedTexName, 0)
{
}
NW_HOOK_CTOR_ARGS(0x10030910, NCHtmlButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10030890
NW_HOOK_DTOR(0x10030890, NCHtmlButton)

// stock body shared with NCExButton's (0x100301f0)
INT NCHtmlButton::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCButton::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10030940 (vslot 50) - clipped to the viewer; the origin goes on before the clip
INT NCHtmlButton::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Viewer = EnclosingHtmlViewer(this);
    if (Viewer)
        Device->SetClip(Viewer->ScreenX, Viewer->ScreenY, Viewer->Width, Viewer->Height);
    DrawHtmlButton(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10030940, NCHtmlButton, OnPaint)

// 0x1002fee0 (vslot 57) - a release inside reports Command, even when disabled, without sound
INT NCHtmlButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        NotifyParent((LPARAM)Command);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x1002fee0, NCHtmlButton, OnLButtonUp)

NW_CHECK_VTABLE(NCHtmlButton, 0x100defe8, 63)

/*-----------------------------------------------------------------------------
    NCHtmlPushButton.
-----------------------------------------------------------------------------*/

// 0x10030c20
NCHtmlPushButton::NCHtmlPushButton()
{
}
NW_HOOK_CTOR(0x10030c20, NCHtmlPushButton)

// 0x10030cc0 - NCPushButton's default constructor, then the textures
NCHtmlPushButton::NCHtmlPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName)
{
    SetNormalTex(NormalTexName);
    SetPushedTex(PushedTexName);
}
NW_HOOK_CTOR_ARGS(0x10030cc0, NCHtmlPushButton, (const TCHAR* A, const TCHAR* B), (A, B))

// 0x10030c70
NW_HOOK_DTOR(0x10030c70, NCHtmlPushButton)

// 0x10030d20 (vslot 50) - clipped to the first ancestor that is not a control
INT NCHtmlPushButton::OnPaint(NCGDevice* Device)
{
    guard(NCHtmlPushButton::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    NCWnd* Owner = FirstNonControlParent(this);
    if (Owner)
        Device->SetClip(Owner->ScreenX, Owner->ScreenY, Owner->Width, Owner->Height);
    DrawHtmlButton(this, Device);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10030d20, NCHtmlPushButton, OnPaint)

NW_CHECK_VTABLE(NCHtmlPushButton, 0x100df110, 63)

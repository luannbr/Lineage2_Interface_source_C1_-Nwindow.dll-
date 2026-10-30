/*=============================================================================
    NCNoticeWnd.cpp - the announcements box and its text. Stock: 0x10079480 .. 0x1007956d,
    0x100796e0 .. 0x10079d69 (NCWarningWnd's functions sit in between and after).
=============================================================================*/
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// NWM_COMMAND id of OK, title bar id (names ours)
enum
{
    NID_NOTICE_OK = 0xF9,
    NID_NOTICE_FRAME = 0xA000F9,
};

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0 };

/*-----------------------------------------------------------------------------
    NCNoticeInfoWnd.
-----------------------------------------------------------------------------*/

// 0x100796e0 - lines 16 pixels apart in the scroll window
NCNoticeInfoWnd::NCNoticeInfoWnd()
{
    Text[0] = 0;
    Unknown0C8 = 16;
}
NW_HOOK_CTOR(0x100796e0, NCNoticeInfoWnd)

// 0x10079740
NW_HOOK_DTOR(0x10079740, NCNoticeInfoWnd)

// stock body shared with NCItemWnd's (0x1006bb90)
INT NCNoticeInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10079480 (vslot 35)
INT NCNoticeInfoWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    return 1;
}
NW_HOOK_METHOD(0x10079480, NCNoticeInfoWnd, OnCreate)

// 0x10079490 (vslot 36)
INT NCNoticeInfoWnd::OnDestroy()
{
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10079490, NCNoticeInfoWnd, OnDestroy)

// 0x10079790 (vslot 50) - the text wrapped at the width less the scroll bar, shifted by the
// scroll position (NCGDevice's origin stack); its height sets the scroll range. The clip goes
// on before BeginPaint.
INT NCNoticeInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNoticeInfoWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    const TCHAR* Str = Text;
    if (Str && *Str)
    {
        Device->PushOrigin(ScrollX, ScrollY);
        Device->DrawText(2, 5, 0xFFFFFFFF, Text, Width - 20, 0, 1, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, Width - 20, 0, 1, 0, 0);
        SetContentHeight(Size.cy);
        Device->PopOrigin();
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10079790, NCNoticeInfoWnd, OnPaint)

// 0x100794a0 - an empty text leaves the old one (wcscpy, as in the stock code)
void NCNoticeInfoWnd::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        wcscpy(Text, InText);
}
NW_HOOK_METHOD(0x100794a0, NCNoticeInfoWnd, SetText)

// 0x100794d0
void NCNoticeInfoWnd::ClearText()
{
    Text[0] = 0;
}
NW_HOOK_METHOD(0x100794d0, NCNoticeInfoWnd, ClearText)

NW_CHECK_VTABLE(NCNoticeInfoWnd, 0x100e8440, 65)

/*-----------------------------------------------------------------------------
    NCNoticeWnd.
-----------------------------------------------------------------------------*/

// 0x100798c0
NCNoticeWnd::NCNoticeWnd()
{
    BackTex = NULL;
    Type = 0;
}
NW_HOOK_CTOR(0x100798c0, NCNoticeWnd)

// 0x10079910
NW_HOOK_DTOR(0x10079910, NCNoticeWnd)

// 0x100794e0 (vslot 24)
INT NCNoticeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_NOTICE_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x100794e0, NCNoticeWnd, WindowProc)

// 0x10079960 (vslot 35) - title bar 532 wide, the text under it, OK at the bottom
INT NCNoticeWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.etcWnd.Announce_back"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_NOTICE_FRAME;
    FrameCtrl->ID = NID_NOTICE_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 532, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    InfoWnd = new NCNoticeInfoWnd;
    InfoWnd->SetWindowRect(6, 24, 525, 373);
    CreateChild(InfoWnd, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.control.defaultbutton"),
                            TEXT("L2UI.control.defaultbutton_click"), 0);
    OKButton->ID = NID_NOTICE_OK;
    OKButton->SetWindowRect(239, 400, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);        // "OK"
    return 1;
}
NW_HOOK_METHOD(0x10079960, NCNoticeWnd, OnCreate)

// 0x10079b70 (vslot 36)
INT NCNoticeWnd::OnDestroy()
{
    if (InfoWnd)
    {
        RemoveChildWindow(InfoWnd->Entry);
        delete InfoWnd;
        InfoWnd = NULL;
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10079b70, NCNoticeWnd, OnDestroy)

// 0x10079be0 (vslot 50) - under the 21-pixel title bar; the clip goes on before BeginPaint
INT NCNoticeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNoticeWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, Width, Height, 0, 0, Width, Height, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10079be0, NCNoticeWnd, OnPaint)

// 0x10079d00 (vslot 53) - Enter does what OK does, with the click sound
INT NCNoticeWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        if (Type && (Type & 1))
            GConsole->ShowWarningWnd();
        else
            GConsole->RequestServerList();
        GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10079d00, NCNoticeWnd, OnKeyDown)

// 0x10079cc0 (vslot 62)
INT NCNoticeWnd::OnOK()
{
    if (!Type)
        GConsole->RequestServerList();
    else if (Type & 1)
        GConsole->ShowWarningWnd();
    else
        GConsole->RequestServerList();
    return 1;
}
NW_HOOK_METHOD(0x10079cc0, NCNoticeWnd, OnOK)

// 0x10079550
void NCNoticeWnd::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        InfoWnd->SetText(InText);
}
NW_HOOK_METHOD(0x10079550, NCNoticeWnd, SetText)

// 0x10079570 - NCLoginWnd::ResetWindows
void NCNoticeWnd::ClearText()
{
    InfoWnd->ClearText();
}
NW_HOOK_METHOD(0x10079570, NCNoticeWnd, ClearText)

NW_CHECK_VTABLE(NCNoticeWnd, 0x100e8578, 63)

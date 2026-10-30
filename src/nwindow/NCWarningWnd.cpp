/*=============================================================================
    NCWarningWnd.cpp - the login screen's warning box. Stock: 0x10079580 .. 0x100796df and
    0x10079d70 .. 0x1007a12f (NCNoticeInfoWnd and NCNoticeWnd sit in between, NCNoticeWnd.cpp).
=============================================================================*/
#include "nwindow/NCWarningWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCNoticeWnd.h"
#include "nwindow/NConsoleWnd.h"

// ids (names ours)
enum
{
    NID_WARNING_OK = 0xF8,
    NID_WARNING_FRAME = 0xA000F9,
};
enum { TIMER_WARNING = 0xF030D1 };  // nothing handles it (OnTimer returns 1)
enum { SOUND_Click = 0 };

// 0x10079d70 - the timer result and the controls are left as allocated
NCWarningWnd::NCWarningWnd()
{
    BackTex = NULL;
    Unknown98 = 0;
}
NW_HOOK_CTOR(0x10079d70, NCWarningWnd)

// 0x10079dc0
NW_HOOK_DTOR(0x10079dc0, NCWarningWnd)

// 0x10079580 (vslot 24)
INT NCWarningWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_WARNING_OK)
        return OnOK() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10079580, NCWarningWnd, WindowProc)

// 0x100795f0
void NCWarningWnd::SetText(const TCHAR* Text)
{
    if (Text && *Text)
        InfoWnd->SetText(Text);
}
NW_HOOK_METHOD(0x100795f0, NCWarningWnd, SetText)

// 0x10079610
void NCWarningWnd::ClearText()
{
    InfoWnd->ClearText();
}
NW_HOOK_METHOD(0x10079610, NCWarningWnd, ClearText)

// 0x10079620 (vslot 62)
INT NCWarningWnd::OnOK()
{
    GConsole->RequestServerList();
    return 1;
}
NW_HOOK_METHOD(0x10079620, NCWarningWnd, OnOK)

// 0x10079640 (vslot 53) - Enter works as OK, then the key goes on to NCWnd
INT NCWarningWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
    {
        GConsole->RequestServerList();
        GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    }
    NCWnd::OnKeyDown(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x10079640, NCWarningWnd, OnKeyDown)

// 0x10079690 (vslot 42) - a one-minute timer nothing listens to
void NCWarningWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TimerResult = SetTimer(TIMER_WARNING, 60000, NULL);
    Unknown98 = 0;
}
NW_HOOK_METHOD(0x10079690, NCWarningWnd, ShowWindow)

// 0x100796c0 (vslot 43)
void NCWarningWnd::HideWindow()
{
    NCWnd::HideWindow();
    KillTimer(TIMER_WARNING);
    Unknown98 = 0;
}
NW_HOOK_METHOD(0x100796c0, NCWarningWnd, HideWindow)

// 0x10079e10 (vslot 35) - title bar, text and OK button of a 532 x 424 box
INT NCWarningWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.etcWnd.Announce_back"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_WARNING_FRAME;
    FrameCtrl->ID = NID_WARNING_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 532, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    InfoWnd = new NCNoticeInfoWnd();
    InfoWnd->SetWindowRect(6, 24, 525, 373);
    CreateChild(InfoWnd, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.control.defaultbutton"), TEXT("L2UI.control.defaultbutton_click"), 0);
    OKButton->ID = NID_WARNING_OK;
    OKButton->SetWindowRect(239, 400, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(140);  // "OK"
    return 1;
}
NW_HOOK_METHOD(0x10079e10, NCWarningWnd, OnCreate)

// 0x1007a020 (vslot 36) - the title bar goes with NCWnd::OnDestroy
INT NCWarningWnd::OnDestroy()
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
NW_HOOK_METHOD(0x1007a020, NCWarningWnd, OnDestroy)

// 0x1007a090 (vslot 50) - the background starts below the 21-pixel title bar; the guard
// carries NCNoticeWnd's name in the stock code
INT NCWarningWnd::OnPaint(NCGDevice* Device)
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
NW_HOOK_METHOD(0x1007a090, NCWarningWnd, OnPaint)

NW_CHECK_VTABLE(NCWarningWnd, 0x100e8758, 63)

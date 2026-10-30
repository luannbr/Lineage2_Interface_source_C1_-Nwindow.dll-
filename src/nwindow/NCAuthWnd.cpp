/*=============================================================================
    NCAuthWnd.cpp - the login box. Stock: 0x1006a730 .. 0x1006a95b (0x1006a7e0 included),
    0x1006aa60 .. 0x1006b152 (NCLoginWnd's functions sit in between).
=============================================================================*/
#include "nwindow/NCAuthWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids of the box's controls, timer id (names ours)
enum
{
    NID_AUTH_OK = 0x1006,
    NID_AUTH_CANCEL = 0x1007,
    NID_AUTH_ACCOUNT = 0x1502,
    NID_AUTH_PASSWORD = 0x1503,
    NID_AUTH_FRAME = (INT)0xAFF12010,
    TIMER_AUTH_RETRY = (INT)0xAFF12013,   // 3 s: the next Log In may ask again
};

// systemmsg lines (names ours)
enum
{
    MSG_EnterAccount = 86,     // "Please insert your ID."
    MSG_EnterPassword = 87,    // "Please insert your password."
    MSG_CannotConnect = 90,    // "Unable to connect to server."
};

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0 };

// 0x1006aa60 - the timer result is left as allocated
NCAuthWnd::NCAuthWnd()
{
    Unknown94 = 0;
    BackTex = NULL;
    AccountEdit = NULL;
    PasswordEdit = NULL;
    OKButton = NULL;
    CancelButton = NULL;
    bRequested = 0;
}
NW_HOOK_CTOR(0x1006aa60, NCAuthWnd)

// 0x1006aad0
NW_HOOK_DTOR(0x1006aad0, NCAuthWnd)

// 0x1006a730 (vslot 24)
INT NCAuthWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_AUTH_OK)
            return OnOK() != 0;
        if (wParam == NID_AUTH_CANCEL)
            return OnCancel() != 0;
        if (wParam == NID_AUTH_PASSWORD)
            return OnPasswordNotify((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1006a730, NCAuthWnd, WindowProc)

// 0x1006a940 (vslot 34)
INT NCAuthWnd::OnTimer(INT TimerID)
{
    if (TimerID == TIMER_AUTH_RETRY)
        bRequested = 0;
    return 1;
}
NW_HOOK_METHOD(0x1006a940, NCAuthWnd, OnTimer)

// 0x1006ab20 (vslot 35) - TAB cycles account, password, Log In, Exit and back
INT NCAuthWnd::OnCreate()
{
    NCWnd::OnCreate();

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_AUTH_FRAME;
    FrameCtrl->ID = NID_AUTH_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 214, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI.LobbyWnd.loginBack"));
    TimerResult = SetTimer(TIMER_AUTH_RETRY, 3000, NULL);

    AccountEdit = new NCEditBox(0, NULL, 1, 128);
    AccountEdit->ID = NID_AUTH_ACCOUNT;
    AccountEdit->SetWindowRect(87, 48, 82, 16);
    CreateChild(AccountEdit, NWS_VISIBLE);

    PasswordEdit = new NCEditBox(2, NULL, 1, 128);
    PasswordEdit->ID = NID_AUTH_PASSWORD;
    PasswordEdit->SetWindowRect(87, 76, 82, 16);
    CreateChild(PasswordEdit, NWS_VISIBLE);

    OKButton = new NCButton(TEXT("L2UI.control.DefaultButton"),
                            TEXT("L2UI.control.DefaultButton_click"), 0);
    OKButton->ID = NID_AUTH_OK;
    OKButton->SetWindowRect(33, 112, 64, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(152);        // "Log In"

    CancelButton = new NCButton(TEXT("L2UI.control.DefaultButton"),
                                TEXT("L2UI.control.DefaultButton_click"), 0);
    CancelButton->ID = NID_AUTH_CANCEL;
    CancelButton->SetWindowRect(118, 112, 64, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(153);    // "Exit"

    SetTabOrder(AccountEdit->Entry, CancelButton->Entry);
    AccountEdit->SetTabOrder(PasswordEdit->Entry, Entry);
    PasswordEdit->SetTabOrder(OKButton->Entry, AccountEdit->Entry);
    OKButton->SetTabOrder(CancelButton->Entry, PasswordEdit->Entry);
    CancelButton->SetTabOrder(Entry, OKButton->Entry);
    return 1;
}
NW_HOOK_METHOD(0x1006ab20, NCAuthWnd, OnCreate)

// 0x1006b090 (vslot 36)
INT NCAuthWnd::OnDestroy()
{
    if (AccountEdit)
    {
        RemoveChildWindow(AccountEdit->Entry);
        delete AccountEdit;
        AccountEdit = NULL;
    }
    if (PasswordEdit)
    {
        RemoveChildWindow(PasswordEdit->Entry);
        delete PasswordEdit;
        PasswordEdit = NULL;
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1006b090, NCAuthWnd, OnDestroy)

// 0x1006a900 (vslot 42) - a new try: the account line gets the focus
void NCAuthWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    AccountEdit->SetFocus();
    bRequested = 0;
}
NW_HOOK_METHOD(0x1006a900, NCAuthWnd, ShowWindow)

// 0x1006af30 (vslot 50) - under the 21-pixel title bar; the clip goes on before BeginPaint
INT NCAuthWnd::OnPaint(NCGDevice* Device)
{
    guard(NCAuthWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, 214, 127, 0, 0, 214, 127, BackTex);
    if (*GL2GameData.SysString[150])        // "ID"
        Device->DrawText(42, 50, 0xFFFFFFFF, *GL2GameData.SysString[150], 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    if (*GL2GameData.SysString[151])        // "PWD"
        Device->DrawText(42, 78, 0xFFFFFFFF, *GL2GameData.SysString[151], 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006af30, NCAuthWnd, OnPaint)

// 0x1006a7f0 (vslot 62) - Log In
INT NCAuthWnd::OnOK()
{
    if (!AccountEdit->CommitText())
        GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_EnterAccount].Message);
    else if (AccountEdit->CommitText() && !PasswordEdit->CommitText())
        GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_EnterPassword].Message);
    else if (!bRequested)
    {
        if (!GConsole->RequestAuthLogin(AccountEdit->CommitText(), PasswordEdit->CommitText()))
            GConsole->ShowMessageWnd(*GL2GameData.SystemMsg[MSG_CannotConnect].Message);
        bRequested = 1;
    }
    GConsole->Engine->OnInterfacePlaySound(SOUND_Click);
    return 1;
}
NW_HOOK_METHOD(0x1006a7f0, NCAuthWnd, OnOK)

// 0x1006a8e0 (vslot 63) - Exit
INT NCAuthWnd::OnCancel()
{
    appRequestExit(0);
    return 1;
}
NW_HOOK_METHOD(0x1006a8e0, NCAuthWnd, OnCancel)

// 0x1006a920 (vslot 64) - Enter in the password line (code 1) logs in
INT NCAuthWnd::OnPasswordNotify(INT Code)
{
    if (Code == 1)
        OnOK();
    return 1;
}
NW_HOOK_METHOD(0x1006a920, NCAuthWnd, OnPasswordNotify)

// 0x1006a7e0 - after a failed login (NCLoginWnd::ResetWindows)
void NCAuthWnd::ClearPassword()
{
    PasswordEdit->ClearText();
}
NW_HOOK_METHOD(0x1006a7e0, NCAuthWnd, ClearPassword)

NW_CHECK_VTABLE(NCAuthWnd, 0x100e5e88, 65)

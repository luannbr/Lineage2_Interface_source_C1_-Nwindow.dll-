#pragma once
// NCAuthWnd - the login box of the login screen (UClass "CAuthWnd", 0xB4): account and password
// lines, "Log In" and "Exit" (NCLoginWnd creates it). Log In, or Enter in the password line,
// checks that both lines are filled and asks NConsoleWnd::RequestAuthLogin - once, until the
// box is shown again or its 3 s timer fires. Exit quits the game.

#include "nwindow/NCWnd.h"

class NCEditBox;
class NCButton;

class NCAuthWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAuthWnd, NCWnd, 0, 0x10135350)

    INT Unknown94;                   // +0x94 (0, never used)
    UTexture* BackTex;               // +0x98 "L2UI.LobbyWnd.loginBack"
    NCEditBox* AccountEdit;          // +0x9C
    NCEditBox* PasswordEdit;         // +0xA0
    NCButton* OKButton;              // +0xA4 "Log In"
    NCButton* CancelButton;          // +0xA8 "Exit"
    INT TimerResult;                 // +0xAC what SetTimer returned (never read)
    UBOOL bRequested;                // +0xB0 a login request went out

    NCAuthWnd();                                                     // 0x1006aa60
    // ~NCAuthWnd                                                    // 0x1006aad0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006a730
    virtual INT OnTimer(INT TimerID);                                // 34 0x1006a940
    virtual INT OnCreate();                                          // 35 0x1006ab20
    virtual INT OnDestroy();                                         // 36 0x1006b090
    virtual void ShowWindow();                                       // 42 0x1006a900
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006af30
    // names ours
    virtual INT OnOK();                                              // 62 0x1006a7f0
    virtual INT OnCancel();                                          // 63 0x1006a8e0
    virtual INT OnPasswordNotify(INT Code);                          // 64 0x1006a920

    void ClearPassword();                                            // 0x1006a7e0 (name ours)
};
NW_ASSERT_OFFSET(NCAuthWnd, BackTex, 0x98);
NW_ASSERT_OFFSET(NCAuthWnd, AccountEdit, 0x9C);
NW_ASSERT_OFFSET(NCAuthWnd, OKButton, 0xA4);
NW_ASSERT_OFFSET(NCAuthWnd, bRequested, 0xB0);
NW_ASSERT_SIZE(NCAuthWnd, 0xB4);

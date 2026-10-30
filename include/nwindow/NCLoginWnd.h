#pragma once
// NCLoginWnd - the login screen (UClass "CLoginWnd", 0xB8; NConsoleWnd +0x36C8): the game
// logo, the login box (NCAuthWnd), the announcement and warning boxes, the server list
// (NCServerSelectWnd) and three buttons in the lower right corner: "New Account" and "Lost
// Account" (created disabled) and "Options".

#include "nwindow/NCWnd.h"

class NCAuthWnd;
class NCNoticeWnd;
class NCServerSelectWnd;
class NCWarningWnd;
class NCButton;

class NCLoginWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoginWnd, NCWnd, 0, 0x10135870)

    NCAuthWnd* AuthWnd;              // +0x94
    NCNoticeWnd* NoticeWnd;          // +0x98 "Announcements"
    NCWarningWnd* WarningWnd;        // +0x9C "Warning"
    NCServerSelectWnd* ServerSelectWnd; // +0xA0
    UTexture* LogoTex;               // +0xA4 FontInfo.ini "<country>_LogoTexture"
    UTexture* MiniLogoTex;           // +0xA8 "L2UI.LoginWnd.mini_logo"
    NCButton* NewAccountButton;      // +0xAC
    NCButton* LostAccountButton;     // +0xB0 (no id)
    NCButton* OptionsButton;         // +0xB4

    NCLoginWnd();                                                    // 0x1006b160
    // ~NCLoginWnd                                                   // 0x1006b1b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006a960
    virtual INT OnCreate();                                          // 35 0x1006b200
    virtual INT OnDestroy();                                         // 36 0x1006b850
    virtual void ShowWindow();                                       // 42 0x1006a9f0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006b9a0
    // names ours
    virtual INT OnNewAccount() { return 1; }                         // 62 0x10005230 (folded)
    virtual INT OnOptions();                                         // 63 0x10076d00 (folded)

    void ResetWindows();                                             // 0x1006aa30 (name ours)
};
NW_ASSERT_OFFSET(NCLoginWnd, AuthWnd, 0x94);
NW_ASSERT_OFFSET(NCLoginWnd, ServerSelectWnd, 0xA0);
NW_ASSERT_OFFSET(NCLoginWnd, LogoTex, 0xA4);
NW_ASSERT_OFFSET(NCLoginWnd, OptionsButton, 0xB4);
NW_ASSERT_SIZE(NCLoginWnd, 0xB8);

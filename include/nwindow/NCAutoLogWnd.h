#pragma once
// NCAutoLogWnd - a development screen that logs in by itself (UClass "CAutoLogWnd", 0xAC):
// black, a progress line, and on its first paint the login with the account and password of
// AutoLog.txt ([AutoLogOn] L2ID and L2Passwd, "zodiac" when missing; L2Slot is read too, and
// nothing uses it). Nothing in NWindow creates it, and nothing moves Step past the first line.

#include "nwindow/NCWnd.h"

// what OnCreate reads from AutoLog.txt (names ours), and the progress lines: "1.Authorizing....",
// "2.Send infomations of character to server..", "3.Loading World......", "Can not connect to
// server" (name ours)
#if NW_HOOK_MODE
#define GAutoLogID       (reinterpret_cast<TCHAR*>(::nw::Addr(0x1012b858)))   // TCHAR[64]
#define GAutoLogPassword (reinterpret_cast<TCHAR*>(::nw::Addr(0x1012b7d8)))   // TCHAR[64]
#define GAutoLogSlot     NW_GLOBAL(INT, 0x1012b8d8)
#define GAutoLogMessages (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x1010ced0)))
#else
extern TCHAR GAutoLogID[64];
extern TCHAR GAutoLogPassword[64];
extern INT GAutoLogSlot;
extern const TCHAR* const GAutoLogMessages[];
#endif

class NCAutoLogWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAutoLogWnd, NCWnd, 0, 0x1012b8e0)

    UTexture* BackTex;               // +0x94 "NWindow.BlackTexture"
    UBOOL bLoginSent;                // +0x98
    INT Unknown9C;                   // +0x9C (0)
    INT Step;                        // +0xA0 lines 0..Step show; 3 shows the fourth alone
    INT TextX;                       // +0xA4 40% of the width
    INT TextY;                       // +0xA8 a quarter of the height

    NCAutoLogWnd();                                                  // 0x10037440
    // ~NCAutoLogWnd                                                 // 0x100374a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10037360
    virtual INT OnDestroy();                                         // 36 0x10083930 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100374f0
};
NW_ASSERT_OFFSET(NCAutoLogWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCAutoLogWnd, Step, 0xA0);
NW_ASSERT_SIZE(NCAutoLogWnd, 0xAC);

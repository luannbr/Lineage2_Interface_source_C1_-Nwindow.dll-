#pragma once
// NCWarningWnd - the login screen's "Warning" box (UClass "CWarningWnd", 0xA8; NCLoginWnd
// creates it): a text in an NCNoticeInfoWnd and an OK button (or Enter) that asks for the
// server list. A copy of NCNoticeWnd's code: even its OnPaint guard is NCNoticeWnd's.

#include "nwindow/NCWnd.h"

class NCButton;
class NCNoticeInfoWnd;

class NCWarningWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCWarningWnd, NCWnd, 0, 0x1013c9c0)

    INT TimerResult;                 // +0x94 SetTimer's (ShowWindow)
    INT Unknown98;                   // +0x98 (0) cleared by ShowWindow / HideWindow
    UTexture* BackTex;               // +0x9C "L2UI.etcWnd.Announce_back" below the title bar
    NCNoticeInfoWnd* InfoWnd;        // +0xA0 the text
    NCButton* OKButton;              // +0xA4

    NCWarningWnd();                                                  // 0x10079d70
    // ~NCWarningWnd                                                 // 0x10079dc0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10079580
    virtual INT OnTimer(INT TimerID) { return 1; }                   // 34 0x10002830 (folded)
    virtual INT OnCreate();                                          // 35 0x10079e10
    virtual INT OnDestroy();                                         // 36 0x1007a020
    virtual void ShowWindow();                                       // 42 0x10079690
    virtual void HideWindow();                                       // 43 0x100796c0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1007a090
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10079640
    // names ours
    virtual INT OnOK();                                              // 62 0x10079620 server list

    void SetText(const TCHAR* Text);                                 // 0x100795f0 empty: ignored
    void ClearText();                                                // 0x10079610
};
NW_ASSERT_OFFSET(NCWarningWnd, TimerResult, 0x94);
NW_ASSERT_OFFSET(NCWarningWnd, BackTex, 0x9C);
NW_ASSERT_OFFSET(NCWarningWnd, OKButton, 0xA4);
NW_ASSERT_SIZE(NCWarningWnd, 0xA8);

#pragma once
// NCNoticeWnd - the announcements box of the login screen (UClass "CNoticeWnd", 0xA4;
// NCLoginWnd +0x98): the text the login server sends, scrolling in an NCNoticeInfoWnd, and
// OK (or Enter), which goes on to the warning box when the notice says so (Type bit 0) or
// else asks for the server list.

#include "nwindow/NCScrollWnd.h"

class NCButton;

// NCNoticeInfoWnd - the scrolling text of the box (UClass "CNoticeInfoWnd", 0x200CC)
class NCNoticeInfoWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCNoticeInfoWnd, NCScrollWnd, 0, 0x1013cef0)

    TCHAR Text[65536];               // +0xCC

    NCNoticeInfoWnd();                                               // 0x100796e0
    // ~NCNoticeInfoWnd                                              // 0x10079740

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90 (folded)
    virtual INT OnCreate();                                          // 35 0x10079480
    virtual INT OnDestroy();                                         // 36 0x10079490
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10079790

    // names ours
    void SetText(const TCHAR* InText);                               // 0x100794a0
    void ClearText();                                                // 0x100794d0
};
NW_ASSERT_OFFSET(NCNoticeInfoWnd, Text, 0xCC);
NW_ASSERT_SIZE(NCNoticeInfoWnd, 0x200CC);

class NCNoticeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCNoticeWnd, NCWnd, 0, 0x1013c4a0)

    INT Type;                        // +0x94 bit 0: the warning box comes next
    UTexture* BackTex;               // +0x98 "L2UI.etcWnd.Announce_back"
    NCNoticeInfoWnd* InfoWnd;        // +0x9C
    NCButton* OKButton;              // +0xA0

    NCNoticeWnd();                                                   // 0x100798c0
    // ~NCNoticeWnd                                                  // 0x10079910

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100794e0
    virtual INT OnCreate();                                          // 35 0x10079960
    virtual INT OnDestroy();                                         // 36 0x10079b70
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10079be0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10079d00
    virtual INT OnOK();                                              // 62 0x10079cc0 (name ours)

    // names ours
    void SetText(const TCHAR* InText);                               // 0x10079550
    void ClearText();                                                // 0x10079570
};
NW_ASSERT_OFFSET(NCNoticeWnd, BackTex, 0x98);
NW_ASSERT_OFFSET(NCNoticeWnd, OKButton, 0xA0);
NW_ASSERT_SIZE(NCNoticeWnd, 0xA4);

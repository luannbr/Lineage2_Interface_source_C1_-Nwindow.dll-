#pragma once
// NCHtmlEdit / NCJHtmlEdit - the <edit> boxes of the HTML pages (UClass "CHtmlEdit" and
// "CJHtmlEdit", 0x23C each, both straight from NCEditBox). Enter sends EnterCommand to the
// viewer instead of NCEditBox's codes; the value is the text. NCHtmlEdit draws "NWindow.chatback"
// stretched instead of the edit frame, clipped to the viewer's height, and a click scrolls it
// into view; NCJHtmlEdit paints like NCEditBox (nothing in NWindow creates one).

#include "nwindow/NCEditBox.h"

class NCHtmlEdit : public NCEditBox
{
    NW_DECLARE_CLASS(NCHtmlEdit, NCEditBox, 0, 0x101293d8)

    TCHAR* EnterCommand;             // +0x238 (name ours) sent on Enter, NULL = NCEditBox's keys

    NCHtmlEdit(INT InExtraWidth = 0, INT InType = 0);                        // 0x100326e0
    // ~NCHtmlEdit                                                   // 0x100327a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100301a0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100327f0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10030160
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100310a0
    virtual const TCHAR* GetValue();                                 // 62 0x100301e0
};
NW_ASSERT_OFFSET(NCHtmlEdit, EnterCommand, 0x238);
NW_ASSERT_SIZE(NCHtmlEdit, 0x23C);

class NCJHtmlEdit : public NCEditBox
{
    NW_DECLARE_CLASS(NCJHtmlEdit, NCEditBox, 0, 0x10128980)

    TCHAR* EnterCommand;             // +0x238

    NCJHtmlEdit(INT Unused1 = 0, INT Unused2 = 1, INT Unused3 = 1);              // 0x10032f10
    // ~NCJHtmlEdit                                                  // 0x10032f70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100301a0 (shared)
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10030160 (shared)
    virtual const TCHAR* GetValue();                                 // 62 0x100301e0 (shared)
};
NW_ASSERT_OFFSET(NCJHtmlEdit, EnterCommand, 0x238);
NW_ASSERT_SIZE(NCJHtmlEdit, 0x23C);

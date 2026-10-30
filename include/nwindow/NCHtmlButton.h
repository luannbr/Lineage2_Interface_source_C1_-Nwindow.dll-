#pragma once
// NCHtmlButton / NCHtmlPushButton - the buttons of the HTML pages (UClass "CHtmlButton" 0x220,
// "CHtmlPushButton" 0x228). They paint like NCButton but clip to the window they live in, so a
// page scrolled under the viewer's edge does not show them: NCHtmlButton to the enclosing
// NCHtmlViewer, NCHtmlPushButton to its first ancestor that is not a control (the scroll
// buttons of an HTML edit box). A click on an NCHtmlButton reports Command to the viewer.

#include "nwindow/NCButton.h"

class NCHtmlButton : public NCButton
{
    NW_DECLARE_CLASS(NCHtmlButton, NCButton, 0, 0x10129e20)

    const TCHAR* Command;      // +0x21C (name ours) sent on a click (NWM_COMMAND lParam): the
                               //   <button> action

    NCHtmlButton();                                                             // 0x10030840
    NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName, const TCHAR* InCommand,
                 UBOOL bInAcceptKey);                                           // 0x100308e0
    NCHtmlButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);       // 0x10030910
    // ~NCHtmlButton                                                            // 0x10030890

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);             // 24 0x100301f0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                                     // 50 0x10030940
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);                      // 57 0x1002fee0
};
NW_ASSERT_OFFSET(NCHtmlButton, Command, 0x21C);
NW_ASSERT_SIZE(NCHtmlButton, 0x220);

class NCHtmlPushButton : public NCPushButton
{
    NW_DECLARE_CLASS(NCHtmlPushButton, NCPushButton, 0, 0x10128eb8)

    NCHtmlPushButton();                                                         // 0x10030c20
    NCHtmlPushButton(const TCHAR* NormalTexName, const TCHAR* PushedTexName);   // 0x10030cc0
    // ~NCHtmlPushButton                                                        // 0x10030c70

    virtual INT OnPaint(NCGDevice* Device);                                     // 50 0x10030d20
};
NW_ASSERT_SIZE(NCHtmlPushButton, 0x228);

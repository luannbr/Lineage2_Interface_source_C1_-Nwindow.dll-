#pragma once
// NCMessageWnd - a line of text on a background (UClass "CMessageWnd", 0x498): the console's
// message window (NConsoleWnd +0x36D8, NConsoleWnd::ShowMessageWnd), e.g. while the login
// waits for the server list. The text is centered, or drawn from (50, 8) and wrapped to the
// screen width when it is wider than the window.

#include "nwindow/NCWnd.h"

class NCMessageWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMessageWnd, NCWnd, 0, 0x1013ba50)

    UTexture* BackTex;               // +0x94 "sek.cbui141" stretched over the window
    TCHAR Text[512];                 // +0x98

    NCMessageWnd();                                                  // 0x10078000
    // ~NCMessageWnd                                                 // 0x10078060

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (NControl's body)
    virtual INT OnCreate();                                          // 35 0x10077fa0
    virtual INT OnDestroy();                                         // 36 0x10083930 (NCCompositeBox's body)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100780b0

    // names ours
    void ClearText();                                                // 0x10077fd0 (folded)
    void SetText(const TCHAR* NewText);                              // 0x10077fe0 NULL keeps the text
};
NW_ASSERT_OFFSET(NCMessageWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCMessageWnd, Text, 0x98);
NW_ASSERT_SIZE(NCMessageWnd, 0x498);

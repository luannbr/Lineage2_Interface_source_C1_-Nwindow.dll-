#pragma once
// NCTestUIWnd - a test window (UClass "CTestUIWnd", 0xD0): a 512 px title bar over the chat
// background stretched on a scroll window. Nothing in NWindow creates it.

#include "nwindow/NCScrollWnd.h"

class NCTestUIWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCTestUIWnd, NCScrollWnd, 0, 0x10139648)

    UTexture* BackTex;               // +0xCC "NWindow.ChatBack"

    NCTestUIWnd();                                                   // 0x100759d0
    // ~NCTestUIWnd                                                  // 0x10075a20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90 (folded)
    virtual INT OnCreate();                                          // 35 0x10075a70
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10075b30
};
NW_ASSERT_OFFSET(NCTestUIWnd, BackTex, 0xCC);
NW_ASSERT_SIZE(NCTestUIWnd, 0xD0);

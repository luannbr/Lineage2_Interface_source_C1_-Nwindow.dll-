#pragma once
// NCLoadingWnd - the loading screen while entering the world (UClass "CLoadingWnd", 0x98): a
// 1024 x 768 picture stretched over the whole window. NConsoleWnd::RequestGameStart picks the
// picture: in Korean one per age rating of the chosen server, elsewhere the country's entry of
// FontInfo.ini.

#include "nwindow/NCWnd.h"

class NCLoadingWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLoadingWnd, NCWnd, 0, 0x101343e0)

    UTexture* LoadingTex;            // +0x94

    NCLoadingWnd();                                                  // 0x10068a20
    // ~NCLoadingWnd                                                 // 0x10068a70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate() { return 1; }                             // 35 0x10005230 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10068ac0

    void SetLoadingTexture(INT ScreenID);                            // 0x10068910 (name ours)
};
NW_ASSERT_OFFSET(NCLoadingWnd, LoadingTex, 0x94);
NW_ASSERT_SIZE(NCLoadingWnd, 0x98);

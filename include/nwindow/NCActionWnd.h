#pragma once
// NCActionWnd - the action window (UClass "CActionWnd", 0xA8; a page of the character window,
// desktop +0x94 +0x98): tabs Basic (the action icons, NCActionItemListWnd) and Clan
// (NCPledgeWnd). The Clan tab is disabled without a clan; shown with one, the window asks the
// server for the member list.

#include "nwindow/NCWnd.h"

class NCActionItemListWnd;
class NCPledgeWnd;
class NCTabControl;

enum { NID_ACTION_TABS = 0xF00A };

class NCActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionWnd, NCWnd, 0, 0x1012a348)

    UTexture* BackTex;                     // +0x94 "sek.cbui304" (loaded, never drawn)
    NCActionItemListWnd* ActionList;       // +0x98
    NCPledgeWnd* PledgeWnd;                // +0x9C
    NCTabControl* TabCtrl;                 // +0xA0
    NCWnd* UnusedA4;                       // +0xA4 destroyed, never created

    NCActionWnd();                                                   // 0x100364c0
    // ~NCActionWnd                                                  // 0x10036510

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10036560
    virtual INT OnDestroy();                                         // 36 0x100367a0 (folded)
    virtual void ShowWindow();                                       // 42 0x10037330
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005f200

    // names ours
    void ResetPledgeWnd();                                           // 0x10036870 (the console)
    void ReloadActions();                                            // 0x10036e20 texts reloaded
    void UpdatePledgeTab();                                          // 0x10037250 (the console)
};
NW_ASSERT_OFFSET(NCActionWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCActionWnd, TabCtrl, 0xA0);
NW_ASSERT_SIZE(NCActionWnd, 0xA8);

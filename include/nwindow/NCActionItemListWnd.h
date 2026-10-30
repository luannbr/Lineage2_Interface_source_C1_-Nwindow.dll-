#pragma once
// NCActionItemListWnd - the Basic page of the action window (UClass "CActionItemListWnd",
// 0xA4): three icon grids, filled from actionname-*.dat by category (1 basic, 2 party,
// 3 social), under the labels "Party" and "Social".

#include "nwindow/NCWnd.h"

class NCActionItemWnd;

class NCActionItemListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionItemListWnd, NCWnd, 0, 0x1012b2b8)

    NCActionItemWnd* BasicGrid;            // +0x94 6 x 2
    NCActionItemWnd* PartyGrid;            // +0x98 6 x 1
    NCActionItemWnd* SocialGrid;           // +0x9C 6 x 3
    UTexture* BackTex;                     // +0xA0 "L2UI.ActionWnd.ActionBack"

    NCActionItemListWnd();                                           // 0x10034d10
    // ~NCActionItemListWnd                                          // 0x10034d60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10037060
    virtual INT OnDestroy();                                         // 36 0x10034eb0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10034db0

    void LoadActions();                                              // 0x10036c70 (name ours)
};
NW_ASSERT_OFFSET(NCActionItemListWnd, BasicGrid, 0x94);
NW_ASSERT_OFFSET(NCActionItemListWnd, BackTex, 0xA0);
NW_ASSERT_SIZE(NCActionItemListWnd, 0xA4);

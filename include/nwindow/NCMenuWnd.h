#pragma once
// NCMenuWnd - the main menu bar (UClass "CMenuWnd", 0xA8): four big icons, each opening or
// closing one of the desktop's windows (Character Status Alt+I, Inventory Alt+V, Community
// Alt+B, System Alt+X), and a thin title bar on the left to drag it.

#include "nwindow/NCWnd.h"

class NCButton;

// NWM_COMMAND ids of the buttons and the title bar
enum
{
    NID_MENU_INVENTORY = 0xF0F000,
    NID_MENU_CHARACTER = 0xF0F001,
    NID_MENU_SYSTEM = 0xF0F003,
    NID_MENU_COMMUNITY = 0xF0F006,
    NID_MENU_FRAME = 0xF0F021,
};

class NCMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMenuWnd, NCWnd, 0, 0x1013b008)

    UTexture* BackTex;                     // +0x94 "L2UI.EtcWndBack.SysMenuBack"
    NCButton* SystemButton;                // +0x98
    NCButton* InventoryButton;             // +0x9C
    NCButton* CharacterButton;             // +0xA0
    NCButton* CommunityButton;             // +0xA4

    NCMenuWnd();                                                     // 0x10077ab0
    // ~NCMenuWnd                                                    // 0x10077b20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10076db0
    virtual INT OnCreate();                                          // 35 0x10077bf0
    virtual INT OnDestroy();                                         // 36 0x100367a0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10077b70
    // names ours
    virtual INT OnSystem();                                          // 62 0x10076e80
    virtual INT OnCharacter();                                       // 63 0x10076ea0
    virtual INT OnInventory();                                       // 64 0x10076ec0
    virtual INT OnCommunity();                                       // 65 0x10076ee0
};
NW_ASSERT_OFFSET(NCMenuWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCMenuWnd, CommunityButton, 0xA4);
NW_ASSERT_SIZE(NCMenuWnd, 0xA8);

#pragma once
// The GM's windows. NCGMWnd (UClass "CGMWnd", 0xC0; desktop +0x114, //gmwnd): a player's name
// and six buttons that ask the server about that player and open the matching view window of
// the desktop (NCConsole::ToggleGM*): Status (NCGMDetailStatusWnd), Inventory, Skills
// (NCGMMagicSkillWnd), Quest, Warehouse (NCGMStoreWnd) and Clan; the seventh button closes the
// view windows.
//
// The view windows are the player's windows under a title bar, filled by the console with the
// server's answer and made read-only: nothing is dragged out of their grids (the item windows'
// release and right button only put the carried item down), the quest window's Cancel is
// disabled, the clan page's buttons are all disabled.
//   NCGMInventoryWnd (0xB0; desktop +0x118): the inventory's paperdoll and two pages with the
//     adena, no weight bar, no trash or crystallize icons.
//   NCGMQuestWnd (0xCC; desktop +0x124), NCGMActionWnd (0x9C; desktop +0x12C, the clan page
//     NCGMPledgeWnd only).

#include "nwindow/NCEquipItemWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NCPledgeWnd.h"
#include "nwindow/NCQuestWnd.h"

class NCButton;
class NCEditBox;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids (names ours)
enum
{
    NID_GM_NAME = 0x1505,
    NID_GM_INVENTORY = 0xF0F006,
    NID_GM_STATUS = 0xF0F007,
    NID_GM_SKILLS = 0xF0F008,
    NID_GM_QUEST = 0xF0F00A,
    NID_GM_CLAN = 0xF0F00B,
    NID_GM_WAREHOUSE = 0xF0F00C,
    NID_GM_FRAME = 0xF0F015,
    NID_GM_CLOSE = 0xF0F016,
    NID_GMINVENTORY_FRAME = 0xF0F00D,
    NID_GMINVENTORY_TABS = 0xF0F00E,
    NID_GMACTION_FRAME = 0xF0F018,
    NID_GMQUEST_FRAME = 0xF0F119,
    NID_GMQUEST_TABS = 0xF0F120,
    NID_GMQUEST_CANCEL = 0xF0F121,     // disabled, and the window procedure ignores it
    NID_GMQUEST_LIST = 0xF0F122,       // not handled: a quest clicked shows nothing
};

class NCGMWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMWnd, NCWnd, 0, 0x10131580)

    UTexture* TopTex;                // +0x94 "sek.ui02"
    UTexture* MidTex;                // +0x98 "sek.cbui157"
    UTexture* BottomTex;             // +0x9C "sek.ui03"
    NCEditBox* NameEdit;             // +0xA0 the player's name (ToggleGMWnd focuses it)
    NCButton* InventoryButton;       // +0xA4
    NCButton* SkillsButton;          // +0xA8
    NCButton* StatusButton;          // +0xAC
    NCButton* QuestButton;           // +0xB0
    NCButton* WarehouseButton;       // +0xB4
    NCButton* ClanButton;            // +0xB8
    NCButton* CloseButton;           // +0xBC

    NCGMWnd();                                                       // 0x1005f250
    // ~NCGMWnd                                                      // 0x1005f2d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005c0e0
    virtual INT OnCreate();                                          // 35 0x1005f320
    virtual INT OnDestroy();                                         // 36 0x1005f9c0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005fb40
    // the buttons (names ours)
    virtual INT OnInventory();                                       // 62 0x1005c210
    virtual INT OnSkills();                                          // 63 0x1005c270
    virtual INT OnStatus();                                          // 64 0x1005c2d0
    virtual INT OnQuest();                                           // 65 0x1005c330
    virtual INT OnWarehouse();                                       // 66 0x1005c390
    virtual INT OnClan();                                            // 67 0x1005c3f0
    virtual INT OnClose();                                           // 68 0x1005c450
};
NW_ASSERT_OFFSET(NCGMWnd, NameEdit, 0xA0);
NW_ASSERT_OFFSET(NCGMWnd, CloseButton, 0xBC);
NW_ASSERT_SIZE(NCGMWnd, 0xC0);

// the GM inventory's pages (UClass "CGMInvenItemWnd", 0x13C)
class NCGMInvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCGMInvenItemWnd, NCInvenItemWnd, 0, 0x101300f8)

    NCGMInvenItemWnd();                                              // 0x1005ffb0
    NCGMInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10060050
    // ~NCGMInvenItemWnd                                             // 0x10060000

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005bca0
    // NCGMSkillItemWnd::OnLButtonUp's body (0x1005bdc0, hooked there): unhooked copies
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1005bdc0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x1005bdc0 (folded)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1005bdc0 (folded)
};
NW_ASSERT_SIZE(NCGMInvenItemWnd, 0x13C);

// the GM inventory's paperdoll (UClass "CGMEquipItemWnd", 0x254)
class NCGMEquipItemWnd : public NCEquipItemWnd
{
    NW_DECLARE_CLASS(NCGMEquipItemWnd, NCEquipItemWnd, 0, 0x10131060)

    NCGMEquipItemWnd(INT InCellSize = 32);                                // 0x1005ff10
    // ~NCGMEquipItemWnd                                             // 0x1005ff60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005bc40
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1005bc80
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x1005bc80 (folded)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 62 0x1005bc80 (folded)
};
NW_ASSERT_SIZE(NCGMEquipItemWnd, 0x254);

class NCGMInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMInventoryWnd, NCWnd, 0, 0x10132f48)

    UTexture* BackTex;               // +0x94 "L2UI.InventoryWnd.InventoryBack"
    UTexture* AdenaTex;              // +0x98 "L2UI.WindowIcon.WindowIconAdena"
    INT Unknown9C;                   // +0x9C
    NCGMInvenItemWnd* ItemWnd[2];    // +0xA0 the pages: 0 "Item", 1 "Quest"
    NCGMEquipItemWnd* EquipWnd;      // +0xA8
    NCTabControl* TabCtrl;           // +0xAC

    NCGMInventoryWnd();                                              // 0x1005c730
    // ~NCGMInventoryWnd                                             // 0x1005c790

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10060080
    virtual INT OnDestroy();                                         // 36 0x1005c980
    virtual void ShowWindow();                                       // 42 0x1005bd10
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005c7e0

    // the console's calls (names ours)
    void ClearItems();                                               // 0x1005bce0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1005bd30
};
NW_ASSERT_OFFSET(NCGMInventoryWnd, ItemWnd, 0xA0);
NW_ASSERT_OFFSET(NCGMInventoryWnd, TabCtrl, 0xAC);
NW_ASSERT_SIZE(NCGMInventoryWnd, 0xB0);

class NCGMQuestWnd : public NCQuestWnd
{
    NW_DECLARE_CLASS(NCGMQuestWnd, NCQuestWnd, 0, 0x10131fd0)

    NCGMQuestWnd();                                                  // 0x1005e310
    // ~NCGMQuestWnd                                                 // 0x1005e360

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005be70
    virtual INT OnCreate();                                          // 35 0x1005e3b0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005e830
    virtual UBOOL IsInDropZone(INT X, INT Y) { return 0; }           // 62 0x100016f0 (folded)
    virtual INT OnTab(INT Page);                                     // 66 0x1005bee0
};
NW_ASSERT_SIZE(NCGMQuestWnd, 0xCC);

// the GM's clan page (UClass "CGMPledgeWnd", 0x3B0): drawn 17 pixels lower than the player's
class NCGMPledgeWnd : public NCPledgeWnd
{
    NW_DECLARE_CLASS(NCGMPledgeWnd, NCPledgeWnd, 0, 0x10132a28)

    NCGMPledgeWnd();                                                 // 0x1005eac0
    // ~NCGMPledgeWnd                                                // 0x1005eb10

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005bfc0
    virtual INT OnDestroy();                                         // 36 0x1005c000
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005eb60
};
NW_ASSERT_SIZE(NCGMPledgeWnd, 0x3B0);

class NCGMActionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMActionWnd, NCWnd, 0, 0x10131aa8)

    UTexture* BackTex;               // +0x94 "L2UI.AcitonWnd.PledgeBack" (sic; never drawn)
    NCGMPledgeWnd* PledgeWnd;        // +0x98

    NCGMActionWnd();                                                 // 0x1005ef40
    // ~NCGMActionWnd                                                // 0x1005ef90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x1005efe0
    virtual INT OnDestroy();                                         // 36 0x100bb540 (shared)
    virtual void ShowWindow();                                       // 42 0x1005c010
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005f200 (shared)
};
NW_ASSERT_OFFSET(NCGMActionWnd, PledgeWnd, 0x98);
NW_ASSERT_SIZE(NCGMActionWnd, 0x9C);

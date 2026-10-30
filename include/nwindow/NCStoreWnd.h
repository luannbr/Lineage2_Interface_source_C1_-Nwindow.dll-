#pragma once
// NCStoreWnd - the warehouse window (UClass "CStoreWnd", 0xC8): two grids with the arrows
// between them, the fee and the player's adena under them, OK and Cancel. Items go from one
// grid to the other with the arrows, a drag or a double click (how many first, for a stack).
// Mode 0 deposits: the inventory on top, what goes into the warehouse below (30 adena a slot);
// mode 1 withdraws: the warehouse's items on top, what comes back below (no fee). OK sends the
// lower grid's list to the server. The console opens the window in one mode or the other
// (Clear, SetMode, SetAdena) and fills the grids (AddInventoryItem / AddStoredItem).
//
// NCStoreItemWnd - its grids (UClass "CStoreItemWnd", 0x124): an item goes to Target, the
// other grid (a drop onto another warehouse grid makes that one the target); an entry's count
// is the amount it holds (NItemEntry::TradeCount). The tool tips are the inventory's
// (NItemToolTip.h).
//
// Desktop (NCConsole) +0xCC is this window.
//
// NCGMStoreWnd - the same window for a GM looking at a character's warehouse (UClass
// "CGMStoreWnd", 0xC8): the arrows, OK and Cancel are disabled.

#include "nwindow/NCItemWnd.h"

class NCButton;

// NWM_COMMAND ids (names ours; NCShopWnd shares this window procedure)
enum
{
    NID_STORE_FRAME = 0xF3,
    NID_STORE_OK = 0xF4,
    NID_STORE_CANCEL = 0xF5,
    NID_STORE_UP = 0xF6,
    NID_STORE_DOWN = 0xF7,
    NID_STORE_CLOSE = 0xF8,          // handled, but no control of the window sends it
};

// NCStoreWnd::Mode (names ours)
enum
{
    STOREMODE_Deposit = 0,
    STOREMODE_Withdraw = 1,
};

class NCStoreItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCStoreItemWnd, NCItemWnd, 0, 0x1014f980)

    NCStoreItemWnd* Target;          // +0x118 where its items go (the other grid)
    INT Mode;                        // +0x11C the window's (STOREMODE_*); the shop's grid reads
                                     //   its copy, this one does not
    INT Side;                        // +0x120 0 the warehouse's grid, 1 the inventory's (not
                                     //   read here either)

    NCStoreItemWnd();                                                // 0x100b69b0
    NCStoreItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100b6a50
    // ~NCStoreItemWnd                                               // 0x100b6a00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10057b10 (shared)
    virtual INT OnDestroy();                                         // 36 0x100a39e0 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100b6a90
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x100b3f50
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x100b6c70 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x100b6dc0

    // names ours; the three setters are one body each with other grids' (shops, private
    // stores), the last two hooked here
    void SetTarget(NCStoreItemWnd* InTarget);                        // 0x10057ba0 (shared)
    void SetMode(INT InMode);                                        // 0x100b3880 (shared)
    void SetSide(INT InSide);                                        // 0x100b3890 (shared)
    void SetToolTip(NItemEntry* Item);                               // 0x100b38a0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100b60e0 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x100b61e0 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x100b62b0 to Target
};
NW_ASSERT_OFFSET(NCStoreItemWnd, Target, 0x118);
NW_ASSERT_OFFSET(NCStoreItemWnd, Side, 0x120);
NW_ASSERT_SIZE(NCStoreItemWnd, 0x124);

class NCStoreWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCStoreWnd, NCWnd, 0, 0x1014fea8)

    NCStoreItemWnd* InventoryItems;  // +0x94 the inventory's side: on top when depositing,
                                     //   below (what comes back) when withdrawing
    NCStoreItemWnd* StoredItems;     // +0x98 the warehouse's side: below (what goes in) when
                                     //   depositing, on top when withdrawing
    INT Unknown9C;                   // +0x9C
    UTexture* BackTex;               // +0xA0 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                   // +0xA4
    NCButton* UpButton;              // +0xA8
    NCButton* DownButton;            // +0xAC
    NCButton* OKButton;              // +0xB0
    NCButton* CancelButton;          // +0xB4
    NCWnd* UnknownB8;                // +0xB8 OnDestroy's, never created
    INT Mode;                        // +0xBC STOREMODE_*
    INT Adena;                       // +0xC0 the player's
    INT Fee;                         // +0xC4 UpdateFee

    NCStoreWnd();                                                    // 0x100b58b0
    // ~NCStoreWnd                                                   // 0x1005c630

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100b56e0 (shared)
    virtual INT OnCreate();                                          // 35 0x100b6e00
    virtual INT OnDestroy();                                         // 36 0x100a6300 (shared)
    virtual void ShowWindow();                                       // 42 0x100b5d80
    virtual void HideWindow();                                       // 43 0x100b5840
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b5910
    // names ours
    virtual INT OnOK();                                              // 62 0x100b5d90
    virtual INT OnCancel();                                          // 63 0x100b5890
    virtual INT OnUp();                                              // 64 0x100b63f0
    virtual INT OnDown();                                            // 65 0x100b66d0
    virtual INT OnClose();                                           // 66 0x100b5890 (OnCancel's body)

    // names ours (the console calls most of these)
    void SetMode(INT InMode);                                        // 0x100b57d0 the grids' too
    void SetAdena(INT InAdena);                                      // 0x100b5800 (shared)
    void Clear();                                                    // 0x100b5810 grids emptied,
                                                                     //   adena and fee 0
    void UpdateFee();                                                // 0x100b5850
    INT GetMode();                                                   // 0x100b5880
    void UpdateLayout();                                             // 0x100b5c80 the grids placed
                                                                     //   by mode
    void ReloadTexts();                                              // 0x100b5fc0 item names and
                                                                     //   descriptions, tool tips
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x100b63d0
    UBOOL AddStoredItem(NItemInfo* Info);                            // 0x100b63e0
};
NW_ASSERT_OFFSET(NCStoreWnd, BackTex, 0xA0);
NW_ASSERT_OFFSET(NCStoreWnd, Mode, 0xBC);
NW_ASSERT_SIZE(NCStoreWnd, 0xC8);

class NCGMStoreWnd : public NCStoreWnd
{
    NW_DECLARE_CLASS(NCGMStoreWnd, NCStoreWnd, 0, 0x1012fbd8)

    NCGMStoreWnd();                                                  // 0x1005e9b0
    // ~NCGMStoreWnd                                                 // 0x1005ea00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1005bf30
    virtual INT OnCreate();                                          // 35 0x1005bf70
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005ea50
};
NW_ASSERT_SIZE(NCGMStoreWnd, 0xC8);

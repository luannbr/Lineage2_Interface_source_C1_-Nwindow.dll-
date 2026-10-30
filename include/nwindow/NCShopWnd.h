#pragma once
// NCShopWnd - an NPC's shop (UClass "CShopWnd", 0xD0): two grids with the arrows between
// them, the total and the player's adena under them, OK and Cancel. Mode 0 buys: the NPC's
// goods on top (they stay there; a stack asks how many whatever its count), what is bought
// below; mode 1 sells: the inventory on top, what is sold below (at most what there is). The
// total is the price times the amount of each item below, in 64 bits (drawn with "%d": its
// low half). OK sends the lower grid's list to the server (RequestBuyItem /
// RequestSellItem). The console opens the window in one mode or the other (Clear, SetMode,
// SetListID, SetAdena) and fills the grids (AddShopItem / AddPlayerItem).
//
// NCShopItemWnd - its grids (UClass "CShopItemWnd", 0x124): the warehouse's (NCStoreWnd.h)
// with the prices (NItemEntry +0xC20) in the tool tips, and the rules of the mode and side
// when an item moves.
//
// Desktop (NCConsole) +0xD0 is this window.

#include "nwindow/NCItemWnd.h"

class NCButton;

// NWM_COMMAND ids (names ours): NCStoreWnd's (NID_STORE_*), the window procedure is one body

// NCShopWnd::Mode, NCShopItemWnd::Mode and Side (names ours)
enum
{
    SHOPMODE_Buy = 0,
    SHOPMODE_Sell = 1,
    SHOPSIDE_Shop = 0,               // the NPC's goods when buying, what is sold when selling
    SHOPSIDE_Player = 1,             // what is bought when buying, the inventory when selling
};

class NCShopItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCShopItemWnd, NCItemWnd, 0, 0x1014a720)

    NCShopItemWnd* Target;           // +0x118 where its items go (the other grid)
    INT Mode;                        // +0x11C the window's (SHOPMODE_*)
    INT Side;                        // +0x120 SHOPSIDE_*

    NCShopItemWnd();                                                 // 0x100a7480
    NCShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100a7520
    // ~NCShopItemWnd                                                // 0x100a74d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10057b10 (shared)
    virtual INT OnDestroy();                                         // 36 0x100a39e0 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100a7560
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x100a4540
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x100a7860 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x100a7ab0

    // names ours
    void SetTarget(NCShopItemWnd* InTarget);                         // 0x10057ba0 (shared)
    void SetMode(INT InMode);                                        // 0x100b3880 (shared)
    void SetSide(INT InSide);                                        // 0x100b3890 (shared)
    void SetToolTip(NItemEntry* Item);                               // 0x100a39f0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100a69b0 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x100a6ab0 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x100a6b80 to Target
};
NW_ASSERT_OFFSET(NCShopItemWnd, Target, 0x118);
NW_ASSERT_OFFSET(NCShopItemWnd, Side, 0x120);
NW_ASSERT_SIZE(NCShopItemWnd, 0x124);

class NCShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCShopWnd, NCWnd, 0, 0x1014ac48)

    NCShopItemWnd* PlayerItems;      // +0x94 SHOPSIDE_Player: below when buying, on top when
                                     //   selling
    NCShopItemWnd* ShopItems;        // +0x98 SHOPSIDE_Shop: on top when buying, below when
                                     //   selling
    INT Unknown9C;                   // +0x9C
    UTexture* BackTex;               // +0xA0 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                   // +0xA4
    NCButton* UpButton;              // +0xA8
    NCButton* DownButton;            // +0xAC
    NCButton* OKButton;              // +0xB0
    NCButton* CancelButton;          // +0xB4
    NCWnd* UnknownB8;                // +0xB8 OnDestroy's, never created
    INT Mode;                        // +0xBC SHOPMODE_*
    INT ListID;                      // +0xC0 (-1) the NPC's list, sent back with the request
    INT Adena;                       // +0xC4 the player's
    SQWORD TotalPrice;               // +0xC8 UpdateTotalPrice

    NCShopWnd();                                                     // 0x100a5db0
    // ~NCShopWnd                                                    // 0x100a5e50

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100b56e0 (shared)
    virtual INT OnCreate();                                          // 35 0x100a7b00
    virtual INT OnDestroy();                                         // 36 0x100a6300
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual void HideWindow();                                       // 43 0x100a5d80
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a5ea0
    // names ours
    virtual INT OnOK();                                              // 62 0x100a6550
    virtual INT OnCancel();                                          // 63 0x100a5d90
    virtual INT OnUp();                                              // 64 0x100a6ed0
    virtual INT OnDown();                                            // 65 0x100a71b0
    virtual INT OnClose();                                           // 66 0x100a5d90 (OnCancel's body)

    // names ours (the console calls most of these)
    void SetAdena(INT InAdena);                                      // 0x100a5d20
    void SetListID(INT InListID);                                    // 0x100b5800 (shared)
    void Clear();                                                    // 0x100a5d30 grids emptied,
                                                                     //   list id -1, adena and
                                                                     //   total 0
    void UpdateLayout();                                             // 0x100a6450 the grids placed
                                                                     //   by mode
    void UpdateTotalPrice();                                         // 0x100a67a0
    void ReloadTexts();                                              // 0x100a6890 item names and
                                                                     //   descriptions, tool tips
    void SetMode(INT InMode);                                        // 0x100a6e70 the grids' too,
                                                                     //   then the layout
    UBOOL AddShopItem(NItemInfo* Info);                              // 0x100a6eb0
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x100a6ec0
};
NW_ASSERT_OFFSET(NCShopWnd, BackTex, 0xA0);
NW_ASSERT_OFFSET(NCShopWnd, Mode, 0xBC);
NW_ASSERT_OFFSET(NCShopWnd, TotalPrice, 0xC8);
NW_ASSERT_SIZE(NCShopWnd, 0xD0);

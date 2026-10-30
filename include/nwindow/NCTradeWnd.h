#pragma once
// NCTradeWnd - the trade window (UClass "CTradeWnd", 0x100): the player's inventory on top,
// the player's offer and the partner's under it with their names, a down arrow that offers the
// selected inventory item, OK and Cancel (both answer the server: RequestTradeDone). The
// console fills it: Reset and the names when a trade starts, the three grids from the server's
// lists, UpdateInventoryItem for changes to the inventory; the texts again after a language
// change (ReloadTexts).
//
// NCTradeItemWnd - those grids (UClass "CTradeItemWnd", 0x11C). Its UClass names NCWnd as the
// super, but its constructors call NCItemWnd's and its vtable inherits NCItemWnd's bodies. In
// the inventory grid (Kind 0) an item is offered by carrying it onto the offer grid or by a
// double click (how many first, for a stack); the other two grids only show their items. An
// entry's count there is the amount offered (NItemEntry +0xC48); the tool tips are the
// inventory's (NItemToolTip.h).
//
// Desktop (NCConsole) +0xE0 is this window.

#include "nwindow/NCItemWnd.h"

class NCButton;

// NWM_COMMAND ids (names ours)
enum
{
    NID_TRADE_FRAME = 0xF3,
    NID_TRADE_OK = 0xF4,
    NID_TRADE_CANCEL = 0xF5,
    NID_TRADE_ADD = 0xF6,            // the down arrow
};

// NCTradeItemWnd::Kind (names ours)
enum
{
    TRADEGRID_Inventory = 0,
    TRADEGRID_MyOffer = 1,
    TRADEGRID_PartnerOffer = 2,
};

class NCTradeItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCTradeItemWnd, NCItemWnd, 0, 0x101503c8)

    INT Kind;                        // +0x118 TRADEGRID_* (SetKind; the constructors leave it)

    NCTradeItemWnd();                                                // 0x100ba410
    NCTradeItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100ba4b0
    // ~NCTradeItemWnd                                               // 0x100ba460

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10057b10 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b9330
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100b94c0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100b9570
    virtual void Clear();                                            // 65 0x100ba1d0
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x100b79e0
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x100b97e0 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x100b9ad0

    // names ours
    void SetKind(INT InKind);                                        // 0x10057ba0 (one body with the
                                                                     //   shop windows' grids')
    void SetToolTip(NItemEntry* Item);                               // 0x100b72f0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100ba230 0 when full
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x100b99d0 0: not here
    UBOOL RemoveItem(INT ObjectID);                                  // 0x10093f40 (shared)
};
NW_ASSERT_OFFSET(NCTradeItemWnd, Kind, 0x118);
NW_ASSERT_SIZE(NCTradeItemWnd, 0x11C);

class NCTradeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTradeWnd, NCWnd, 0, 0x101508f0)

    NCTradeItemWnd* InventoryItems;  // +0x94 6 x 4 icons, 80 at most
    NCTradeItemWnd* MyItems;         // +0x98 6 icons
    NCTradeItemWnd* PartnerItems;    // +0x9C 6 icons
    UTexture* BackTex;               // +0xA0 "L2UI.storeWnd.TradeBack"
    NCButton* AddButton;             // +0xA4
    NCButton* OKButton;              // +0xA8
    NCButton* CancelButton;          // +0xAC
    TCHAR MyName[20];                // +0xB0 over the player's offer
    TCHAR PartnerName[20];           // +0xD8 over the partner's (Reset clears it)

    NCTradeWnd();                                                    // 0x100b9b90
    // ~NCTradeWnd                                                   // 0x100b9c00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100b9170
    virtual INT OnCreate();                                          // 35 0x100ba4e0
    virtual INT OnDestroy();                                         // 36 0x100b9d50
    virtual void ShowWindow();                                       // 42 0x100b9260
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b9c50
    // names ours
    virtual INT OnOK();                                              // 62 0x100b92f0
    virtual INT OnCancel();                                          // 63 0x100b9310
    virtual INT OnAdd();                                             // 64 0x100b9e70 the down arrow

    // names ours (the console's trade handlers call these)
    void Reset();                                                    // 0x100b9220
    void SetMyName(const TCHAR* Name);                               // 0x100b9290
    void SetPartnerName(const TCHAR* Name);                          // 0x100b92c0
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x100ba370
    UBOOL AddMyItem(NItemInfo* Info);                                // 0x100ba380
    UBOOL AddPartnerItem(NItemInfo* Info);                           // 0x100ba390
    UBOOL UpdateInventoryItem(INT Op, NItemInfo* Info);              // 0x100ba3a0 1 add, 3 change,
                                                                     //   2 remove
    void ReloadTexts();                                              // 0x100ba030 item names and
                                                                     //   descriptions, tool tips
};
NW_ASSERT_OFFSET(NCTradeWnd, BackTex, 0xA0);
NW_ASSERT_OFFSET(NCTradeWnd, MyName, 0xB0);
NW_ASSERT_OFFSET(NCTradeWnd, PartnerName, 0xD8);
NW_ASSERT_SIZE(NCTradeWnd, 0x100);

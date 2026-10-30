#pragma once
// NCPrivateShopWnd - the private store (sell) window (UClass "CPrivateShopWnd", 0xD4), in two
// modes: setting one's own store up (Mode 1: the inventory on top, the items on sale with
// their prices below; Start, Message, Abort) or buying at someone's (Mode 0: the store's items
// on top, the ones chosen below; OK). The arrows and drags move items between the two grids
// (how many first, for a stack; the price first, going on sale), and the window adds up the
// price of the list that is sent. The console fills it (SetMode, the items, SetAdena); the
// desktop (NCConsole) keeps it at +0x130.
//
// NCPrivateShopItemWnd - its two grids (UClass "CPrivateShopItemWnd", 0x128): Side 0 is the
// store's list, Side 1 the player's side. An entry's count is the amount there (NItemEntry::
// TradeCount), its price NItemEntry::UnknownC20 (Unknown1CA8: the item's reference price, a
// price 5 times away from it asks first). The questions go through the desktop's dialog box,
// which answers with the NWM_STORE_* messages.

#include "nwindow/NCItemWnd.h"

class NCButton;

// what the dialog box sends the store windows (NCDialogBox::OnOK / OnCancel), names ours
enum
{
    NWM_STORE_PRICE = (INT)0xF0000006,         // dialog 0x21: the price typed
    NWM_STORE_AMOUNT = (INT)0xF0000007,        // dialog 0x22: the amount typed
    NWM_STORE_PRICE_OK = (INT)0xF0000008,      // dialog 0x24: the unusual price confirmed
    NWM_STORE_PRICE_CANCEL = (INT)0xF0000009,  // dialog 0x24 cancelled
    NWM_STORE_BUY_OK = (INT)0xF000000A,        // dialog 0x25: buying at unusual prices confirmed
};

class NCPrivateShopItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPrivateShopItemWnd, NCItemWnd, 0, 0x10148318)

    NCPrivateShopItemWnd* Partner;   // +0x118 the other grid (SetPartner), or the one an item
                                     //   was carried onto
    INT Mode;                        // +0x11C the store window's (SetMode)
    INT Side;                        // +0x120 0: the store's list, 1: the player's side (SetSide)
    INT Unknown124;                  // +0x124 cleared, never read here

    NCPrivateShopItemWnd();                                          // 0x1009eeb0
    NCPrivateShopItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1009ef50
    // ~NCPrivateShopItemWnd                                         // 0x1009ef00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1009ad90
    virtual INT OnDestroy();                                         // 36 0x100a39e0 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1009ef90
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x1009bf30
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1009f290
    virtual INT OnPriceCancel(WPARAM wParam, LPARAM lParam) { return 1; } // 69 0x100098f0 (folded)
    virtual INT OnPrice(WPARAM wParam, LPARAM lParam);               // 70 0x1009f600
    virtual INT OnAmount(WPARAM wParam, LPARAM lParam);              // 71 0x1009f7e0
    virtual INT OnPriceOK(WPARAM wParam, LPARAM lParam);             // 72 0x1009f4f0

    // names ours
    void SetPartner(NCPrivateShopItemWnd* InPartner);                // 0x10057ba0 (hooked as
                                                                     //   NCTradeItemWnd::SetKind)
    void SetMode(INT InMode);                                        // 0x100b3880 (shared with the
                                                                     //   NPC shops' grids)
    void SetSide(INT InSide);                                        // 0x100b3890 (shared)
    void SetToolTip(NItemEntry* Item);                               // 0x1009ae80
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1009e560 0 when full
    UBOOL AddEntry(NItemEntry* Src, INT Count);                      // 0x1009e660 a copy, 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x1009e730 to Partner
};
NW_ASSERT_OFFSET(NCPrivateShopItemWnd, Partner, 0x118);
NW_ASSERT_OFFSET(NCPrivateShopItemWnd, Side, 0x120);
NW_ASSERT_SIZE(NCPrivateShopItemWnd, 0x128);

// 4-byte packing: TotalPrice sits at +0xCC
#pragma pack(push, 4)
class NCPrivateShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPrivateShopWnd, NCWnd, 0, 0x10147df0)

    NCPrivateShopItemWnd* PlayerItems;   // +0x94 Side 1: the inventory (Mode 1), the items
                                         //   chosen (Mode 0)
    NCPrivateShopItemWnd* StoreItems;    // +0x98 Side 0: the items on sale
    INT Unknown9C;                       // +0x9C
    UTexture* BackTex;                   // +0xA0 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                       // +0xA4
    NCButton* UpButton;                  // +0xA8
    NCButton* DownButton;                // +0xAC
    NCButton* StartButton;               // +0xB0 "Start" (Mode 1) / "OK" (Mode 0)
    NCButton* MessageButton;             // +0xB4 Mode 1 only
    NCButton* AbortButton;               // +0xB8 Mode 1 only
    NCWnd* UnknownBC;                    // +0xBC a child the destructor would remove; not made
    INT Mode;                            // +0xC0 0: buying at someone's store, 1: one's own (SetMode)
    INT OwnerID;                         // +0xC4 (-1) object id of the store's owner (Mode 0)
    INT Adena;                           // +0xC8 the player's (SetAdena)
    SQWORD TotalPrice;                   // +0xCC the list's (UpdateTotal)

    NCPrivateShopWnd();                                              // 0x1009d9d0
    // ~NCPrivateShopWnd                                             // 0x1009da70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1009d7e0
    virtual INT OnCreate();                                          // 35 0x1009f840
    virtual INT OnDestroy();                                         // 36 0x10098e10 (shared)
    virtual void ShowWindow();                                       // 42 0x100982c0 (shared)
    virtual void HideWindow();                                       // 43 0x1009d960 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1009dac0
    // names ours
    virtual INT OnStart();                                           // 62 0x1009e020
    virtual INT OnUp();                                              // 63 0x1009eab0
    virtual INT OnDown();                                            // 64 0x1009ed00
    virtual INT OnClose();                                           // 65 0x1009d970
    virtual INT OnMessage();                                         // 66 0x1009e380
    virtual INT OnAbort();                                           // 67 0x1009d9a0
    virtual INT OnBuyConfirmed(WPARAM wParam, LPARAM lParam);        // 68 0x1009e3f0

    // names ours
    void SetAdena(INT InAdena);                                      // 0x1009d910
    void SetOwnerID(INT InOwnerID);                                  // 0x100a5d20 (NCShopWnd::
                                                                     //   SetAdena's body)
    void Reset();                                                    // 0x1009d920 (also the buy
                                                                     //   window's)
    void UpdateLayout();                                             // 0x1009df20
    void SetMode(INT InMode);                                        // 0x1009ea30
    void AddStoreItem(NItemInfo* Info);                              // 0x1009ea70
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x1009eaa0
    void UpdateTotal();                                              // 0x10099360 (shared)
};
#pragma pack(pop)
NW_ASSERT_OFFSET(NCPrivateShopWnd, BackTex, 0xA0);
NW_ASSERT_OFFSET(NCPrivateShopWnd, Mode, 0xC0);
NW_ASSERT_OFFSET(NCPrivateShopWnd, TotalPrice, 0xCC);
NW_ASSERT_SIZE(NCPrivateShopWnd, 0xD4);

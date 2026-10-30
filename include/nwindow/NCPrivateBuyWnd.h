#pragma once
// NCPrivateBuyWnd - the private store (buy) window (UClass "CPrivateBuyWnd", 0xD4): the sell
// window's twin (NCPrivateShopWnd.h, the same fields at the same places; several bodies are
// shared) for a store that buys. Setting one's own up (Mode 1): the inventory on top as a
// catalog, the wish list below with the prices offered and the amounts wanted; at someone's
// (Mode 0): the items the store wants on top, the ones sold to it below. The desktop keeps it
// at +0x134.
//
// NCPrivateBuyItemWnd - its two grids (UClass "CPrivateBuyItemWnd", 0x12C), Side and Mode as in
// NCPrivateShopItemWnd; NItemEntry::UnknownC34 is the amount the store still wants. At
// someone's store the inventory's items the player lacks (count 0) are covered.

#include "nwindow/NCItemWnd.h"

class NCButton;

// what the dialog box sends the buy store windows (NCDialogBox::OnOK), names ours
enum
{
    NWM_BUYSTORE_PRICE = (INT)0xF000000B,      // dialog 0x26: the price typed
    NWM_BUYSTORE_AMOUNT = (INT)0xF000000C,     // dialog 0x27: the amount typed
    NWM_BUYSTORE_PRICE_OK = (INT)0xF000000D,   // dialog 0x29: the unusual price confirmed
    NWM_BUYSTORE_PRICE_CANCEL = (INT)0xF000000E, // never sent
    NWM_BUYSTORE_SELL_OK = (INT)0xF000000F,    // dialog 0x2A: selling at unusual prices confirmed
};

class NCPrivateBuyItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPrivateBuyItemWnd, NCItemWnd, 0, 0x101478c8)

    UTexture* CoverTex;              // +0x118 "NWindow.Chat", then "NWindow.ChatBack" (OnPaint)
                                     //   over the items the player lacks
    NCPrivateBuyItemWnd* Partner;    // +0x11C the other grid (SetPartner), or the one an item
                                     //   was carried onto
    INT Mode;                        // +0x120 the store window's (SetMode)
    INT Side;                        // +0x124 0: the store's list, 1: the player's side (SetSide)
    INT Unknown128;                  // +0x128 cleared, never read here

    NCPrivateBuyItemWnd();                                           // 0x1009a090
    NCPrivateBuyItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1009a180
    // ~NCPrivateBuyItemWnd                                          // 0x1009a130

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100955a0
    virtual INT OnDestroy();                                         // 36 0x100a39e0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100983a0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1009a200
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x10096790
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1009a500
    virtual INT OnPriceCancel(WPARAM wParam, LPARAM lParam) { return 1; } // 69 0x100098f0 (folded)
    virtual INT OnPrice(WPARAM wParam, LPARAM lParam);               // 70 0x10098730
    virtual INT OnAmount(WPARAM wParam, LPARAM lParam);              // 71 0x1009a770
    virtual INT OnPriceOK(WPARAM wParam, LPARAM lParam);             // 72 0x10098660

    // names ours
    void SetPartner(NCPrivateBuyItemWnd* InPartner);                 // 0x100b3880 (shared with the
                                                                     //   NPC shops' grids)
    void SetMode(INT InMode);                                        // 0x100b3890 (shared)
    void SetSide(INT InSide);                                        // 0x10095690
    void SetToolTip(NItemEntry* Item);                               // 0x100956a0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10099680 0 when full
    UBOOL AddEntry(NItemEntry* Src, INT Count);                      // 0x10099780 a copy, 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x10099850 to Partner
};
NW_ASSERT_OFFSET(NCPrivateBuyItemWnd, Partner, 0x11C);
NW_ASSERT_OFFSET(NCPrivateBuyItemWnd, Side, 0x124);
NW_ASSERT_SIZE(NCPrivateBuyItemWnd, 0x12C);

// 4-byte packing: TotalPrice sits at +0xCC
#pragma pack(push, 4)
class NCPrivateBuyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPrivateBuyWnd, NCWnd, 0, 0x101473a8)

    NCPrivateBuyItemWnd* PlayerItems;    // +0x94 Side 1: the inventory (Mode 1), the items sold
                                         //   (Mode 0)
    NCPrivateBuyItemWnd* StoreItems;     // +0x98 Side 0: the items wanted
    INT Unknown9C;                       // +0x9C
    UTexture* BackTex;                   // +0xA0 "L2UI.StoreWnd.StoreBack"
    INT UnknownA4;                       // +0xA4
    NCButton* UpButton;                  // +0xA8
    NCButton* DownButton;                // +0xAC
    NCButton* StartButton;               // +0xB0 "Start" (Mode 1) / "OK" (Mode 0)
    NCButton* MessageButton;             // +0xB4 Mode 1 only
    NCButton* AbortButton;               // +0xB8 Mode 1 only
    NCWnd* UnknownBC;                    // +0xBC a child the destructor would remove; not made
    INT Mode;                            // +0xC0 0: selling to someone's store, 1: one's own
    INT OwnerID;                         // +0xC4 (-1) object id of the store's owner (Mode 0)
    INT Adena;                           // +0xC8 the player's (SetAdena)
    SQWORD TotalPrice;                   // +0xCC the list's (UpdateTotal)

    NCPrivateBuyWnd();                                               // 0x100988c0
    // ~NCPrivateBuyWnd                                              // 0x10098960

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10098190
    virtual INT OnCreate();                                          // 35 0x1009a7f0
    virtual INT OnDestroy();                                         // 36 0x10098e10
    virtual void ShowWindow();                                       // 42 0x100982c0
    virtual void HideWindow();                                       // 43 0x1009d960
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100989b0
    // names ours
    virtual INT OnStart();                                           // 62 0x10098f90
    virtual INT OnUp();                                              // 63 0x10099c70
    virtual INT OnDown();                                            // 64 0x10099ed0
    virtual INT OnClose();                                           // 65 0x10098340
    virtual INT OnMessage();                                         // 66 0x10099450
    virtual INT OnAbort();                                           // 67 0x10098370
    virtual INT OnSellConfirmed(WPARAM wParam, LPARAM lParam);       // 68 0x100994c0

    // names ours
    void SetAdena(INT InAdena);                                      // 0x1009d910 (hooked as the
                                                                     //   sell window's)
    void SetOwnerID(INT InOwnerID);                                  // 0x100a5d20 (NCShopWnd::
                                                                     //   SetAdena's body)
    void Reset();                                                    // 0x1009d920 (the sell window's)
    void UpdateLayout();                                             // 0x1009df20 (the sell window's)
    void SetMode(INT InMode);                                        // 0x10099bf0
    void AddStoreItem(NItemInfo* Info);                              // 0x10099c30
    UBOOL AddPlayerItem(NItemInfo* Info);                            // 0x10099c60
    void UpdateTotal();                                              // 0x10099360
};
#pragma pack(pop)
NW_ASSERT_OFFSET(NCPrivateBuyWnd, BackTex, 0xA0);
NW_ASSERT_OFFSET(NCPrivateBuyWnd, Mode, 0xC0);
NW_ASSERT_OFFSET(NCPrivateBuyWnd, TotalPrice, 0xCC);
NW_ASSERT_SIZE(NCPrivateBuyWnd, 0xD4);

#pragma once
// NCVIPShopWnd - the VIP shop (UClass "CVIPShopWnd", 0xC4; desktop +0xD4, title "Store"): two
// pages. The list page (NCVIPShopItemListWnd) shows the products; a click on one opens the item
// page: the item's information (NCVIPShopItemInfoWnd) over the items it costs
// (NCVIPShopNeededItemWnd), with OK (buy) and Cancel (back to the list). The console fills it
// through its virtuals 158..161: Open (a list id, everything emptied), AddRecord for each
// product, AddNeededItem for each item a product costs, then the window shown on the list
// (ShowList). OK sends the list id and the product id (console virtual 162, the network
// handler's slot 186). Hiding the window empties it.
//
// The three pages are NCWnd / NCScrollWnd children laid over one another; the item page is the
// window's own background with "Item information" and "Items required" (sysstring 564, 565).

#include "nwindow/NCItemWnd.h"
#include "nwindow/NCVIPShopItemListBox.h"

class NCButton;

// NWM_COMMAND ids (names ours)
enum
{
    NID_VIPSHOP_FRAME = 0xF0E400,
    NID_VIPSHOP_LIST = 0xF0E401,
    NID_VIPSHOP_OK = 0xF0E402,
    NID_VIPSHOP_CANCEL = 0xF0E403,
};

// An item a product costs (name ours; 0x20C bytes, built on the stack by the console's
// virtual 160 and copied by NCVIPShopWnd::AddNeededItem, then by the needed items' page)
struct NVIPNeededItem
{
    INT ItemID;                // +0x000
    TCHAR Name[128];           // +0x004 the item's
    INT Count;                 // +0x104 how many
    UTexture* Icon;            // +0x108 (AddNeededItem finds it by IconName)
    TCHAR IconName[128];       // +0x10C

    NVIPNeededItem()
    {
        Name[0] = 0;
        IconName[0] = 0;
        Count = 0;
        Icon = NULL;
        ItemID = 0;
    }
};
NW_ASSERT_OFFSET(NVIPNeededItem, Count, 0x104);
NW_ASSERT_OFFSET(NVIPNeededItem, IconName, 0x10C);
NW_ASSERT_SIZE(NVIPNeededItem, 0x20C);

// A product of the shop (name ours; 0x324 bytes, NCVIPShopWnd::AddRecord): the list row and the
// items it costs (never freed: Reset only empties the window's array)
struct NVIPShopRecord : public NVIPShopEntry
{
    TArray<NVIPNeededItem*> NeededItems;   // +0x318

    NVIPShopRecord();                                                // 0x1006e790
};
NW_ASSERT_OFFSET(NVIPShopRecord, NeededItems, 0x318);
NW_ASSERT_SIZE(NVIPShopRecord, 0x324);

// the list page (UClass "CVIPShopItemListWnd", 0x9C)
class NCVIPShopItemListWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCVIPShopItemListWnd, NCWnd, 0, 0x10135d98)

    NCVIPShopItemListBox* ListBox;   // +0x94
    UTexture* BackTex;               // +0x98 "L2UI.VipWnd.VipWndBack1"

    NCVIPShopItemListWnd();                                          // 0x1006d610
    // ~NCVIPShopItemListWnd                                         // 0x1006d660

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb10
    virtual INT OnCreate();                                          // 35 0x1006e920
    virtual INT OnDestroy();                                         // 36 0x1006d6b0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006d6f0
    virtual INT OnSelect(INT Index);                                 // 62 0x1006e9f0 (name ours)

    // names ours
    void Clear();                                                    // 0x1006bb80
    void AddEntry(const NVIPShopEntry* Src);                         // 0x1006df20
};
NW_ASSERT_OFFSET(NCVIPShopItemListWnd, BackTex, 0x98);
NW_ASSERT_SIZE(NCVIPShopItemListWnd, 0x9C);

// the item page's information (UClass "CVIPShopItemInfoWnd", 0x1D7C): the product's item as an
// item window entry, completed from the item table
class NCVIPShopItemInfoWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCVIPShopItemInfoWnd, NCScrollWnd, 0, 0x10137238)

    NItemEntry Item;                 // +0xCC
    INT Unknown1D78;                 // +0x1D78

    NCVIPShopItemInfoWnd();                                          // 0x1006d980
    // ~NCVIPShopItemInfoWnd                                         // 0x1006da00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90 (shared)
    virtual INT OnCreate();                                          // 35 0x10079480 (shared)
    virtual INT OnDestroy();                                         // 36 0x10079490 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006da50

    // names ours
    void Clear();                                                    // 0x1006d2e0
    void SetItem(NItemEntry* Src);                                   // 0x1006e090
    INT DrawItemInfo(NCGDevice* Device, NItemEntry* Entry, INT X, INT Y); // 0x1006bbd0 the Y
                                                                     //   under the texts
};
NW_ASSERT_OFFSET(NCVIPShopItemInfoWnd, Item, 0xCC);
NW_ASSERT_SIZE(NCVIPShopItemInfoWnd, 0x1D7C);

// the item page's needed items (UClass "CVIPShopNeededItemWnd", 0xE4): icon, name and "X count",
// 34 pixels a row
class NCVIPShopNeededItemWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCVIPShopNeededItemWnd, NCScrollWnd, 0, 0x10136d10)

    TArray<NVIPNeededItem*> Items;   // +0xCC
    INT UnknownD8;                   // +0xD8
    INT UnknownDC;                   // +0xDC (Clear: 0)
    INT UnknownE0;                   // +0xE0 (Clear: 0)

    NCVIPShopNeededItemWnd();                                        // 0x1006ea20
    // ~NCVIPShopNeededItemWnd                                       // 0x1006ea80

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90 (shared)
    virtual INT OnCreate();                                          // 35 0x10079480 (shared)
    virtual INT OnDestroy();                                         // 36 0x1006df30
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006d810

    // names ours
    void AddItem(const NVIPNeededItem* Src);                         // 0x1006dfa0
    void Clear();                                                    // 0x1006e030
};
NW_ASSERT_OFFSET(NCVIPShopNeededItemWnd, Items, 0xCC);
NW_ASSERT_SIZE(NCVIPShopNeededItemWnd, 0xE4);

// NCVIPShopWnd::Page (names ours)
enum
{
    VIPSHOPPAGE_List = 0,
    VIPSHOPPAGE_Item = 1,
};

class NCVIPShopWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCVIPShopWnd, NCWnd, 0, 0x101367e8)

    NCVIPShopItemListWnd* ListWnd;   // +0x94
    NCVIPShopItemInfoWnd* InfoWnd;   // +0x98
    NCVIPShopNeededItemWnd* NeededWnd; // +0x9C
    NCButton* OKButton;              // +0xA0 the item page only
    NCButton* CancelButton;          // +0xA4 the item page only
    UTexture* BackTex;               // +0xA8 "L2UI.VipWnd.VipWndBack2" the item page
    INT Selected;                    // +0xAC the product shown (index in Records), -1 none
    INT ListID;                      // +0xB0 (Open) sent back with the purchase
    INT Page;                        // +0xB4 VIPSHOPPAGE_*
    TArray<NVIPShopRecord*> Records; // +0xB8

    NCVIPShopWnd();                                                  // 0x1006eae0
    // ~NCVIPShopWnd                                                 // 0x1006eb40

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006d2f0
    virtual INT OnCreate();                                          // 35 0x1006eba0
    virtual INT OnDestroy();                                         // 36 0x1006daf0
    virtual void HideWindow();                                       // 43 0x1006ef90
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006dbe0
    // names ours
    virtual INT OnOK();                                              // 62 0x1006dce0
    virtual INT OnCancel();                                          // 63 0x1006e6d0

    // names ours (the console calls the last four)
    void ShowList();                                                 // 0x1006e2d0
    void ShowItem();                                                 // 0x1006e3a0 Records(Selected)
    void Reset();                                                    // 0x1006e5a0
    void AddNeededItem(INT ProductID, const NVIPNeededItem* Src);    // 0x1006e5f0
    void Open(INT InListID);                                         // 0x1006e6e0
    void AddRecord(const NVIPShopEntry* Src);                        // 0x1006efa0
};
NW_ASSERT_OFFSET(NCVIPShopWnd, BackTex, 0xA8);
NW_ASSERT_OFFSET(NCVIPShopWnd, Records, 0xB8);
NW_ASSERT_SIZE(NCVIPShopWnd, 0xC4);

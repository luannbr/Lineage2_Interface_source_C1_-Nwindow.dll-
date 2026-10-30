#pragma once
// NCVIPShopItemListBox - the item list of the VIP shop window (UClass "CVIPShopItemListBox",
// 0x1F4). Same rows as NCTrainListBox (its code is shared where identical): the icon, the name
// and, below it, the description, plus "X <count>" for type 5 entries.

#include "nwindow/NCListBox.h"

// one item row (names ours; 0x318 bytes). The VIP shop window passes a copy of its own record
// (NVIPShopRecord, NCVIPShopWnd.h); the console builds one per product the server lists.
struct NVIPShopEntry
{
    INT ProductID;             // +0x000 what the purchase request names
    INT ItemID;                // +0x004 never copied here
    TCHAR Name[128];           // +0x008 the item's
    TCHAR Desc[128];           // +0x108
    UTexture* Icon;            // +0x208 (NCVIPShopWnd::AddRecord finds it by IconName)
    TCHAR IconName[128];       // +0x20C never copied here
    INT BodyPart;              // +0x30C never copied here
    INT Type;                  // +0x310 5: Count is shown
    INT Count;                 // +0x314

    NVIPShopEntry()
    {
        Name[0] = 0;
        Desc[0] = 0;
        IconName[0] = 0;
        Icon = NULL;
        ProductID = 0;
        ItemID = 0;
        BodyPart = 0;
        Type = 0;
        Count = 0;
    }
};
NW_ASSERT_OFFSET(NVIPShopEntry, Icon, 0x208);
NW_ASSERT_OFFSET(NVIPShopEntry, Type, 0x310);
NW_ASSERT_SIZE(NVIPShopEntry, 0x318);

class NCVIPShopItemListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCVIPShopItemListBox, NCListBox, 0, 0x101362b8)

    TArray<NVIPShopEntry*> Entries;  // +0x1E8

    NCVIPShopItemListBox();                                          // 0x1006e810
    NCVIPShopItemListBox(INT InMaxLines, INT InVisibleLines, INT Unused); // 0x1006e8d0
    // ~NCVIPShopItemListBox                                         // 0x1006e870

    // stock bodies shared with NCTrainListBox (hooked there) except OnPaint
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100576d0 (NCDebugWnd's)
    virtual INT OnCreate();                                          // 35 0x100ba9a0
    virtual INT OnDestroy();                                         // 36 0x1006dda0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006d380
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x1006bad0
    virtual INT HitTest(INT X, INT Y);                               // 63 0x100ba9d0
    virtual void Clear();                                            // 65 0x100bc570

    // names ours
    void UpdateEntryScrollBar();                                     // 0x100baaa0 (shared)
    void AddEntry(const NVIPShopEntry* Src);                         // 0x1006de10
};
NW_ASSERT_OFFSET(NCVIPShopItemListBox, Entries, 0x1E8);
NW_ASSERT_SIZE(NCVIPShopItemListBox, 0x1F4);

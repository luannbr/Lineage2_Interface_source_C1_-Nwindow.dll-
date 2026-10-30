#pragma once
// NCPetInventoryWnd - the Items page of the pet window (UClass "CPetInventoryWnd", 0x9C): the
// pet's inventory in an NCPetInvenItemWnd grid, 6 x 2 icons of 32 pixels (160 at most). The
// console fills it through NCPetWnd (AddInventoryItem / UpdateInventoryItem / ClearInventory).
//
// NCPetInvenItemWnd - that grid (UClass "CPetInvenItemWnd", 0x134): equipped items get a frame
// behind their icon; a double click or the right button uses an item (the pet equips / eats
// it), dragging it inside the grid changes its place, onto the player's inventory or paperdoll
// gives it back (how many, for a stack), onto the shortcut bar registers it. The tool tips are
// the inventory's (NItemToolTip.h).

#include "nwindow/NCItemWnd.h"

class NCPetInvenItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCPetInvenItemWnd, NCItemWnd, 0, 0x10144a78)

    UTexture* EquippedTex;           // +0x118 "l2ui.PetWnd.petitem_click" behind equipped items
    // the question asked last, which OnDialogResult answers (names ours)
    UBOOL bDropPending;              // +0x11C on the ground (nothing here sets it: the
                                     //   inventory's code, kept)
    UBOOL bToOwnerPending;           // +0x120 back to the player
    UBOOL bDblClicked;               // +0x124 the release after a double click moves nothing
    FVector DropLocation;            // +0x128 (never written here)

    NCPetInvenItemWnd();                                             // 0x100941c0
    NCPetInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10094270
    // ~NCPetInvenItemWnd                                            // 0x10094220

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1008eea0
    virtual INT OnCreate();                                          // 35 0x1008ef30
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10091830
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100919e0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10093a10
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x10091b10
    virtual void Clear();                                            // 65 0x10093fb0
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x1008f650
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x10091a80 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x10091d10

    // names ours
    void SetToolTip(NItemEntry* Item);                               // 0x1008ef60
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10093e50 0 when full
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x10091b70 0: not here
    UBOOL RemoveItem(INT ObjectID);                                  // 0x10093f40 (NCTradeItemWnd's too)
    UBOOL HasItem(INT ObjectID);                                     // 0x10091c70
    UBOOL FindItem(INT ObjectID, INT* Index);                        // 0x10091cc0
};
NW_ASSERT_OFFSET(NCPetInvenItemWnd, EquippedTex, 0x118);
NW_ASSERT_OFFSET(NCPetInvenItemWnd, DropLocation, 0x128);
NW_ASSERT_SIZE(NCPetInvenItemWnd, 0x134);

class NCPetInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetInventoryWnd, NCWnd, 0, 0x101454c0)

    UTexture* BackTex;               // +0x94 "L2UI.PetWnd.pet03" (the Action page's)
    NCPetInvenItemWnd* Items;        // +0x98

    NCPetInventoryWnd();                                             // 0x10091ed0
    // ~NCPetInventoryWnd                                            // 0x10091f20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100942b0
    virtual INT OnDestroy();                                         // 36 0x100bb540 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10091680 (folded)

    // names ours (NCPetWnd passes the console's calls on)
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10094010
    UBOOL UpdateItem(INT Op, NItemInfo* Info);                       // 0x10094030 1 add, 2 change,
                                                                     //   3 remove
    NItemEntry* GetItem(INT ObjectID);                               // 0x10091f70 NULL: none
    void ClearItems();                                               // 0x100bab50 (shared with
                                                                     //   NCTrainListWnd's Clear)
};
NW_ASSERT_OFFSET(NCPetInventoryWnd, Items, 0x98);
NW_ASSERT_SIZE(NCPetInventoryWnd, 0x9C);

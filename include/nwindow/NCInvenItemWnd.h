#pragma once
// NCInvenItemWnd - a page of the inventory's items (UClass "CInvenItemWnd", 0x13C; the
// inventory has two: "Item" and "Quest", 6 x 4 icons of 32 pixels, 160 at most). An item is
// used with a double click or the right button; dragged inside the page it changes place,
// onto the paperdoll it is equipped, onto the shortcut bar it gets a shortcut, onto the pet
// window it is given to the pet, onto the inventory's trash / crystallize icons or the ground
// it asks first (the answer comes back as NWM_DIALOG_RESULT). With an enchant scroll in use
// (SetEnchantMode) the next item released is the one to enchant.

#include "nwindow/NCItemWnd.h"

class NCInvenItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCInvenItemWnd, NCItemWnd, 0, 0x10133990)

    // the question asked last (OnLButtonUp), which OnDialogResult answers (names ours)
    UBOOL bDropPending;              // +0x118 on the ground
    UBOOL bDestroyPending;           // +0x11C on the trash icon
    UBOOL bCrystallizePending;       // +0x120 on the crystallize icon
    UBOOL bPetPending;               // +0x124 on the pet window
    UBOOL bEnchantMode;              // +0x128 (SetEnchantMode) the cursor stays the scroll's
    UBOOL bDblClicked;               // +0x12C the release after a double click moves nothing
    FVector DropLocation;            // +0x130 the ground point under the mouse (engine +0x1EC)

    NCInvenItemWnd();                                                // 0x10068440
    NCInvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10068480
    // ~NCInvenItemWnd                                               // 0x1005fe00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10057b10
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100655f0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100657b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10065120
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10067690
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x10065060
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x10062a60
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x10065240 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x100652d0

    // names ours
    void SetEnchantMode(UBOOL bOn);                                  // 0x10062360
    void SetToolTip(NItemEntry* Item);                               // 0x10062370
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x100654c0 0: not here
    INT GetAdena();                                                  // 0x100655c0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10068030 0 when full
    UBOOL RemoveItem(INT ObjectID);                                  // 0x10068120
    // NCPetInvenItemWnd's bodies (0x10091c70, 0x10091cc0): unhooked copies
    UBOOL HasItem(INT ObjectID);
    UBOOL FindItem(INT ObjectID, INT* Index);
};
NW_ASSERT_OFFSET(NCInvenItemWnd, bDropPending, 0x118);
NW_ASSERT_OFFSET(NCInvenItemWnd, bEnchantMode, 0x128);
NW_ASSERT_OFFSET(NCInvenItemWnd, DropLocation, 0x130);
NW_ASSERT_SIZE(NCInvenItemWnd, 0x13C);

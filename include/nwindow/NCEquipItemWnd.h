#pragma once
// NCEquipItemWnd - the inventory's paperdoll (UClass "CEquipItemWnd", 0x254; built like the
// item windows but on NCWnd): the equipped items in 14 slots of a 5 x 3 grid, 35 pixels apart
// (the console tells the slot of each item: NConsoleWnd::GetEquipSlot; 14 and 15 are the
// two-handed weapon and the full armor, drawn over two slots). A double click or the right
// button takes an item off; dragged onto the inventory's page it is taken off too, onto the
// shortcut bar it gets a shortcut, onto the pet inventory it is given to the pet, onto the
// trash / crystallize icons or the ground it asks first (NWM_DIALOG_RESULT answers).

#include "nwindow/NCItemWnd.h"

class NCEquipItemWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCEquipItemWnd, NCWnd, 0, 0x10133470)

    INT SelectedIndex;               // +0x94 (-1) index in Items of the item picked up
    UBOOL bNoDrag;                   // +0x98 the next click selects without picking up
    TArray<NItemEntry*> Items;       // +0x9C
    BYTE _UnknownA8[0xE4 - 0xA8];
    UTexture* SelectTex;             // +0xE4 "l2ui.nwindow.item_click"
    UTexture* ToolTipTex;            // +0xE8 "L2UI.tooltip.mini-tool01"
    UTexture* TipTopTex;             // +0xEC "L2UI.Tooltip.Tool02"  box of the selected item
    UTexture* TipMidTex;             // +0xF0 "L2UI.Tooltip.Tool01"
    UTexture* TipBottomTex;          // +0xF4 "L2UI.Tooltip.Tool03"
    UTexture* DualCapTex;            // +0xF8 "L2UI.InventoryWnd.Icon_dualcap" over the second
                                     //   slot of a two-slot item
    UBOOL bDragging;                 // +0xFC the selected item is being carried
    UBOOL bHasUnknown1C98;           // +0x100 an item with NItemEntry::Unknown1C98 is equipped
                                     //   (AddItem): a bow is then drawn in slot 5
    INT Unknown104;                  // +0x104 (5)
    INT Unknown108;                  // +0x108 (3)
    INT Unknown10C;                  // +0x10C
    INT Unknown110;                  // +0x110
    INT NumSlots;                    // +0x114 (14) also the most items it keeps
    INT CellSize;                    // +0x118 (constructor's argument, 32)
    INT Selected;                    // +0x11C slot, -1 = none
    INT HoverSlot;                   // +0x120 slot under the mouse, -1 = none
    // the question asked last (OnLButtonUp), which OnDialogResult answers (names ours)
    UBOOL bDropPending;              // +0x124 on the ground
    UBOOL bDestroyPending;           // +0x128 on the trash icon
    UBOOL bCrystallizePending;       // +0x12C on the crystallize icon
    UBOOL bPetPending;               // +0x130 on the pet inventory
    UBOOL bEnchantMode;              // +0x134 (SetEnchantMode)
    FVector DropLocation;            // +0x138 the ground point under the mouse
    BYTE _Unknown144[0x254 - 0x144];

    NCEquipItemWnd(INT InCellSize = 32);                                  // 0x10068380
    // ~NCEquipItemWnd                                               // 0x1005fd70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10060730
    virtual INT OnCreate();                                          // 35 0x100607c0
    virtual INT OnDestroy();                                         // 36 0x100673a0
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100642d0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x100646e0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100663b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100661a0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10066530
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x10066e80
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 62 0x10067140
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 63 0x10064c70
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 64 0x10060e60

    // names ours
    void SetEnchantMode(UBOOL bOn);                                  // 0x10060840
    void SetToolTip(NItemEntry* Item);                               // 0x10060850
    void UnselectAll();                                              // 0x10064b40
    INT HitTest(INT X, INT Y);                                       // 0x10064b70 slot; 0 outside
    UBOOL UpdateItem(NItemInfo* Info);                               // 0x10064ec0 0: not here
    UBOOL HasItem(INT ObjectID);                                     // 0x10064fc0
    UBOOL FindItem(INT ObjectID, INT* Index);                        // 0x10065010
    UBOOL AddItem(NItemInfo* Info);                                  // 0x10067410 0 when full
    UBOOL RemoveItem(INT ObjectID);                                  // 0x100675c0
    void Clear();                                                    // 0x10067630
};
NW_ASSERT_OFFSET(NCEquipItemWnd, Items, 0x9C);
NW_ASSERT_OFFSET(NCEquipItemWnd, SelectTex, 0xE4);
NW_ASSERT_OFFSET(NCEquipItemWnd, NumSlots, 0x114);
NW_ASSERT_OFFSET(NCEquipItemWnd, bDropPending, 0x124);
NW_ASSERT_OFFSET(NCEquipItemWnd, DropLocation, 0x138);
NW_ASSERT_SIZE(NCEquipItemWnd, 0x254);

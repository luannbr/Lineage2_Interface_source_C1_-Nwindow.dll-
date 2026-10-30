#pragma once
// NCItemWnd - grid of item icons (UClass "CItemWnd", 0x118), base of the item windows:
// inventory, pet inventory, shops, warehouse, private stores, skills, quest items, trade
// (NCTradeItemWnd's UClass names NCWnd as its super, but its constructor calls NCItemWnd's
// and its vtable inherits these bodies). The items are NItemEntry records the window owns
// (Clear frees them), shown as Columns x VisibleRows icons of CellSize pixels, 3 apart,
// scrolled by whole rows. A click selects an item and picks it up (bDragging: the cursor
// becomes its icon); the tool tip over an item is its name, or for the selected item a box
// whose text the subclass draws (DrawItemToolTip).

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NItemInfo.h"

// An item of an item window (name ours; 0x1CAC bytes, created with new from an NItemInfo by
// the subclasses' AddItem and deleted by NCItemWnd::Clear): the NItemInfo fields in another
// order, the tool tip texts (written by the subclass) and the selection.
struct NItemEntry
{
    UTexture* Icon;            // +0x000
    INT Unknown004;            // +0x004 second icon (GetIcon2: the paperdoll's two-slot items)
    INT Unknown008;            // +0x008 third icon (GetIcon3)
    TCHAR Name[128];           // +0x00C
    TCHAR IconName[128];       // +0x10C
    TCHAR Unknown20C[128];     // +0x20C texture of the second icon
    TCHAR Unknown30C[128];     // +0x30C texture of the third icon
    TCHAR Description[1024];   // +0x40C
    INT Type;                  // +0xC0C (-1) NItemInfo::Param; 0..3: the enchant shows
    INT ObjectID;              // +0xC10 the server's id of the item
    INT ID;                    // +0xC14
    INT UnknownC18;            // +0xC18 (-1) NItemInfo +0xC20
    INT Count;                 // +0xC1C
    INT UnknownC20;            // +0xC20 NItemInfo +0xC1C
    INT BodyPart;              // +0xC24 (NConsoleWnd::GetItemSlotText)
    INT Weight;                // +0xC28
    INT UnknownC2C;            // +0xC2C (-1)
    INT UnknownC30;            // +0xC30 (-1) weapon type (NConsoleWnd::GetWeaponTypeName; 6: bow)
    INT UnknownC34;            // +0xC34 (-1) NItemInfo +0xC90
    INT Enchant;               // +0xC38
    INT UnknownC3C;            // +0xC3C NItemInfo +0xC84
    UBOOL bStackable;          // +0xC40 the count shows: "Name(Count)"
    UBOOL bEquipped;           // +0xC44 (NItemInfo +0xC8C)
    INT TradeCount;            // +0xC48 only the constructors clear it; the trade grid's amount
                               //   offered
    TCHAR ToolTipName[1024];   // +0xC4C tool tip of the item, not selected
    TCHAR ToolTipText[1024];   // +0x144C tool tip of the selected item
    INT Unknown1C4C;           // +0x1C4C
    INT Unknown1C50;           // +0x1C50
    UBOOL bSelected;           // +0x1C54
    INT Unknown1C58;           // +0x1C58 NItemInfo +0xC4C
    INT CrystalGrade;          // +0x1C5C (NItemInfo +0xC50)
    INT Unknown1C60;           // +0x1C60
    INT Unknown1C64;           // +0x1C64
    INT Unknown1C68;           // +0x1C68 NItemInfo +0xC5C
    INT Unknown1C6C;           // +0x1C6C attack speed (NConsoleWnd::GetAttackSpeedText)
    INT Unknown1C70;           // +0x1C70
    INT SubType;               // +0x1C74 (-1) (NConsoleWnd::GetItemSlotText)
    INT Unknown1C78;           // +0x1C78 a shield's evasion
    INT Unknown1C7C;           // +0x1C7C Copy leaves it
    INT Unknown1C80;           // +0x1C80 NItemInfo +0xC34 P.Atk
    INT Unknown1C84;           // +0x1C84 M.Atk
    INT Unknown1C88;           // +0x1C88 P.Def
    INT Unknown1C8C;           // +0x1C8C M.Def
    INT Unknown1C90;           // +0x1C90 a shield's P.Def
    INT Unknown1C94;           // +0x1C94 a shield's rate
    INT Unknown1C98;           // +0x1C98 NItemInfo +0xC94 (the paperdoll: the item is not taken
                               //   off by a click or a drag; a bow is drawn in one slot)
    INT Unknown1C9C;           // +0x1C9C NItemInfo +0xC78 MP bonus (robes)
    INT Unknown1CA0;           // +0x1CA0 soulshots used
    INT Unknown1CA4;           // +0x1CA4 spiritshots used
    INT Unknown1CA8;           // +0x1CA8 NItemInfo +0xC98; Reset leaves it

    NItemEntry();                                                    // 0x10015b80
    NItemEntry(NItemInfo* Src);                                      // 0x10015cb0

    // The stock code calls these through getters folded with other classes' (0x10033ac0,
    // 0x10033ad0, 0x10033a40, 0x10033a50).
    UTexture* GetIcon() { return Icon; }
    INT GetUnknown004() { return Unknown004; }
    INT GetUnknown008() { return Unknown008; }
    // Unknown004 / Unknown008 are the second and third icons of an item drawn over two slots
    // (from Unknown20C / Unknown30C: NCEquipItemWnd)
    UTexture* GetIcon2() { return (UTexture*)(INT_PTR)Unknown004; }
    UTexture* GetIcon3() { return (UTexture*)(INT_PTR)Unknown008; }
    const TCHAR* GetName()
    {
        const TCHAR* S = Name;
        return S && *S ? S : NULL;
    }

    // names ours
    const TCHAR* GetToolTipName();                                   // 0x10015f30 NULL when empty
    const TCHAR* GetToolTipText();                                   // 0x10015f50 NULL when empty
    const TCHAR* GetDescription();                                   // 0x10015f70 NULL when empty
    INT GetUnknownC20();                                             // 0x10015f90
    INT GetUnknownC34();                                             // 0x10015fa0
    void SetUnknownC34(INT Value);                                   // 0x10015fb0
    void Copy(NItemEntry* Src);                                      // 0x10015fc0
    void Select();                                                   // 0x10016230
    UBOOL IsSelected();                                              // 0x10016240
    void Unselect();                                                 // 0x10016250
    UBOOL IsStackable();                                             // 0x10016260
    void Reset();                                                    // 0x10016270
};
NW_ASSERT_OFFSET(NItemEntry, Name, 0xC);
NW_ASSERT_OFFSET(NItemEntry, Description, 0x40C);
NW_ASSERT_OFFSET(NItemEntry, Type, 0xC0C);
NW_ASSERT_OFFSET(NItemEntry, Count, 0xC1C);
NW_ASSERT_OFFSET(NItemEntry, Enchant, 0xC38);
NW_ASSERT_OFFSET(NItemEntry, ToolTipName, 0xC4C);
NW_ASSERT_OFFSET(NItemEntry, ToolTipText, 0x144C);
NW_ASSERT_OFFSET(NItemEntry, bSelected, 0x1C54);
NW_ASSERT_OFFSET(NItemEntry, SubType, 0x1C74);
NW_ASSERT_OFFSET(NItemEntry, Unknown1C98, 0x1C98);
NW_ASSERT_SIZE(NItemEntry, 0x1CAC);

// An item as the item requests to the server take it, by value (name ours): the engine's
// ItemInfo (UE2Engine.h) with NWindow's constructor (Type and Unknown2C -1, Unknown02 not
// set, the rest 0); the destroy and drop requests fill ObjectID and Count.
struct NItemRequest : public ItemInfo
{
    NItemRequest();                                                  // 0x10064290
};
NW_ASSERT_OFFSET(NItemRequest, ObjectID, 0x4);
NW_ASSERT_OFFSET(NItemRequest, Count, 0xC);
NW_ASSERT_SIZE(NItemRequest, 0x30);

// The console's requests that take an NItemRequest by value
inline void NRequestDestroyItem(NItemRequest Request)
{
    GConsole->RequestDestroyItem(Request);
}
inline void NRequestDropItem(NItemRequest Request, FVector Location)
{
    GConsole->RequestDropItem(Request, Location);
}

class NCItemWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCItemWnd, NCScrollWnd, 0, 0x10120eb0)

    UTexture* BackTex;               // +0xCC "NWindow.BlackTexture" (the subclasses draw it)
    UTexture* SelectTex;             // +0xD0 "l2ui.nwindow.item_click" around the selected item
    UTexture* ToolTipTex;            // +0xD4 "L2UI.tooltip.mini-tool01" under the name tip
    UTexture* TipTopTex;             // +0xD8 "L2UI.Tooltip.Tool02"  box of the selected item:
    UTexture* TipMidTex;             // +0xDC "L2UI.Tooltip.Tool01"  top, a strip per line of
    UTexture* TipBottomTex;          // +0xE0 "L2UI.Tooltip.Tool03"  text, bottom
    TArray<NItemEntry*> Items;       // +0xE4
    UBOOL bDragging;                 // +0xF0 the selected item is being carried
    UBOOL bNoDrag;                   // +0xF4 the next click selects without picking up
    INT Columns;                     // +0xF8 (5)
    INT VisibleRows;                 // +0xFC (5)
    INT TopRow;                      // +0x100 first row shown
    INT NumRows;                     // +0x104 rows of items (UpdateScrollRange)
    INT MaxItems;                    // +0x108 (25) AddItem refuses more
    INT CellSize;                    // +0x10C (32) icon size
    INT Selected;                    // +0x110 -1 = none
    INT HoverIndex;                  // +0x114 item under the mouse, -1 = none

    NCItemWnd();                                                     // 0x100176b0
    NCItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100177e0
    // ~NCItemWnd                                                    // 0x10017780

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006bb90
    virtual INT OnCreate();                                          // 35 0x10016960
    virtual INT OnDestroy();                                         // 36 0x10016370
    virtual void HideWindow();                                       // 43 0x100164f0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100169f0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x10016ba0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x10017010
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10016ef0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 1; } // 57 0x100098f0 (folded)
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x10016440
    // by rows instead of NCScrollWnd's pixels
    virtual INT ScrollUp();                                          // 62 0x10016690
    virtual INT ScrollDown();                                        // 63 0x100166d0
    virtual INT OnScrollBar(INT Value);                              // 64 0x10016670
    // names ours
    virtual void Clear();                                            // 65 0x10017350 deletes the items
    virtual void UnselectAll();                                      // 66 0x100170b0
    // text of the selected item's tool tip box, from (X, Y); nothing here
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x10016720 (folded)

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x10016390 item index, -1 outside
    INT GetSelected();                                               // 0x10016410 -1: none
    INT GetItemCount();                                              // 0x10016430
    void InitScrollBar();                                            // 0x10016570
    void UpdateScrollRange();                                        // 0x100165b0 after adding items
};
NW_ASSERT_OFFSET(NCItemWnd, BackTex, 0xCC);
NW_ASSERT_OFFSET(NCItemWnd, Items, 0xE4);
NW_ASSERT_OFFSET(NCItemWnd, Columns, 0xF8);
NW_ASSERT_OFFSET(NCItemWnd, MaxItems, 0x108);
NW_ASSERT_OFFSET(NCItemWnd, HoverIndex, 0x114);
NW_ASSERT_SIZE(NCItemWnd, 0x118);

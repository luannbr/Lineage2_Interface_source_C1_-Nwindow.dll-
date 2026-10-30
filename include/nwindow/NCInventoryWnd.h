#pragma once
// NCInventoryWnd - the inventory (UClass "CInventoryWnd", 0xF8; the desktop, NCConsole, creates
// it in OnCreate and keeps it at +0xBC): the paperdoll (NCEquipItemWnd) on top, two tab pages
// of items below ("Item" and "Quest", both NCInvenItemWnd: the quest items, NItemInfo::Param
// 3, go to the second), the adena and the weight bar at the bottom, between the crystallize
// icon (shown with the skill) and the trash icon; the item windows light those up while an
// item is carried over them. The console fills it through AddItem / UpdateItem / ClearItems.

#include "nwindow/NCConsole.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NGameData.h"

class NCEquipItemWnd;
class NCInvenItemWnd;
class NCTabControl;
struct NItemEntry;
struct NItemInfo;

class NCInventoryWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCInventoryWnd, NCWnd, 0, 0x10133ec0)

    UTexture* BackTex;               // +0x94 "L2UI.InventoryWnd.InventoryBack"
    UTexture* CrystallizeTex;        // +0x98 "L2UI.WindowIcon.Crystalize"
    UTexture* CrystallizeActiveTex;  // +0x9C "L2UI.WindowIcon.Crystalize_active"
    UTexture* WeightTex[4];          // +0xA0 "L2UI.InventoryWnd.InventoryWT1".."4": the weight
                                     //   bar up to 50%, 66.66%, 80%, above
    UTexture* TrashTex;              // +0xB0 "L2UI.WindowIcon.DestroyItem"
    UTexture* TrashActiveTex;        // +0xB4 "L2UI.WindowIcon.DestroyItem_active"
    UTexture* AdenaTex;              // +0xB8 "L2UI.WindowIcon.WindowIconAdena"
    UTexture* WeightIconTex;         // +0xBC "L2UI.WindowIcon.Weight"
    NCInvenItemWnd* ItemWnd[2];      // +0xC0 the pages: 0 "Item", 1 "Quest"
    NCEquipItemWnd* EquipWnd;        // +0xC8
    NCTabControl* TabCtrl;           // +0xCC
    UBOOL bOverCrystallize;          // +0xD0 an item is carried over the crystallize icon
    UBOOL bOverTrash;                // +0xD4 ... over the trash icon
    INT TrashX;                      // +0xD8 (190, 315, 32 x 32)
    INT TrashY;                      // +0xDC
    INT TrashW;                      // +0xE0
    INT TrashH;                      // +0xE4
    INT CrystallizeX;                // +0xE8 (16, 315, 32 x 32)
    INT CrystallizeY;                // +0xEC
    INT CrystallizeW;                // +0xF0
    INT CrystallizeH;                // +0xF4

    NCInventoryWnd();                                                // 0x10065920
    // ~NCInventoryWnd                                               // 0x10065a00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100684d0
    virtual INT OnDestroy();                                         // 36 0x10065de0
    virtual void ShowWindow();                                       // 42 0x10065eb0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10065a50
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 1; } // 55 0x100098f0 (folded)

    // names ours
    UBOOL IsInTrash(INT X, INT Y);                                   // 0x100641f0 edges included
    UBOOL IsInCrystallize(INT X, INT Y);                             // 0x10064240
    void SetEnchantMode(UBOOL bOn);                                  // 0x10065f10 enchant scroll
    void ReloadTexts();                                              // 0x10065f90 names and tool tips
    NItemEntry* FindItem(INT ObjectID);                              // 0x10066120 paperdoll, "Item" page
    void ClearItems();                                               // 0x100681a0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100681d0
    UBOOL UpdateItem(NItemInfo* Info, INT Change);                   // 0x10068220 1 added, 2 changed,
                                                                     //   3 removed
};
NW_ASSERT_OFFSET(NCInventoryWnd, WeightTex, 0xA0);
NW_ASSERT_OFFSET(NCInventoryWnd, ItemWnd, 0xC0);
NW_ASSERT_OFFSET(NCInventoryWnd, bOverCrystallize, 0xD0);
NW_ASSERT_OFFSET(NCInventoryWnd, TrashX, 0xD8);
NW_ASSERT_OFFSET(NCInventoryWnd, CrystallizeX, 0xE8);
NW_ASSERT_SIZE(NCInventoryWnd, 0xF8);

// The item table's search by the item id (NDataMap::FindRef, 0x1004dda0): the entry, NULL when
// there is none.
namespace NItemTable
{
    inline NViewItemData* Find(INT* ItemID)
    {
        return GL2GameData.ItemTable->FindRef(*ItemID);
    }
}

// a system line in the desktop's chat window
inline void DesktopSystemMessage(const TCHAR* Text)
{
    GDesktop->AddSystemText(Text, 0, 0xFFFFFFFF);
}

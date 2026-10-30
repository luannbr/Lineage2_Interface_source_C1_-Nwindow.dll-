#pragma once
// NCQuestItemWnd - the quest window's "Item" page (UClass "CQuestItemWnd", 0x118): the quest
// items, 6 x 4 icons of 32 pixels (at most 80). The selected item's tool tip lists its name
// (enchant, count), slot and description. An item dropped on the quest window's drop zone
// asks whether to destroy it (how many, when stacked).

#include "nwindow/NCItemWnd.h"

class NCQuestItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCQuestItemWnd, NCItemWnd, 0, 0x10148838)

    NCQuestItemWnd();                                                // 0x100a17c0
    NCQuestItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x100a1860
    // ~NCQuestItemWnd                                               // 0x100a1810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1009fe70
    virtual INT OnCreate();                                          // 35 0x1009fee0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a0b40
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100a0d00
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x100a0070
    // names ours
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 68 0x100a0f10

    // names ours
    UBOOL AddItem(NItemInfo* Info);                                  // 0x100a1490 0 when full
    void SetToolTip(NItemEntry* Item);                               // 0x1009fef0
};
NW_ASSERT_SIZE(NCQuestItemWnd, 0x118);

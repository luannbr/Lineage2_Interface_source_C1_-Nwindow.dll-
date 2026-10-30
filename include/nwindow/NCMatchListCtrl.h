#pragma once
// NCMatchListCtrl - the party matching list (UClass "CMatchListCtrl", 0x210): class icon,
// level icon, name, range and party status per player (NCMatchWnd fills it). Class and level
// sort by the key the window stored in the cell's Param1; a click selects the player and asks
// the server for the details the tool tip shows (level, class and memo, when open).
//
// Row data (NCMatchWnd): Cells[0].Param2 class ID, Cells[1].Param2 level, Cells[2].Param1 the
// player's object ID; row Param1 / Param2: level / class open, row Text: the memo.

#include "nwindow/NCListCtrl.h"

class NCMatchListCtrl : public NCListCtrl
{
    NW_DECLARE_CLASS(NCMatchListCtrl, NCListCtrl, 0, 0x1013a090)

    INT Unknown20C;                  // +0x20C not used

    NCMatchListCtrl();                                               // 0x10075ea0
    NCMatchListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x10075f40
    // ~NCMatchListCtrl                                              // 0x10075ef0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10073dc0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100750a0
    virtual INT SortByColumn(INT Column);                            // 68 0x10075c90
    virtual void DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y); // 69 0x10073f90

    // names ours
    void BuildToolTip(NListRow* Row);                                // 0x10073e30
    INT GetRowPlayerID(INT Index);                                   // 0x10075000 -1: none
    INT GetSelectedPlayerID();                                       // 0x10075030 -1: none
    const TCHAR* GetSelectedName();                                  // 0x10075060 NULL: none
};
NW_ASSERT_OFFSET(NCMatchListCtrl, Unknown20C, 0x20C);
NW_ASSERT_SIZE(NCMatchListCtrl, 0x210);

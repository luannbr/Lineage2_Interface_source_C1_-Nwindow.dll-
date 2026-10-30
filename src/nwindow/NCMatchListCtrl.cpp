/*=============================================================================
    NCMatchListCtrl.cpp - the party matching list. Stock: 0x10073dc0 .. 0x100744de,
    0x10075000 .. 0x10075092, 0x100750a0 .. 0x10075107, 0x10075c90 .. 0x10075e97,
    0x10075ea0 .. 0x10075f67.
=============================================================================*/
#include "nwindow/NCMatchListCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <wchar.h>

// sysstring lines
enum
{
    STR_Cls = 391,
    STR_Lv = 392,
    STR_NotOpen = 411,
};

enum { MATCH_TEXT_COLOR = 0xFFB09B79, MATCH_LABEL_COLOR = 0xFFA3A3A3, MATCH_MEMO_COLOR = 0xFFB2BECF };

// NWM_COMMAND id the window procedure sorts on; no control sends it (the headers report
// NID_LISTCTRL_HEADER)
enum { NID_MATCHLIST_SORT = 0xF7 };

// 0x10075ea0
NCMatchListCtrl::NCMatchListCtrl()
{
}
NW_HOOK_CTOR(0x10075ea0, NCMatchListCtrl)

// 0x10075f40
NCMatchListCtrl::NCMatchListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused)
    : NCListCtrl(InMaxRows, InVisibleRows, InMaxColumns, Unused)
{
}
NW_HOOK_CTOR_ARGS(0x10075f40, NCMatchListCtrl, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10075ef0
NW_HOOK_DTOR(0x10075ef0, NCMatchListCtrl)

// 0x10073dc0 (vslot 24)
INT NCMatchListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCListCtrl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_MATCHLIST_SORT)
        return SortByColumn((INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10073dc0, NCMatchListCtrl, WindowProc)

// 0x100750a0 (vslot 56) - the owner gets the row; without a tool tip yet, the server is
// asked for the player's details (HoverRow is left alone, unlike NCListCtrl's)
INT NCMatchListCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Rows.Num())
    {
        NotifyParent(Selected);
        NListRow* Row = Rows(Selected);
        if (Row && !Row->GetToolTip())
            GConsole->RequestPartyMatchDetail(GetSelectedPlayerID());
    }
    return 1;
}
NW_HOOK_METHOD(0x100750a0, NCMatchListCtrl, OnLButtonDown)

// 0x10075c90 (vslot 68) - NCListCtrl's bubble sort, but class and level (columns 0 and 1)
// sort by the key in Param1, compared unsigned, and rows without the icon stay in place
INT NCMatchListCtrl::SortByColumn(INT Column)
{
    NListRow* SelRow = GetSelectedRow();
    for (INT i = 0; i < Rows.Num(); i++)
    {
        for (INT j = 0; j < Rows.Num() - i && j < Rows.Num() - 1; j++)
        {
            UBOOL bSwap;
            if (Column == 0 || Column == 1)
            {
                NListCell* A = &Rows(j)->Cells[Column];
                if (!A->Icon)
                    continue;
                NListCell* B = &Rows(j + 1)->Cells[Column];
                if (!B->Icon)
                    continue;
                if (Columns(Column)->bAscending)
                    bSwap = (DWORD)A->Param1 > (DWORD)B->Param1;
                else
                    bSwap = (DWORD)A->Param1 < (DWORD)B->Param1;
            }
            else
            {
                const TCHAR* A = Rows(j)->Cells[Column].Text;
                if (!*A)
                    continue;
                const TCHAR* B = Rows(j + 1)->Cells[Column].Text;
                if (!*B)
                    continue;
                UBOOL bAscending = Columns(Column)->bAscending;
                INT Result = _wcsicmp(A, B);
                bSwap = bAscending ? Result > 0 : Result < 0;
            }
            if (bSwap)
            {
                NListRow* Temp = Rows(j);
                Rows(j) = Rows(j + 1);
                Rows(j + 1) = Temp;
            }
        }
    }
    Columns(Column)->bAscending = !Columns(Column)->bAscending;
    if (SelRow)
    {
        for (Selected = 0; Selected < Rows.Num(); Selected++)
        {
            if (Rows(Selected) == SelRow)
                break;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10075c90, NCMatchListCtrl, SortByColumn)

// 0x10073f90 (vslot 69) - "Lv : n", "Cls : name" ("Not open" when the player keeps them
// hidden), then the memo wrapped at 168 pixels
void NCMatchListCtrl::DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y)
{
    if (!Row)
        return;
    TCHAR Buf[128];  // Row->Text goes into it too: loot rule and the player's memo
    SIZE Size;

    appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[STR_Lv]);
    Device->DrawText(X, Y, MATCH_LABEL_COLOR, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT(" : "));
    Device->DrawText(Size.cx + X, Y, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%s : "), *GL2GameData.SysString[STR_Lv]);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    if (Row->Param1)
        appSprintf(Buf, TEXT("%d"), Row->Cells[1].Param2);
    else
        appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[STR_NotOpen]);
    Device->DrawText(Size.cx + X, Y, MATCH_TEXT_COLOR, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;

    appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[STR_Cls]);
    Device->DrawText(X, Y, MATCH_LABEL_COLOR, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT(" : "));
    Device->DrawText(Size.cx + X, Y, 0xFFFFFFFF, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Buf, TEXT("%s : "), *GL2GameData.SysString[STR_Cls]);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    if (Row->Param2)
        appSprintf(Buf, TEXT("%s"), GConsole->GetClassNameString(Row->Cells[0].Param2));
    else
        appSprintf(Buf, TEXT("%s"), *GL2GameData.SysString[STR_NotOpen]);
    Device->DrawText(Size.cx + X, Y, MATCH_TEXT_COLOR, Buf, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Y += 13;

    const TCHAR* Memo = Row->Text;
    if (Memo && *Memo)
    {
        appSprintf(Buf, TEXT("%s"), Memo);
        Device->DrawText(X, Y, MATCH_MEMO_COLOR, Buf, 168, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 1);
    }
}
NW_HOOK_METHOD(0x10073f90, NCMatchListCtrl, DrawRowToolTip)

// 0x10073e30 - the tool tip text (NCListCtrl sizes its box with it; DrawRowToolTip draws
// the same lines from the row data)
void NCMatchListCtrl::BuildToolTip(NListRow* Row)
{
    TCHAR ClassLine[32];
    TCHAR LevelLine[32];
    if (!Row)
        return;
    if (Row->Param1)
        appSprintf(LevelLine, TEXT("%s : %d"), *GL2GameData.SysString[STR_Lv],
                   Row->Cells[1].Param2);
    else
        appSprintf(LevelLine, TEXT("%s : %s"), *GL2GameData.SysString[STR_Lv],
                   *GL2GameData.SysString[STR_NotOpen]);
    if (Row->Param2)
        appSprintf(ClassLine, TEXT("%s : %s"), *GL2GameData.SysString[STR_Cls],
                   GConsole->GetClassNameString(Row->Cells[0].Param2));
    else
        appSprintf(ClassLine, TEXT("%s : %s"), *GL2GameData.SysString[STR_Cls],
                   *GL2GameData.SysString[STR_NotOpen]);
    const TCHAR* Memo = Row->Text;
    if (Memo && *Memo)
        appSprintf(Row->ToolTip, TEXT("%s\n%s\n%s"), LevelLine, ClassLine, Memo);
    else
        appSprintf(Row->ToolTip, TEXT("%s\n%s"), LevelLine, ClassLine);
}
NW_HOOK_METHOD(0x10073e30, NCMatchListCtrl, BuildToolTip)

// 0x10075000 - the object ID in the name cell
INT NCMatchListCtrl::GetRowPlayerID(INT Index)
{
    if (Index >= 0 && Index < Rows.Num() && Rows(Index))
    {
        INT ID = Rows(Index)->Cells[2].Param1;
        if (ID != 0)
            return ID;
    }
    return -1;
}
NW_HOOK_METHOD(0x10075000, NCMatchListCtrl, GetRowPlayerID)

// 0x10075030
INT NCMatchListCtrl::GetSelectedPlayerID()
{
    if (Selected >= 0 && Selected < Rows.Num() && Rows(Selected))
    {
        INT ID = Rows(Selected)->Cells[2].Param1;
        if (ID != 0)
            return ID;
    }
    return -1;
}
NW_HOOK_METHOD(0x10075030, NCMatchListCtrl, GetSelectedPlayerID)

// 0x10075060
const TCHAR* NCMatchListCtrl::GetSelectedName()
{
    if (Selected >= 0 && Selected < Rows.Num() && Rows(Selected))
    {
        const TCHAR* Name = Rows(Selected)->Cells[2].Text;
        if (Name && *Name)
            return Name;
    }
    return NULL;
}
NW_HOOK_METHOD(0x10075060, NCMatchListCtrl, GetSelectedName)

NW_CHECK_VTABLE(NCMatchListCtrl, 0x100e7888, 70)

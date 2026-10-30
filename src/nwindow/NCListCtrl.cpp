/*=============================================================================
    NCListCtrl.cpp - list with columns: cells, rows, column headers (NCHeaderCtrl) and the
    list itself. Stock: 0x100186f0 .. 0x100189d7, 0x100189f0 .. 0x1001a43d (0x100189e0 is
    NCButton::OnDestroy, shared with NCHeaderCtrl; 0x10019880 and 0x1001a010 are the
    TArray<pointer> templates Remove and ~TArray).
=============================================================================*/
#include "nwindow/NCListCtrl.h"
#include "nwindow/NCListBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCrt.h"

#include <wchar.h>

// interface sounds (FL2NetNotify::OnInterfacePlaySound)
enum { SOUND_Click = 0, SOUND_ClickDisabled = 2 };

// the row tool tip box: 184 pixels wide, text wrapped at 168
enum { TIP_Width = 184, TIP_TextWidth = 168 };

/*-----------------------------------------------------------------------------
    NListCell.
-----------------------------------------------------------------------------*/

// 0x100186f0
void NListCell::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x100186f0, NListCell, SetText)

// 0x10018720
void NListCell::SetNumber(INT Value)
{
    appSprintf(Text, TEXT("%d"), Value);
}
NW_HOOK_METHOD(0x10018720, NListCell, SetNumber)

// 0x10018740
void NListCell::SetIcon(UTexture* InIcon, INT W, INT H)
{
    if (InIcon)
    {
        Icon = InIcon;
        IconW = W;
        IconH = H;
    }
}
NW_HOOK_METHOD(0x10018740, NListCell, SetIcon)

// 0x10018770
void NListCell::SetText2(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text2, InText);
    else
        Text2[0] = 0;
}
NW_HOOK_METHOD(0x10018770, NListCell, SetText2)

// 0x100187a0
void NListCell::SetParams(INT InParam1, INT InParam2)
{
    Param1 = InParam1;
    Param2 = InParam2;
}
NW_HOOK_METHOD(0x100187a0, NListCell, SetParams)

// 0x100187c0
void NListCell::SetColor(DWORD InColor)
{
    Color = InColor;
}
NW_HOOK_METHOD(0x100187c0, NListCell, SetColor)

/*-----------------------------------------------------------------------------
    NListColumnInfo.
-----------------------------------------------------------------------------*/

// 0x100187d0
void NListColumnInfo::SetTitleID(INT TextID)
{
    if (*GL2GameData.SysString[TextID])
        appStrcpy(Title, *GL2GameData.SysString[TextID]);
    else
        Title[0] = 0;
}
NW_HOOK_METHOD(0x100187d0, NListColumnInfo, SetTitleID)

// 0x10018830
void NListColumnInfo::SetTitle(const TCHAR* InTitle)
{
    if (InTitle && *InTitle)
        appStrcpy(Title, InTitle);
    else
        Title[0] = 0;
}
NW_HOOK_METHOD(0x10018830, NListColumnInfo, SetTitle)

// 0x10018860
void NListColumnInfo::SetNormalTex(const TCHAR* Name)
{
    if (Name && *Name)
        appStrcpy(NormalTex, Name);
    else
        NormalTex[0] = 0;
}
NW_HOOK_METHOD(0x10018860, NListColumnInfo, SetNormalTex)

// 0x10018890
void NListColumnInfo::SetPushedTex(const TCHAR* Name)
{
    if (Name && *Name)
        appStrcpy(PushedTex, Name);
    else
        PushedTex[0] = 0;
}
NW_HOOK_METHOD(0x10018890, NListColumnInfo, SetPushedTex)

/*-----------------------------------------------------------------------------
    NListRow.
-----------------------------------------------------------------------------*/

// 0x10018fa0
NListRow::NListRow(INT NumCells)
{
    Cells = new NListCell[NumCells];
    Param1 = -1;
    Param2 = -1;
    Text[0] = 0;
    ToolTip[0] = 0;
}
NW_HOOK_CTOR_ARGS(0x10018fa0, NListRow, (INT A), (A))

// 0x10019040
NListRow::~NListRow()
{
    if (Cells)
    {
        delete[] Cells;
        Cells = NULL;
    }
}
NW_HOOK_DTOR(0x10019040, NListRow)

// 0x100188c0
void NListRow::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x100188c0, NListRow, SetText)

// 0x100188f0
void NListRow::SetParams(INT InParam1, INT InParam2)
{
    Param1 = InParam1;
    Param2 = InParam2;
}
NW_HOOK_METHOD(0x100188f0, NListRow, SetParams)

// 0x10018910
const TCHAR* NListRow::GetToolTip()
{
    const TCHAR* Tip = ToolTip;
    if (!Tip || !*Tip)
        return NULL;
    return Tip;
}
NW_HOOK_METHOD(0x10018910, NListRow, GetToolTip)

/*-----------------------------------------------------------------------------
    NCHeaderCtrl.
-----------------------------------------------------------------------------*/

// 0x10019060 - NControl's constructor is inlined; Title is left as allocated
NCHeaderCtrl::NCHeaderCtrl()
{
    NormalTex = NULL;
    PushedTex = NULL;
    bNumeric = 0;
    ColumnWidth = 0;
    ColumnHeight = 0;
    bReleased = 1;
    bClickable = 1;
    bAscending = 1;
}
NW_HOOK_CTOR(0x10019060, NCHeaderCtrl)

// 0x10019100
NW_HOOK_DTOR(0x10019100, NCHeaderCtrl)

// stock body shared with NCButton's (0x100051f0)
INT NCHeaderCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NControl::WindowProc(Msg, wParam, lParam) != 0;
}

// stock body shared with NCButton's (0x100189e0)
INT NCHeaderCtrl::OnDestroy()
{
    NControl::OnDestroy();
    return 1;
}

// 0x10019150 (vslot 50) - the texture's first 67 x 15 texels stretched over the header;
// Width and Height are halved unsigned, as in the stock code
INT NCHeaderCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCHeaderCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (bReleased)
    {
        if (NormalTex)
            Device->DrawStretchedTex(0, 0, Width, Height, 0, 0, 67, 15, NormalTex);
    }
    else if (PushedTex)
        Device->DrawStretchedTex(0, 0, Width, Height, 0, 0, 67, 15, PushedTex);
    const TCHAR* Text = Title;
    if (Text && *Text)
    {
        SIZE Size;
        NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
        Device->DrawText(((DWORD)Width >> 1) - Size.cx / 2, ((DWORD)Height >> 1) - Size.cy / 2 + 1,
                         0xFFFFFFFF, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10019150, NCHeaderCtrl, OnPaint)

// 0x10018930 (vslot 56) - pushed; the click sound plays wherever the button went down
INT NCHeaderCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bReleased = 0;
    GConsole->Engine->OnInterfacePlaySound(IsEnabled() ? SOUND_Click : SOUND_ClickDisabled);
    return 1;
}
NW_HOOK_METHOD(0x10018930, NCHeaderCtrl, OnLButtonDown)

// 0x10018990 (vslot 57) - released over the header: the list sorts by this column
INT NCHeaderCtrl::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)) && bClickable && NotifyID != -1)
        NotifyParent(NotifyID);
    bReleased = 1;
    return 1;
}
NW_HOOK_METHOD(0x10018990, NCHeaderCtrl, OnLButtonUp)

// 0x100189f0
void NCHeaderCtrl::SetColumnInfo(NListColumnInfo* Info)
{
    const TCHAR* InTitle = Info->Title;
    if (InTitle && *InTitle)
        appStrcpy(Title, InTitle);
    bClickable = Info->bClickable;
    bNumeric = Info->bNumeric;
    ColumnWidth = Info->Width;
    ColumnHeight = Info->Height;
    const TCHAR* Normal = Info->NormalTex;
    NormalTex = FindTexture(Normal && *Normal ? Normal : TEXT("NWindow.BlackTexture"));
    const TCHAR* Pushed = Info->PushedTex;
    PushedTex = FindTexture(Pushed && *Pushed ? Pushed : TEXT("NWindow.WhiteTexture"));
}
NW_HOOK_METHOD(0x100189f0, NCHeaderCtrl, SetColumnInfo)

NW_CHECK_VTABLE(NCHeaderCtrl, 0x100dcd98, 63)

/*-----------------------------------------------------------------------------
    NCListCtrl.
-----------------------------------------------------------------------------*/

// 0x1001a0a0 - NControl's constructor is inlined; HeaderHeight is left as allocated
NCListCtrl::NCListCtrl()
{
    BackTex = NULL;
    HighlightTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopRow = 0;
    NumRows = 0;
    TextX = 0;
    TextY = 0;
    MaxRows = 100;
    VisibleRows = 10;
    MaxColumns = 3;
    LineGap = 1;
    Unknown1F8 = 16;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_CTOR(0x1001a0a0, NCListCtrl)

// 0x1001a220 - the fourth argument is never read (the clan window passes 12)
NCListCtrl::NCListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused)
{
    BackTex = NULL;
    HighlightTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    MaxRows = InMaxRows;
    VisibleRows = InVisibleRows;
    ScrollUpButton = NULL;
    ScrollDownButton = NULL;
    ScrollBar = NULL;
    TopRow = 0;
    NumRows = 0;
    TextX = 0;
    TextY = 0;
    MaxColumns = InMaxColumns;
    LineGap = 1;
    Unknown1F8 = 12;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_CTOR_ARGS(0x1001a220, NCListCtrl, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1001a1b0
NW_HOOK_DTOR(0x1001a1b0, NCListCtrl)

// 0x10018aa0 (vslot 24) - the headers, the scroll buttons and the slider report here
INT NCListCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if ((INT)wParam == NID_LISTCTRL_HEADER)
            return SortByColumn((INT)lParam) != 0;
        if ((INT)wParam == NID_LIST_SCROLLUP)
            return ScrollUp() != 0;
        if ((INT)wParam == NID_LIST_SCROLLDOWN)
            return ScrollDown() != 0;
        if ((INT)wParam == NID_LIST_SCROLLBAR)
            return SetTopRow((INT)lParam) != 0;
    }
    else if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10018aa0, NCListCtrl, WindowProc)

// 0x10018b90 (vslot 35) - the scroll controls wait for the first column (AddColumn)
INT NCListCtrl::OnCreate()
{
    NControl::OnCreate();
    HighlightTex = FindTexture(TEXT("NWindow.MenuHighlight"));
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    return 1;
}
NW_HOOK_METHOD(0x10018b90, NCListCtrl, OnCreate)

// 0x10019910 (vslot 36)
INT NCListCtrl::OnDestroy()
{
    for (INT i = 0; i < Rows.Num(); i++)
        delete Rows(i);
    for (INT i = 0; i < Columns.Num(); i++)
    {
        NCHeaderCtrl* Column = Columns(i);
        if (Column)
        {
            RemoveChildWindow(Column->Entry);
            delete Column;
        }
    }
    Rows.Empty();
    Columns.Empty();
    if (ScrollUpButton)
    {
        RemoveChildWindow(ScrollUpButton->Entry);
        delete ScrollUpButton;
        ScrollUpButton = NULL;
    }
    if (ScrollDownButton)
    {
        RemoveChildWindow(ScrollDownButton->Entry);
        delete ScrollDownButton;
        ScrollDownButton = NULL;
    }
    if (ScrollBar)
    {
        RemoveChildWindow(ScrollBar->Entry);
        delete ScrollBar;
        ScrollBar = NULL;
    }
    Selected = -1;
    HoverRow = -1;
    TopRow = 0;
    NumRows = 0;
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10019910, NCListCtrl, OnDestroy)

// 0x100192b0 (vslot 50) - a grey bar under the selected row; each cell is clipped to its
// column. The icon is centered on the header's height, not the row's, as in the stock code.
INT NCListCtrl::OnPaint(NCGDevice* Device)
{
    guard(NCListCtrl::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    INT Line = 0;
    for (INT i = TopRow; i < TopRow + VisibleRows && i <= NumRows && i < Rows.Num(); i++, Line++)
    {
        Device->SetClip(ScreenX, ScreenY + HeaderHeight, Width, Height);
        if (i == Selected)
            Device->DrawRect(0, (LineGap + 12) * Line + TextY + HeaderHeight - 1, Width - 18,
                             LineGap + 12, 0xFF646464);
        Device->ReleaseClip();
        INT X = TextX;
        NListRow* Row = Rows(i);
        for (INT c = 0; c < Columns.Num(); c++)
        {
            NCHeaderCtrl* Column = Columns(c);
            INT Y = (LineGap + 12) * Line + TextY + HeaderHeight;
            Device->SetClip(ScreenX + X, ScreenY + Y - 1, Column->ColumnWidth, 14);
            NListCell& Cell = Row->Cells[c];
            if (Cell.Icon)
                Device->DrawTexture(Column->ColumnWidth / 2 - Cell.IconW / 2 + X,
                                    Column->ColumnHeight / 2 - Cell.IconH / 2 + Y, Cell.IconW,
                                    Cell.IconH, 0, 0, Cell.IconW, Cell.IconH, Cell.Icon);
            Device->DrawText(Cell.IconW + X, Y, Cell.Color, Cell.Text, 0, 0, 0, L2FONT_Normal, 0, 0,
                             0, 0, 0, 0);
            X += Column->ColumnWidth;
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100192b0, NCListCtrl, OnPaint)

// 0x10019580 (vslot 51) - the tool tip of the selected row, while the mouse is over it:
// a box above the row (below it when there is no room), kept on the screen, its text left
// to DrawRowToolTip
INT NCListCtrl::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (HoverRow >= 0 && HoverRow < MaxRows && HoverRow == Selected)
    {
        NListRow* Row = Rows(Selected);
        INT Y = (HoverRow - TopRow) % VisibleRows * (LineGap + 12) + TextY + HeaderHeight;
        if (Row && Row->GetToolTip())
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Row->GetToolTip(), TIP_TextWidth, 0, 0, 0, 0);
            Size.cy += Size.cy / 12 + 2;
            INT X = 0;
            INT DesktopW = GConsole->Desktop->LocalW;
            if (ScreenX + TIP_Width > DesktopW)
                X = DesktopW - ScreenX - TIP_Width;
            else if (ScreenX < 0)
                X = -ScreenX;
            Y += -15 - Size.cy;
            if (ScreenY + Y < 0)
                Y += LineGap + Size.cy + 28;
            Device->SetClip(ScreenX + X, ScreenY + Y, TIP_Width, Size.cy / 12 * 13 + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, TIP_Width, 8, 0, 0, TIP_Width, 8, TipTopTex);
            INT Strips = Size.cy / 12;
            for (INT k = 0; k < Strips; k++)
            {
                if (TipMidTex)
                    Device->DrawTexture(X, Y + 8 + k * 13, TIP_Width, 13, 0, 0, TIP_Width, 13,
                                        TipMidTex);
            }
            if (TipBottomTex)
                Device->DrawTexture(X, Strips * 13 + Y + 8, TIP_Width, 8, 0, 0, TIP_Width, 8,
                                    TipBottomTex);
            DrawRowToolTip(Device, Row, X + 8, Y + 8);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10019580, NCListCtrl, OnDrawToolTip)

// 0x10018d20 (vslot 55)
INT NCListCtrl::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverRow = HitTest(LOWORD(lParam), HIWORD(lParam));
    return 1;
}
NW_HOOK_METHOD(0x10018d20, NCListCtrl, OnMouseMove)

// 0x10018ce0 (vslot 56) - selects without telling the owner (the stock code still tests the
// row against Rows.Num(), with nothing depending on it)
INT NCListCtrl::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Row = HitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Row;
    HoverRow = Row;
    return 1;
}
NW_HOOK_METHOD(0x10018ce0, NCListCtrl, OnLButtonDown)

// 0x10018da0 (vslot 61) - one row per wheel notch (120)
INT NCListCtrl::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    INT Delta = Notches;
    if (Notches > 0)
    {
        if (TopRow > Delta)
            TopRow -= Delta;
        else
            TopRow = 0;
    }
    else if (NumRows >= VisibleRows)
    {
        if (NumRows - TopRow + Delta > VisibleRows)
            TopRow -= Delta;
        else
            TopRow = NumRows - VisibleRows;
    }
    else
    {
        if (NumRows - TopRow + Delta > VisibleRows)
            TopRow -= Delta;
        else
            TopRow = 0;
    }
    ScrollBar->SetValue(TopRow);
    return 1;
}
NW_HOOK_METHOD(0x10018da0, NCListCtrl, OnMouseWheel)

// 0x10019f40 (vslot 63) - rows and columns go; the scroll controls stay
void NCListCtrl::Clear()
{
    for (INT i = 0; i < Rows.Num(); i++)
        delete Rows(i);
    for (INT i = 0; i < Columns.Num(); i++)
    {
        NCHeaderCtrl* Column = Columns(i);
        if (Column)
        {
            RemoveChildWindow(Column->Entry);
            delete Column;
        }
    }
    Rows.Empty();
    Columns.Empty();
    ScrollBar->Reset();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_METHOD(0x10019f40, NCListCtrl, Clear)

// 0x10018d50 (vslot 64) - the owner gets the row (NWM_COMMAND, lParam = index)
INT NCListCtrl::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    INT Row = HitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Row;
    HoverRow = Row;
    if (Row >= 0 && Row < Rows.Num())
        NotifyParent(Row);
    return 1;
}
NW_HOOK_METHOD(0x10018d50, NCListCtrl, OnLButtonDblClick)

// 0x10018e50 (vslot 65)
INT NCListCtrl::ScrollUp()
{
    if (NumRows > VisibleRows)
    {
        if (--TopRow < 0)
            TopRow = 0;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x10018e50, NCListCtrl, ScrollUp)

// 0x10018e90 (vslot 66)
INT NCListCtrl::ScrollDown()
{
    if (NumRows > VisibleRows)
    {
        INT Last = NumRows - VisibleRows;
        if (++TopRow > Last)
            TopRow = Last;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x10018e90, NCListCtrl, ScrollDown)

// 0x10018ee0 (vslot 67) - from the slider
INT NCListCtrl::SetTopRow(INT Row)
{
    TopRow = Row;
    return 1;
}
NW_HOOK_METHOD(0x10018ee0, NCListCtrl, SetTopRow)

// 0x10019a70 (vslot 68) - bubble sort on the column's text (or value); rows with the cell
// empty, or next to one, stay where they are. The selection follows its row.
INT NCListCtrl::SortByColumn(INT Column)
{
    NListRow* SelRow = GetSelectedRow();
    for (INT i = 0; i < Rows.Num(); i++)
    {
        for (INT j = 0; j < Rows.Num() - i && j < Rows.Num() - 1; j++)
        {
            const TCHAR* A = Rows(j)->Cells[Column].Text;
            if (!*A)
                continue;
            const TCHAR* B = Rows(j + 1)->Cells[Column].Text;
            if (!*B)
                continue;
            NCHeaderCtrl* Header = Columns(Column);
            UBOOL bSwap;
            if (Header->bNumeric)
            {
                if (Header->bAscending)
                    bSwap = NWtol(A) > NWtol(B);
                else
                    bSwap = NWtol(A) < NWtol(B);
            }
            else
            {
                if (Header->bAscending)
                    bSwap = _wcsicmp(A, B) > 0;
                else
                    bSwap = _wcsicmp(A, B) < 0;
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
NW_HOOK_METHOD(0x10019a70, NCListCtrl, SortByColumn)

// 0x10018c00 - the row under a screen point, among the ones shown; -1 outside the rows
INT NCListCtrl::HitTest(INT X, INT Y)
{
    INT RX = X - ScreenX;
    INT RY = Y - ScreenY;
    INT Result = -1;
    if (RX < 0 || RX > Width || RY < HeaderHeight || RY > Height)
        return -1;
    INT Line = 0;
    for (INT i = TopRow; i < TopRow + VisibleRows; i++, Line++)
    {
        if (i >= NumRows)
            break;
        if (RX < Width && RY >= (LineGap + 12) * Line + TextY + HeaderHeight &&
            RY < (Line + 1) * (LineGap + 12) + TextY + HeaderHeight)
        {
            Result = i;
            break;
        }
    }
    return Result;
}
NW_HOOK_METHOD(0x10018c00, NCListCtrl, HitTest)

// 0x10018f00
TArray<NListRow*>* NCListCtrl::GetRows()
{
    return &Rows;
}
NW_HOOK_METHOD(0x10018f00, NCListCtrl, GetRows)

// 0x10018f10
INT NCListCtrl::GetRowCount()
{
    return Rows.Num();
}
NW_HOOK_METHOD(0x10018f10, NCListCtrl, GetRowCount)

// 0x10018f20
void NCListCtrl::SetSelected(INT Index)
{
    if (Index >= 0 && Index < Rows.Num())
        Selected = Index;
}
NW_HOOK_METHOD(0x10018f20, NCListCtrl, SetSelected)

// 0x10018f40
void NCListCtrl::SetTextOffset(INT X, INT Y)
{
    TextX = X;
    TextY = Y;
}
NW_HOOK_METHOD(0x10018f40, NCListCtrl, SetTextOffset)

// 0x10018f60
void NCListCtrl::UpdateScrollRange()
{
    NumRows = Rows.Num();
    ScrollBar->SetRange(Rows.Num(), VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x10018f60, NCListCtrl, UpdateScrollRange)

// 0x100197f0
NListRow* NCListCtrl::GetSelectedRow()
{
    if (Selected >= 0 && Selected < Rows.Num())
        return Rows(Selected);
    return NULL;
}
NW_HOOK_METHOD(0x100197f0, NCListCtrl, GetSelectedRow)

// 0x10019810 - the scroll buttons are created without auto-repeat, so this has no effect
void NCListCtrl::SetRepeatDelay(INT Delay)
{
    if (ScrollUpButton && ScrollDownButton)
    {
        ScrollUpButton->RepeatDelay = Delay;
        ScrollDownButton->RepeatDelay = Delay;
    }
}
NW_HOOK_METHOD(0x10019810, NCListCtrl, SetRepeatDelay)

// 0x10019840 - the range uses TopRow as the total (0 at creation), as in the stock code
void NCListCtrl::InitScrollBar()
{
    SetRepeatDelay(200);
    ScrollBar->SetRange(TopRow, VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x10019840, NCListCtrl, InitScrollBar)

// 0x10019c50 - full lists refuse the row (the caller still owns it then)
INT NCListCtrl::AddRow(NListRow* Row)
{
    if (MaxRows <= Rows.Num())
        return 0;
    if (Row)
        Rows.AddItem(Row);
    UpdateScrollRange();
    return 1;
}
NW_HOOK_METHOD(0x10019c50, NCListCtrl, AddRow)

// 0x10019ca0 - the columns stay
void NCListCtrl::ClearRows()
{
    for (INT i = 0; i < Rows.Num(); i++)
        delete Rows(i);
    Rows.Empty();
    ScrollBar->Reset();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverRow = -1;
}
NW_HOOK_METHOD(0x10019ca0, NCListCtrl, ClearRows)

// 0x10019d20 - up / down buttons and the slider along the right edge
void NCListCtrl::CreateScrollControls()
{
    INT W = LocalW;
    INT H = LocalH;

    ScrollUpButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollUp"),
                                      TEXT("L2UI.NWindow.WindowScrollUp"));
    ScrollUpButton->ID = NID_LIST_SCROLLUP;
    ScrollUpButton->SetWindowRect(W - 12, 0, 12, 12);
    CreateChild(ScrollUpButton, NWS_VISIBLE);

    ScrollDownButton = new NCPushButton(TEXT("L2UI.NWindow.WindowScrollDown"),
                                        TEXT("L2UI.NWindow.WindowScrollDown"));
    ScrollDownButton->ID = NID_LIST_SCROLLDOWN;
    ScrollDownButton->SetWindowRect(W - 12, H - 12, 12, 12);
    CreateChild(ScrollDownButton, NWS_VISIBLE);

    ScrollBar = new NCSliderBar(TEXT("L2UI.NWindow.WindowScrollBar1"),
                                TEXT("L2UI.NWindow.WindowScrollBar2"),
                                TEXT("L2UI.NWindow.WindowScrollBar3"));
    ScrollBar->ID = NID_LIST_SCROLLBAR;
    ScrollBar->SetWindowRect(W - 12, 12, 12, H - 24);
    CreateChild(ScrollBar, NWS_VISIBLE);

    InitScrollBar();
}
NW_HOOK_METHOD(0x10019d20, NCListCtrl, CreateScrollControls)

// 0x1001a310 - a header after the existing ones (Index is only checked against MaxColumns:
// NCServerSelectWnd passes 0..3, the other owners 0); the rows start under the last header added
INT NCListCtrl::AddColumn(INT Index, NListColumnInfo* Info)
{
    if (Index > MaxColumns)
        return 0;
    NCHeaderCtrl* Header = new NCHeaderCtrl;
    Header->SetColumnInfo(Info);
    Header->ID = NID_LISTCTRL_HEADER;
    INT X = 0;
    for (INT i = 0; i < Columns.Num(); i++)
        X += Columns(i)->ColumnWidth;
    Header->SetWindowRect(X, 0, Info->Width, Info->Height);
    CreateChild(Header, NWS_VISIBLE);
    Header->NotifyID = Columns.Num();
    Columns.AddItem(Header);
    if (!ScrollBar)
        CreateScrollControls();
    UpdateScrollRange();
    HeaderHeight = Info->Height;
    return 1;
}
NW_HOOK_METHOD(0x1001a310, NCListCtrl, AddColumn)

NW_CHECK_VTABLE(NCListCtrl, 0x100dcee8, 70)

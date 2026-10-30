#pragma once
// NCListCtrl - list with columns (UClass "CListCtrl", 0x20C): a header button per column
// (NCHeaderCtrl; a click sorts the rows by that column, the next click reverses the order),
// rows of cells under them (text, color, optional icon), scroll buttons and a slider on the
// right. The owner builds each row (NListRow, one NListCell per column) and hands it over
// with AddRow; from then on the list owns it. A double click reports the row (NWM_COMMAND,
// lParam = index); the selected row shows its tool tip while the mouse is over it.
// The server list of the login screen, the clan members, party matching and the siege
// lists derive from it (NCServerSelectListCtrl, NCMatchListCtrl, NCSiegeInfoListCtrl).

#include "nwindow/NControl.h"

class NCPushButton;
class NCSliderBar;

// NWM_COMMAND id of the header buttons (NotifyID = column index)
enum { NID_LISTCTRL_HEADER = 0xF5 };

// A cell (name ours; 0xA1C bytes). What the list draws: Icon centered in the column, then
// Text after IconW pixels, in Color.
struct NListCell
{
    TCHAR Text[1024];          // +0x000
    UTexture* Icon;            // +0x800
    DWORD Color;               // +0x804 (-1: white)
    TCHAR Text2[256];          // +0x808 not drawn by the list (the owners keep data here)
    INT Param1;                // +0xA08 (-1) for the owner
    INT Param2;                // +0xA0C (-1)
    INT UnknownA10;            // +0xA10 (0)
    INT IconW;                 // +0xA14
    INT IconH;                 // +0xA18

    // inlined in NListRow's constructor
    NListCell()
    {
        Text[0] = 0;
        Icon = NULL;
        Color = 0xFFFFFFFF;
        Text2[0] = 0;
        Param1 = -1;
        Param2 = -1;
        UnknownA10 = 0;
        IconW = 0;
        IconH = 0;
    }
    void SetText(const TCHAR* InText);                   // 0x100186f0
    void SetNumber(INT Value);                           // 0x10018720 "%d"
    void SetIcon(UTexture* InIcon, INT W, INT H);        // 0x10018740 (NULL keeps the old one)
    void SetText2(const TCHAR* InText);                  // 0x10018770
    void SetParams(INT InParam1, INT InParam2);          // 0x100187a0
    void SetColor(DWORD InColor);                        // 0x100187c0
};
NW_ASSERT_OFFSET(NListCell, Icon, 0x800);
NW_ASSERT_OFFSET(NListCell, Text2, 0x808);
NW_ASSERT_OFFSET(NListCell, Param1, 0xA08);
NW_ASSERT_OFFSET(NListCell, IconW, 0xA14);
NW_ASSERT_SIZE(NListCell, 0xA1C);

// A row (name ours; 0x100C bytes, created by the owner with new).
struct NListRow
{
    NListCell* Cells;          // +0x000 one per column
    TCHAR Text[1024];          // +0x004 not drawn by the list
    INT Param1;                // +0x804 (-1) for the owner
    INT Param2;                // +0x808 (-1)
    TCHAR ToolTip[1024];       // +0x80C shown while the selected row is under the mouse

    NListRow(INT NumCells);                              // 0x10018fa0
    ~NListRow();                                         // 0x10019040
    void SetText(const TCHAR* InText);                   // 0x100188c0
    void SetParams(INT InParam1, INT InParam2);          // 0x100188f0
    const TCHAR* GetToolTip();                           // 0x10018910 NULL when empty
};
NW_ASSERT_OFFSET(NListRow, Text, 0x4);
NW_ASSERT_OFFSET(NListRow, Param1, 0x804);
NW_ASSERT_OFFSET(NListRow, ToolTip, 0x80C);
NW_ASSERT_SIZE(NListRow, 0x100C);

// What a column is made of (name ours; 0x1814 bytes, built on the owner's stack and handed
// to NCListCtrl::AddColumn).
struct NListColumnInfo
{
    INT Width;                 // +0x000
    INT Height;                // +0x004 of the header; the rows start under it
    TCHAR Title[1024];         // +0x008
    UBOOL bClickable;          // +0x808 (1) a click on the header sorts
    UBOOL bNumeric;            // +0x80C (0) sort by value (_wtol) instead of by text
    INT Unknown810;            // +0x810 (left as it was on the stack)
    TCHAR NormalTex[1024];     // +0x814 header textures, "NWindow.BlackTexture" /
    TCHAR PushedTex[1024];     // +0x1014  "NWindow.WhiteTexture" when empty

    // inlined in the owners
    NListColumnInfo()
    {
        Title[0] = 0;
        bClickable = 1;
        bNumeric = 0;
        NormalTex[0] = 0;
        PushedTex[0] = 0;
    }
    void SetTitleID(INT TextID);                         // 0x100187d0 sysstring line
    void SetTitle(const TCHAR* InTitle);                 // 0x10018830
    void SetNormalTex(const TCHAR* Name);                // 0x10018860
    void SetPushedTex(const TCHAR* Name);                // 0x10018890
};
NW_ASSERT_OFFSET(NListColumnInfo, Title, 0x8);
NW_ASSERT_OFFSET(NListColumnInfo, bClickable, 0x808);
NW_ASSERT_OFFSET(NListColumnInfo, NormalTex, 0x814);
NW_ASSERT_OFFSET(NListColumnInfo, PushedTex, 0x1014);
NW_ASSERT_SIZE(NListColumnInfo, 0x1814);

// NCHeaderCtrl - a column's header button (UClass "CHeaderCtrl", 0x9C0): its texture
// stretched, the title centered, pushed while the mouse button is down. Released over it,
// it tells the list (NID_LISTCTRL_HEADER, lParam = NotifyID = column index).
class NCHeaderCtrl : public NControl
{
    NW_DECLARE_CLASS(NCHeaderCtrl, NControl, 0, 0x10121918)

    UTexture* NormalTex;             // +0x1A0
    UTexture* PushedTex;             // +0x1A4
    TCHAR Title[1024];               // +0x1A8
    UBOOL bReleased;                 // +0x9A8 (1)
    UBOOL bClickable;                // +0x9AC (1)
    UBOOL bNumeric;                  // +0x9B0 (0) the column sorts by value
    UBOOL bAscending;                // +0x9B4 (1) order of the next sort; each sort flips it
    INT ColumnWidth;                 // +0x9B8
    INT ColumnHeight;                // +0x9BC

    NCHeaderCtrl();                                                  // 0x10019060
    // ~NCHeaderCtrl                                                 // 0x10019100

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnDestroy();                                         // 36 0x100189e0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10019150
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10018930
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10018990

    void SetColumnInfo(NListColumnInfo* Info);                       // 0x100189f0 (name ours)
};
NW_ASSERT_OFFSET(NCHeaderCtrl, Title, 0x1A8);
NW_ASSERT_OFFSET(NCHeaderCtrl, bReleased, 0x9A8);
NW_ASSERT_OFFSET(NCHeaderCtrl, ColumnWidth, 0x9B8);
NW_ASSERT_SIZE(NCHeaderCtrl, 0x9C0);

class NCListCtrl : public NControl
{
    NW_DECLARE_CLASS(NCListCtrl, NControl, 0, 0x10121e40)

    UTexture* BackTex;               // +0x1A0 set by the owner
    UTexture* HighlightTex;          // +0x1A4 "NWindow.MenuHighlight" (not drawn here)
    UTexture* ToolTipTex;            // +0x1A8 "L2UI.tooltip.mini-tool01" (not drawn here)
    UTexture* TipTopTex;             // +0x1AC "L2UI.Tooltip.Tool02"  the row tool tip: top,
    UTexture* TipMidTex;             // +0x1B0 "L2UI.Tooltip.Tool01"  a strip per text line,
    UTexture* TipBottomTex;          // +0x1B4 "L2UI.Tooltip.Tool03"  bottom
    TArray<NListRow*> Rows;          // +0x1B8
    TArray<NCHeaderCtrl*> Columns;   // +0x1C4
    NCPushButton* ScrollUpButton;    // +0x1D0 the scroll controls come with the first column
    NCPushButton* ScrollDownButton;  // +0x1D4
    NCSliderBar* ScrollBar;          // +0x1D8
    INT VisibleRows;                 // +0x1DC (10)
    INT MaxRows;                     // +0x1E0 (100) AddRow refuses more
    INT MaxColumns;                  // +0x1E4 (3)
    INT TopRow;                      // +0x1E8 first row shown
    INT NumRows;                     // +0x1EC rows when the scroll range was last set
    INT Selected;                    // +0x1F0 -1 = none
    INT HoverRow;                    // +0x1F4 row under the mouse, -1 = none
    INT Unknown1F8;                  // +0x1F8 (16; 12 from the sized constructor), never read
    INT HeaderHeight;                // +0x1FC the rows start under the headers (AddColumn)
    INT TextX;                       // +0x200 where the cells start
    INT TextY;                       // +0x204
    INT LineGap;                     // +0x208 (1) rows are 12 + LineGap high

    NCListCtrl();                                                    // 0x1001a0a0
    NCListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x1001a220
    // ~NCListCtrl                                                   // 0x1001a1b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10018aa0
    virtual INT OnCreate();                                          // 35 0x10018b90
    virtual INT OnDestroy();                                         // 36 0x10019910
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100192b0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x10019580
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x10018d20
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10018ce0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x10018da0
    // names ours
    virtual void Clear();                                            // 63 0x10019f40 rows and columns
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 64 0x10018d50
    virtual INT ScrollUp();                                          // 65 0x10018e50
    virtual INT ScrollDown();                                        // 66 0x10018e90
    virtual INT SetTopRow(INT Row);                                  // 67 0x10018ee0
    virtual INT SortByColumn(INT Column);                            // 68 0x10019a70
    // text of the row tool tip, drawn at (X, Y) in its box (the subclasses draw it)
    virtual void DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y) {} // 69 0x10016720 (folded)

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x10018c00 row, -1 = none
    TArray<NListRow*>* GetRows();                                    // 0x10018f00
    INT GetRowCount();                                               // 0x10018f10
    void SetSelected(INT Index);                                     // 0x10018f20
    void SetTextOffset(INT X, INT Y);                                // 0x10018f40
    void UpdateScrollRange();                                        // 0x10018f60
    NListRow* GetSelectedRow();                                      // 0x100197f0
    void SetRepeatDelay(INT Delay);                                  // 0x10019810
    void InitScrollBar();                                            // 0x10019840
    INT AddRow(NListRow* Row);                                       // 0x10019c50
    void ClearRows();                                                // 0x10019ca0
    void CreateScrollControls();                                     // 0x10019d20
    INT AddColumn(INT Index, NListColumnInfo* Info);                 // 0x1001a310
};
NW_ASSERT_OFFSET(NCListCtrl, Rows, 0x1B8);
NW_ASSERT_OFFSET(NCListCtrl, Columns, 0x1C4);
NW_ASSERT_OFFSET(NCListCtrl, ScrollBar, 0x1D8);
NW_ASSERT_OFFSET(NCListCtrl, TopRow, 0x1E8);
NW_ASSERT_OFFSET(NCListCtrl, Selected, 0x1F0);
NW_ASSERT_OFFSET(NCListCtrl, HeaderHeight, 0x1FC);
NW_ASSERT_OFFSET(NCListCtrl, LineGap, 0x208);
NW_ASSERT_SIZE(NCListCtrl, 0x20C);

/*=============================================================================
    NCItemWnd.cpp - grid of item icons, base of the item windows, and NItemEntry, the record
    of each item. Stock: 0x10015b80 .. 0x100178e0 (NCCoolTimeIcon's code sits in between:
    0x10016620 .. 0x10016850, 0x100170e0 .. 0x100172a0, 0x100173c0 .. 0x10017550) plus
    WindowProc (0x1006bb90).
=============================================================================*/
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NConsoleWnd.h"
#if NW_FEATURES
// phase 6: ItemHoverTip (not in NWindow_C1: the hover tip of the item grids in the recent style)
#include "features/ItemHoverTip.h"
#endif

enum { SOUND_PickUp = 3 };  // an item picked up (name ours)

// the desktop's message box (NDialogInfo::bMainBox) and the second one
static FORCEINLINE NCDialogBox* MainDialogBox()
{
    return GDesktop->MainDialogBox;
}
static FORCEINLINE NCDialogBox* SecondDialogBox()
{
    return GDesktop->SecondDialogBox;
}

/*-----------------------------------------------------------------------------
    NItemEntry
-----------------------------------------------------------------------------*/

// 0x10015b80
NItemEntry::NItemEntry()
{
    Name[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    ToolTipName[0] = 0;
    ToolTipText[0] = 0;
    TradeCount = 0;
    bSelected = 0;
    Type = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC20 = 0;
    UnknownC18 = -1;
    BodyPart = 0;
    Weight = 0;
    UnknownC2C = -1;
    UnknownC30 = -1;
    Unknown1C80 = 0;
    Unknown1C84 = 0;
    Unknown1C88 = 0;
    Unknown1C8C = 0;
    Unknown1C90 = 0;
    Unknown1C94 = 0;
    Unknown1C58 = 0;
    CrystalGrade = 0;
    Unknown1C60 = 0;
    Unknown1C64 = 0;
    Unknown1C68 = 0;
    Unknown1C6C = 0;
    Unknown1C70 = 0;
    SubType = -1;
    Unknown1C78 = 0;
    Unknown1C7C = 0;
    Enchant = 0;
    UnknownC3C = 0;
    bStackable = 0;
    bEquipped = 0;
    UnknownC34 = -1;
    Unknown1C98 = 0;
    Unknown1C9C = 0;
    Unknown1CA0 = 0;
    Unknown1CA4 = 0;
    Unknown1CA8 = 0;
}
NW_HOOK_CTOR(0x10015b80, NItemEntry)

// 0x10015cb0 - the fields do not keep NItemInfo's order; its +0xC54 / +0xC58 are dropped and
// Unknown1C60 / Unknown1C64 stay uninitialized. The stock code tests the addresses of the
// texts but the first before copying them (never NULL).
NItemEntry::NItemEntry(NItemInfo* Src)
{
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    Name[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    ToolTipName[0] = 0;
    ToolTipText[0] = 0;
    TradeCount = 0;
    bSelected = 0;

    Icon = Src->Icon;
    Unknown004 = Src->UnknownC04;
    Unknown008 = Src->UnknownC08;
    appStrcpy(Name, Src->Name);
    appStrcpy(IconName, Src->IconName);
    appStrcpy(Unknown20C, Src->Unknown200);
    appStrcpy(Unknown30C, Src->Unknown300);
    appStrcpy(Description, Src->Description);
    Type = Src->Param;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    UnknownC20 = Src->UnknownC1C;
    UnknownC18 = Src->UnknownC20;
    BodyPart = Src->BodyPart;
    Weight = Src->UnknownC28;
    UnknownC2C = Src->UnknownC2C;
    UnknownC30 = Src->UnknownC30;
    Unknown1C80 = Src->UnknownC34[0];
    Unknown1C84 = Src->UnknownC34[1];
    Unknown1C88 = Src->UnknownC34[2];
    Unknown1C8C = Src->UnknownC34[3];
    Unknown1C90 = Src->UnknownC34[4];
    Unknown1C94 = Src->UnknownC34[5];
    Unknown1C58 = Src->UnknownC34[6];
    CrystalGrade = Src->UnknownC34[7];
    Unknown1C68 = Src->UnknownC34[10];
    Unknown1C6C = Src->UnknownC34[11];
    Unknown1C70 = Src->UnknownC34[12];
    SubType = Src->SubType;
    Unknown1C78 = Src->UnknownC6C;
    Unknown1C7C = Src->UnknownC70;
    Enchant = Src->Enchant;
    UnknownC3C = Src->UnknownC84;
    bStackable = Src->bStackable;
    bEquipped = Src->UnknownC8C;
    UnknownC34 = Src->UnknownC90;
    Unknown1C98 = Src->UnknownC94;
    Unknown1C9C = Src->UnknownC78;
    Unknown1CA0 = Src->UnknownC7C;
    Unknown1CA4 = Src->UnknownC80;
    Unknown1CA8 = Src->UnknownC98;
}
NW_HOOK_CTOR_ARGS(0x10015cb0, NItemEntry, (NItemInfo* A), (A))

// 0x10015f30
const TCHAR* NItemEntry::GetToolTipName()
{
    const TCHAR* S = ToolTipName;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x10015f30, NItemEntry, GetToolTipName)

// 0x10015f50
const TCHAR* NItemEntry::GetToolTipText()
{
    const TCHAR* S = ToolTipText;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x10015f50, NItemEntry, GetToolTipText)

// 0x10015f70
const TCHAR* NItemEntry::GetDescription()
{
    const TCHAR* S = Description;
    return S && *S ? S : NULL;
}
NW_HOOK_METHOD(0x10015f70, NItemEntry, GetDescription)

// 0x10015f90
INT NItemEntry::GetUnknownC20()
{
    return UnknownC20;
}
NW_HOOK_METHOD(0x10015f90, NItemEntry, GetUnknownC20)

// 0x10015fa0
INT NItemEntry::GetUnknownC34()
{
    return UnknownC34;
}
NW_HOOK_METHOD(0x10015fa0, NItemEntry, GetUnknownC34)

// 0x10015fb0
void NItemEntry::SetUnknownC34(INT Value)
{
    UnknownC34 = Value;
}
NW_HOOK_METHOD(0x10015fb0, NItemEntry, SetUnknownC34)

// 0x10015fc0 - the texts and the first three fields only when set in Src; IconName,
// Unknown20C / 30C, UnknownC48, Unknown1C4C / 50 and Unknown1C7C are left alone
void NItemEntry::Copy(NItemEntry* Src)
{
    if (Src->GetIcon())
        Icon = Src->GetIcon();
    if (Src->GetUnknown004())
        Unknown004 = Src->GetUnknown004();
    if (Src->GetUnknown008())
        Unknown008 = Src->GetUnknown008();
    if (Src->GetName())
        appStrcpy(Name, Src->GetName());
    if (Src->GetDescription())
        appStrcpy(Description, Src->GetDescription());
    if (Src->GetToolTipName())
        appStrcpy(ToolTipName, Src->GetToolTipName());
    if (Src->GetToolTipText())
        appStrcpy(ToolTipText, Src->GetToolTipText());
    Type = Src->Type;
    ObjectID = Src->ObjectID;
    ID = Src->ID;
    Count = Src->Count;
    UnknownC20 = Src->UnknownC20;
    UnknownC18 = Src->UnknownC18;
    BodyPart = Src->BodyPart;
    Weight = Src->Weight;
    UnknownC2C = Src->UnknownC2C;
    UnknownC30 = Src->UnknownC30;
    Enchant = Src->Enchant;
    UnknownC3C = Src->UnknownC3C;
    bStackable = Src->bStackable;
    bEquipped = Src->bEquipped;
    UnknownC34 = Src->UnknownC34;
    bSelected = Src->bSelected;
    Unknown1C58 = Src->Unknown1C58;
    CrystalGrade = Src->CrystalGrade;
    Unknown1C60 = Src->Unknown1C60;
    Unknown1C64 = Src->Unknown1C64;
    Unknown1C68 = Src->Unknown1C68;
    Unknown1C6C = Src->Unknown1C6C;
    Unknown1C70 = Src->Unknown1C70;
    SubType = Src->SubType;
    Unknown1C80 = Src->Unknown1C80;
    Unknown1C84 = Src->Unknown1C84;
    Unknown1C88 = Src->Unknown1C88;
    Unknown1C8C = Src->Unknown1C8C;
    Unknown1C90 = Src->Unknown1C90;
    Unknown1C94 = Src->Unknown1C94;
    Unknown1C78 = Src->Unknown1C78;
    Unknown1C98 = Src->Unknown1C98;
    Unknown1C9C = Src->Unknown1C9C;
    Unknown1CA0 = Src->Unknown1CA0;
    Unknown1CA4 = Src->Unknown1CA4;
    Unknown1CA8 = Src->Unknown1CA8;
}
NW_HOOK_METHOD(0x10015fc0, NItemEntry, Copy)

// 0x10016230
void NItemEntry::Select()
{
    bSelected = 1;
}
NW_HOOK_METHOD(0x10016230, NItemEntry, Select)

// 0x10016240
UBOOL NItemEntry::IsSelected()
{
    return bSelected;
}
NW_HOOK_METHOD(0x10016240, NItemEntry, IsSelected)

// 0x10016250
void NItemEntry::Unselect()
{
    bSelected = 0;
}
NW_HOOK_METHOD(0x10016250, NItemEntry, Unselect)

// 0x10016260
UBOOL NItemEntry::IsStackable()
{
    return bStackable;
}
NW_HOOK_METHOD(0x10016260, NItemEntry, IsStackable)

// 0x10016270 - the constructor's defaults, but the tool tips, UnknownC48, bSelected and
// Unknown1CA8 are kept
void NItemEntry::Reset()
{
    Name[0] = 0;
    IconName[0] = 0;
    Unknown20C[0] = 0;
    Unknown30C[0] = 0;
    Description[0] = 0;
    Icon = NULL;
    Unknown004 = 0;
    Unknown008 = 0;
    Type = -1;
    ObjectID = 0;
    ID = 0;
    Count = 0;
    UnknownC20 = 0;
    UnknownC18 = -1;
    BodyPart = 0;
    Weight = 0;
    UnknownC2C = -1;
    UnknownC30 = -1;
    Unknown1C80 = 0;
    Unknown1C84 = 0;
    Unknown1C88 = 0;
    Unknown1C8C = 0;
    Unknown1C90 = 0;
    Unknown1C94 = 0;
    Unknown1C58 = 0;
    CrystalGrade = 0;
    Unknown1C60 = 0;
    Unknown1C64 = 0;
    Unknown1C68 = 0;
    Unknown1C6C = 0;
    Unknown1C70 = 0;
    SubType = -1;
    Unknown1C78 = 0;
    Unknown1C7C = 0;
    Enchant = 0;
    UnknownC3C = 0;
    bStackable = 0;
    bEquipped = 0;
    UnknownC34 = -1;
    Unknown1C98 = 0;
    Unknown1C9C = 0;
    Unknown1CA0 = 0;
    Unknown1CA4 = 0;
}
NW_HOOK_METHOD(0x10016270, NItemEntry, Reset)

// 0x10064290 (among the inventory's code) - Unknown02 is left as it is
NItemRequest::NItemRequest()
{
    Type = -1;
    ObjectID = 0;
    ItemID = 0;
    Count = 0;
    Unknown10 = 0;
    Unknown14 = 0;
    BodyPart = 0;
    Enchant = 0;
    Unknown1C = 0;
    bEquipped = 0;
    Unknown24 = 0;
    Unknown28 = 0;
    Unknown2C = -1;
}
NW_HOOK_CTOR(0x10064290, NItemRequest)

/*-----------------------------------------------------------------------------
    NCItemWnd
-----------------------------------------------------------------------------*/

// 0x100176b0 - 5 x 5 icons of 32 pixels
NCItemWnd::NCItemWnd()
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    Columns = 5;
    VisibleRows = 5;
    TopRow = 0;
    NumRows = 0;
    bDragging = 0;
    bNoDrag = 0;
    MaxItems = 25;
    CellSize = 32;
    Unknown0C8 = 16;
    Selected = -1;
    HoverIndex = -1;
}
NW_HOOK_CTOR(0x100176b0, NCItemWnd)

// 0x100177e0 - NumRows and bNoDrag are left as allocated
NCItemWnd::NCItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize)
{
    BackTex = NULL;
    SelectTex = NULL;
    ToolTipTex = NULL;
    TipTopTex = NULL;
    TipMidTex = NULL;
    TipBottomTex = NULL;
    Columns = InColumns;
    VisibleRows = InVisibleRows;
    MaxItems = InMaxItems;
    TopRow = 0;
    bDragging = 0;
    CellSize = InCellSize;
    Unknown0C8 = 16;
    Selected = -1;
    HoverIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x100177e0, NCItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10017780 - Items' destructor is TArray<NItemEntry*>'s (0x10017620)
NW_HOOK_DTOR(0x10017780, NCItemWnd)

// 0x1006bb90 (vslot 24) - stock body shared with NCNoticeInfoWnd; also the base call of the
// subclasses' WindowProc
INT NCItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCScrollWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1006bb90, NCItemWnd, WindowProc)

// 0x10016960 (vslot 35)
INT NCItemWnd::OnCreate()
{
    NCScrollWnd::OnCreate();
    BackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    SelectTex = FindTexture(TEXT("l2ui.nwindow.item_click"));
    ToolTipTex = FindTexture(TEXT("L2UI.tooltip.mini-tool01"));
    TipTopTex = FindTexture(TEXT("L2UI.Tooltip.Tool02"));
    TipMidTex = FindTexture(TEXT("L2UI.Tooltip.Tool01"));
    TipBottomTex = FindTexture(TEXT("L2UI.Tooltip.Tool03"));
    InitScrollBar();
    return 1;
}
NW_HOOK_METHOD(0x10016960, NCItemWnd, OnCreate)

// 0x10016570 - the range starts as TopRow rows, as in the stock code
void NCItemWnd::InitScrollBar()
{
    SetRepeatDelay(200);
    ScrollBar->SetRange(TopRow, VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x10016570, NCItemWnd, InitScrollBar)

// 0x10016370 (vslot 36)
INT NCItemWnd::OnDestroy()
{
    Clear();
    NCScrollWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10016370, NCItemWnd, OnDestroy)

// 0x100164f0 (vslot 43) - closes the desktop's message boxes this window opened
void NCItemWnd::HideWindow()
{
    if (SecondDialogBox()->GetOwner() == Entry)
        SecondDialogBox()->HideWindow();
    if (MainDialogBox()->GetOwner() == Entry)
        MainDialogBox()->HideWindow();
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x100164f0, NCItemWnd, HideWindow)

// 0x10016390 - the cells count CellSize + 1 pixels here, not the CellSize + 3 of OnPaint;
// the right and bottom edges belong to the last column / row
INT NCItemWnd::HitTest(INT X, INT Y)
{
    X -= ScreenX;
    Y -= ScreenY;
    INT Cell = CellSize + 1;
    if (X < 0 || X > Columns * Cell || Y < 0 || Y > VisibleRows * Cell)
        return -1;
    INT Col = X / Cell;
    if (Col >= Columns)
        Col--;
    INT Row = Y / Cell;
    if (Row >= VisibleRows)
        Row--;
    return (TopRow + Row) * Columns + Col;
}
NW_HOOK_METHOD(0x10016390, NCItemWnd, HitTest)

// 0x10016410
INT NCItemWnd::GetSelected()
{
    if (Selected >= 0 && Selected < Items.Num())
        return Selected;
    return -1;
}
NW_HOOK_METHOD(0x10016410, NCItemWnd, GetSelected)

// 0x10016430
INT NCItemWnd::GetItemCount()
{
    return Items.Num();
}
NW_HOOK_METHOD(0x10016430, NCItemWnd, GetItemCount)

// 0x10016440 (vslot 61) - a row per notch
INT NCItemWnd::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    SWORD Notches = (SWORD)((SWORD)HIWORD(wParam) / 120);
    if (Notches > 0)
    {
        if (TopRow > Notches)
            TopRow -= Notches;
        else
            TopRow = 0;
    }
    else if (NumRows - TopRow + Notches > VisibleRows)
        TopRow -= Notches;
    else if (NumRows >= VisibleRows)
        TopRow = NumRows - VisibleRows;
    else
        TopRow = 0;
    ScrollBar->SetValue(TopRow);
    return 1;
}
NW_HOOK_METHOD(0x10016440, NCItemWnd, OnMouseWheel)

// 0x100165b0 - rows for the items; TopRow comes back when the last row would leave the view
void NCItemWnd::UpdateScrollRange()
{
    NumRows = Items.Num() / Columns;
    if (Items.Num() % Columns)
        NumRows++;
    if (NumRows >= VisibleRows && NumRows - TopRow < VisibleRows)
        TopRow = NumRows - VisibleRows;
    ScrollBar->SetRange(NumRows, VisibleRows);
    ScrollBar->SetValue(TopRow);
}
NW_HOOK_METHOD(0x100165b0, NCItemWnd, UpdateScrollRange)

// 0x10016670 (vslot 64)
INT NCItemWnd::OnScrollBar(INT Value)
{
    TopRow = Value;
    return 1;
}
NW_HOOK_METHOD(0x10016670, NCItemWnd, OnScrollBar)

// 0x10016690 (vslot 62)
INT NCItemWnd::ScrollUp()
{
    if (NumRows > VisibleRows)
    {
        if (--TopRow < 0)
            TopRow = 0;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x10016690, NCItemWnd, ScrollUp)

// 0x100166d0 (vslot 63)
INT NCItemWnd::ScrollDown()
{
    if (NumRows > VisibleRows)
    {
        TopRow++;
        if (TopRow > NumRows - VisibleRows)
            TopRow = NumRows - VisibleRows;
        ScrollBar->SetValue(TopRow);
    }
    return 1;
}
NW_HOOK_METHOD(0x100166d0, NCItemWnd, ScrollDown)

// 0x10016720 (vslot 67) - `ret 0x10` shared with other empty four-argument methods
void NCItemWnd::DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y)
{
}
NW_HOOK_METHOD(0x10016720, NCItemWnd, DrawItemToolTip)

// 0x100169f0 (vslot 50) - the clip opens a pixel around the window before BeginPaint and
// closes after EndPaint; the rows stop at VisibleRows * (CellSize + 1) pixels
INT NCItemWnd::OnPaint(NCGDevice* Device)
{
    guard(NCItemWnd::OnPaint);
    Device->SetClip(ScreenX - 1, ScreenY - 1, Width + 2, Height + 2);
    Device->BeginPaint(ScreenX, ScreenY);
    INT X = 0;
    INT Y = 0;
    for (INT i = TopRow * Columns; i < Items.Num(); i++)
    {
        NItemEntry* Item = Items(i);
        if (!Item)
            continue;
        if (i % Columns == 0)
        {
            X = 0;
            if (i != TopRow * Columns)
                Y += CellSize + 3;
            if (VisibleRows * (CellSize + 1) <= Y)
                break;
        }
        else
            X += CellSize + 3;
        if (Item->IsSelected())
            Device->DrawTexture(X - 1, Y - 1, 34, 34, 0, 0, 34, 34, SelectTex);
        if (Item->GetIcon())
            Device->DrawTexture(X, Y, CellSize, CellSize, 0, 0, CellSize, CellSize, Item->GetIcon());
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100169f0, NCItemWnd, OnPaint)

// 0x10016ba0 (vslot 51) - over the selected item a 184-pixel box (a strip per text line,
// the text by DrawItemToolTip) above the item, below it when there is no room; over another
// item its name on the small tip. Both are pushed back inside the screen, the box to its
// edge, the name tip by the window's own X.
INT NCItemWnd::OnDrawToolTip(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (HoverIndex >= 0 && HoverIndex < Items.Num())
    {
        INT Index = HoverIndex - TopRow * Columns;
        INT Y = (CellSize + 3) * (Index / Columns);
        INT X = (CellSize + 3) * (Index % Columns);
        NItemEntry* Item = Items(HoverIndex);
        if (Item->IsSelected())
        {
            SIZE Size;
            SIZE Char;
            NCGDevice::GetTextExtent(&Size, Item->GetToolTipText(), 168, 0, 0, 0, 0);
            NCGDevice::GetCharSize(&Char, 'A');
            INT TextH = Size.cy + Size.cy / Char.cy - 1;
            INT ScreenW = GConsole->Desktop->LocalW;
            if (ScreenX + X + 184 > ScreenW)
                X = ScreenW - ScreenX - 184;
            else if (ScreenX + X < 0)
                X = -ScreenX;
            Y += -15 - TextH;
            if (ScreenY + Y < 0)
                Y += CellSize + TextH + 16;
            Device->SetClip(ScreenX + X, ScreenY + Y, 184, TextH / Char.cy * 13 + 16);
            if (TipTopTex)
                Device->DrawTexture(X, Y, 184, 8, 0, 0, 184, 8, TipTopTex);
            INT Lines = TextH / Char.cy;
            for (INT i = 0; i < Lines; i++)
                if (TipMidTex)
                    Device->DrawTexture(X, i * Char.cy + Y + 8, 184, Char.cy, 0, 0, 184, Char.cy,
                                        TipMidTex);
            if (TipBottomTex)
                Device->DrawTexture(X, Lines * Char.cy + Y + 8, 184, 8, 0, 0, 184, 8, TipBottomTex);
            DrawItemToolTip(Device, Items(HoverIndex), X + 8, Y + 8);
            Device->ReleaseClip();
        }
        else if (Item->GetToolTipName())
#if NW_FEATURES
        // phase 6: ItemHoverTip: the recent chronicles' name tip (a dark box, the grade symbol);
        // with [ItemTip] HoverStyle=0 the stock strip below
        if (!nwf::ItemHoverTip_Draw(this, Device, Item, X, Y, CellSize))
#endif
        {
            SIZE Size;
            NCGDevice::GetTextExtent(&Size, Items(HoverIndex)->GetToolTipName(), 0, 0, 0, 0, 0);
            INT ScreenW = GConsole->Desktop->LocalW;
            if (Size.cx + ScreenX + X > ScreenW)
                X = ScreenW - ScreenX - Size.cx - 3;
            else if (ScreenX + X < 0)
                X -= ScreenX;
            Y -= Size.cy;
            if (ScreenY + Y < 0)
                Y += CellSize + Size.cy + 1;
            Device->SetClip(ScreenX + X, ScreenY + Y, Size.cx, Size.cy);
            if (ToolTipTex)
                Device->DrawStretchedTex(X, Y, Size.cx, Size.cy, ToolTipTex);
            Device->DrawText(X, Y, 0xFFFFFFFF, Items(HoverIndex)->GetToolTipName(), 0, 0, 0,
                             L2FONT_Normal, 0, 0, 0, 0, 0, 0);
            Device->ReleaseClip();
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10016ba0, NCItemWnd, OnDrawToolTip)

// 0x10016ef0 (vslot 56) - selects the item under the mouse and picks it up (unless bNoDrag,
// which lasts one click); the desktop's message box closes, and keeps the focus if still up.
INT NCItemWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    INT Index = HitTest(LOWORD(lParam), HIWORD(lParam));
    Selected = Index;
    HoverIndex = Index;
    if (Index >= 0 && Index < Items.Num())
    {
        UnselectAll();
        Items(Selected)->Select();
        if (!bNoDrag)
        {
            if (MainDialogBox()->IsShowWindow())
            {
                MainDialogBox()->Reset();
                MainDialogBox()->HideWindow();
            }
            bDragging = 1;
            GConsole->Engine->OnInterfacePlaySound(SOUND_PickUp);
        }
        else
            bNoDrag = 0;
        if (MainDialogBox()->IsShowWindow())
            MainDialogBox()->SetFocus();
    }
    return 1;
}
NW_HOOK_METHOD(0x10016ef0, NCItemWnd, OnLButtonDown)

// 0x10017010 (vslot 55) - while an item is carried the cursor is its icon (type 4: a color
// cursor built from the texture); otherwise the arrow comes back, unless Ctrl is held
INT NCItemWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    HoverIndex = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (IsFocused())
    {
        if (bDragging)
        {
            if (Selected >= 0 && Selected < Items.Num())
            {
                SetCursor(4, Items(Selected)->GetIcon());
                LockCursor();
                return 1;
            }
        }
        else if (!GConsole->IsCtrlDown())
        {
            UnlockCursor();
            SetCursor(0, NULL);
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10017010, NCItemWnd, OnMouseMove)

// 0x100170b0 (vslot 66)
void NCItemWnd::UnselectAll()
{
    for (INT i = 0; i < Items.Num(); i++)
        Items(i)->Unselect();
}
NW_HOOK_METHOD(0x100170b0, NCItemWnd, UnselectAll)

// 0x10017350 (vslot 65)
void NCItemWnd::Clear()
{
    for (INT i = 0; i < Items.Num(); i++)
        if (Items(i))
            delete Items(i);
    Items.Empty();
    TopRow = 0;
    NumRows = 0;
    Selected = -1;
    HoverIndex = -1;
}
NW_HOOK_METHOD(0x10017350, NCItemWnd, Clear)

NW_CHECK_VTABLE(NCItemWnd, 0x100dcaa8, 68)

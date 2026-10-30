/*=============================================================================
    ItemHoverTip.cpp - phase 6: the name tip of an item under the mouse, in the recent
    chronicles' style (include/features/ItemHoverTip.h).

    The stock strip is the name over the translucent L2UI.tooltip.mini-tool01; the grade symbol is
    only in the box of the selected item, where DrawText takes it as an inline glyph: the text
    `graded` .. `grades` (NConsoleWnd::GetGradeText) with its ninth argument 1 (ItemTip::DrawLines).
    Here the tip is a box of two rectangles (a thin border, a dark fill) with the name, and the
    same inline glyph after it for a graded item. The glyph is about 11 pixels wide and its own
    frame is part of it. Items without a grade get no glyph (`none` draws nothing anyway).
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCItemWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NGameData.h"
#include "nwindow/NCWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "features/ItemHoverTip.h"

namespace nwf {

namespace {

// pixels, as the box measures in the reference screenshots: 1 border + 4 / 3 of air, and the
// glyph 3 pixels after the name, 11 wide with its shadow
const INT PadX = 4;
const INT PadY = 3;
const INT GradeGap = 3;
const INT GradeW = 12;

const DWORD ColorBorder = 0xFF3A3A38;
const DWORD ColorFill = 0xFF141414;
const DWORD ColorName = 0xFFFFFFFF;

// the text up to its first line break (a private store's tip is "name\nprice"), and its width
INT FirstLineWidth(const TCHAR* Text)
{
    TCHAR Line[256];
    INT Length = 0;
    while (Text[Length] && Text[Length] != '\n' && Text[Length] != '\r' && Length < 255)
    {
        Line[Length] = Text[Length];
        Length++;
    }
    Line[Length] = 0;
    SIZE Size;
    Size.cx = 0;
    Size.cy = 0;
    NCGDevice::GetTextExtent(&Size, Line, 0, 0, 0, 0, 0);
    return Size.cx;
}

// The tip: Name (first line followed by the grade's glyph, when Grade is 1..5) in the box above
// the cell at (X, Y) of Wnd, CellSize wide; below it when there is no room, inside the screen.
void DrawTip(NCWnd* Wnd, NCGDevice* Device, const TCHAR* Name, INT Grade, INT X, INT Y, INT CellSize,
             DWORD MaxWidth)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Name, MaxWidth, 0, 0, 0, 0);
    const bool bGrade = Grade >= 1 && Grade <= 5 && GConsole->GetGradeText(Grade) != NULL;
    const INT FirstW = bGrade ? FirstLineWidth(Name) : 0;
    INT ContentW = Size.cx;
    if (bGrade && FirstW + GradeGap + GradeW > ContentW)
        ContentW = FirstW + GradeGap + GradeW;
    const INT BoxW = ContentW + 2 * PadX + 2;
    const INT BoxH = Size.cy + 2 * PadY + 2;

    // above the cell, its border one pixel left of it; pushed inside the screen, and below the
    // cell when the window is at the top (as the stock strip)
    const INT ScreenW = GConsole->Desktop->LocalW;
    X -= 1;
    if (Wnd->ScreenX + X + BoxW > ScreenW)
        X = ScreenW - Wnd->ScreenX - BoxW;
    else if (Wnd->ScreenX + X < 0)
        X = -Wnd->ScreenX;
    Y -= BoxH;
    if (Wnd->ScreenY + Y < 0)
        Y += CellSize + BoxH + 1;

    Device->SetClip(Wnd->ScreenX + X, Wnd->ScreenY + Y, BoxW, BoxH);
    Device->DrawRect(X, Y, BoxW, BoxH, ColorBorder);
    Device->DrawRect(X + 1, Y + 1, BoxW - 2, BoxH - 2, ColorFill);
    Device->DrawText(X + 1 + PadX, Y + 1 + PadY, ColorName, Name, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (bGrade)
    {
        TCHAR Glyph[32];
        appSprintf(Glyph, TEXT("`%s`"), GConsole->GetGradeText(Grade));
        Device->DrawText(X + 1 + PadX + FirstW + GradeGap, Y + 1 + PadY, ColorName, Glyph, 0, 0, 0,
                         L2FONT_Normal, 1, 0, 0, 0, 0, 0);
    }
    Device->ReleaseClip();
}

// [ItemTip] HoverStyle from the registry, looked at once a second: the tip is drawn every frame
// the mouse rests on an item, and a registry read each time is not worth it
bool HoverStyleOn()
{
    static INT GStyle = 1;
    static DWORD GAsked = 0;
    const DWORD Now = GetTickCount();
    if (!GAsked || Now - GAsked >= 1000)
    {
        GStyle = IniInt("ItemTip", "HoverStyle", 1);
        GAsked = Now ? Now : 1;
    }
    return GStyle != 0;
}

}  // namespace

bool ItemHoverTip_Draw(NCWnd* Wnd, NCGDevice* Device, NItemEntry* Item, INT X, INT Y, INT CellSize,
                       DWORD MaxWidth)
{
    if (!HoverStyleOn())
        return false;
    const TCHAR* Name = Item->GetToolTipName();
    if (Name)   // no name: nothing to show, as the stock code
        DrawTip(Wnd, Device, Name, Item->CrystalGrade, X, Y, CellSize, MaxWidth);
    return true;
}

bool ItemHoverTip_DrawShortcut(NCWnd* Wnd, NCGDevice* Device, const TCHAR* Name, INT Type, INT ID, INT X,
                               INT Y, INT CellSize, DWORD MaxWidth)
{
    if (!HoverStyleOn())
        return false;
    // the slot keeps no grade: an item's comes from the item table, as the inventory takes it
    // (weapons and armor keep it apart; etc items have none); skills and actions have none
    INT Grade = 0;
    if (Type == SHORTCUT_Item)
    {
        NViewItemData* Data = NItemTable::Find(&ID);
        if (Data && Data->Class == ITEMCLASS_Weapon)
            Grade = Data->CrystalGrade;
        else if (Data && Data->Class == ITEMCLASS_Armor)
            Grade = Data->ArmorCrystalGrade;
    }
    if (Name)
        DrawTip(Wnd, Device, Name, Grade, X, Y, CellSize, MaxWidth);
    return true;
}

}  // namespace nwf

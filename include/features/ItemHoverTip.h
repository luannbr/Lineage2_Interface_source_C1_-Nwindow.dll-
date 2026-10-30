/*=============================================================================
    ItemHoverTip.h - phase 6: the tool tip of an item under the mouse, as the recent chronicles
    draw it, asked by the owner on 29/09/2026 (screenshots of Chronicle 3 onward: the name in a
    dark box with a thin border and the item's grade symbol after it). C1's grids show only the
    name on a translucent strip (L2UI.tooltip.mini-tool01) and the grade only in the box of the
    selected item; not in NWindow_C1. Notes: docs/notas/F6_ItemHoverTip.md.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NCGDevice;
class NCWnd;
struct NItemEntry;

namespace nwf {

// From NCItemWnd::OnDrawToolTip and NCEquipItemWnd::OnDrawToolTip, for an item under the mouse
// that is not the selected one (the selected item keeps its box): draws the name tip of Item,
// above its cell (X, Y: the cell's top left in the window Wnd, CellSize wide), below it when
// there is no room, kept inside the screen. MaxWidth: the wrap width the window measures the
// name with (0 none). False with nothing drawn when the setting is off ([ItemTip] HoverStyle=0),
// and the caller draws the stock strip.
bool ItemHoverTip_Draw(NCWnd* Wnd, NCGDevice* Device, NItemEntry* Item, INT X, INT Y, INT CellSize,
                       DWORD MaxWidth = 0);

// From NCShortCutWnd::OnDrawToolTip, for the slot under the mouse: the same tip for the slot's
// DisplayName (an item's with the enchant or the count, a skill's with its MP, an action's), above
// the slot at (X, Y) of Wnd, CellSize wide. Type / ID: the slot's (NShortCutItem::Type, ::ID); an
// item's grade symbol comes from the item table. False with nothing drawn when the setting is
// off, and the caller draws the stock strip.
bool ItemHoverTip_DrawShortcut(NCWnd* Wnd, NCGDevice* Device, const TCHAR* Name, INT Type, INT ID, INT X,
                               INT Y, INT CellSize, DWORD MaxWidth = 0);

}  // namespace nwf

#endif  // NW_FEATURES

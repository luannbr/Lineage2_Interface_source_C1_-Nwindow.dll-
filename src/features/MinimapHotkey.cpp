/*=============================================================================
    MinimapHotkey.cpp - phase 6: Alt+M / M toggle the map window
    (include/features/MinimapHotkey.h).

    NWindow_C1 src/inject/MinimapHotkey.cpp retargeted the one call of the stock hotkey handler
    (0x10055abc in DispatchWndMsgX) to a wrapper: Alt+M before the handler (whose WM_SYSKEYDOWN
    branch takes every key), M after it, only when it declined and the letters were shortcuts.
    Here both are cases of NConsoleWnd::HookShortCutKey's own switches, at the same points.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCMinimapWnd.h"
#include "features/MinimapHotkey.h"

namespace nwf {

// LineageWindow.ini [Hotkeys], read once
static INT GMinimapKey = -1;        // Minimap: the feature
static INT GMinimapBareKey = 1;     // MinimapBareKey: M alone
static INT GMinimapGmPetition = 0;  // MinimapGmPetition: Alt+M keeps the GM petition for GMs

static void LoadConfig()
{
    if (GMinimapKey >= 0)
        return;
    GMinimapKey = IniInt("Hotkeys", "Minimap", 1) != 0;
    GMinimapBareKey = IniInt("Hotkeys", "MinimapBareKey", 1) != 0;
    GMinimapGmPetition = IniInt("Hotkeys", "MinimapGmPetition", 0) != 0;
}

// a key press, not the keyboard's repeat of a held key
static UBOOL IsFirstPress(LPARAM lParam)
{
    return !(lParam & 0x40000000);
}

// shown -> hidden, hidden -> shown, as a map item does
static void ToggleMinimap(NConsoleWnd* Console)
{
    NCConsole* Desktop = Console->GetDesktop();
    NCMinimapWnd* Map = Desktop ? Desktop->MinimapWnd : NULL;
    if (!Map)
        return;
    if (Map->MapID != MINIMAP_Aden && Map->MapID != MINIMAP_Elmore)
    {
        // never shown yet (map id 0): NCMinimapWnd::OnPaint would divide by an uninitialised
        // value. ShowMapItem sets the id, centres on the player and toggles, as the stock path.
        Console->ShowMapItem(MINIMAP_Aden);
        return;
    }
    Desktop->ToggleMinimap();  // with the stock sound
}

UBOOL MinimapHotkey_AltM(NConsoleWnd* Console, LPARAM lParam)
{
    LoadConfig();
    if (!GMinimapKey)
        return 0;
    // the stock Alt+M: the GM petition window, for GM levels 1 .. 6 (players: nothing)
    User* Me = GMinimapGmPetition ? Console->GetMyUser() : NULL;
    if (Me && Me->BuilderLevel > 0 && Me->BuilderLevel < 7)
        return 0;
    if (IsFirstPress(lParam))
        ToggleMinimap(Console);
    return 1;
}

UBOOL MinimapHotkey_BareM(NConsoleWnd* Console, LPARAM lParam)
{
    LoadConfig();
    if (!GMinimapKey || !GMinimapBareKey)
        return 0;
    if (IsFirstPress(lParam))
        ToggleMinimap(Console);
    return 1;
}

}  // namespace nwf

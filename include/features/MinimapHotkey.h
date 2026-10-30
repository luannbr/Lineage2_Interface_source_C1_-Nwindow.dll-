/*=============================================================================
    MinimapHotkey.h - phase 6: Alt+M, and M alone when the letters are shortcuts, toggle the
    map window (NCConsole::MinimapWnd), as in C3. Stock C1 has no key for it: a map item on the
    shortcut bar or the server open it. NWindow_C1 src/inject/MinimapHotkey.cpp, its doc
    MINIMAP_HOTKEY.md; notes: docs/notas/F6_MinimapHotkey.md.

    LineageWindow.ini [Hotkeys]: Minimap (1) the whole feature, MinimapBareKey (1) M alone,
    MinimapGmPetition (0) Alt+M stays the stock GM petition window for GMs.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NConsoleWnd;

namespace nwf {

// NConsoleWnd::HookShortCutKey, Alt+M (WM_SYSKEYDOWN): 1 when the map took it (toggled but for
// the key's repeat); 0 leaves the stock GM petition.
UBOOL MinimapHotkey_AltM(NConsoleWnd* Console, LPARAM lParam);

// NConsoleWnd::HookShortCutKey, M among the letter shortcuts (the chat needs Enter and no
// control takes the keys, as the stock letters): 1 when the map took it.
UBOOL MinimapHotkey_BareM(NConsoleWnd* Console, LPARAM lParam);

}  // namespace nwf

#endif  // NW_FEATURES

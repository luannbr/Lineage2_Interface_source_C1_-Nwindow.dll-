/*=============================================================================
    CombatLog.h - phase 6: the combat log (NWindow_C1 src/inject/combatlog/CombatLog.cpp,
    docs COMBAT_LOG.md and SESSION_2026-07-16_COMBATLOG_UI.md).

    A second list above the stock chat, fed with the system and combat lines the chat no
    longer gets: exp / SP, drops, damage dealt and received, kills and levels, skills. A
    gear button (L2UI.ChatWnd.OptionBtn) or Alt+L opens a filter under it, one line per
    category, kept in combatlog.ini [filter] next to NWindow.dll. The list is dragged with
    the left button, hides with the rest of the interface (Alt+H) and is emptied when the
    character leaves or enters the world.

    NWindow_C1 hooked the stock NCChatWnd::AddChatMessage (0x10037b30) and ran from its own
    port of NConsoleWnd::Tick; here the two stock functions call it (#if NW_FEATURES blocks).
    Implementation and the differences from NWindow_C1: src/features/CombatLog.cpp,
    docs/notas/F6_CombatLog.md.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NConsoleWnd;

namespace nwf {

// NConsoleWnd::Tick, before the engine's tick (NWindow_C1: WindowBridge_OnConsole, the first
// thing its Tick port did): follows the console state, creates the list 60 ticks into the
// world, lays it out and handles the mouse and Alt+L.
void CombatLog_TrySpawn(NConsoleWnd* Console);

// NCChatWnd::AddChatMessage, before anything else: 1 = a system line (type 5, but the petition
// chat, sub type 6), taken by the combat log (shown there when its filter lets it through), the
// chat must not add it. A line of a chat type is never taken, whatever its text. Nothing is
// taken before the list was created for the first time.
UBOOL CombatLog_DivertChatMessage(const TCHAR* Text, DWORD Color, INT Type, INT SubType);

// After the periodic FLUSH / OBJ GARBAGE (CacheCleaner): the windows back into the root set,
// the background texture found again and the list shown, or a collected list dropped so that
// the next CombatLog_TrySpawn makes a new one.
void CombatLog_OnAfterCacheClean();

}  // namespace nwf

#endif  // NW_FEATURES

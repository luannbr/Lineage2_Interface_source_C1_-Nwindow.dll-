/*=============================================================================
    CacheCleaner.h - phase 6: every 5 minutes of play in the world, the console commands FLUSH
    and OBJ GARBAGE (Interlude's UIDATA_PAWNVIEWER ExecuteCommand pair), to keep the client's
    memory down in long sessions. NWindow_C1 src/inject/CacheCleaner.cpp (no ini: the interval
    and the FLUSH are constants there too); notes: docs/notas/F6_CacheCleaner.md.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NConsoleWnd;

namespace nwf {

// NConsoleWnd::Tick: counts the time in the world (only there: no hitch at the login or the
// character selection) and cleans when the interval is over.
void CacheCleaner_OnTick(NConsoleWnd* Console, FLOAT DeltaTime);

}  // namespace nwf

#endif  // NW_FEATURES

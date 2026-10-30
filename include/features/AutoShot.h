/*=============================================================================
    AutoShot.h - phase 6: automatic soulshot / spiritshot on the shortcut bar, as in C3. A
    right click on a shot toggles its automatic use on the server ("ss_set <item> <0|1>"); the
    server keeps the state and pushes it back ("L2W shot 1 state" data messages), and the bar
    draws the toggle skills' animated mark over each shot that is on. The same bar also moves a
    shortcut dropped on another slot (C3; stock C1 ignores that drop). NWindow_C1
    src/inject/AutoShot.cpp (plus the "ss_ui 1" announce and the data channel of its
    craft/CraftUi.cpp), its doc AUTO_SHOT.md; notes: docs/notas/F6_AutoShot.md.

    LineageWindow.ini [Shots] AutoShot (1).
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NConsoleWnd;
class NCShortCutWnd;
class NCGDevice;

namespace nwf {

// [Shots] AutoShot, read once
UBOOL AutoShot_Enabled();

// NConsoleWnd::Tick: "ss_ui 1" once the player is in the world (again after every return to
// the character selection or the login: the server forgets the automatic shots), and the
// server's message of the last state into the chat.
void AutoShot_OnTick(NConsoleWnd* Console);

// NConsoleWnd::NpcHtmlMessage: 1 when the page is one of our data messages ("L2W " but the
// craft windows' "L2W mfg"): the shots' state, taken here and never shown.
UBOOL AutoShot_OnNpcHtml(const TCHAR* Html);

// NCShortCutWnd::OnPaint, after the stock bar: the mark over each shot that is on
void AutoShot_OnPaint(NCShortCutWnd* Wnd, NCGDevice* Device);

// NCShortCutWnd::OnRButtonDown (the stock bar leaves the right button to NCWnd's no-op): a shot
// asks the server for the other state; 1 taken (no mouse look), 0 anything else.
UBOOL AutoShot_OnRButtonDown(NCShortCutWnd* Wnd, WPARAM wParam, LPARAM lParam);

// NCShortCutWnd::OnLButtonUp, after the stock code: a shortcut dragged onto another slot moves
// there (swapping with what is in it).
void AutoShot_OnLButtonUp(NCShortCutWnd* Wnd, LPARAM lParam);

}  // namespace nwf

#endif  // NW_FEATURES

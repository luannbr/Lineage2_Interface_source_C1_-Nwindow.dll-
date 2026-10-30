/*=============================================================================
    CraftUi.h - phase 6: the native C2-style craft windows (NWindow_C1
    src/inject/craft/CraftUi.cpp): settings, transport, the windows' tick.
    Plan and protocol: docs/notas/F6_Craft.md (NWindow_C1 docs/CRAFT_UI_C2.md,
    docs/RECIPE_UI_C2_SPEC.md).

    LineageWindow.ini [Craft]: NativeUI=1 turns the windows on (off by default: the
    server keeps sending the HTML shop); TextureTest=1 loads the 14 textures of
    SysTextures/c1recipe.utx once and logs them ("craftui:" lines, [Window] Log=1).
    [CraftUi] RecipeX/RecipeY, RecipeTreeX/RecipeTreeY: where the windows were closed.

    Transport, the same as NWindow_C1's with the callbacks that are ours now:
      * entering the world (the local User the Engine makes with the first UserInfo:
        NConsoleWnd::GetMyUser from the tick) sends the bypass "mfg_ui 1"; the server
        then answers the player's C1Manufacture views with data messages, not HTML;
      * a data message is an NpcHtmlMessage whose text starts with "L2W mfg"
        (NConsoleWnd::NpcHtmlMessage hands it here): consumed, never a page;
      * the windows' actions go back as the mfg_* bypasses (CraftSession.h).
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

class NConsoleWnd;

// the console tick (NConsoleWnd::Tick): the [Craft] settings the first time; then, with
// NativeUI, the announce on entering the world, the data received opened in the windows, the
// windows closed outside the world, Esc; the texture test
void CraftUi_OnTick(NConsoleWnd* Console);

// NConsoleWnd::NpcHtmlMessage: 1 when the text was a craft data message (consumed, no page)
UBOOL CraftUi_OnNpcHtml(const TCHAR* Html);

// the manufacture windows' tree button: the tree closes if open, else is asked for
void CraftUi_ToggleTree(INT RecipeID);

#endif  // NW_FEATURES

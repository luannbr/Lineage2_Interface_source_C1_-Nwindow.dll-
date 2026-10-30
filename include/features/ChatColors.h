/*=============================================================================
    ChatColors.h - phase 6: the colors of the chat types C1's NConsoleWnd::Say2 does not know,
    asked by the owner on 29/09/2026 (the VIP chat is sent as a chat type, and the type is what
    colors it: Lucera's WorldChatMsgColor = ALL, SHOUT, TELL, PARTY, CLAN, TRADE, ALLIANCE,
    COMMANDCHANNEL_ALL, COMMANDCHANNEL_COMMANDER or HERO_VOICE). C1 colors the first seven and
    draws every other type white; the values here are Interlude's own (NWindow.dll of the Lucera
    Interlude client, NConsoleWnd::Say2). Not in NWindow_C1. Notes: docs/notas/F6_ChatColors.md.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

namespace nwf {

// From NConsoleWnd::Say2's last branch: the color of Type (the protocol's chat type) when it is
// one of the extra ones, true; false for the rest (white, as stock).
bool ChatColors_Find(INT Type, DWORD* Color);

}  // namespace nwf

#endif  // NW_FEATURES

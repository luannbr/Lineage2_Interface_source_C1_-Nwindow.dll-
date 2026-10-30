/*=============================================================================
    ChatColors.cpp - phase 6: the Interlude colors of the chat types C1 draws white
    (include/features/ChatColors.h).

    NConsoleWnd::Say2 of the Lucera Interlude client (NWindow.dll, build of 2007-06-19, 0x10163d90)
    colors, besides the types C1 has (shout FF7200, party 00FF00, clan 7D77FF, tell FF00FF, trade
    EAA5F5, alliance 77FF99), these:
        15 COMMANDCHANNEL_ALL        FF9695
        16 COMMANDCHANNEL_COMMANDER  FFF8B2
        17 HERO_VOICE                408CFF
    (the C4 client of 2006 has the hero one, the C3 of 2005 none). The line goes to the chat as a
    normal one, as C1 draws these types today: the All tab, the Normal filter, no other side effect
    (the tell type is left out on purpose: it adds the speaker to the tell list).
=============================================================================*/
#include "features/ChatColors.h"

namespace nwf {

bool ChatColors_Find(INT Type, DWORD* Color)
{
    switch (Type)
    {
    case 15:   // COMMANDCHANNEL_ALL
        *Color = 0xFFFF9695;
        return true;
    case 16:   // COMMANDCHANNEL_COMMANDER
        *Color = 0xFFFFF8B2;
        return true;
    case 17:   // HERO_VOICE
        *Color = 0xFF408CFF;
        return true;
    }
    return false;
}

}  // namespace nwf

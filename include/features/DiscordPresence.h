/*=============================================================================
    DiscordPresence.h - phase 6: Discord Rich Presence of the server ("Playing L2GIRAN.COM",
    the details line, the elapsed time and two link buttons), asked by the owner on 29/09/2026
    in place of luannbr/L2DiscordPresence (a separate DLL loaded by an ogg.dll stub, configured
    by presence.ini at build time).

    Here the same Discord IPC (the desktop client's local pipe \\.\pipe\discord-ipc-N: handshake,
    then SET_ACTIVITY, then answering its pings) is our own code in NWindow.dll, and what
    presence.ini carried is compiled in (src/features/DiscordPresence.cpp). No file, no Game SDK.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

namespace nwf {

// From our DllMain: starts the presence thread when the host process is the game (L2.exe), so the
// smoke and diff tools never touch the developer's Discord. It connects when Discord is running,
// and again whenever Discord restarts. Idempotent; never blocks the load.
void DiscordPresence_Start();

}  // namespace nwf

#endif  // NW_FEATURES

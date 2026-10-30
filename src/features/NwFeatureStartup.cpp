// Phase 6: the start and stop of the LineageWindow features (include/features/NwFeatures.h).
//
// NWindow_C1's LineageWindow.dll was an import of the stock NWindow.dll, so its DllMain ran in
// the LoadLibrary of NWindow.dll (Core's UPackage::Bind, "Bound to NWindow" in L2.log): after
// WinDrv.dll and Network.dll were mapped, before the lobby map and the viewport window. Our
// DllMain runs in that same LoadLibrary (src/standalone/NWindowPackage.cpp), after the static
// initializers (the 178 UClass), so the features start here, in NWindow_C1's order
// (src/inject/NWindow_inject.cpp):
//   CraftBalloon, AutoShot, MinimapHotkey, ReturnToLogin, ManufactureCmd, BuffTimer,
//   CacheCleaner, WindowFix, CraftUi
// (the ones of other phase 6 workers go in their place in that list). Its deferred thread only
// repeated these installs a moment later; ReturnToLogin's retry is the Tick's, as it was there.
// Under the loader lock, as NWindow_C1's DllMain: module handles, ini reads, page protection,
// CreateThread (the thread starts once the load ends).
#include "features/NwFeatures.h"
#include "features/DiscordPresence.h"
#include "features/ReturnToLogin.h"
#include "features/WindowFix.h"

namespace nwf {

void OnDllMain(DWORD Reason)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        // Unreachable game server / disconnect popup -> back to the login screen (Network.dll is
        // mapped with WinDrv, before NWindow; the Tick retries otherwise)
        Log("returntologin", ReturnToLogin_Install() ? "install @attach OK" : "install @attach FAIL (will retry)");
        // Windowed mode: movable frame, UI mouse in client space, click filter. WinDrv is already
        // loaded (bound long before NWindow) and its viewport window is only opened later, after
        // the lobby map: the hooks must be in before that.
        WindowFix_Install();
        // Discord Rich Presence of the server (own thread, only in L2.exe; not in NWindow_C1)
        DiscordPresence_Start();
    }
}

}  // namespace nwf

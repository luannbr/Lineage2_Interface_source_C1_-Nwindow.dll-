// Phase 6: the features' per-frame work, the first thing NConsoleWnd::Tick does (before the stock
// Tick), in NWindow_C1's Tick order: combat log, craft UI, AutoShot, ReturnToLogin, the bypass
// queue, the cache cleaner. "mfg_ui 1" (craft) must go out before "ss_ui 1" (AutoShot).
#include "nwindow/NConsoleWnd.h"
#include "features/NwFeatures.h"
#include "features/AutoShot.h"
#include "features/CacheCleaner.h"
#include "features/CombatLog.h"
#include "features/CraftBalloon.h"
#include "features/craft/CraftUi.h"
#include "features/ManufactureCmd.h"
#include "features/ReturnToLogin.h"

namespace nwf {

void OnConsoleTick(NConsoleWnd* Console, float DeltaTime)
{
    // under one handler, as NWindow_C1's Tick port: a fault in a feature loses that frame's
    // feature work and the stock Tick still runs
    try
    {
        // CombatLog (NWindow_C1 src/inject/combatlog/CombatLog.cpp): the panel, once in the world
        CombatLog_TrySpawn(Console);
        // CraftBalloon (NWindow_C1 src/inject/CraftBalloon.cpp): the Engine.dll patch, once
        ::CraftBalloon_OnTick();
        // CraftUi (NWindow_C1 src/inject/craft/CraftUi.cpp): "mfg_ui 1" on entering the world, the
        // server's data in the craft windows
        ::CraftUi_OnTick(Console);
        // AutoShot (NWindow_C1 src/inject/AutoShot.cpp): "ss_ui 1" on entering the world, the
        // server's message
        AutoShot_OnTick(Console);
        // ReturnToLogin (NWindow_C1 src/inject/ReturnToLogin.cpp): the unreachable game server and
        // the disconnect box go back to the login screen
        ReturnToLogin_OnTick(Console);
        // ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp): the next queued bypass
        BypassFlush();
        // CacheCleaner (NWindow_C1 src/inject/CacheCleaner.cpp): FLUSH + OBJ GARBAGE
        CacheCleaner_OnTick(Console, DeltaTime);
    }
    catch (...)
    {
    }
}

}  // namespace nwf

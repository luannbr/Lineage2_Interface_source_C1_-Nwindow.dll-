/*=============================================================================
    CacheCleaner.cpp - phase 6: FLUSH + OBJ GARBAGE every 5 minutes in the world
    (include/features/CacheCleaner.h).

    NWindow_C1 src/inject/CacheCleaner.cpp, from its port of NConsoleWnd::Tick, with the Core
    exports taken by GetProcAddress; here they are imports and the tick is ours. Same tunables
    (constants, no ini), same order of attempts: the global command chain (GExec), else
    UObject::StaticExec, else CollectGarbage keeping the native objects; an exception inside is
    swallowed, as NWindow_C1's __try did.

    Two settings (registry, HKCU\Software\Lineage2C1\NWindow\CacheCleaner, read at each clean):
    Enable=0 turns the whole clean off, Flush=0 keeps only the garbage collection. Both default to
    1 (the NWindow_C1 behavior). The stock client never did either: FLUSH is UEngine::Flush(1), which
    drops the render device's textures and buffers and lets it precache them again, in a 32-bit
    process that is not large address aware (2 GB), so a clean can be what tips a long session over
    (docs/notas/F6_CacheCleaner.md, "Suspeita"). With NW_FEATURE_LOG the memory around each clean is
    logged.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "features/CacheCleaner.h"

#include <psapi.h>

// The combat log heals its panel after a clean: GC and FLUSH can take its textures and paint
// state (NWindow_C1: CombatLog_OnAfterCacheClean).
#include "features/CombatLog.h"

namespace nwf {

static const FLOAT CacheCleanInterval = 300.f;  // seconds in the world (600: 10 minutes)
static const UBOOL bCacheCleanFlush = 1;        // also FLUSH (the render caches): a longer hitch

static FLOAT GCacheCleanTime = 0.f;  // in the world since the last clean

// 1 run, 0 no output device, -1 an exception
static INT ExecCommand(const TCHAR* Cmd)
{
    FOutputDevice* Ar = GNull ? GNull : GLog;  // quiet first
    if (!Ar)
        return 0;
    if (GExec)
    {
        try
        {
            GExec->Exec(Cmd, *Ar);
            return 1;
        }
        catch (...)
        {
        }
    }
    try
    {
        UObject::StaticExec(Cmd, *Ar);
        return 1;
    }
    catch (...)
    {
        return -1;
    }
}

// what the 2 GB of a 32-bit process is doing: the address space left is what a texture upload or
// a package load needs (diagnosis builds only)
static void LogMemory(const char* When)
{
    if (!LogEnabled())
        return;
    PROCESS_MEMORY_COUNTERS_EX Pmc;
    MEMORYSTATUSEX Status;
    Status.dwLength = sizeof(Status);
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&Pmc, sizeof(Pmc)) &&
        GlobalMemoryStatusEx(&Status))
        Log("cachecleaner", "%s: private %d MB, working set %d MB, address space left %d MB", When,
            (INT)(Pmc.PrivateUsage >> 20), (INT)(Pmc.WorkingSetSize >> 20), (INT)(Status.ullAvailVirtual >> 20));
}

static void CleanCaches()
{
    LogMemory("before");
    if (bCacheCleanFlush && IniInt("CacheCleaner", "Flush", 1))
        ExecCommand(TEXT("FLUSH"));
    if (ExecCommand(TEXT("OBJ GARBAGE")) <= 0)
    {
        try
        {
            UObject::CollectGarbage(RF_Native);
        }
        catch (...)
        {
        }
    }
    CombatLog_OnAfterCacheClean();
    LogMemory("after");
}

void CacheCleaner_OnTick(NConsoleWnd* Console, FLOAT DeltaTime)
{
    if (!Console || DeltaTime <= 0.f || DeltaTime > 5.f)
        return;
    if (Console->State != L2CS_InGame)
        return;  // the time at the login and the character selection does not count
    GCacheCleanTime += DeltaTime;
    if (GCacheCleanTime < CacheCleanInterval)
        return;
    GCacheCleanTime = 0.f;
    if (!IniInt("CacheCleaner", "Enable", 1))
        return;
    try
    {
        CleanCaches();
    }
    catch (...)
    {
    }
}

}  // namespace nwf

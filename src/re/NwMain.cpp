/*=============================================================================
    NwMain.cpp - NWindowDecomp.dll entry point (hook mode).

    Load order: the engine LoadLibrary()s NWindow.dll (stock image + an import of
    NWindowDecomp_Attach added by tools/install_test.py) -> the loader maps
    NWindowDecomp.dll and runs our DllMain first -> we verify the stock image and
    patch the reimplemented functions -> the stock DllMain / static
    initializers run, already on top of our code where hooked.
=============================================================================*/
#include "re/NwHook.h"
#include "re/NwLog.h"
#include "re/NwDebug.h"

namespace nw {

static const DWORD kStockTimeDateStamp = 0x40DBD452;
static const DWORD kStockSizeOfCode = 0x000D6000;
static const DWORD kStockEntryPoint = 0x000BF1A3;

static const char* StateText(int s)
{
    switch (s)
    {
    case HOOK_INSTALLED: return "ok";
    case HOOK_DISABLED: return "disabled (ini)";
    case HOOK_ERR_UNKNOWN: return "ERROR not a stock function entry";
    case HOOK_ERR_BYTES: return "ERROR stock bytes differ";
    case HOOK_ERR_SPACE: return "ERROR no room for the jump";
    case HOOK_ERR_PROTECT: return "ERROR VirtualProtect";
    case HOOK_ERR_DUPLICATE: return "ERROR duplicate";
    default: return "pending";
    }
}

static bool IsStockImage(HMODULE mod)
{
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)mod;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;
    const IMAGE_NT_HEADERS32* nt = (const IMAGE_NT_HEADERS32*)((const BYTE*)mod + dos->e_lfanew);
    Log("stock NWindow.dll at %08lx: TimeDateStamp %08lx SizeOfCode %08lx Entry %08lx",
        (DWORD)mod, nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfCode,
        nt->OptionalHeader.AddressOfEntryPoint);
    return nt->Signature == IMAGE_NT_SIGNATURE && nt->FileHeader.TimeDateStamp == kStockTimeDateStamp &&
           nt->OptionalHeader.SizeOfCode == kStockSizeOfCode &&
           nt->OptionalHeader.AddressOfEntryPoint == kStockEntryPoint;
}

// link time of this DLL (PE TimeDateStamp: seconds since 1970, UTC) in local time; __DATE__
// would only tell when this file was last compiled
static SYSTEMTIME BuildTime(HMODULE self)
{
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)self;
    const IMAGE_NT_HEADERS32* nt = (const IMAGE_NT_HEADERS32*)((const BYTE*)self + dos->e_lfanew);
    ULONGLONG t = (ULONGLONG)nt->FileHeader.TimeDateStamp * 10000000ULL + 116444736000000000ULL;
    FILETIME utc, local;
    utc.dwLowDateTime = (DWORD)t;
    utc.dwHighDateTime = (DWORD)(t >> 32);
    SYSTEMTIME st = {};
    FileTimeToLocalFileTime(&utc, &local);
    FileTimeToSystemTime(&local, &st);
    return st;
}

static void Startup(HMODULE self)
{
    LogOpen(self);
    LoadConfig(self);
    g_buildTime = BuildTime(self);
    Log("NWindowDecomp build %04u-%02u-%02u %02u:%02u:%02u attach (pid %lu)", g_buildTime.wYear,
        g_buildTime.wMonth, g_buildTime.wDay, g_buildTime.wHour, g_buildTime.wMinute,
        g_buildTime.wSecond, GetCurrentProcessId());
    if (GetConfig().debugMarker)
        Log("[Debug] Marker=1: proof marks on screen (re/NwDebug.h)");

    HMODULE stock = GetModuleHandleW(L"NWindow.dll");
    if (!stock)
    {
        Log("NWindow.dll is not loaded: nothing to do (load NWindowDecomp.dll through the patched NWindow.dll)");
        return;
    }
    if (!IsStockImage(stock))
    {
        Log("NWindow.dll is not the stock C1 build this project decompiles: no hook installed");
        return;
    }
    g_nwBase = (DWORD)stock;

    int total = 0;
    for (HookEntry* h = FirstHook(); h; h = h->next)
        total++;
    if (!GetConfig().hooksEnabled)
        Log("[Hooks] Enabled=0: running the stock code only");
    int installed = InstallHooks();
    g_hooksInstalled = installed;

    int errors = 0;
    for (HookEntry* h = FirstHook(); h; h = h->next)
    {
        if (h->state < 0)
            errors++;
        // "ours=" is the RVA of the replacement in NWindowDecomp.dll (tools/verify_hooks.py)
        Log("  %08lx %-48s ours=%06lx %s%s", h->va, h->name, (DWORD)h->fn - (DWORD)self,
            StateText(h->state), h->state == HOOK_INSTALLED && !h->trampoline ? " (no trampoline)" : "");
    }
    Log("hooks: %d installed, %d registered, %d errors", installed, total, errors);

    // vtables compiled here, slot by slot: stock VA of the target / RVA in this DLL
    for (VtableCheck* v = FirstVtable(); v; v = v->next)
    {
        const DWORD* stock = (const DWORD*)Addr(v->stockVA);
        for (int k = 0; k < v->slots; k++)
            Log("  vt %s %d stock=%08lx ours=%06lx", v->name, k, stock[k] - g_nwBase + kStockBase,
                (DWORD)v->ours[k] - (DWORD)self);
    }
}

}  // namespace nw

// Imported by the patched stock NWindow.dll only to make the loader bring us in.
extern "C" __declspec(dllexport) void __cdecl NWindowDecomp_Attach() {}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(inst);
        nw::Startup(inst);
    }
    return TRUE;
}

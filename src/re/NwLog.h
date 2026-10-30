#pragma once
// NWindowDecomp.log / NWindowDecomp.ini, both next to NWindowDecomp.dll.
// Plain Win32 only: used from DllMain, before the engine is usable.

#include <windows.h>

namespace nw {

void LogOpen(HMODULE self);
void Log(const char* fmt, ...);

struct Config
{
    bool hooksEnabled = true;  // [Hooks] Enabled=0 keeps the whole stock DLL
    char disable[4096] = {};   // [Hooks] Disable= list of stock addresses / Class::Method / Class::*
    bool debugMarker = false;  // [Debug] Marker=1: proof marks drawn by our code (re/NwDebug.h)
    bool logFlush = true;      // [Log] Flush=0: no FlushFileBuffers per line (the lines still
                               // reach the file if the game crashes; only a crash of Windows
                               // could lose them). tools/difftest.py, where a flush per line
                               // costs up to 8 ms when the disk is busy
};

const Config& GetConfig();
void LoadConfig(HMODULE self);
bool IsDisabledByConfig(DWORD va, const char* name);

}  // namespace nw

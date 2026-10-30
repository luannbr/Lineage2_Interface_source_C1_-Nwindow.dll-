/*=============================================================================
    NwFeatures.h - phase 6: the LineageWindow features built into our NWindow.dll.

    NWindow_C1's LineageWindow.dll added them to the stock NWindow.dll from outside
    (import injection, vtable and ILT patches at stock addresses). Here they are
    source: feature code in src/features/, and each place the decompiled code has to
    call it is an `#if NW_FEATURES` block with a "phase 6:" comment. Only the NWindowL2
    target defines NW_FEATURES=1; hook mode and the faithful NWindowStandalone never
    see this code, so every stock comparison keeps running on them. Plan, feature list
    and conventions: docs/notas/Fase6.md.

    Settings keep NWindow_C1's LineageWindow.ini sections and keys, but no file: the
    values are compiled in, and what the player changes in the game is kept in the
    registry (HKCU\Software\Lineage2C1\NWindow). The log exists only in builds with
    NW_FEATURE_LOG=1 (src/features/NwFeatureConfig.cpp).
=============================================================================*/
#pragma once

#ifndef NW_FEATURES
#define NW_FEATURES 0
#endif

#if NW_FEATURES

#include <windows.h>

class NConsoleWnd;

namespace nwf {

// Our DllMain (src/standalone/NWindowPackage.cpp) at DLL_PROCESS_ATTACH / DLL_PROCESS_DETACH:
// what NWindow_C1 started from LineageWindow.dll's DllMain, which ran in the same LoadLibrary
// (an import of NWindow.dll), before the engine opened its viewport window
// (src/features/NwFeatureStartup.cpp).
void OnDllMain(DWORD Reason);

// NConsoleWnd::Tick, first thing: every feature's per-frame work in NWindow_C1's order, under one
// exception handler (src/features/NwFeatureTick.cpp)
void OnConsoleTick(NConsoleWnd* Console, float DeltaTime);

// <folder of our NWindow.dll>\<Name>
void ModuleSibling(const char* Name, char* Out, DWORD OutSize);

// A setting by LineageWindow.ini's section and key: the value the player saved in the game
// (registry), else the compiled-in value, else Default. The writes go to the registry.
INT  IniInt(const char* Section, const char* Key, INT Default);
void IniString(const char* Section, const char* Key, const char* Default, char* Out, DWORD OutSize);
void IniWriteInt(const char* Section, const char* Key, INT Value);
void IniWriteString(const char* Section, const char* Key, const char* Value);

// LineageWindow.log next to our module: one "<tag>: <text>" line (NWindow_C1's wlog / clog /
// chatlog lines), only in builds with NW_FEATURE_LOG=1; LogEnabled() says which.
bool LogEnabled();
void Log(const char* Tag, const char* Fmt, ...);

}  // namespace nwf

#endif  // NW_FEATURES

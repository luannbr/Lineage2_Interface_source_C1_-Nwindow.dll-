/*=============================================================================
    NWindowPackage.cpp - IMPLEMENT_PACKAGE(NWindow) of the standalone NWindow.dll.

    The engine never imports NWindow.dll: Core's UPackage::Bind loads it by name
    (package "NWindow" + ".dll") when Engine.dll's StaticLoadClass asks for
    NWindow.ConsoleWnd or NWindow.L2Font, then finds GPackage and every
    autoclass<Class> with GetProcAddress (docs/notas/Standalone.md, section 1).
    The classes register themselves in Classes.gen.cpp; this file only has the
    package name and the entry point (stock 0x10024080, run by the C runtime's
    _DllMainCRTStartup after the static initializers, as in the stock image).
=============================================================================*/
#include "nwindow/NWindow.h"
#include "features/NwFeatures.h"

#if NW_HOOK_MODE
#error "NWindowPackage.cpp is for the standalone build only"
#endif

#if !NW_FEATURES
NW_IMPLEMENT_PACKAGE(NWindow)
#else
// phase 6: NW_IMPLEMENT_PACKAGE plus the start and stop of the LineageWindow features
// (src/features/NwFeatureStartup.cpp): NWindow_C1's LineageWindow.dll was an import of
// NWindow.dll, so its DllMain ran in this same LoadLibrary (NWindow_C1
// src/inject/NWindow_inject.cpp)
TCHAR GPackage[] = TEXT("NWindow");
HINSTANCE GNWindowInstance;
extern "C" INT STDCALL DllMain(HINSTANCE hInInstance, DWORD Reason, void* Reserved)
{
    GNWindowInstance = hInInstance;
    nwf::OnDllMain(Reason);
    return 1;
}
#endif

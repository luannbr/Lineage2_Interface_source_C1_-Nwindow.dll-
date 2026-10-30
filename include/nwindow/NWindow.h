/*=============================================================================
    NWindow.h - common header of the reconstructed NWindow package.

    Conventions (docs/CONVENCOES.md):
      * every reimplemented function carries the stock address it replaces
        (NW_HOOK_* next to the definition, "// 0x1000cc40" in declarations);
      * names we invented are marked "(name ours)" until a string or an export
        confirms them; stock names come from guard() strings, exports, vtables;
      * layouts carry NW_ASSERT_OFFSET / NW_ASSERT_SIZE against the stock sizes
        (analysis/classes.md).
=============================================================================*/
#pragma once

#include "ue2/UE2Core.h"
#include "ue2/UE2Engine.h"

#include <imm.h>

// UE2's class operator new (Outer, Name, Flags) has no matching delete, like the original
#pragma warning(disable : 4291)

#if NW_HOOK_MODE
#include "re/NwHook.h"
#else
#include "standalone/NwStandalone.h"
#endif

/*-----------------------------------------------------------------------------
    NW_DECLARE_CLASS: UE2's DECLARE_CLASS for a class of this package.
    Hook mode: the UClass object is the stock one (IMPLEMENT_CLASS ran in the
    stock image), StaticClassVA is its address (analysis/classes.json).
    Standalone: the UClass is ours (NW_IMPLEMENT_CLASS below), StaticClassVA
    is unused.
    operator new is forced inline: inside a guard (/EHa) our compiler kept it
    out of line, where the stock code has it inline.
    NW_DECLARE_ABSTRACT_CLASS: UE2's DECLARE_ABSTRACT_CLASS, adds CLASS_Abstract
    and declares no InternalConstructor, so the parent's is registered (the
    stock NCCmdTarget and NCAbstractMain register NCObject's, 0x100042f0).
-----------------------------------------------------------------------------*/
enum { CLASS_Abstract = 0x00001, CLASS_Config = 0x00004 };
enum EInternal { EC_Internal };

#if NW_HOOK_MODE
#define NW_DECLARE_CLASS(TClass, TSuperClass, TStaticFlags, StaticClassVA)                  \
public:                                                                                     \
    typedef TSuperClass Super;                                                              \
    typedef TClass ThisClass;                                                               \
    enum { StaticClassFlags = TStaticFlags };                                               \
    static UClass* StaticClass() { return reinterpret_cast<UClass*>(::nw::Addr(StaticClassVA)); } \
    virtual ~TClass() { ConditionalDestroy(); }                                             \
    FORCEINLINE void* operator new(size_t,                                                  \
                                   UObject* Outer = (UObject*)UObject::GetTransientPackage(), \
                                   FName Name = NAME_None, DWORD SetFlags = 0)              \
    {                                                                                       \
        return StaticAllocateObject(StaticClass(), Outer, Name, SetFlags);                  \
    }                                                                                       \
    void* operator new(size_t, EInternal* Mem) { return (void*)Mem; }
#define NW_DECLARE_ABSTRACT_CLASS(TClass, TSuperClass, TStaticFlags, StaticClassVA)         \
    NW_DECLARE_CLASS(TClass, TSuperClass, (TStaticFlags) | CLASS_Abstract, StaticClassVA)
#else
// InternalConstructor is the UClass's ClassConstructor: Core's StaticAllocateObject fills in
// the UObject header and then constructs the object in place (default-initialized, no
// parentheses: a class without a constructor of its own must not be zeroed).
#define NW_DECLARE_BASE_CLASS(TClass, TSuperClass, TStaticFlags)                            \
private:                                                                                    \
    static UClass PrivateStaticClass;                                                       \
                                                                                            \
public:                                                                                     \
    typedef TSuperClass Super;                                                              \
    typedef TClass ThisClass;                                                               \
    enum { StaticClassFlags = TStaticFlags };                                               \
    static UClass* StaticClass() { return &PrivateStaticClass; }                            \
    virtual ~TClass() { ConditionalDestroy(); }                                             \
    FORCEINLINE void* operator new(size_t,                                                  \
                                   UObject* Outer = (UObject*)UObject::GetTransientPackage(), \
                                   FName Name = NAME_None, DWORD SetFlags = 0)              \
    {                                                                                       \
        return StaticAllocateObject(StaticClass(), Outer, Name, SetFlags);                  \
    }                                                                                       \
    void* operator new(size_t, EInternal* Mem) { return (void*)Mem; }
#define NW_DECLARE_CLASS(TClass, TSuperClass, TStaticFlags, StaticClassVA)                  \
    NW_DECLARE_BASE_CLASS(TClass, TSuperClass, TStaticFlags)                                \
    static void InternalConstructor(void* X) { new ((EInternal*)X) TClass; }
#define NW_DECLARE_ABSTRACT_CLASS(TClass, TSuperClass, TStaticFlags, StaticClassVA)         \
    NW_DECLARE_BASE_CLASS(TClass, TSuperClass, (TStaticFlags) | CLASS_Abstract)

/*-----------------------------------------------------------------------------
    NW_IMPLEMENT_CLASS: UE2's IMPLEMENT_CLASS, with the registration the stock
    image makes (analysis/classes.json, src/standalone/Classes.gen.cpp from
    tools/gen_implement_class.py). The parent UClass and the class flags are
    arguments, not TClass::Super / StaticClassFlags: the stock image registers
    NCTestHtmlViewer and NCTradeItemWnd under NCWnd, whatever their C++ base.
    Within UObject, GUID 0, UObject's config name and static constructor, as
    in all 178 stock initializers. autoclass<Class> is exported through
    src/standalone/NWindow.def (UE2 marks it DLL_EXPORT here).
-----------------------------------------------------------------------------*/
#define NW_IMPLEMENT_CLASS(TClass, TSuperClass, TClassFlags)                                    \
    UClass TClass::PrivateStaticClass(EC_NativeConstructor, sizeof(TClass), (TClassFlags),       \
                                      TSuperClass::StaticClass(), UObject::StaticClass(),       \
                                      FGuid(), TEXT(#TClass) + 1, GPackage,                     \
                                      UObject::StaticConfigName(),                              \
                                      RF_Public | RF_Standalone | RF_Transient | RF_Native,      \
                                      (void (*)(void*))TClass::InternalConstructor,             \
                                      (void (UObject::*)()) & UObject::StaticConstructor);      \
    extern "C" UClass* autoclass##TClass;                                                       \
    UClass* autoclass##TClass = TClass::StaticClass();

/*-----------------------------------------------------------------------------
    NW_IMPLEMENT_PACKAGE: UE2's IMPLEMENT_PACKAGE (src/standalone/NWindowPackage.cpp).
    GPackage names the package the classes register under; DllMain keeps the
    module handle for the resources (the color cursors), stock 0x10024080:
    "hInstance = hInInstance; return 1;". Both exported through NWindow.def.
-----------------------------------------------------------------------------*/
#define NW_IMPLEMENT_PACKAGE(pkgname)                                                           \
    TCHAR GPackage[] = TEXT(#pkgname);                                                          \
    HINSTANCE GNWindowInstance;                                                                 \
    extern "C" INT STDCALL DllMain(HINSTANCE hInInstance, DWORD Reason, void* Reserved)         \
    {                                                                                           \
        GNWindowInstance = hInInstance;                                                         \
        return 1;                                                                               \
    }
#endif

class NConsoleWnd;
class FL2TextAlignParser;

/*-----------------------------------------------------------------------------
    Package globals (names ours).
-----------------------------------------------------------------------------*/
#if NW_HOOK_MODE
#define GConsole         NW_GLOBAL(NConsoleWnd*, 0x10122894)   // set by the console init (1004d6c0)
#define GCanvas          NW_GLOBAL(UCanvas*, 0x10122898)       // viewport canvas, set with GConsole
#define GTextAlignParser NW_GLOBAL(FL2TextAlignParser*, 0x101228a4)
#define GNWindowDebug    NW_GLOBAL(INT, 0x10122888)            // NCObject::Debugf target: 0 off, 1 debug window
#define GViewportHWnd    NW_GLOBAL(HWND, 0x10122890)           // game window: IME and ClientToScreen
#define GNWindowInstance NW_GLOBAL(HINSTANCE, 0x10124284)      // this package's module (DllMain), for resources
#else
// standalone: defined with the stock initial values in src/standalone/NwGlobals.cpp
// (tools/gen_globals.py), GNWindowInstance by NW_IMPLEMENT_PACKAGE
extern NConsoleWnd* GConsole;
extern UCanvas* GCanvas;
extern FL2TextAlignParser* GTextAlignParser;
extern INT GNWindowDebug;
extern HWND GViewportHWnd;
extern HINSTANCE GNWindowInstance;
#endif

/*=============================================================================
    NwStandalone.h - the hook-mode macros for the standalone build (NW_HOOK_MODE=0).

    The same source builds NWindow.dll on its own (phase 5, docs/notas/Standalone.md):
      * the NW_HOOK_* registrations are empty and NW_CHECK_VTABLE only keeps the
        nwvt_<Class> name of our vtable;
      * what still reaches the stock image - NW_STOCK, NW_GLOBAL, nw::Addr,
        nw::StockCall - has no definition here, so the link fails and names each
        one (_nw_stock_<va>, nw::StockData<va>::Bytes, nw::Addr, nw::StockCall<...>):
        the list of what is left to write (tools/standalone_report.py);
      * nw::Field and nw::VirtualCall keep working (no stock code behind them).
    Included by NWindow.h after the UE2 headers, in place of re/NwHook.h.
=============================================================================*/
#pragma once

#include <windows.h>

#if NW_HOOK_MODE
#error "NwStandalone.h is for the standalone build only (NW_HOOK_MODE=0)"
#endif

/*-----------------------------------------------------------------------------
    What IMPLEMENT_CLASS (src/standalone/Classes.gen.cpp) needs from Core.
-----------------------------------------------------------------------------*/

enum ENativeConstructor { EC_NativeConstructor };

// Core.dll's UClass, opaque: only its native constructor (the one IMPLEMENT_CLASS calls) and
// its destructor (registered with atexit for every PrivateStaticClass). 0x520 bytes in C1:
// the spacing of the stock PrivateStaticClass objects.
class UClass : public UField
{
public:
    // ??0UClass@@QAE@W4ENativeConstructor@@KKPAV0@1VFGuid@@PBG33KP6AXPAX@ZP8UObject@@AEXXZ@Z
    CORE_API UClass(ENativeConstructor, DWORD InSize, DWORD InClassFlags, UClass* InSuperClass,
                    UClass* InWithinClass, FGuid InGuid, const TCHAR* InNameString,
                    const TCHAR* InPackageName, const TCHAR* InClassConfigName, DWORD InFlags,
                    void (*InClassConstructor)(void*), void (UObject::*InClassStaticConstructor)());
    CORE_API virtual ~UClass();

private:
    BYTE Opaque[0x520 - sizeof(UField)];
};
NW_ASSERT_SIZE(UClass, 0x520);

// IMPLEMENT_PACKAGE(NWindow) (src/standalone/NWindowPackage.cpp): the package name Core
// registers the classes under. Exported by name through src/standalone/NWindow.def.
extern "C" TCHAR GPackage[];

/*-----------------------------------------------------------------------------
    The hook macros: nothing to register.
-----------------------------------------------------------------------------*/

#define NW_CAT_(a, b) a##b
#define NW_CAT(a, b) NW_CAT_(a, b)
#define NW_UNPAREN(...) __VA_ARGS__

#define NW_HOOK_METHOD_SIG(va, Class, Method, Sig)
#define NW_HOOK_METHOD(va, Class, Method)
#define NW_HOOK_VARARGS(va, Class, Method, Sig)
#define NW_HOOK_FUNC(va, Func)
#define NW_HOOK_CTOR(va, Class)
#define NW_HOOK_CTOR_ARGS(va, Class, Params, Args)
#define NW_HOOK_DTOR(va, Class)

// our vtable stays reachable as nwvt_<Class> (NConsoleWndWindows.cpp's NW_HOOK_VARARGS_SLOT
// reads slots from it); there is no stock vtable to compare with
#define NW_CHECK_VTABLE(Class, StockVA, Slots)                                                  \
    __pragma(comment(linker, "/alternatename:_nwvt_" #Class "=??_7" #Class "@@6B@"))            \
    extern "C" const void* const nwvt_##Class[];
#define NW_CHECK_VTABLE_FOR(Class, Base, StockVA, Slots)                                        \
    __pragma(comment(linker, "/alternatename:_nwvt_" #Class "=??_7" #Class "@@6B" #Base "@@@")) \
    extern "C" const void* const nwvt_##Class[];

namespace nw {

// the registration object NW_HOOK_VARARGS_SLOT builds: nothing to register
struct HookReg
{
    HookReg(DWORD, void*, const char*, const char*) {}
};

}  // namespace nw

/*-----------------------------------------------------------------------------
    What still reaches the stock image: declared, never defined.
-----------------------------------------------------------------------------*/

#define NW_NAKED __declspec(naked)

// a body still in the stock image: the link reports "_nw_stock_<va>"
#define NW_STOCK(va) { __pragma(comment(linker, "/include:_nw_stock_" #va)) __asm int 3 }

namespace nw {

// a stock global still reached by address: the link reports nw::StockData<va>::Bytes (va in
// decimal)
template <DWORD VA> struct StockData
{
    static BYTE Bytes[];
};

// a stock table still read by address, and thiscall into stock code: one unresolved symbol
// each (nw::Addr, nw::StockCall<...>); tools/standalone_report.py lists the call sites
void* Addr(DWORD va);
template <class R, class... A> R StockCall(DWORD va, const void* self, A... a);

// Field of an object whose class has no declaration here yet.
template <class T> inline T& Field(const void* obj, DWORD offset)
{
    return *reinterpret_cast<T*>(reinterpret_cast<BYTE*>(const_cast<void*>(obj)) + offset);
}

// thiscall through vtable slot `slot` of an object whose class does not declare its virtuals.
template <class R, class... A> inline R VirtualCall(const void* self, int slot, A... a)
{
    typedef R(__fastcall * Fn)(const void*, void*, A...);
    return (*reinterpret_cast<Fn* const*>(self))[slot](self, 0, a...);
}

}  // namespace nw

#define NW_GLOBAL(Type, va) (*reinterpret_cast<Type*>(::nw::StockData<(va)>::Bytes))

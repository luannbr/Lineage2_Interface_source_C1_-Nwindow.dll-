/*=============================================================================
    NCIME.cpp - base of the language IMEs. Stock: 0x1000dee0 .. 0x1000e22f,
    0x1000e840 .. 0x1000e88f, 0x1000f2b0 .. 0x1000f2ff, 0x1000f6f0 .. 0x1000f758.
=============================================================================*/
#include "nwindow/NCIME.h"
#include "nwindow/NCrt.h"

#include <stdlib.h>

// 0x1000f6f0 - NControl's constructor is inlined in the stock code
NCIME::NCIME(INT InMode)
{
    Property = 0;
    OpenStatus = 0;
    Conversion = 0;
    Sentence = 0;
    CodePage = 0;
    bActive = 0;
    Mode = InMode;
    bNotComposing = 1;
}
NW_HOOK_CTOR_ARGS(0x1000f6f0, NCIME, (INT A), (A))

// 0x1000f2b0
NW_HOOK_DTOR(0x1000f2b0, NCIME)

// 0x1000dee0 (vslot 24)
INT NCIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_IME_NOTIFY)
        return OnImeNotify(wParam, lParam) != 0;
    if (Msg == WM_IME_SETCONTEXT)
        return OnImeSetContext(wParam, lParam) != 0;
    if (Msg == WM_IME_STARTCOMPOSITION)
        return OnImeStartComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_ENDCOMPOSITION)
        return OnImeEndComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_CONTROL)
        return OnImeControl(wParam, lParam) != 0;
    if (Msg == WM_INPUTLANGCHANGE)
        return OnInputLangChange(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITIONFULL)
        return OnImeCompositionFull(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITION)
        return OnImeComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_CHAR)
        return OnImeChar(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1000dee0, NCIME, WindowProc)

// 0x1000e050 (vslot 35) - own input context; the IME's default window stays hidden
INT NCIME::OnCreate()
{
    NControl::OnCreate();
    hIMC = ImmCreateContext();
    HWND IMEWnd = ImmGetDefaultIMEWnd(GViewportHWnd);
    if (IMEWnd)
        ::ShowWindow(IMEWnd, SW_HIDE);
    return 1;
}
NW_HOOK_METHOD(0x1000e050, NCIME, OnCreate)

// 0x1000e150 (vslot 63) - the game window uses this IME's context
void NCIME::Activate()
{
    bActive = 1;
    if (hIMC)
        ImmAssociateContext(GViewportHWnd, hIMC);
}
NW_HOOK_METHOD(0x1000e150, NCIME, Activate)

// 0x1000e090 (vslot 66)
INT NCIME::OnImeStartComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 0;
    return 1;
}
NW_HOOK_METHOD(0x1000e090, NCIME, OnImeStartComposition)

// 0x1000e0b0 (vslot 67)
INT NCIME::OnImeEndComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 1;
    return 1;
}
NW_HOOK_METHOD(0x1000e0b0, NCIME, OnImeEndComposition)

// 0x1000e0e0 (vslot 68) - lets the IME draw its own UI when it has to; an IME that draws at
// the caret gets the context without its composition window
INT NCIME::OnImeSetContext(WPARAM wParam, LPARAM lParam)
{
    if (Property & IME_PROP_SPECIAL_UI)
        ImeIsUIMessage(WM_IME_SETCONTEXT, wParam, lParam);
    else if (Property & IME_PROP_AT_CARET)
        ImeIsUIMessage(WM_IME_SETCONTEXT, wParam, lParam & ~ISC_SHOWUICOMPOSITIONWINDOW);
    return 1;
}
NW_HOOK_METHOD(0x1000e0e0, NCIME, OnImeSetContext)

// 0x1000e0c0 (vslot 70)
INT NCIME::OnInputLangChange(WPARAM wParam, LPARAM lParam)
{
    Property = ImeGetProperty(IGP_PROPERTY);
    return 1;
}
NW_HOOK_METHOD(0x1000e0c0, NCIME, OnInputLangChange)

// 0x1000e120
TCHAR* NCIME::GetResultStr()
{
    return ResultStr;
}
NW_HOOK_METHOD(0x1000e120, NCIME, GetResultStr)

// 0x1000e130
TCHAR* NCIME::GetCompStr()
{
    return CompStr;
}
NW_HOOK_METHOD(0x1000e130, NCIME, GetCompStr)

// 0x1000e140
TCHAR* NCIME::GetReadingStr()
{
    return ReadingStr;
}
NW_HOOK_METHOD(0x1000e140, NCIME, GetReadingStr)

// 0x1000e180
UBOOL NCIME::IsNotComposing()
{
    return bNotComposing;
}
NW_HOOK_METHOD(0x1000e180, NCIME, IsNotComposing)

// 0x1000e190 - NT, or Windows 4.x from 98 on; asked once
UBOOL NCIME::IsUnicodeOS()
{
    if (!GIMEOSChecked)
    {
        OSVERSIONINFOW Info;
        Info.dwOSVersionInfoSize = sizeof(Info);
#pragma warning(suppress : 4996)  // deprecated in current SDKs; the stock code calls it
        GetVersionExW(&Info);
        if (Info.dwPlatformId == VER_PLATFORM_WIN32_NT || Info.dwMajorVersion > 4 ||
            (Info.dwMajorVersion == 4 && Info.dwMinorVersion > 0))
            GIMEUnicodeOS = 1;
        GIMEOSChecked = 1;
    }
    return GIMEUnicodeOS;
}
NW_HOOK_METHOD(0x1000e190, NCIME, IsUnicodeOS)

// 0x1000e1f0 - CodePage becomes the default ANSI code page of the keyboard layout's language
void NCIME::UpdateCodePage()
{
    ANSICHAR Buf[12];
    if (GetLocaleInfoA(LOWORD(GetKeyboardLayout(0)), LOCALE_IDEFAULTANSICODEPAGE, Buf, 10))
        CodePage = NAtol(Buf);
}
NW_HOOK_METHOD(0x1000e1f0, NCIME, UpdateCodePage)

// 0x1000e840 - the library is not freed
void NCIME::LoadImmFunctions()
{
    HMODULE Imm = LoadLibraryA("imm32.dll");
    if (Imm)
    {
        GImmLockIMC = (NImmLockIMC)GetProcAddress(Imm, "ImmLockIMC");
        GImmUnlockIMC = (NImmUnlockIMC)GetProcAddress(Imm, "ImmUnlockIMC");
        GImmLockIMCC = (NImmLockIMCC)GetProcAddress(Imm, "ImmLockIMCC");
        GImmUnlockIMCC = (NImmUnlockIMCC)GetProcAddress(Imm, "ImmUnlockIMCC");
    }
}
NW_HOOK_FUNC(0x1000e840, NCIME::LoadImmFunctions)

NW_CHECK_VTABLE(NCIME, 0x100d9fd8, 74)

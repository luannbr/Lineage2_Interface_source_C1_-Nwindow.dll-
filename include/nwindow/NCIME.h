#pragma once
// NCIME - base of the language IMEs (UClass "CIME", 0x41F8), hidden controls owned by
// NCIMEManager. Each keeps its own input context and a copy of every composition string
// (ImmGetCompositionStringW), which the edit boxes read. WindowProc turns the IME window
// messages into virtual calls; the base handlers only follow the composition state and the
// IME properties. Activate/Deactivate run when an edit box gains or loses the focus.
//
// Private messages between the IMEs, NCIMEManager and the edit boxes (names ours). The IMEs send
// them to their parent (NControl::SendToParent), the manager passes them on to the edit box:
//   0xF0000012 composition changed (wParam: last composed character, 0 when none, or 1)
//   0xF0000013 result string ready          0xF0000017 composition ended (Japanese, Chinese)
//   0xF0000015 caret position wanted: wParam = the window to answer with 0xF0000016 (X, Y)
//   0xF0000018 composition attribute on/off 0xF0000019 cursor position (wParam)
//   0xF000001A reading string changed (Chinese, Taiwanese)

#include "nwindow/NControl.h"
#include "nwindow/NCCandidateBox.h"

enum
{
    NIM_COMPOSITION = (INT)0xF0000012,
    NIM_RESULT = (INT)0xF0000013,
    NIM_14 = (INT)0xF0000014,
    NIM_GETCARET = (INT)0xF0000015,
    NIM_CARET = (INT)0xF0000016,
    NIM_ENDCOMPOSITION = (INT)0xF0000017,
    NIM_ATTRIBUTE = (INT)0xF0000018,
    NIM_CURSORPOS = (INT)0xF0000019,
    NIM_READING = (INT)0xF000001A,
};

class NCIME : public NControl
{
    NW_DECLARE_CLASS(NCIME, NControl, 0, 0x1011ef28)

    HIMC hIMC;                       // +0x1A0 own input context (ImmCreateContext)
    UBOOL bNotComposing;             // +0x1A4 cleared between WM_IME_START/ENDCOMPOSITION
    INT Mode;                        // +0x1A8 (Korean: 1 starts in native mode)
    UBOOL bActive;                   // +0x1AC between Activate and Deactivate
    INT Unknown1B0;                  // +0x1B0
    INT CursorPos;                   // +0x1B4 GCS_CURSORPOS
    INT Unknown1B8;                  // +0x1B8
    DWORD Property;                  // +0x1BC IGP_PROPERTY of the current IME
    DWORD Conversion;                // +0x1C0 conversion / sentence mode kept while inactive
    DWORD Sentence;                  // +0x1C4
    UBOOL OpenStatus;                // +0x1C8 ImmGetOpenStatus kept (Japanese, Taiwanese, Chinese)
    UINT CodePage;                   // +0x1CC of the keyboard layout (non-Unicode systems)
    TCHAR CompStr[1024];             // +0x1D0 GCS_COMPSTR
    TCHAR CompReadStr[1024];         // +0x9D0 GCS_COMPREADSTR
    TCHAR ResultStr[1024];           // +0x11D0 GCS_RESULTSTR
    TCHAR Unknown19D0[1024];         // +0x19D0
    TCHAR ReadingStr[1024];          // +0x21D0 the reading string (Taiwanese, Chinese: NIM_READING)
    BYTE CompAttr[1024];             // +0x29D0 GCS_COMPATTR
    BYTE CompReadAttr[1024];         // +0x2DD0 GCS_COMPREADATTR
    DWORD CompClause[256];           // +0x31D0 GCS_COMPCLAUSE
    DWORD CompReadClause[256];       // +0x35D0 GCS_COMPREADCLAUSE
    DWORD ResultClause[256];         // +0x39D0 GCS_RESULTCLAUSE
    DWORD Unknown3DD0[256];          // +0x3DD0
    INT CompStrLen;                  // +0x41D0 characters
    INT CompAttrLen;                 // +0x41D4
    INT CompClauseLen;               // +0x41D8 entries
    INT CompReadStrLen;              // +0x41DC
    INT CompReadAttrLen;             // +0x41E0
    INT CompReadClauseLen;           // +0x41E4
    INT ResultStrLen;                // +0x41E8
    INT ResultClauseLen;             // +0x41EC
    INT Unknown41F0;                 // +0x41F0
    INT Unknown41F4;                 // +0x41F4

    NCIME(INT InMode = 0);                                           // 0x1000f6f0
    // ~NCIME                                                        // 0x1000f2b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000dee0
    virtual INT OnCreate();                                          // 35 0x1000e050
    virtual INT OnDestroy() { return 1; }                            // 36 0x10005230 (folded)
    // names ours
    virtual void Activate();                                         // 63 0x1000e150
    virtual void Deactivate() {}                                     // 64 0x10045870 (folded)
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam) { return 1; }          // 65 0x100098f0
    virtual INT OnImeStartComposition(WPARAM wParam, LPARAM lParam);             // 66 0x1000e090
    virtual INT OnImeEndComposition(WPARAM wParam, LPARAM lParam);               // 67 0x1000e0b0
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam);                   // 68 0x1000e0e0
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam) { return 1; }     // 69 0x100098f0
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam);                 // 70 0x1000e0c0
    virtual INT OnImeControl(WPARAM wParam, LPARAM lParam) { return 1; }         // 71 0x100098f0
    virtual INT OnImeCompositionFull(WPARAM wParam, LPARAM lParam) { return 1; } // 72 0x100098f0
    virtual INT OnImeChar(WPARAM wParam, LPARAM lParam) { return 1; }            // 73 0x100098f0

    // names ours
    TCHAR* GetResultStr();                                           // 0x1000e120
    TCHAR* GetCompStr();                                             // 0x1000e130
    TCHAR* GetReadingStr();                                          // 0x1000e140
    UBOOL IsNotComposing();                                          // 0x1000e180
    UBOOL IsUnicodeOS();                                             // 0x1000e190 NT or Win98+
    // the Taiwanese and Chinese IMEs' setup (names ours)
    void UpdateCodePage();                                           // 0x1000e1f0
    static void LoadImmFunctions();                                  // 0x1000e840

    // the test every WM_IME_COMPOSITION handler makes before each string (inlined): Chinese,
    // Taiwanese and Japanese IMEs on a system without Unicode IME calls convert from CodePage
    UBOOL IsAnsiIME()
    {
        return (GLanguageType == 2 || GLanguageType == 3 || GLanguageType == 4) && !IsUnicodeOS();
    }
};
NW_ASSERT_OFFSET(NCIME, hIMC, 0x1A0);
NW_ASSERT_OFFSET(NCIME, CodePage, 0x1CC);
NW_ASSERT_OFFSET(NCIME, CompAttr, 0x29D0);
NW_ASSERT_OFFSET(NCIME, ResultClause, 0x39D0);
NW_ASSERT_OFFSET(NCIME, CompStrLen, 0x41D0);
NW_ASSERT_SIZE(NCIME, 0x41F8);

// imm32.dll's input context locks, found by NCIME::LoadImmFunctions: the reading string of the
// Taiwanese and Chinese IMEs without the GetReadingString API is read in the IME's private data
// (names ours)
typedef BYTE*(WINAPI* NImmLockIMC)(HIMC);
typedef BOOL(WINAPI* NImmUnlockIMC)(HIMC);
typedef LPVOID(WINAPI* NImmLockIMCC)(HIMCC);
typedef BOOL(WINAPI* NImmUnlockIMCC)(HIMCC);
#if NW_HOOK_MODE
#define GImmLockIMC NW_GLOBAL(NImmLockIMC, 0x1011e9f8)
#define GImmUnlockIMC NW_GLOBAL(NImmUnlockIMC, 0x1011e9fc)
#define GImmLockIMCC NW_GLOBAL(NImmLockIMCC, 0x1011ea00)
#define GImmUnlockIMCC NW_GLOBAL(NImmUnlockIMCC, 0x1011ea04)
#else
extern NImmLockIMC GImmLockIMC;
extern NImmUnlockIMC GImmUnlockIMC;
extern NImmLockIMCC GImmLockIMCC;
extern NImmUnlockIMCC GImmUnlockIMCC;
#endif

// an IME's GetReadingString export (names ours)
typedef UINT(WINAPI* NGetReadingString)(HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT);

// INPUTCONTEXT::hPrivate (immdev.h, not included)
inline HIMCC IMCPrivate(BYTE* IC) { return *(HIMCC*)(IC + 0x124); }

// OSVERSIONINFO cache of NCIME::IsUnicodeOS (function statics in the stock code)
#if NW_HOOK_MODE
#define GIMEOSChecked NW_GLOBAL(UBOOL, 0x101208ec)
#define GIMEUnicodeOS NW_GLOBAL(UBOOL, 0x101208e8)
#else
extern UBOOL GIMEOSChecked;
extern UBOOL GIMEUnicodeOS;
#endif

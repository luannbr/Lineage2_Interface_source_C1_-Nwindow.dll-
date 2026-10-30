#pragma once
// NCChineseIME - the IME of the Chinese client (UClass "CChineseIME", 0x6224). The composition
// is read as the Japanese IME reads it, but only with a Chinese IME (see IsChineseIME); the
// candidate list goes into an NCCandidateBox (9 a page, 10 for the IME type 9). The IME types
// 11 and 12 (NConsoleWnd::GetIMEType; 11 is never returned) also show the letters typed in an
// NCCompositeBox, rebuilt from the keyboard state on IMN_PRIVATE, and 12 takes a candidate with
// the digit keys. The reading string (ReadingStr, NIM_READING) comes from the IME's
// GetReadingString export or, for the old Microsoft Pinyin IMEs without it, from the IME's
// private data (IMN_PRIVATE).

#include "nwindow/NCIME.h"

class NCCompositeBox;

// the IME's GetReadingString (NCChineseIME::LoadReadingString; name ours), and
// NCChineseIME::IsNT's function statics in the stock code (names ours)
#if NW_HOOK_MODE
#define GChineseGetReadingString NW_GLOBAL(NGetReadingString, 0x1011e9f4)
#define GChineseOSVersion NW_GLOBAL(OSVERSIONINFOA, 0x101208f0)
#define GChineseOSVersionRead NW_GLOBAL(bool, 0x10120984)
#define GChineseNT NW_GLOBAL(bool, 0x10120985)
#define GChineseNTChecked NW_GLOBAL(bool, 0x10120986)
#else
extern NGetReadingString GChineseGetReadingString;
extern OSVERSIONINFOA GChineseOSVersion;
extern bool GChineseOSVersionRead;
extern bool GChineseNT;
extern bool GChineseNTChecked;
#endif

class NCChineseIME : public NCIME
{
    NW_DECLARE_CLASS(NCChineseIME, NCIME, 0, 0x1011f448)

    TArray<NImeText> Clauses;        // +0x41F8 as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4204 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x4208
    NCCompositeBox* CompositeBox;    // +0x420C letters typed (IME types 11 and 12)
    FString LastComp;                // +0x4210 the composition before the last one (IME type 10;
                                     //   the edit boxes move the box's caret by it)
    BYTE Unknown421C[0x2000];        // +0x421C not used
    UBOOL bCandOpened;               // +0x621C IME type 11: a candidate list opened since the
                                     //   last key (its changes are read only then)
    UBOOL bOpenResultSent;           // +0x6220 IME type 12: an opening candidate list sent a
                                     //   result (the next opening sends none)

    NCChineseIME(INT InMode = 0);                                    // 0x10014b60
    // ~NCChineseIME                                                 // 0x10014c10

    // WindowProc: the stock body is the Japanese IME's (0x1000ee70)
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000ee70
    virtual INT OnCreate();                                          // 35 0x1000fbf0
    virtual INT OnDestroy();                                         // 36 0x10013300
    virtual void ShowWindow();                                       // 42 0x1000f090
    virtual void Deactivate();                                       // 64 0x10014530
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 65 0x10014c90
    virtual INT OnImeEndComposition(WPARAM wParam, LPARAM lParam);   // 67 0x1000f1f0
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam) { return 1; }   // 68 0x100098f0 (folded)
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 69 0x100134c0
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 70 0x100098f0 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 74 0x10010130

    // names ours
    void FreeCandList();                                             // 0x1000e6a0 (shared)
    void FillCandidateBox();                                         // 0x1000ffa0
    void ReadCandList();                                             // 0x100133b0
    void ReadReadingString();                                        // 0x1000fd60
    UBOOL IsChineseIME();                                            // 0x1000efe0 (this unused)
    static void LoadReadingString();                                 // 0x1000ef80
    static DWORD GetImeId();                                         // 0x1000f0c0 the old IMEs'
                                                                     //   version, 0 for the others
    static bool IsNT();                                              // 0x1000f390
};
NW_ASSERT_OFFSET(NCChineseIME, Clauses, 0x41F8);
NW_ASSERT_OFFSET(NCChineseIME, CompositeBox, 0x420C);
NW_ASSERT_OFFSET(NCChineseIME, LastComp, 0x4210);
NW_ASSERT_OFFSET(NCChineseIME, bCandOpened, 0x621C);
NW_ASSERT_SIZE(NCChineseIME, 0x6224);

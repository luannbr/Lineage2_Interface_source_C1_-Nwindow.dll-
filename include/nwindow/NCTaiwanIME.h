#pragma once
// NCTaiwanIME - the IME of the Taiwanese client (UClass "CTaiwanIME", 0x620C). The composition
// is read as the Korean IME reads it, but only with a Taiwanese IME (see IsTaiwanIME); the
// candidate list goes into an NCCandidateBox as the Japanese IME's does, laid out by the IME
// type the console reports. The reading string (ReadingStr, NIM_READING) comes from the IME's
// GetReadingString export or, for the old Microsoft IMEs without it, from the IME's private
// data (IMN_PRIVATE).

#include "nwindow/NCIME.h"

// the IME's GetReadingString (NCTaiwanIME::LoadReadingString; name ours)
#if NW_HOOK_MODE
#define GTaiwanGetReadingString NW_GLOBAL(NGetReadingString, 0x1011e9f0)
#else
extern NGetReadingString GTaiwanGetReadingString;
#endif

class NCTaiwanIME : public NCIME
{
    NW_DECLARE_CLASS(NCTaiwanIME, NCIME, 0, 0x101203c0)

    TArray<NImeText> Clauses;        // +0x41F8 as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4204 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x4208
    BYTE Unknown420C[0x2000];        // +0x420C not used

    NCTaiwanIME(INT InMode = 0);                                     // 0x10014860
    // ~NCTaiwanIME                                                  // 0x100148d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000e6f0
    virtual INT OnCreate();                                          // 35 0x1000f890
    virtual INT OnDestroy();                                         // 36 0x10011120 (shared)
    virtual void ShowWindow();                                       // 42 0x1000e680 (shared)
    virtual void Deactivate();                                       // 64 0x1000ed90
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 65 0x10014930
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam);       // 68 0x1000ecc0
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 69 0x10012310
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 70 0x100098f0 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 74 0x1000fad0

    // names ours
    void FreeCandList();                                             // 0x1000e6a0 (shared)
    void FillCandidateBox();                                         // 0x1000f960 (shared)
    void ReadCandList();                                             // 0x10013210
    void ReadReadingString();                                        // 0x1000ea00
    UBOOL IsTaiwanIME();                                             // 0x1000ec20 (this unused)
    static void LoadReadingString();                                 // 0x1000e7e0
    static DWORD GetImeId();                                         // 0x1000e890 the old IMEs'
                                                                     //   version, 0 for the others
};
NW_ASSERT_OFFSET(NCTaiwanIME, Clauses, 0x41F8);
NW_ASSERT_OFFSET(NCTaiwanIME, CandidateBox, 0x4208);
NW_ASSERT_SIZE(NCTaiwanIME, 0x620C);

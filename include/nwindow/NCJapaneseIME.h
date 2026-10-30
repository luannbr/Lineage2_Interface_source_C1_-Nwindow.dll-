#pragma once
// NCJapaneseIME - the IME of the Japanese client (UClass "CJapaneseIME", 0x620C). The
// composition is read as the Korean IME reads it (clauses included) but only with a Japanese
// keyboard layout, and the cursor comes from the IME only while some characters are still
// being typed (attribute 0). The IME's candidate list is copied (CandList) and shown in an
// NCCandidateBox, 9 a page, placed at the caret the edit box reports (NIM_CARET).

#include "nwindow/NCIME.h"

class NCJapaneseIME : public NCIME
{
    NW_DECLARE_CLASS(NCJapaneseIME, NCIME, 0, 0x1011ea08)

    TArray<NImeText> Clauses;        // +0x41F8 as NCKoreanIME's
    CANDIDATELIST* CandList;         // +0x4204 copy of the IME's list (ImmGetCandidateListW)
    NCCandidateBox* CandidateBox;    // +0x4208
    BYTE Unknown420C[0x2000];        // +0x420C not used

    NCJapaneseIME(INT InMode = 0);                                   // 0x10014710
    // ~NCJapaneseIME                                                // 0x10014780

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000ee70 (shared)
    virtual INT OnCreate();                                          // 35 0x1000f760
    virtual INT OnDestroy();                                         // 36 0x10011120
    virtual void ShowWindow();                                       // 42 0x1000e680
    virtual void Deactivate();                                       // 64 0x10012210
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 65 0x100147e0
    virtual INT OnImeEndComposition(WPARAM wParam, LPARAM lParam);   // 67 0x1000e6d0
    virtual INT OnImeSetContext(WPARAM wParam, LPARAM lParam) { return 1; } // 68 0x100098f0 (folded)
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 69 0x10011270
    virtual INT OnInputLangChange(WPARAM wParam, LPARAM lParam) { return 1; } // 70 0x100098f0 (folded)
    // name ours: the edit box's answer to NIM_GETCARET
    virtual INT OnCaret(INT X, INT Y);                               // 74 0x1000f820

    // names ours
    void FreeCandList();                                             // 0x1000e6a0 (shared)
    void FillCandidateBox();                                         // 0x1000f960 (shared)
    void ReadCandList();                                             // 0x100111a0
    UBOOL IsJapaneseLayout();                                        // 0x1000e660 (this unused)
};
NW_ASSERT_OFFSET(NCJapaneseIME, Clauses, 0x41F8);
NW_ASSERT_OFFSET(NCJapaneseIME, CandidateBox, 0x4208);
NW_ASSERT_SIZE(NCJapaneseIME, 0x620C);

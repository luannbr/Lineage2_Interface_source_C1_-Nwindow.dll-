#pragma once
// NCKoreanIME - the IME of Korean and English clients (UClass "CKoreanIME", 0x4204). Keeps the
// Hangul/alphanumeric mode across focus changes: while the chat line has the focus the mode is
// saved on Deactivate and restored on Activate. A composition with clause information is split
// into Clauses (Flag = target clause) for the edit box.

#include "nwindow/NCIME.h"

class NCKoreanIME : public NCIME
{
    NW_DECLARE_CLASS(NCKoreanIME, NCIME, 0, 0x1011fe90)

    TArray<NImeText> Clauses;        // +0x41F8

    NCKoreanIME(INT InMode = 0);                                     // 0x10014640
    // ~NCKoreanIME                                                  // 0x100146b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1000e230
    virtual INT OnCreate();                                          // 35 0x1000e2c0
    virtual void Activate();                                         // 63 0x1000e540
    virtual void Deactivate();                                       // 64 0x1000e5a0
    virtual INT OnImeNotify(WPARAM wParam, LPARAM lParam);           // 65 0x1000e310
    virtual INT OnImeComposition(WPARAM wParam, LPARAM lParam);      // 69 0x10010230
};
NW_ASSERT_OFFSET(NCKoreanIME, Clauses, 0x41F8);
NW_ASSERT_SIZE(NCKoreanIME, 0x4204);

#pragma once
// NCIMEManager - the IME of an edit box (UClass "CIMEManager", 0x1B4): a hidden child that
// creates the IME of the client language (GLanguageType 0/1 Korean, 2 Japanese, 3 Taiwanese,
// 4 Chinese), forwards Activate/Deactivate to it and passes the IME's private messages
// (NIM_*) on to the edit box. The getters read the IME's strings.

#include "nwindow/NCIME.h"

class NCIMEManager : public NControl
{
    NW_DECLARE_CLASS(NCIMEManager, NControl, 0, 0x1011f968)

    NCIME* KoreanIME;                // +0x1A0 NCKoreanIME
    NCIME* JapaneseIME;              // +0x1A4 NCJapaneseIME
    NCIME* TaiwanIME;                // +0x1A8 NCTaiwanIME
    NCIME* ChineseIME;               // +0x1AC NCChineseIME
    INT Mode;                        // +0x1B0 passed to the IME

    NCIMEManager(INT InMode = 0);                                    // 0x1000f400
    // ~NCIMEManager                                                 // 0x1000f490

    // WindowProc: the stock body is NCEditBox's (0x10007f80): NIM_* messages to 65..72
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10007f80
    virtual INT vfn033() { return 1; }                               // 33 0x10005230 (folded)
    virtual INT OnCreate();                                          // 35 0x100152a0
    virtual INT OnDestroy();                                         // 36 0x1000f4e0
    // names ours
    virtual void Activate();                                         // 63 0x1000dd60
    virtual void Deactivate();                                       // 64 0x1000ddb0
    virtual INT OnComposition(WPARAM wParam, LPARAM lParam);         // 65 0x1000de00
    virtual INT OnResult(WPARAM wParam, LPARAM lParam);              // 66 0x1000de20
    virtual INT OnAttribute(WPARAM wParam, LPARAM lParam);           // 67 0x1000de40
    virtual INT OnReading(WPARAM wParam, LPARAM lParam);             // 68 0x1000de60
    virtual INT On14(WPARAM wParam, LPARAM lParam) { return 1; }     // 69 0x100098f0 (folded)
    virtual INT OnGetCaret(WPARAM wParam, LPARAM lParam);            // 70 0x1000de80
    virtual INT OnEndComposition(WPARAM wParam, LPARAM lParam);      // 71 0x1000dea0
    virtual INT OnCursorPos(WPARAM wParam, LPARAM lParam);           // 72 0x1000dec0

    // names ours; NULL (-1 for IsNotComposing) without an IME
    TCHAR* GetCompStr();                                             // 0x1000f5f0
    TCHAR* GetResultStr();                                           // 0x1000f630
    TCHAR* GetReadingStr();                                          // 0x1000f670
    UBOOL IsNotComposing();                                          // 0x1000f6b0

    // the first IME created (inlined in the getters)
    NCIME* GetIME()
    {
        if (KoreanIME)
            return KoreanIME;
        if (JapaneseIME)
            return JapaneseIME;
        if (TaiwanIME)
            return TaiwanIME;
        return ChineseIME;
    }
};
NW_ASSERT_OFFSET(NCIMEManager, KoreanIME, 0x1A0);
NW_ASSERT_SIZE(NCIMEManager, 0x1B4);

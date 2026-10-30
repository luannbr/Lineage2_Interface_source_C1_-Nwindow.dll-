#pragma once
// NCCompositeBox - the IME composition window (UClass "CCompositeBox", 0xB0): the string
// being composed, drawn one character at a time from FirstChar until about 100 px, with a
// "|" caret before the character at CaretPos while bShowCaret is set. Filled by the IMEs.

#include "nwindow/NCWnd.h"

class NCCompositeBox : public NCWnd
{
    NW_DECLARE_CLASS(NCCompositeBox, NCWnd, 0, 0x10118c20)

    UTexture* BackTex;         // +0x94 NWindow.BlackTexture
    FString Text;              // +0x98 composition string
    INT FirstChar;             // +0xA4 first character drawn
    INT CaretPos;              // +0xA8
    UBOOL bShowCaret;          // +0xAC

    NCCompositeBox();                                                // 0x10003380
    // the Chinese IME's: size and place given (BackTex is left as allocated)
    NCCompositeBox(INT W, INT H, INT X, INT Y, UBOOL bInShowCaret);  // 0x10003490
    // ~NCCompositeBox                                               // 0x10003420

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (NControl's)
    virtual INT OnCreate();                                          // 35 0x100029e0
    virtual INT OnDestroy();                                         // 36 0x10083930 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10003520

    // names ours; the text scrolls so that the caret stays within 97 px
    void CaretLeft();                                                // 0x10002a10
    void CaretRight();                                               // 0x100036f0
    void SetText(const TCHAR* InText);                               // 0x10003860
    FString GetText();                                               // 0x10002a40 a copy
    void SetShowCaret(UBOOL bShow);                                  // 0x10002a70
};
NW_ASSERT_OFFSET(NCCompositeBox, Text, 0x98);
NW_ASSERT_SIZE(NCCompositeBox, 0xB0);

#pragma once
// NCEditBox - single line text input (UClass "CEditBox", 0x238): the chat line, login fields,
// amounts in dialogs... The text is kept in pieces around the caret: Left (before it), Comp
// (the IME composition, drawn at the caret) and Right (after it); arrows move characters
// between Left and Right. The text may grow ExtraWidth px past the box (0: the box width)
// and scrolls from ViewStart. A hidden NCIMEManager child gives it an IME, whose private
// messages (NIM_*) arrive in 65..72.
//
// Type: 0 plain, 1 chat (" first and Up/Down are reported to the parent), 2 password (drawn
// as '*'), 3 digits only. The parent gets NWM_COMMAND: 0 Enter on an empty box, 1 Enter,
// 3 '"' typed first, 4 Up, 5 Down (chat).

#include "nwindow/NControl.h"

class NCIMEManager;

class NCEditBox : public NControl
{
    NW_DECLARE_CLASS(NCEditBox, NControl, 0, 0x1011b550)

    NCIMEManager* IMEManager;        // +0x1A0
    UTexture* Edit1;                 // +0x1A4 focused frame: left, middle, right
    UTexture* Edit2;                 // +0x1A8
    UTexture* Edit3;                 // +0x1AC
    UTexture* EditDisable1;          // +0x1B0 frame without the focus
    UTexture* EditDisable2;          // +0x1B4
    UTexture* EditDisable3;          // +0x1B8
    INT Type;                        // +0x1BC see above
    UBOOL bCanInsert;                // +0x1C0 the text is narrower than MaxWidth - 12
    UBOOL bCaretOn;                  // +0x1C4 caret blink phase
    INT CaretTimer;                  // +0x1C8 SetTimer id (500 ms)
    UTexture* BackTex;               // +0x1CC constructor argument (not drawn here)
    UTexture* WhiteTex;              // +0x1D0 composing character background
    UBOOL bIMEPosFixed;              // +0x1D4 constructor argument: IME window placement
    UBOOL bReading;                  // +0x1D8 a reading string is shown (NIM_READING)
    INT Unknown1DC;                  // +0x1DC
    INT ViewStart;                   // +0x1E0 first character of Left drawn
    INT ExtraWidth;                  // +0x1E4 px the text may grow past the box
    INT MaxWidth;                    // +0x1E8 Width + ExtraWidth (OnPaint)
    INT ViewStart2;                  // +0x1EC copy of ViewStart restored by End
    FString Left;                    // +0x1F0
    FString Right;                   // +0x1FC
    FString Comp;                    // +0x208
    FString Reading;                 // +0x214
    DWORD CaretX;                    // +0x220 caret position in the box (OnPaint)
    INT Unknown224;                  // +0x224
    INT IMECaretX;                   // +0x228 answer to NIM_GETCARET
    INT IMECaretY;                   // +0x22C
    INT CursorPos;                   // +0x230 NIM_CURSORPOS
    INT ReadingParam;                // +0x234 lParam of NIM_READING

    NCEditBox(INT InType = 0, const TCHAR* BackTexName = NULL, UBOOL bInIMEPosFixed = 1,
              INT InExtraWidth = 0);                                 // 0x10009a60
    // ~NCEditBox                                                    // 0x10009be0

    // WindowProc: the stock body is shared with NCIMEManager (hooked there)
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10007f80
    virtual void SetFocus();                                         // 28 0x10009140
    virtual void KillFocus();                                        // 29 0x10009160
    virtual INT vfn032() { return 1; }                               // 32 0x10005230 (folded)
    virtual INT OnTimer(INT TimerID);                                // 34 0x10009100
    virtual INT OnCreate();                                          // 35 0x10009c80
    virtual INT OnDestroy();                                         // 36 0x10009e70
    virtual void ShowWindow();                                       // 42 0x100a5d70 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1000b370
    virtual INT OnChar(WPARAM wParam, LPARAM lParam);                // 52 0x10008f50
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x1000abe0
    // names ours
    virtual const TCHAR* CommitText();                               // 63 0x10008bf0
    virtual void ClearText();                                        // 64 0x10008f10
    virtual INT OnComposition(WPARAM wParam, LPARAM lParam);         // 65 0x10009440
    virtual INT OnResult(WPARAM wParam, LPARAM lParam);              // 66 0x100091e0
    virtual INT OnAttribute(WPARAM wParam, LPARAM lParam);           // 67 0x10009700
    virtual INT OnReading(WPARAM wParam, LPARAM lParam);             // 68 0x10009820
    virtual INT On14(WPARAM wParam, LPARAM lParam) { return 1; }     // 69 0x100098f0 (folded)
    virtual INT OnGetCaret(WPARAM wParam, LPARAM lParam);            // 70 0x10009900
    virtual INT OnEndComposition(WPARAM wParam, LPARAM lParam);      // 71 0x10009930
    virtual INT OnCursorPos(WPARAM wParam, LPARAM lParam);           // 72 0x10009960

    // names ours
    void SetText(const TCHAR* InText);                               // 0x10008be0 Left only
    void InsertText(const TCHAR* InText, UBOOL bAppend);             // 0x10008c40
    void DeleteChar(INT Index);                                      // 0x10008d30
    INT GetTextWidth(FString Text);                                  // 0x10008ea0
    void SetType(INT InType);                                        // 0x10009130
    INT GetCompCaretX();                                             // 0x10009980 (Japanese)

    // composition drawing of the Korean client (names ours)
    void PaintComposingKR(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x100080d0
    void PaintCompositionKR(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device); // 0x100087d0
    // the same for the other IMEs (names ours)
    void PaintComposingTW(INT X, INT Y, INT TextW, INT CompW, INT ClauseW, INT ReadingW,
                          NCGDevice* Device);                                        // 0x10008180
    void PaintCompositionJP(INT X, INT Y, INT TextW, INT Offset, NCGDevice* Device); // 0x100085b0
    void PaintCompositionTW(INT X, INT Y, INT TextW, INT* ClauseW, NCGDevice* Device); // 0x10008980
    void PaintComposingJP(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x10009ed0
    void PaintComposingCN(INT X, INT Y, INT TextW, INT CompW, NCGDevice* Device);   // 0x1000a180
    void PaintCompositionCN(INT X, INT Y, INT TextW, NCGDevice* Device);            // 0x1000a7f0
};
NW_ASSERT_OFFSET(NCEditBox, IMEManager, 0x1A0);
NW_ASSERT_OFFSET(NCEditBox, Type, 0x1BC);
NW_ASSERT_OFFSET(NCEditBox, ViewStart, 0x1E0);
NW_ASSERT_OFFSET(NCEditBox, Left, 0x1F0);
NW_ASSERT_OFFSET(NCEditBox, Reading, 0x214);
NW_ASSERT_OFFSET(NCEditBox, CaretX, 0x220);
NW_ASSERT_SIZE(NCEditBox, 0x238);

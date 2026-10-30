#pragma once
// NCMultiEdit - text box of several lines with word wrap (UClass "CMultiEdit", 0x264): the memo
// of the party matching options, and (as NCHtmlMultiEdit) the <multiedit> boxes of the HTML
// pages. The text is a list of logical lines (NLineList, a line ends with its '\n'); each line
// is wrapped into screen rows at Width - 36 px, TopLine is the first row shown. The caret has a
// logical position (CaretLine, CaretCol) and a screen one (CaretRow, CaretX). The scroll
// buttons and the scroll bar sit outside the box, at x = LocalW + 2. A hidden NCIMEManager child
// gives it an IME, whose private messages (NIM_*) arrive in 67..71; the composition is kept in
// Comp and drawn at the caret, by the painter of the client language (Korean here, the other
// ones not decompiled yet).

#include "nwindow/NControl.h"

class NCIMEManager;
class NCPushButton;
class NCSliderBar;
class NCHtmlPushButton;
class NCHtmlSliderBar;
class NString;

// NWM_COMMAND ids of the scroll controls (names ours)
enum
{
    NID_MULTIEDIT_UP = (INT)0xF0F01409,
    NID_MULTIEDIT_DOWN = (INT)0xF0F01410,
    NID_MULTIEDIT_BAR = (INT)0xF0F01411,
};

// NLineNode - one logical line of NCMultiEdit (0x14 bytes, allocated with the global new; name
// ours). The text is at the start, so a node pointer is used as an FString*.
struct NLineNode
{
    FString Text;              // +0x00
    NLineNode* Prev;           // +0x0C
    NLineNode* Next;           // +0x10

    NLineNode();                                                     // 0x1001b390
    ~NLineNode();                                                    // 0x1001b3e0
};
NW_ASSERT_SIZE(NLineNode, 0x14);

// NLineList - the lines of NCMultiEdit (0x18 bytes, name ours): a doubly linked list with a
// cached cursor (Current, CurIndex) that GetLine walks from, whichever end is nearest.
struct NLineList
{
    INT Count;                 // +0x00
    INT Unknown4;              // +0x04 (0)
    NLineNode* Head;           // +0x08
    NLineNode* Current;        // +0x0C cursor of GetLine
    NLineNode* Tail;           // +0x10
    INT CurIndex;              // +0x14 index of Current

    NLineList();                                                     // 0x1001bc20 one empty line
    ~NLineList();                                                    // 0x10020a20

    // the getter is a folded body the other classes share (0x10033ac0, hooked as NActionItem's)
    INT Num() { return Count; }
    NLineNode* GetLine(INT Index);                                   // 0x1001bd10
    INT TotalLength();                                               // 0x1001bca0 characters
    INT Insert(INT Index);                                           // 0x1001de60 -1 = at the end
    void RemoveAt(INT Index);                                        // 0x1001e000
    void Clear();                                                    // 0x1001e0c0

    // cursor steps (inlined in the stock code); NULL at an end
    NLineNode* StepBack()
    {
        if (Current && Current->Prev)
        {
            Current = Current->Prev;
            if (CurIndex > 0)
                CurIndex--;
            return Current;
        }
        return NULL;
    }
    NLineNode* StepForward()
    {
        if (Current && Current->Next)
        {
            Current = Current->Next;
            if (CurIndex < Count - 1)
                CurIndex++;
            return Current;
        }
        return NULL;
    }
};
NW_ASSERT_SIZE(NLineList, 0x18);

// NStringArray - growable array of NString (0xC bytes, name ours), grown 128 at a time. It
// starts with one empty string and nothing adds to it: only NCHtmlMultiEdit's scrolling reads
// its count.
struct NStringArray
{
    INT Num;                   // +0x00
    NString* Data;             // +0x04 new NString[Max]
    INT Max;                   // +0x08

    NStringArray();                                                  // 0x10022130
    ~NStringArray();                                                 // 0x10021f70 (jmp Empty)

    INT Empty();                                                     // 0x10021f00 returns 0
    void InsertAt(INT Index);                                        // 0x10021f80 -1 = at the end
};
NW_ASSERT_SIZE(NStringArray, 0xC);

class NCMultiEdit : public NControl
{
    NW_DECLARE_CLASS(NCMultiEdit, NControl, 0, 0x10122360)

    NCIMEManager* IMEManager;        // +0x1A0
    UTexture* BackTex;               // +0x1A4 "NWindow.chatback", stretched over the box
    UTexture* WhiteTex;              // +0x1A8 "NWindow.WhiteTexture", composing character
    NStringArray Strings;            // +0x1AC see above
    NLineList Lines;                 // +0x1B8
    FString Comp;                    // +0x1D0 IME composition, drawn at the caret
    TCHAR* TextBuf;                  // +0x1DC GetText's result (new TCHAR[])
    INT CharH;                       // +0x1E0 height of the text
    INT LineGap;                     // +0x1E4 2: between the rows, and the text's left margin
    INT CaretTimer;                  // +0x1E8 SetTimer id (0xFA0D05, 500 ms)
    INT CaretLine;                   // +0x1EC logical line of the caret
    INT CaretCol;                    // +0x1F0 character of the caret in its line
    UBOOL bCaretOn;                  // +0x1F4 caret blink phase
    INT VisibleRows;                 // +0x1F8 (Height - 2) / (CharH + 2)
    INT TopLine;                     // +0x1FC first screen row shown (scroll position)
    INT NumLines;                    // +0x200 copy of Lines.Count
    INT CaretRow;                    // +0x204 screen row of the caret
    INT CaretX;                      // +0x208 px of the caret in its row
    UBOOL bCompRow;                  // +0x20C GetRowStart/GetRowEnd: the row breaks inside the
                                     //   composition
    INT Unknown210;                  // +0x210 RecalcRows's argument
    _WORD Unknown214;                // +0x214 (0)
    _WORD Unknown216;                // +0x216 (0)
    INT CharW;                       // +0x218 width of a Hangul character
    INT CharH2;                      // +0x21C CharH again
    INT Unknown220;                  // +0x220 (0)
    NCPushButton* UpButton;          // +0x224 0xF0F01409
    NCPushButton* DownButton;        // +0x228 0xF0F01410
    NCSliderBar* ScrollBar;          // +0x22C 0xF0F01411
    INT IMECaretX;                   // +0x230 answer to NIM_GETCARET
    INT IMECaretY;                   // +0x234
    UBOOL bReading;                  // +0x238 a reading string is shown (NIM_READING)
    FString Reading;                 // +0x23C
    INT ReadingParam;                // +0x248 lParam of NIM_READING
    INT Unknown24C;                  // +0x24C (1)
    INT Unknown250;                  // +0x250 (1)
    INT Unknown254;                  // +0x254 (0)
    INT Unknown258;                  // +0x258 (0)
    INT Unknown25C;                  // +0x25C (16)
    INT TotalRows;                   // +0x260 screen rows of the whole text

    NCMultiEdit();                                                   // 0x10020b30
    // ~NCMultiEdit                                                  // 0x10020a90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1001a440
    virtual void SetFocus();                                         // 28 0x10009140 (shared)
    virtual void KillFocus();                                        // 29 0x1001a5e0
    virtual INT vfn032() { return 1; }                               // 32 0x10005230 (folded)
    virtual INT OnTimer(INT TimerID);                                // 34 0x1001a600
    virtual INT OnCreate();                                          // 35 0x1001cd00
    virtual INT OnDestroy();                                         // 36 0x1001e550
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1001e700
    virtual INT OnChar(WPARAM wParam, LPARAM lParam);                // 52 0x1001fb60
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x1001fbc0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x1001ba00
    // names ours
    virtual void UpdateScrollBar();                                  // 63 0x1001bbe0
    virtual INT OnScrollUp();                                        // 64 0x1001bad0
    virtual INT OnScrollDown();                                      // 65 0x1001bb10
    virtual INT OnScrollBar(INT Value);                              // 66 0x1001b360
    virtual INT OnComposition(WPARAM wParam, LPARAM lParam);         // 67 0x1001dc20
    virtual INT OnResult(WPARAM wParam, LPARAM lParam);              // 68 0x1001dbd0
    virtual INT OnGetCaret(WPARAM wParam, LPARAM lParam);            // 69 0x1001b2c0
    virtual INT OnReading(WPARAM wParam, LPARAM lParam);             // 70 0x1001b2f0
    virtual INT OnAttribute(WPARAM wParam, LPARAM lParam);           // 71 0x1001b340

    // names ours
    void ScrollToCaret();                                            // 0x1001a5a0
    void SetRepeatDelay(INT Delay);                                  // 0x1001bb70
    void InitScrollBar();                                            // 0x1001bba0
    INT CountRows(INT Line);                                         // 0x1001be80
    void UpdateRowCount(INT Line, INT OldRows, UBOOL bComp);         // 0x1001c120
    void RecalcRows(INT Param);                                      // 0x1001c3d0
    void InsertString(TCHAR* Str, UBOOL bResult);                    // 0x1001c9e0
    void SetCaretX(INT Line, INT Col);                               // 0x1001d040
    INT GetRowStart(INT Row);                                        // 0x1001d150
    INT GetRowEnd(INT Start, INT Line);                              // 0x1001d4c0
    INT GetRowLine(INT Row);                                         // 0x1001d8e0
    TCHAR* GetText();                                                // 0x1001dd60
    void InsertChar(TCHAR Ch);                                       // 0x1001e0f0
    void SetText(const TCHAR* Text);                                 // 0x1001e500
    void Clear();                                                    // 0x1001e670

    // composition painters of the client languages (names ours): Korean, Japanese, Taiwanese
    // and Chinese
    void PaintCompositionKR(FString Text, INT X, INT Y, NCGDevice* Device);           // 0x1001b1a0
    void PaintCompositionJP(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001a620
    void PaintCompositionTW(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001aad0
    void PaintCompositionCN(FString Text, INT CompStart, INT X, INT Y, INT RowNum,
                            NCGDevice* Device, INT* Out);                               // 0x1001b430
};
NW_ASSERT_OFFSET(NCMultiEdit, Strings, 0x1AC);
NW_ASSERT_OFFSET(NCMultiEdit, Lines, 0x1B8);
NW_ASSERT_OFFSET(NCMultiEdit, Comp, 0x1D0);
NW_ASSERT_OFFSET(NCMultiEdit, CaretLine, 0x1EC);
NW_ASSERT_OFFSET(NCMultiEdit, CharW, 0x218);
NW_ASSERT_OFFSET(NCMultiEdit, Reading, 0x23C);
NW_ASSERT_OFFSET(NCMultiEdit, TotalRows, 0x260);
NW_ASSERT_SIZE(NCMultiEdit, 0x264);

// NCHtmlMultiEdit - the <multiedit> box of the HTML pages (UClass "CHtmlMultiEdit", 0x270): its
// own scroll controls (NCHtmlPushButton, NCHtmlSliderBar) inside the box's right edge, clipped
// like the page's other controls; the value is the text. Its scrolling counts Strings instead of
// the lines (always 1): the scroll buttons do nothing and scrolling down goes back to the top.
class NCHtmlMultiEdit : public NCMultiEdit
{
    NW_DECLARE_CLASS(NCHtmlMultiEdit, NCMultiEdit, 0, 0x10127a10)

    NCHtmlPushButton* HtmlUpButton;  // +0x264 NID_MULTIEDIT_UP
    NCHtmlPushButton* HtmlDownButton; // +0x268 NID_MULTIEDIT_DOWN
    NCHtmlSliderBar* HtmlScrollBar;  // +0x26C NID_MULTIEDIT_BAR

    NCHtmlMultiEdit();                                               // 0x10030ff0
    // ~NCHtmlMultiEdit                                              // 0x10031050

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1002ff20
    virtual INT OnCreate();                                          // 35 0x10033540
    virtual INT OnDestroy();                                         // 36 0x100310c0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100311b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100310a0 (shared)
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x1002ff70
    virtual const TCHAR* GetValue();                                 // 62 0x1002ff60
    virtual void UpdateScrollBar();                                  // 63 0x10030120
    virtual INT OnScrollUp();                                        // 64 0x10030060
    virtual INT OnScrollDown();                                      // 65 0x100300b0
};
NW_ASSERT_OFFSET(NCHtmlMultiEdit, HtmlUpButton, 0x264);
NW_ASSERT_SIZE(NCHtmlMultiEdit, 0x270);

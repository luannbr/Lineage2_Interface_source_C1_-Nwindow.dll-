#pragma once
// NCCandidateBox - the IME candidate list (UClass "CCandidateBox", 0xFC): pages of PageSize
// candidates, RowHeight px each, the selected one on a white bar. The frame textures come in
// three layouts picked by Mode. Up/Down (keys or the buttons) move the selection a page at a
// time when crossing a page edge; PageUp/PageDown show or hide the box. Filled by the IMEs.

#include "nwindow/NCWnd.h"

class NCPushButton;
class NCSliderBar;

// a string of the IMEs (names ours; 0x804 bytes): a candidate of NCCandidateBox, a clause of
// the Korean composition (Flag = target clause)
struct NImeText
{
    TCHAR Text[1024];          // +0x000
    INT Flag;                  // +0x800
};
NW_ASSERT_SIZE(NImeText, 0x804);

// NWM_COMMAND ids of the box's own controls
enum
{
    NID_CANDIDATE_SCROLLBAR = (INT)0xF0F01415,
    NID_CANDIDATE_UP = 0xF01416,
    NID_CANDIDATE_DOWN = 0xF01417,
};

class NCCandidateBox : public NCWnd
{
    NW_DECLARE_CLASS(NCCandidateBox, NCWnd, 0, 0x101186f8)

    INT PageSize;                    // +0x94 candidates per page (9)
    UTexture* LeftEnable1;           // +0x98 frame, modes 0..2
    UTexture* LeftDisable1;          // +0x9C   (rows without a candidate)
    UTexture* LeftEnable2;           // +0xA0 frame, modes 3..5
    UTexture* LeftDisable2;          // +0xA4
    UTexture* Center1;               // +0xA8
    UTexture* Center2;               // +0xAC
    UTexture* Right1;                // +0xB0
    UTexture* Right2;                // +0xB4
    UTexture* WhiteTex;              // +0xB8 selection bar
    INT RowHeight;                   // +0xBC (15)
    TArray<NImeText> Items;          // +0xC0
    INT Selected;                    // +0xCC
    INT PageStart;                   // +0xD0 first candidate of the page shown
    INT Count;                       // +0xD4 candidates
    INT MarginX;                     // +0xD8 hit test rectangle of the rows
    INT MarginY;                     // +0xDC
    INT UnknownE0;                   // +0xE0
    DWORD TextColor;                 // +0xE4 selected candidate (black)
    INT UnknownE8;                   // +0xE8
    NCPushButton* UpButton;          // +0xEC
    NCPushButton* DownButton;        // +0xF0
    NCSliderBar* ScrollBar;          // +0xF4
    INT Mode;                        // +0xF8 frame layout: 0..2, 3..4, 5

    NCCandidateBox(INT InPageSize = 9, INT InRowHeight = 15);        // 0x10003c50
    // ~NCCandidateBox                                               // 0x10003cf0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100025a0
    virtual INT OnCreate();                                          // 35 0x10002ac0
    virtual INT OnDestroy();                                         // 36 0x10003b00
    virtual UBOOL PtInRect(INT X, INT Y, INT RectX, INT RectY, INT RectW, INT RectH); // 40 0x10002840
    virtual void SetSize(INT W, INT H);                              // 41 0x10002880
    virtual void ShowWindow();                                       // 42 0x100029a0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10002d90
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x10002710
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10003270
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10002800
    // names ours
    virtual INT OnUpButton();                                        // 62 0x100027c0
    virtual INT OnDownButton();                                      // 63 0x100027e0
    virtual INT OnScrollBar(INT Pos) { return 1; }                   // 64 0x10002830 (folded)

    void SelectPrev();                                               // 0x10002690 (name ours)
    void SelectNext();                                               // 0x100026e0 (name ours)
    // the IMEs fill it with these (names ours)
    void SetScroll(INT Total, INT Value);                            // 0x10002930 scroll bar, 9 a page
    void SetMode(INT InMode);                                        // 0x10002960 and the height
    void AddItem(const TCHAR* Text, INT Flag);                       // 0x10003a50 Flag not stored (0)
    void Clear();                                                    // 0x10003ad0
};
NW_ASSERT_OFFSET(NCCandidateBox, Items, 0xC0);
NW_ASSERT_OFFSET(NCCandidateBox, TextColor, 0xE4);
NW_ASSERT_SIZE(NCCandidateBox, 0xFC);

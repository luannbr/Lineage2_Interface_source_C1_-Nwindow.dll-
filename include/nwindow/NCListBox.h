#pragma once
// NCListBox - scrolling list of colored text lines (UClass "CListBox", 0x1E8) with scroll up /
// down buttons and a slider on the right. At most MaxLines are kept (the oldest goes first);
// a click selects a line and reports its index (NWM_COMMAND, lParam = index).

#include "nwindow/NControl.h"

class NString;
class NCPushButton;
class NCSliderBar;

struct NListItem
{
    NString* Text;             // Color = line color
    INT Param;
};

// NWM_COMMAND ids of the list's own controls
enum
{
    NID_LIST_SCROLLUP = (INT)0xF0F01406,
    NID_LIST_SCROLLDOWN = (INT)0xF0F01407,
    NID_LIST_SCROLLBAR = (INT)0xF0F01408,
};

class NCListBox : public NControl
{
    NW_DECLARE_CLASS(NCListBox, NControl, 0, 0x101213f0)

    UTexture* BackTex;               // +0x1A0 set by the owner
    UTexture* HighlightTex;          // +0x1A4 "NWindow.MenuHighlight" (not drawn here)
    TArray<NListItem> Items;         // +0x1A8
    NCPushButton* ScrollUpButton;    // +0x1B4
    NCPushButton* ScrollDownButton;  // +0x1B8
    NCSliderBar* ScrollBar;          // +0x1BC
    INT VisibleLines;                // +0x1C0 (10)
    INT MaxLines;                    // +0x1C4 (100)
    INT TopIndex;                    // +0x1C8 first line shown
    INT NumLines;                    // +0x1CC
    INT Selected;                    // +0x1D0 -1 = none
    INT Unknown1D4;                  // +0x1D4 (12)
    INT TextX;                       // +0x1D8
    INT TextY;                       // +0x1DC
    INT LineGap;                     // +0x1E0 lines are 12 + LineGap high
    UBOOL bAutoScroll;               // +0x1E4 keep the last line in view

    NCListBox();                                                     // 0x100184f0
    NCListBox(INT InMaxLines, INT InVisibleLines, INT Unused);       // 0x10018630 (3rd never read)
    // ~NCListBox                                                    // 0x100185d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10017890
    virtual INT OnCreate();                                          // 35 0x10017f30
    virtual INT OnDestroy();                                         // 36 0x10018170
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10017c30
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x10017a50
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x10017a90
    // names ours
    virtual INT HitTest(INT X, INT Y);                               // 63 0x10017980
    virtual void AddString(const TCHAR* Text, DWORD Color, INT Param); // 64 0x10018310
    virtual void Clear();                                            // 65 0x10018270
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam) { return 1; } // 66 0x100098f0 (folded)
    virtual INT ScrollUp();                                          // 67 0x10017b40
    virtual INT ScrollDown();                                        // 68 0x10017b80
    virtual INT SetTopIndex(INT Index);                              // 69 0x10017bd0

    void UpdateScrollBar();                                          // 0x10017bf0
    void SetRepeatDelay(INT Delay);                                  // 0x10017e20
    void InitScrollBar();                                            // 0x10017e50
    INT GetItemParam(INT Index);                                     // 0x10017e90
};
NW_ASSERT_OFFSET(NCListBox, Items, 0x1A8);
NW_ASSERT_OFFSET(NCListBox, ScrollBar, 0x1BC);
NW_ASSERT_OFFSET(NCListBox, Selected, 0x1D0);
NW_ASSERT_OFFSET(NCListBox, bAutoScroll, 0x1E4);
NW_ASSERT_SIZE(NCListBox, 0x1E8);

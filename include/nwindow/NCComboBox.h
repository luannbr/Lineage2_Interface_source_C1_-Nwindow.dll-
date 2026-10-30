#pragma once
// NCComboBox - drop-down list (UClass "CComboBox", 0x1F8). Closed, it shows the selected item
// and a drop button (id 0x100B); open, the control itself grows to hold the item list and
// a click picks the item under the mouse (NWM_COMMAND, lParam = index). Losing the focus
// closes it. Row height = height of L"\xAC00" (Hangul GA) in the default font.

#include "nwindow/NControl.h"

class NString;
class NCButton;

enum { NID_COMBO_DROP = 0x100B };

class NCComboBox : public NControl
{
    NW_DECLARE_CLASS(NCComboBox, NControl, 0, 0x10119b98)

    INT TextX;                       // +0x1A0 (5)
    INT TextY;                       // +0x1A4 (4) also the row spacing of the open list
    TArray<NString*> Items;          // +0x1A8
    UTexture* LeftTex;               // +0x1B4 "L2UI.Control.Combobox1"
    UTexture* MidTex;                // +0x1B8 "L2UI.Control.Combobox2"
    UTexture* LeftDisabledTex;       // +0x1BC "..._unable"
    UTexture* MidDisabledTex;        // +0x1C0
    UTexture* ListLeftTex;           // +0x1C4 "L2UI.Control.Combobox_under1_1"
    UTexture* ListMidTex;            // +0x1C8 "..._under1_2"
    UTexture* ListRightTex;          // +0x1CC "..._under1_3"
    UTexture* ListBottomTex;         // +0x1D0 "L2UI.Control.Combobox_under2"
    UTexture* HighlightTex;          // +0x1D4 "NWindow.MenuHighlight"
    NCButton* DropButton;            // +0x1D8
    UBOOL bDropped;                  // +0x1DC
    INT ClosedW;                     // +0x1E0 size when closed (OnCreate)
    INT ClosedH;                     // +0x1E4
    SIZE RowSize;                    // +0x1E8 extent of L"\xAC00": cy is the row height
    INT Selected;                    // +0x1F0 -1 = none
    INT Hover;                       // +0x1F4 item under the mouse in the open list

    NCComboBox();                                                    // 0x10004d70
    // ~NCComboBox                                                   // 0x10004e20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004300
    virtual void KillFocus();                                        // 29 0x10004450
    virtual INT OnCreate();                                          // 35 0x10004500
    virtual INT OnDestroy();                                         // 36 0x10004a80
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100046a0
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x10004370
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100043e0
    virtual INT DropDown();                                          // 63 0x10004a50 (name ours)

    // names ours
    void SetSelected(INT Index);                                     // 0x10004490
    void UpdateSize();                                               // 0x100044a0
    const TCHAR* GetItemText(INT Index);                             // 0x10004a20
    void ClearItems();                                               // 0x10004b20
    void AddItem(INT SysStringID);                                   // 0x10004b90
    void AddItem(const TCHAR* Text);                                 // 0x10004c50
};
NW_ASSERT_OFFSET(NCComboBox, Items, 0x1A8);
NW_ASSERT_OFFSET(NCComboBox, DropButton, 0x1D8);
NW_ASSERT_OFFSET(NCComboBox, RowSize, 0x1E8);
NW_ASSERT_OFFSET(NCComboBox, Hover, 0x1F4);
NW_ASSERT_SIZE(NCComboBox, 0x1F8);

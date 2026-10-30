#pragma once
// NCCheckBox - check box with a label (UClass "CCheckBox", 0x2C4). A click toggles bChecked
// and reports it to the parent (NWM_COMMAND, lParam = new state), unless bDisabled.

#include "nwindow/NControl.h"

class NCCheckBox : public NControl
{
    NW_DECLARE_CLASS(NCCheckBox, NControl, 0, 0x10119140)

    UTexture* CheckedTex;            // +0x1A0 "L2UI.Control.CheckBox_Checked"
    UTexture* UncheckedTex;          // +0x1A4 "L2UI.Control.CheckBox"
    UTexture* UncheckedDisabledTex;  // +0x1A8 "L2UI.Control.CheckBox_Unable"
    UTexture* CheckedDisabledTex;    // +0x1AC "L2UI.Control.CheckBox_Checked_unable"
    TCHAR Label[128];                // +0x1B0
    UBOOL bChecked;                  // +0x2B0
    UBOOL bDisabled;                 // +0x2B4 grey, clicks ignored
    UBOOL bLabelLeft;                // +0x2B8 label first, box after it
    INT BoxW;                        // +0x2BC (12)
    INT BoxH;                        // +0x2C0 (12)

    NCCheckBox(const TCHAR* InLabel = TEXT("default"), UBOOL bInChecked = 0,
               UBOOL bInLabelLeft = 0);                                  // 0x10003df0
    NCCheckBox(INT LabelID, UBOOL bInChecked, UBOOL bInLabelLeft);           // 0x10003f00 SysString
    // ~NCCheckBox                                                       // 0x10003eb0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);      // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                              // 35 0x10003d70
    virtual INT OnDestroy();                                             // 36 0x100189e0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                              // 50 0x10004000
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);             // 56 0x10003dc0
};
NW_ASSERT_OFFSET(NCCheckBox, Label, 0x1B0);
NW_ASSERT_OFFSET(NCCheckBox, bChecked, 0x2B0);
NW_ASSERT_OFFSET(NCCheckBox, BoxH, 0x2C0);
NW_ASSERT_SIZE(NCCheckBox, 0x2C4);

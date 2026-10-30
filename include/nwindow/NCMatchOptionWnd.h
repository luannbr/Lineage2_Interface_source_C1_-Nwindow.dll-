#pragma once
// NCMatchOptionWnd - party matching options (UClass "CMatchOptionWnd", 0xBC): automatic
// registration, level open, class open and a memo, kept per character in MatchOption.ini
// ([character name] autoregister / levelopen / classopen / memo) and sent to the server on
// OK. Showing the window loads them back. The desktop (NCConsole +0xF0) owns it.

#include "nwindow/NControl.h"

class NCCheckBox;
class NCButton;
class NCMultiEdit;

class NCMatchOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMatchOptionWnd, NCWnd, 0, 0x10139b68)

    UTexture* BackTex;               // +0x94 "L2UI.MatchWnd.option"
    NCCheckBox* LevelOpenCheck;      // +0x98
    NCCheckBox* ClassOpenCheck;      // +0x9C
    NCCheckBox* AutoRegisterCheck;   // +0xA0
    NCMultiEdit* MemoEdit;           // +0xA4
    NCButton* OKButton;              // +0xA8
    NCButton* CancelButton;          // +0xAC
    UBOOL bAutoRegister;             // +0xB0
    UBOOL bLevelOpen;                // +0xB4
    UBOOL bClassOpen;                // +0xB8

    NCMatchOptionWnd();                                              // 0x10074600
    // ~NCMatchOptionWnd                                             // 0x10074670

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10073d30
    virtual INT OnCreate();                                          // 35 0x100746c0
    virtual INT OnDestroy();                                         // 36 0x10074ad0
    virtual void ShowWindow();                                       // 42 0x10075c80
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10074bf0
    // names ours
    virtual INT OnOK();                                              // 62 0x10074cd0
    virtual INT OnCancel();                                          // 63 0x10059d00 (folded)

    void LoadOptions();                                              // 0x10074ec0 (name ours)
};
NW_ASSERT_OFFSET(NCMatchOptionWnd, MemoEdit, 0xA4);
NW_ASSERT_OFFSET(NCMatchOptionWnd, bAutoRegister, 0xB0);
NW_ASSERT_SIZE(NCMatchOptionWnd, 0xBC);

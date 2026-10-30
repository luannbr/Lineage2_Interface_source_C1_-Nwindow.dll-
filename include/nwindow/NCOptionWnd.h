#pragma once
// NCOptionWnd - the options window (UClass "COptionWnd", 0xB8; system menu > Options): three
// tab pages (video, audio, game) and OK / Cancel / Apply. OK and Apply make every page apply
// its settings and mark the first run done (Option.ini [FirstRun]); OK and Cancel close it.

#include "nwindow/NCWnd.h"

class NCButton;
class NCTabControl;
class NCVideoOptionWnd;
class NCAudioOptionWnd;
class NCGameOptionWnd;

// NWM_COMMAND ids of the window's controls
enum
{
    NID_OPTION_FRAME = 0xF001,
    NID_OPTION_OK = 0xF002,
    NID_OPTION_CANCEL = 0xF003,
    NID_OPTION_APPLY = 0xF004,
    NID_OPTION_TABS = 0xF006,
};

class NCOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCOptionWnd, NCWnd, 0, 0x1013e8a8)

    UTexture* BackTex;                     // +0x94 "L2UI.etcWnd.OptionBack"
    NCVideoOptionWnd* VideoWnd;            // +0x98
    NCAudioOptionWnd* AudioWnd;            // +0x9C
    NCGameOptionWnd* GameWnd;              // +0xA0
    NCButton* OKButton;                    // +0xA4
    NCButton* CancelButton;                // +0xA8
    NCButton* ApplyButton;                 // +0xAC
    NCTabControl* TabCtrl;                 // +0xB0
    NCWnd* UnusedB4;                       // +0xB4 destroyed, never created

    NCOptionWnd();                                                   // 0x100817c0
    // ~NCOptionWnd                                                  // 0x10081810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007de70
    virtual INT OnCreate();                                          // 35 0x10081860
    virtual INT OnDestroy();                                         // 36 0x10081ed0
    virtual void ShowWindow();                                       // 42 0x1007df20
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10081df0
    // names ours
    virtual INT OnOK();                                              // 62 0x10082050
    virtual INT OnCancel();                                          // 63 0x10059d00 (folded)
    virtual INT OnApply();                                           // 64 0x100820b0
};
NW_ASSERT_OFFSET(NCOptionWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCOptionWnd, OKButton, 0xA4);
NW_ASSERT_OFFSET(NCOptionWnd, TabCtrl, 0xB0);
NW_ASSERT_SIZE(NCOptionWnd, 0xB8);

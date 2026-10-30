#pragma once
// NCSystemMenuWnd - the system menu (UClass "CSystemMenuWnd", 0xAC; Alt+X): Help, Options,
// Restart and Exit Game. Restart and Exit ask first (dialogs 0x1B / 0x1C); the dialog box
// answers with NWM_DIALOG_RESULT and the dialog id, and the menu sends the request.

#include "nwindow/NCWnd.h"

class NCButton;

// Buttons[] (names ours); the fifth slot is never created
enum
{
    SYSMENU_Help,
    SYSMENU_Options,
    SYSMENU_Restart,
    SYSMENU_Exit,
    SYSMENU_Unused,
    SYSMENU_MAX
};

// NWM_COMMAND ids of the buttons and title bar, and the dialogs the menu opens
enum
{
    NID_SYSMENU_HELP = 0xF0F006,
    NID_SYSMENU_OPTIONS = 0xF0F007,
    NID_SYSMENU_RESTART = 0xF0F008,
    NID_SYSMENU_EXIT = 0xF0F009,
    NID_SYSMENU_FRAME = 0xF0F011,
    DIALOG_SYSMENU_RESTART = 0x1B,
    DIALOG_SYSMENU_EXIT = 0x1C,
};

class NCSystemMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSystemMenuWnd, NCWnd, 0, 0x1013b528)

    UTexture* BackTex;                     // +0x94 "L2UI.etcWnd.systemBack"
    NCButton* Buttons[SYSMENU_MAX];        // +0x98 SYSMENU_*

    NCSystemMenuWnd();                                               // 0x10077420
    // ~NCSystemMenuWnd                                              // 0x10077490

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10076bf0
    virtual INT OnCreate();                                          // 35 0x100774e0
    virtual INT OnDestroy();                                         // 36 0x10077900 (folded)
    virtual void ShowWindow();                                       // 42 0x10078950 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10077870
    // names ours
    virtual INT OnHelp();                                            // 62 0x10076ce0
    virtual INT OnOptions();                                         // 63 0x10076d00 (folded)
    virtual INT OnRestart();                                         // 64 0x100779f0
    virtual INT OnExit();                                            // 65 0x10077a50
    virtual INT OnDialogResult(WPARAM DialogID, LPARAM Unused);      // 66 0x10076d20
};
NW_ASSERT_OFFSET(NCSystemMenuWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCSystemMenuWnd, Buttons, 0x98);
NW_ASSERT_SIZE(NCSystemMenuWnd, 0xAC);

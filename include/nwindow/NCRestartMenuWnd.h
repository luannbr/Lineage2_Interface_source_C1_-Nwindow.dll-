#pragma once
// NCRestartMenuWnd - the menu that opens when the character dies (UClass "CRestartMenuWnd",
// 0xC0): a button per place to restart at (nearest village, hideaway, castle, siege
// headquarters, fixed). The server says which of them apply (SetRestartPoints); the others
// hide when the menu shows. A click sends the choice (RequestRestartPoint) and closes it.

#include "nwindow/NCWnd.h"

class NCButton;

// Buttons[] and what the server gets for each (names ours)
enum
{
    RESTART_Village,
    RESTART_Hideaway,
    RESTART_Castle,
    RESTART_SiegeHQ,
    RESTART_Fixed,
    RESTART_MAX
};

// NWM_COMMAND ids of the buttons
enum
{
    NID_RESTART_VILLAGE = 0xF0F012,
    NID_RESTART_HIDEAWAY = 0xF0F013,
    NID_RESTART_CASTLE = 0xF0F014,
    NID_RESTART_SIEGEHQ = 0xF0F015,
    NID_RESTART_FIXED = 0xF0F020,
};

class NCRestartMenuWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCRestartMenuWnd, NCWnd, 0, 0x1013aae8)

    UTexture* BackTex;                     // +0x94 "L2UI.EtcWnd.RestartBack"
    NCButton* Buttons[RESTART_MAX];        // +0x98 RESTART_*
    UBOOL bAvailable[RESTART_MAX];         // +0xAC (1) from the server (SetRestartPoints)

    NCRestartMenuWnd();                                              // 0x10076f00
    // ~NCRestartMenuWnd                                             // 0x10076f90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100767c0
    virtual INT OnCreate();                                          // 35 0x10076fe0
    virtual INT OnDestroy();                                         // 36 0x10077900
    virtual void ShowWindow();                                       // 42 0x10077410
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10077380
    // names ours
    virtual INT OnVillage();                                         // 62 0x10076970
    virtual INT OnHideaway();                                        // 63 0x100769f0
    virtual INT OnCastle();                                          // 64 0x10076a70
    virtual INT OnSiegeHQ();                                         // 65 0x10076af0
    virtual INT OnFixed();                                           // 66 0x10076b70

    // names ours
    void HideUnavailable();                                          // 0x100768b0
    void SetRestartPoints(UBOOL bVillage, UBOOL bHideaway, UBOOL bCastle, UBOOL bSiegeHQ,
                          UBOOL bFixed);                             // 0x10076930 (the console,
                                                                     //   when the player dies)
};
NW_ASSERT_OFFSET(NCRestartMenuWnd, BackTex, 0x94);
NW_ASSERT_OFFSET(NCRestartMenuWnd, Buttons, 0x98);
NW_ASSERT_OFFSET(NCRestartMenuWnd, bAvailable, 0xAC);
NW_ASSERT_SIZE(NCRestartMenuWnd, 0xC0);

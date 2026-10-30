#pragma once
// NCDetailStatusWnd - the character tab of the status window (UClass "CDetailStatusWnd",
// 0xD4; Alt+T): title and name, clan, level and class, HP / MP / exp / weight bars, the combat
// values, the six stats, karma and PvP, all read from the player's User at each paint.
// NCGMDetailStatusWnd derives from it.
//
// NCMainWnd +0x94 is this window (tab 0, created at 0, 65, 240 x 300).

#include "nwindow/NCWnd.h"

class NCDetailStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCDetailStatusWnd, NCWnd, 0, 0x1014ea18)

    User* Player;                    // +0x94 the player (each paint)
    UTexture* BackTex;               // +0x98 "L2UI.StatusWnd.statusback"
    UTexture* HPTex;                 // +0x9C "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0xA0 "L2UI.StatusWnd.statusMP1"
    UTexture* EXPTex;                // +0xA4 "L2UI.StatusWnd.statusEXP1"
    UTexture* SPTex;                 // +0xA8 "L2UI.StatusWnd.statusSP" (loaded, never drawn)
    UTexture* WeightTex[4];          // +0xAC "L2UI.StatusWnd.statusWt1" .. 4: up to 50%, 66.66%,
                                     //   80% of the weight limit, above
    UTexture* NameLeftTex;           // +0xBC "L2UI.StatusWnd.statusName" (left of the name)
    UTexture* NameRightTex;          // +0xC0 "L2UI.StatusWnd.statusName-1" (right of the name)
    UTexture* CrestTex;              // +0xC4 NCGMDetailStatusWnd: the clan crest found by each
                                     //   paint (not used by this class)
    INT UnknownC8[3];                // +0xC8

    NCDetailStatusWnd();                                             // 0x100b1be0
    // ~NCDetailStatusWnd                                            // 0x1005c5b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100aea90
    virtual INT OnDestroy();                                         // 36 0x10083930 (folded)
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b1c00
};
NW_ASSERT_OFFSET(NCDetailStatusWnd, Player, 0x94);
NW_ASSERT_OFFSET(NCDetailStatusWnd, WeightTex, 0xAC);
NW_ASSERT_OFFSET(NCDetailStatusWnd, NameRightTex, 0xC0);
NW_ASSERT_OFFSET(NCDetailStatusWnd, CrestTex, 0xC4);
NW_ASSERT_SIZE(NCDetailStatusWnd, 0xD4);

// NCGMDetailStatusWnd - a GM's view of another player's status (UClass "CGMDetailStatusWnd",
// 0x2DC): the character tab under a title bar, laid out a little lower, drawn from the User it
// keeps (the console fills it with the server's answer). HP and MP in the common font with a
// shadow, exp in percent at the right, a white name (no karma color) with the title before it
// in one string, the crest looked up by its texture name.
//
// Desktop (NCConsole) +0x120 is this window.
class NCGMDetailStatusWnd : public NCDetailStatusWnd
{
    NW_DECLARE_CLASS(NCGMDetailStatusWnd, NCDetailStatusWnd, 0, 0x1012f6b8)

    User Target;                     // +0xD4 the player shown (built by Engine's User::User)

    NCGMDetailStatusWnd();                                           // 0x1005cdb0
    // ~NCGMDetailStatusWnd                                          // 0x1005ce30

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                          // 35 0x1005ce80
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005cf30

    void SetTarget(User InTarget);                                   // 0x1005be50 (name ours) the
                                                                     //   console's, by value
};
NW_ASSERT_OFFSET(NCGMDetailStatusWnd, Target, 0xD4);
NW_ASSERT_SIZE(NCGMDetailStatusWnd, 0x2DC);

// NWM_COMMAND id of the GM window's title bar
enum { NID_GMDETAIL_FRAME = 0xF0F118 };

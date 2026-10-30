#pragma once
// NCAbnormalStatusWnd - the buffs and debuffs on the player (UClass "CAbnormalStatusWnd",
// 0xCC): up to 30 skill icons, ten per row, behind a vertical title bar on the left. The
// console adds one per server notice (AddStatus) and clears them all (ClearStatus); a one
// second timer counts their time down, and an icon blinks during its last ten seconds. The
// tool tip of the icon under the mouse shows its name, level and description.
//
// Desktop (NCConsole) +0xC8 is this window.

#include "nwindow/NCWnd.h"

// One buff / debuff (name ours; 0x1214 bytes, filled by the console, copied into the list).
struct FAbnormalStatus
{
    TCHAR Name[128];                 // +0x0000 the skill's name
    TCHAR IconName[128];             // +0x0100 texture name
    TCHAR Desc[1024];                // +0x0200 the skill's description (escaped newlines)
    TCHAR ToolTip[1024];             // +0x0A00 "name\ndescription" (MakeToolTip)
    UTexture* Icon;                  // +0x1200 NULL: not drawn
    INT RemainTime;                  // +0x1204 seconds (the timer counts it down)
    INT SkillID;                     // +0x1208
    INT Level;                       // +0x120C
    UBOOL bVisible;                  // +0x1210 (1) flips every second in the last ten

    // inlined in the console's AddAbnormalStatus (Desc is left)
    FAbnormalStatus()
    {
        Name[0] = 0;
        IconName[0] = 0;
        ToolTip[0] = 0;
        Icon = NULL;
        RemainTime = 0;
        SkillID = 0;
        Level = 0;
        bVisible = 1;
    }
};
NW_ASSERT_OFFSET(FAbnormalStatus, IconName, 0x100);
NW_ASSERT_OFFSET(FAbnormalStatus, Desc, 0x200);
NW_ASSERT_OFFSET(FAbnormalStatus, ToolTip, 0xA00);
NW_ASSERT_OFFSET(FAbnormalStatus, Icon, 0x1200);
NW_ASSERT_OFFSET(FAbnormalStatus, bVisible, 0x1210);
NW_ASSERT_SIZE(FAbnormalStatus, 0x1214);

enum { TIMER_AbnormalStatus = 0xF9 };

class NCAbnormalStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCAbnormalStatusWnd, NCWnd, 0, 0x1014ef40)

    TArray<FAbnormalStatus> Statuses; // +0x94
    INT TimerID;                     // +0xA0 SetTimer's result
    UTexture* BackTex;               // +0xA4 "L2UI.EtcWndBack.AbnormalBack" under each icon
    UTexture* Frame1Tex;             // +0xA8 left edge for one, two, three rows
    UTexture* Frame2Tex;             // +0xAC
    UTexture* Frame3Tex;             // +0xB0
    UTexture* ToolTipTex;            // +0xB4 "L2UI.tooltip.mini-tool01" (not drawn here)
    UTexture* TipTopTex;             // +0xB8 "L2UI.Tooltip.Tool02"  tool tip box: top,
    UTexture* TipMidTex;             // +0xBC "L2UI.Tooltip.Tool01"  a strip per text line,
    UTexture* TipBottomTex;          // +0xC0 "L2UI.Tooltip.Tool03"  bottom
    INT IconSize;                    // +0xC4 constructor argument (24), set to 26 by each tool
                                     //   tip paint; HitTest uses it
    INT HoverIndex;                  // +0xC8 icon under the mouse, -1 = none

    NCAbnormalStatusWnd(INT InIconSize = 24);                             // 0x100b3780
    // ~NCAbnormalStatusWnd                                          // 0x100b3810

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnTimer(INT InTimerID);                              // 34 0x100af230
    virtual INT OnCreate();                                          // 35 0x100aebf0
    virtual INT OnDestroy();                                         // 36 0x100b3380
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100aed40
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x100aef90
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100af200
    // text of the tool tip, drawn at (X, Y) in its box (name ours)
    virtual void DrawStatusToolTip(NCGDevice* Device, FAbnormalStatus Status, INT X,
                                   INT Y);                           // 62 0x100ae0f0

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x100adf40
    void UpdateSize();                                               // 0x100adf90
    void MakeToolTip(FAbnormalStatus* Status);                       // 0x100ae010
    INT AddStatus(FAbnormalStatus* Status);                          // 0x100b33d0
    void ClearStatus();                                              // 0x100b34d0
};
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, Statuses, 0x94);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, TimerID, 0xA0);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, TipTopTex, 0xB8);
NW_ASSERT_OFFSET(NCAbnormalStatusWnd, HoverIndex, 0xC8);
NW_ASSERT_SIZE(NCAbnormalStatusWnd, 0xCC);

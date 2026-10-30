#pragma once
// NCSkillWnd - the skill list (UClass "CSkillWnd", 0x1AC; the page of NCMagicSkillWnd): tabs
// Active and Passive (sysstring 120 / 121), each an NCSkillItemWnd grid of 6 x 7 icons (80 at
// most). The console fills them from the server's skill list (AddSkill).

#include "nwindow/NControl.h"

class NCSkillItemWnd;
class NCGMSkillItemWnd;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids of the tab controls
enum { NID_SKILL_TABS = 0x0AF11000, NID_GMSKILL_TABS = 0xFFE2 };

class NCSkillWnd : public NControl
{
    NW_DECLARE_CLASS(NCSkillWnd, NControl, 0, 0x10138bf8)

    NCSkillItemWnd* ActiveList;            // +0x1A0
    NCSkillItemWnd* PassiveList;           // +0x1A4
    NCTabControl* TabCtrl;                 // +0x1A8

    NCSkillWnd();                                                    // 0x10073190
    // ~NCSkillWnd                                                   // 0x10073200

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                          // 35 0x10073af0
    virtual INT OnDestroy();                                         // 36 0x10073250
    virtual void ShowWindow();                                       // 42 0x1005bde0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005f200 (folded)

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x1005be00 the console
    void ClearLists();                                               // 0x10072980
};
NW_ASSERT_OFFSET(NCSkillWnd, ActiveList, 0x1A0);
NW_ASSERT_OFFSET(NCSkillWnd, TabCtrl, 0x1A8);
NW_ASSERT_SIZE(NCSkillWnd, 0x1AC);

// NCGMSkillWnd - the GM's view of a player's skills (UClass "CGMSkillWnd", 0x1AC; the page of
// NCGMMagicSkillWnd): NCSkillWnd again, a class of its own under NControl with NCGMSkillItemWnd
// grids. Its other methods are copies of NCSkillWnd's that the linker folded into one body
// each (the console's AddSkill calls land on NCSkillWnd's hooks).
class NCGMSkillWnd : public NControl
{
    NW_DECLARE_CLASS(NCGMSkillWnd, NControl, 0, 0x10132508)

    NCGMSkillItemWnd* ActiveList;          // +0x1A0
    NCGMSkillItemWnd* PassiveList;         // +0x1A4
    NCTabControl* TabCtrl;                 // +0x1A8

    NCGMSkillWnd();                                                  // 0x1005ca50
    // ~NCGMSkillWnd                                                 // 0x1005cac0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                          // 35 0x100604d0
    virtual INT OnDestroy();                                         // 36 0x10073250 (folded)
    virtual void ShowWindow();                                       // 42 0x1005bde0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005f200 (folded)

    // names ours
    INT AddSkill(NItemInfo* Info);                                   // 0x1005be00 (folded)
    void ClearLists();                                               // 0x10072980 (folded)
};
NW_ASSERT_OFFSET(NCGMSkillWnd, ActiveList, 0x1A0);
NW_ASSERT_OFFSET(NCGMSkillWnd, TabCtrl, 0x1A8);
NW_ASSERT_SIZE(NCGMSkillWnd, 0x1AC);

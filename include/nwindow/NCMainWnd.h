#pragma once
// NCMainWnd - the character window (UClass "CMainWnd", 0xAC; Alt+T, Alt+K, Alt+C, Alt+U): four
// pages under a row of tab buttons - status (NCDetailStatusWnd), skills (NCMagicSkillWnd),
// actions (NCActionWnd) and quests (NCQuestWnd) - and the title of the page shown. Minimized,
// its icon is the page's.
//
// Desktop (NCConsole) +0x94 is this window.

#include "nwindow/NCWnd.h"

class NCDetailStatusWnd;
class NCActionWnd;
class NCTabControl;

// NWM_COMMAND ids (name ours)
enum
{
    NID_MAIN_FRAME = 0xF2A6,
    NID_MAIN_TABS = 0xF2A7,           // lParam: the tab clicked
};

// the pages, in tab order (names ours)
enum
{
    MAINTAB_Status = 0,
    MAINTAB_Skills = 1,
    MAINTAB_Actions = 2,
    MAINTAB_Quests = 3,
};

class NCMainWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMainWnd, NCWnd, 0, 0x1012cd70)

    NCDetailStatusWnd* DetailWnd;    // +0x94 tab 0
    NCActionWnd* ActionWnd;          // +0x98 tab 2
    NCWnd* SkillWnd;                 // +0x9C tab 1 NCMagicSkillWnd
    NCWnd* QuestWnd;                 // +0xA0 tab 3 NCQuestWnd
    NCTabControl* TabCtrl;           // +0xA4
    UTexture* BackTex;               // +0xA8 "L2UI.NWindow.mainwndback" (behind the tabs)

    NCMainWnd();                                                     // 0x1003e8b0
    // ~NCMainWnd                                                    // 0x1003e900

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1003e720
    virtual INT OnCreate();                                          // 35 0x10040f30
    virtual INT OnDestroy();                                         // 36 0x10041380
    virtual void ShowWindow();                                       // 42 0x100415e0
    virtual void Minimize();                                         // 47 0x100416f0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10041660
    virtual INT OnTabChanged(INT Tab);                               // 62 0x1003e790 (name ours)

    // names ours
    void ShowTab(INT Tab);                                           // 0x10041470 (shortcut keys)
};
NW_ASSERT_OFFSET(NCMainWnd, DetailWnd, 0x94);
NW_ASSERT_OFFSET(NCMainWnd, TabCtrl, 0xA4);
NW_ASSERT_SIZE(NCMainWnd, 0xAC);

#pragma once
// NCTabControl - row of tab buttons, each showing one page window (UClass "CTabControl",
// 0x1C4). The tabs are NCRadioButtons whose ID is the control's GroupID and whose Value is
// the tab index: a click comes back as NWM_COMMAND(GroupID, index) and selects that page.

#include "nwindow/NControl.h"

class NCRadioButton;

#if NW_HOOK_MODE
#define GNextTabGroupID NW_GLOBAL(INT, 0x1010a6a8)   // starts at 0xFA0000 (name ours)
#else
extern INT GNextTabGroupID;
#endif

class NCTabControl : public NControl
{
    NW_DECLARE_CLASS(NCTabControl, NControl, 0, 0x10123828)

    INT NextTabX;                    // +0x1A0 X of the next tab button
    TArray<NCRadioButton*> Tabs;     // +0x1A4
    INT GroupID;                     // +0x1B0 ID of the tab buttons (OnCreate)
    NCWnd* CurrentPage;              // +0x1B4
    TArray<NCWnd*> Pages;            // +0x1B8 one per tab (not owned)

    NCTabControl();                                                  // 0x10022a10
    // ~NCTabControl                                                 // 0x10022ab0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10022150
    virtual INT OnCreate();                                          // 35 0x100221c0
    virtual INT OnDestroy();                                         // 36 0x100228e0
    virtual void ShowCurrentPage();                                  // 63 0x100221e0 (name ours)
    virtual INT SelectTab(INT Index);                                // 64 0x10022230 (name ours)

    // names ours; a tab of W x H 0 is sized to its label
    INT AddTab(INT TextID, NCWnd* Page, INT W, INT H, const TCHAR* NormalTexName,
               const TCHAR* PushedTexName, INT Gap, INT ToolTipID);  // 0x10022490
    INT AddTab(const TCHAR* Text, NCWnd* Page, INT W, INT H, const TCHAR* NormalTexName,
               const TCHAR* PushedTexName, INT Gap, INT ToolTipID);  // 0x100226d0
    void SetTab(INT Index);                                          // 0x10022330 silent SelectTab
    TArray<NCRadioButton*> GetTabs();                                // 0x10022b20
};
NW_ASSERT_OFFSET(NCTabControl, Tabs, 0x1A4);
NW_ASSERT_OFFSET(NCTabControl, GroupID, 0x1B0);
NW_ASSERT_OFFSET(NCTabControl, Pages, 0x1B8);
NW_ASSERT_SIZE(NCTabControl, 0x1C4);

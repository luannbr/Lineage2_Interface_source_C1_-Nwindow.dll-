#pragma once
// NCQuestWnd - the quest window (UClass "CQuestWnd", 0xCC, 240 x 300; NCMainWnd creates it,
// NCGMQuestWnd derives from it): the accepted quests in a list and, below, two tab pages: the
// selected quest's description (NCQuestInfoWnd) and the quest items (NCQuestItemWnd). Opening
// it asks the server for the quests again; "Cancel" gives up the selected quest after asking.

#include "nwindow/NCWnd.h"

class NCButton;
class NCListBox;
class NCQuestInfoWnd;
class NCQuestItemWnd;
class NCTabControl;
struct NItemInfo;

class NCQuestWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCQuestWnd, NCWnd, 0, 0x10149280)

    UTexture* TopTex;                // +0x94 "L2UI.QuestWnd.questBack1" the top 115 pixels
    UTexture* InfoBackTex;           // +0x98 "L2UI.QuestWnd.questBack2" the rest, page 0
    UTexture* ItemBackTex;           // +0x9C "L2UI.QuestWnd.questBack3" the rest, page 1
    UTexture* ItemPageIconTex;       // +0xA0 "sek.cbui303" 14 x 14 at (220, 127) on page 1
    NCListBox* QuestList;            // +0xA4 the quest names, the quest ids as parameters
    NCQuestInfoWnd* InfoWnd;         // +0xA8 page 0 "Description"
    NCQuestItemWnd* ItemWnd;         // +0xAC page 1 "Item"
    NCTabControl* TabCtrl;           // +0xB0
    NCButton* CancelButton;          // +0xB4 page 0 only
    INT DropX;                       // +0xB8 (160) the rectangle where a quest item dropped
    INT DropY;                       // +0xBC (420)   asks to be destroyed (IsInDropZone): below
    INT DropW;                       // +0xC0 (27)    the window, so a drop inside never hits it
    INT DropH;                       // +0xC4 (27)
    INT CurTab;                      // +0xC8 page shown

    NCQuestWnd();                                                    // 0x100a0fd0
    // ~NCQuestWnd                                                   // 0x1005c6b0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100a0370
    virtual INT OnCreate();                                          // 35 0x100a1890
    virtual INT OnDestroy();                                         // 36 0x100a1040
    virtual void ShowWindow();                                       // 42 0x100a15b0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a1130
    // names ours
    virtual UBOOL IsInDropZone(INT X, INT Y);                        // 62 0x100a04a0
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 63 0x100a12e0
    virtual INT OnCancel();                                          // 64 0x100a1310
    virtual INT OnSelectQuest(INT Index);                            // 65 0x100a13a0
    virtual INT OnTab(INT Page);                                     // 66 0x100a0440

    // names ours: the console fills the window with these
    void Clear();                                                    // 0x100a1580
    INT AddQuest(INT QuestID, const TCHAR* Name, INT Step);          // 0x100a1600
    INT AddQuestItem(NItemInfo* Info);                               // 0x100a1640
};
NW_ASSERT_OFFSET(NCQuestWnd, TopTex, 0x94);
NW_ASSERT_OFFSET(NCQuestWnd, QuestList, 0xA4);
NW_ASSERT_OFFSET(NCQuestWnd, DropX, 0xB8);
NW_ASSERT_OFFSET(NCQuestWnd, CurTab, 0xC8);
NW_ASSERT_SIZE(NCQuestWnd, 0xCC);

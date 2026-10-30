#pragma once
// NCQuestInfoWnd - the quest window's "Description" page (UClass "CQuestInfoWnd", 0x8E4): the
// text of the selected quest at one of its steps (quest data, NConsoleWnd::AddQuestID), shown
// at the step the player is at; the arrows page back to the earlier steps and forward again.

#include "nwindow/NCScrollWnd.h"

class NCPushButton;

// a quest of the list (name ours; 12 bytes)
struct NQuestEntry
{
    INT QuestID;               // +0x0
    INT Step;                  // +0x4 the step the player is at
    INT ShownStep;             // +0x8 1 .. Step
};
NW_ASSERT_SIZE(NQuestEntry, 0xC);

class NCQuestInfoWnd : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCQuestInfoWnd, NCScrollWnd, 0, 0x10148d58)

    TArray<NQuestEntry> Quests;      // +0xCC in the order of NCQuestWnd's list
    TCHAR Text[1024];                // +0xD8
    INT Current;                     // +0x8D8 index in Quests, -1 = none
    NCPushButton* PrevButton;        // +0x8DC
    NCPushButton* NextButton;        // +0x8E0

    NCQuestInfoWnd();                                                // 0x100a16f0
    // ~NCQuestInfoWnd                                               // 0x100a1760

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1009fde0
    virtual INT OnCreate();                                          // 35 0x100a04f0
    virtual INT OnDestroy();                                         // 36 0x100a13d0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a0680
    virtual INT ScrollUp();                                          // 62 0x100a07d0
    virtual INT ScrollDown();                                        // 63 0x100a0860
    // names ours
    virtual INT NextStep();                                          // 65 0x100a0a00
    virtual INT PrevStep();                                          // 66 0x100a0aa0

    // names ours
    void ShowQuest(INT Index);                                       // 0x100a0960
    void AddQuest(INT QuestID, INT Step);                            // 0x100a1400
    void ClearQuests();                                              // 0x100a1450
};
NW_ASSERT_OFFSET(NCQuestInfoWnd, Quests, 0xCC);
NW_ASSERT_OFFSET(NCQuestInfoWnd, Text, 0xD8);
NW_ASSERT_OFFSET(NCQuestInfoWnd, Current, 0x8D8);
NW_ASSERT_SIZE(NCQuestInfoWnd, 0x8E4);

#pragma once
// NCPawnSelectWnd - the character panel of the character selection (UClass "CPawnSelectWnd",
// 0xEEC; NCLobbyWnd +0x94): up to seven characters (their User records by value, each with the
// actor shown on the stage), a name combo box, and the chosen one's level, class, HP / MP / exp
// bars and numbers. Choosing a character marked for deletion asks whether to restore it.

#include "nwindow/NCWnd.h"

class NCComboBox;

class NCPawnSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnSelectWnd, NCWnd, 0, 0x10134908)

    UTexture* BackTex;               // +0x94 "L2UI.statusWnd.selectback"
    UTexture* HPTex;                 // +0x98 "L2UI.StatusWnd.statusHP1"
    UTexture* MPTex;                 // +0x9C "L2UI.StatusWnd.statusMP1"
    UTexture* EXPTex;                // +0xA0 "L2UI.StatusWnd.statusEXP1"
    INT UnknownA4;                   // +0xA4 (0)
    NCComboBox* NameCombo;           // +0xA8
    User Characters[7];              // +0xAC Pawn: the actor on the stage
    INT NumCharacters;               // +0xEE4
    INT Selected;                    // +0xEE8 -1 = none

    NCPawnSelectWnd();                                               // 0x100691a0
    // ~NCPawnSelectWnd                                              // 0x10069270

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10068b60
    virtual INT OnCreate();                                          // 35 0x100692c0
    virtual INT OnDestroy();                                         // 36 0x10069bd0
    virtual void ShowWindow();                                       // 42 0x100a5d70 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10069470
    // names ours
    virtual void SelectCharacter(INT Index);                         // 62 0x10068d20
    virtual INT OnSelect(INT Index);                                 // 63 0x10069c10
    virtual INT OnRestoreAnswer(INT bYes, INT Unused);               // 64 0x10068e00

    // names ours
    INT AddCharacter(User Info);                                     // 0x10068c00
    INT GetSelected();                                               // 0x10068d50
    TCHAR* GetCharacterName(INT Index);                              // 0x10068d60
    void ClearCharacters();                                          // 0x10068d80
};
NW_ASSERT_OFFSET(NCPawnSelectWnd, NameCombo, 0xA8);
NW_ASSERT_OFFSET(NCPawnSelectWnd, Characters, 0xAC);
NW_ASSERT_OFFSET(NCPawnSelectWnd, NumCharacters, 0xEE4);
NW_ASSERT_SIZE(NCPawnSelectWnd, 0xEEC);

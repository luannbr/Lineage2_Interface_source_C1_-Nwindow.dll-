#pragma once
// NCLobbyWnd - the character selection screen (UClass "CLobbyWnd", 0xB0; NConsoleWnd +0x36CC):
// the character panel (NCPawnSelectWnd), the characters' names floating over the stage, a
// yes / no box, and "Create", "Delete", "Start", "Previous" in the lower right corner. At most
// seven characters; Delete and Start wait for a choice.

#include "nwindow/NCWnd.h"

class NCPawnSelectWnd;
class NCDialogBox;
class NCButton;

class NCLobbyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCLobbyWnd, NCWnd, 0, 0x10134e30)

    NCPawnSelectWnd* PawnSelectWnd;  // +0x94
    NCDialogBox* DialogBox;          // +0x98 deletion / restoration questions
    NCButton* StartButton;           // +0x9C
    NCButton* PreviousButton;        // +0xA0
    NCButton* CreateButton;          // +0xA4
    NCButton* DeleteButton;          // +0xA8
    INT NumCharacters;               // +0xAC

    NCLobbyWnd();                                                    // 0x10069e20
    // ~NCLobbyWnd                                                   // 0x10069e90

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10068eb0
    virtual INT OnCreate();                                          // 35 0x1006a050
    virtual INT OnDestroy();                                         // 36 0x1006a510
    virtual void ShowWindow();                                       // 42 0x10069160
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10069ee0
    // names ours
    virtual INT OnCommand10001() { return 1; }                       // 62 0x10005230 (folded)
    virtual INT OnStart();                                           // 63 0x10069030
    virtual INT OnPrevious();                                        // 64 0x100690c0
    virtual INT OnCreateCharacter();                                 // 65 0x100690e0
    virtual INT OnDelete();                                          // 66 0x1006a630
    virtual INT OnDeleteAnswer(INT bYes, INT Unused);                // 67 0x10069130

    // names ours
    INT AddCharacter(User Info);                                     // 0x10068fc0
    void ClearCharacters();                                          // 0x10069010
};
NW_ASSERT_OFFSET(NCLobbyWnd, PawnSelectWnd, 0x94);
NW_ASSERT_OFFSET(NCLobbyWnd, DialogBox, 0x98);
NW_ASSERT_OFFSET(NCLobbyWnd, DeleteButton, 0xA8);
NW_ASSERT_SIZE(NCLobbyWnd, 0xB0);

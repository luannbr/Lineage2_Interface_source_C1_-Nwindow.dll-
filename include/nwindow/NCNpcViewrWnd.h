#pragma once
// NCNpcViewrWnd - the NPC viewer (UClass "CNpcViewrWnd", 0x2750), a developer tool the console
// builds for its pawn viewer mode (NConsoleWnd::InitializePawnViewer, the console's
// PawnViewerWnd[1]): the item table in a grid, the items whose name holds the text searched in a second
// one; a right click (or a double click) on an item dresses the pawn shown with it. Beside it a
// top-level NCNpcView_CharacterSelectWnd lists the NPC table (NPCs and monsters, by name or by
// mesh): a double click turns the player's pawn into that NPC, "Summon" spawns it next to the
// player; the pawn's animations play from a combo box and two edit lines set its speed.
//
// NCNpcView_CharacterSelect_List - that NPC list (UClass "CNpcView_CharacterSelect_List",
// 0x1E8, a debug list): each line's parameter is its NViewNpcData.
//
// NCNpcView_InvenItemWnd - the item grids (UClass "CNpcView_InvenItemWnd", 0x13C): nothing
// can be dragged.

#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NViewerData.h"

class NCButton;
class NCCheckBox;
class NCComboBox;
class NCEditBox;
class NCTabControl;
struct NItemInfo;

// NWM_COMMAND ids of the viewer's controls (names ours)
enum
{
    NID_NPCVIEW_FRAME = 0xF2,
    NID_NPCVIEW_SEARCH = 0xFC,
    NID_NPCVIEW_TAB = 0xFE001A,
    NID_NPCVIEW_SELECT_FRAME = 0xF3,
    NID_NPCVIEW_SELECT_SEARCHEDIT = 0xF4,
    NID_NPCVIEW_SELECT_SEARCH = 0xF5,
    NID_NPCVIEW_SELECT_RESET = 0xF6,
    NID_NPCVIEW_SELECT_RATE = 0xF7,
    NID_NPCVIEW_SELECT_SPEED = 0xF8,
    NID_NPCVIEW_SELECT_APPLY = 0xF9,
    NID_NPCVIEW_SELECT_SUMMON = 0xFA,
    NID_NPCVIEW_SELECT_MESHNAME = 0xFB,
    NID_NPCVIEW_SELECT_ANIM = 0x1F05,
};

class NCNpcView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCNpcView_InvenItemWnd, NCInvenItemWnd, 0, 0x1013e388)

    NCNpcView_InvenItemWnd();                                        // 0x1007be90
    NCNpcView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1007bf30
    // ~NCNpcView_InvenItemWnd                                       // 0x1007bee0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a1a0 (also the
                                                                     //   skill viewer's grids')
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 1; } // 55 0x100098f0 (folded)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; } // 57 0x100016f0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x1007a7f0
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1007a210
};
NW_ASSERT_SIZE(NCNpcView_InvenItemWnd, 0x13C);

class NCNpcView_CharacterSelect_List : public NCDebugWnd
{
    NW_DECLARE_CLASS(NCNpcView_CharacterSelect_List, NCDebugWnd, 0, 0x1013d938)

    NCNpcView_CharacterSelect_List();                                // 0x1007c5d0
    NCNpcView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines); // 0x1007c670
    // ~NCNpcView_CharacterSelect_List                               // 0x1007c620

    // 24, 61, 67, 68 and 69 are also NCPawnView_CharacterSelect_List's
    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a530
    virtual INT OnCreate();                                          // 35 0x1007a570
    virtual INT OnDestroy();                                         // 36 0x1007a670
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1007a5f0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x1007a730
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 66 0x1007acf0
    virtual INT ScrollUp();                                          // 67 0x1007a6f0
    virtual INT ScrollDown();                                        // 68 0x1007a700
    virtual INT SetTopIndex(INT Index);                              // 69 0x1007a710

    void ToggleNames(UBOOL bMeshNames);                              // 0x1007b190 (guard name)
    // name ours: the scroll bar's range (an inline method the stock compiler emitted)
    void SetScrollRange(INT Total, INT Page);                        // 0x1007a750
};
NW_ASSERT_SIZE(NCNpcView_CharacterSelect_List, 0x1E8);

class NCNpcView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCNpcView_CharacterSelectWnd, NCWnd, 0, 0x1013de58)

    NCNpcView_CharacterSelect_List* List; // +0x94
    NCComboBox* AniCombo;                // +0x98 the pawn's animations
    NCEditBox* SearchEdit;               // +0x9C
    NCButton* SearchButton;              // +0xA0
    NCButton* ResetButton;               // +0xA4 the whole table again
    NCEditBox* RateEdit;                 // +0xA8 the pawn's speed factor (Rate)
    NCEditBox* SpeedEdit;                // +0xAC its speed (Speed)
    NCButton* ApplyButton;               // +0xB0 the two edit lines to the pawn
    NCCheckBox* MeshNameCheck;           // +0xB4 labeled "Mesh name" (in Korean): the list
                                         //   shows the meshes
    NCButton* SummonButton;              // +0xB8 labeled "Summon" (in Korean)
    NCCheckBox* NpcCheck;                // +0xBC the NPCs are listed
    NCCheckBox* MonsterCheck;            // +0xC0 the monsters are listed
    UTexture* TitleTex;                  // +0xC4 "sek.cbui168"
    UTexture* BackTex;                   // +0xC8 "sek.cbui144"
    FLOAT Rate;                          // +0xCC (1.0)
    INT Speed;                           // +0xD0

    NCNpcView_CharacterSelectWnd();                                  // 0x1007b240
    // ~NCNpcView_CharacterSelectWnd                                 // 0x1007b290

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007bd80
    virtual INT OnCreate();                                          // 35 0x1007c6a0
    virtual INT OnDestroy();                                         // 36 0x1007b3e0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1007b610
    // names ours
    virtual INT OnSearch();                                          // 62 0x1007cfe0
    virtual INT OnReset();                                           // 63 0x1007d210
    virtual INT OnApply();                                           // 64 0x1007b800
    virtual INT OnToggleNames();                                     // 65 0x1007b920
    virtual INT OnSummon();                                          // 66 0x1007b940

    INT OnAniNameCombo(INT Index);                                   // 0x1007b2e0 (guard name)
};
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, List, 0x94);
NW_ASSERT_OFFSET(NCNpcView_CharacterSelectWnd, TitleTex, 0xC4);
NW_ASSERT_SIZE(NCNpcView_CharacterSelectWnd, 0xD4);

class NCNpcViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCNpcViewrWnd, NCWnd, 0, 0x1013d410)

    NCNpcView_InvenItemWnd* ItemWnd[2];  // +0x94 the tabs: all the items, the search's
    NCNpcView_CharacterSelectWnd* CharSelectWnd; // +0x9C top-level
    NCTabControl* TabCtrl;               // +0xA0
    NCEditBox* SearchEdit;               // +0xA4
    NCButton* SearchButton;              // +0xA8
    UTexture* BackTex;                   // +0xAC "L2UI.QuestWnd.questBack3"
    UTexture* OptionBackTex;             // +0xB0 "L2UI.etcWnd.OptionBack"
    User Npcs[19];                       // +0xB4 only the first is used: the pawn shown
    ULevel* Level;                       // +0x274C the game's when created

    NCNpcViewrWnd();                                                 // 0x1007a9a0
    // ~NCNpcViewrWnd                                                // 0x1007aa60

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a2d0
    virtual INT OnCreate();                                          // 35 0x1007d260
    virtual INT OnDestroy();                                         // 36 0x1007abc0
    virtual void ShowWindow();                                       // 42 0x1007a3d0
    virtual void HideWindow();                                       // 43 0x1007a340
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1007aab0
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x1007a510
    virtual INT OnSearchBtnClick();                                  // 62 0x1007c370 (guard name)

    // names from the guards
    INT AddItem(INT Tab, NItemInfo* Info);                           // 0x1007a470
    void AddAllCharacter();                                          // 0x1007bf70
    void AddAllItem();                                               // 0x1007c170
};
NW_ASSERT_OFFSET(NCNpcViewrWnd, CharSelectWnd, 0x9C);
NW_ASSERT_OFFSET(NCNpcViewrWnd, Npcs, 0xB4);
NW_ASSERT_OFFSET(NCNpcViewrWnd, Level, 0x274C);
NW_ASSERT_SIZE(NCNpcViewrWnd, 0x2750);

// the console's NPC viewer (the second of its pawn viewer windows)
FORCEINLINE NCNpcViewrWnd* NpcViewer() { return static_cast<NCNpcViewrWnd*>(GConsole->PawnViewerWnd[1]); }

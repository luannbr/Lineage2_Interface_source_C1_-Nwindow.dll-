#pragma once
// NCMSViewrWnd - the skill viewer (UClass "CMSViewrWnd", 0x276C; titled "SKILLVIEWER"), a
// developer tool the console builds for its pawn viewer mode (NConsoleWnd::
// InitializePawnViewer, the console's PawnViewerWnd[2]): the level-1 skills of the skill table
// in three tabs (magic, active, passive) and the results of a search by name or by id in a
// fourth; a right click (or a double click) on a skill casts it with the player's pawn at its
// target. Beside it a top-level NCMSView_CharacterSelectWnd picks the character to cast with
// (race, sex and class: the pawn is spawned anew) and plays its animations.
//
// NCMSView_InvenItemWnd - the skill grids (UClass "CMSView_InvenItemWnd", 0x13C): nothing can
// be dragged.

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
    NID_MSVIEW_FRAME = 0xF2,
    NID_MSVIEW_NAMEEDIT = 0xF3,
    NID_MSVIEW_IDEDIT = 0xF4,
    NID_MSVIEW_SEARCHNAME = 0xF5,
    NID_MSVIEW_SEARCHID = 0xF6,
    NID_MSVIEW_TAB = 0xF7,
    NID_MSVIEW_SELECT_FRAME = (INT)0xAFF12011,
    NID_MSVIEW_SELECT_NAME = 0x1F04,
    NID_MSVIEW_SELECT_ANIM = 0x1F05,
};

class NCMSView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCMSView_InvenItemWnd, NCInvenItemWnd, 0, 0x10137758)

    NCMSView_InvenItemWnd();                                         // 0x10070cf0
    NCMSView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x10070d90
    // ~NCMSView_InvenItemWnd                                        // 0x10070d40

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a1a0 (hooked as
                                                                     //   NCNpcView_InvenItemWnd's)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 1; } // 55 0x100098f0 (folded)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; } // 57 0x100016f0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x10070b80
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1006f0c0
};
NW_ASSERT_SIZE(NCMSView_InvenItemWnd, 0x13C);

class NCMSView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMSView_CharacterSelectWnd, NCWnd, 0, 0x101381a8)

    NCComboBox* NameCombo;               // +0x94 the characters: race, sex, class
    NCComboBox* AniCombo;                // +0x98 the pawn's animations
    UTexture* TitleTex;                  // +0x9C "sek.cbui168"
    UTexture* BackTex;                   // +0xA0 "sek.cbui144"
    UTexture* Unknown67Tex;              // +0xA4 "sek.cbui67" (loaded, not drawn)
    UTexture* Unknown68Tex;              // +0xA8 "sek.cbui68"
    UTexture* Unknown69Tex;              // +0xAC "sek.cbui69"
    INT UnknownB0;                       // +0xB0
    INT Selected;                        // +0xB4 (-1) the character shown

    NCMSView_CharacterSelectWnd();                                   // 0x10070100
    // ~NCMSView_CharacterSelectWnd                                  // 0x10070180

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006fc10
    virtual INT OnCreate();                                          // 35 0x100701d0
    virtual INT OnDestroy();                                         // 36 0x10089310 (hooked as
                                                                     //   NCTrainWnd's)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10070470
    // names from the guards
    virtual INT OnNameComboBox(INT Index);                           // 62 0x100706a0
    virtual INT OnAniNameComboBox(INT Index);                        // 63 0x10070a40
};
NW_ASSERT_OFFSET(NCMSView_CharacterSelectWnd, TitleTex, 0x9C);
NW_ASSERT_OFFSET(NCMSView_CharacterSelectWnd, Selected, 0xB4);
NW_ASSERT_SIZE(NCMSView_CharacterSelectWnd, 0xB8);

class NCMSViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMSViewrWnd, NCWnd, 0, 0x10137c80)

    NCMSView_InvenItemWnd* ItemWnd[4];   // +0x94 the tabs: magic, active, passive, the search's
    NCMSView_CharacterSelectWnd* CharSelectWnd; // +0xA4 top-level
    NCTabControl* TabCtrl;               // +0xA8
    UTexture* BackTex;                   // +0xAC "L2UI.SkillWnd.SkillBack"
    UTexture* HtmlBackTex;               // +0xB0 "L2UI.etcwnd.NPCHtml_BACK" behind the search
    NCEditBox* NameEdit;                 // +0xB4
    NCEditBox* IDEdit;                   // +0xB8
    NCCheckBox* MagicCheck;              // +0xBC the search lists the magic skills
    NCCheckBox* ActiveCheck;             // +0xC0 the active ones
    NCCheckBox* PassiveCheck;            // +0xC4 the passive ones
    NCButton* SearchNameButton;          // +0xC8
    NCButton* SearchIDButton;            // +0xCC
    User Characters[19];                 // +0xD0 the characters offered (AddAllCharacter)
    ULevel* Level;                       // +0x2768 the game's when created

    NCMSViewrWnd();                                                  // 0x1006fca0
    // ~NCMSViewrWnd                                                 // 0x1006fd70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1006f180
    virtual INT OnCreate();                                          // 35 0x10071580
    virtual INT OnDestroy();                                         // 36 0x1006ff70
    virtual void ShowWindow();                                       // 42 0x1006fad0
    virtual void HideWindow();                                       // 43 0x1006fa40
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1006fdc0
    // names ours
    virtual INT OnSearchName();                                      // 62 0x10071030
    virtual INT OnSearchID();                                        // 63 0x100712e0

    // names from the guards
    INT AddItem(INT Tab, NItemInfo* Info);                           // 0x1006fb70
    void AddAllCharacter();                                          // 0x1006f210
    void AddAllItem();                                               // 0x10070dd0
};
NW_ASSERT_OFFSET(NCMSViewrWnd, CharSelectWnd, 0xA4);
NW_ASSERT_OFFSET(NCMSViewrWnd, Characters, 0xD0);
NW_ASSERT_OFFSET(NCMSViewrWnd, Level, 0x2768);
NW_ASSERT_SIZE(NCMSViewrWnd, 0x276C);

// the console's skill viewer (the third of its pawn viewer windows)
FORCEINLINE NCMSViewrWnd* MSViewer() { return static_cast<NCMSViewrWnd*>(GConsole->PawnViewerWnd[2]); }

#pragma once
// PawnView - the developers' pawn viewer (not in the normal game: NConsoleWnd's
// InitializePawnViewer makes it, the console's PawnViewerWnd[0]). It dresses and animates the
// pawn the player controls:
//
// NCPawnViewrWnd - the main window (UClass "CPawnViewrWnd", 0x27D0): three pages over the
// pawn (combined animations, hair meshes, a crowd simulator), the item table by category as
// icon grids or as text lists (the "GUI" check box picks), a search of the item names, and the
// weapon of a weapon type for the animations ("Use"). Each item worn goes into the record of
// the default character chosen in NCPawnView_CharacterSelectWnd (Characters, 18 races /
// genders / class types), which dresses the pawn (User::SetPawnResource).
//
// NCPawnView_CharacterSelectWnd - a top-level window (UClass "CPawnView_CharacterSelectWnd",
// 0xBC): the default character in a combo box (the player's pawn is replaced by a new one of
// its class) and its mesh's animation sequences in NCPawnView_CharacterSelect_List (a double
// click plays one in a loop, a second one stops it), sorted by weapon or by action.
//
// NCPawnView_InvenItemWnd / NCPawnView_InvenTextWnd - a category's items as icons (a right or
// double click puts the item on or takes it off) and as text lines (a click does).
//
// NCPawnViewr_MixAnimWnd - three animations taken from the list and a speed: "Start" plays
// them as the three steps of a skill (APawn::SetMagicInfo / InitSkillProcess).
//
// NCPawnViewr_HairChangeWnd - hair style and color, then the two hair meshes (a / b) found for
// them in LineageWarriors, and their textures; "OK" puts them on the pawn.
//
// NCPawnViewr_Simulator - spawns N random characters around the pawn that attack each other,
// cast, cancel and get replaced, by the percentages typed (its per-frame part never runs:
// nothing sets bRunning).

#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCInvenItemWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NViewerData.h"

class APawn;
class NCButton;
class NCCheckBox;
class NCComboBox;
class NCEditBox;
class NCTabControl;
class NCPawnViewrWnd;

class NCPawnView_InvenItemWnd : public NCInvenItemWnd
{
    NW_DECLARE_CLASS(NCPawnView_InvenItemWnd, NCInvenItemWnd, 0, 0x10142158)

    NCPawnView_InvenItemWnd();                                       // 0x1008c750
    NCPawnView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1008c7f0
    // ~NCPawnView_InvenItemWnd                                      // 0x1008c7a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a1a0 (shared)
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam) { return 1; } // 55 0x100098f0 (folded)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam) { return 0; } // 57 0x100016f0 (folded)
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58 0x100888d0
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x10087420
};
NW_ASSERT_SIZE(NCPawnView_InvenItemWnd, 0x13C);

class NCPawnView_InvenTextWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnView_InvenTextWnd, NCWnd, 0, 0x101411e0)

    NCListBox* List;                 // +0x94 item names; a line's param is its NItemEntry

    NCPawnView_InvenTextWnd();                                       // 0x10088830
    // ~NCPawnView_InvenTextWnd                                      // 0x10088880

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100881f0
    virtual INT OnCreate();                                          // 35 0x100894e0
    virtual void ShowWindow() {}                                     // 42 0x10045870 (folded)
    virtual INT OnInvenItemList(INT Index);                          // 62 0x10089380
};
NW_ASSERT_SIZE(NCPawnView_InvenTextWnd, 0x98);

class NCPawnView_CharacterSelect_List : public NCDebugWnd
{
    NW_DECLARE_CLASS(NCPawnView_CharacterSelect_List, NCDebugWnd, 0, 0x10141700)

    INT PlayingIndex;                // +0x1E8 line whose animation plays, -1 = none

    NCPawnView_CharacterSelect_List();                               // 0x1008d070
    NCPawnView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines); // 0x1008d110
    // ~NCPawnView_CharacterSelect_List                              // 0x1008d0c0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007a530 (shared)
    virtual INT OnCreate();                                          // 35 0x10088450
    virtual INT OnDestroy();                                         // 36 0x10088550
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100884d0
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x1007a730 (shared)
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 66 0x10089f10
    virtual INT ScrollUp();                                          // 67 0x1007a6f0 (shared)
    virtual INT ScrollDown();                                        // 68 0x1007a700 (shared)
    virtual INT SetTopIndex(INT Index);                              // 69 0x1007a710 (shared)
};
NW_ASSERT_OFFSET(NCPawnView_CharacterSelect_List, PlayingIndex, 0x1E8);
NW_ASSERT_SIZE(NCPawnView_CharacterSelect_List, 0x1EC);

class NCPawnView_CharacterSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnView_CharacterSelectWnd, NCWnd, 0, 0x10141c28)

    NCComboBox* NameCombo;           // +0x94 the default characters
    NCPawnView_CharacterSelect_List* AnimList; // +0x98 the pawn mesh's animations
    NCCheckBox* ByActionCheck;       // +0x9C sort the animations by action (else by weapon)
    UTexture* TitleTex;              // +0xA0 "sek.cbui168"
    UTexture* BackTex;               // +0xA4 "sek.cbui144"
    UTexture* UnknownA8;             // +0xA8 "sek.cbui67" (not drawn)
    UTexture* UnknownAC;             // +0xAC "sek.cbui68" (not drawn)
    UTexture* UnknownB0;             // +0xB0 "sek.cbui69" (not drawn)
    INT UnknownB4;                   // +0xB4 (0)
    INT Selected;                    // +0xB8 default character shown, -1 = none

    NCPawnView_CharacterSelectWnd();                                 // 0x10089140
    // ~NCPawnView_CharacterSelectWnd                                // 0x100891c0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10088140
    virtual INT OnCreate();                                          // 35 0x1008d960
    virtual INT OnDestroy();                                         // 36 0x10089310 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10089210
    virtual INT OnNameComboBox(INT Index);                           // 62 0x1008cd20
    virtual INT OnAniNameComboBox(INT Index) { return 1; }           // 63 0x10002830 (folded)
    virtual INT OnByActionCheck();                                   // 64 0x1008d060 (name ours)

    void UpdateAnimList();                                           // 0x1008c3c0 (name ours)
};
NW_ASSERT_OFFSET(NCPawnView_CharacterSelectWnd, TitleTex, 0xA0);
NW_ASSERT_OFFSET(NCPawnView_CharacterSelectWnd, Selected, 0xB8);
NW_ASSERT_SIZE(NCPawnView_CharacterSelectWnd, 0xBC);

class NCPawnViewr_MixAnimWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_MixAnimWnd, NCWnd, 0, 0x101430c0)

    NCButton* TakeButtons[3];        // +0x94 "Use": the list's animation into the edit line
    NCEditBox* AnimEdits[3];         // +0xA0 the skill's three animations (disabled: filled
                                     //   by the buttons only)
    NCEditBox* SpeedEdit;            // +0xAC ("1.9")
    NCButton* StartButton;           // +0xB0 "Start"

    NCPawnViewr_MixAnimWnd();                                        // 0x10089620
    // ~NCPawnViewr_MixAnimWnd                                       // 0x10089680

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10088260
    virtual INT OnCreate();                                          // 35 0x100896d0
    virtual INT OnDestroy();                                         // 36 0x10089dc0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10089cb0
    // names ours
    virtual INT OnTake1();                                           // 62 0x10088330
    virtual INT OnTake2();                                           // 63 0x10088390
    virtual INT OnTake3();                                           // 64 0x100883f0
    virtual INT OnStart();                                           // 65 0x10089b10
};
NW_ASSERT_OFFSET(NCPawnViewr_MixAnimWnd, AnimEdits, 0xA0);
NW_ASSERT_SIZE(NCPawnViewr_MixAnimWnd, 0xB4);

class NCPawnViewr_HairChangeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_HairChangeWnd, NCWnd, 0, 0x10142ba0)

    NCComboBox* HairStyleCombo;      // +0x94 0..4
    NCComboBox* HairColorCombo;      // +0x98 0..1
    NCComboBox* MeshACombo;          // +0x9C "a" hair meshes found
    NCComboBox* MeshBCombo;          // +0xA0 "b" hair meshes found
    INT HairColor;                   // +0xA4 as searched
    INT HairStyle;                   // +0xA8 as searched
    INT MeshANumbers[10];            // +0xAC the mesh number of each MeshACombo line
    INT MeshBNumbers[10];            // +0xD4
    TCHAR MeshATexture[256];         // +0xFC "<race>.<race>_m00N_t0C_m0S_ah"
    TCHAR MeshBTexture[256];         // +0x2FC "..._bh"
    NCButton* OKButton;              // +0x4FC
    NCButton* ClearButton;           // +0x500 "Cancel"
    NCButton* SearchButton;          // +0x504

    NCPawnViewr_HairChangeWnd();                                     // 0x1008a040
    // ~NCPawnViewr_HairChangeWnd                                    // 0x1008a120

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100885d0
    virtual INT OnCreate();                                          // 35 0x1008a170
    virtual INT OnDestroy();                                         // 36 0x1008bb20
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1008b7d0
    // names ours
    virtual INT OnOK();                                              // 62 0x1008a670
    virtual INT OnClear();                                           // 63 0x100886d0
    virtual INT OnMeshA(INT Index);                                  // 64 0x1008aed0
    virtual INT OnMeshB(INT Index);                                  // 65 0x1008b350
    virtual INT OnSearch();                                          // 66 0x1008a830
};
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshANumbers, 0xAC);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, MeshATexture, 0xFC);
NW_ASSERT_OFFSET(NCPawnViewr_HairChangeWnd, OKButton, 0x4FC);
NW_ASSERT_SIZE(NCPawnViewr_HairChangeWnd, 0x508);

// a pawn of the simulator (name ours; 8 bytes)
struct NSimPawn
{
    APawn* Pawn;
    INT Kind;                        // 1: attacks and casts the skill table's skills, 2: the
                                     //   bow share (casts a few fixed skills), 0: none
};

// the stock's std::vector<NSimPawn> (MSVC 7.1 layout: the empty allocator, then first, last and
// end of storage). The simulator only reserves it and writes its elements past the size.
struct NSimPawnVector
{
    INT Allocator;                   // +0x0
    NSimPawn* First;                 // +0x4
    NSimPawn* Last;                  // +0x8
    NSimPawn* End;                   // +0xC

    // <vector>'s reserve, _Xlen and _Ucopy's _Uninitialized_copy (NStdLib.cpp; names ours)
    void Reserve(DWORD Count);                                       // 0x1008dd30
    void Xlen() const;                                               // 0x1008dcb0
    static NSimPawn* UninitializedCopy(NSimPawn* First, NSimPawn* Last, NSimPawn* Dest,
                                       void* Al, INT Tag);           // 0x1008c720

    // the vector's destructor, inline in the simulator's
    ~NSimPawnVector()
    {
        if (First)
            appFree(First);
        First = NULL;
        Last = NULL;
        End = NULL;
    }
};
NW_ASSERT_SIZE(NSimPawnVector, 0x10);

// 4-byte packing: LastTime sits at +0xC4
#pragma pack(push, 4)
class NCPawnViewr_Simulator : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewr_Simulator, NCWnd, 0, 0x10142678)

    NCEditBox* CountEdit;            // +0x94 how many pawns
    NCEditBox* SkillEdit;            // +0x98 % a second to use a skill or attack
    NCEditBox* CancelEdit;           // +0x9C % a second to cancel
    NCEditBox* ReplaceEdit;          // +0xA0 % a second to be replaced by a new pawn
    NCEditBox* BowEdit;              // +0xA4 % of the new pawns of the bow kind
    NCButton* StartButton;           // +0xA8 "Start"
    UBOOL bRunning;                  // +0xAC never set
    INT NumPawns;                    // +0xB0
    FLOAT SkillRate;                 // +0xB4 the percentages typed
    FLOAT CancelRate;                // +0xB8
    FLOAT ReplaceRate;               // +0xBC
    FLOAT BowRate;                   // +0xC0
    DOUBLE LastTime;                 // +0xC4 appSeconds
    DOUBLE DeltaTime;                // +0xCC seconds since the last frame
    NSimPawnVector Pawns;            // +0xD4
    INT SkillIDs[2000];              // +0xE4 active skills with an animation (OnCreate)
    INT NumSkills;                   // +0x2024

    NCPawnViewr_Simulator();                                         // 0x1008e0f0
    // ~NCPawnViewr_Simulator                                        // 0x1008e190

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10088770
    virtual INT OnCreate();                                          // 35 0x1008d150
    virtual INT OnDestroy();                                         // 36 0x1006a510 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1008bc10
    // names ours
    virtual INT OnStart();                                           // 62 0x1008de40
    virtual NSimPawn SpawnPawn();                                    // 63 0x1008bf30
    virtual INT TryAction(INT Index);                                // 64 0x1008d590
    virtual INT TryCancel(INT Index);                                // 65 0x1008d830
    virtual INT TryReplace(INT Index);                               // 66 0x1008d880
};
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, bRunning, 0xAC);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, LastTime, 0xC4);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, Pawns, 0xD4);
NW_ASSERT_OFFSET(NCPawnViewr_Simulator, SkillIDs, 0xE4);
NW_ASSERT_SIZE(NCPawnViewr_Simulator, 0x2028);
#pragma pack(pop)

class NCPawnViewrWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnViewrWnd, NCWnd, 0, 0x101435e8)

    NCPawnView_InvenItemWnd* ItemWnds[8]; // +0x94 a grid per category (6 made: chest, legs,
                                     //   gloves, feet, headgear, etc)
    NCPawnView_InvenTextWnd* TextWnds[8]; // +0xB4 the same as text lists
    NCPawnView_CharacterSelectWnd* CharSelectWnd; // +0xD4 top-level (not a child)
    NCPawnViewr_MixAnimWnd* MixAnimWnd; // +0xD8 the pages of PageTabs
    NCPawnViewr_HairChangeWnd* HairChangeWnd; // +0xDC
    NCPawnViewr_Simulator* Simulator; // +0xE0
    NCTabControl* CategoryTabs;      // +0xE4 no pages: OnCategoryTab shows the grid or list
    NCTabControl* PageTabs;          // +0xE8
    NCCheckBox* GUICheck;            // +0xEC "GUI": the grids, else the text lists
    UTexture* GridBackTex;           // +0xF0 "L2UI.QuestWnd.questBack3" (under the grids)
    UTexture* BackTex;               // +0xF4 "L2UI.etcWnd.OptionBack"
    NCWnd* UnknownF8;                // +0xF8 destroyed by OnDestroy, never created
    NCEditBox* SearchEdit;           // +0xFC
    NCButton* SearchButton;          // +0x100 "Search"
    NCButton* ListButton;            // +0x104 "View List"
    User Characters[19];             // +0x108 the 18 default characters (race, gender, class
                                     //   from the creation's table; the 19th reads past it)
    ULevel* Level;                   // +0x27A0 the engine's level when created (not read)
    INT CurrentPage;                 // +0x27A4 category shown
    NCComboBox* WeaponTypeCombo;     // +0x27A8 "0(HAND)" .. "7(DUALFIST)"
    NCButton* UseButton;             // +0x27AC "Use": the weapon of that type on the character
    INT DefaultWeapons[8];           // +0x27B0 first item id found of each weapon type, -1 none

    NCPawnViewrWnd();                                                // 0x10088a50
    // ~NCPawnViewrWnd                                               // 0x10088b20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100874e0
    virtual INT OnCreate();                                          // 35 0x1008e230
    virtual INT OnDestroy();                                         // 36 0x10088dd0
    virtual void ShowWindow();                                       // 42 0x10089030
    virtual void HideWindow();                                       // 43 0x10087fd0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10088c10
    // names ours
    virtual INT vfn062() { return 1; }                               // 62 0x10005230 (folded) id 0xF3
    virtual INT OnCategoryTab(INT Index);                            // 63 0x10088b70
    virtual INT OnGUICheck();                                        // 64 0x10088d60
    virtual INT OnSearch();                                          // 65 0x1008ca60
    virtual INT OnViewList();                                        // 66 0x1008cc90
    virtual INT OnUseWeapon();                                       // 67 0x1008c2d0

    void HidePages();                                                // 0x100875f0 (name ours)
    void AddAllCharacter();                                          // 0x10087630
    INT GetItemCategory(NViewItemData* Data, INT* BodyPart, INT ItemID); // 0x10087e60 (name ours)
    void Clear();                                                    // 0x10087f10
    INT AddItem(INT Page, NItemInfo* Info);                          // 0x10088060
    void AddAllItem();                                               // 0x1008c830
};
NW_ASSERT_OFFSET(NCPawnViewrWnd, TextWnds, 0xB4);
NW_ASSERT_OFFSET(NCPawnViewrWnd, CharSelectWnd, 0xD4);
NW_ASSERT_OFFSET(NCPawnViewrWnd, SearchEdit, 0xFC);
NW_ASSERT_OFFSET(NCPawnViewrWnd, Characters, 0x108);
NW_ASSERT_OFFSET(NCPawnViewrWnd, Level, 0x27A0);
NW_ASSERT_OFFSET(NCPawnViewrWnd, DefaultWeapons, 0x27B0);
NW_ASSERT_SIZE(NCPawnViewrWnd, 0x27D0);

/*-----------------------------------------------------------------------------
    The viewer's helpers (inline in the stock code, names ours).
-----------------------------------------------------------------------------*/
namespace NPawnView
{
    // the console's PawnViewerWnd[0]
    FORCEINLINE NCPawnViewrWnd* Viewer()
    {
        return static_cast<NCPawnViewrWnd*>(GConsole->PawnViewerWnd[0]);
    }
    // the pawn the player's controller drives
    FORCEINLINE APawn* PlayerPawn() { return ViewActor()->Pawn; }
}

#pragma once
// NCPawnCreateWnd - the character creation screen (UClass "CPawnCreateWnd", 0x108; NConsoleWnd
// +0x36D0): the choices panel (NCPawnSetupWnd), the chosen class's description (NCPawnInfoWnd),
// "Create Character" and "Previous" in the lower right corner, and under the stage buttons that
// turn the chosen default character and zoom on its chest. The engine keeps 18 default
// characters, one per race / class type / gender (index (Race * 2 + ClassType) * 2 + Sex);
// choosing the race, class type or gender plays a lobby event that moves the camera
// (NCPawnSetupWnd::ExecCameraEvent).
//
// The four classes share one stretch of the stock code (0x10083c00 .. 0x10087410).

#include "nwindow/NControl.h"

class NString;
class NCButton;
class NCExButton;
class NCPushButton;
class NCEditBox;
class NCPawnCreateWnd;

// NCSelectCtrl - a choice between SysString items, stepped through with a button on each side
// (UClass "CSelectCtrl", 0x1B8). A step wraps around and reports the new index to the parent
// (NWM_COMMAND, lParam = index).
class NCSelectCtrl : public NControl
{
    NW_DECLARE_CLASS(NCSelectCtrl, NControl, 0, 0x10140cb8)

    NCPushButton* LeftButton;        // +0x1A0 "L2UI.LobbyWnd.LeftBtn"
    NCPushButton* RightButton;       // +0x1A4 "L2UI.LobbyWnd.rightBtn"
    INT Selected;                    // +0x1A8 -1 = none
    TArray<NString*> Items;          // +0x1AC

    NCSelectCtrl();                                                  // 0x10086df0
    // ~NCSelectCtrl                                                 // 0x10086eb0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100837f0
    virtual INT OnCreate();                                          // 35 0x10083ed0
    virtual INT OnDestroy();                                         // 36 0x10086370
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10084030
    // names ours
    virtual INT SelectPrev();                                        // 63 0x10083880
    virtual INT SelectNext();                                        // 64 0x100838d0

    // names ours
    void ClearItems();                                               // 0x100862d0
    void AddItem(INT SysStringID);                                   // 0x100863f0

    // inlined in the stock code (name ours): an index past the end keeps the old one
    void SetSelected(INT Index)
    {
        if (Index < Items.Num())
            Selected = Index;
        if (Index < 0)
            Selected = -1;
    }
};
NW_ASSERT_OFFSET(NCSelectCtrl, LeftButton, 0x1A0);
NW_ASSERT_OFFSET(NCSelectCtrl, Selected, 0x1A8);
NW_ASSERT_OFFSET(NCSelectCtrl, Items, 0x1AC);
NW_ASSERT_SIZE(NCSelectCtrl, 0x1B8);

// NCPawnInfoWnd - the description of the chosen class under the choices (UClass
// "CPawnInfoWnd", 0x498): a classinfo text wrapped at 228 pixels.
class NCPawnInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnInfoWnd, NCWnd, 0, 0x10140268)

    UTexture* BackTex;               // +0x94 "L2UI.LobbyWnd.MakingBack2"
    TCHAR Text[512];                 // +0x98

    NCPawnInfoWnd();                                                 // 0x100840d0
    // ~NCPawnInfoWnd                                                // 0x10084130

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (shared)
    virtual INT OnCreate();                                          // 35 0x10084180
    virtual INT OnDestroy();                                         // 36 0x10083930 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100841c0

    // names ours
    void SetText(const TCHAR* InText);                               // 0x10083940
    void ClearText();                                                // 0x10077fd0 (shared)
};
NW_ASSERT_OFFSET(NCPawnInfoWnd, Text, 0x98);
NW_ASSERT_SIZE(NCPawnInfoWnd, 0x498);

// NCPawnSetupWnd - the choices panel (UClass "CPawnSetupWnd", 0xD4): the name, then race, class
// type, gender, hair style, hair color and face, each an NCSelectCtrl. A choice clears the ones
// after it and enables the next one; hair and face are applied to the default character on the
// stage. Out of order choices get a system message.
class NCPawnSetupWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnSetupWnd, NCWnd, 0, 0x10140788)

    NCSelectCtrl* RaceSelect;        // +0x94 sysstring 170..174
    NCSelectCtrl* ClassTypeSelect;   // +0x98 175 warrior, 176 spellcaster (dwarves: warrior)
    NCSelectCtrl* SexSelect;         // +0x9C 177 male, 178 female
    NCSelectCtrl* HairStyleSelect;   // +0xA0 types A..D, men; A..F, women
    NCSelectCtrl* HairColorSelect;   // +0xA4 types A, B
    NCSelectCtrl* FaceSelect;        // +0xA8 types A, B
    INT UnknownAC;                   // +0xAC (0)
    UTexture* BackTex;               // +0xB0 "L2UI.LobbyWnd.MakingBack1"
    NCEditBox* NameEdit;             // +0xB4
    INT Race;                        // +0xB8 the choices, -1 = none yet
    INT ClassType;                   // +0xBC
    INT Sex;                         // +0xC0
    INT HairStyle;                   // +0xC4
    INT HairColor;                   // +0xC8
    INT Face;                        // +0xCC
    INT CameraStep;                  // +0xD0 last camera view played (ExecCameraEvent)

    NCPawnSetupWnd();                                                // 0x100842c0
    // ~NCPawnSetupWnd                                               // 0x10084360

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10083970
    virtual INT OnCreate();                                          // 35 0x10086f10
    virtual INT OnDestroy();                                         // 36 0x100845b0
    virtual void ShowWindow();                                       // 42 0x10083a90
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100843b0
    // a selector changed (names ours)
    virtual INT OnRace(INT Index);                                   // 62 0x10086630
    virtual INT OnClassType(INT Index);                              // 63 0x10086810
    virtual INT OnSex(INT Index);                                    // 64 0x10086a60
    virtual INT OnHairStyle(INT Index);                              // 65 0x10084850
    virtual INT OnHairColor(INT Index);                              // 66 0x10084b40
    virtual INT OnFace(INT Index);                                   // 67 0x10084e40

    // names ours
    INT GetCharacterType();                                          // 0x10083ab0
    void ResetPreviews();                                            // 0x10083ad0
    INT GetClassID();                                                // 0x10083b50
    void SetMakingRace(INT InRace);                                  // 0x10083bf0
    void Reset();                                                    // 0x10084760
    UBOOL GetCreateInfo(CharacterInfo* Info);                        // 0x10085190
    void ExecCameraEvent(INT Step);                                  // 0x10085310
    void InitSelectors();                                            // 0x10086490

private:
    NCPawnCreateWnd* CreateWnd();
};
NW_ASSERT_OFFSET(NCPawnSetupWnd, RaceSelect, 0x94);
NW_ASSERT_OFFSET(NCPawnSetupWnd, BackTex, 0xB0);
NW_ASSERT_OFFSET(NCPawnSetupWnd, Race, 0xB8);
NW_ASSERT_OFFSET(NCPawnSetupWnd, CameraStep, 0xD0);
NW_ASSERT_SIZE(NCPawnSetupWnd, 0xD4);

class NCPawnCreateWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPawnCreateWnd, NCWnd, 0, 0x1013fd40)

    NCPawnSetupWnd* SetupWnd;        // +0x94
    NCPawnInfoWnd* InfoWnd;          // +0x98
    RecommandedDataStr Recommended;  // +0x9C the last class template the server sent
    NCButton* CreateButton;          // +0xEC
    NCButton* PreviousButton;        // +0xF0
    NCExButton* LeftButton;          // +0xF4 turn the character while held
    NCExButton* RightButton;         // +0xF8
    NCButton* ZoomInButton;          // +0xFC one of the two is shown; both have the same id
    NCButton* ZoomOutButton;         // +0x100
    UBOOL bZoomed;                   // +0x104

    NCPawnCreateWnd();                                               // 0x10085740
    // ~NCPawnCreateWnd                                              // 0x100857a0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10083c00
    virtual INT OnCreate();                                          // 35 0x100857f0
    virtual INT OnDestroy();                                         // 36 0x10085e60
    virtual void ShowWindow();                                       // 42 0x10083e30
    virtual void HideWindow();                                       // 43 0x10083d40
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10085db0
    // names ours
    virtual INT OnPrevious();                                        // 62 0x10083d20
    virtual INT OnCreateCharacter();                                 // 63 0x10086000
    virtual INT OnTurnLeft(INT bPressed);                            // 64 0x10083e50
    virtual INT OnTurnRight(INT bPressed);                           // 65 0x10083e90
    virtual INT OnZoom();                                            // 66 0x10086130

    // names ours
    void SetRecommendedData(RecommandedDataStr Data);                // 0x10083d00
    void EndRecommendedData() {}                                     // 0x10045870 (empty, folded;
                                                                     //   the console calls it)
    void ShowViewButtons(UBOOL bShow);                               // 0x10083d60
    void Reset();                                                    // 0x10085fe0
};
NW_ASSERT_OFFSET(NCPawnCreateWnd, Recommended, 0x9C);
NW_ASSERT_OFFSET(NCPawnCreateWnd, CreateButton, 0xEC);
NW_ASSERT_OFFSET(NCPawnCreateWnd, ZoomOutButton, 0x100);
NW_ASSERT_SIZE(NCPawnCreateWnd, 0x108);

#pragma once
// NCStatusWnd - base of the HP / MP windows (UClass "CStatusWnd", 0x2FC): the player's
// (NCPlayerStatusWnd: bars, level, exp), the target's (NCTargetStatusWnd: name colored by the
// level difference, HP / MP of monsters and party members, clan and alliance) and one per
// party member (NCPartyStatusWnd, inside NCPartyWnd), the pet's (NCPetStatusWnd) and the
// summon's (NCSummonedStatusWnd). The base only keeps the textures and numbers the subclasses
// draw.
//
// Desktop (NCConsole) +0xC0 is the target window, +0xC4 the player window, +0xB4 the pet
// window, +0xB8 the summon window.

#include "nwindow/NCWnd.h"

class NCButton;
class NCResizeFrame;
class NCProgressCtrl;

class NCStatusWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCStatusWnd, NCWnd, 0, 0x1014cb30)

    UTexture* MPTex;                 // +0x94
    UTexture* HPTex;                 // +0x98
    UTexture* EXPTex;                // +0x9C
    UTexture* BackTex;               // +0xA0 "NWindow.WhiteTexture" (OnCreate), the subclasses
                                     //   load their own
    UTexture* BackTopTex;            // +0xA4 target window: background (top, center, bottom)
    UTexture* BackCenterTex;         // +0xA8
    UTexture* BackBottomTex;         // +0xAC
    UTexture* FrameTopTex;           // +0xB0 target window: left edge (top, center, bottom)
    UTexture* FrameCenterTex;        // +0xB4
    UTexture* FrameBottomTex;        // +0xB8
    UTexture* UpBtnTex;              // +0xBC target window: loaded, never drawn (the toggle
    UTexture* UpBtnClickTex;         // +0xC0   button loads its own)
    UTexture* DownBtnTex;            // +0xC4
    UTexture* DownBtnClickTex;       // +0xC8
    UTexture* LeftBackTex;           // +0xCC player window: edges
    UTexture* RightBackTex;          // +0xD0
    UTexture* MPBackTex;             // +0xD4 empty bars
    UTexture* HPBackTex;             // +0xD8
    INT HP;                          // +0xDC party member (SetInfo)
    INT MaxHP;                       // +0xE0
    INT MP;                          // +0xE4
    INT MaxMP;                       // +0xE8
    INT ObjectID;                    // +0xEC target window: whom a click targets / attacks
    INT BarWidth;                    // +0xF0 constructor argument (the target window sets 132)
    INT MaxExtraWidth;               // +0xF4 player window: how far it can be widened (202)
    INT ExtraWidth;                  // +0xF8 player window: widened by this (bars grow with it)
    TCHAR Name[256];                 // +0xFC party member, empty = free slot

    NCStatusWnd(INT InBarWidth = 100);                                     // 0x100af2b0
    // ~NCStatusWnd                                                  // 0x100aeb70

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x100af2f0
    virtual INT OnDestroy();                                         // 36 0x10083930 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100af320
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam) { return 1; } // 56 0x100098f0 (folded)

    // names ours
    UBOOL IsName(const TCHAR* InName);                               // 0x100ae300
    void Clear();                                                    // 0x100ae330
    UBOOL IsEmpty();                                                 // 0x100ae9a0
};
NW_ASSERT_OFFSET(NCStatusWnd, MPTex, 0x94);
NW_ASSERT_OFFSET(NCStatusWnd, LeftBackTex, 0xCC);
NW_ASSERT_OFFSET(NCStatusWnd, HP, 0xDC);
NW_ASSERT_OFFSET(NCStatusWnd, ObjectID, 0xEC);
NW_ASSERT_OFFSET(NCStatusWnd, ExtraWidth, 0xF8);
NW_ASSERT_OFFSET(NCStatusWnd, Name, 0xFC);
NW_ASSERT_SIZE(NCStatusWnd, 0x2FC);

// NWM_COMMAND ids of the status windows' controls
enum
{
    NID_PLAYERSTATUS_RESIZE = 0xF20F04,  // right edge of the player window
    NID_TARGETSTATUS_CLOSE = 0xFA,
    NID_TARGETSTATUS_TOGGLE = 0xFB,      // shows / hides the clan and alliance lines
};

// NCPlayerStatusWnd - the player's name, level, HP / MP / exp bars (UClass "CPlayerStatusWnd",
// 0x308). The right edge widens it (the bars grow with it); a click targets the player.
class NCPlayerStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPlayerStatusWnd, NCStatusWnd, 0, 0x1014daa0)

    User* Player;                    // +0x2FC set by each paint
    INT Unknown300;                  // +0x300 (0)
    NCResizeFrame* ResizeFrame;      // +0x304

    NCPlayerStatusWnd(INT InBarWidth = 100);                               // 0x100aff90
    // ~NCPlayerStatusWnd                                            // 0x100afff0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ae5b0
    virtual INT OnCreate();                                          // 35 0x100b0040
    virtual INT OnDestroy();                                         // 36 0x100b0970
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b0200
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100ae630
    // names ours
    virtual INT OnResize(INT Delta);                                 // 62 0x100b09b0

    INT Resize(INT Delta);                                           // 0x100ae620 (name ours)
};
NW_ASSERT_OFFSET(NCPlayerStatusWnd, Player, 0x2FC);
NW_ASSERT_OFFSET(NCPlayerStatusWnd, ResizeFrame, 0x304);
NW_ASSERT_SIZE(NCPlayerStatusWnd, 0x308);

// NCTargetStatusWnd - the target (UClass "CTargetStatusWnd", 0x310): its name, HP / MP bars
// for monsters, siege NPCs, the player, its pet and party members; a door or other static
// object shows its name only. For players the window can be expanded to show the clan and
// the alliance. A click targets again (Ctrl: attacks); the close button cancels the target.
class NCTargetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCTargetStatusWnd, NCStatusWnd, 0, 0x1014dfc0)

    NCButton* CloseButton;           // +0x2FC
    INT Unknown300;                  // +0x300
    INT LevelDiff;                   // +0x304 the player's level minus the target's (name color)
    UBOOL bExpanded;                 // +0x308 clan and alliance lines shown
    NCButton* ToggleButton;          // +0x30C

    NCTargetStatusWnd(INT InBarWidth = 100);                               // 0x100af3d0
    // ~NCTargetStatusWnd                                            // 0x100af430

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ae360
    virtual INT OnCreate();                                          // 35 0x100af480
    virtual INT OnDestroy();                                         // 36 0x100af780
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100af7c0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100ae3f0
    // names ours
    virtual INT OnClose();                                           // 62 0x100ae530
    virtual INT OnToggle();                                          // 63 0x100ae490

    void SetLevelDiff(INT Diff);                                     // 0x100ae480 (name ours)

private:
    void Collapse();                 // inlined in the stock OnPaint (name ours)
};
NW_ASSERT_OFFSET(NCTargetStatusWnd, CloseButton, 0x2FC);
NW_ASSERT_OFFSET(NCTargetStatusWnd, LevelDiff, 0x304);
NW_ASSERT_OFFSET(NCTargetStatusWnd, ToggleButton, 0x30C);
NW_ASSERT_SIZE(NCTargetStatusWnd, 0x310);

// NCPartyStatusWnd - one party member (UClass "CPartyStatusWnd", 0x300): name, HP / MP bars.
// A click targets the member (Ctrl: attacks), a right click targets what the member targets.
class NCPartyStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPartyStatusWnd, NCStatusWnd, 0, 0x1014d058)

    INT MemberID;                    // +0x2FC object id of the member (-1)

    NCPartyStatusWnd(INT InBarWidth = 100);                                // 0x100b13e0
    // ~NCPartyStatusWnd                                             // 0x100b1440

    virtual INT OnCreate();                                          // 35 0x100b1490
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b1500
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100ae820
    virtual INT OnRButtonUp(WPARAM wParam, LPARAM lParam);           // 59 0x100ae8b0

    void SetInfo(INT InMemberID, const TCHAR* InName, INT InHP, INT InMaxHP, INT InMP,
                 INT InMaxMP);                                       // 0x100ae940 (name ours)
};
NW_ASSERT_OFFSET(NCPartyStatusWnd, MemberID, 0x2FC);
NW_ASSERT_SIZE(NCPartyStatusWnd, 0x300);

// NCPetStatusWnd - the pet (UClass "CPetStatusWnd", 0x3D0): name, HP / MP bars and the food
// bar, from what the network handler keeps about the pet (read again by each paint). Shown
// only while the player has a pet (not a summon); a click right of the title bar acts on it.
class NCPetStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCPetStatusWnd, NCStatusWnd, 0, 0x1014f460)

    UTexture* FedTex;                // +0x2FC "L2UI.PetWnd.PetGreenBar1"
    UTexture* FedBackTex;            // +0x300 "L2UI.PetWnd.PetGreenBar2"
    PetInfo Pet;                     // +0x304 copied by each paint
    INT Unknown3B8;                  // +0x3B8 (0)
    INT Unknown3BC[2];               // +0x3BC
    INT Unknown3C4;                  // +0x3C4 SetInfo
    INT Unknown3C8;                  // +0x3C8
    INT Unknown3CC;                  // +0x3CC

    NCPetStatusWnd(INT InBarWidth = 100);                                  // 0x100b0a50
    // ~NCPetStatusWnd                                               // 0x100b0ab0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                          // 35 0x100b0b00
    virtual void ShowWindow();                                       // 42 0x100ae730
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b0c30
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100ae6d0

    void SetInfo(L2ParamStack& Params);                              // 0x100ae650 (name ours)
};
NW_ASSERT_OFFSET(NCPetStatusWnd, Pet, 0x304);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown3B8, 0x3B8);
NW_ASSERT_OFFSET(NCPetStatusWnd, Unknown3C4, 0x3C4);
NW_ASSERT_SIZE(NCPetStatusWnd, 0x3D0);

// NCSummonedStatusWnd - the summon (UClass "CSummonedStatusWnd", 0x3BC): name, HP / MP bars
// and a bar for the time it has left, run by the console (StartTimer, SetRemaining). Shown only
// while the player has a summon (not a pet).
class NCSummonedStatusWnd : public NCStatusWnd
{
    NW_DECLARE_CLASS(NCSummonedStatusWnd, NCStatusWnd, 0, 0x1014d578)

    NCProgressCtrl* TimeBar;         // +0x2FC
    PetInfo Summon;                  // +0x300 copied by each paint
    UBOOL bTimerStarted;             // +0x3B4 (StartTimer)
    INT Unknown3B8;                  // +0x3B8 (0)

    NCSummonedStatusWnd(INT InBarWidth = 100);                             // 0x100b0ec0
    // ~NCSummonedStatusWnd                                          // 0x100b0f20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100051f0 (folded)
    virtual INT OnCreate();                                          // 35 0x100b0f70
    virtual INT OnDestroy();                                         // 36 0x100b1140
    virtual void ShowWindow();                                       // 42 0x100ae7e0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b11b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100ae780

    // names ours
    void SetRemaining(DWORD Remaining);                              // 0x100ae770
    void StartTimer(DWORD Max);                                      // 0x100b1180
};
NW_ASSERT_OFFSET(NCSummonedStatusWnd, Summon, 0x300);
NW_ASSERT_OFFSET(NCSummonedStatusWnd, bTimerStarted, 0x3B4);
NW_ASSERT_SIZE(NCSummonedStatusWnd, 0x3BC);

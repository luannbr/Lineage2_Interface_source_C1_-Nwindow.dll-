#pragma once
// NCGameOptionWnd - the game page of the options window (UClass "CGameOptionWnd", 0xD4):
// language (native or English), which names show over characters (player, monsters, other PCs
// - and among those clan, party, general), camera tracking, the chat input options, a button
// that puts every window back in place, party loot and the game pad. The settings live in Core
// globals (GL2Show*, GIsEnterChatting...) and in Option.ini [Game].

#include "nwindow/NCWnd.h"

class NCComboBox;
class NCButton;
class NCCheckBox;

// NameBoxes[] (names ours): "Other PCs" switches the last three
enum
{
    NAMEBOX_Player,
    NAMEBOX_Monster,
    NAMEBOX_OtherPC,
    NAMEBOX_Clan,
    NAMEBOX_Party,
    NAMEBOX_General,
    NAMEBOX_MAX
};

// NWM_COMMAND ids of the page's controls
enum
{
    NID_GAMEOPT_LANGUAGE = 0x1F15,
    NID_GAMEOPT_PLAYER = (INT)0xF0F01459,
    NID_GAMEOPT_MONSTER = (INT)0xF0F01460,
    NID_GAMEOPT_OTHERPC = (INT)0xF0F01461,
    NID_GAMEOPT_CLAN = (INT)0xF0F01462,
    NID_GAMEOPT_PARTY = (INT)0xF0F01463,
    NID_GAMEOPT_GENERAL = (INT)0xF0F01464,
    NID_GAMEOPT_TRACKING = (INT)0xF0F01470,
    NID_GAMEOPT_ENTERCHAT = (INT)0xF0F01471,
    NID_GAMEOPT_OLDCHAT = (INT)0xF0F01472,
    NID_GAMEOPT_GAMEPAD = (INT)0xF0F01473,
    NID_GAMEOPT_RESET = (INT)0xFFFF1400,
    NID_GAMEOPT_PARTYLOOT = (INT)0xFFFF1401,
};

class NCGameOptionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGameOptionWnd, NCWnd, 0, 0x1013f2f0)

    NCComboBox* LanguageCombo;             // +0x94 0 native, 1 English
    NCButton* ResetButton;                 // +0x98 "Initialize"
    NCWnd* Unused9C[3];                    // +0x9C destroyed, never created
    NCCheckBox* NameBoxes[NAMEBOX_MAX];    // +0xA8
    NCCheckBox* EnterChatBox;              // +0xC0 GIsEnterChatting
    NCCheckBox* OldChatBox;                // +0xC4 GIsOldChat
    NCCheckBox* TrackingBox;               // +0xC8 GL2UseAutoTrackingPawn
    NCComboBox* PartyLootCombo;            // +0xCC GL2PartyLooting: 0 finders keepers, 1 random
    NCCheckBox* GamePadBox;                // +0xD0 only with a game pad (GL2CanUseJoystick)

    NCGameOptionWnd();                                               // 0x10080970
    // ~NCGameOptionWnd                                              // 0x100809d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1007dd70
    virtual INT OnCreate();                                          // 35 0x10082f10
    virtual INT OnDestroy();                                         // 36 0x10080de0
    virtual void ShowWindow();                                       // 42 0x1007de20
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10080a20
    // names ours
    virtual INT OnOtherPCClicked();                                  // 62 0x10081750
    virtual INT OnReset();                                           // 63 0x1007de00

    // names ours
    void LoadSettings();                                             // 0x10081010
    void Apply();                                                    // 0x100813a0
    void InitItems();                                                // 0x10082440
};
NW_ASSERT_OFFSET(NCGameOptionWnd, Unused9C, 0x9C);
NW_ASSERT_OFFSET(NCGameOptionWnd, NameBoxes, 0xA8);
NW_ASSERT_OFFSET(NCGameOptionWnd, EnterChatBox, 0xC0);
NW_ASSERT_OFFSET(NCGameOptionWnd, PartyLootCombo, 0xCC);
NW_ASSERT_SIZE(NCGameOptionWnd, 0xD4);

#pragma once
// NCMatchWnd - party matching (UClass "CMatchWnd", 0xB4): the list of players looking for a
// party (NCMatchListCtrl) and Invite, Options, Whisper and Targeting on the selected one. The
// console fills it: ClearList starts a new list (remembering the selection), AddPlayer adds
// each player the server sends, UpdatePlayerInfo stores the details asked for on a click.

#include "nwindow/NCWnd.h"

class NCMatchListCtrl;
class NCButton;

// NWM_COMMAND ids of the buttons (names ours)
enum
{
    NID_MATCH_INVITE = 0xF2,
    NID_MATCH_OPTIONS = 0xF3,
    NID_MATCH_WHISPER = 0xF4,
    NID_MATCH_TARGETING = 0xF5,
};

class NCMatchWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCMatchWnd, NCWnd, 0, 0x1013a5b0)

    INT Unknown94;                   // +0x94 not used
    UTexture* BackTex;               // +0x98 "L2UI.MatchWnd.MatchingBack"
    NCMatchListCtrl* MatchList;      // +0x9C
    NCButton* InviteButton;          // +0xA0
    NCButton* OptionsButton;         // +0xA4
    NCButton* WhisperButton;         // +0xA8
    NCButton* TargetingButton;       // +0xAC
    INT SelectedID;                  // +0xB0 object ID selected when the list was cleared,
                                     //   selected again when it comes back

    NCMatchWnd();                                                    // 0x10075110
    // ~NCMatchWnd                                                   // 0x10075180

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100744f0
    virtual INT OnCreate();                                          // 35 0x10075f70
    virtual INT OnDestroy();                                         // 36 0x100751d0
    virtual void ShowWindow();                                       // 42 0x10078950 (folded)
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100752c0
    // the buttons (names ours)
    virtual INT OnInvite();                                          // 62 0x100757d0
    virtual INT OnOptions();                                         // 63 0x100745c0
    virtual INT OnWhisper();                                         // 64 0x10075870
    virtual INT OnTargeting();                                       // 65 0x10075990

    // the console's calls (names ours)
    void AddPlayer(L2ParamStack& Params);                            // 0x10075350
    void ClearList();                                                // 0x100757b0
    void UpdatePlayerInfo(L2ParamStack& Params);                     // 0x100765f0
};
NW_ASSERT_OFFSET(NCMatchWnd, BackTex, 0x98);
NW_ASSERT_OFFSET(NCMatchWnd, MatchList, 0x9C);
NW_ASSERT_OFFSET(NCMatchWnd, SelectedID, 0xB0);
NW_ASSERT_SIZE(NCMatchWnd, 0xB4);

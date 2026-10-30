#pragma once
// NCSiegeInfoMainWnd - the castle siege window (UClass "CSiegeInfoMainWnd", 0x3C0; desktop
// NCConsole +0x110): the castle's name and its owner (lord, clan and alliance, with their
// crests) over three tabs: the siege date, the attacking clans and the defending clans. The
// server's siege info (NConsoleWnd slot 190) opens it (SetCastleInfo); the tab of a side asks
// for its list once, and the console fills it (AddAttacker / AddDefender, slots 191 / 192)
// and then updates its buttons (slots 194 / 195).
//
// NCSiegePartInfoWnd - a side's tab (UClass "CSiegePartInfoWnd", 0xD0): the clans in an
// NCSiegeInfoListCtrl, their count, and the buttons: apply / cancel the application for
// everybody but the owner; on the defending side the owner approves or refuses the clans
// waiting. Each button asks first (NCDialogBox ids 0x3A..0x3F answer the server).
//
// NCSiegeDateInfoWnd - the date tab (UClass "CSiegeDateInfoWnd", 0xB8): the current time
// and the siege's; while no date is set, the owner picks one in a combo box among those the
// server offers (slot 193 adds them), after a confirmation.
//
// NCSiegeInfoListCtrl - the clan list of a side (UClass "CSiegeInfoListCtrl", 0x20C): the
// clan's crest and name, then the alliance name after its crest (attackers) or the status
// (defenders); the tool tip of a row has the leaders, the alliance and the date of the
// application.
//
// Row data (NCSiegePartInfoWnd::AddClan): Cells[0] clan name, Text2 its leader, Param1 the
// application time, Param2 the side (1 = defenders); Cells[1] alliance name or status, Text2
// the alliance's leader; row Text the alliance name, Param1 the clan's pledge id, Param2
// NSiegeClanInfo::CrestID.

#include "nwindow/NCListCtrl.h"

class NCButton;
class NCComboBox;
class NCTabControl;

// The server's siege info of a castle (name ours; 0x318 bytes, NConsoleWnd slot 190 builds
// it on its stack).
struct NSiegeCastleInfo
{
    TCHAR LordName[128];       // +0x000 the owner clan's leader
    TCHAR ClanName[128];       // +0x100 the owner clan
    TCHAR AllianceName[128];   // +0x200 its alliance
    INT CastleID;              // +0x300
    UBOOL bIsOwner;            // +0x304 the player's clan owns the castle
    INT PledgeID;              // +0x308 the owner clan (the crests are drawn from it)
    INT AllianceID;            // +0x30C (not used)
    INT CurrentTime;           // +0x310 server time
    INT SiegeTime;             // +0x314 0 while not set

    // inlined (the constructor in the console's slot 190 and the window's, Clear in Reset)
    NSiegeCastleInfo() { Clear(); }
    void Clear()
    {
        CastleID = 0;
        bIsOwner = 0;
        PledgeID = 0;
        AllianceID = 0;
        CurrentTime = 0;
        SiegeTime = 0;
        LordName[0] = 0;
        ClanName[0] = 0;
        AllianceName[0] = 0;
    }
    NSiegeCastleInfo& operator=(const NSiegeCastleInfo& Other);      // 0x100ab750
};
NW_ASSERT_OFFSET(NSiegeCastleInfo, CastleID, 0x300);
NW_ASSERT_OFFSET(NSiegeCastleInfo, SiegeTime, 0x314);
NW_ASSERT_SIZE(NSiegeCastleInfo, 0x318);

// NSiegeClanInfo::Type on the defending side (names ours)
enum
{
    SIEGECLAN_Owner = 1,
    SIEGECLAN_Waiting = 2,
    SIEGECLAN_Approved = 3,
    SIEGECLAN_Refused = 4,
};

// A clan of a side's list (name ours; 0x414 bytes, NConsoleWnd slots 191 / 192 build it on
// their stack; the tab keeps a copy of each).
struct NSiegeClanInfo
{
    INT PledgeID;              // +0x000
    TCHAR ClanName[128];       // +0x004
    TCHAR LeaderName[128];     // +0x104
    INT CrestID;               // +0x204 (the row's Param2, not read)
    INT ApplyTime;             // +0x208 when it applied, 0 = not known
    INT Type;                  // +0x20C defenders: SIEGECLAN_*
    INT AllianceID;            // +0x210 (not used)
    TCHAR AllianceName[128];   // +0x214
    TCHAR AllianceLeader[128]; // +0x314

    // inlined in the console's slots 191 / 192
    NSiegeClanInfo()
    {
        PledgeID = 0;
        ClanName[0] = 0;
        LeaderName[0] = 0;
        CrestID = 0;
        ApplyTime = 0;
        Type = 0;
        AllianceID = 0;
        AllianceName[0] = 0;
        AllianceLeader[0] = 0;
    }
    NSiegeClanInfo(const NSiegeClanInfo& Other);                     // 0x100ab7e0
};
NW_ASSERT_OFFSET(NSiegeClanInfo, LeaderName, 0x104);
NW_ASSERT_OFFSET(NSiegeClanInfo, Type, 0x20C);
NW_ASSERT_OFFSET(NSiegeClanInfo, AllianceName, 0x214);
NW_ASSERT_OFFSET(NSiegeClanInfo, AllianceLeader, 0x314);
NW_ASSERT_SIZE(NSiegeClanInfo, 0x414);

// NCSiegePartInfoWnd::Side (names ours)
enum
{
    SIEGESIDE_Attacker = 0,
    SIEGESIDE_Defender = 1,
};

class NCSiegeInfoListCtrl : public NCListCtrl
{
    NW_DECLARE_CLASS(NCSiegeInfoListCtrl, NCListCtrl, 0, 0x1014c0e0)

    NCSiegeInfoListCtrl();                                           // 0x100ad610
    NCSiegeInfoListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x100ad6b0
    // ~NCSiegeInfoListCtrl                                          // 0x100ad660

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ab3a0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100ab940
    virtual void DrawRowToolTip(NCGDevice* Device, NListRow* Row, INT X, INT Y); // 69 0x100ab530

    // names ours
    void BuildToolTip(NListRow* Row);                                // 0x100ab3e0
    INT GetSelectedPledgeID();                                       // 0x100ab870 0: none
    INT GetSelectedStatus();                                         // 0x100ab8a0 SIEGECLAN_Approved
                                                                     //   / _Waiting by the status
                                                                     //   text, -1 otherwise
};
NW_ASSERT_SIZE(NCSiegeInfoListCtrl, 0x20C);

class NCSiegePartInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSiegePartInfoWnd, NCWnd, 0, 0x1014bbb8)

    UTexture* BackTex;               // +0x94 "L2UI.SiegeWnd.siege_back2"
    NCSiegeInfoListCtrl* List;       // +0x98
    INT Side;                        // +0x9C SIEGESIDE_* (SetSide)
    NCButton* ApplyButton;           // +0xA0
    NCButton* CancelApplyButton;     // +0xA4
    NCButton* ApproveButton;         // +0xA8
    NCButton* RefuseButton;          // +0xAC
    BYTE bRegistered;                // +0xB0 the player's clan is in the list
    INT NumApproved;                 // +0xB4 defenders
    INT NumWaiting;                  // +0xB8 defenders (the owner counts here too)
    TArray<NSiegeClanInfo*> Clans;   // +0xBC copies of what AddClan got (never read back)
    INT CastleID;                    // +0xC8
    BYTE bListed;                    // +0xCC the list was asked for: the tab does not ask again

    NCSiegePartInfoWnd();                                            // 0x100ad6e0
    // ~NCSiegePartInfoWnd                                           // 0x100ad790

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ab570
    virtual INT OnCreate();                                          // 35 0x100ad7f0
    virtual INT OnDestroy();                                         // 36 0x100acea0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100abc70
    // names ours
    virtual INT OnApply();                                           // 62 0x100abf20
    virtual INT OnCancelApply();                                     // 63 0x100abfd0
    virtual INT OnApprove();                                         // 64 0x100ac070
    virtual INT OnRefuse();                                          // 65 0x100ac150

    // names ours
    void SetSide(INT InSide);                                        // 0x100abdc0 and the columns
    void UpdateButtons();                                            // 0x100ac240
    void Reset();                                                    // 0x100acfe0
    void ClearList();                                                // 0x100ad060
    void AddClan(NSiegeClanInfo* Info);                              // 0x100ad0a0
};
NW_ASSERT_OFFSET(NCSiegePartInfoWnd, List, 0x98);
NW_ASSERT_OFFSET(NCSiegePartInfoWnd, bRegistered, 0xB0);
NW_ASSERT_OFFSET(NCSiegePartInfoWnd, Clans, 0xBC);
NW_ASSERT_OFFSET(NCSiegePartInfoWnd, bListed, 0xCC);
NW_ASSERT_SIZE(NCSiegePartInfoWnd, 0xD0);

class NCSiegeDateInfoWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSiegeDateInfoWnd, NCWnd, 0, 0x1014c608)

    UTexture* BackTex;               // +0x94 "L2UI.SiegeWnd.siege_back3"
    TArray<INT> Times;               // +0x98 the dates offered, the combo's items after the first
    NCComboBox* TimeCombo;           // +0xA4
    INT CastleID;                    // +0xA8
    INT SelectedIndex;               // +0xAC combo item chosen, 0 = automatic setting
    INT CurrentTime;                 // +0xB0
    INT SiegeTime;                   // +0xB4 0 while not set

    NCSiegeDateInfoWnd();                                            // 0x100ad2d0
    // ~NCSiegeDateInfoWnd                                           // 0x100ad370

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ab640
    virtual INT OnCreate();                                          // 35 0x100ac380
    virtual INT OnDestroy();                                         // 36 0x100ac430
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100ac470
    // names ours
    virtual INT OnSelectTime(INT Index);                             // 62 0x100ac6a0
    virtual INT OnConfirmTime(WPARAM wParam, LPARAM lParam);         // 63 0x100ac730

    // names ours
    void Reset();                                                    // 0x100ac630
    void SetTimes(INT InCurrentTime, INT InSiegeTime);               // 0x100ac7d0
    void AddTime(INT Time);                                          // 0x100ad3d0
};
NW_ASSERT_OFFSET(NCSiegeDateInfoWnd, Times, 0x98);
NW_ASSERT_OFFSET(NCSiegeDateInfoWnd, TimeCombo, 0xA4);
NW_ASSERT_SIZE(NCSiegeDateInfoWnd, 0xB8);

class NCSiegeInfoMainWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSiegeInfoMainWnd, NCWnd, 0, 0x1014b698)

    UTexture* BackTex;               // +0x94 "L2UI.SiegeWnd.Siege_Back1"
    NSiegeCastleInfo Castle;         // +0x98
    NCTabControl* TabCtrl;           // +0x3B0
    NCSiegePartInfoWnd* AttackerInfo; // +0x3B4
    NCSiegePartInfoWnd* DefenderInfo; // +0x3B8
    NCSiegeDateInfoWnd* DateInfo;    // +0x3BC

    NCSiegeInfoMainWnd();                                            // 0x100ac830
    // ~NCSiegeInfoMainWnd                                           // 0x100ac8f0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ab6e0
    virtual INT OnCreate();                                          // 35 0x100adb80
    virtual INT OnDestroy();                                         // 36 0x100ac940
    virtual void ShowWindow();                                       // 42 0x100ad530
    virtual void HideWindow();                                       // 43 0x100ad520
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100aca10
    virtual INT OnTab(INT Tab);                                      // 62 0x100accd0 (name ours)

    // names ours (the console calls the last three)
    void Reset();                                                    // 0x100ad430
    void SetCastleInfo(NSiegeCastleInfo* Info);                      // 0x100ad490
    void AddAttacker(NSiegeClanInfo* Info);                          // 0x100ad500
    void AddDefender(NSiegeClanInfo* Info);                          // 0x100ad510
};
NW_ASSERT_OFFSET(NCSiegeInfoMainWnd, Castle, 0x98);
NW_ASSERT_OFFSET(NCSiegeInfoMainWnd, TabCtrl, 0x3B0);
NW_ASSERT_OFFSET(NCSiegeInfoMainWnd, DateInfo, 0x3BC);
NW_ASSERT_SIZE(NCSiegeInfoMainWnd, 0x3C0);

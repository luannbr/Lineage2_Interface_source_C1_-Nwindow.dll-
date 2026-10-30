#pragma once
// NCPledgeWnd - the clan page of the action window (UClass "CPledgeWnd", 0x3B0): crest, name,
// leader, level, castle / hideout, the member list (name, level, on / out; NCListCtrl) and
// nine buttons: invite, title, crest, war, truce, surrender, expel, yield, leave. Only the
// clan leader gets the first seven; only the others get Leave. The console fills it
// (NConsoleWnd::ReceivePledgeInfo, AddPledgeMember...). NCActionWnd +0x9C is this window.

#include "nwindow/NCWnd.h"

class NCListCtrl;
class NCButton;

// What the console hands over about the clan (name ours; 0xEC bytes, built on the stack by
// NConsoleWnd::ReceivePledgeInfo). The window keeps a copy (Info) but SetPledgeInfo copies
// only part of it.
struct NPledgeInfo
{
    TCHAR Name[32];            // +0x00
    INT PledgeID;              // +0x40 the crest texture is looked up by it
    INT CrestID;               // +0x44 > 0: the clan has a crest
    TCHAR LeaderName[32];      // +0x48
    INT Level;                 // +0x88
    UBOOL bHasCastle;          // +0x8C
    UBOOL bHasHideout;         // +0x90
    INT Unknown94;             // +0x94
    INT Unknown98;             // +0x98
    TCHAR StatusText[32];      // +0x9C "In War", "In Siege", "Dispersing"... (not copied)
    INT Status;                // +0xDC index the console picked StatusText by
    INT UnknownE0;             // +0xE0 (not copied)
    INT UnknownE4;             // +0xE4 (not copied)
    INT UnknownE8;             // +0xE8

    // inlined in the console's slots 75, 76 and 203 (UnknownE8 is left)
    NPledgeInfo()
    {
        Name[0] = 0;
        LeaderName[0] = 0;
        StatusText[0] = 0;
        PledgeID = 0;
        CrestID = 0;
        UnknownE4 = 0;
        Level = 0;
        bHasCastle = 0;
        bHasHideout = 0;
        Unknown94 = 0;
        Unknown98 = 0;
        Status = 0;
        UnknownE0 = 0;
    }
};
NW_ASSERT_OFFSET(NPledgeInfo, PledgeID, 0x40);
NW_ASSERT_OFFSET(NPledgeInfo, LeaderName, 0x48);
NW_ASSERT_OFFSET(NPledgeInfo, StatusText, 0x9C);
NW_ASSERT_SIZE(NPledgeInfo, 0xEC);

// A member as the console hands it over (name ours; 0x214 bytes, built on the stack by
// NConsoleWnd::AddPledgeMember)
struct NPledgeMember
{
    TCHAR Name[256];           // +0x000
    INT Level;                 // +0x200
    INT ClassID;               // +0x204 (-1)
    INT Unknown208;            // +0x208 (-1)
    INT Unknown20C;            // +0x20C (-1)
    UBOOL bOnline;             // +0x210

    // inlined in the console's slots 78, 79 and 205
    NPledgeMember()
    {
        Name[0] = 0;
        Level = 0;
        ClassID = -1;
        Unknown208 = -1;
        Unknown20C = -1;
        bOnline = 0;
    }
};
NW_ASSERT_OFFSET(NPledgeMember, Level, 0x200);
NW_ASSERT_OFFSET(NPledgeMember, bOnline, 0x210);
NW_ASSERT_SIZE(NPledgeMember, 0x214);

// NWM_COMMAND ids of the buttons (names ours)
enum
{
    NID_PLEDGE_CREST = 0xF00C,
    NID_PLEDGE_WAR = 0xF00D,
    NID_PLEDGE_TRUCE = 0xF00E,
    NID_PLEDGE_SURRENDER = 0xF010,
    NID_PLEDGE_INVITE = 0xF011,
    NID_PLEDGE_YIELD = 0xF012,
    NID_PLEDGE_TITLE = 0xF013,
    NID_PLEDGE_EXPEL = 0xF014,
    NID_PLEDGE_LEAVE = 0xF015,
};

class NCPledgeWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPledgeWnd, NCWnd, 0, 0x1012a870)

    UTexture* BackTex;               // +0x94 "L2UI.ActionWnd.PledgeBack"
    UTexture* CrestTex;              // +0x98 (NULL) NCGMPledgeWnd: the crest found by each paint
    NPledgeInfo Info;                // +0x9C
    NCListCtrl* MemberList;          // +0x188
    NCButton* InviteButton;          // +0x18C
    NCButton* TitleButton;           // +0x190
    NCButton* CrestButton;           // +0x194
    NCButton* WarButton;             // +0x198
    NCButton* TruceButton;           // +0x19C
    NCButton* SurrenderButton;       // +0x1A0
    NCButton* ExpelButton;           // +0x1A4
    NCButton* YieldButton;           // +0x1A8 always enabled
    NCButton* LeaveButton;           // +0x1AC
    BYTE _Unknown1B0[0x3B0 - 0x1B0]; // +0x1B0 not used by this class

    NCPledgeWnd();                                                   // 0x10034f50
    // ~NCPledgeWnd                                                  // 0x10035000

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10033da0
    virtual INT OnCreate();                                          // 35 0x10035050
    virtual INT OnDestroy();                                         // 36 0x10035c10
    virtual void ShowWindow();                                       // 42 0x10033f30
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10035880
    // the buttons (names ours): dialogs NCDialogBox answers by ID, or messages
    virtual INT OnInvite();                                          // 62 0x10034210
    virtual INT OnSetCrest();                                        // 63 0x10036180
    virtual INT OnDeclareWar();                                      // 64 0x100361e0
    virtual INT OnEndWar();                                          // 65 0x10036240
    virtual INT OnSurrender();                                       // 66 0x100362a0
    virtual INT OnExpel();                                           // 67 0x10036300
    virtual INT OnSurrenderPersonally();                             // 68 0x100363b0
    virtual INT OnWithdraw();                                        // 69 0x10036410
    virtual INT OnGiveTitle();                                       // 70 0x100360e0
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam) { return 1; } // 71 0x100098f0 (folded)

    // the console's calls (names ours)
    void SetPledgeInfo(NPledgeInfo* InInfo);                         // 0x10034060
    void UpdatePledgeInfo(NPledgeInfo* InInfo);                      // 0x10034110
    const TCHAR* GetPledgeName();                                    // 0x10034180 NULL: no clan
    void ClearMembers();                                             // 0x100341a0
    void ClearInfo();                                                // 0x100341b0
    void Reset();                                                    // 0x10035de0
    void AddMember(NPledgeMember* Member);                           // 0x10035df0
    void UpdateMember(NPledgeMember* Member);                        // 0x10035fe0
    void DeleteMember(const TCHAR* Name);                            // 0x10036dc0
};
NW_ASSERT_OFFSET(NCPledgeWnd, Info, 0x9C);
NW_ASSERT_OFFSET(NCPledgeWnd, MemberList, 0x188);
NW_ASSERT_OFFSET(NCPledgeWnd, LeaveButton, 0x1AC);
NW_ASSERT_SIZE(NCPledgeWnd, 0x3B0);

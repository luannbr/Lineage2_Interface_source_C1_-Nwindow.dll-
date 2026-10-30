#pragma once
// NCPartyWnd - the party (UClass "CPartyWnd", 0xD0): a column of eight NCPartyStatusWnd, one
// per member, under a vertical title bar on the left. The console hands it the server's party
// packets (AddMember, UpdateMember, DeleteMember, DeleteAll); the window grows and shrinks with
// the members shown.
//
// Desktop (NCConsole) +0xB0 is the party window; the console's vslots 130..133 (add, update,
// delete, delete all) forward the party packets here.

#include "nwindow/NCWnd.h"

class NCPartyStatusWnd;

enum { NUM_PARTY_SLOTS = 8 };

class NCPartyWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPartyWnd, NCWnd, 0, 0x1014e4e8)

    INT Unknown94;                   // +0x94
    UTexture* FrameTopTex;           // +0x98 left edge, shown while there are members
    UTexture* FrameCenterTex;        // +0x9C
    UTexture* FrameBottomTex;        // +0xA0
    INT NumMembers;                  // +0xA4
    UBOOL bShown;                    // +0xA8 members listed (0: only the title bar)
    NCPartyStatusWnd* Members[NUM_PARTY_SLOTS]; // +0xAC filled from the top, empty = no name
    INT UnknownCC;                   // +0xCC

    NCPartyWnd();                                                    // 0x100b1720
    // ~NCPartyWnd                                                   // 0x100b1770

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100ae9b0
    virtual INT OnCreate();                                          // 35 0x100b17c0
    virtual INT OnDestroy();                                         // 36 0x100b1940
    virtual void ShowWindow();                                       // 42 0x100b1bd0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100b1990
    virtual INT OnToggle();                                          // 62 0x100b36a0 (name ours)

    // names ours
    void UpdateMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                      INT MaxMP);                                    // 0x100aea20 found by name
    void UpdateLayout();                                             // 0x100b1a70
    void AddMember(INT MemberID, const TCHAR* Name, INT HP, INT MaxHP, INT MP,
                   INT MaxMP);                                       // 0x100b34f0
    void DeleteMember(INT MemberID, const TCHAR* Name);              // 0x100b3570 by name, the
                                                                     //   id is not read
    void DeleteAll();                                                // 0x100b3650
};
NW_ASSERT_OFFSET(NCPartyWnd, FrameTopTex, 0x98);
NW_ASSERT_OFFSET(NCPartyWnd, NumMembers, 0xA4);
NW_ASSERT_OFFSET(NCPartyWnd, Members, 0xAC);
NW_ASSERT_SIZE(NCPartyWnd, 0xD0);

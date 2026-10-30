#pragma once
// NCPetitionWnd - the player's petition window (UClass "CPetitionWnd", 0xA8; NConsoleWnd
// +0x36EC): the conversation with the GM (NCPetitionListBox), a line to write in (Enter sends
// it as chat type 6) and "Cancel petition" (sysstring 473). NCGMPetitionWnd is the GM's side
// (desktop +0xAC, the same layout): its line takes "//" builder commands too, and "Start" /
// "End" (474 / 475) send the builder commands startpet / endpet. The desktop adds the
// server's petition chat to them (AddMessage).

#include "nwindow/NCWnd.h"

class NCEditBox;
class NCPetitionListBox;
class NCButton;

// NWM_COMMAND ids of the two windows' controls
enum
{
    NID_PETITION_LIST = 0xF0A0B1,
    NID_PETITION_EDIT = 0xF0A0B2,
    NID_PETITION_SEND = 0xF0A0B3,      // no such control (the command does nothing)
    NID_PETITION_CANCEL = 0xF0A0B4,
    NID_PETITION_FRAME = 0xF0A0B5,
    NID_GMPETITION_LIST = 0xF0A0C1,
    NID_GMPETITION_EDIT = 0xF0A0C2,
    NID_GMPETITION_START = 0xF0A0C3,
    NID_GMPETITION_END = 0xF0A0C4,
    NID_GMPETITION_FRAME = 0xF0A0C5,
};

class NCPetitionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCPetitionWnd, NCWnd, 0, 0x10146438)

    UTexture* BackTex;                     // +0x94 "L2UI.etcwnd.Petitionback"
    NCEditBox* EditBox;                    // +0x98
    NCPetitionListBox* ListBox;            // +0x9C
    NCButton* UnusedA0;                    // +0xA0 (NCGMPetitionWnd's Start button)
    NCButton* CancelButton;                // +0xA4

    NCPetitionWnd();                                                 // 0x100472e0
    // ~NCPetitionWnd                                                // 0x10047330

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10094380
    virtual INT OnCreate();                                          // 35 0x10094f10
    virtual INT OnDestroy();                                         // 36 0x100949c0
    virtual void ShowWindow();                                       // 42 0x10078950 (folded)
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10094a60
    // names ours
    virtual INT OnEdit(LPARAM lParam);                               // 62 0x10094430 1: Enter
    virtual INT OnSend() { return 1; }                               // 63 0x10005230 (folded)
    virtual INT OnCancel();                                          // 64 0x100944e0

    // names ours (also called on the desktop's NCGMPetitionWnd)
    void AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param);  // 0x10094480
    void Clear();                                                    // 0x100944c0
};
NW_ASSERT_OFFSET(NCPetitionWnd, EditBox, 0x98);
NW_ASSERT_OFFSET(NCPetitionWnd, CancelButton, 0xA4);
NW_ASSERT_SIZE(NCPetitionWnd, 0xA8);

class NCGMPetitionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCGMPetitionWnd, NCWnd, 0, 0x10146e88)

    UTexture* BackTex;                     // +0x94 "L2UI.etcwnd.Petitionback"
    NCEditBox* EditBox;                    // +0x98
    NCPetitionListBox* ListBox;            // +0x9C
    NCButton* StartButton;                 // +0xA0
    NCButton* EndButton;                   // +0xA4

    NCGMPetitionWnd();                                               // 0x1003e810
    // ~NCGMPetitionWnd                                              // 0x1003e860

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10094500
    virtual INT OnCreate();                                          // 35 0x100951f0
    virtual INT OnDestroy();                                         // 36 0x10094af0
    virtual void ShowWindow();                                       // 42 0x10078950 (folded)
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10094a60 (folded)
    // names ours
    virtual INT OnEdit(LPARAM lParam);                               // 62 0x100945b0 1: Enter
    virtual INT OnStart();                                           // 63 0x100946e0
    virtual INT OnEnd();                                             // 64 0x10094760

    // names ours: NCPetitionWnd's bodies
    void AddMessage(TCHAR* Text, DWORD Color, INT Type, INT Param);  // 0x10094480 (folded)
    void Clear();                                                    // 0x100944c0 (folded)
};
NW_ASSERT_OFFSET(NCGMPetitionWnd, EditBox, 0x98);
NW_ASSERT_OFFSET(NCGMPetitionWnd, EndButton, 0xA4);
NW_ASSERT_SIZE(NCGMPetitionWnd, 0xA8);

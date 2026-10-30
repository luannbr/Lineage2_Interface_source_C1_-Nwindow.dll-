#pragma once
// NCChatFilterWnd - the chat filter panel that opens above the chat window (UClass
// "CChatFilterWnd", 0xC8): one check box per NChatFilter switch of the current tab, OK and
// Cancel. OK copies the boxes into the tab's list and saves them to chatfilter.ini, one
// section per tab (entire_tab, market_tab, party_tab, pledge_tab, ally_tab); LoadFilters reads
// the five sections back when the chat is created. The tab's own channel stays checked and
// grey (SetFilter).

#include "nwindow/NCWnd.h"
#include "nwindow/NCChatListBox.h"

class NCCheckBox;
class NCButton;

// Filters[] order (names ours) and the NWM_COMMAND ids of the panel's controls
enum
{
    CHATFILTER_System,
    CHATFILTER_Damage,
    CHATFILTER_Chat,
    CHATFILTER_Normal,
    CHATFILTER_Shout,
    CHATFILTER_Pledge,
    CHATFILTER_Party,
    CHATFILTER_Market,
    CHATFILTER_Tell,
    CHATFILTER_Ally,
    CHATFILTER_MAX
};
enum
{
    NID_CHATFILTER_SYSTEM = (INT)0xF0F01450,   // Filters[i]: 0xF0F01450 + i, the last 0xF0F01470
    NID_CHATFILTER_CHAT = (INT)0xF0F01452,
    NID_CHATFILTER_ALLY = (INT)0xF0F01470,
    NID_CHATFILTER_OK = (INT)0xF0FF1458,
    NID_CHATFILTER_CANCEL = (INT)0xF0FF1459,
};

class NCChatFilterWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCChatFilterWnd, NCWnd, 0, 0x1012be08)

    NCCheckBox* Filters[CHATFILTER_MAX];   // +0x94 CHATFILTER_*
    NCButton* OKButton;                    // +0xBC
    NCButton* CancelButton;                // +0xC0
    UTexture* BackTex;                     // +0xC4 "L2UI.ChatWnd.chatoption"

    NCChatFilterWnd();                                               // 0x100390b0
    // ~NCChatFilterWnd                                              // 0x10039100

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10038450
    virtual INT OnCreate();                                          // 35 0x10039150
    virtual INT OnDestroy();                                         // 36 0x100398f0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100399a0
    // names ours
    virtual INT OnSystemClicked();                                   // 62 0x10039d10
    virtual INT OnChatClicked();                                     // 63 0x10039d50
    virtual INT OnOK();                                              // 64 0x10039e80
    virtual INT OnCancel();                                          // 65 0x10038520

    void SetFilter(NChatFilter Filter);                              // 0x10039a80 (name ours)
    void LoadFilters();                                              // 0x1003a140 (name ours)

private:
    void GreyTabChannel();          // inlined twice in the stock code (name ours)
};
NW_ASSERT_OFFSET(NCChatFilterWnd, Filters, 0x94);
NW_ASSERT_OFFSET(NCChatFilterWnd, OKButton, 0xBC);
NW_ASSERT_OFFSET(NCChatFilterWnd, BackTex, 0xC4);
NW_ASSERT_SIZE(NCChatFilterWnd, 0xC8);

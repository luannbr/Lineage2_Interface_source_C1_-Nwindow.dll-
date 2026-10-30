#pragma once
// NCChatListBox - the chat message list (UClass "CChatListBox", 0x210). Long messages are
// wrapped to the width (continuation lines indented 12 px); the scroll controls sit on the
// left and the visible line count follows the height (12 px lines). Each chat tab has one;
// its Filter says which message types it shows (IsChannelShown, NCChatWnd::AddChatMessage).

#include "nwindow/NCListBox.h"

// The channel switches of one chat tab (names ours, after their chatfilter.ini keys; the
// message types are those of L2's Say2 packet). Passed by value to
// NCChatFilterWnd::SetFilter, hence a struct.
struct NChatFilter
{
    UBOOL bSystem;             // +0x00 "system" type 5 (system / GM messages)
    UBOOL bChat;               // +0x04 "chat"   master switch of the chat types below
    UBOOL bDamage;             // +0x08 "damage" type 5 subtype 3, needs bSystem too
    UBOOL bNormal;             // +0x0C "normal" type 0
    UBOOL bShout;              // +0x10 "shout"  type 1 (!)
    UBOOL bPledge;             // +0x14 "pledge" type 4 (@)
    UBOOL bParty;              // +0x18 "party"  type 3 (#)
    UBOOL bMarket;             // +0x1C "market" type 8 (+)
    UBOOL bTell;               // +0x20 "tell"   type 2 (")
    UBOOL bAlly;               // +0x24 "ally"   type 9 ($)
};
NW_ASSERT_SIZE(NChatFilter, 0x28);

class NCChatListBox : public NCListBox
{
    NW_DECLARE_CLASS(NCChatListBox, NCListBox, 0, 0x1012c328)

    NChatFilter Filter;        // +0x1E8

    NCChatListBox();                                                 // 0x1003cb50
    NCChatListBox(INT InMaxLines, INT InVisibleLines, INT Unused);   // 0x1003cbf0
    // ~NCChatListBox                                                // 0x1003cba0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100576d0 (NCDebugWnd's)
    virtual INT OnCreate();                                          // 35 0x10038580
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100385f0
    virtual void AddString(const TCHAR* Text, DWORD Color, INT Param); // 64 0x1003aa80
    virtual UBOOL IsChannelShown(INT Type, INT SubType);             // 70 0x10037630 (name ours)
};
NW_ASSERT_OFFSET(NCChatListBox, Filter, 0x1E8);
NW_ASSERT_SIZE(NCChatListBox, 0x210);

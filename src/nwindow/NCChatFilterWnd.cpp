/*=============================================================================
    NCChatFilterWnd.cpp - chat filter panel. Stock: 0x10038450 .. 0x1003853e,
    0x100390b0 .. 0x10039d03 (0x10039080 is NCWnd::OffsetWindow), 0x10039d10 .. 0x1003aa60.
=============================================================================*/
#include "nwindow/NCChatFilterWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCGDevice.h"

static NCChatWnd* ChatWndOf(NCWnd* Wnd)
{
    return (NCChatWnd*)Wnd->Entry->Parent->Wnd;
}

// 0x100390b0
NCChatFilterWnd::NCChatFilterWnd()
{
}
NW_HOOK_CTOR(0x100390b0, NCChatFilterWnd)

// 0x10039100
NW_HOOK_DTOR(0x10039100, NCChatFilterWnd)

// 0x10038450 (vslot 24)
INT NCChatFilterWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_CHATFILTER_OK)
            return OnOK() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_CANCEL)
            return OnCancel() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_SYSTEM)
            return OnSystemClicked() != 0;
        if (wParam == (WPARAM)NID_CHATFILTER_CHAT)
            return OnChatClicked() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10038450, NCChatFilterWnd, WindowProc)

// one check box of the panel (repeated inline in the stock OnCreate)
static NCCheckBox* CreateFilterBox(NCWnd* Parent, INT TextID, INT ID, INT X, INT Y, INT W)
{
    NCCheckBox* Box = new NCCheckBox(TextID, 0, 0);
    Box->ID = ID;
    Box->SetWindowRect(X, Y, W, 12);
    Parent->CreateChild(Box, NWS_VISIBLE);
    return Box;
}

// 0x10039150 (vslot 35) - NCWnd::OnCreate is not called
INT NCChatFilterWnd::OnCreate()
{
    BackTex = FindTexture(TEXT("L2UI.ChatWnd.chatoption"));

    Filters[CHATFILTER_System] = CreateFilterBox(this, 0x164, (INT)0xF0F01450, 13, 13, 100);
    Filters[CHATFILTER_Damage] = CreateFilterBox(this, 0x165, (INT)0xF0F01451, 27, 31, 80);
    Filters[CHATFILTER_Chat]   = CreateFilterBox(this, 0x166, (INT)0xF0F01452, 13, 54, 100);
    Filters[CHATFILTER_Normal] = CreateFilterBox(this, 0x17F, (INT)0xF0F01453, 27, 72, 80);
    Filters[CHATFILTER_Shout]  = CreateFilterBox(this, 0x167, (INT)0xF0F01454, 27, 88, 80);
    Filters[CHATFILTER_Pledge] = CreateFilterBox(this, 0x168, (INT)0xF0F01455, 27, 105, 80);
    Filters[CHATFILTER_Party]  = CreateFilterBox(this, 0x169, (INT)0xF0F01456, 132, 72, 80);
    Filters[CHATFILTER_Market] = CreateFilterBox(this, 0x16A, (INT)0xF0F01457, 132, 88, 80);
    Filters[CHATFILTER_Tell]   = CreateFilterBox(this, 0x16B, (INT)0xF0F01458, 132, 105, 80);
    Filters[CHATFILTER_Ally]   = CreateFilterBox(this, 0x250, (INT)0xF0F01470, 27, 121, 80);

    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_CHATFILTER_OK;
    OKButton->SetWindowRect(87, 139, 67, 19);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(0x8C);

    CancelButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    CancelButton->ID = NID_CHATFILTER_CANCEL;
    CancelButton->SetWindowRect(163, 139, 67, 19);
    CreateChild(CancelButton, NWS_VISIBLE);
    CancelButton->SetTextID(0x8D);
    return 1;
}
NW_HOOK_METHOD(0x10039150, NCChatFilterWnd, OnCreate)

// 0x100398f0 (vslot 36)
INT NCChatFilterWnd::OnDestroy()
{
    for (INT i = 0; i < CHATFILTER_MAX; i++)
    {
        if (Filters[i])
        {
            RemoveChildWindow(Filters[i]->Entry);
            delete Filters[i];
            Filters[i] = NULL;
        }
    }
    if (OKButton)
    {
        RemoveChildWindow(OKButton->Entry);
        delete OKButton;
        OKButton = NULL;
    }
    if (CancelButton)
    {
        RemoveChildWindow(CancelButton->Entry);
        delete CancelButton;
        CancelButton = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100398f0, NCChatFilterWnd, OnDestroy)

// 0x100399a0 (vslot 50)
INT NCChatFilterWnd::OnPaint(NCGDevice* Device)
{
    guard(NCChatFilterWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->DrawTexture(0, 0, 240, 161, 0, 0, 240, 161, BackTex);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100399a0, NCChatFilterWnd, OnPaint)

// the current tab's own channel cannot be switched off
void NCChatFilterWnd::GreyTabChannel()
{
    NCChatWnd* Chat = ChatWndOf(this);
    if (Chat->CurTab == CHATTAB_Pledge)
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Party)
        Filters[CHATFILTER_Party]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Market)
        Filters[CHATFILTER_Market]->bDisabled = 1;
    if (Chat->CurTab == CHATTAB_Ally)
        Filters[CHATFILTER_Ally]->bDisabled = 1;
}

// 0x10039d10 (vslot 62) - damage messages are system messages
INT NCChatFilterWnd::OnSystemClicked()
{
    if (!Filters[CHATFILTER_System]->bChecked)
        Filters[CHATFILTER_Damage]->bDisabled = 1;
    else
        Filters[CHATFILTER_Damage]->bDisabled = 0;
    return 1;
}
NW_HOOK_METHOD(0x10039d10, NCChatFilterWnd, OnSystemClicked)

// 0x10039d50 (vslot 63) - "chat" switches the chat channels
INT NCChatFilterWnd::OnChatClicked()
{
    if (!Filters[CHATFILTER_Chat]->bChecked)
    {
        Filters[CHATFILTER_Normal]->bDisabled = 1;
        Filters[CHATFILTER_Shout]->bDisabled = 1;
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
        Filters[CHATFILTER_Party]->bDisabled = 1;
        Filters[CHATFILTER_Market]->bDisabled = 1;
        Filters[CHATFILTER_Tell]->bDisabled = 1;
        Filters[CHATFILTER_Ally]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Normal]->bDisabled = 0;
        Filters[CHATFILTER_Shout]->bDisabled = 0;
        Filters[CHATFILTER_Pledge]->bDisabled = 0;
        Filters[CHATFILTER_Party]->bDisabled = 0;
        Filters[CHATFILTER_Market]->bDisabled = 0;
        Filters[CHATFILTER_Tell]->bDisabled = 0;
        Filters[CHATFILTER_Ally]->bDisabled = 0;
    }
    GreyTabChannel();
    return 1;
}
NW_HOOK_METHOD(0x10039d50, NCChatFilterWnd, OnChatClicked)

// 0x10039e80 (vslot 64) - the boxes become the tab's filter and its chatfilter.ini section.
// The stock switch has no default (the list and section stay uninitialized); CurTab is
// always 0..4.
INT NCChatFilterWnd::OnOK()
{
    NCChatWnd* Chat = ChatWndOf(this);
    NCChatListBox* List = NULL;
    const TCHAR* Section = NULL;
    switch (Chat->CurTab)
    {
    case CHATTAB_All:
        List = Chat->Lists[CHATLIST_All];
        Section = TEXT("entire_tab");
        break;
    case CHATTAB_Market:
        List = Chat->Lists[CHATLIST_Market];
        Section = TEXT("market_tab");
        break;
    case CHATTAB_Party:
        List = Chat->Lists[CHATLIST_Party];
        Section = TEXT("party_tab");
        break;
    case CHATTAB_Pledge:
        List = Chat->Lists[CHATLIST_Pledge];
        Section = TEXT("pledge_tab");
        break;
    case CHATTAB_Ally:
        List = Chat->Lists[CHATLIST_Ally];
        Section = TEXT("ally_tab");
        break;
    }
    NChatFilter& F = List->Filter;
    F.bSystem = Filters[CHATFILTER_System]->bChecked;
    F.bDamage = Filters[CHATFILTER_Damage]->bChecked;
    F.bChat = Filters[CHATFILTER_Chat]->bChecked;
    F.bNormal = Filters[CHATFILTER_Normal]->bChecked;
    F.bShout = Filters[CHATFILTER_Shout]->bChecked;
    F.bPledge = Filters[CHATFILTER_Pledge]->bChecked;
    F.bParty = Filters[CHATFILTER_Party]->bChecked;
    F.bMarket = Filters[CHATFILTER_Market]->bChecked;
    F.bTell = Filters[CHATFILTER_Tell]->bChecked;
    F.bAlly = Filters[CHATFILTER_Ally]->bChecked;

    const TCHAR* Ini = TEXT("chatfilter.ini");
    GConfig->SetBool(Section, TEXT("system"), Filters[CHATFILTER_System]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("damage"), Filters[CHATFILTER_Damage]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("chat"), Filters[CHATFILTER_Chat]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("normal"), Filters[CHATFILTER_Normal]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("shout"), Filters[CHATFILTER_Shout]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("pledge"), Filters[CHATFILTER_Pledge]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("party"), Filters[CHATFILTER_Party]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("market"), Filters[CHATFILTER_Market]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("tell"), Filters[CHATFILTER_Tell]->bChecked, Ini);
    GConfig->SetBool(Section, TEXT("ally"), Filters[CHATFILTER_Ally]->bChecked, Ini);
    HideWindow();
    Chat->bFilterHidden = 1;
    return 1;
}
NW_HOOK_METHOD(0x10039e80, NCChatFilterWnd, OnOK)

// 0x10038520 (vslot 65)
INT NCChatFilterWnd::OnCancel()
{
    NCChatWnd* Chat = ChatWndOf(this);
    HideWindow();
    Chat->bFilterHidden = 1;
    return 1;
}
NW_HOOK_METHOD(0x10038520, NCChatFilterWnd, OnCancel)

// 0x10039a80 - the boxes show a tab's filter; disabled boxes keep their state
void NCChatFilterWnd::SetFilter(NChatFilter Filter)
{
    Filters[CHATFILTER_Normal]->bDisabled = 0;
    Filters[CHATFILTER_Shout]->bDisabled = 0;
    Filters[CHATFILTER_Pledge]->bDisabled = 0;
    Filters[CHATFILTER_Party]->bDisabled = 0;
    Filters[CHATFILTER_Market]->bDisabled = 0;
    Filters[CHATFILTER_Tell]->bDisabled = 0;
    Filters[CHATFILTER_Ally]->bDisabled = 0;
    if (!Filters[CHATFILTER_System]->bDisabled)
        Filters[CHATFILTER_System]->bChecked = Filter.bSystem;
    if (!Filters[CHATFILTER_Damage]->bDisabled)
        Filters[CHATFILTER_Damage]->bChecked = Filter.bDamage;
    if (!Filters[CHATFILTER_Chat]->bDisabled)
        Filters[CHATFILTER_Chat]->bChecked = Filter.bChat;
    if (!Filters[CHATFILTER_Normal]->bDisabled)
        Filters[CHATFILTER_Normal]->bChecked = Filter.bNormal;
    if (!Filters[CHATFILTER_Shout]->bDisabled)
        Filters[CHATFILTER_Shout]->bChecked = Filter.bShout;
    if (!Filters[CHATFILTER_Pledge]->bDisabled)
        Filters[CHATFILTER_Pledge]->bChecked = Filter.bPledge;
    if (!Filters[CHATFILTER_Party]->bDisabled)
        Filters[CHATFILTER_Party]->bChecked = Filter.bParty;
    if (!Filters[CHATFILTER_Market]->bDisabled)
        Filters[CHATFILTER_Market]->bChecked = Filter.bMarket;
    if (!Filters[CHATFILTER_Tell]->bDisabled)
        Filters[CHATFILTER_Tell]->bChecked = Filter.bTell;
    if (!Filters[CHATFILTER_Ally]->bDisabled)
        Filters[CHATFILTER_Ally]->bChecked = Filter.bAlly;

    if (!Filters[CHATFILTER_System]->bChecked)
        Filters[CHATFILTER_Damage]->bDisabled = 1;
    else
        Filters[CHATFILTER_Damage]->bDisabled = 0;
    if (!Filters[CHATFILTER_Chat]->bChecked)
    {
        Filters[CHATFILTER_Normal]->bDisabled = 1;
        Filters[CHATFILTER_Shout]->bDisabled = 1;
        Filters[CHATFILTER_Pledge]->bDisabled = 1;
        Filters[CHATFILTER_Party]->bDisabled = 1;
        Filters[CHATFILTER_Market]->bDisabled = 1;
        Filters[CHATFILTER_Tell]->bDisabled = 1;
        Filters[CHATFILTER_Ally]->bDisabled = 1;
    }
    else
    {
        Filters[CHATFILTER_Normal]->bDisabled = 0;
        Filters[CHATFILTER_Shout]->bDisabled = 0;
        Filters[CHATFILTER_Pledge]->bDisabled = 0;
        Filters[CHATFILTER_Party]->bDisabled = 0;
        Filters[CHATFILTER_Market]->bDisabled = 0;
        Filters[CHATFILTER_Tell]->bDisabled = 0;
        Filters[CHATFILTER_Ally]->bDisabled = 0;
    }
    GreyTabChannel();
}
NW_HOOK_METHOD(0x10039a80, NCChatFilterWnd, SetFilter)

// one [xxx_tab] section of chatfilter.ini (inlined five times in the stock code); a missing
// key takes the tab's default: system, damage, chat, shout and tell on, and the given ones
static FORCEINLINE void LoadTabFilter(const TCHAR* Section, NChatFilter& F, UBOOL bNormal,
                                      UBOOL bPledge, UBOOL bParty, UBOOL bMarket, UBOOL bAlly)
{
    const TCHAR* Ini = TEXT("chatfilter.ini");
    if (!GConfig->GetBool(Section, TEXT("system"), F.bSystem, Ini))
        F.bSystem = 1;
    if (!GConfig->GetBool(Section, TEXT("chat"), F.bChat, Ini))
        F.bChat = 1;
    if (!GConfig->GetBool(Section, TEXT("normal"), F.bNormal, Ini))
        F.bNormal = bNormal;
    if (!GConfig->GetBool(Section, TEXT("shout"), F.bShout, Ini))
        F.bShout = 1;
    if (!GConfig->GetBool(Section, TEXT("pledge"), F.bPledge, Ini))
        F.bPledge = bPledge;
    if (!GConfig->GetBool(Section, TEXT("party"), F.bParty, Ini))
        F.bParty = bParty;
    if (!GConfig->GetBool(Section, TEXT("market"), F.bMarket, Ini))
        F.bMarket = bMarket;
    if (!GConfig->GetBool(Section, TEXT("tell"), F.bTell, Ini))
        F.bTell = 1;
    if (!GConfig->GetBool(Section, TEXT("damage"), F.bDamage, Ini))
        F.bDamage = 1;
    if (!GConfig->GetBool(Section, TEXT("ally"), F.bAlly, Ini))
        F.bAlly = bAlly;
}

// 0x1003a140 - every tab's filter from chatfilter.ini; the boxes show the current tab's
void NCChatFilterWnd::LoadFilters()
{
    NCChatWnd* Chat = ChatWndOf(this);

    NChatFilter& All = Chat->Lists[CHATLIST_All]->Filter;
    LoadTabFilter(TEXT("entire_tab"), All, 1, 1, 1, 0, 0);
    if (Chat->CurTab == CHATTAB_All)
        SetFilter(All);

    NChatFilter& Pledge = Chat->Lists[CHATLIST_Pledge]->Filter;
    LoadTabFilter(TEXT("pledge_tab"), Pledge, 0, 1, 0, 0, 0);
    if (Chat->CurTab == CHATTAB_Pledge)
        SetFilter(Pledge);

    NChatFilter& Party = Chat->Lists[CHATLIST_Party]->Filter;
    LoadTabFilter(TEXT("party_tab"), Party, 0, 0, 1, 0, 0);
    if (Chat->CurTab == CHATTAB_Party)
        SetFilter(Party);

    NChatFilter& Market = Chat->Lists[CHATLIST_Market]->Filter;
    LoadTabFilter(TEXT("market_tab"), Market, 0, 0, 0, 1, 0);
    if (Chat->CurTab == CHATTAB_Market)
        SetFilter(Market);

    NChatFilter& Ally = Chat->Lists[CHATLIST_Ally]->Filter;
    LoadTabFilter(TEXT("ally_tab"), Ally, 0, 0, 0, 0, 1);
    if (Chat->CurTab == CHATTAB_Ally)
        SetFilter(Ally);
}
NW_HOOK_METHOD(0x1003a140, NCChatFilterWnd, LoadFilters)

NW_CHECK_VTABLE(NCChatFilterWnd, 0x100e0258, 66)

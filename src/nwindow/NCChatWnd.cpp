/*=============================================================================
    NCChatWnd.cpp - the chat window. Stock: 0x100376f0 .. 0x10038395, 0x100383a0 .. 0x1003844a,
    0x100387f0 .. 0x10038e0f, 0x10038e20 .. 0x10039064, 0x1003aed0 .. 0x1003b693,
    0x1003b6a0 .. 0x1003ca63, 0x1003cc20 .. 0x1003d73f; 0x1003dab0 (HideWindow, shared).
=============================================================================*/
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCChatFilterWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NCChineseIME.h"
#include "nwindow/NCCompositeBox.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NString.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NCrt.h"

#if NW_FEATURES
// phase 6: CombatLog (NWindow_C1 src/inject/combatlog/CombatLog.cpp)
#include "features/CombatLog.h"
#endif

// Say2 types (the chat channels) as the tabs use them (names ours)
enum
{
    SAY2_Normal = 0,
    SAY2_Shout = 1,
    SAY2_Tell = 2,
    SAY2_Party = 3,
    SAY2_Pledge = 4,
    SAY2_System = 5,
    SAY2_Market = 8,
    SAY2_Ally = 9,
};

// 0x1003cc20 - TellNames and Parser are built first; Lists[3], Lists[4] and the other
// children are left as allocated
NCChatWnd::NCChatWnd()
{
    HeadTex = NULL;
    BodyTex = NULL;
    Unknown9C = NULL;
    EditBox = NULL;
    TellIndex = -1;
    bTellBrowse = 0;
    Lists[CHATLIST_All] = NULL;
    Lists[CHATLIST_Party] = NULL;
    Lists[CHATLIST_Pledge] = NULL;
    OptionButton = NULL;
    CreateHeight = 0;
    Unknown14C = 0;
    Unknown158 = 0;
    LastTellName[0] = 0;
    CurTab = CHATTAB_All;
    bFilterHidden = 0;
    CaptureFile = NULL;
    Unknown150 = 2;
}
NW_HOOK_CTOR(0x1003cc20, NCChatWnd)

// 0x1003d6d0
NW_HOOK_DTOR(0x1003d6d0, NCChatWnd)

// 0x100376f0 (vslot 24)
INT NCChatWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == (WPARAM)NID_CHAT_TABS)
            return OnTabSelected(lParam) != 0;
        if (wParam == (WPARAM)NID_CHAT_EDIT)
            return OnEditCommand(lParam) != 0;
        if (wParam == (WPARAM)NID_CHAT_OPTION)
            return OnOptionButton() != 0;
        if (wParam == (WPARAM)NID_CHAT_RESIZE)
            return OnResize(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100376f0, NCChatWnd, WindowProc)

// 0x10038380 (vslot 28) - the focus goes to the input line
void NCChatWnd::SetFocus()
{
    EditBox->SetFocus();
    KillFocus();
}
NW_HOOK_METHOD(0x10038380, NCChatWnd, SetFocus)

// 0x1003cd00 (vslot 35)
INT NCChatWnd::OnCreate()
{
    NCWnd::OnCreate();
    HeadTex = FindTexture(TEXT("L2UI.ChatWnd.Head"));
    BodyTex = FindTexture(TEXT("L2UI.ChatWnd.Body"));
    BottomTex = FindTexture(TEXT("L2UI.ChatWnd.Bottom"));
    Bottom1Tex = FindTexture(TEXT("L2UI.ChatWnd.Bottom1"));
    Bottom2Tex = FindTexture(TEXT("L2UI.ChatWnd.Bottom2"));
    IMEJpTex = FindTexture(TEXT("L2UI.ChatWnd.IME_jp"));
    IMEEnTex = FindTexture(TEXT("L2UI.ChatWnd.IME_en"));
    IMEKrTex = FindTexture(TEXT("L2UI.ChatWnd.IME_kr"));
    IMETw1Tex = FindTexture(TEXT("L2UI.ChatWnd.IME_tw1"));
    IMETw2Tex = FindTexture(TEXT("L2UI.ChatWnd.IME_tw2"));
    IMETw3Tex = FindTexture(TEXT("L2UI.ChatWnd.IME_tw3"));

    ResizeFrame = new NCResizeFrame(RESIZE_Vertical);
    ResizeFrame->ID = NID_CHAT_RESIZE;
    ResizeFrame->SetWindowRect(0, 0, 24, 10);
    CreateChild(ResizeFrame, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown

    EditBox = new NCEditBox(1, NULL, 1, 0x108);
    EditBox->ID = NID_CHAT_EDIT;
    EditBox->SetWindowRect(28, Height - 22, 312, 16);
    CreateChild(EditBox, NWS_VISIBLE);

    // the lists fill the window above the tab row; only the first one starts visible
    for (INT i = 0; i < CHATLIST_MAX; i++)
    {
        Lists[i] = new NCChatListBox(200, 10, 12);
        Lists[i]->ID = NID_CHAT_LIST;
        Lists[i]->SetWindowRect(6, 7, Width - 10, 128);
        CreateChild(Lists[i], NWS_NOHITTEST);
        Lists[i]->bAutoScroll = 1;
        Lists[i]->LineGap = 0;
        if (i != CHATLIST_All)
            Lists[i]->HideWindow();
    }

    TabCtrl = new NCTabControl();
    TabCtrl->ID = NID_CHAT_TABS;
    TabCtrl->SetWindowRect(28, Height - 43, 251, 17);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x90, Lists[CHATLIST_All], 63, 17, TEXT("L2UI.ChatWnd.ChatTab2"),
                    TEXT("L2UI.ChatWnd.ChatTab1"), 0, -1);
    TabCtrl->AddTab(0x163, Lists[CHATLIST_Market], 63, 17, TEXT("L2UI.ChatWnd.ChatTab2"),
                    TEXT("L2UI.ChatWnd.ChatTab1"), 0, -1);
    TabCtrl->AddTab(0xBC, Lists[CHATLIST_Party], 63, 17, TEXT("L2UI.ChatWnd.ChatTab2"),
                    TEXT("L2UI.ChatWnd.ChatTab1"), 0, -1);
    TabCtrl->AddTab(0x80, Lists[CHATLIST_Pledge], 63, 17, TEXT("L2UI.ChatWnd.ChatTab2"),
                    TEXT("L2UI.ChatWnd.ChatTab1"), 0, -1);
    TabCtrl->AddTab(0x22F, Lists[CHATLIST_Ally], 63, 17, TEXT("L2UI.ChatWnd.ChatTab2"),
                    TEXT("L2UI.ChatWnd.ChatTab1"), 0, -1);

    OptionButton = new NCButton(TEXT("L2UI.ChatWnd.OptionBtn"), TEXT("L2UI.ChatWnd.OptionBtn_click"), 0);
    OptionButton->ID = NID_CHAT_OPTION;
    OptionButton->SetWindowRect(3, Height - 40, 16, 16);
    CreateChild(OptionButton, NWS_VISIBLE);
    CreateHeight = Height;

    FilterWnd = new NCChatFilterWnd();
    FilterWnd->SetWindowRect(0, -161, 240, 161);
    CreateChild(FilterWnd, NWS_VISIBLE);
    FilterWnd->LoadFilters();
    return 1;
}
NW_HOOK_METHOD(0x1003cd00, NCChatWnd, OnCreate)

// 0x100387f0 (vslot 36) - the names in TellNames are freed, the array itself goes with the
// destructor
INT NCChatWnd::OnDestroy()
{
    if (EditBox)
    {
        RemoveChildWindow(EditBox->Entry);
        delete EditBox;
        EditBox = NULL;
    }
    for (INT i = 0; i < CHATLIST_MAX; i++)
    {
        if (Lists[i])
        {
            RemoveChildWindow(Lists[i]->Entry);
            delete Lists[i];
            Lists[i] = NULL;
        }
    }
    if (TabCtrl)
    {
        RemoveChildWindow(TabCtrl->Entry);
        delete TabCtrl;
        TabCtrl = NULL;
    }
    if (OptionButton)
    {
        RemoveChildWindow(OptionButton->Entry);
        delete OptionButton;
        OptionButton = NULL;
    }
    if (Unknown138)
    {
        RemoveChildWindow(Unknown138->Entry);
        delete Unknown138;
        Unknown138 = NULL;
    }
    if (FilterWnd)
    {
        RemoveChildWindow(FilterWnd->Entry);
        delete FilterWnd;
        FilterWnd = NULL;
    }
    if (ResizeFrame)
    {
        RemoveChildWindow(ResizeFrame->Entry);
        delete ResizeFrame;
        ResizeFrame = NULL;
    }
    for (INT i = 0; i < TellNames.Num(); i++)
        delete TellNames(i);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100387f0, NCChatWnd, OnDestroy)

// 0x100383a0 (vslot 42) - NCWnd shows every child; the tab row shows the current list
void NCChatWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    Lists[CHATLIST_Pledge]->HideWindow();
    Lists[CHATLIST_Party]->HideWindow();
    TabCtrl->ShowCurrentPage();
    FilterWnd->HideWindow();
    bFilterHidden = 1;
}
NW_HOOK_METHOD(0x100383a0, NCChatWnd, ShowWindow)

// 0x1003dab0 (vslot 43) - the same body serves NCAbnormalStatusWnd, NCPetitionWnd and others
void NCChatWnd::HideWindow()
{
    NCWnd::HideWindow();
}
NW_HOOK_METHOD(0x1003dab0, NCChatWnd, HideWindow)

// vslot 44 - stock body shared with NCWnd::OffsetWindow (0x10039080): the chat window does
// not snap
void NCChatWnd::MoveWindow(INT DX, INT DY)
{
    LocalX += DX;
    LocalY += DY;
    OffsetScreen(DX, DY);
}

// 0x10038bf0 (vslot 50) - frame, the IME state icon left of the input line and, for the
// Chinese client, the place of the IME's window
INT NCChatWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (HeadTex)
        Device->DrawTexture(0, 1, 345, 10, 0, 0, 345, 10, HeadTex);
    if (Bottom1Tex)
        Device->DrawTexture(0, Height - 43, 345, 44, 0, 0, 345, 44, Bottom1Tex);
    for (INT i = 0; i < (INT)((DWORD)(Height - 44) / 12); i++)
    {
        if (BodyTex)
            Device->DrawTexture(0, 11 + i * 12, 345, 12, 0, 0, 345, 12, BodyTex);
    }

    INT IME = GConsole->GetIMEType();
    if (IME == 0)
    {
        if (IMEKrTex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMEKrTex);
    }
    else if (IME == 1)
    {
        if (IMEEnTex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMEEnTex);
    }
    else if (IME == 2)
    {
        if (IMEJpTex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMEJpTex);
    }
    else if (IME == 6)
    {
        if (IMETw1Tex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMETw1Tex);
    }
    else if (IME == 4)
    {
        if (IMETw2Tex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMETw2Tex);
    }
    else if (IME == 5)
    {
        if (IMETw3Tex)
            Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16, IMETw3Tex);
    }
    else if (IME == -1)
    {
        Device->DrawTexture(3, Height - 21, 16, 16, 0, 0, 16, 16,
                            FindTexture(TEXT("NWindow.BlackTexture")));
    }

    if (EditBox && GLanguageType == 4 && EditBox->Unknown1DC && GConsole->GetIMEType() != 8)
    {
        NCIMEManager* Manager = EditBox->IMEManager;
        if (Manager && Manager->ChineseIME)
        {
            // the Chinese IME's composition window goes above the chat
            NCWnd* IMEWnd = (NCWnd*)static_cast<NCChineseIME*>(Manager->ChineseIME)->CompositeBox;
            if (IMEWnd)
                IMEWnd->SetWindowRect(0, 6 - LocalH, 100, 15);
        }
    }
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10038bf0, NCChatWnd, OnPaint)

/*-----------------------------------------------------------------------------
    Text capture: GL2TextCapture is set by the options (1 start, 3 stop), vslot 66 acts on
    it; while it is 2 every line the current tab shows goes to L2_mm_dd_hh_mm.log (UTF-16).
-----------------------------------------------------------------------------*/

// 0x100377f0 (vslot 62)
void NCChatWnd::StartTextCapture()
{
    if (GL2TextCapture != 1)
        return;
    INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
    TCHAR Line[1024];
    TCHAR FileName[1024];
    appSystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);
    appSprintf(FileName, TEXT("L2_%02d_%02d_%02d_%02d.log"), Month, Day, Hour, Min);
    CaptureFile = GFileManager->CreateFileWriter(FileName, FILEWRITE_Unbuffered, GNull);
    if (CaptureFile)
    {
        INT ByteOrderMark = 0xFEFF;   // the file is UTF-16 LE: FF FE
        CaptureFile->Serialize(&ByteOrderMark, 2);
        appSprintf(Line, TEXT("[ L2 Text Capture] [ %02d/%02d %02d:%02d:%02d ]"), Month, Day, Hour,
                   Min, Sec);
        WriteCapture(Line);
    }
    appSprintf(Line, TEXT("[ Text Capture Start ][ FileName : %s ]"), FileName);
    NCChatListBox* List = TabList(CurTab);
    if (List)
        List->AddString(Line, (DWORD)-1, -1);
    GL2TextCapture = 2;
}
NW_HOOK_METHOD(0x100377f0, NCChatWnd, StartTextCapture)

// 0x10037970 (vslot 63)
void NCChatWnd::StopTextCapture()
{
    if (GL2TextCapture != 3)
        return;
    if (CaptureFile)
    {
        CaptureFile->Close();
        delete CaptureFile;
        CaptureFile = NULL;
    }
    NCChatListBox* List = TabList(CurTab);
    if (List)
        List->AddString(TEXT("=====[ Text Capture End ]====="), (DWORD)-1, -1);
    GL2TextCapture = 0;
}
NW_HOOK_METHOD(0x10037970, NCChatWnd, StopTextCapture)

// 0x10037a40 (vslot 64)
void NCChatWnd::WriteCapture(const TCHAR* Text)
{
    if (CaptureFile)
    {
        CaptureFile->Serialize((void*)Text, appStrlen(Text) * sizeof(TCHAR));
        CaptureFile->Serialize((void*)TEXT("\r\n"), appStrlen(TEXT("\r\n")) * sizeof(TCHAR));
    }
}
NW_HOOK_METHOD(0x10037a40, NCChatWnd, WriteCapture)

// 0x10037aa0 (vslot 65)
void NCChatWnd::CaptureMessage(const TCHAR* Text, INT Type, INT SubType)
{
    if (GL2TextCapture != 2)
        return;
    NCChatListBox* List = TabList(CurTab);
    if (List && List->IsChannelShown(Type, SubType))
        WriteCapture(Text);
}
NW_HOOK_METHOD(0x10037aa0, NCChatWnd, CaptureMessage)

// 0x100377c0 (vslot 66)
void NCChatWnd::UpdateTextCapture()
{
    if (GL2TextCapture == 1)
        StartTextCapture();
    else if (GL2TextCapture == 3)
        StopTextCapture();
}
NW_HOOK_METHOD(0x100377c0, NCChatWnd, UpdateTextCapture)

/*-----------------------------------------------------------------------------
    Tabs, filter panel, resizing.
-----------------------------------------------------------------------------*/

// the input line starts with a channel prefix, ASCII or full width
static UBOOL StartsWithPrefix(const TCHAR* First, INT Type)
{
    return !wcscmp(First, GChatPrefix[Type][0]) || !wcscmp(First, GChatPrefix[Type][1]);
}
static UBOOL StartsWithTabPrefix(const TCHAR* First)
{
    return StartsWithPrefix(First, SAY2_Party) || StartsWithPrefix(First, SAY2_Pledge) ||
           StartsWithPrefix(First, SAY2_Market) || StartsWithPrefix(First, SAY2_Ally);
}

// 0x1003aed0 (vslot 67) - the tab row picked a tab: its list is shown and the filter panel
// shows its filter. With GIsOldChat the input line follows the tab: the tab's prefix
// (# party, @ pledge, + market, $ ally) replaces the one of another tab.
INT NCChatWnd::OnTabSelected(INT Tab)
{
    static const INT TabType[] = { SAY2_Normal, SAY2_Market, SAY2_Party, SAY2_Pledge, SAY2_Ally };
    TCHAR First[2];
    First[0] = 0;
    if (Tab < CHATTAB_All || Tab > CHATTAB_Ally)
        return 1;
    CurTab = Tab;
    NCChatListBox* Shown = TabList(Tab);
    static const INT Order[] = { CHATLIST_All, CHATLIST_Pledge, CHATLIST_Party, CHATLIST_Market,
                                 CHATLIST_Ally };
    for (INT i = 0; i < CHATLIST_MAX; i++)
    {
        if (Lists[Order[i]] == Shown)
            Lists[Order[i]]->ShowWindow();
        else
            Lists[Order[i]]->HideWindow();
    }
    if (GIsOldChat)
    {
        if (EditBox->CommitText())
            wcsncpy(First, EditBox->CommitText(), 1);
        First[1] = 0;
        if (Tab == CHATTAB_All)
        {
            if (StartsWithTabPrefix(First))
                EditBox->DeleteChar(0);
        }
        else if (!StartsWithPrefix(First, TabType[Tab]))
        {
            if (StartsWithTabPrefix(First))
                EditBox->DeleteChar(0);
            EditBox->InsertText(GChatPrefix[TabType[Tab]][0], 0);
        }
    }
    FilterWnd->SetFilter(TabList(Tab)->Filter);
    return 1;
}
NW_HOOK_METHOD(0x1003aed0, NCChatWnd, OnTabSelected)

// 0x1003c9c0 (vslot 69) - the option button opens the filter panel on the current tab's
// filter (re-read from chatfilter.ini) or closes it
INT NCChatWnd::OnOptionButton()
{
    if (bFilterHidden)
    {
        FilterWnd->LoadFilters();
        FilterWnd->ShowWindow();
        bFilterHidden = 0;
    }
    else
    {
        FilterWnd->HideWindow();
        bFilterHidden = 1;
    }
    if (CurTab < CHATTAB_All || CurTab > CHATTAB_Ally)
        return 1;
    FilterWnd->SetFilter(TabList(CurTab)->Filter);
    return 1;
}
NW_HOOK_METHOD(0x1003c9c0, NCChatWnd, OnOptionButton)

// 0x10038e20 (vslot 70) - the grip on the top edge sends the mouse's vertical moves; the
// window grows upwards in 12 px steps (one line), between 174 and 500 px. The lists keep
// their last lines in view.
INT NCChatWnd::OnResize(INT Delta)
{
    ResizeDelta += Delta;
    if (ResizeDelta < 12 && ResizeDelta > -12)
        return 1;
    ResizeDelta = -(ResizeDelta / 12) * 12;
    DWORD NewHeight = (DWORD)(ResizeDelta + Height);
    if (NewHeight >= 174 && NewHeight <= 500)
    {
        static const INT Order[] = { CHATLIST_All, CHATLIST_Pledge, CHATLIST_Party,
                                     CHATLIST_Market, CHATLIST_Ally };
        NCChatListBox* List = NULL;
        for (INT i = 0; i < CHATLIST_MAX; i++)
        {
            if (Lists[Order[i]])
                List = Lists[Order[i]];
            // the visible line count is taken from the height before the resize
            INT H = List->LocalH;
            List->SetSize(Width - 10, ResizeDelta + H);
            List->ScrollBar->SetSize(12, H + ResizeDelta - 24);
            INT OldVisible = List->VisibleLines;
            List->VisibleLines = (DWORD)H / 12;
            if (ResizeDelta > 0 || List->Items.Num() > H / 12)
            {
                INT Top = List->TopIndex - List->VisibleLines + OldVisible;
                if (Top < 0)
                    Top = 0;
                List->TopIndex = Top;
            }
            List->ScrollBar->SetRange(List->NumLines, List->VisibleLines);
            List->ScrollBar->SetValue(List->TopIndex);
            List->ScrollDownButton->MoveWindow(0, ResizeDelta);
        }
        MoveWindow(0, -ResizeDelta);
        EditBox->MoveWindow(0, ResizeDelta);
        OptionButton->MoveWindow(0, ResizeDelta);
        TabCtrl->MoveWindow(0, ResizeDelta);
        SetSize(345, Height + ResizeDelta);
        ResizeFrame->SetWindowRect(0, 0, 24, 10);
        // the tutorial's question mark, docked above the chat
        GDesktop->TutorialQuestionWnd->SetWindowRect(5, GSEKScreenY - Height - 37, 0, 0);
    }
    ResizeDelta = 0;
    return 1;
}
NW_HOOK_METHOD(0x10038e20, NCChatWnd, OnResize)

/*-----------------------------------------------------------------------------
    Messages.
-----------------------------------------------------------------------------*/

// every list whose filter has both switches on gets the line (inlined per channel in the
// stock code; the list pointers are not tested there)
static void AddToLists(NCChatListBox* const* Lists, UBOOL NChatFilter::*Channel,
                       UBOOL NChatFilter::*Master, const TCHAR* Text, DWORD Color)
{
    for (INT i = 0; i < CHATLIST_MAX; i++)
        if (Lists[i]->Filter.*Channel && Lists[i]->Filter.*Master)
            Lists[i]->AddString(Text, Color, -1);
}

// 0x10037b30 - a chat or system line from the server (NConsoleWnd): each tab filters it by
// its type; a GM's answer to a petition (5/6) goes to the petition windows instead
void NCChatWnd::AddChatMessage(const TCHAR* Text, DWORD Color, INT Type, INT SubType)
{
#if NW_FEATURES
    // phase 6: CombatLog (NWindow_C1 src/inject/combatlog/CombatLog.cpp): the system lines go
    // to the combat log instead
    if (nwf::CombatLog_DivertChatMessage(Text, Color, Type, SubType))
        return;
#endif
    switch (Type)
    {
    case SAY2_Market:
        AddToLists(Lists, &NChatFilter::bMarket, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Pledge:
        AddToLists(Lists, &NChatFilter::bPledge, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Party:
        AddToLists(Lists, &NChatFilter::bParty, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Shout:
        AddToLists(Lists, &NChatFilter::bShout, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Tell:
        AddToLists(Lists, &NChatFilter::bTell, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Normal:
        AddToLists(Lists, &NChatFilter::bNormal, &NChatFilter::bChat, Text, Color);
        break;
    case SAY2_Ally:
        AddToLists(Lists, &NChatFilter::bAlly, &NChatFilter::bChat, Text, Color);
        break;
    case 6:
    case 7:
    case 10:
        for (INT i = 0; i < CHATLIST_MAX; i++)
            if (Lists[i])
                Lists[i]->AddString(Text, Color, -1);
        break;
    }
    if (Type == SAY2_System)
    {
        if (SubType == 3)
            AddToLists(Lists, &NChatFilter::bDamage, &NChatFilter::bSystem, Text, Color);
        else if (SubType == 6)
        {
            // the petition chat goes to the GM's window on the desktop; while that one is
            // hidden, also to the player's, which is shown
            GDesktop->GMPetitionWnd->AddMessage((TCHAR*)Text, 0xFF80FFFF, 5, 0);
            if (!GDesktop->GMPetitionWnd->IsShowWindow())
            {
                GConsole->PetitionWnd->ShowWindow();
                static_cast<NCPetitionWnd*>(GConsole->PetitionWnd)
                    ->AddMessage((TCHAR*)Text, 0xFF80FFFF, 5, 0);
            }
        }
        else
        {
            for (INT i = 0; i < CHATLIST_MAX; i++)
                if (Lists[i] && Lists[i]->Filter.bSystem)
                    Lists[i]->AddString(Text, Color, -1);
        }
    }
    if (GL2TextCapture == 2)
        CaptureMessage(Text, Type, SubType);
}
NW_HOOK_METHOD(0x10037b30, NCChatWnd, AddChatMessage)

/*-----------------------------------------------------------------------------
    Tell partners: '"' typed first puts the last one in the line, Up/Down walk the list.
-----------------------------------------------------------------------------*/

// 0x10038320 - "name " of a tell just sent. A 31-character name writes the terminator over
// TellNames (stock bug; names have 16 characters at most).
void NCChatWnd::SetLastTellName(const TCHAR* Name)
{
    if (Name && *Name && wcslen(Name) < 32)
    {
        appStrcpy(LastTellName, Name);
        INT Len = (INT)wcslen(LastTellName);
        TCHAR* P = LastTellName;
        P[Len] = ' ';
        P[Len + 1] = 0;
    }
}
NW_HOOK_METHOD(0x10038320, NCChatWnd, SetLastTellName)

// 0x10038a30 - NULL: the last partner
void NCChatWnd::BeginTell(const TCHAR* Name)
{
    TCHAR Buffer[32];
    if (Name)
    {
        appStrcpy(Buffer, Name);
        INT Len = (INT)wcslen(Buffer);
        Buffer[Len] = ' ';
        Buffer[Len + 1] = 0;
        EditBox->InsertText(Buffer, 1);
    }
    else if (TellNames.Num() > 0)
    {
        appStrcpy(Buffer, TellNames(TellNames.Num() - 1)->GetAt(0));
        INT Len = (INT)wcslen(Buffer);
        Buffer[Len] = ' ';
        Buffer[Len + 1] = 0;
        EditBox->InsertText(Buffer, 1);
    }
    bTellBrowse = 1;
}
NW_HOOK_METHOD(0x10038a30, NCChatWnd, BeginTell)

// 0x10038ad0 - the line becomes "name ; Down past the newest leaves only the "
void NCChatWnd::BrowseTellNames(UBOOL bUp)
{
    if (!bTellBrowse)
        return;
    if (bUp)
    {
        if (--TellIndex < 0)
            TellIndex = 0;
        if (TellNames.Num() <= 0)
            return;
    }
    else
    {
        INT Last = TellNames.Num() - 1;
        if (TellIndex == Last)
        {
            TellIndex++;
            EditBox->ClearText();
            EditBox->InsertText(GChatPrefix[SAY2_Tell][0], 1);
            return;
        }
        if (TellIndex > Last)
            return;
        if (++TellIndex > Last)
            TellIndex = Last;
        if (TellNames.Num() <= 0)
            return;
    }
    if (!TellNames(TellIndex)->GetAt(0))
        return;
    EditBox->ClearText();
    EditBox->InsertText(GChatPrefix[SAY2_Tell][0], 1);
    EditBox->InsertText(TellNames(TellIndex)->GetAt(0), 1);
    EditBox->InsertText(TEXT(" "), 1);
}
NW_HOOK_METHOD(0x10038ad0, NCChatWnd, BrowseTellNames)

// 0x1003ace0 - a known name moves to the newest place; past 64 names the oldest goes.
// Names starting with '-' are not kept.
void NCChatWnd::AddTellName(const TCHAR* Name)
{
    if (!Name || !*Name || wcslen(Name) >= 32 || Name[0] == '-')
        return;
    for (INT i = 0; i < TellNames.Num(); i++)
    {
        if (!_wcsicmp(TellNames(i)->GetAt(0), Name))
        {
            NString* Found = TellNames(i);
            INT j;
            for (j = i; j < TellNames.Num() - 1; j++)
                TellNames(j) = TellNames(j + 1);
            TellNames(j) = Found;
            TellIndex = TellNames.Num() - 1;
            return;
        }
    }
    if (TellNames.Num() > 63)
    {
        delete TellNames(0);
        INT j;
        for (j = 0; j < TellNames.Num() - 1; j++)
            TellNames(j) = TellNames(j + 1);
        TellNames(j) = new NString(Name);
    }
    else
    {
        NString* Str = new NString(Name);
        TellNames.AddItem(Str);
    }
    TellIndex = TellNames.Num() - 1;
}
NW_HOOK_METHOD(0x1003ace0, NCChatWnd, AddTellName)

// 0x1003d5a0
void NCChatWnd::EnablePledgeTab(bool bEnable)
{
    TArray<NCRadioButton*> Tabs = TabCtrl->GetTabs();
    Tabs(CHATTAB_Pledge)->EnableWindow(bEnable);
}
NW_HOOK_METHOD(0x1003d5a0, NCChatWnd, EnablePledgeTab)

// 0x1003d610
void NCChatWnd::EnableAllyTab(bool bEnable)
{
    TArray<NCRadioButton*> Tabs = TabCtrl->GetTabs();
    Tabs(CHATTAB_Ally)->EnableWindow(bEnable);
}
NW_HOOK_METHOD(0x1003d610, NCChatWnd, EnableAllyTab)

/*-----------------------------------------------------------------------------
    The input line: Enter runs it (chat or a /command).
-----------------------------------------------------------------------------*/

// C is a channel prefix, ASCII or full width
static FORCEINLINE UBOOL IsPrefixChar(TCHAR C, INT Type)
{
    return C == GChatPrefix[Type][0][0] || C == GChatPrefix[Type][1][0];
}

// "/useskill name": the skill of commandname's table, with Ctrl (force) / Shift (stand)
static FORCEINLINE void UseSkillCommand(NCommandParser& Parser, UBOOL bForce, UBOOL bStand)
{
    TCHAR* Name = Parser.GetNextToken();
    if (!Name)
        return;
    FL2MagicSkillCommandData* Skill = GL2GameData.GetMSCommandData(Name);
    if (!Skill)
        return;
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Skill->SkillID);
    Params.PushBack((void*)(INT_PTR)bForce);
    Params.PushBack((void*)(INT_PTR)bStand);
    GConsole->RequestMagicSkillUse(Params);
}

// "/useshortcut page slot" (1..10 each): shortcut (page - 1) * 10 + slot - 1
static FORCEINLINE void UseShortcutCommand(NCommandParser& Parser, UBOOL bForce, UBOOL bStand)
{
    TCHAR* Token = Parser.GetNextToken();
    if (!Token)
        return;
    INT Page = NWtol(Token);
    if (Page < 1 || Page > 10)
        return;
    Token = Parser.GetNextToken();
    if (!Token)
        return;
    INT Slot = NWtol(Token);
    if (Slot < 1 || Slot > 10)
        return;
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)(Slot + Page * 10 - 11));
    Params.PushBack((void*)(INT_PTR)bForce);
    Params.PushBack((void*)(INT_PTR)bStand);
    GConsole->UseShortcut(Params);
}

// a request whose only argument is the command's next word
#define NAME_REQUEST(Request)                                                                  \
    {                                                                                          \
        TCHAR* Name = Parser.GetNextToken();                                                   \
        if (Name)                                                                              \
        {                                                                                      \
            L2ParamStack Params(10);                                                           \
            Params.PushBack(Name);                                                             \
            GConsole->Request(Params);                                                         \
        }                                                                                      \
    }

// 0x1003b6a0 (vslot 68) - the input line reports 3 ('"' typed first), 4 / 5 (Up / Down) and
// 1 (Enter). Enter: the line is plain chat (the channel from the tab or the first character)
// or a command; then it is cleared, the tab's prefix put back with GIsOldChat, and with
// GIsEnterChatting (Enter opens and closes the chat) it loses the focus.
INT NCChatWnd::OnEditCommand(INT Code)
{
    if (Code == 3)
    {
        BeginTell(NULL);
        return 1;
    }
    if (Code == 4)
    {
        BrowseTellNames(1);
        return 1;
    }
    if (Code == 5)
    {
        BrowseTellNames(0);
        return 1;
    }
    if (Code != 1)
        return 1;

    bTellBrowse = 0;
    const TCHAR* Text = EditBox->CommitText();
    if (!Text)
        return 1;
    INT BuilderLevel = 0;
    User* Me = GConsole->GetMyUser();
    if (Me)
        BuilderLevel = Me->BuilderLevel;
    TCHAR Line[1024];
    appStrcpy(Line, Text);

    INT Command = Parser.GetCommand(Line);
    if (Command > NCMD_Builder)
    {
        if (Command == NCMD_Exec)
        {
            // builders of level 9 may only run "stat fps"; the check reads one character too
            // far, so it passes with "/// stat fps"
            if (BuilderLevel && (BuilderLevel != 9 || !_wcsicmp(&Line[4], TEXT("stat fps"))))
                GConsole->ExecCommand(&Line[3]);
        }
        else if (Command == NCMD_Chat)
        {
            // the channel: from the tab (unless GIsOldChat, where the tab put its prefix in the
            // line), then from the first character
            TCHAR* Message = &Line[1];
            INT Type = SAY2_Normal;
            if (!GIsOldChat)
            {
                switch (CurTab)
                {
                case CHATTAB_All:    Type = SAY2_Normal; break;
                case CHATTAB_Market: Type = SAY2_Market; break;
                case CHATTAB_Party:  Type = SAY2_Party;  break;
                case CHATTAB_Pledge: Type = SAY2_Pledge; break;
                case CHATTAB_Ally:   Type = SAY2_Ally;   break;
                }
            }
            TCHAR First = Line[0];
            if (IsPrefixChar(First, SAY2_Shout))
                Type = SAY2_Shout;
            else if (IsPrefixChar(First, SAY2_Pledge))
                Type = SAY2_Pledge;
            else if (IsPrefixChar(First, SAY2_Party))
                Type = SAY2_Party;
            else if (IsPrefixChar(First, SAY2_Tell))
                Type = SAY2_Tell;
            else if (IsPrefixChar(First, 6))
                Type = 6;
            else if (IsPrefixChar(First, 7))
                Type = 7;
            else if (IsPrefixChar(First, SAY2_Market))
                Type = SAY2_Market;
            else if (IsPrefixChar(First, SAY2_Ally))
                Type = SAY2_Ally;

            if (GIsOldChat && Type == SAY2_Normal)
                GConsole->SendChatMsgToServer(SAY2_Normal, Line, NULL);
            else
            {
                // '~' is the normal channel from any tab
                if (!GIsOldChat && IsPrefixChar(First, SAY2_Normal))
                    Type = SAY2_Normal;
                if (Type == SAY2_Tell)
                {
                    // "name text
                    INT NameLen = 0;
                    if (Line[1] != ' ')
                    {
                        do
                        {
                            if (NameLen >= (INT)wcslen(&Line[1]))
                                break;
                            NameLen++;
                        } while (Line[1 + NameLen] != ' ');
                    }
                    if (NameLen)
                    {
                        TCHAR Name[128];
                        wcsncpy(Name, &Line[1], NameLen);
                        Name[NameLen] = 0;
                        if ((INT)wcslen(Line) > NameLen + 2)
                        {
                            SetLastTellName(Name);
                            AddTellName(Name);
                            GConsole->SendChatMsgToServer(SAY2_Tell, &Line[NameLen + 2], Name);
                        }
                    }
                    else
                        GConsole->SendChatMsgToServer(SAY2_Tell, Line, NULL);
                }
                else
                {
                    // the prefix is not part of the message
                    if (!GIsOldChat)
                    {
                        if (IsPrefixChar(First, SAY2_Shout) || IsPrefixChar(First, SAY2_Pledge) ||
                            IsPrefixChar(First, SAY2_Party) || IsPrefixChar(First, SAY2_Tell) ||
                            IsPrefixChar(First, 6) || IsPrefixChar(First, 7) ||
                            IsPrefixChar(First, SAY2_Market) || IsPrefixChar(First, SAY2_Ally) ||
                            IsPrefixChar(First, SAY2_Normal))
                            Message = &Line[1];
                        else
                            Message = &Line[0];
                    }
                    if (*Message)
                        GConsole->SendChatMsgToServer(Type, Message, NULL);
                }
            }
        }
    }
    else if (Command == NCMD_Builder)
    {
        if (BuilderLevel)
        {
            L2ParamStack Params(10);
            Params.PushBack(&Line[2]);
            GConsole->BuilderCommand(Params);
        }
    }
    else
    {
        switch (Command)
        {
        case NCMD_Loc:
        {
            FVector Loc = GConsole->GetMyLocation();
            TCHAR Buffer[128];
            appSprintf(Buffer, TEXT("%s X : %.1f  Y : %.1f Z :%.1f"), *GL2GameData.SysString[471],
                       Loc.X, Loc.Y, Loc.Z);
            AddChatMessage(Buffer, 0xFFB09B79, SAY2_System, 0);
            break;
        }
        case NCMD_Petition:
            // "/gm text": the petition goes to the server; the window opens or closes
            if (wcslen(Line) >= 5)
                GConsole->RequestPetition(&Line[4]);
            GDesktop->TogglePetition();
            break;
        case NCMD_PetitionCancel:
            GConsole->RequestPetitionCancel();
            break;
        case NCMD_GMList:
            GConsole->RequestGMList();
            break;
        case NCMD_Sit:
            GConsole->RequestChangeWaitType(0);
            break;
        case NCMD_Stand:
            GConsole->RequestChangeWaitType(1);
            break;
        case NCMD_Walk:
            GConsole->RequestChangeMoveType(0);
            break;
        case NCMD_Run:
            GConsole->RequestChangeMoveType(1);
            break;
        case NCMD_Attack:
        {
            User* Target = GConsole->GetSelectedActor();
            User* Self = GConsole->GetMyUser();
            if (Self && Target)
                GConsole->RequestAttack(Target->ObjectID, Self->Pawn->Location);
            break;
        }
        case NCMD_AttackForce:
        {
            User* Target = GConsole->GetSelectedActor();
            User* Self = GConsole->GetMyUser();
            if (Self && Target)
                GConsole->RequestAttackForce(Target->ObjectID, Self->Pawn->Location);
            break;
        }
        case NCMD_AttackStand:
        {
            User* Target = GConsole->GetSelectedActor();
            User* Self = GConsole->GetMyUser();
            if (Self && Target)
                GConsole->RequestAttackStand(Target->ObjectID, Self->Pawn->Location);
            break;
        }
        case NCMD_Trade:
        {
            User* Target = GConsole->GetSelectedActor();
            if (Target && Target->Pawn)
                GConsole->RequestTrade(Target->Pawn->CreatureID);
            break;
        }
        case NCMD_Target:
        {
            TCHAR* Name = Parser.GetNextToken();
            if (Name)
            {
                User* Found = GConsole->FindUserByName(Name);
                if (Found && Found->Pawn)
                    GConsole->RequestTarget(Found->ObjectID);
            }
            break;
        }
        case NCMD_TargetNext:
        {
            User* Target = GConsole->GetSelectedActor();
            GConsole->RequestTargetNext(200, Target && Target->Pawn ? Target->Pawn->CreatureID : -1);
            break;
        }
        case NCMD_PickUp:
            GConsole->RequestPickUp(200, -1);
            break;
        case NCMD_Assist:
            GConsole->RequestAssist();
            break;
        case NCMD_PartyInvite:
        {
            User* Target = GConsole->GetSelectedActor();
            if (!Target || Target->Unknown8)
            {
                GConsole->ShowSystemMessage(185, 0xFFB09B79);
                break;
            }
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Target->Pawn->CreatureID);
            Params.PushBack((void*)(INT_PTR)GConsole->PartyItemDistribution);
            GConsole->RequestPartyInvite(Params);
            break;
        }
        case NCMD_PartyLeave:
        {
            L2ParamStack Params(10);
            GConsole->RequestPartyLeave(Params);
            break;
        }
        case NCMD_PartyDismiss:
        {
            TCHAR* Name = Parser.GetNextToken();
            if (!Name)
            {
                GConsole->ShowSystemMessage(187, 0xFFB09B79);
                break;
            }
            L2ParamStack Params(10);
            Params.PushBack(Name);
            GConsole->RequestPartyDismiss(Params);
            break;
        }
        case NCMD_Vendor:
            GConsole->RequestVendor();
            break;
        case NCMD_PartyMatching:
            GDesktop->TogglePartyMatch();
            break;
        case NCMD_SocialHello:
            GConsole->RequestSocialAction(2);
            break;
        case NCMD_SocialVictory:
            GConsole->RequestSocialAction(3);
            break;
        case NCMD_SocialCharge:
            GConsole->RequestSocialAction(4);
            break;
        case NCMD_UseSkill:
            UseSkillCommand(Parser, 0, 0);
            break;
        case NCMD_UseSkillForce:
            UseSkillCommand(Parser, 1, 0);
            break;
        case NCMD_UseSkillStand:
            UseSkillCommand(Parser, 0, 1);
            break;
        case NCMD_UseShortcut:
            UseShortcutCommand(Parser, 0, 0);
            break;
        case NCMD_UseShortcutForce:
            UseShortcutCommand(Parser, 1, 0);
            break;
        case NCMD_UseShortcutStand:
            UseShortcutCommand(Parser, 0, 1);
            break;
        case NCMD_AllyInvite:
        {
            User* Target = GConsole->GetSelectedActor();
            if (!Target || Target->Unknown8)
            {
                GConsole->ShowSystemMessage(511, 0xFFB09B79);
                break;
            }
            L2ParamStack Params(10);
            Params.PushBack((void*)(INT_PTR)Target->Pawn->CreatureID);
            GConsole->ShowSystemMessage(510, 0xFFB09B79);
            GConsole->RequestAllyInvite(Params);
            break;
        }
        // the alliance commands that need a confirmation: the dialog box sends the request
        case NCMD_AllyLeave:
        {
            NDialogInfo Info;
            Info.Owner = Entry;
            Info.ID = 0x2D;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.SetSystemMessage(0x200);
            GConsole->ShowDialog(&Info);
            break;
        }
        case NCMD_AllyOust:
        {
            NDialogInfo Info;
            Info.bMainBox = 1;
            Info.bInput = 1;
            Info.ID = 0x2E;
            Info.Type = 0;
            Info.Owner = Entry;
            Info.SetSystemMessage(0x201);
            GConsole->ShowDialog(&Info);
            break;
        }
        case NCMD_AllyDismiss:
        {
            NDialogInfo Info;
            Info.ID = 0x2C;
            Info.bMainBox = 1;
            Info.bInput = 0;
            Info.Owner = Entry;
            Info.SetSystemMessage(0x202);
            GConsole->ShowDialog(&Info);
            break;
        }
        case NCMD_AllyCrest:
        {
            NDialogInfo Info;
            Info.Owner = Entry;
            Info.ID = 0x2B;
            Info.bMainBox = 1;
            Info.bInput = 1;
            Info.Type = 0;
            Info.SetSystemMessage(0x203);
            GConsole->ShowDialog(&Info);
            break;
        }
        case NCMD_AllyInfo:
            GConsole->RequestAllyInfo();
            break;
        case NCMD_FriendInvite:
            NAME_REQUEST(RequestFriendInvite)
            break;
        case NCMD_FriendList:
            GConsole->RequestFriendList();
            break;
        case NCMD_FriendDel:
            NAME_REQUEST(RequestFriendDel)
            break;
        case NCMD_Buy:
            GConsole->RequestBuy();
            break;
        case NCMD_AllyWarStart:
            NAME_REQUEST(RequestStartAllianceWar)
            break;
        case NCMD_AllyWarStop:
            NAME_REQUEST(RequestStopAllianceWar)
            break;
        case NCMD_AllyWarSurrender:
            NAME_REQUEST(RequestSurrenderAllianceWar)
            break;
        case NCMD_BlockList:
        {
            L2ParamStack Params(10);
            Params.PushBack((void*)2);
            GConsole->RequestBlock(Params);
            break;
        }
        case NCMD_Block:
        case NCMD_Unblock:
        {
            TCHAR* Name = Parser.GetNextToken();
            if (Name)
            {
                L2ParamStack Params(10);
                Params.PushBack((void*)(INT_PTR)(Command == NCMD_Unblock));
                Params.PushBack(Name);
                GConsole->RequestBlock(Params);
            }
            break;
        }
        case NCMD_SocialNo:
            GConsole->RequestSocialAction(5);
            break;
        case NCMD_SocialYes:
            GConsole->RequestSocialAction(6);
            break;
        case NCMD_SocialBow:
            GConsole->RequestSocialAction(7);
            break;
        case NCMD_SocialUnaware:
            GConsole->RequestSocialAction(8);
            break;
        case NCMD_SocialWaiting:
            GConsole->RequestSocialAction(9);
            break;
        case NCMD_SocialLaugh:
            GConsole->RequestSocialAction(10);
            break;
        // NCMD_Escape: nothing here
        case NCMD_DwarvenManufacture:
            GConsole->RequestDwarvenManufacture();
            break;
        }
    }

    EditBox->ClearText();
    if (GIsOldChat)
    {
        if (CurTab == CHATTAB_Pledge)
            EditBox->InsertText(GChatPrefix[SAY2_Pledge][0], 0);
        else if (CurTab == CHATTAB_Party)
            EditBox->InsertText(GChatPrefix[SAY2_Party][0], 0);
        else if (CurTab == CHATTAB_Market)
            EditBox->InsertText(GChatPrefix[SAY2_Market][0], 0);
        else if (CurTab == CHATTAB_Ally)
            EditBox->InsertText(GChatPrefix[SAY2_Ally][0], 0);
    }
    if (GIsEnterChatting)
        EditBox->KillFocus();
    return 1;
}
NW_HOOK_METHOD(0x1003b6a0, NCChatWnd, OnEditCommand)

#undef NAME_REQUEST

// 0x100383f0 - the saved height restored (NConsoleWnd::LoadWindowPositions: the default
// height minus the saved one)
INT NCChatWnd::ResizeBy(INT Delta)
{
    return OnResize(Delta);
}
NW_HOOK_METHOD(0x100383f0, NCChatWnd, ResizeBy)

// 0x10038400 - the next tab, or the one before, around the five
void NCChatWnd::CycleTab(UBOOL bNext)
{
    if (bNext)
        TabCtrl->SetTab(CurTab < 4 ? CurTab + 1 : 0);
    else
        TabCtrl->SetTab(CurTab > 0 ? CurTab - 1 : 4);
}
NW_HOOK_METHOD(0x10038400, NCChatWnd, CycleTab)


// 0x1003ca80 - the input line, every list and the tell partners emptied (desktop reset)
void NCChatWnd::ClearAll()
{
    EditBox->ClearText();
    Lists[CHATLIST_All]->Clear();
    Lists[CHATLIST_Pledge]->Clear();
    Lists[CHATLIST_Party]->Clear();
    Lists[CHATLIST_Market]->Clear();
    Lists[CHATLIST_Ally]->Clear();
    bTellBrowse = 0;
    for (INT i = 0; i < TellNames.Num(); i++)
    {
        NString* Name = TellNames(i);
        if (Name)
        {
            Name->~NString();
            appFree(Name);
        }
    }
    TellNames.Empty();
}
NW_HOOK_METHOD(0x1003ca80, NCChatWnd, ClearAll)

NW_CHECK_VTABLE(NCChatWnd, 0x100e0640, 71)

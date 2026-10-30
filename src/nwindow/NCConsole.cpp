/*=============================================================================
    NCConsole.cpp - the desktop. Stock: 0x1003d740 .. 0x1003d7d1, 0x1003d7e0 .. 0x1003daa0,
    0x1003dac0 .. 0x1003e716, 0x1003e950 .. 0x10040f1e, 0x10041800 .. 0x10041b19,
    0x10041bc0 .. 0x100424d1, 0x10042530 .. 0x100427ba (HideWindow is NCChatWnd's body; the
    TArray<NChatBalloon> code at 0x10041b20 and 0x100424e0 is the template's).
=============================================================================*/
#include "nwindow/NCConsole.h"
#include "nwindow/NCAbnormalStatusWnd.h"
#include "nwindow/NCActionWnd.h"
#include "nwindow/NCBoardWnd.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCDeliverWnd.h"
#include "nwindow/NCDetailStatusWnd.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCGMWnd.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCInventoryWnd.h"
#include "nwindow/NCMagicSkillWnd.h"
#include "nwindow/NCMainWnd.h"
#include "nwindow/NCMatchOptionWnd.h"
#include "nwindow/NCMatchWnd.h"
#include "nwindow/NCMenuWnd.h"
#include "nwindow/NCMinimapWnd.h"
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCOptionWnd.h"
#include "nwindow/NCPartyWnd.h"
#include "nwindow/NCPetitionWnd.h"
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCPrivateBuyWnd.h"
#include "nwindow/NCPrivateShopWnd.h"
#include "nwindow/NCRadarWnd.h"
#include "nwindow/NCShopWnd.h"
#include "nwindow/NCShortCutWnd.h"
#include "nwindow/NCSiegeInfoMainWnd.h"
#include "nwindow/NCStatusWnd.h"
#include "nwindow/NCStoreWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCSystemMenuWnd.h"
#include "nwindow/NCTestHtmlViewer.h"
#include "nwindow/NCTradeWnd.h"
#include "nwindow/NCTrainWnd.h"
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCVIPShopWnd.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NClasses.gen.h"

#include <stdio.h>
#include <wchar.h>

// OnCreate numbers the windows it makes, in order (NCWnd +0x78; name ours)
#if NW_HOOK_MODE
#define GNextWndNumber NW_GLOBAL(INT, 0x10123d58)
#else
extern INT GNextWndNumber;
#endif

// interface sounds of the toggles (names ours)
enum { SOUND_WindowOpen = 5, SOUND_WindowClose = 6, SOUND_Minimap = 8, SOUND_Petition = 10 };

// the player's petition window (NConsoleWnd::PetitionWnd is declared as an NCWnd)
static FORCEINLINE NCPetitionWnd* PlayerPetitionWnd()
{
    return static_cast<NCPetitionWnd*>(GConsole->PetitionWnd);
}

// the console's options window
static FORCEINLINE NCWnd* OptionWnd()
{
    return GConsole->OptionWnd;
}

// NCStoreWnd::GetMode; the stock code calls it on the NPC shop too, whose mode is at the same
// place (+0xBC)
static FORCEINLINE INT StoreWndMode(NCWnd* Store)
{
    return static_cast<NCStoreWnd*>(Store)->GetMode();
}

/*-----------------------------------------------------------------------------
    Construction.
-----------------------------------------------------------------------------*/

// 0x10042530 - most window fields are left as allocated
NCConsole::NCConsole()
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    HelpWnd = NULL;
    Owner = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MatchOptionWnd = NULL;
    MinimapWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[1] = 0;
    Unknown170[0] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
}
NW_HOOK_CTOR(0x10042530, NCConsole)

// 0x100426c0 - NConsoleWnd::Initialize's; also clears TestNpcHtmlWnd
NCConsole::NCConsole(NConsoleWnd* InOwner)
{
    ChatWnd = NULL;
    NpcHtmlWnd = NULL;
    HelpWnd = NULL;
    InventoryWnd = NULL;
    TargetStatusWnd = NULL;
    PlayerStatusWnd = NULL;
    AbnormalStatusWnd = NULL;
    MenuWnd = NULL;
    SystemMenuWnd = NULL;
    ShortCutWnd = NULL;
    TrainWnd = NULL;
    MatchWnd = NULL;
    MatchOptionWnd = NULL;
    MinimapWnd = NULL;
    BoardWnd = NULL;
    PetWnd = NULL;
    GMWnd = NULL;
    GMInventoryWnd = NULL;
    GMMagicSkillWnd = NULL;
    GMDetailStatusWnd = NULL;
    GMQuestWnd = NULL;
    GMStoreWnd = NULL;
    TestNpcHtmlWnd = NULL;
    BlackTex = NULL;
    BalloonTex = NULL;
    UnknownA8 = 1;
    Unknown150 = 1;
    BalloonTimer = 0;
    Ticks = 0;
    Unknown170[1] = 0;
    Unknown170[0] = 0;
    Unknown170[2] = 0;
    Unknown170[3] = 0;
    Owner = InOwner;
}
NW_HOOK_CTOR_ARGS(0x100426c0, NCConsole, (NConsoleWnd* A), (A))

// 0x10042660 - Balloons' destructor is TArray<NChatBalloon>'s (0x100424e0)
NW_HOOK_DTOR(0x10042660, NCConsole)

/*-----------------------------------------------------------------------------
    Messages.
-----------------------------------------------------------------------------*/

// 0x1003e950 (vslot 24) - NID_CONSOLE_QUIT quits (stock: a call to NCAuthWnd::OnCancel's
// body, 0x1006a8e0)
INT NCConsole::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CONSOLE_QUIT)
        {
            appRequestExit(0);
            return 1;
        }
    }
    else if (Msg == (UINT)NWM_MOVETO)
        return OnMoveTo(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1003e950, NCConsole, WindowProc)

// 0x1003d7e0 - the player walks to the FVector wParam points at
INT NCConsole::OnMoveTo(WPARAM wParam, LPARAM lParam)
{
    Owner->MoveToLocation(*(FVector*)wParam, 0);
    return 1;
}
NW_HOOK_METHOD(0x1003d7e0, NCConsole, OnMoveTo)

// 0x10042400 (vslot 34) - each second the balloons older than 5 seconds go (and off their
// pawns)
INT NCConsole::OnTimer(INT TimerID)
{
    if (TimerID == NTIMER_BALLOONS)
    {
        Ticks++;
        for (INT i = 0; i < Balloons.Num();)
        {
            NChatBalloon& Balloon = Balloons(i);
            if (Balloon.Tick + 5 < Ticks)
            {
                User* Speaker = GConsole->FindUserByID(Balloon.ObjectID);
                if (Speaker && Speaker->Pawn)
                {
                    Speaker->Pawn->ChatBalloon = NULL;
                    Speaker->Pawn->Unknown5C++;
                }
                if (Balloon.Text)
                {
                    delete[] Balloon.Text;
                    Balloon.Text = NULL;
                }
                Balloons.Remove(i, 1);
            }
            else
                i++;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x10042400, NCConsole, OnTimer)

// 0x1003d740 (vslot 50) - draws nothing
INT NCConsole::OnPaint(NCGDevice* Device)
{
    guard(NCConsole::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1003d740, NCConsole, OnPaint)

/*-----------------------------------------------------------------------------
    Creation.
-----------------------------------------------------------------------------*/

// A child hidden before its creation message (inlined in the stock OnCreate for eight
// windows: the shortcut bar, the trainer, the party matching and its options, the minimap, the
// GM window and the two test pages; name ours): registered, hidden, then attached (entry,
// screen position, WM_CREATE)
static FORCEINLINE void CreateHiddenChild(NCWnd* Parent, NCWnd* Child, DWORD ChildStyle)
{
    NWndEntry* E = Parent->AddChildWindow(Child, ChildStyle);
    Child->HideWindow();
    Child->AttachEntry(E);
}

// 0x1003e9e0 (vslot 35) - every in-game window. Most are numbered (NCWnd +0x78) in creation
// order; the shops, the freight, the dialogs and a few more are not. Positions come from the
// size of the desktop: LocalW / LocalH for some, Width / Height for others, halved unsigned.
INT NCConsole::OnCreate()
{
    DWORD ScreenW = LocalW;
    DWORD ScreenH = LocalH;

    MainWnd = new NCMainWnd;
    MainWnd->SetWindowRect(207, 50, 240, 365);
    CreateChild(MainWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    MainWnd->Unknown78 = ++GNextWndNumber;

    GMPetitionWnd = new NCGMPetitionWnd;
    GMPetitionWnd->SetWindowRect(200, 200, 300, 301);
    CreateChild(GMPetitionWnd, NWS_VISIBLE);
    GMPetitionWnd->SetTitle(*GL2GameData.SysString[472]);
    GMPetitionWnd->Unknown78 = ++GNextWndNumber;

    NpcHtmlWnd = new NCNPCHtmlViewer;
    NpcHtmlWnd->SetWindowRect(300, 250, 300, 277);
    CreateChild(NpcHtmlWnd, NWS_VISIBLE | NWS_TOPMOST);
    NpcHtmlWnd->HideWindow();
    NpcHtmlWnd->Unknown78 = ++GNextWndNumber;
    NpcHtmlWnd->SetTitle(*GL2GameData.SysString[444]);

    BoardWnd = new NCBoardWnd;
    BoardWnd->SetWindowRect(60, 20, 646, 483);
    CreateChild(BoardWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    BoardWnd->HideWindow();
    BoardWnd->Unknown78 = ++GNextWndNumber;
    BoardWnd->SetTitle(*GL2GameData.SysString[387]);

    InventoryWnd = new NCInventoryWnd;
    InventoryWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(InventoryWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    InventoryWnd->Unknown78 = ++GNextWndNumber;
    InventoryWnd->SetTitle(*GL2GameData.SysString[138]);

    PlayerStatusWnd = new NCPlayerStatusWnd(100);
    PlayerStatusWnd->SetWindowRect(0, 0, 152, 71);
    CreateChild(PlayerStatusWnd, NWS_VISIBLE | NWS_TOPMOST);
    PlayerStatusWnd->Unknown78 = ++GNextWndNumber;

    TargetStatusWnd = new NCTargetStatusWnd(130);
    DWORD HalfW = ScreenW / 2;
    TargetStatusWnd->SetWindowRect(HalfW - 76, 0, 152, 36);
    CreateChild(TargetStatusWnd, NWS_VISIBLE | NWS_TOPMOST);
    TargetStatusWnd->Unknown78 = ++GNextWndNumber;

    AbnormalStatusWnd = new NCAbnormalStatusWnd(24);
    AbnormalStatusWnd->SetWindowRect(154, 2, 252, 50);
    CreateChild(AbnormalStatusWnd, NWS_VISIBLE | NWS_TOPMOST);
    AbnormalStatusWnd->Unknown78 = ++GNextWndNumber;

    PartyWnd = new NCPartyWnd;
    PartyWnd->SetWindowRect(0, 72, 152, 36);
    CreateChild(PartyWnd, NWS_VISIBLE | NWS_TOPMOST);
    PartyWnd->Unknown78 = ++GNextWndNumber;

    PetStatusWnd = new NCPetStatusWnd(100);
    PetStatusWnd->SetWindowRect(152, 0, 152, 43);
    CreateChild(PetStatusWnd, NWS_VISIBLE | NWS_TOPMOST);
    PetStatusWnd->Unknown78 = ++GNextWndNumber;

    SummonedStatusWnd = new NCSummonedStatusWnd(100);
    SummonedStatusWnd->SetWindowRect(152, 0, 152, 43);
    CreateChild(SummonedStatusWnd, NWS_VISIBLE | NWS_TOPMOST);
    SummonedStatusWnd->Unknown78 = ++GNextWndNumber;

    StoreWnd = new NCStoreWnd;
    StoreWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(StoreWnd, NWS_VISIBLE | NWS_TOPMOST);
    StoreWnd->Unknown78 = ++GNextWndNumber;
    StoreWnd->SetTitle(*GL2GameData.SysString[131]);

    DeliverWnd = new NCDeliverWnd;
    DeliverWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(DeliverWnd, NWS_VISIBLE | NWS_TOPMOST);
    DeliverWnd->SetTitle(*GL2GameData.SysString[557]);

    SelectDeliverWnd = new NCSelectDeliverWnd;
    SelectDeliverWnd->SetWindowRect(300, 250, 300, 277);
    CreateChild(SelectDeliverWnd, NWS_VISIBLE | NWS_TOPMOST);
    SelectDeliverWnd->SetTitle(*GL2GameData.SysString[558]);

    ShopWnd = new NCShopWnd;
    ShopWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(ShopWnd, NWS_VISIBLE | NWS_TOPMOST);
    ShopWnd->Unknown78 = ++GNextWndNumber;
    ShopWnd->SetTitle(*GL2GameData.SysString[136]);

    VIPShopWnd = new NCVIPShopWnd;
    VIPShopWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(VIPShopWnd, NWS_VISIBLE | NWS_TOPMOST);
    VIPShopWnd->SetTitle(*GL2GameData.SysString[136]);

    TradeWnd = new NCTradeWnd;
    TradeWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(TradeWnd, NWS_VISIBLE | NWS_TOPMOST);
    TradeWnd->Unknown78 = ++GNextWndNumber;
    TradeWnd->SetTitle(*GL2GameData.SysString[445]);

    MenuWnd = new NCMenuWnd;
    MenuWnd->SetWindowRect(Width - 166, Height - 44, 167, 44);
    CreateChild(MenuWnd, NWS_VISIBLE | NWS_TOPMOST);
    MenuWnd->Unknown78 = ++GNextWndNumber;

    SystemMenuWnd = new NCSystemMenuWnd;
    SystemMenuWnd->SetWindowRect(Width - 290, 250, 240, 122);
    CreateChild(SystemMenuWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    SystemMenuWnd->SetTitle(*GL2GameData.SysString[446]);

    HelpWnd = new NCNPCHtmlViewer;
    DWORD HalfH = ScreenH / 2;
    HelpWnd->SetWindowRect(HalfW - 162, HalfH - 139, 300, 277);
    CreateChild(HelpWnd, NWS_VISIBLE | NWS_TOPMOST);
    HelpWnd->HideWindow();
    HelpWnd->Unknown78 = ++GNextWndNumber;
    HelpWnd->SetTitle(*GL2GameData.SysString[145]);

    MainDialogBox = new NCDialogBox;
    MainDialogBox->SetWindowRect((DWORD)Width / 2 - 101, (DWORD)Height / 2 - 43, 203, 87);
    CreateChild(MainDialogBox, NWS_VISIBLE | 0x20);
    MainDialogBox->SetButtonTexts(140, 141);

    SecondDialogBox = new NCDialogBox;
    SecondDialogBox->SetWindowRect(347, Height - 89, 203, 87);
    CreateChild(SecondDialogBox, NWS_VISIBLE | 0x20);
    SecondDialogBox->SetButtonTexts(184, 185);

    ShortCutWnd = new NCShortCutWnd(10, 10, 32);
    ShortCutWnd->SetWindowRect(Width - 45, (DWORD)Height / 2 - 218, 44, 437);
    CreateHiddenChild(this, ShortCutWnd, NWS_VISIBLE | 0x20 | NWS_TOPMOST);
    ShortCutWnd->Unknown78 = ++GNextWndNumber;

    TrainWnd = new NCTrainWnd;
    TrainWnd->SetWindowRect(ScreenW - 258, 64, 240, 365);
    CreateHiddenChild(this, TrainWnd, NWS_VISIBLE | NWS_TOPMOST);
    TrainWnd->Unknown78 = ++GNextWndNumber;

    MatchWnd = new NCMatchWnd;
    MatchWnd->SetWindowRect(ScreenW - 350, 215, 300, 355);
    CreateHiddenChild(this, MatchWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    MatchWnd->Unknown78 = ++GNextWndNumber;
    MatchWnd->SetTitle(*GL2GameData.SysString[389]);

    MatchOptionWnd = new NCMatchOptionWnd;
    MatchOptionWnd->SetWindowRect(HalfW - 120, HalfH - 80, 240, 161);
    CreateHiddenChild(this, MatchOptionWnd, NWS_VISIBLE);

    MinimapWnd = new NCMinimapWnd;
    MinimapWnd->SetWindowRect(HalfW - 167, HalfH - 178, 334, 357);
    CreateHiddenChild(this, MinimapWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    MinimapWnd->Unknown78 = ++GNextWndNumber;
    MinimapWnd->SetTitle(*GL2GameData.SysString[447]);

    PrivateShopWnd = new NCPrivateShopWnd;
    PrivateShopWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(PrivateShopWnd, NWS_VISIBLE | NWS_TOPMOST);
    PrivateShopWnd->HideWindow();
    PrivateShopWnd->Unknown78 = ++GNextWndNumber;
    PrivateShopWnd->SetTitle(*GL2GameData.SysString[596]);

    PrivateBuyWnd = new NCPrivateBuyWnd;
    PrivateBuyWnd->SetWindowRect(ScreenW - 290, 205, 240, 365);
    CreateChild(PrivateBuyWnd, NWS_VISIBLE | NWS_TOPMOST);
    PrivateBuyWnd->HideWindow();
    PrivateBuyWnd->SetTitle(*GL2GameData.SysString[597]);

    TutorialWnd = new NCTutorialViewerWnd;
    TutorialWnd->SetWindowRect(605, 250, 300, 277);
    CreateChild(TutorialWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_NOCLOSE);
    TutorialWnd->HideWindow();
    TutorialWnd->SetTitle(*GL2GameData.SysString[448]);

    TutorialQuestionWnd = new NCTutorialQuestionWnd;
    TutorialQuestionWnd->SetWindowRect(5, ScreenH - 218, 32, 32);
    CreateChild(TutorialQuestionWnd, NWS_VISIBLE | NWS_TOPMOST);
    TutorialQuestionWnd->HideWindow();

    RadarWnd = new NCRadarWnd;
    RadarWnd->SetWindowRect(ScreenW - 102, 0, 102, 102);
    CreateChild(RadarWnd, NWS_VISIBLE | NWS_NOHITTEST);

    PetWnd = new NCPetWnd;
    PetWnd->SetWindowRect(207, 50, 240, 213);
    CreateChild(PetWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    PetWnd->HideWindow();
    PetWnd->SetTitle(*GL2GameData.SysString[494]);

    SummonedWnd = new NCSummonedWnd;
    SummonedWnd->SetWindowRect(207, 50, 240, 213);
    CreateChild(SummonedWnd, NWS_VISIBLE | NWS_TOPMOST | NWS_MINIMIZABLE);
    SummonedWnd->HideWindow();
    SummonedWnd->SetTitle(*GL2GameData.SysString[495]);

    ChatWnd = new NCChatWnd;
    ChatWnd->SetWindowRect(0, Height - 173, 345, 174);
    CreateChild(ChatWnd, NWS_VISIBLE | NWS_NOHITTEST | NWS_TOPMOST);
    ChatWnd->Unknown78 = ++GNextWndNumber;

    GMWnd = new NCGMWnd;
    GMWnd->SetWindowRect(Width - 209, Height - 327, 208, 229);
    CreateHiddenChild(this, GMWnd, NWS_VISIBLE);
    GMWnd->Unknown78 = ++GNextWndNumber;

    GMInventoryWnd = new NCGMInventoryWnd;
    DWORD GMX = ScreenW - 209;
    GMInventoryWnd->SetWindowRect(GMX, 100, 240, 506);
    CreateChild(GMInventoryWnd, NWS_VISIBLE);
    GMInventoryWnd->Unknown78 = ++GNextWndNumber;

    GMMagicSkillWnd = new NCGMMagicSkillWnd;
    GMMagicSkillWnd->SetWindowRect(GMX, 100, 240, 313);
    CreateChild(GMMagicSkillWnd, NWS_VISIBLE);
    GMMagicSkillWnd->Unknown78 = ++GNextWndNumber;

    GMDetailStatusWnd = new NCGMDetailStatusWnd;
    GMDetailStatusWnd->SetWindowRect(0, 100, 240, 321);
    CreateChild(GMDetailStatusWnd, NWS_VISIBLE);
    GMDetailStatusWnd->Unknown78 = ++GNextWndNumber;

    GMQuestWnd = new NCGMQuestWnd;
    GMQuestWnd->SetWindowRect(GMX, 100, 240, 313);
    CreateChild(GMQuestWnd, NWS_VISIBLE | 0x20);
    GMQuestWnd->Unknown78 = ++GNextWndNumber;

    GMStoreWnd = new NCGMStoreWnd;
    GMStoreWnd->SetWindowRect(GMX, 100, 240, 506);
    CreateChild(GMStoreWnd, NWS_VISIBLE | 0x20);
    GMStoreWnd->Unknown78 = ++GNextWndNumber;

    GMActionWnd = new NCGMActionWnd;
    GMActionWnd->SetWindowRect(GMX, 100, 240, 313);
    CreateChild(GMActionWnd, NWS_VISIBLE);
    GMActionWnd->Unknown78 = ++GNextWndNumber;

    SiegeInfoWnd = new NCSiegeInfoMainWnd;
    SiegeInfoWnd->SetWindowRect(200, 100, 240, 365);
    CreateChild(SiegeInfoWnd, NWS_VISIBLE | NWS_TOPMOST);
    SiegeInfoWnd->SetTitle(*GL2GameData.SysString[593]);

    if (GIsOnlyL2Console)
    {
        TestNpcHtmlWnd = new NCNPCHtmlViewer;
        TestNpcHtmlWnd->SetWindowRect(60, 20, 646, 483);
        CreateHiddenChild(this, TestNpcHtmlWnd, NWS_VISIBLE | 0x20 | NWS_TOPMOST);
        TestNpcHtmlWnd->HtmlViewer->LoadHtmlFile((TCHAR*)TEXT("test.html"));
    }
    if (GIsTestHtml)
    {
        TestHtmlWnd = new NCTestHtmlViewer;
        TestHtmlWnd->SetWindowRect(HalfW, HalfH, 208, 412);
        CreateHiddenChild(this, TestHtmlWnd, NWS_VISIBLE | 0x20);
    }

    BlackTex = FindTexture(TEXT("NWindow.BlackTexture"));
    BalloonTex = FindTexture(TEXT("NWindow.chat"));
    BalloonTimer = SetTimer(NTIMER_BALLOONS, 1000, NULL);
    return 1;
}
NW_HOOK_METHOD(0x1003e9e0, NCConsole, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10041bc0 (vslot 36) - the balloons and every window but the pet and summon status ones
// (never destroyed), then the timer
INT NCConsole::OnDestroy()
{
    for (INT i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
    Balloons.Empty();
    DestroyChild(this, ChatWnd);
    DestroyChild(this, NpcHtmlWnd);
    DestroyChild(this, HelpWnd);
    DestroyChild(this, GMPetitionWnd);
    DestroyChild(this, BoardWnd);
    DestroyChild(this, ShortCutWnd);
    DestroyChild(this, InventoryWnd);
    DestroyChild(this, TargetStatusWnd);
    DestroyChild(this, PlayerStatusWnd);
    DestroyChild(this, AbnormalStatusWnd);
    DestroyChild(this, StoreWnd);
    DestroyChild(this, ShopWnd);
    DestroyChild(this, VIPShopWnd);
    DestroyChild(this, MenuWnd);
    DestroyChild(this, SystemMenuWnd);
    DestroyChild(this, TradeWnd);
    DestroyChild(this, MainDialogBox);
    DestroyChild(this, SecondDialogBox);
    DestroyChild(this, TrainWnd);
    DestroyChild(this, MatchWnd);
    DestroyChild(this, MatchOptionWnd);
    DestroyChild(this, MinimapWnd);
    DestroyChild(this, PetWnd);
    DestroyChild(this, SummonedWnd);
    DestroyChild(this, GMWnd);
    DestroyChild(this, GMInventoryWnd);
    DestroyChild(this, GMMagicSkillWnd);
    DestroyChild(this, GMDetailStatusWnd);
    DestroyChild(this, GMQuestWnd);
    DestroyChild(this, GMStoreWnd);
    DestroyChild(this, GMActionWnd);
    DestroyChild(this, PrivateShopWnd);
    DestroyChild(this, PrivateBuyWnd);
    DestroyChild(this, MainWnd);
    DestroyChild(this, PartyWnd);
    DestroyChild(this, TutorialWnd);
    DestroyChild(this, TutorialQuestionWnd);
    DestroyChild(this, RadarWnd);
    DestroyChild(this, DeliverWnd);
    DestroyChild(this, SelectDeliverWnd);
    DestroyChild(this, SiegeInfoWnd);
    if (GIsOnlyL2Console)
        DestroyChild(this, TestNpcHtmlWnd);
    if (GIsTestHtml)
        DestroyChild(this, TestHtmlWnd);
    if (BalloonTimer)
    {
        KillTimer(BalloonTimer);
        BalloonTimer = 0;
    }
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10041bc0, NCConsole, OnDestroy)

// 0x1003d810 (vslot 42) - entering the game: the bars, the chat, the radar and the status
// windows show, the rest hides (the party and the pet / summon status windows are left)
void NCConsole::ShowWindow()
{
    NCWnd::ShowWindow();
    SiegeInfoWnd->HideWindow();
    DeliverWnd->HideWindow();
    SelectDeliverWnd->HideWindow();
    GMPetitionWnd->HideWindow();
    GConsole->PetitionWnd->HideWindow();
    MainWnd->HideWindow();
    BoardWnd->HideWindow();
    HelpWnd->HideWindow();
    ShortCutWnd->ShowWindow();
    ChatWnd->ShowWindow();
    InventoryWnd->HideWindow();
    TargetStatusWnd->ShowWindow();
    AbnormalStatusWnd->ShowWindow();
    PlayerStatusWnd->ShowWindow();
    MainDialogBox->HideWindow();
    SecondDialogBox->HideWindow();
    MenuWnd->ShowWindow();
    SystemMenuWnd->HideWindow();
    StoreWnd->HideWindow();
    ShopWnd->HideWindow();
    VIPShopWnd->HideWindow();
    TradeWnd->HideWindow();
    NpcHtmlWnd->HideWindow();
    TrainWnd->HideWindow();
    MatchWnd->HideWindow();
    MatchOptionWnd->HideWindow();
    MinimapWnd->HideWindow();
    TutorialWnd->HideWindow();
    TutorialQuestionWnd->HideWindow();
    RadarWnd->ShowWindow();
    PetWnd->HideWindow();
    SummonedWnd->HideWindow();
    GMWnd->HideWindow();
    GMInventoryWnd->HideWindow();
    GMMagicSkillWnd->HideWindow();
    GMDetailStatusWnd->HideWindow();
    GMQuestWnd->HideWindow();
    GMStoreWnd->HideWindow();
    GMActionWnd->HideWindow();
    PrivateShopWnd->HideWindow();
    PrivateBuyWnd->HideWindow();
    OptionWnd()->HideWindow();
    if (GIsOnlyL2Console)
        TestNpcHtmlWnd->ShowWindow();
    if (GIsTestHtml)
        TestHtmlWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1003d810, NCConsole, ShowWindow)

// stock body shared with NCChatWnd's (0x1003dab0)
void NCConsole::HideWindow()
{
    NCWnd::HideWindow();
}

/*-----------------------------------------------------------------------------
    Toggles and resets (the menus, the shortcut keys, the console).
-----------------------------------------------------------------------------*/

// (repeated inline in the stock CloseWindows)
static FORCEINLINE void HideIfShown(NCWnd* Wnd)
{
    if (Wnd->IsShowWindow())
        Wnd->HideWindow();
}

// 0x1003dac0 - the private stores are left (and closed on the server when set up), the
// trade cancelled, the party matching left, the options and petition windows hidden, then
// the whole interface shown or hidden. The trade test reads TrainWnd (+0xE8), not TradeWnd
// (+0xE0), in the stock code: with the trainer open the server is told the trade is off.
void NCConsole::ToggleInterface()
{
    if (PrivateShopWnd->IsShowWindow())
    {
        if (PrivateShopWnd->Mode == 1)
            GConsole->RequestPrivateStoreQuitSell();
        PrivateShopWnd->Reset();
    }
    if (PrivateBuyWnd->IsShowWindow())
    {
        if (PrivateBuyWnd->Mode == 1)
            GConsole->RequestPrivateStoreQuitBuy();
        PrivateBuyWnd->Reset();  // the stock code calls the sell window's body for both
    }
    if (TrainWnd->IsShowWindow())
        GConsole->RequestTradeDone(0);
    if (MatchWnd->IsShowWindow())
        GConsole->RequestPartyMatchList(3);
    if (OptionWnd())
        OptionWnd()->HideWindow();
    if (GConsole->PetitionWnd)
        GConsole->PetitionWnd->HideWindow();
    if (IsShowWindow())
        HideWindow();
    else
        ShowWindow();
}
NW_HOOK_METHOD(0x1003dac0, NCConsole, ToggleInterface)

// 0x1003dbc0 - the dialogs, shops, stores, menus and the radar close
void NCConsole::CloseWindows()
{
    HideIfShown(InventoryWnd);
    HideIfShown(ShopWnd);
    HideIfShown(VIPShopWnd);
    HideIfShown(StoreWnd);
    HideIfShown(TradeWnd);
    HideIfShown(SystemMenuWnd);
    HideIfShown(HelpWnd);
    HideIfShown(MainDialogBox);
    HideIfShown(SecondDialogBox);
    HideIfShown(RadarWnd);
    HideIfShown(PrivateShopWnd);
    HideIfShown(PrivateBuyWnd);
    HideIfShown(MainWnd);
    HideIfShown(GConsole->PetitionWnd);
    HideIfShown(GMPetitionWnd);
    HideIfShown(DeliverWnd);
    HideIfShown(SelectDeliverWnd);
    HideIfShown(SiegeInfoWnd);
}
NW_HOOK_METHOD(0x1003dbc0, NCConsole, CloseWindows)

// 0x1003de10 - leaving the game: the dialogs, the chat, the shortcuts, the buffs, the party
// matching list, the clan page, the radar and both petition windows empty
void NCConsole::Reset()
{
    MainDialogBox->Reset();
    SecondDialogBox->Reset();
    ChatWnd->ClearAll();
    ShortCutWnd->ClearShortCuts();
    AbnormalStatusWnd->ClearStatus();
    MatchWnd->ClearList();
    MainWnd->ActionWnd->ResetPledgeWnd();
    RadarWnd->ClearTargets();
    GMPetitionWnd->Clear();
    PlayerPetitionWnd()->Clear();
}
NW_HOOK_METHOD(0x1003de10, NCConsole, Reset)

// 0x1003de90 - the player's petition window, unless the GM one is up
void NCConsole::TogglePetition()
{
    if (GConsole->PetitionWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        GConsole->PetitionWnd->HideWindow();
    }
    else if (!GMPetitionWnd->IsShowWindow())
    {
        GConsole->PetitionWnd->ShowWindow();
        GConsole->Engine->OnInterfacePlaySound(SOUND_Petition);
    }
}
NW_HOOK_METHOD(0x1003de90, NCConsole, TogglePetition)

// 0x1003df20
void NCConsole::ToggleSystemMenu()
{
    if (SystemMenuWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        SystemMenuWnd->HideWindow();
    }
    else
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
        SystemMenuWnd->ShowWindow();
        SystemMenuWnd->SetFocus();
    }
}
NW_HOOK_METHOD(0x1003df20, NCConsole, ToggleSystemMenu)

// 0x1003df80 - not while a shop (mode 1), a warehouse (mode 0), a trade or the freight is
// open; opening asks the server for the item list (its answer shows the window)
void NCConsole::ToggleInventory()
{
    if (ShopWnd->IsShowWindow() && StoreWndMode(ShopWnd) == 1)
        return;
    if (StoreWnd->IsShowWindow() && !StoreWndMode(StoreWnd))
        return;
    if (TradeWnd->IsShowWindow() || DeliverWnd->IsShowWindow())
        return;
    if (InventoryWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        InventoryWnd->HideWindow();
    }
    else
    {
        GConsole->RequestItemList();
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
    }
}
NW_HOOK_METHOD(0x1003df80, NCConsole, ToggleInventory)

// 0x1003e050
void NCConsole::ToggleChat()
{
    if (ChatWnd->IsShowWindow())
        ChatWnd->HideWindow();
    else
        ChatWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1003e050, NCConsole, ToggleChat)

// 0x1003e080
void NCConsole::ToggleHelp()
{
    if (HelpWnd->IsShowWindow())
    {
        HelpWnd->HideWindow();
        return;
    }
    HelpWnd->LoadFile((TCHAR*)TEXT("..\\L2text\\help.htm"));
    HelpWnd->ShowWindow();
    HelpWnd->SetFocus();
}
NW_HOOK_METHOD(0x1003e080, NCConsole, ToggleHelp)

// 0x1003e0d0 - closing leaves the matching room (3); opening sends the player's saved
// options (MatchOption.ini, section = the character's name) and joins as registered or not
void NCConsole::TogglePartyMatch()
{
    if (MatchWnd->IsShowWindow())
    {
        GConsole->RequestPartyMatchList(3);
        MatchWnd->HideWindow();
        return;
    }
    if (!GConsole->GetMyUser())
        return;
    const TCHAR* Section = GConsole->GetMyUser()->GetName();
    UBOOL bAutoRegister, bLevelOpen, bClassOpen;
    TCHAR Memo[256];
    if (!GConfig->GetBool(Section, TEXT("autoregister"), bAutoRegister, TEXT("MatchOption.ini")))
        bAutoRegister = 1;
    if (!GConfig->GetBool(Section, TEXT("levelopen"), bLevelOpen, TEXT("MatchOption.ini")))
        bLevelOpen = 0;
    if (!GConfig->GetBool(Section, TEXT("classopen"), bClassOpen, TEXT("MatchOption.ini")))
        bClassOpen = 0;
    if (!GConfig->GetString(Section, TEXT("memo"), Memo, 256, TEXT("MatchOption.ini")))
        Memo[0] = 0;
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)bAutoRegister);
    Params.PushBack((void*)(INT_PTR)bLevelOpen);
    Params.PushBack((void*)(INT_PTR)bClassOpen);
    Params.PushBack(Memo);
    GConsole->RequestPartyMatchConfig(Params);
    GConsole->RequestPartyMatchList(bAutoRegister ? 1 : 0);
    MatchOptionWnd->LoadOptions();
    MatchWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1003e0d0, NCConsole, TogglePartyMatch)

// 0x1003e2b0 - shown centered on the player
void NCConsole::ToggleMinimap()
{
    if (MinimapWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_Minimap);
        MinimapWnd->HideWindow();
    }
    else
    {
        MinimapWnd->ShowWindow();
        MinimapWnd->CenterOnPlayer();
        GConsole->Engine->OnInterfacePlaySound(SOUND_Minimap);
    }
}
NW_HOOK_METHOD(0x1003e2b0, NCConsole, ToggleMinimap)

// 0x1003e320
void NCConsole::ToggleMatchOption()
{
    if (MatchOptionWnd->IsShowWindow())
        MatchOptionWnd->HideWindow();
    else
        MatchOptionWnd->ShowWindow();
}
NW_HOOK_METHOD(0x1003e320, NCConsole, ToggleMatchOption)

// 0x1003e350 - opening asks the server for the board (its answer shows the window)
void NCConsole::ToggleBoard()
{
    if (BoardWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        BoardWnd->HideWindow();
        return;
    }
    L2ParamStack Params(10);
    GConsole->RequestShowBoard(Params);
    GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
}
NW_HOOK_METHOD(0x1003e350, NCConsole, ToggleBoard)

// 0x1003e410 - the GM window's name box takes the focus
void NCConsole::ToggleGMWnd()
{
    if (GMWnd->IsShowWindow())
    {
        GMWnd->HideWindow();
        return;
    }
    GMWnd->ShowWindow();
    GMWnd->NameEdit->SetFocus();
}
NW_HOOK_METHOD(0x1003e410, NCConsole, ToggleGMWnd)

// the GM view windows: hidden, or the server asked about the player Name and the window
// shown (repeated in the six stock toggles)
#define NCCONSOLE_GM_TOGGLE(Method, Wnd, Request)                                             \
    void NCConsole::Method(TCHAR* Name)                                                       \
    {                                                                                         \
        if (Wnd->IsShowWindow())                                                              \
        {                                                                                     \
            Wnd->HideWindow();                                                                \
            return;                                                                           \
        }                                                                                     \
        GConsole->Request(Name);                                                              \
        Wnd->ShowWindow();                                                                    \
        Wnd->SetFocus();                                                                      \
    }

// 0x1003e450
NCCONSOLE_GM_TOGGLE(ToggleGMInventory, GMInventoryWnd, RequestGMInventory)
NW_HOOK_METHOD(0x1003e450, NCConsole, ToggleGMInventory)
// 0x1003e4b0
NCCONSOLE_GM_TOGGLE(ToggleGMMagicSkill, GMMagicSkillWnd, RequestGMSkills)
NW_HOOK_METHOD(0x1003e4b0, NCConsole, ToggleGMMagicSkill)
// 0x1003e510
NCCONSOLE_GM_TOGGLE(ToggleGMDetailStatus, GMDetailStatusWnd, RequestGMStatus)
NW_HOOK_METHOD(0x1003e510, NCConsole, ToggleGMDetailStatus)
// 0x1003e570
NCCONSOLE_GM_TOGGLE(ToggleGMQuest, GMQuestWnd, RequestGMQuests)
NW_HOOK_METHOD(0x1003e570, NCConsole, ToggleGMQuest)
// 0x1003e5d0
NCCONSOLE_GM_TOGGLE(ToggleGMStore, GMStoreWnd, RequestGMWarehouse)
NW_HOOK_METHOD(0x1003e5d0, NCConsole, ToggleGMStore)
// 0x1003e630
NCCONSOLE_GM_TOGGLE(ToggleGMAction, GMActionWnd, RequestGMPledge)
NW_HOOK_METHOD(0x1003e630, NCConsole, ToggleGMAction)

// 0x10041800 - a tab of the character window (NCMainWnd::ShowTab), or with -1 the window
// shown or hidden
void NCConsole::ToggleCharacter(INT Tab)
{
    if (Tab != -1)
    {
        MainWnd->ShowTab(Tab);
        return;
    }
    if (MainWnd->IsShowWindow())
    {
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowClose);
        MainWnd->HideWindow();
    }
    else
    {
        MainWnd->ShowWindow();
        GConsole->Engine->OnInterfacePlaySound(SOUND_WindowOpen);
    }
}
NW_HOOK_METHOD(0x10041800, NCConsole, ToggleCharacter)

/*-----------------------------------------------------------------------------
    Chat.
-----------------------------------------------------------------------------*/

// 0x1003e690 - a systemmsg line (type 5), when the message has a text
void NCConsole::AddSystemMessage(INT MsgID, DWORD Color)
{
    if (*GL2GameData.SystemMsg[MsgID].Message)
        ChatWnd->AddChatMessage(*GL2GameData.SystemMsg[MsgID].Message, Color, 5, 0);
}
NW_HOOK_METHOD(0x1003e690, NCConsole, AddSystemMessage)

// 0x1003e6f0 - a system line (type 5), when not empty
void NCConsole::AddSystemText(const TCHAR* Text, INT SubType, DWORD Color)
{
    if (Text && *Text)
        ChatWnd->AddChatMessage(Text, Color, 5, SubType);
}
NW_HOOK_METHOD(0x1003e6f0, NCConsole, AddSystemText)

// 0x10041880 - "Name: Text" of the chat types but the petition ones (6, 7: into the GM
// petition window, or the player's, shown, when the GM one is hidden). Then the speaker's
// balloon: its first 19 characters ("..." after a longer text) for 5 seconds, a newer one
// replacing it - not when the speaker's User +0x1F8 (16 bits) is set.
void NCConsole::Say(INT ObjectID, const TCHAR* Name, const TCHAR* Text, DWORD Color, INT Type)
{
    TCHAR Line[1024];
    _swprintf(Line, TEXT("%s: %s"), Name, Text);
    if (Type == 6 || Type == 7)
    {
        GMPetitionWnd->AddMessage(Line, Color, Type, 0);
        if (!GMPetitionWnd->IsShowWindow())
        {
            GConsole->PetitionWnd->ShowWindow();
            PlayerPetitionWnd()->AddMessage(Line, Color, Type, 0);
        }
        return;
    }
    ChatWnd->AddChatMessage(Line, Color, Type, 0);
    User* Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->PrivateStoreType)
        return;
    NChatBalloon Balloon;
    TCHAR* Short = new TCHAR[23];
    if (wcslen(Text) < 19)
        appStrcpy(Short, Text);
    else
    {
        wcsncpy(Short, Text, 19);
        Short[19] = '.';
        Short[20] = '.';
        Short[21] = '.';
        Short[22] = 0;
    }
    Balloon.Tick = Ticks;
    Balloon.ObjectID = ObjectID;
    Balloon.Text = Short;
    INT i;
    for (i = 0; i < Balloons.Num(); i++)
        if (Balloons(i).ObjectID == ObjectID)
            break;
    if (i < Balloons.Num())
    {
        if (Balloons(i).Text)
            delete[] Balloons(i).Text;
        Balloons(i) = Balloon;
    }
    else
        Balloons(Balloons.Add()) = Balloon;
    Speaker = GConsole->FindUserByID(ObjectID);
    if (Speaker && Speaker->Pawn)
    {
        if (!Speaker->Pawn->ChatBalloon)
            Speaker->Pawn->Unknown5C++;
        Speaker->Pawn->ChatBalloon = Short;
    }
}
NW_HOOK_METHOD(0x10041880, NCConsole, Say)

/*-----------------------------------------------------------------------------
    HTML page commands.
-----------------------------------------------------------------------------*/

// 0x10040b50 (vslot 62) - "Link <bypass>": to the server (the tutorial pages' links go their
// own way). "Write <board> <a> <b> <control> [<control> [<control>]]": a board post, the texts
// of the page's controls named; boards 4, 5 and 7 take "-" for the second text, 4 and 7 for
// the third. A missing third text still sends the post, and returns 0.
INT NCConsole::OnHtmlCommand(const TCHAR* Command, NCHtmlViewer* Source)
{
    const TCHAR* Str = Command;
    if (ParseCommand(&Str, TEXT("Link")))
    {
        TCHAR Bypass[256] = { 0 };
        if (ParseToken(Str, Bypass, 256, 1))
        {
            L2ParamStack Params(10);
            Params.PushBack(Bypass);
            if (Source->IsA(NClass::NCTutorialHtmlViewer()))
                GConsole->RequestTutorialLinkHtml(Params);
            else
                GConsole->RequestLinkHtml(Params);
        }
        return 1;
    }
    if (!ParseCommand(&Str, TEXT("Write")))
        return 0;
    L2ParamStack Params(10);
    TCHAR Board[256], Arg1[256], Arg2[256], Control1[256], Control2[256], Control3[256];
    if (!ParseToken(Str, Board, 256, 1))
        return 0;
    Params.PushBack(Board);
    if (!ParseToken(Str, Arg1, 256, 1))
        return 0;
    Params.PushBack(Arg1);
    if (!ParseToken(Str, Arg2, 256, 1))
        return 0;
    Params.PushBack(Arg2);
    if (!ParseToken(Str, Control1, 256, 1) || !Source)
        return 0;
    TCHAR* Text = Source->GetControlValue(Control1);
    if (!Text)
        return 0;
    Params.PushBack(Text);
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("5")) && appStrcmp(Board, TEXT("7")))
    {
        if (!ParseToken(Str, Control2, 256, 1))
            return 0;
        Text = Source->GetControlValue(Control2);
        if (!Text)
            return 0;
        Params.PushBack(Text);
    }
    else
    {
        appStrcpy(Control2, TEXT("-"));
        Params.PushBack(Control2);
    }
    if (appStrcmp(Board, TEXT("4")) && appStrcmp(Board, TEXT("7")))
    {
        if (ParseToken(Str, Control3, 256, 1))
        {
            Text = Source->GetControlValue(Control3);
            if (!Text)
            {
                GConsole->RequestBBSWrite(Params);
                return 0;
            }
            Params.PushBack(Text);
        }
    }
    else
    {
        appStrcpy(Control3, TEXT("-"));
        Params.PushBack(Control3);
    }
    GConsole->RequestBBSWrite(Params);
    return 1;
}
NW_HOOK_METHOD(0x10040b50, NCConsole, OnHtmlCommand)

// 0x1003e800 - a balloon's destructor (the text is freed by hand; Say's unwind code)
NW_HOOK_DTOR(0x1003e800, NChatBalloon)

NW_CHECK_VTABLE(NCConsole, 0x100e1028, 63)

#pragma once
// NCConsole - the desktop (UClass "CConsole", 0x180; NConsoleWnd +0x36DC, built by the
// console's Initialize): the screen-sized parent of every in-game window. OnCreate makes them
// all, numbered in order (NCWnd +0x78); ShowWindow (entering the game) hides the dialogs and
// shows the bars, the chat and the status windows. It owns the toggles the menus and the
// shortcut keys use, the HTML page commands ("Link", "Write") and the chat balloons: the
// first 19 characters a character says are drawn over it (its pawn's +0x142C) for 5 seconds.

#include "nwindow/NCWnd.h"
#include "nwindow/NConsoleWnd.h"

class NConsoleWnd;
class NCMainWnd;
class NCChatWnd;
class NCNPCHtmlViewer;
class NCPartyWnd;
class NCPetStatusWnd;
class NCSummonedStatusWnd;
class NCTargetStatusWnd;
class NCPlayerStatusWnd;
class NCAbnormalStatusWnd;
class NCMenuWnd;
class NCSystemMenuWnd;
class NCShortCutWnd;
class NCTrainWnd;
class NCMatchWnd;
class NCMatchOptionWnd;
class NCMinimapWnd;
class NCTutorialViewerWnd;
class NCTutorialQuestionWnd;
class NCPetWnd;
class NCSummonedWnd;
class NCRadarWnd;
class NCGMMagicSkillWnd;
class NCGMDetailStatusWnd;
class NCDialogBox;
class NCHtmlViewer;
class NCGMPetitionWnd;
class NCInventoryWnd;
class NCTradeWnd;
class NCGMWnd;
class NCGMInventoryWnd;
class NCGMQuestWnd;
class NCGMActionWnd;
class NCVIPShopWnd;
class NCStoreWnd;
class NCShopWnd;
class NCGMStoreWnd;
class NCDeliverWnd;
class NCSelectDeliverWnd;
class NCPrivateShopWnd;
class NCPrivateBuyWnd;

// NWM_COMMAND id the desktop answers: quit the game (name ours)
enum { NID_CONSOLE_QUIT = 0x1002 };

// the desktop's own messages (names ours): wParam an FVector, the player moves there
enum { NWM_MOVETO = (INT)0xF0000001 };

// the balloon timer (SetTimer id, once a second)
enum { NTIMER_BALLOONS = 0xFA0D05 };

// A chat balloon (name ours; 12 bytes, NCConsole::Balloons).
struct NChatBalloon
{
    INT ObjectID;              // +0x0 who said it
    TCHAR* Text;               // +0x4 new TCHAR[23]: at most 19 characters, then "..."; the
                               //   pawn's balloon (APawn +0x142C) points at it
    INT Tick;                  // +0x8 NCConsole::Ticks when said

    NChatBalloon() : ObjectID(0), Text(NULL), Tick(0) {}
    ~NChatBalloon() { Text = NULL; }   // the text is freed by hand
};
NW_ASSERT_SIZE(NChatBalloon, 0xC);

class NCConsole : public NCWnd
{
    NW_DECLARE_CLASS(NCConsole, NCWnd, 0, 0x1012d298)

    NCMainWnd* MainWnd;                    // +0x94 character window
    NCChatWnd* ChatWnd;                    // +0x98
    NCNPCHtmlViewer* NpcHtmlWnd;           // +0x9C NPC dialogs
    NCNPCHtmlViewer* HelpWnd;              // +0xA0 "..\L2text\help.htm"
    NConsoleWnd* Owner;                    // +0xA4 the console (the second constructor)
    INT UnknownA8;                         // +0xA8 (1)
    NCGMPetitionWnd* GMPetitionWnd;        // +0xAC
    NCPartyWnd* PartyWnd;                  // +0xB0
    NCPetStatusWnd* PetStatusWnd;          // +0xB4
    NCSummonedStatusWnd* SummonedStatusWnd;// +0xB8
    NCInventoryWnd* InventoryWnd;          // +0xBC
    NCTargetStatusWnd* TargetStatusWnd;    // +0xC0
    NCPlayerStatusWnd* PlayerStatusWnd;    // +0xC4
    NCAbnormalStatusWnd* AbnormalStatusWnd;// +0xC8 buffs
    NCStoreWnd* StoreWnd;                  // +0xCC warehouse
    NCShopWnd* ShopWnd;                    // +0xD0 NPC shop
    NCVIPShopWnd* VIPShopWnd;              // +0xD4
    NCMenuWnd* MenuWnd;                    // +0xD8 the menu bar
    NCSystemMenuWnd* SystemMenuWnd;        // +0xDC
    NCTradeWnd* TradeWnd;                  // +0xE0
    NCShortCutWnd* ShortCutWnd;            // +0xE4
    NCTrainWnd* TrainWnd;                  // +0xE8 skill trainer
    NCMatchWnd* MatchWnd;                  // +0xEC party matching
    NCMatchOptionWnd* MatchOptionWnd;      // +0xF0
    NCMinimapWnd* MinimapWnd;              // +0xF4
    NCWnd* BoardWnd;                       // +0xF8 NCBoardWnd (community)
    NCTutorialViewerWnd* TutorialWnd;      // +0xFC
    NCTutorialQuestionWnd* TutorialQuestionWnd; // +0x100 the question mark
    NCPetWnd* PetWnd;                      // +0x104
    NCSummonedWnd* SummonedWnd;            // +0x108
    NCRadarWnd* RadarWnd;                  // +0x10C
    NCWnd* SiegeInfoWnd;                   // +0x110 NCSiegeInfoMainWnd
    NCGMWnd* GMWnd;                        // +0x114
    NCGMInventoryWnd* GMInventoryWnd;      // +0x118
    NCGMMagicSkillWnd* GMMagicSkillWnd;    // +0x11C
    NCGMDetailStatusWnd* GMDetailStatusWnd;// +0x120
    NCGMQuestWnd* GMQuestWnd;              // +0x124
    NCGMStoreWnd* GMStoreWnd;              // +0x128
    NCGMActionWnd* GMActionWnd;            // +0x12C
    NCPrivateShopWnd* PrivateShopWnd;      // +0x130 selling
    NCPrivateBuyWnd* PrivateBuyWnd;        // +0x134 buying
    NCDeliverWnd* DeliverWnd;              // +0x138 freight
    NCSelectDeliverWnd* SelectDeliverWnd;  // +0x13C
    NCNPCHtmlViewer* TestNpcHtmlWnd;       // +0x140 "test.html" (GIsOnlyL2Console only)
    NCWnd* TestHtmlWnd;                    // +0x144 NCTestHtmlViewer (GIsTestHtml only)
    NCDialogBox* MainDialogBox;            // +0x148 NDialogInfo::bMainBox
    NCDialogBox* SecondDialogBox;          // +0x14C
    INT Unknown150;                        // +0x150 (1)
    UTexture* BlackTex;                    // +0x154 "NWindow.BlackTexture"
    UTexture* BalloonTex;                  // +0x158 "NWindow.chat"
    TArray<NChatBalloon> Balloons;         // +0x15C
    INT BalloonTimer;                      // +0x168 SetTimer result
    INT Ticks;                             // +0x16C seconds (the balloon timer)
    INT Unknown170[4];                     // +0x170

    NCConsole();                                                     // 0x10042530
    NCConsole(NConsoleWnd* InOwner);                                 // 0x100426c0 (Initialize)
    // ~NCConsole                                                    // 0x10042660

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x1003e950
    virtual INT OnTimer(INT TimerID);                                // 34 0x10042400
    virtual INT OnCreate();                                          // 35 0x1003e9e0
    virtual INT OnDestroy();                                         // 36 0x10041bc0
    virtual void ShowWindow();                                       // 42 0x1003d810
    virtual void HideWindow();                                       // 43 0x1003dab0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1003d740
    // name ours: an HTML page's "Link <bypass>" or "Write <board> ..." command
    virtual INT OnHtmlCommand(const TCHAR* Command, NCHtmlViewer* Source); // 62 0x10040b50

    // names ours
    INT OnMoveTo(WPARAM wParam, LPARAM lParam);                      // 0x1003d7e0 NWM_MOVETO
    void ToggleInterface();                                          // 0x1003dac0 (a shortcut key)
    void CloseWindows();                                             // 0x1003dbc0 (a shortcut key)
    void Reset();                                                    // 0x1003de10 (the console)
    void TogglePetition();                                           // 0x1003de90 (the chat line)
    void ToggleSystemMenu();                                         // 0x1003df20
    void ToggleInventory();                                          // 0x1003df80
    void ToggleChat();                                               // 0x1003e050
    void ToggleHelp();                                               // 0x1003e080
    void TogglePartyMatch();                                         // 0x1003e0d0
    void ToggleMinimap();                                            // 0x1003e2b0
    void ToggleMatchOption();                                        // 0x1003e320
    void ToggleBoard();                                              // 0x1003e350
    void ToggleGMWnd();                                              // 0x1003e410
    void ToggleGMInventory(TCHAR* Name);                             // 0x1003e450 (the GM window)
    void ToggleGMMagicSkill(TCHAR* Name);                            // 0x1003e4b0
    void ToggleGMDetailStatus(TCHAR* Name);                          // 0x1003e510
    void ToggleGMQuest(TCHAR* Name);                                 // 0x1003e570
    void ToggleGMStore(TCHAR* Name);                                 // 0x1003e5d0
    void ToggleGMAction(TCHAR* Name);                                // 0x1003e630
    void AddSystemMessage(INT MsgID, DWORD Color);                   // 0x1003e690 into the chat
    void AddSystemText(const TCHAR* Text, INT SubType, DWORD Color); // 0x1003e6f0 into the chat
    void ToggleCharacter(INT Tab);                                   // 0x10041800 -1: show / hide
    void Say(INT ObjectID, const TCHAR* Name, const TCHAR* Text, DWORD Color,
             INT Type);                                              // 0x10041880 the console
};
NW_ASSERT_OFFSET(NCConsole, MainWnd, 0x94);
NW_ASSERT_OFFSET(NCConsole, Owner, 0xA4);
NW_ASSERT_OFFSET(NCConsole, InventoryWnd, 0xBC);
NW_ASSERT_OFFSET(NCConsole, ShortCutWnd, 0xE4);
NW_ASSERT_OFFSET(NCConsole, RadarWnd, 0x10C);
NW_ASSERT_OFFSET(NCConsole, MainDialogBox, 0x148);
NW_ASSERT_OFFSET(NCConsole, Balloons, 0x15C);
NW_ASSERT_OFFSET(NCConsole, Ticks, 0x16C);
NW_ASSERT_SIZE(NCConsole, 0x180);

// the desktop (NConsoleWnd::Desktop is declared as an NCWnd)
#define GDesktop (static_cast<NCConsole*>(GConsole->Desktop))
inline NCConsole* NConsoleWnd::GetDesktop() const
{
    return static_cast<NCConsole*>(Desktop);
}

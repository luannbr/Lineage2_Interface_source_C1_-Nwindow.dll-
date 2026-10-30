#pragma once
// NCChatWnd - the chat window (UClass "CChatWnd", 0x57C): five message lists, one per tab
// (all, market, party, pledge, ally), the input line, the tab row, the filter panel
// (NCChatFilterWnd) and a resize grip on top. Incoming messages go through AddChatMessage,
// which hands each line to every tab whose filter shows its type; the input line reports to
// OnEditCommand (Enter runs the line: chat or a /command; '"' and Up/Down recall the names
// of recent tells). Optionally the chat is logged to a file ("text capture", GL2TextCapture).
//
// Desktop (NCConsole) +0x98 is the chat window.

#include "nwindow/NCWnd.h"
#include "nwindow/NCChatListBox.h"
#include "nwindow/NCommandParser.h"

class NCEditBox;
class NCTabControl;
class NCButton;
class NCChatFilterWnd;
class NCResizeFrame;

// Lists[] (names ours). The tabs are in another order: see TabList.
enum
{
    CHATLIST_All,
    CHATLIST_Party,
    CHATLIST_Pledge,
    CHATLIST_Market,
    CHATLIST_Ally,
    CHATLIST_MAX
};

// CurTab: the tab row, left to right (names ours)
enum
{
    CHATTAB_All,
    CHATTAB_Market,
    CHATTAB_Party,
    CHATTAB_Pledge,
    CHATTAB_Ally,
};

// NWM_COMMAND ids of the window's controls
enum
{
    NID_CHAT_EDIT = 0x1501,
    NID_CHAT_LIST = 0x1A01,
    NID_CHAT_TABS = (INT)0xF0F01409,
    NID_CHAT_OPTION = (INT)0xF0F01410,
    NID_CHAT_RESIZE = 0xF007,
};

// Prefix of each Say2 type in the input line, ASCII and full width (name ours): 1 '!',
// 2 '"', 3 '#', 4 '@', 8 '+', 9 '$'...
#if NW_HOOK_MODE
#define GChatPrefix (reinterpret_cast<const TCHAR* const (*)[2]>(::nw::Addr(0x1010cee0)))
#else
extern const TCHAR* const GChatPrefix[][2];
#endif

class NCChatWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCChatWnd, NCWnd, 0, 0x1012c850)

    UTexture* HeadTex;               // +0x94 "L2UI.ChatWnd.Head"
    UTexture* BodyTex;               // +0x98 "L2UI.ChatWnd.Body", repeated down the window
    UTexture* Unknown9C;             // +0x9C cleared, never loaded
    UTexture* BottomTex;             // +0xA0 "L2UI.ChatWnd.Bottom" (not drawn)
    UTexture* Bottom1Tex;            // +0xA4 "L2UI.ChatWnd.Bottom1", under the input line
    UTexture* Bottom2Tex;            // +0xA8 "L2UI.ChatWnd.Bottom2" (not drawn)
    UTexture* IMEJpTex;              // +0xAC IME state icons: "L2UI.ChatWnd.IME_jp"
    UTexture* IMEEnTex;              // +0xB0 "L2UI.ChatWnd.IME_en"
    UTexture* IMEKrTex;              // +0xB4 "L2UI.ChatWnd.IME_kr"
    UTexture* IMETw1Tex;             // +0xB8 "L2UI.ChatWnd.IME_tw1"
    UTexture* IMETw2Tex;             // +0xBC "L2UI.ChatWnd.IME_tw2"
    UTexture* IMETw3Tex;             // +0xC0 "L2UI.ChatWnd.IME_tw3"
    NCEditBox* EditBox;              // +0xC4 the input line
    TCHAR LastTellName[32];          // +0xC8 "name " (SetLastTellName), never read here
    TArray<NString*> TellNames;      // +0x108 recent tell partners, oldest first (64 at most)
    INT TellIndex;                   // +0x114 the one Up/Down shows (-1 before any)
    UBOOL bTellBrowse;               // +0x118 '"' was typed: Up/Down walk TellNames
    NCChatListBox* Lists[CHATLIST_MAX]; // +0x11C CHATLIST_*
    NCTabControl* TabCtrl;           // +0x130
    NCButton* OptionButton;          // +0x134 opens / closes FilterWnd
    NCWnd* Unknown138;               // +0x138 destroyed by OnDestroy, never created
    NCChatFilterWnd* FilterWnd;      // +0x13C
    UBOOL bFilterHidden;             // +0x140
    NCResizeFrame* ResizeFrame;      // +0x144 grip on the top edge
    INT ResizeDelta;                 // +0x148 drag not applied yet (OnResize, 12 px steps)
    INT Unknown14C;                  // +0x14C (0)
    INT Unknown150;                  // +0x150 (2)
    INT CreateHeight;                // +0x154 Height when created
    INT Unknown158;                  // +0x158 (0)
    INT CurTab;                      // +0x15C CHATTAB_*
    FArchive* CaptureFile;           // +0x160 text capture log
    NCommandParser Parser;           // +0x164

    NCChatWnd();                                                     // 0x1003cc20
    // ~NCChatWnd                                                    // 0x1003d6d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100376f0
    virtual void SetFocus();                                         // 28 0x10038380
    virtual INT OnCreate();                                          // 35 0x1003cd00
    virtual INT OnDestroy();                                         // 36 0x100387f0
    virtual void ShowWindow();                                       // 42 0x100383a0
    virtual void HideWindow();                                       // 43 0x1003dab0 (shared)
    virtual void MoveWindow(INT DX, INT DY);                         // 44 0x10039080 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10038bf0
    // names ours
    virtual void StartTextCapture();                                 // 62 0x100377f0
    virtual void StopTextCapture();                                  // 63 0x10037970
    virtual void WriteCapture(const TCHAR* Text);                    // 64 0x10037a40
    virtual void CaptureMessage(const TCHAR* Text, INT Type, INT SubType); // 65 0x10037aa0
    virtual void UpdateTextCapture();                                // 66 0x100377c0
    virtual INT OnTabSelected(INT Tab);                              // 67 0x1003aed0
    virtual INT OnEditCommand(INT Code);                             // 68 0x1003b6a0
    virtual INT OnOptionButton();                                    // 69 0x1003c9c0
    virtual INT OnResize(INT Delta);                                 // 70 0x10038e20

    // names ours
    void AddChatMessage(const TCHAR* Text, DWORD Color, INT Type, INT SubType); // 0x10037b30
    void SetLastTellName(const TCHAR* Name);                         // 0x10038320
    void BeginTell(const TCHAR* Name);                               // 0x10038a30
    void BrowseTellNames(UBOOL bUp);                                 // 0x10038ad0
    void AddTellName(const TCHAR* Name);                             // 0x1003ace0
    // the party, pledge and alliance tabs enabled or greyed (NConsoleWnd::EnablePartyChat...,
    // as the server says); the party's does nothing
    void EnablePartyTab(bool bEnable) {}                             // 0x10001700 (folded)
    void EnablePledgeTab(bool bEnable);                              // 0x1003d5a0
    void EnableAllyTab(bool bEnable);                                // 0x1003d610
    // used by the console (names ours)
    INT ResizeBy(INT Delta);                                         // 0x100383f0 OnResize out of
                                                                     //   line (WindowsInfo.ini)
    void CycleTab(UBOOL bNext);                                      // 0x10038400 Alt + PageUp /
                                                                     //   PageDown
    void ClearAll();                                                 // 0x1003ca80 (name ours)

    // list of a tab (inlined in the stock code, name ours)
    NCChatListBox* TabList(INT Tab)
    {
        switch (Tab)
        {
        case CHATTAB_All:    return Lists[CHATLIST_All];
        case CHATTAB_Market: return Lists[CHATLIST_Market];
        case CHATTAB_Party:  return Lists[CHATLIST_Party];
        case CHATTAB_Pledge: return Lists[CHATLIST_Pledge];
        case CHATTAB_Ally:   return Lists[CHATLIST_Ally];
        }
        return NULL;
    }
};
NW_ASSERT_OFFSET(NCChatWnd, EditBox, 0xC4);
NW_ASSERT_OFFSET(NCChatWnd, LastTellName, 0xC8);
NW_ASSERT_OFFSET(NCChatWnd, TellNames, 0x108);
NW_ASSERT_OFFSET(NCChatWnd, Lists, 0x11C);
NW_ASSERT_OFFSET(NCChatWnd, FilterWnd, 0x13C);
NW_ASSERT_OFFSET(NCChatWnd, ResizeDelta, 0x148);
NW_ASSERT_OFFSET(NCChatWnd, CurTab, 0x15C);
NW_ASSERT_OFFSET(NCChatWnd, Parser, 0x164);
NW_ASSERT_SIZE(NCChatWnd, 0x57C);

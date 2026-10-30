#pragma once
// NCTutorialViewerWnd - the tutorial window (UClass "CTutorialViewerWnd", 0xA0): the NPC dialog
// layout around an NCTutorialHtmlViewer, pages given as text.
// NCTutorialQuestionWnd - the blinking question mark that opens a tutorial page (UClass
// "CTutorialQuestionWnd", 0xA8): a click asks the server for the page of its question and
// hides it.

#include "nwindow/NCWnd.h"

class NCTutorialHtmlViewer;

class NCTutorialViewerWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTutorialViewerWnd, NCWnd, 0, 0x101527c8)

    NCTutorialHtmlViewer* HtmlViewer; // +0x94
    INT Unknown98;                   // +0x98 (0)
    UTexture* BackTex;               // +0x9C "L2UI.etcwnd.NPCHtml_BACK"

    NCTutorialViewerWnd();                                           // 0x100bced0
    // ~NCTutorialViewerWnd                                          // 0x100bcf20

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100bcd30 (shared)
    virtual INT OnCreate();                                          // 35 0x100bcf70
    virtual INT OnDestroy();                                         // 36 0x100bd0e0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100bd120
    virtual INT OnClose(WPARAM wParam, LPARAM lParam);               // 62 0x100bcda0 (shared)

    void ShowPage(TCHAR* Text);                                      // 0x100bcdb0 (name ours)
};
NW_ASSERT_OFFSET(NCTutorialViewerWnd, BackTex, 0x9C);
NW_ASSERT_SIZE(NCTutorialViewerWnd, 0xA0);

class NCTutorialQuestionWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCTutorialQuestionWnd, NCWnd, 0, 0x101522a8)

    UTexture* HelpTex;               // +0x94 "L2UI.etcWnd.TutorialHelp"
    UTexture* HelpClickTex;          // +0x98 "L2UI.etcWnd.TutorialHelp_click"
    INT QuestionID;                  // +0x9C (name ours) sent when clicked
    UBOOL bLit;                      // +0xA0 HelpTex, else HelpClickTex: the timer blinks it,
                                     //   a press holds it off
    INT TimerID;                     // +0xA4 (0xFA0D06, 500 ms)

    NCTutorialQuestionWnd();                                         // 0x100bd200
    // ~NCTutorialQuestionWnd                                        // 0x100bd260

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (shared)
    virtual INT OnTimer(INT InTimerID);                              // 34 0x100bcea0
    virtual INT OnCreate();                                          // 35 0x100bcdd0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100bd2b0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100bce20
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100bce50
};
NW_ASSERT_OFFSET(NCTutorialQuestionWnd, QuestionID, 0x9C);
NW_ASSERT_SIZE(NCTutorialQuestionWnd, 0xA8);

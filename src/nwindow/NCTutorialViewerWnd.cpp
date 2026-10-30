/*=============================================================================
    NCTutorialViewerWnd.cpp - the tutorial window and its question mark. Stock: 0x100bcdb0 ..
    0x100bd0dc, 0x100bd120 .. 0x100bd339 (WindowProc, OnClose and OnDestroy are bodies the
    linker folded: NCNPCHtmlViewer hooks the first two).
=============================================================================*/
#include "nwindow/NCTutorialViewerWnd.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// title bar id, blink timer (names ours)
enum
{
    NID_TUTORIAL_FRAME = (INT)0xFFF00001,
    TIMER_TutorialBlink = 0xFA0D06,
};

/*-----------------------------------------------------------------------------
    NCTutorialViewerWnd.
-----------------------------------------------------------------------------*/

// 0x100bced0
NCTutorialViewerWnd::NCTutorialViewerWnd()
{
    Unknown98 = 0;
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100bced0, NCTutorialViewerWnd)

// 0x100bcf20
NW_HOOK_DTOR(0x100bcf20, NCTutorialViewerWnd)

// stock body shared with NCNPCHtmlViewer's (0x100bcd30)
INT NCTutorialViewerWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == (UINT)NWM_HTML_CLOSE)
        return OnClose(wParam, lParam) != 0;
    return 0;
}

// 0x100bcf70 (vslot 35) - like the NPC dialog, with a 300 px title bar and a 247 px viewer
INT NCTutorialViewerWnd::OnCreate()
{
    NCWnd::OnCreate();
    INT W = LocalW;

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_TUTORIAL_FRAME;
    FrameCtrl->ID = NID_TUTORIAL_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 300, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI.etcwnd.NPCHtml_BACK"));

    HtmlViewer = new NCTutorialHtmlViewer;
    HtmlViewer->SetWindowRect(12, 28, W - 12, 247);
    CreateChild(HtmlViewer, NWS_VISIBLE);
    HtmlViewer->SetOwner(Entry);
    return 1;
}
NW_HOOK_METHOD(0x100bcf70, NCTutorialViewerWnd, OnCreate)

// stock body shared with NCGMMagicSkillWnd's (0x100bd0e0)
INT NCTutorialViewerWnd::OnDestroy()
{
    if (HtmlViewer)
    {
        RemoveChildWindow(HtmlViewer->Entry);
        delete HtmlViewer;
        HtmlViewer = NULL;
    }
    NCWnd::OnDestroy();
    return 1;
}

// 0x100bd120 (vslot 50) - the background at its size under the title bar; the guard names
// the NPC dialog, as in the stock code
INT NCTutorialViewerWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNPCHtmlViewer::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 300, 256, 0, 0, 300, 256, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100bd120, NCTutorialViewerWnd, OnPaint)

// stock body shared with NCNPCHtmlViewer's (0x100bcda0)
INT NCTutorialViewerWnd::OnClose(WPARAM wParam, LPARAM lParam)
{
    HideWindow();
    return 1;
}

// 0x100bcdb0 - an empty text leaves the page as it is
void NCTutorialViewerWnd::ShowPage(TCHAR* Text)
{
    if (Text && *Text)
        HtmlViewer->LoadHtml(Text);
}
NW_HOOK_METHOD(0x100bcdb0, NCTutorialViewerWnd, ShowPage)

NW_CHECK_VTABLE(NCTutorialViewerWnd, 0x100f0b40, 63)

/*-----------------------------------------------------------------------------
    NCTutorialQuestionWnd.
-----------------------------------------------------------------------------*/

// 0x100bd200 - QuestionID is set by whoever shows it
NCTutorialQuestionWnd::NCTutorialQuestionWnd()
{
    HelpTex = NULL;
    HelpClickTex = NULL;
    bLit = 0;
}
NW_HOOK_CTOR(0x100bd200, NCTutorialQuestionWnd)

// 0x100bd260
NW_HOOK_DTOR(0x100bd260, NCTutorialQuestionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCTutorialQuestionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100bcea0 (vslot 34) - blinks
INT NCTutorialQuestionWnd::OnTimer(INT InTimerID)
{
    if (InTimerID == TIMER_TutorialBlink)
        bLit = !bLit;
    return 1;
}
NW_HOOK_METHOD(0x100bcea0, NCTutorialQuestionWnd, OnTimer)

// 0x100bcdd0 (vslot 35)
INT NCTutorialQuestionWnd::OnCreate()
{
    NCWnd::OnCreate();
    HelpTex = FindTexture(TEXT("L2UI.etcWnd.TutorialHelp"));
    HelpClickTex = FindTexture(TEXT("L2UI.etcWnd.TutorialHelp_click"));
    TimerID = SetTimer(TIMER_TutorialBlink, 500, 0);
    return 1;
}
NW_HOOK_METHOD(0x100bcdd0, NCTutorialQuestionWnd, OnCreate)

// 0x100bd2b0 (vslot 50) - no guard; the clip goes on after the origin
INT NCTutorialQuestionWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, bLit ? HelpTex : HelpClickTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100bd2b0, NCTutorialQuestionWnd, OnPaint)

// 0x100bce20 (vslot 56)
INT NCTutorialQuestionWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
        bLit = 0;
    return 1;
}
NW_HOOK_METHOD(0x100bce20, NCTutorialQuestionWnd, OnLButtonDown)

// 0x100bce50 (vslot 57) - a release on it asks for the page and hides it
INT NCTutorialQuestionWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    if (IsInWindow(LOWORD(lParam), HIWORD(lParam)))
    {
        GConsole->RequestTutorialQuestionMark(QuestionID);
        HideWindow();
    }
    bLit = 1;
    return 1;
}
NW_HOOK_METHOD(0x100bce50, NCTutorialQuestionWnd, OnLButtonUp)

NW_CHECK_VTABLE(NCTutorialQuestionWnd, 0x100f0c70, 62)

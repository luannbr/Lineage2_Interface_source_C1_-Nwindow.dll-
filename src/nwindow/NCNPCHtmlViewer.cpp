/*=============================================================================
    NCNPCHtmlViewer.cpp - the NPC dialog window. Stock: 0x10026b30 .. 0x10026ba1,
    0x100276e0 .. 0x100277a8, 0x10029770 .. 0x100298cc; WindowProc and OnClose sit with
    NCTutorialViewerWnd (0x100bcd30, 0x100bcda0: one body for both classes), OnDestroy is a
    body shared with other windows, LoadFile sits with NCTestHtmlViewer's (0x1002c640).
=============================================================================*/
#include "nwindow/NCNPCHtmlViewer.h"
#include "nwindow/NCHtmlViewer.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"

// title bar id (name ours)
enum { NID_NPCHTML_FRAME = (INT)0xF0F01412 };

// 0x100276e0
NCNPCHtmlViewer::NCNPCHtmlViewer()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x100276e0, NCNPCHtmlViewer)

// 0x10026b30
NW_HOOK_DTOR(0x10026b30, NCNPCHtmlViewer)

// 0x100bcd30 (vslot 24) - also NCTutorialViewerWnd's
INT NCNPCHtmlViewer::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == (UINT)NWM_HTML_CLOSE)
        return OnClose(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x100bcd30, NCNPCHtmlViewer, WindowProc)

// 0x10029770 (vslot 35) - the viewer 12 px in, under the title bar, 242 px high
INT NCNPCHtmlViewer::OnCreate()
{
    NCWnd::OnCreate();
    INT W = LocalW;

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_NPCHTML_FRAME;
    FrameCtrl->ID = NID_NPCHTML_FRAME;
    FrameCtrl->SetWindowRect(0, 0, W, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    BackTex = FindTexture(TEXT("L2UI.etcwnd.NPCHtml_BACK"));

    HtmlViewer = new NCHtmlViewer;
    HtmlViewer->SetWindowRect(12, 28, W - 14, 242);
    CreateChild(HtmlViewer, NWS_VISIBLE);
    HtmlViewer->SetOwner(Entry);
    return 1;
}
NW_HOOK_METHOD(0x10029770, NCNPCHtmlViewer, OnCreate)

// stock body shared with NCGMMagicSkillWnd's (0x100bd0e0)
INT NCNPCHtmlViewer::OnDestroy()
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

// 0x10027700 (vslot 50) - the background under the title bar; the clip goes on after the
// origin
INT NCNPCHtmlViewer::OnPaint(NCGDevice* Device)
{
    guard(NCNPCHtmlViewer::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawStretchedTex(0, 21, Width, Height - 21, 0, 0, 300, 256, BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10027700, NCNPCHtmlViewer, OnPaint)

// 0x100bcda0 (vslot 62) - a page command closed the dialog; also NCTutorialViewerWnd's
INT NCNPCHtmlViewer::OnClose(WPARAM wParam, LPARAM lParam)
{
    HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100bcda0, NCNPCHtmlViewer, OnClose)

// 0x1002c640 - a page from a file (the desktop's help, the test page's button)
void NCNPCHtmlViewer::LoadFile(const TCHAR* FileName)
{
    if (FileName && *FileName)
        HtmlViewer->LoadHtmlFile((TCHAR*)FileName);
}
NW_HOOK_METHOD(0x1002c640, NCNPCHtmlViewer, LoadFile)

// the stock body is NCTutorialViewerWnd's (0x100bcdb0): an empty text leaves the page as it is
void NCNPCHtmlViewer::ShowPage(TCHAR* Text)
{
    if (Text && *Text)
        HtmlViewer->LoadHtml(Text);
}

NW_CHECK_VTABLE(NCNPCHtmlViewer, 0x100de360, 63)

#pragma once
// NCNPCHtmlViewer - the NPC dialog window (UClass "CNPCHtmlViewer", 0x9C): a title bar, the
// "NPCHtml_BACK" background stretched, and an NCHtmlViewer that closes the window when a page
// command asks for it (NWM_HTML_CLOSE).

#include "nwindow/NCWnd.h"

class NCHtmlViewer;

class NCNPCHtmlViewer : public NCWnd
{
    NW_DECLARE_CLASS(NCNPCHtmlViewer, NCWnd, 0, 0x10126050)

    NCHtmlViewer* HtmlViewer;        // +0x94
    UTexture* BackTex;               // +0x98 "L2UI.etcwnd.NPCHtml_BACK"

    NCNPCHtmlViewer();                                               // 0x100276e0
    // ~NCNPCHtmlViewer                                              // 0x10026b30

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100bcd30
    virtual INT OnCreate();                                          // 35 0x10029770
    virtual INT OnDestroy();                                         // 36 0x100bd0e0 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10027700
    virtual INT OnClose(WPARAM wParam, LPARAM lParam);               // 62 0x100bcda0 (name ours)

    void LoadFile(const TCHAR* FileName);                            // 0x1002c640 (name ours)
    void ShowPage(TCHAR* Text);                                      // 0x100bcdb0 (folded with
                                                                     //   NCTutorialViewerWnd's)
};
NW_ASSERT_OFFSET(NCNPCHtmlViewer, HtmlViewer, 0x94);
NW_ASSERT_SIZE(NCNPCHtmlViewer, 0x9C);

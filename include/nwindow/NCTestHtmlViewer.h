#pragma once
// NCTestHtmlViewer - the HTML test page (UClass "CTestHtmlViewer", 0xA4; the desktop's
// TestHtmlWnd, made when GIsTestHtml is set): the NPC dialog window with an edit line and a
// button under the page, which loads ..\test_html\<the name typed>. Its class registration
// names NCWnd as the super class, though the C++ base is NCNPCHtmlViewer.

#include "nwindow/NCNPCHtmlViewer.h"

class NCButton;
class NCEditBox;

class NCTestHtmlViewer : public NCNPCHtmlViewer
{
    NW_DECLARE_CLASS(NCTestHtmlViewer, NCNPCHtmlViewer, 0, 0x10125b28)

    NCEditBox* FileEdit;             // +0x9C created hidden
    NCButton* LoadButton;            // +0xA0 a white square

    NCTestHtmlViewer();                                              // 0x100277e0
    // ~NCTestHtmlViewer                                             // 0x10027830

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10026640
    virtual INT OnCreate();                                          // 35 0x100298d0
    virtual INT OnLoad();                                            // 63 0x1002c660 (name ours)
};
NW_ASSERT_OFFSET(NCTestHtmlViewer, FileEdit, 0x9C);
NW_ASSERT_SIZE(NCTestHtmlViewer, 0xA4);

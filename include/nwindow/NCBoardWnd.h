#pragma once
// NCBoardWnd - the community board (Alt+B) (UClass "CBoardWnd", 0x6B8): a title bar, a row of
// six buttons (sysstrings 377..382) that each send the bypass command the console gave them,
// and the board's page in an NCBBSHtmlViewer.
//
// NCBBSHtmlViewer - the board's page (UClass "CBBSHtmlViewer", 0x2412C): an NCHtmlViewer drawn
// on "Sek.cbui353" and clipped inside that frame. A command the console answers with 1 locks it
// until the console clears the flag; the click sound plays even for an empty link. Its check of
// a form value lets anything of 2..3000 bytes through, bad words replaced. The console hands
// the page over in pieces (AddPiece, "keyvalue"): a page, or a post (template, fields,
// comments, thread), a write form (template, fields) or a page in three parts, assembled when
// all their pieces are in.

#include "nwindow/NCHtmlViewer.h"

class NCButton;

class NCBBSHtmlViewer : public NCHtmlViewer
{
    NW_DECLARE_CLASS(NCBBSHtmlViewer, NCHtmlViewer, 0, 0x10126ab0)

    INT Unknown110;                  // +0x110 (0)
    UTexture* BoardBackTex;          // +0x114 "Sek.cbui353", drawn without <html imgsrc>
    INT PageKind;                    // +0x118 last page assembled (names ours): 0 a whole page,
                                     //   1 a page in parts, 2 a post, 3 a write form
    UBOOL bLocked;                   // +0x11C a command was accepted (the console answered 1)
    INT NumPostPieces;               // +0x120 (0) pieces "1".."4" received
    INT NumFormPieces;               // +0x124 (0) pieces "1001", "1002" received
    INT NumPageParts;                // +0x128 (0) pieces "101".."103" received
    TCHAR PostTemplate[0x2000];      // +0x12C "1": the post page, with <!--FIELD--> marks
    TCHAR PostFields[0x2000];        // +0x412C "2": its fields, separated by ''
    TCHAR PostComments[0x2000];      // +0x812C "3": for <!--COMMENT-->
    TCHAR PostThread[0x2000];        // +0xC12C "4": for <!--THREAD-->
    TCHAR FormTemplate[0x2000];      // +0x1012C "1001": the write form
    TCHAR FormFields[0x2000];        // +0x1412C "1002": its fields
    TCHAR PageParts[3][0x2000];      // +0x1812C "101", "102", "103"

    NCBBSHtmlViewer();                                               // 0x100296b0
    // ~NCBBSHtmlViewer                                              // 0x10029720

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10026420
    virtual INT OnCreate();                                          // 35 0x10026490
    virtual INT OnDestroy();                                         // 36 0x1002a890
    virtual void ShowWindow();                                       // 42 0x10027560
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10027460
    virtual TCHAR* GetControlValue(const TCHAR* Name);               // 65 0x10027570
    virtual INT OnActionLink(TCHAR* Action);                         // 67 0x10026510
    virtual INT OnButton(TCHAR* Action);                             // 68 0x100265b0
    // name ours: NWM_COMMAND 0xF0F01411 hides the viewer (nothing in NWindow sends it)
    virtual INT OnHideCommand();                                     // 72 0x10059d00 (shared)

    // names ours
    INT ShowPage(TCHAR* Text);                                       // 0x1002be70
    void BuildPostPage();                                            // 0x1002be80
    void BuildFormPage();                                            // 0x1002c110
    void ShowPageParts();                                            // 0x1002c3e0
    void AddPiece(TCHAR* Text);                                      // 0x1002c470
};
NW_ASSERT_OFFSET(NCBBSHtmlViewer, bLocked, 0x11C);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, PostTemplate, 0x12C);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, FormTemplate, 0x1012C);
NW_ASSERT_OFFSET(NCBBSHtmlViewer, PageParts, 0x1812C);
NW_ASSERT_SIZE(NCBBSHtmlViewer, 0x2412C);

class NCBoardWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCBoardWnd, NCWnd, 0, 0x10125608)

    NCBBSHtmlViewer* HtmlViewer;     // +0x94
    UTexture* BackTex;               // +0x98 "sek.cbui354", the strip under the title bar
    NCButton* Buttons[6];            // +0x9C NWM_COMMAND 0xF0F01422 .. 0xF0F01427
    TCHAR Commands[6][128];          // +0xB4 bypass command of each button (from the console)
    UBOOL bLocked;                   // +0x6B4 a button's command was accepted (answered 1)

    NCBoardWnd();                                                    // 0x10027880
    // ~NCBoardWnd                                                   // 0x100278d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100266b0
    virtual INT OnCreate();                                          // 35 0x10029a40
    virtual INT OnDestroy();                                         // 36 0x10027920
    virtual void ShowWindow();                                       // 42 0x10078950 (shared)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10027ad0
    // names ours
    virtual INT OnButton1();                                         // 62 0x100267c0
    virtual INT OnButton2();                                         // 63 0x10026810
    virtual INT OnButton3();                                         // 64 0x10026860
    virtual INT OnButton4();                                         // 65 0x100268b0
    virtual INT OnButton5();                                         // 66 0x10026900
    virtual INT OnButton6();                                         // 67 0x10026950

    // names ours (called by the console's ShowBoard)
    void SetCommands(TCHAR* Command1, TCHAR* Command2, TCHAR* Command3, TCHAR* Command4,
                     TCHAR* Command5, TCHAR* Command6);              // 0x100269a0
    void LoadPage(TCHAR* Html);                                      // 0x1002c6c0 into HtmlViewer
};
NW_ASSERT_OFFSET(NCBoardWnd, Commands, 0xB4);
NW_ASSERT_OFFSET(NCBoardWnd, bLocked, 0x6B4);
NW_ASSERT_SIZE(NCBoardWnd, 0x6B8);

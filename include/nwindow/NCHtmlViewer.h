#pragma once
// NCHtmlViewer - shows an HTML page (UClass "CHtmlViewer", 0x110): the NPC dialogs, the
// tutorial, the board. The page is parsed into a tree (NHtmlLayout.h) that is laid out at the
// viewer's width and drawn scrolled; links and form fields become child controls (NCFileLinker,
// NCActionLinker, NCHtmlButton, NCHtmlEdit, NCHtmlMultiEdit, NCHtmlComboBox) that move with the
// page and report back through NWM_COMMAND with the viewer's ids. A command goes to the
// console (bypass to the server, $name replaced by the value of the named control); when it
// asks for it (-h) the owner window, or else the viewer, closes.
//
// NCTutorialHtmlViewer - the same for the tutorial window (UClass "CTutorialHtmlViewer").

#include "nwindow/NCScrollWnd.h"
#include "nwindow/NClasses.gen.h"

class NControl;
class NHtmlBlock;
struct NHtmlTagList;

// sent to the owner window when a command asks to close (name ours)
enum { NWM_HTML_CLOSE = (INT)0xF0000004 };

class NCHtmlViewer : public NCScrollWnd
{
    NW_DECLARE_CLASS(NCHtmlViewer, NCScrollWnd, 0, 0x10126578)

    NHtmlBlock* Doc;                 // +0xCC the page, NULL while none is loaded
    UTexture* BackTex;               // +0xD0 <html imgsrc>, NULL = transparent
    INT FileLinkID;                  // +0xD4 NWM_COMMAND ids of the controls it creates
    INT ActionLinkID;                // +0xD8   (0xFF00FF00 .. 0xFF00FF05)
    INT ButtonID;                    // +0xDC
    INT EditID;                      // +0xE0
    INT MultiEditID;                 // +0xE4
    INT ComboBoxID;                  // +0xE8
    TArray<NControl*> Controls;      // +0xEC the page's controls, in page order
    UBOOL bLoaded;                   // +0xF8 Doc holds a page
    INT UnknownFC;                   // +0xFC (0)
    INT Unknown100;                  // +0x100 (0)
    INT CreateWidth;                 // +0x104 size at OnCreate
    INT CreateHeight;                // +0x108
    NWndEntry* Owner;                // +0x10C window a closing command goes to, NULL = itself

    NCHtmlViewer();                                                  // 0x100292e0
    // ~NCHtmlViewer                                                 // 0x10028a00

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10025f50
    virtual INT OnCreate();                                          // 35 0x10026070
    virtual INT OnDestroy();                                         // 36 0x1002a7e0
    virtual void ShowWindow();                                       // 42 0x100273d0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10028040
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // 61 0x100271f0
    virtual INT ScrollUp();                                          // 62 0x10027180
    virtual INT ScrollDown();                                        // 63 0x100271a0
    virtual INT OnScrollBar(INT Value);                              // 64 0x100271c0
    // names ours
    virtual TCHAR* GetControlValue(const TCHAR* Name);               // 65 0x10027220 checked
    virtual INT OnFileLink(TCHAR* File);                             // 66 0x1002be30
    virtual INT OnActionLink(TCHAR* Action);                         // 67 0x10026110
    virtual INT OnButton(TCHAR* Action);                             // 68 0x10026190
    virtual INT OnEdit(LPARAM Param);                                // 69 0x10026200
    virtual INT OnMultiEdit(LPARAM Param);                           // 70 0x100262c0
    virtual INT OnComboBox(LPARAM Param);                            // 71 0x10026380

    // names ours
    void SetBackTexture(const TCHAR* Name);                          // 0x100260a0
    void SetOwner(NWndEntry* InOwner);                               // 0x100260e0
    UTexture* FindPageTexture(const TCHAR* Name);                    // 0x100260f0
    void ScrollControls(INT DX, INT DY);                             // 0x10027100
    INT AddFileLink(INT X, INT Y, INT W, INT H, TCHAR* File);        // 0x10028170
    INT AddActionLink(INT X, INT Y, INT W, INT H, TCHAR* Action, TCHAR* Msg); // 0x10028260
    INT AddButton(INT X, INT Y, INT W, INT H, TCHAR* NormalTexName, TCHAR* PushedTexName,
                  TCHAR* Action, TCHAR* Label);                      // 0x10028380
    INT AddMultiEdit(INT X, INT Y, INT W, INT H, TCHAR* Var);        // 0x100284e0
    INT AddEdit(INT X, INT Y, INT W, INT H, TCHAR* Var, INT Type, TCHAR* Action); // 0x100285d0
    INT AddComboBox(INT X, INT Y, INT W, INT H, TCHAR* Var, TCHAR* List, TCHAR* Sel); // 0x10029400
    void Clear();                                                    // 0x10029510
    void BuildDocument(NHtmlTagList* Tags);                          // 0x1002bac0
    INT LoadHtmlFile(TCHAR* Source);                                 // 0x1002bc70
    INT LoadHtml(TCHAR* Text);                                       // 0x1002bd60
};
NW_ASSERT_OFFSET(NCHtmlViewer, Doc, 0xCC);
NW_ASSERT_OFFSET(NCHtmlViewer, Controls, 0xEC);
NW_ASSERT_OFFSET(NCHtmlViewer, Owner, 0x10C);
NW_ASSERT_SIZE(NCHtmlViewer, 0x110);

class NCTutorialHtmlViewer : public NCHtmlViewer
{
    NW_DECLARE_CLASS(NCTutorialHtmlViewer, NCHtmlViewer, 0, 0x10126fd0)

    NCTutorialHtmlViewer();                                          // 0x10029610
    // ~NCTutorialHtmlViewer                                         // 0x10029660

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100263d0
    virtual INT OnCreate();                                          // 35 0x10026410
    virtual INT OnDestroy();                                         // 36 0x1002a890 (shared)
};
NW_ASSERT_SIZE(NCTutorialHtmlViewer, 0x110);

// The windows the controls of a page clip to (the loop is inlined in each control's OnPaint in
// the stock code; names ours): the enclosing HTML viewer, or the first ancestor that is not a
// control. NULL at the top of the tree (no clip is set then).
inline NCWnd* EnclosingHtmlViewer(NCWnd* Wnd)
{
    NWndEntry* E = Wnd->Entry;
    for (;;)
    {
        NCWnd* W = E->Parent->Wnd;
        if (!W || W->IsA(NCHtmlViewer::StaticClass()))
            return W;
        E = W->Entry;
    }
}
inline NCWnd* FirstNonControlParent(NCWnd* Wnd)
{
    NWndEntry* E = Wnd->Entry;
    for (;;)
    {
        NCWnd* W = E->Parent->Wnd;
        if (!W || !W->IsA(NClass::NControl()))
            return W;
        E = W->Entry;
    }
}

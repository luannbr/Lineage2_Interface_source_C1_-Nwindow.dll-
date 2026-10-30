/*=============================================================================
    CraftWnd.h - phase 6: the base of the native craft windows (NWindow_C1
    src/inject/craft/NwUi.h: the Window and ScrollList classes, and the common
    look of CraftWindows.cpp's RecipeWnd).

    NWindow_C1 built these windows from the outside: a stock NCWnd / NCScrollWnd
    object with a copy of the stock vtable carrying its overrides, the window's
    state kept in a separate C++ object. Here they are classes of their own on top
    of ours (UClasses registered only by NWindowL2: NwL2Class.h), and the same
    behaviour comes from plain virtual overrides:
      * a window is a child of the desktop (NCConsole) with style NWS_VISIBLE,
        made the first time it opens and afterwards only shown and hidden, like
        every stock window. Its title bar is a stock NCFrameCtrl (title, close
        button, drag); the controls are stock NCButtons and NCRecipeListWnds;
      * the close button hides it through HideWindow while the desktop is visible:
        that is a close by the user (OnUserClose). Alt+H and leaving the world hide
        the desktop first: those keep the window open, and the desktop's
        ShowWindow brings it back;
      * the prompts are the desktop's stock NCDialogBoxes (NConsoleWnd::ShowDialog),
        the answer routed back to the window (NWM_DIALOG_RESULT and 0xF00000xx);
      * where a window was closed is kept in LineageWindow.ini [CraftUi].
    NCRecipeWnd holds all that and the common look of the six windows
    (CraftWindows.h); NCRecipeListWnd is the list or icon grid its owner paints.
=============================================================================*/
#pragma once

#include "features/NwL2Class.h"

#if NW_FEATURES

#include "nwindow/NCScrollWnd.h"

class NCButton;
class NCGDevice;
class NCRecipeWnd;

// what the craft windows share (C2: every recipe window is 240 x 365, "Item Creation")
enum
{
    RECIPEWND_Width = 240,
    RECIPEWND_Height = 365,
    RECIPEWND_TitleH = 21,        // the stock title bar
    RECIPEWND_ScrollBarW = 12,    // NCScrollWnd's bar on the right edge
};

/*-----------------------------------------------------------------------------
    NCRecipeListWnd - rows, or a grid of icons, painted by the owner window
    (NWindow_C1 NwUi ScrollList): the stock NCScrollWnd with its scroll buttons
    and bar. A click selects the item under the cursor and tells the owner (a
    double click too), the tool tip pass asks the owner for the item's tool tip,
    the wheel scrolls one row a notch.
-----------------------------------------------------------------------------*/
class NCRecipeListWnd : public NCScrollWnd
{
    NW_DECLARE_L2_CLASS(NCRecipeListWnd, NCScrollWnd, 0)

    NCRecipeWnd* Owner;     // paints the items, gets the clicks, draws the tool tips
    INT ListID;             // the owner's name for the list
    INT RowH;               // px from a row to the next
    INT Cols;               // 1: rows; more: an icon grid
    INT CellW;              // px from a column to the next
    INT PadTop;             // px above the first row
    INT Count;              // items
    INT Selected;           // the item clicked last, -1 none

    NCRecipeListWnd();
    NCRecipeListWnd(NCRecipeWnd* InOwner, INT InID, INT InRowH, INT InCols, INT InCellW,
                    INT InPadTop);

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // the double click
    virtual INT OnPaint(NCGDevice* Device);
    virtual INT OnDrawToolTip(NCGDevice* Device);
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);

    INT DrawWidth();                    // the width without the scroll bar
    void SetItems(INT N);               // content height, scroll clamped, bar; a selection
                                        //   past the end cleared
    void Select(INT Index) { Selected = Index; }
    void ScrollBy(INT Pixels);          // positive: towards the end
    INT ItemAt(INT X, INT Y);           // the item at a screen point, -1 none

private:
    void Click(LPARAM Pos, UBOOL bDouble);
};

// the stock prompt a window waits for (NCRecipeWnd::DialogKind; names ours)
enum
{
    CRAFTDLG_None = 0,
    CRAFTDLG_Number = 1,     // the keypad prompt: OnDialogNumber
    CRAFTDLG_Text = 2,       // the text prompt: OnDialogText
    CRAFTDLG_Confirm = 3,    // yes / no: OnDialogConfirm
};

/*-----------------------------------------------------------------------------
    NCRecipeWnd - a craft window (NWindow_C1 NwUi Window + CraftWindows RecipeWnd).
-----------------------------------------------------------------------------*/
class NCRecipeWnd : public NCWnd
{
    NW_DECLARE_L2_ABSTRACT_CLASS(NCRecipeWnd, NCWnd, 0)

    TCHAR Title[64];            // the title bar's
    TCHAR PosKey[32];           // LineageWindow.ini [CraftUi] <PosKey>X / <PosKey>Y: where
                                //   the last window with this key was closed (windows that
                                //   take each other's place share one)
    const TCHAR* BackTexName;   // the background, 240 x 344 under the title bar
    INT DefaultFromRight;       // the first spot: (screen width - DefaultFromRight,
    INT DefaultTop;             //   DefaultTop)
    UBOOL bWantOpen;            // open: shown, or to be shown again with the desktop
    UBOOL bClosingByCode;       // the HideWindow of Close() (no OnUserClose)
    UBOOL bCreating;            // the HideWindow that follows the creation (no position saved)
    INT DialogKind;             // CRAFTDLG_*
    NCWnd* Children[12];        // made by AddButton / AddList, destroyed by OnDestroy
    INT NumChildren;
    NCRecipeListWnd* Lists[4];  // the wheel goes to the one under the cursor
    INT NumLists;
    // the common look (LoadCommon)
    UTexture* BackTex;
    UTexture* MpTex;            // "L2UI.PlayerStatusWnd.MP": MP bars
    UTexture* ItemClickTex;     // "l2ui.nwindow.item_click": the selected icon's frame
    UTexture* GradeTex[5];      // "symbol.grade_d" .. "symbol.grade_s"

    NCRecipeWnd();
    NCRecipeWnd(const TCHAR* InBackTexName, const TCHAR* InTitle, const TCHAR* InPosKey,
                INT InFromRight, INT InTop);

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // commands, prompts
    virtual INT OnCreate();                                          // title bar, controls
    virtual INT OnDestroy();
    virtual void ShowWindow();                                       // only when open
    virtual void HideWindow();                                       // the close button
    virtual INT OnPaint(NCGDevice* Device);
    virtual INT OnMouseWheel(WPARAM wParam, LPARAM lParam);          // to the list below

    // what each window fills in
    virtual void OnCreateChildren() {}
    virtual void PaintWindow(NCGDevice*) {}
    virtual void OnCommand(INT, INT) {}                              // NWM_COMMAND (id, value)
    virtual void OnUserClose() {}                                    // the close button
    virtual void OnDialogNumber(SQWORD) {}
    virtual void OnDialogText(const TCHAR*) {}
    virtual void OnDialogConfirm() {}
    // X, Y: the item's cell (a row's top left), list coordinates
    virtual void PaintListItem(NCRecipeListWnd*, NCGDevice*, INT, INT, INT, UBOOL) {}
    virtual void OnListClick(NCRecipeListWnd*, INT, UBOOL) {}
    virtual void PaintListToolTip(NCRecipeListWnd*, NCGDevice*, INT, INT, INT) {}

    // made hidden, as a child of the desktop
    void Create(NCWnd* Parent);
    // shown where the last window of its PosKey was closed, in front
    void Open();
    // hidden by the code: no OnUserClose, no sound
    void Close();
    UBOOL IsOpen() const { return bWantOpen; }
    UBOOL FocusInside();           // the keyboard focus is in this window
    UBOOL Frontmost();             // the desktop's frontmost child

    // what the paints use (coordinates relative to the window painted; names ours)
    static SIZE Measure(const TCHAR* Text, INT WrapW = 0);
    static void PaintText(NCGDevice* Device, INT X, INT Y, DWORD Color, const TCHAR* Text,
                          INT WrapW = 0);
    static void PaintTextRight(NCGDevice* Device, INT Right, INT Y, DWORD Color,
                               const TCHAR* Text);
    static void PaintTex(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H, INT U,
                         INT V, INT UL, INT VL);
    static void PaintStretchUV(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H,
                               INT U, INT V, INT UL, INT VL);
    static void PaintStretch(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H);
    static void PaintIcon(NCGDevice* Device, INT ItemID, INT X, INT Y);   // 32 x 32
    static UTexture* LoadTex(const TCHAR* Name);           // NCObject::FindTexture's
    static UTexture* ItemIcon(INT ItemID);                 // kept per item id
    static const TCHAR* ItemName(INT ItemID);              // "" when unknown
    static const TCHAR* ItemDescription(INT ItemID);       // "" when none
    static void Sound(INT Index);   // 0 click, 2 failed, 3 pickup, 5 window open, 6 close
    static void SystemMessage(const TCHAR* Text);          // a system line in the chat
    static INT ScreenW();                                  // the desktop's size
    static INT ScreenH();
    static UBOOL CursorPos(INT* X, INT* Y);                // in the game window's client area
    static void ForgetCaches();                            // another console: new textures

protected:
    NCButton* AddButton(INT ID, INT X, INT Y, INT W, INT H, const TCHAR* Label,
                        const TCHAR* NormalTex = TEXT("L2UI.Control.DefaultButton"),
                        const TCHAR* PushedTex = TEXT("L2UI.Control.DefaultButton_click"));
    // rows (Cols 1) or an icon grid (Cols > 1, CellW the column pitch)
    NCRecipeListWnd* AddList(INT ID, INT X, INT Y, INT W, INT H, INT RowH, INT Cols = 1,
                             INT CellW = 0, INT PadTop = 0);
    static void SetButtonText(NCButton* Button, const TCHAR* Text);
    // the stock prompts, answered to this window
    UBOOL AskNumber(INT SysMsgID);                         // keypad, 0 .. 100,000,000
    UBOOL AskText(INT SysMsgID, const TCHAR* Initial);     // a line of text
    UBOOL AskConfirm(const TCHAR* Text);                   // yes / no
    void CancelDialog();                                   // hides a prompt still waiting
    // the common look (RecipeWnd)
    void LoadCommon();
    void PaintBack(NCGDevice* Device);
    void PaintMpBar(NCGDevice* Device, INT X, INT Y, INT Cur, INT Max);
    void PaintGrade(NCGDevice* Device, INT X, INT Y, INT Grade);
    void PaintSelection(NCGDevice* Device, INT X, INT Y);

private:
    void SavePos();
    void RestorePos();
};

/*-----------------------------------------------------------------------------
    NCraftToolTip - an item tool tip with the stock NCItemWnd look (184 px wide,
    L2UI.Tooltip.Tool02 / Tool01 / Tool03): built in a PaintListToolTip, then drawn
    above the item (below it when there is no room).
-----------------------------------------------------------------------------*/
class NCraftToolTip
{
public:
    NCraftToolTip();
    void Line(const TCHAR* Text, DWORD Color);      // starts a line
    void Seg(const TCHAR* Text, DWORD Color);       // adds to the current line
    void Wrapped(const TCHAR* Text, DWORD Color);   // a paragraph wrapped to the box
    // above the anchor (painter coordinates), kept on the screen
    void Draw(NCGDevice* Device, INT AX, INT AY, INT AW, INT AH);

private:
    struct NSeg
    {
        INT Text;          // offset in Pool
        DWORD Color;
    };
    struct NLine
    {
        NSeg Segs[4];
        INT NumSegs;
        UBOOL bWrap;
    };
    INT AddText(const TCHAR* Text);

    NLine Lines[10];
    INT NumLines;
    TCHAR Pool[1024];
    INT Used;
};

#endif  // NW_FEATURES

#pragma once
// NCSelectDeliverWnd - "Send package", first step (UClass "CSelectDeliverWnd", 0x8CC): a
// combo box with the characters a package can go to (8 at most, the console adds them), OK
// asks the server for what can be sent to the one chosen, Cancel closes.
//
// NCDeliverWnd - the package window (UClass "CDeliverWnd", 0xC0): the warehouse's layout
// (NCStoreWnd.h) with one mode: the inventory on top, what is sent below, 1000 adena a slot.
// OK sends the list to the character chosen (TargetID). The console opens it with the
// target and the player's adena and fills the upper grid.
//
// NCDeliverItemWnd - its grids (UClass "CDeliverItemWnd", 0x11C): the warehouse's grid
// without the mode and side fields; the tool tips are the inventory's with 128-character
// buffers (NItemToolTip.h; the box's text buffer has 1024, see DrawItemToolTip).
//
// Desktop (NCConsole) +0x138 is NCDeliverWnd, +0x13C NCSelectDeliverWnd.

#include "nwindow/NCItemWnd.h"

class NCButton;
class NCComboBox;

// NWM_COMMAND ids of NCSelectDeliverWnd (names ours; NCDeliverWnd uses NCStoreWnd's)
enum
{
    NID_SELDELIVER_FRAME = 0xE432,
    NID_SELDELIVER_TARGETS = 0xE433,  // the combo box: lParam the entry chosen
    NID_SELDELIVER_OK = 0xE434,
    NID_SELDELIVER_CANCEL = 0xE435,
};

class NCDeliverItemWnd : public NCItemWnd
{
    NW_DECLARE_CLASS(NCDeliverItemWnd, NCItemWnd, 0, 0x1012e220)

    NCDeliverItemWnd* Target;        // +0x118 where its items go (the other grid)

    NCDeliverItemWnd();                                              // 0x1005ae50
    NCDeliverItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems, INT InCellSize); // 0x1005aef0
    // ~NCDeliverItemWnd                                             // 0x1005aea0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10057b10 (shared)
    virtual INT OnDestroy();                                         // 36 0x100a39e0 (shared)
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x1005af30
    virtual void DrawItemToolTip(NCGDevice* Device, NItemEntry* Item, INT X, INT Y); // 67 0x10058260
    // names ours
    virtual INT OnLButtonDblClick(WPARAM wParam, LPARAM lParam);     // 68 0x1005b110 WM_LBUTTONDBLCLK
    virtual INT OnDialogResult(WPARAM wParam, LPARAM lParam);        // 69 0x1005b260

    // names ours
    void SetTarget(NCDeliverItemWnd* InTarget);                      // 0x10057ba0 (shared)
    void SetToolTip(NItemEntry* Item);                               // 0x10057bb0
    UBOOL AddItem(NItemInfo* Info);                                  // 0x1005a830 0 when full
    UBOOL AddCopy(NItemEntry* Src, INT Count);                       // 0x1005a930 0 when full
    void MoveItem(NItemEntry* Item, INT Count);                      // 0x1005aa00 to Target
};
NW_ASSERT_OFFSET(NCDeliverItemWnd, Target, 0x118);
NW_ASSERT_SIZE(NCDeliverItemWnd, 0x11C);

class NCDeliverWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCDeliverWnd, NCWnd, 0, 0x1012ec70)

    NCDeliverItemWnd* InventoryItems; // +0x94 on top
    NCDeliverItemWnd* SendItems;     // +0x98 below: what is sent
    UTexture* BackTex;               // +0x9C "L2UI.StoreWnd.StoreBack"
    NCButton* UpButton;              // +0xA0
    NCButton* DownButton;            // +0xA4
    NCButton* OKButton;              // +0xA8
    NCButton* CancelButton;          // +0xAC
    NCWnd* UnknownB0;                // +0xB0 OnDestroy's, never created
    INT Adena;                       // +0xB4 the player's
    INT Fee;                         // +0xB8 UpdateFee
    INT TargetID;                    // +0xBC (-1) the character the package goes to

    NCDeliverWnd();                                                  // 0x10059d30
    // ~NCDeliverWnd                                                 // 0x10059dc0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100b56e0 (shared)
    virtual INT OnCreate();                                          // 35 0x1005b2a0
    virtual INT OnDestroy();                                         // 36 0x1005a110
    virtual void ShowWindow();                                       // 42 0x10059a80
    virtual void HideWindow();                                       // 43 0x10059a90
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x10059e10
    // names ours
    virtual INT OnOK();                                              // 62 0x1005a260
    virtual INT OnCancel();                                          // 63 0x10059ad0
    virtual INT OnUp();                                              // 64 0x1005ab30
    virtual INT OnDown();                                            // 65 0x1005acc0
    virtual INT OnClose();                                           // 66 0x10059ad0 (OnCancel's body)

    // names ours (the console calls most of these)
    void SetAdena(INT InAdena);                                      // 0x10059a20
    void SetTargetID(INT InTargetID);                                // 0x10059a30
    void Clear();                                                    // 0x10059a40 grids emptied,
                                                                     //   target -1, adena and fee 0
    void UpdateFee();                                                // 0x10059aa0
    UBOOL AddInventoryItem(NItemInfo* Info);                         // 0x1005ab20
};
NW_ASSERT_OFFSET(NCDeliverWnd, BackTex, 0x9C);
NW_ASSERT_OFFSET(NCDeliverWnd, TargetID, 0xBC);
NW_ASSERT_SIZE(NCDeliverWnd, 0xC0);

// one character a package can go to
struct NDeliverTarget
{
    INT ID;                          // +0x00 sent back with the request
    TCHAR Name[128];                 // +0x04 the combo box's line
};
NW_ASSERT_SIZE(NDeliverTarget, 0x104);

class NCSelectDeliverWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCSelectDeliverWnd, NCWnd, 0, 0x1012e748)

    UTexture* BackTex;               // +0x94 "L2UI.etcwnd.NPCHtml_BACK"
    NCComboBox* TargetCombo;         // +0x98
    NCButton* OKButton;              // +0x9C
    NCButton* CancelButton;          // +0xA0
    INT Selected;                    // +0xA4 (-1) entry chosen in the combo box
    NDeliverTarget Targets[8];       // +0xA8
    INT NumTargets;                  // +0x8C8

    NCSelectDeliverWnd();                                            // 0x1005a3a0
    // ~NCSelectDeliverWnd                                           // 0x1005a400

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10059af0
    virtual INT OnCreate();                                          // 35 0x1005a450
    virtual INT OnDestroy();                                         // 36 0x1005a7d0
    virtual void ShowWindow();                                       // 42 0x10059c40
    virtual void HideWindow();                                       // 43 0x10059c50
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x1005a710
    // names ours
    virtual INT OnSelect(LPARAM Index);                              // 62 0x10059d10
    virtual INT OnOK();                                              // 63 0x10059c60
    virtual INT OnCancel();                                          // 64 0x10059d00 (folded)

    // names ours (the console)
    void AddTarget(INT ID, const TCHAR* Name);                       // 0x10059ba0 8 at most
    void Clear();                                                    // 0x10059c10
};
NW_ASSERT_OFFSET(NCSelectDeliverWnd, Selected, 0xA4);
NW_ASSERT_OFFSET(NCSelectDeliverWnd, Targets, 0xA8);
NW_ASSERT_OFFSET(NCSelectDeliverWnd, NumTargets, 0x8C8);
NW_ASSERT_SIZE(NCSelectDeliverWnd, 0x8CC);

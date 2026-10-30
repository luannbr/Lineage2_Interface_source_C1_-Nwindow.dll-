#pragma once
// NCShortCutWnd - the shortcut bar (UClass "CShortCutWnd", 0x214; desktop +0xE4): Pages pages
// of Cols slots (10 x 10), each an item, a skill or an action the server registered
// (NConsoleWnd hands them over: SetShortCut). F1 .. F10 (or 1 .. 0 while the chat line needs
// Enter) or a click uses the slot of the current page; the arrows turn the pages, the corner
// button turns the bar between vertical (44 x 417) and horizontal. A slot dragged off the bar
// is deleted on the server. Each slot's key waits before it works again: until released,
// until its skill's cool time is over, or a fixed time (shots, attack, pick up).

#include "nwindow/NControl.h"
#include "nwindow/NCCoolTimeIcon.h"

struct NItemInfo;
class NCPushButton;

// NWM_COMMAND ids of the bar's controls
enum
{
    NID_SHORTCUT_FRAME = (INT)0xFF000003,
    NID_SHORTCUT_NEXT = (INT)0xFF000004,
    NID_SHORTCUT_PREV = (INT)0xFF000005,
    NID_SHORTCUT_ROTATE = (INT)0xFF000008,
};

// NShortCutItem::Type (names ours)
enum { SHORTCUT_Item = 1, SHORTCUT_Skill = 2, SHORTCUT_Action = 3 };

// A slot of the bar (name ours; 0xA80 bytes, an array of Cols * Pages).
struct NShortCutItem
{
    UTexture* Icon;                // +0x000
    TCHAR Name[128];               // +0x004
    TCHAR IconName[128];           // +0x104
    NCCoolTimeIcon CoolTime;       // +0x204
    INT Type;                      // +0x24C SHORTCUT_*, -1 when cleared
    INT ObjectID;                  // +0x250 items: the inventory object
    INT ID;                        // +0x254 item class, skill or action id
    INT Level;                     // +0x258 skills (-1 when cleared)
    INT Count;                     // +0x25C items: "name(count)"
    INT Unknown260;                // +0x260 (-1 when cleared)
    INT Enchant;                   // +0x264 items: "+enchant name"
    UBOOL bStackable;              // +0x268 items
    INT Owner;                     // +0x26C items: 1 the player's inventory, 2 the pet's
    TCHAR DisplayName[1024];       // +0x270 the tool tip: name with enchant, count or MP
    INT UnknownA70[2];             // +0xA70
    INT MpConsume;                 // +0xA78 skills: "name(MP:n)"
    UBOOL bUsed;                   // +0xA7C

    NShortCutItem();                                                 // 0x100a7ff0
    NShortCutItem(const NItemInfo& Info);                            // 0x100a8010
    ~NShortCutItem() {}                                              // 0x100a8b10 (jumps to
                                                                     //   the cool time's)
    // accessors (names ours)
    UTexture* GetIcon() { return Icon; }                             // 0x10033ac0 (folded)
    UBOOL IsStackable();                                             // 0x100a8140
    TCHAR* GetName();                                                // 0x100a8110 NULL when empty
    TCHAR* GetDisplayName();                                         // 0x100a8120 NULL when empty
    void CopyFrom(NShortCutItem* Src);                               // 0x100a8150
    void Clear();                                                    // 0x100a8240
};
NW_ASSERT_OFFSET(NShortCutItem, CoolTime, 0x204);
NW_ASSERT_OFFSET(NShortCutItem, Type, 0x24C);
NW_ASSERT_OFFSET(NShortCutItem, Owner, 0x26C);
NW_ASSERT_OFFSET(NShortCutItem, DisplayName, 0x270);
NW_ASSERT_OFFSET(NShortCutItem, MpConsume, 0xA78);
NW_ASSERT_SIZE(NShortCutItem, 0xA80);

// When the key of a slot works again (name ours; 0x14 bytes, an array of Cols * Pages).
struct NShortCutKey
{
    DWORD Elapsed;                 // +0x00 ms since the last use
    DWORD Interval;                // +0x04 ms it waits (shots, attack: 200, pick up: 500)
    UBOOL bReady;                  // +0x08 the key uses the slot
    UBOOL bOnKeyUp;                // +0x0C ready again when the key is released
    UBOOL bOnCoolTime;             // +0x10 ready again when the cool time is over

    NShortCutKey() : Elapsed(0), Interval(0), bReady(1), bOnKeyUp(0), bOnCoolTime(0) {}
};
NW_ASSERT_SIZE(NShortCutKey, 0x14);

class NCShortCutWnd : public NControl
{
    NW_DECLARE_CLASS(NCShortCutWnd, NControl, 0, 0x1014b170)

    NShortCutKey* Keys;                    // +0x1A0
    NShortCutItem* Items;                  // +0x1A4
    TArray<INT> Unused1A8;                 // +0x1A8
    UTexture* BackTex;                     // +0x1B4 "L2UI.ShortcutWnd.Back" (vertical),
                                           //   "...Back1" (horizontal)
    UTexture* MaskTex;                     // +0x1B8 "L2UI.ShortcutWnd.mask" (not drawn)
    UTexture* ToolTipTex;                  // +0x1BC "L2UI.tooltip.mini-tool01"
    UTexture* IconBackTex;                 // +0x1C0 "l2ui.nwindow.icon_back" frame of a slot
    UTexture* IconClickTex;                // +0x1C4 "l2ui.nwindow.icon_click" over the pressed one
    UTexture* ToggleTex;                   // +0x1C8 "L2_SkillTime.ToggleEffect.ToggleEffect001"
                                           //   over a toggle skill that is on
    UTexture* PadButtonTex[4];             // +0x1CC "L2UI.ShortcutWnd.pad_button_1" .. 4
    NCPushButton* NextButton;              // +0x1DC right / up arrow
    NCPushButton* PrevButton;              // +0x1E0 left / down arrow
    NCPushButton* RotateButton;            // +0x1E4 vertical / horizontal
    UBOOL bPadShift1;                      // +0x1E8 game pad: which slots its four buttons
    UBOOL bPadShift2;                      // +0x1EC   use (set by the console, not read here
                                           //   but for drawing the button marks)
    UBOOL bPressed;                        // +0x1F0 mouse button down on PressedSlot
    UBOOL bDragging;                       // +0x1F4
    UBOOL bHorizontal;                     // +0x1F8
    INT Cols;                              // +0x1FC slots per page (10)
    INT Pages;                             // +0x200 (10)
    INT Page;                              // +0x204
    INT IconSize;                          // +0x208 (32)
    INT PressedSlot;                       // +0x20C -1 = none
    INT HoverSlot;                         // +0x210 slot under the mouse, -1 = none

    NCShortCutWnd(INT InCols = 10, INT InPages = 10, INT InIconSize = 32);          // 0x100ab1f0
    // ~NCShortCutWnd                                                // 0x100ab320

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100a82a0
    virtual INT OnCreate();                                          // 35 0x100aab00
    virtual INT OnDestroy();                                         // 36 0x100a8b20
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a8c10
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x100a9660
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100a9980
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100a9940
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x100a9a20
#if NW_FEATURES
    // phase 6: AutoShot (NWindow_C1 src/inject/AutoShot.cpp): not stock, the stock bar leaves
    // the right button to NCWnd's no-op
    virtual INT OnRButtonDown(WPARAM wParam, LPARAM lParam);         // 58
#endif
    // names ours
    virtual INT OnNextClicked();                                     // 63 0x100aaa80
    virtual INT OnPrevClicked();                                     // 64 0x100aaaa0
    virtual INT OnRotateClicked();                                   // 65 0x100a88b0

    // names ours; the console calls most of them (UL2ConsoleWnd slots 115 .. 119, 263, 264,
    // Tick, HookShortCutKey, ReloadGameTexts, ResetWindowPositions)
    INT UpdateItem(INT ObjectID);                                    // 0x100a8350 inventory
    INT UpdatePetItem(INT ObjectID);                                 // 0x100a84a0 pet inventory
    INT SlotAt(INT X, INT Y);                                        // 0x100a85f0 -1 = none
    void UpdateLayout();                                             // 0x100a86e0 after a turn
    void ReloadNames();                                              // 0x100a88e0 new language
    INT PageSlot(INT Index);                                         // 0x100a8ae0
    INT DeleteShortCut(INT Slot);                                    // 0x100aa0d0
    void SetPage(INT NewPage);                                       // 0x100aa130
    void Tick(FLOAT DeltaTime);                                      // 0x100aa1a0
    INT OnShortCutKeyUp(UINT Key, LPARAM lParam);                    // 0x100aa260
    INT OnShortCutKeyDown(UINT Key, LPARAM lParam);                  // 0x100aa310
    void NextPage();                                                 // 0x100aa9a0
    void PrevPage();                                                 // 0x100aaa10
    void ClearShortCuts();                                           // 0x100aaac0
    INT SetShortCut(NItemInfo* Info, INT Owner);                     // 0x100aaf30
};
NW_ASSERT_OFFSET(NCShortCutWnd, Keys, 0x1A0);
NW_ASSERT_OFFSET(NCShortCutWnd, BackTex, 0x1B4);
NW_ASSERT_OFFSET(NCShortCutWnd, NextButton, 0x1DC);
NW_ASSERT_OFFSET(NCShortCutWnd, Cols, 0x1FC);
NW_ASSERT_OFFSET(NCShortCutWnd, HoverSlot, 0x210);
NW_ASSERT_SIZE(NCShortCutWnd, 0x214);

#pragma once
// NCActionItemWnd - a grid of action icons (UClass "CActionItemWnd", 0x154): one of the three
// on the Basic page of the action window (basic, party, social). A click runs the action
// (UseAction: requests to the server, social motions...); an icon dragged onto the shortcut
// bar is registered there. The selected icon's tool tip shows its name and description.

#include "nwindow/NCWnd.h"

struct NItemInfo;

// actionname-*.dat ids the actions use (names from the English table; the shortcut bar
// uses them too)
enum
{
    ACTION_SitStand, ACTION_WalkRun, ACTION_Attack, ACTION_Exchange, ACTION_NextTarget,
    ACTION_PickUp, ACTION_Assist, ACTION_Invite, ACTION_LeaveParty, ACTION_Dismiss,
    ACTION_PrivateStoreSell, ACTION_PartyMatching, ACTION_Greeting, ACTION_Victory,
    ACTION_Advance, ACTION_PetStay, ACTION_PetAttack, ACTION_PetStop, ACTION_PetPickUp,
    ACTION_PetReturn, ACTION_PetSkill, ACTION_SummonStay, ACTION_SummonAttack,
    ACTION_SummonStop, ACTION_SocialYes, ACTION_SocialNo, ACTION_SocialBow, ACTION_SummonSkill,
    ACTION_PrivateStoreBuy, ACTION_SocialUnaware, ACTION_SocialWaiting, ACTION_SocialLaugh,
};

// An icon of the grid (name ours; 0x1A10 bytes, created by AddItem).
struct NActionItem
{
    INT ActionID;              // +0x0000 (-1) FL2ActionData::ActionID
    INT Param;                 // +0x0004 (-1) the social action of the social ones
    UTexture* Icon;            // +0x0008
    TCHAR Name[128];           // +0x000C
    TCHAR IconName[128];       // +0x010C
    TCHAR Description[1024];   // +0x020C
    TCHAR UnknownA0C[1024];    // +0x0A0C (empty)
    TCHAR ToolTip[1024];       // +0x120C "name\ndescription" (SetItemToolTip)
    UBOOL bSelected;           // +0x1A0C

    NActionItem(const NItemInfo& Info);                              // 0x10033990
    // accessors the stock code shares with other structs' (bodies folded)
    UTexture* GetIcon() { return Icon; }                             // 0x10033a40 (folded)
    INT GetActionID() { return ActionID; }                           // 0x10033ac0 (folded)
    INT GetParam() { return Param; }                                 // 0x10033ad0 (folded)
    TCHAR* GetName()                                                 // 0x10033a50 (folded)
    {
        TCHAR* S = Name;
        return S && *S ? S : NULL;
    }
    // names ours
    TCHAR* GetDescription();                                         // 0x10033a60 NULL when empty
    TCHAR* GetToolTip();                                             // 0x10033a80 NULL when empty
    void Select();                                                   // 0x10033aa0
    void Deselect();                                                 // 0x10033ab0
};
NW_ASSERT_OFFSET(NActionItem, Name, 0xC);
NW_ASSERT_OFFSET(NActionItem, Description, 0x20C);
NW_ASSERT_OFFSET(NActionItem, ToolTip, 0x120C);
NW_ASSERT_OFFSET(NActionItem, bSelected, 0x1A0C);
NW_ASSERT_SIZE(NActionItem, 0x1A10);

class NCActionItemWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCActionItemWnd, NCWnd, 0, 0x1012ad98)

    TArray<NActionItem*> Items;            // +0x94
    BYTE _UnknownA0[0xD8 - 0xA0];
    UTexture* ItemClickTex;                // +0xD8 "l2ui.nwindow.item_click" (not drawn here)
    UTexture* ToolTipTex;                  // +0xDC "L2UI.tooltip.mini-tool01" (not drawn here)
    UTexture* TipTopTex;                   // +0xE0 "L2UI.Tooltip.Tool02"  tool tip box: top,
    UTexture* TipMidTex;                   // +0xE4 "L2UI.Tooltip.Tool01"  a strip per text line,
    UTexture* TipBottomTex;                // +0xE8 "L2UI.Tooltip.Tool03"  bottom
    UTexture* IconBackTex;                 // +0xEC "l2ui.nwindow.icon_back" frame of each icon
    UTexture* IconClickTex;                // +0xF0 "l2ui.nwindow.icon_click" over the pressed one
    UBOOL bPressed;                        // +0xF4 mouse button down on PressedIndex
    UBOOL bDragging;                       // +0xF8 an icon is being dragged
    INT Cols;                              // +0xFC
    INT Rows;                              // +0x100
    INT MaxItems;                          // +0x104
    INT IconSize;                          // +0x108 (32)
    INT PressedIndex;                      // +0x10C (-1)
    INT HoverIndex;                        // +0x110 (-1) icon under the mouse
    BYTE _Unknown114[0x154 - 0x114];

    NCActionItemWnd(INT InCols = 5, INT InRows = 2, INT InMaxItems = 15, INT InIconSize = 32);  // 0x10036f00
    // ~NCActionItemWnd                                              // 0x10037000

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x10004e90 (folded)
    virtual INT OnCreate();                                          // 35 0x10033ae0
    virtual INT OnDestroy();                                         // 36 0x10036af0
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100343e0
    virtual INT OnDrawToolTip(NCGDevice* Device);                    // 51 0x10034550
    virtual INT OnMouseMove(WPARAM wParam, LPARAM lParam);           // 55 0x100347a0
    virtual INT OnLButtonDown(WPARAM wParam, LPARAM lParam);         // 56 0x100368b0
    virtual INT OnLButtonUp(WPARAM wParam, LPARAM lParam);           // 57 0x10036930
    // names ours
    virtual void UseAction();                                        // 62 0x10034840
    virtual void DrawItemToolTip(NCGDevice* Device, NActionItem* Item, INT X, INT Y); // 63 0x10033c70

    // names ours
    INT HitTest(INT X, INT Y);                                       // 0x10033b50 icon, -1 = none
    void SetItemToolTip(NActionItem* Item);                          // 0x10033bd0
    void ClearSelection();                                           // 0x10034ce0
    INT AddItem(NItemInfo* Info);                                    // 0x10036b60
    void ClearItems();                                               // 0x10036c10
};
NW_ASSERT_OFFSET(NCActionItemWnd, Items, 0x94);
NW_ASSERT_OFFSET(NCActionItemWnd, ItemClickTex, 0xD8);
NW_ASSERT_OFFSET(NCActionItemWnd, bPressed, 0xF4);
NW_ASSERT_OFFSET(NCActionItemWnd, IconSize, 0x108);
NW_ASSERT_OFFSET(NCActionItemWnd, HoverIndex, 0x110);
NW_ASSERT_SIZE(NCActionItemWnd, 0x154);

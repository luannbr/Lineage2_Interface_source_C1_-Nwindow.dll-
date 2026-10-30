/*=============================================================================
    NCActionItemListWnd.cpp - the Basic page of the action window. Stock: 0x10034d10 ..
    0x10034f4e, 0x10036c70 .. 0x10036db3, 0x10037060 .. 0x1003724c.
=============================================================================*/
#include "nwindow/NCActionItemListWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NCGDevice.h"

// 0x10034d10 - only BackTex is cleared
NCActionItemListWnd::NCActionItemListWnd()
{
    BackTex = NULL;
}
NW_HOOK_CTOR(0x10034d10, NCActionItemListWnd)

// 0x10034d60
NW_HOOK_DTOR(0x10034d60, NCActionItemListWnd)

// stock body shared with NControl's (0x10004e90)
INT NCActionItemListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// one grid of the page (repeated inline in the stock OnCreate)
static FORCEINLINE NCActionItemWnd* CreateGrid(NCWnd* Parent, INT Rows, INT Y, INT H)
{
    NCActionItemWnd* Grid = new NCActionItemWnd(6, Rows, 15, 32);
    Grid->SetWindowRect(15, Y, 207, H);
    Parent->CreateChild(Grid, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    return Grid;
}

// 0x10037060 (vslot 35)
INT NCActionItemListWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.ActionWnd.ActionBack"));
    BasicGrid = CreateGrid(this, 2, 15, 70);
    PartyGrid = CreateGrid(this, 1, 109, 35);
    SocialGrid = CreateGrid(this, 3, 168, 100);
    LoadActions();
    return 1;
}
NW_HOOK_METHOD(0x10037060, NCActionItemListWnd, OnCreate)

// a child window goes (repeated inline in the stock OnDestroy)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// 0x10034eb0 (vslot 36)
INT NCActionItemListWnd::OnDestroy()
{
    DestroyChild(this, BasicGrid);
    DestroyChild(this, PartyGrid);
    DestroyChild(this, SocialGrid);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10034eb0, NCActionItemListWnd, OnDestroy)

// 0x10034db0 (vslot 50) - the background reaches 17 pixels above the page (behind the tabs);
// labels "Party" / "Social" (sysstring 130 / 204). The clip is set before the origin.
INT NCActionItemListWnd::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY - 17, Width, Height + 17);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, -17, 240, 301, 0, 0, 240, 301, BackTex);
    Device->DrawText(16, 90, 0xFFFFFFFF, *GL2GameData.SysString[130], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(16, 150, 0xFFFFFFFF, *GL2GameData.SysString[204], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10034db0, NCActionItemListWnd, OnPaint)

// 0x10036c70 - every action of the table into the grid of its category (a missing icon is
// replaced by black); a full grid drops the rest
void NCActionItemListWnd::LoadActions()
{
    for (INT i = 0; i < 1000; i++)
    {
        FL2ActionData& Data = GL2GameData.Actions[i];
        if (!Data.bValid)
            continue;
        INT Category = Data.Category;
        if (!Category)
            continue;
        NItemInfo Info;
        Info.ID = Data.ActionID;
        Info.Param = Data.Param;
        appStrcpy(Info.Name, *Data.Name);
        appStrcpy(Info.IconName, *Data.IconName);
        appStrcpy(Info.Description, *Data.Description);
        Info.Icon = FindTexture(Info.IconName);
        if (!Info.Icon)
            Info.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        switch (Category)
        {
        case 1:
            BasicGrid->AddItem(&Info);
            break;
        case 2:
            PartyGrid->AddItem(&Info);
            break;
        case 3:
            SocialGrid->AddItem(&Info);
            break;
        }
    }
}
NW_HOOK_METHOD(0x10036c70, NCActionItemListWnd, LoadActions)

NW_CHECK_VTABLE(NCActionItemListWnd, 0x100df7f8, 62)

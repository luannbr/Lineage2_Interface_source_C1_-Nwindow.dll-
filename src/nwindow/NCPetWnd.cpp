/*=============================================================================
    NCPetWnd.cpp - the pet and summon windows and their Information and Action pages.
    Stock: 0x1008ee70 .. 0x1008ee90, 0x10090e10 .. 0x100916fb, 0x10091700 .. 0x10091823,
    0x10091fb0 .. 0x100938ca, 0x10093940 .. 0x10093a01, 0x100940b0 .. 0x100940ea,
    0x100940f0 .. 0x100941b1, 0x100bb540 .. 0x100bb57e.
=============================================================================*/
#include "nwindow/NCPetWnd.h"
#include "nwindow/NCSummonedWnd.h"
#include "nwindow/NCActionItemWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIcon.h"
#include "nwindow/NCPetInventoryWnd.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NDialogInfo.h"
#include "nwindow/NItemInfo.h"

#include <stdio.h>
#include <wchar.h>

// the action categories the two Action pages list (FL2ActionData::Category)
enum { ACTION_Pet = 4, ACTION_Summon = 5 };

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// back from the icon, at its place (inlined in the stock ShowWindows, name ours)
static FORCEINLINE void RestoreFromIcon(NCWnd* Wnd)
{
    if (Wnd->Icon)
    {
        Wnd->SetWindowRect(Wnd->Icon->ScreenX, Wnd->Icon->ScreenY, Wnd->Width, Wnd->Height);
        Wnd->Icon->SetWindowRect(0, 0, 0, 0);
        Wnd->Icon->HideWindow();
    }
}

// every text of these windows goes out with the same trailing arguments
static FORCEINLINE void DrawString(NCGDevice* Device, INT X, INT Y, DWORD Color, const TCHAR* Text)
{
    Device->DrawText(X, Y, Color, Text, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

// a number centered on CenterX in the values' color (spelled out at each use in the stock code)
static FORCEINLINE void DrawNumber(NCGDevice* Device, TCHAR* Buf, INT CenterX, INT Y, INT Value)
{
    SIZE Size;
    appSprintf(Buf, TEXT("%d"), Value);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    DrawString(Device, CenterX - Size.cx / 2, Y, 0xFFB09B79, Buf);
}

// "now / max" around a slash centered at 76 (HP) and 181 (MP), in the large digits
static FORCEINLINE void DrawHPMP(NCGDevice* Device, TCHAR* Buf, INT Y, const PetInfo& Info)
{
    SIZE Size;
    appSprintf(Buf, TEXT("%s"), TEXT("/"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 76 - Size.cx / 2, Y, 0xFFFFFFFF, Buf);
    appSprintf(Buf, TEXT("%d"), Info.HP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 66 - Size.cx, Y, 0xFFFFFFFF, Buf);
    appSprintf(Buf, TEXT("%d"), Info.MaxHP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 82, Y, 0xFFFFFFFF, Buf);
    appSprintf(Buf, TEXT("%s"), TEXT("/"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 181 - Size.cx / 2, Y, 0xFFFFFFFF, Buf);
    appSprintf(Buf, TEXT("%d"), Info.MP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 171 - Size.cx, Y, 0xFFFFFFFF, Buf);
    appSprintf(Buf, TEXT("%d"), Info.MaxMP);
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 187, Y, 0xFFFFFFFF, Buf);
}

// a bar Max pixels wide at most (the stock code repeats the test for each bar)
static FORCEINLINE void DrawBar(NCGDevice* Device, INT X, INT Y, INT W, INT Max, UTexture* Tex)
{
    if (W > 0 && W <= Max)
        Device->DrawTexture(X, Y, W, 10, 0, 0, W, 10, Tex);
    else if (W > Max)
        Device->DrawTexture(X, Y, Max, 10, 0, 0, Max, 10, Tex);
}

// "<level> <name>" after the "Lv" label; a nameless pet or summon gets a NULL name, which
// appSprintf prints as "(null)", as in the stock code
static FORCEINLINE void DrawLevelName(NCGDevice* Device, const PetInfo& Info)
{
    TCHAR LevelText[64];
    DrawString(Device, 17, 30, 0xFFA3A3A3, *GL2GameData.SysString[88]);   // "Lv"
    const TCHAR* Name = Info.Name;
    appSprintf(LevelText, TEXT("%d %s"), Info.Level, Name && *Name ? Name : NULL);
    DrawString(Device, 35, 30, 0xFFB09B79, LevelText);
}

// The Information page of both windows (one stock body, 0x10090f90): the combat values of the
// pet or summon, read again at each paint.
static FORCEINLINE INT PaintInfoPage(NCWnd* Wnd, UTexture* BackTex, PetInfo& Info,
                                     NCGDevice* Device)
{
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX, Wnd->ScreenY, Wnd->Width, Wnd->Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Wnd->Width, Wnd->Height, 0, 0, Wnd->Width, Wnd->Height,
                            BackTex);
    DrawString(Device, 17, 24, 0xFFA3A3A3, *GL2GameData.SysString[94]);   // "P.Atk."
    DrawString(Device, 17, 41, 0xFFA3A3A3, *GL2GameData.SysString[95]);   // "P.Def."
    DrawString(Device, 17, 58, 0xFFA3A3A3, *GL2GameData.SysString[96]);   // "Accuracy"
    DrawString(Device, 17, 75, 0xFFA3A3A3, *GL2GameData.SysString[113]);  // "Critical"
    DrawString(Device, 17, 92, 0xFFA3A3A3, *GL2GameData.SysString[111]);  // "Atk. Spd."
    DrawString(Device, 122, 24, 0xFFA3A3A3, *GL2GameData.SysString[98]);  // "M.Atk."
    DrawString(Device, 122, 41, 0xFFA3A3A3, *GL2GameData.SysString[99]);  // "M.Def."
    DrawString(Device, 122, 58, 0xFFA3A3A3, *GL2GameData.SysString[97]);  // "Evasion"
    DrawString(Device, 122, 75, 0xFFA3A3A3, *GL2GameData.SysString[432]); // "Speed"
    DrawString(Device, 122, 92, 0xFFA3A3A3, *GL2GameData.SysString[112]); // "Casting Spd."
    Info = GConsole->GetPetInfo();
    TCHAR Buf[64];
    DrawNumber(Device, Buf, 89, 24, Info.PAtk);
    DrawNumber(Device, Buf, 89, 41, Info.PDef);
    DrawNumber(Device, Buf, 89, 58, Info.Accuracy);
    DrawNumber(Device, Buf, 89, 75, Info.Critical);
    DrawNumber(Device, Buf, 89, 92, Info.PAtkSpeed);
    DrawNumber(Device, Buf, 206, 24, Info.MAtk);
    DrawNumber(Device, Buf, 206, 41, Info.MDef);
    DrawNumber(Device, Buf, 206, 58, Info.Evasion);
    DrawNumber(Device, Buf, 206, 75, Info.Speed);
    DrawNumber(Device, Buf, 206, 92, Info.CastSpeed);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// The Action page of both windows (one stock body, 0x10091680): the background; the icons are
// the grid's.
static FORCEINLINE INT PaintActionPage(NCWnd* Wnd, UTexture* BackTex, NCGDevice* Device)
{
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX, Wnd->ScreenY, Wnd->Width, Wnd->Height);
    if (BackTex)
        Device->DrawTexture(0, 0, Wnd->Width, Wnd->Height, 0, 0, Wnd->Width, Wnd->Height,
                            BackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}

// The icon grid of both Action pages (repeated in the stock OnCreates): 6 x 2 icons of 32
// pixels, 15 actions at most.
static FORCEINLINE NCActionItemWnd* CreateActionGrid(NCWnd* Parent)
{
    NCActionItemWnd* Grid = new NCActionItemWnd(6, 2, 15, 32);
    Grid->SetWindowRect(16, 29, 209, 69);
    Parent->CreateChild(Grid, NWS_VISIBLE | 0x20);   // 0x20: meaning unknown
    return Grid;
}

// The actions of one category of actionname-*.dat into a grid (two stock copies, one per
// page, that differ in the category only); an icon that is not found shows black.
static FORCEINLINE void LoadActionCategory(NCWnd* Wnd, NCActionItemWnd* Grid, INT Wanted)
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
        Info.Icon = Wnd->FindTexture(Info.IconName);
        if (!Info.Icon)
            Info.Icon = Wnd->FindTexture(TEXT("NWindow.BlackTexture"));
        if (Category == Wanted)
            Grid->AddItem(&Info);
    }
}

/*-----------------------------------------------------------------------------
    NCPetDetailStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10090ee0
NCPetDetailStatusWnd::NCPetDetailStatusWnd()
{
    BackTex = NULL;
    Pet.Name[0] = 0;
    Pet.UnknownB0 = 0;
}
NW_HOOK_CTOR(0x10090ee0, NCPetDetailStatusWnd)

// 0x10090f40
NW_HOOK_DTOR(0x10090f40, NCPetDetailStatusWnd)

// stock body shared with NControl's (0x10004e90)
INT NCPetDetailStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x1008ee70 (vslot 35)
INT NCPetDetailStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.Pet02"));
    return 1;
}
NW_HOOK_METHOD(0x1008ee70, NCPetDetailStatusWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCPetDetailStatusWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// 0x10090f90 (vslot 50) - shared with NCSummonedDetailStatusWnd
INT NCPetDetailStatusWnd::OnPaint(NCGDevice* Device)
{
    return PaintInfoPage(this, BackTex, Pet, Device);
}
NW_HOOK_METHOD(0x10090f90, NCPetDetailStatusWnd, OnPaint)

NW_CHECK_VTABLE(NCPetDetailStatusWnd, 0x100ebc10, 62)

/*-----------------------------------------------------------------------------
    NCPetActionWnd.
-----------------------------------------------------------------------------*/

// 0x100915e0
NCPetActionWnd::NCPetActionWnd()
{
}
NW_HOOK_CTOR(0x100915e0, NCPetActionWnd)

// 0x10091630
NW_HOOK_DTOR(0x10091630, NCPetActionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCPetActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10093940 (vslot 35)
INT NCPetActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.pet03"));
    Items = CreateActionGrid(this);
    LoadActions();
    return 1;
}
NW_HOOK_METHOD(0x10093940, NCPetActionWnd, OnCreate)

// 0x100bb540 (vslot 36) - one body for the windows whose only child is at +0x98: also
// NCSummonedActionWnd's, NCPetInventoryWnd's, NCGMActionWnd's, NCMagicSkillWnd's and
// NCTrainListWnd's
INT NCPetActionWnd::OnDestroy()
{
    DestroyChild(this, Items);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100bb540, NCPetActionWnd, OnDestroy)

// 0x10091680 (vslot 50) - also NCSummonedActionWnd's and NCPetInventoryWnd's
INT NCPetActionWnd::OnPaint(NCGDevice* Device)
{
    return PaintActionPage(this, BackTex, Device);
}
NW_HOOK_METHOD(0x10091680, NCPetActionWnd, OnPaint)

// 0x10091700 - the pet's actions
void NCPetActionWnd::LoadActions()
{
    LoadActionCategory(this, Items, ACTION_Pet);
}
NW_HOOK_METHOD(0x10091700, NCPetActionWnd, LoadActions)

NW_CHECK_VTABLE(NCPetActionWnd, 0x100ebd08, 62)

/*-----------------------------------------------------------------------------
    NCPetWnd.
-----------------------------------------------------------------------------*/

// 0x10091fb0 - the name button is left as allocated
NCPetWnd::NCPetWnd()
{
    BackTex = NULL;
    Pet.Name[0] = 0;
    Pet.UnknownB0 = 0;
    DetailWnd = NULL;
    ActionWnd = NULL;
    InventoryWnd = NULL;
    TabCtrl = NULL;
    bCanName = 1;
}
NW_HOOK_CTOR(0x10091fb0, NCPetWnd)

// 0x10092030
NW_HOOK_DTOR(0x10092030, NCPetWnd)

// 0x10090e10 (vslot 24)
INT NCPetWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && (INT)wParam == NID_PET_NAME)
        return OnName() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10090e10, NCPetWnd, WindowProc)

// 0x10092080 (vslot 35) - the pages start hidden; the tab control shows one
INT NCPetWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_PET_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.PetWnd.PetBack"));
    HPTex = FindTexture(TEXT("L2UI.StatusWnd.statusHP1"));
    MPTex = FindTexture(TEXT("L2UI.StatusWnd.statusMP1"));
    EXPTex = FindTexture(TEXT("L2UI.StatusWnd.statusEXP1"));
    WeightTex[0] = FindTexture(TEXT("L2UI.StatusWnd.statusWt1"));
    WeightTex[1] = FindTexture(TEXT("L2UI.StatusWnd.statusWt2"));
    WeightTex[2] = FindTexture(TEXT("L2UI.StatusWnd.statusWt3"));
    WeightTex[3] = FindTexture(TEXT("L2UI.StatusWnd.statusWt4"));
    FedTex = FindTexture(TEXT("L2UI.PetWnd.statusEat1"));

    DetailWnd = new NCPetDetailStatusWnd;
    DetailWnd->SetWindowRect(0, 104, 240, 109);
    CreateChild(DetailWnd, NWS_NOHITTEST);
    ActionWnd = new NCPetActionWnd;
    ActionWnd->SetWindowRect(0, 104, 240, 109);
    CreateChild(ActionWnd, NWS_NOHITTEST);
    InventoryWnd = new NCPetInventoryWnd;
    InventoryWnd->SetWindowRect(0, 104, 240, 109);
    CreateChild(InventoryWnd, NWS_NOHITTEST);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_PET_TABS;
    TabCtrl->SetWindowRect(15, 106, 207, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(0x1EB, DetailWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);                  // "Information"
    TabCtrl->AddTab(0x1EC, ActionWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);                  // "Action"
    TabCtrl->AddTab(0x1ED, InventoryWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);                  // "Items"

    NameButton = new NCButton(TEXT("L2UI.control.DefaultButton"),
                              TEXT("L2UI.control.DefaultButton_click"), 0);
    NameButton->ID = NID_PET_NAME;
    NameButton->SetWindowRect(156, 25, 67, 19);
    CreateChild(NameButton, NWS_VISIBLE);
    NameButton->SetTextID(0x1F1);                                       // "Name"
    return 1;
}
NW_HOOK_METHOD(0x10092080, NCPetWnd, OnCreate)

// 0x10092530 (vslot 36)
INT NCPetWnd::OnDestroy()
{
    DestroyChild(this, DetailWnd);
    DestroyChild(this, ActionWnd);
    DestroyChild(this, InventoryWnd);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, NameButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10092530, NCPetWnd, OnDestroy)

// 0x10092620 (vslot 50) - bars 84 (HP, MP, weight) and 189 (food, exp) pixels wide, the
// weight bar not clamped and textured by how loaded the pet is (as the status window). The
// food and exp percentages divide without a check.
INT NCPetWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 83, 0, 0, 240, 83, BackTex);
    Pet = GConsole->GetPetInfo();
    INT HPW = 0;
    INT MPW = 0;
    INT WeightW = 0;
    INT FedW = 0;
    INT EXPW = 0;
    if (Pet.MaxHP)
        HPW = Pet.HP * 84 / Pet.MaxHP;
    if (Pet.MaxMP)
        MPW = Pet.MP * 84 / Pet.MaxMP;
    if (Pet.MaxLoad)
        WeightW = Pet.Load * 84 / Pet.MaxLoad;
    if (Pet.MaxFed)
        FedW = Pet.Fed * 189 / Pet.MaxFed;
    if (Pet.NextExp)
        EXPW = (INT)((DOUBLE)(Pet.Exp - Pet.LevelExp) * 189.0 / (Pet.NextExp - Pet.LevelExp));
    DrawBar(Device, 34, 75, HPW, 84, HPTex);
    DrawBar(Device, 139, 75, MPW, 84, MPTex);
    DrawBar(Device, 34, 46, FedW, 189, FedTex);
    DrawBar(Device, 34, 62, EXPW, 189, EXPTex);

    FLOAT WeightPct = 0.f;
    if (WeightW > 0)
    {
        // the first test uses the unrounded quotient (fst, then fcomp); the others reload the
        // stored float, as the stock code does
        DOUBLE Pct = (DOUBLE)Pet.Load / Pet.MaxLoad * 100.0;
        WeightPct = (FLOAT)Pct;
        if (Pct <= 50.0)
            Device->DrawTexture(139, 89, WeightW, 10, 0, 0, WeightW, 10, WeightTex[0]);
        else if (WeightPct <= 66.66)
            Device->DrawTexture(139, 89, WeightW, 10, 0, 0, WeightW, 10, WeightTex[1]);
        else if (WeightPct > 66.66 && WeightPct <= 80.f)
            Device->DrawTexture(139, 89, WeightW, 10, 0, 0, WeightW, 10, WeightTex[2]);
        else if (WeightPct > 80.f)
            Device->DrawTexture(139, 89, WeightW, 10, 0, 0, WeightW, 10, WeightTex[3]);
    }

    TCHAR Buf[64];
    SIZE Size;
    swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"), (DOUBLE)Pet.Fed * 100.0 / Pet.MaxFed, TEXT("%"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 128 - Size.cx / 2, 47, 0xFFB4B4B4, Buf);
    swprintf(Buf, ARRAY_COUNT(Buf), TEXT("%.2f%s"),
             (DOUBLE)(Pet.Exp - Pet.LevelExp) * 100.0 / (Pet.NextExp - Pet.LevelExp), TEXT("%"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 128 - Size.cx / 2, 63, 0xFFB4B4B4, Buf);
    DrawHPMP(Device, Buf, 76, Pet);
    DrawNumber(Device, Buf, 76, 89, Pet.SP);
    appSprintf(Buf, TEXT("%.2f%s"), WeightPct, TEXT("%"));
    NCGDevice::GetTextExtent(&Size, Buf, 0, 0, 0, 0, 0);
    Device->DrawSpecialDigit(1, 181 - Size.cx / 2, 90, 0xFFFFFFFF, Buf);
    DrawLevelName(Device, Pet);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x10092620, NCPetWnd, OnPaint)

// 0x10092d10 (vslot 42) - the Name button only while the pet can still be named
void NCPetWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    if (bCanName)
        NameButton->ShowWindow();
    else
        NameButton->HideWindow();
    RestoreFromIcon(this);
}
NW_HOOK_METHOD(0x10092d10, NCPetWnd, ShowWindow)

// 0x10092d90 (vslot 62) - asks for the pet's name (systemmsg 535); the dialog box sends it
INT NCPetWnd::OnName()
{
    NDialogInfo Info;
    Info.ID = 0x35;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.Type = 0;
    Info.Owner = Entry;
    Info.SetSystemMessage(0x217);
    GConsole->ShowDialog(&Info);
    return 1;
}
NW_HOOK_METHOD(0x10092d90, NCPetWnd, OnName)

// 0x10090e80 - once named, the button goes away
void NCPetWnd::SetCanName(UBOOL bCan)
{
    bCanName = bCan;
    if (!bCan)
        NameButton->HideWindow();
}
NW_HOOK_METHOD(0x10090e80, NCPetWnd, SetCanName)

// 0x10090ea0 - the Items page's list emptied
void NCPetWnd::ClearInventory()
{
    InventoryWnd->ClearItems();
}
NW_HOOK_METHOD(0x10090ea0, NCPetWnd, ClearInventory)

// 0x100940b0 - from the console (NConsoleWnd::AddPetInventoryItem)
UBOOL NCPetWnd::AddInventoryItem(NItemInfo* Info)
{
    InventoryWnd->AddItem(Info);
    return 1;
}
NW_HOOK_METHOD(0x100940b0, NCPetWnd, AddInventoryItem)

// 0x100940d0 - from the console: 1 added, 2 changed, 3 removed
UBOOL NCPetWnd::UpdateInventoryItem(INT Op, NItemInfo* Info)
{
    InventoryWnd->UpdateItem(Op, Info);
    return 1;
}
NW_HOOK_METHOD(0x100940d0, NCPetWnd, UpdateInventoryItem)

NW_CHECK_VTABLE(NCPetWnd, 0x100ebef8, 63)

/*-----------------------------------------------------------------------------
    NCSummonedDetailStatusWnd.
-----------------------------------------------------------------------------*/

// 0x10092e00
NCSummonedDetailStatusWnd::NCSummonedDetailStatusWnd()
{
    BackTex = NULL;
    Summon.Name[0] = 0;
    Summon.UnknownB0 = 0;
}
NW_HOOK_CTOR(0x10092e00, NCSummonedDetailStatusWnd)

// 0x10092e60
NW_HOOK_DTOR(0x10092e60, NCSummonedDetailStatusWnd)

// stock body shared with NControl's (0x10004e90)
INT NCSummonedDetailStatusWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10090eb0 (vslot 35)
INT NCSummonedDetailStatusWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.Summon02"));
    return 1;
}
NW_HOOK_METHOD(0x10090eb0, NCSummonedDetailStatusWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCSummonedDetailStatusWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// stock body shared with NCPetDetailStatusWnd's (0x10090f90)
INT NCSummonedDetailStatusWnd::OnPaint(NCGDevice* Device)
{
    return PaintInfoPage(this, BackTex, Summon, Device);
}

NW_CHECK_VTABLE(NCSummonedDetailStatusWnd, 0x100ec120, 62)

/*-----------------------------------------------------------------------------
    NCSummonedActionWnd.
-----------------------------------------------------------------------------*/

// 0x10092eb0
NCSummonedActionWnd::NCSummonedActionWnd()
{
}
NW_HOOK_CTOR(0x10092eb0, NCSummonedActionWnd)

// 0x10092f00
NW_HOOK_DTOR(0x10092f00, NCSummonedActionWnd)

// stock body shared with NControl's (0x10004e90)
INT NCSummonedActionWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x100940f0 (vslot 35)
INT NCSummonedActionWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.PetWnd.Summon03"));
    Items = CreateActionGrid(this);
    LoadActions();
    return 1;
}
NW_HOOK_METHOD(0x100940f0, NCSummonedActionWnd, OnCreate)

// stock body shared with NCPetActionWnd's (0x100bb540)
INT NCSummonedActionWnd::OnDestroy()
{
    DestroyChild(this, Items);
    NCWnd::OnDestroy();
    return 1;
}

// stock body shared with NCPetActionWnd's (0x10091680)
INT NCSummonedActionWnd::OnPaint(NCGDevice* Device)
{
    return PaintActionPage(this, BackTex, Device);
}

// 0x10092f50 - the summon's actions
void NCSummonedActionWnd::LoadActions()
{
    LoadActionCategory(this, Items, ACTION_Summon);
}
NW_HOOK_METHOD(0x10092f50, NCSummonedActionWnd, LoadActions)

NW_CHECK_VTABLE(NCSummonedActionWnd, 0x100ec218, 62)

/*-----------------------------------------------------------------------------
    NCSummonedWnd.
-----------------------------------------------------------------------------*/

// 0x10093080
NCSummonedWnd::NCSummonedWnd()
{
    BackTex = NULL;
    DetailWnd = NULL;
    ActionWnd = NULL;
    TabCtrl = NULL;
    Summon.Name[0] = 0;
    Summon.UnknownB0 = 0;
}
NW_HOOK_CTOR(0x10093080, NCSummonedWnd)

// 0x100930f0
NW_HOOK_DTOR(0x100930f0, NCSummonedWnd)

// stock body shared with NControl's (0x10004e90)
INT NCSummonedWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10093140 (vslot 35) - the tab labels are the texts, not their ids as in the pet window
INT NCSummonedWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameCtrl->ID = NID_SUMMON_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.PetWnd.Summon01"));
    HPTex = FindTexture(TEXT("L2UI.StatusWnd.statusHP1"));
    MPTex = FindTexture(TEXT("L2UI.StatusWnd.statusMP1"));

    DetailWnd = new NCSummonedDetailStatusWnd;
    DetailWnd->SetWindowRect(0, 104, 240, 109);
    CreateChild(DetailWnd, NWS_NOHITTEST);
    ActionWnd = new NCSummonedActionWnd;
    ActionWnd->SetWindowRect(0, 104, 240, 109);
    CreateChild(ActionWnd, NWS_NOHITTEST);

    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_PET_TABS;
    TabCtrl->SetWindowRect(15, 106, 138, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(*GL2GameData.SysString[491], DetailWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);                  // "Information"
    TabCtrl->AddTab(*GL2GameData.SysString[492], ActionWnd, 69, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);                  // "Action"
    return 1;
}
NW_HOOK_METHOD(0x10093140, NCSummonedWnd, OnCreate)

// 0x10093440 (vslot 36)
INT NCSummonedWnd::OnDestroy()
{
    DestroyChild(this, DetailWnd);
    DestroyChild(this, ActionWnd);
    DestroyChild(this, TabCtrl);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10093440, NCSummonedWnd, OnDestroy)

// 0x100934e0 (vslot 50) - bars 84 pixels wide
INT NCSummonedWnd::OnPaint(NCGDevice* Device)
{
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 21, 240, 83, 0, 0, 240, 83, BackTex);
    Summon = GConsole->GetPetInfo();
    INT HPW = 0;
    INT MPW = 0;
    if (Summon.MaxHP)
        HPW = Summon.HP * 84 / Summon.MaxHP;
    if (Summon.MaxMP)
        MPW = Summon.MP * 84 / Summon.MaxMP;
    DrawBar(Device, 34, 75, HPW, 84, HPTex);
    DrawBar(Device, 139, 75, MPW, 84, MPTex);
    TCHAR Buf[64];
    DrawHPMP(Device, Buf, 76, Summon);
    DrawLevelName(Device, Summon);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
}
NW_HOOK_METHOD(0x100934e0, NCSummonedWnd, OnPaint)

// 0x10093870 (vslot 42)
void NCSummonedWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    RestoreFromIcon(this);
}
NW_HOOK_METHOD(0x10093870, NCSummonedWnd, ShowWindow)

NW_CHECK_VTABLE(NCSummonedWnd, 0x100ec310, 62)

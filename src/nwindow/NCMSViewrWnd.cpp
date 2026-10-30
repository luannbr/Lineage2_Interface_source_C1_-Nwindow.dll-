/*=============================================================================
    NCMSViewrWnd.cpp - the skill viewer, its character selection and skill grids. Stock:
    0x1006f0c0 .. 0x1006f16f, 0x1006f180 .. 0x10070b6d, 0x10070b80 .. 0x10071fb8 (the
    selection window's OnDestroy is NCTrainWnd's body, the grids' WindowProc
    NCNpcView_InvenItemWnd's: unhooked copies here; the TArray<AActor*>::AddItem instance
    0x10070b40 is left out).
=============================================================================*/
#include "nwindow/NCMSViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NString.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

// UObject::StaticLoadClass flags (UE2)
enum { LOAD_NoFail = 0x1 };

// sysstring texts (names ours): the selection window's labels, the search's tabs and check
// boxes, "search"
enum
{
    STR_MSViewTitle = 157,
    STR_MSViewLabel1 = 50,
    STR_MSViewLabel2 = 88,
    STR_MSViewLabel3 = 90,
    STR_MSViewLabel4 = 91,
    STR_MSViewLabel5 = 92,
    STR_MSViewLabel6 = 101,
    STR_MSViewRace0 = 170,               // the character names: the races (170 .. 174),
    STR_MSViewFighter = 175,             //   then fighter or mystic, male or female
    STR_MSViewMale = 177,
    STR_MSViewActive = 120,
    STR_MSViewPassive = 121,
    STR_MSViewMagic = 122,
    STR_MSViewSearch = 420,
};

// The characters the viewer offers (race, sex, class id; 18 entries at 0x101114c0 for the 18
// names of AddAllCharacter, which reads 19: the last one is whatever follows the table).
struct NMSViewCharacter
{
    INT Race;
    INT Sex;
    INT ClassID;
};
#if NW_HOOK_MODE
#define GMSViewCharacters (reinterpret_cast<NMSViewCharacter*>(::nw::Addr(0x101114c0)))
#else
extern NMSViewCharacter GMSViewCharacters[];
#endif

template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

/*-----------------------------------------------------------------------------
    NCMSView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x10070cf0
NCMSView_InvenItemWnd::NCMSView_InvenItemWnd()
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR(0x10070cf0, NCMSView_InvenItemWnd)

// 0x10070d90 - the viewer's are 6 x 7 icons of 32 pixels, 80 at first (AddAllItem raises it)
NCMSView_InvenItemWnd::NCMSView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                             INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR_ARGS(0x10070d90, NCMSView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x10070d40
NW_HOOK_DTOR(0x10070d40, NCMSView_InvenItemWnd)

// stock body shared with NCNpcView_InvenItemWnd's (0x1007a1a0)
INT NCMSView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}

// 0x10070b80 (vslot 58) - with a character picked, the skill under the mouse (level 1) is
// cast by the player's pawn at the controller's target (+0x634): the pawn's controller stops
// moving (StopMove) first, the magic info set, the target added to the pawn's list (+0x59C); when the skill
// process starts, the pawn's +0x624 = 5 and +0x630 the target
INT NCMSView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCMSView_InvenItemWnd::OnRButtonDown);
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        INT Character = MSViewer()->CharSelectWnd->Selected;
        if (Character >= 0 && Character <= 19)
        {
            FL2MagicSkillData* Data = GL2GameData.GetMSData(Items(Selected)->ID, 1);
            if (Data)
            {
                APlayerController* Controller = ViewActor();
                APawn* Pawn = Controller->Pawn;
                AActor* Target = Controller->Target;
                Pawn->Controller->StopMove();
                Pawn->SetMagicInfo(Data->SkillID, Data->Level, Data->HitTime, Data->HitTime, Target,
                                   1.0f);
                Pawn->MagicTargets.AddItem(Target);
                if (Pawn->InitSkillProcess())
                {
                    Pawn->ActionType = 5;
                    Pawn->ActionTarget = Target;
                }
            }
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10070b80, NCMSView_InvenItemWnd, OnRButtonDown)

// 0x1006f0c0 (vslot 68) - as the right button
INT NCMSView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCMSView_InvenItemWnd::OnLButtonDblClick);
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    UnselectAll();
    OnRButtonDown(wParam, lParam);
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006f0c0, NCMSView_InvenItemWnd, OnLButtonDblClick)

/*-----------------------------------------------------------------------------
    NCMSView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x10070100
NCMSView_CharacterSelectWnd::NCMSView_CharacterSelectWnd()
{
    TitleTex = NULL;
    BackTex = NULL;
    Unknown67Tex = NULL;
    Unknown68Tex = NULL;
    Unknown69Tex = NULL;
    UnknownB0 = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x10070100, NCMSView_CharacterSelectWnd)

// 0x10070180
NW_HOOK_DTOR(0x10070180, NCMSView_CharacterSelectWnd)

// 0x1006fc10 (vslot 24)
INT NCMSView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MSVIEW_SELECT_NAME)
            return OnNameComboBox(lParam) != 0;
        if (wParam == NID_MSVIEW_SELECT_ANIM)
            return OnAniNameComboBox(lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1006fc10, NCMSView_CharacterSelectWnd, WindowProc)

// 0x100701d0 (vslot 35) - the two combo boxes under the title strip
INT NCMSView_CharacterSelectWnd::OnCreate()
{
    guard(NCMSView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_MSVIEW_SELECT_FRAME;
    FrameCtrl->ID = NID_MSVIEW_SELECT_FRAME;
    FrameCtrl->SetWindowRect(2, 3, Width - 6, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    Unknown67Tex = FindTexture(TEXT("sek.cbui67"));
    Unknown68Tex = FindTexture(TEXT("sek.cbui68"));
    Unknown69Tex = FindTexture(TEXT("sek.cbui69"));
    NameCombo = new NCComboBox;
    NameCombo->ID = NID_MSVIEW_SELECT_NAME;
    NameCombo->SetWindowRect(41, 28, 129, 18);
    CreateChild(NameCombo, NWS_VISIBLE);
    AniCombo = new NCComboBox;
    AniCombo->ID = NID_MSVIEW_SELECT_ANIM;
    AniCombo->SetWindowRect(41, 52, 129, 18);
    CreateChild(AniCombo, NWS_VISIBLE);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100701d0, NCMSView_CharacterSelectWnd, OnCreate)

// stock body shared with NCTrainWnd's (0x10089310): the combo boxes (the title bar is left to
// NCWnd::OnDestroy)
INT NCMSView_CharacterSelectWnd::OnDestroy()
{
    DestroyChild(this, NameCombo);
    DestroyChild(this, AniCombo);
    NCWnd::OnDestroy();
    return 1;
}

// 0x10070470 (vslot 50) - the background and the title strip, the title and six labels
INT NCMSView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCMSView_CharacterSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    if (TitleTex)
        Device->DrawTexture(2, 3, Width - 4, 20, 0, 0, Width - 4, 20, TitleTex);
    Device->DrawText(23, 8, 0xFFFFFFFF, *GL2GameData.SysString[STR_MSViewTitle], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(8, 32, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel1], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(8, 72, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel2], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(8, 100, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel3], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(107, 100, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel4], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(8, 114, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel5], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->DrawText(6, 128, 0xFFA3A3A3, *GL2GameData.SysString[STR_MSViewLabel6], 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10070470, NCMSView_CharacterSelectWnd, OnPaint)

// 0x100706a0 (vslot 62) - another character (0..19, not the one shown) becomes the player's
// pawn: a player controller spawned for it, the pawn class loaded by the character's class
// name, the pawn spawned 30 units above the player's, which it replaces (the old one
// destroyed, the controller kept the viewport's); then its idle animation looped and its
// animations into the combo box (the animation set is not checked for NULL here)
INT NCMSView_CharacterSelectWnd::OnNameComboBox(INT Index)
{
    guard(NCMSView_CharacterSelectWnd::OnNameComboBox);
    if (Index > 19 || Index == Selected)
        return 1;
    FName ControllerClassName = NAME_None;
    ULevel* Level = GameLevel();
    APlayerController* Viewer = ViewActor();
    ControllerClassName = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* Controller = (AController*)Level->SpawnActor(
        StaticLoadClass(AController::StaticClass(), NULL, *ControllerClassName, NULL, LOAD_NoFail,
                        NULL));
    if (!Controller)
        appFailAssert("pController", "Src\\NMSViewerWnd.cpp", 959);
    User Character = MSViewer()->Characters[Index];
    Controller->PawnClass = StaticLoadClass(APawn::StaticClass(), NULL,
                                            Character.GetClassNamePointer(), NULL, 0, NULL);
    if (!Controller->PawnClass)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    Location.Z += 30.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(Controller->PawnClass, NAME_None, Location,
                                            FRotator(0, 0, 0), NULL, 1, 1, Controller, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(ViewActor()->Pawn, 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = Viewer;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Selected = Index;
    MSViewer()->Characters[Index].Pawn = Pawn;
    MSViewer()->Characters[Selected].SetPawnResource();
    // the idle animation of the pawn's weapon
    Pawn->PlayAnim(0, Pawn->GetWaitAnimName(), 1.0f, 0.f, 1, 0);
    APawn* ViewPawn = ViewActor()->Pawn;
    if (ViewPawn)
    {
        UMesh* Mesh = ViewPawn->Mesh;
        if (Mesh)
        {
            Mesh->MeshGetInstance(ViewPawn);
            UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
            FName Sequence = NAME_None;
            INT Num = Anim->AnimSeqs.Num();
            AniCombo->ClearItems();
            for (INT i = 0; i < Num; i++)
            {
                Sequence = Anim->AnimSeqs(i).Name;
                AniCombo->AddItem(*Sequence);
            }
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100706a0, NCMSView_CharacterSelectWnd, OnNameComboBox)

// 0x10070a40 (vslot 63) - the animation picked plays looped on the player's pawn. Items(Index)
// is read up to Index == Num (">=" in the stock code).
INT NCMSView_CharacterSelectWnd::OnAniNameComboBox(INT Index)
{
    guard(NCMSView_CharacterSelectWnd::OnAniNameComboBox);
    if (AniCombo->Items.Num() >= Index)
    {
        TCHAR* Name = AniCombo->Items(Index)->GetAt(0);
        APawn* Pawn = ViewActor()->Pawn;
        Pawn->AnimFlags |= 0x10;
        Pawn->Unknown13D4 &= ~1;
        Pawn->PlayAnim(0, FName(Name, FNAME_Add), 1.0f, 0.f, 1, 0);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10070a40, NCMSView_CharacterSelectWnd, OnAniNameComboBox)

/*-----------------------------------------------------------------------------
    NCMSViewrWnd.
-----------------------------------------------------------------------------*/

// 0x1006fca0 - the character records built; the search controls and the selection window are
// left as allocated
NCMSViewrWnd::NCMSViewrWnd()
{
    TabCtrl = NULL;
    BackTex = NULL;
    HtmlBackTex = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    ItemWnd[2] = NULL;
    ItemWnd[3] = NULL;
    Level = NULL;
}
NW_HOOK_CTOR(0x1006fca0, NCMSViewrWnd)

// 0x1006fd70
NW_HOOK_DTOR(0x1006fd70, NCMSViewrWnd)

// 0x1006f180 (vslot 24)
INT NCMSViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_MSVIEW_SEARCHNAME)
            return OnSearchName() != 0;
        if (wParam == NID_MSVIEW_SEARCHID)
            return OnSearchID() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1006f180, NCMSViewrWnd, WindowProc)

// 0x10071580 (vslot 35) - the search by name and by id, the kinds the search lists (magic and
// active checked), the four skill grids under their tabs, the selection window (top-level,
// its first character shown); both lists loaded. NCWnd::OnCreate is not called.
INT NCMSViewrWnd::OnCreate()
{
    guard(NCMSViewrWnd::OnCreate);
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_MSVIEW_FRAME;
    FrameCtrl->ID = NID_MSVIEW_FRAME;
    FrameCtrl->SetWindowRect(0, 0, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    HtmlBackTex = FindTexture(TEXT("L2UI.etcwnd.NPCHtml_BACK"));
    BackTex = FindTexture(TEXT("L2UI.SkillWnd.SkillBack"));
    NameEdit = new NCEditBox(1, NULL, 1, 64);
    NameEdit->ID = NID_MSVIEW_NAMEEDIT;
    NameEdit->SetWindowRect(55, 30, 65, 16);
    CreateChild(NameEdit, NWS_VISIBLE);
    IDEdit = new NCEditBox(1, NULL, 1, 64);
    IDEdit->ID = NID_MSVIEW_IDEDIT;
    IDEdit->SetWindowRect(55, 55, 65, 16);
    CreateChild(IDEdit, NWS_VISIBLE);
    MagicCheck = new NCCheckBox(STR_MSViewMagic, 0, 0);
    MagicCheck->SetWindowRect(15, 80, 65, 16);
    CreateChild(MagicCheck, NWS_VISIBLE);
    if (!MagicCheck->bDisabled)
        MagicCheck->bChecked = 1;
    ActiveCheck = new NCCheckBox(STR_MSViewActive, 0, 0);
    ActiveCheck->SetWindowRect(80, 80, 65, 16);
    CreateChild(ActiveCheck, NWS_VISIBLE);
    if (!ActiveCheck->bDisabled)
        ActiveCheck->bChecked = 1;
    PassiveCheck = new NCCheckBox(STR_MSViewPassive, 0, 0);
    PassiveCheck->SetWindowRect(145, 80, 65, 16);
    CreateChild(PassiveCheck, NWS_VISIBLE);
    if (!PassiveCheck->bDisabled)
        PassiveCheck->bChecked = 0;
    SearchNameButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                    TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchNameButton->ID = NID_MSVIEW_SEARCHNAME;
    SearchNameButton->SetWindowRect(140, 30, 32, 16);
    CreateChild(SearchNameButton, NWS_VISIBLE);
    SearchNameButton->SetTextID(STR_MSViewSearch);
    SearchIDButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                  TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchIDButton->ID = NID_MSVIEW_SEARCHID;
    SearchIDButton->SetWindowRect(140, 55, 32, 16);
    CreateChild(SearchIDButton, NWS_VISIBLE);
    SearchIDButton->SetTextID(STR_MSViewSearch);
    for (INT Tab = 0; Tab < 4; Tab++)
    {
        ItemWnd[Tab] = new NCMSView_InvenItemWnd(6, 7, 80, 32);
        ItemWnd[Tab]->SetWindowRect(16, 133, 220, 255);
        CreateChild(ItemWnd[Tab], NWS_VISIBLE | 0x20);
    }
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_MSVIEW_TAB;
    TabCtrl->SetWindowRect(15, 102, 138, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(STR_MSViewMagic, ItemWnd[0], 35, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_MSViewActive, ItemWnd[1], 35, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_MSViewPassive, ItemWnd[2], 35, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    TabCtrl->AddTab(STR_MSViewSearch, ItemWnd[3], 33, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    for (INT i = 0; i < 19; i++)
        Characters[i].Pawn = NULL;
    CharSelectWnd = new NCMSView_CharacterSelectWnd;
    CharSelectWnd->SetWindowRect(34, 34, 208, 161);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE | NWS_NOHITTEST);
    AddAllItem();
    AddAllCharacter();
    Level = GameLevel();
    CharSelectWnd->OnNameComboBox(0);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10071580, NCMSViewrWnd, OnCreate)

// 0x1006ff70 (vslot 36) - the selection window deleted (top-level, not cleared); the search
// controls are left to NCWnd::OnDestroy
INT NCMSViewrWnd::OnDestroy()
{
    guard(NCMSViewrWnd::OnDestroy);
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, ItemWnd[2]);
    DestroyChild(this, ItemWnd[3]);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, FrameCtrl);
    delete CharSelectWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006ff70, NCMSViewrWnd, OnDestroy)

// 0x1006fad0 (vslot 42) - with the selection window
void NCMSViewrWnd::ShowWindow()
{
    guard(NCMSViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x1006fad0, NCMSViewrWnd, ShowWindow)

// 0x1006fa40 (vslot 43)
void NCMSViewrWnd::HideWindow()
{
    guard(NCMSViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x1006fa40, NCMSViewrWnd, HideWindow)

// 0x1006fdc0 (vslot 50) - the grids' background, the search's, and the search lines' labels
// ("skill name:", "skill ID:" in Korean) centered in 55 x 16 before them
INT NCMSViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCMSViewrWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 100, 240, 300, 0, 0, 240, 300, BackTex);
    if (HtmlBackTex)
        Device->DrawStretchedTex(0, 0, 240, 100, 0, 0, 300, 60, HtmlBackTex);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, TEXT("\xC2A4\xD0AC\xBA85:"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 30, 0xFFFFFFFF,
                     TEXT("\xC2A4\xD0AC\xBA85:"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("\xC2A4\xD0AC") TEXT("ID:"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 55, 0xFFFFFFFF,
                     TEXT("\xC2A4\xD0AC") TEXT("ID:"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006fdc0, NCMSViewrWnd, OnPaint)

// the kind of a skill for the search (Type: its name in Korean): 0 when the search leaves it
// out; toggles (and other kinds) keep the Type of the skill before them
static FORCEINLINE UBOOL SearchKind(NCMSViewrWnd* Wnd, FL2MagicSkillData* Data, TCHAR* Type)
{
    if (Data->bMagic)
    {
        if (!Wnd->MagicCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\xB9C8\xBC95"));                          // "magic"
    }
    else if (Data->OperateType >= 0 && Data->OperateType <= 1)
    {
        if (!Wnd->ActiveCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\xC561\xD2F0\xBE0C"));                    // "active"
    }
    else if (Data->OperateType == 2)
    {
        if (!Wnd->PassiveCheck->bChecked)
            return 0;
        appStrcpy(Type, TEXT("\xD328\xC2DC\xBE0C"));                    // "passive"
    }
    return 1;
}

// A skill found by a search into the fourth grid (level 1 only): the icon of its level (or
// "NWindow.Darkyellow"), "ID = n\nTYPE = kind" as its description.
static FORCEINLINE void AddFoundSkill(NCMSViewrWnd* Wnd, INT SkillID, INT Level, const TCHAR* Type)
{
    NItemInfo Info;
    if (Level != 1)
        return;
    wcscpy(Info.IconName, GConsole->GetSkillIconName(SkillID, Level));
    Info.Icon = Wnd->FindTexture(GConsole->GetSkillIconName(SkillID, Level));
    if (GConsole->GetSkillIconName(SkillID, Level))
        Info.Icon = Wnd->FindTexture(GConsole->GetSkillIconName(SkillID, Level));
    if (!Info.Icon)
        Info.Icon = Wnd->FindTexture(TEXT("NWindow.Darkyellow"));
    if (GConsole->GetSkillName(SkillID, 1))
    {
        appSprintf(Info.Description, TEXT("ID = %d\nTYPE = %s"), SkillID, Type);
        appStrcpy(Info.Name, GConsole->GetSkillName(SkillID, 1));
    }
    Info.ID = SkillID;
    Info.BodyPart = 3;
    Wnd->AddItem(3, &Info);
    Wnd->ItemWnd[3]->MaxItems++;
}

// 0x10071030 (vslot 62) - the skills whose name holds the text, of the kinds checked
INT NCMSViewrWnd::OnSearchName()
{
    const TCHAR* Text = NameEdit->CommitText();
    if (Text && appStrlen(Text) > 0)
    {
        ItemWnd[3]->Clear();
        NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
        TCHAR Type[20];
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            FL2MagicSkillData* Data = Map->Pairs(i).Value;
            if (!Data || !*(INT*)&Data->Name)
                continue;
            INT SkillID = Data->SkillID;
            INT Level = Data->Level;
            if (!appStrstr(*Data->Name, Text) || !SearchKind(this, Data, Type))
                continue;
            AddFoundSkill(this, SkillID, Level, Type);
        }
        TabCtrl->SetTab(3);
    }
    return 1;
}
NW_HOOK_METHOD(0x10071030, NCMSViewrWnd, OnSearchName)

// 0x100712e0 (vslot 63) - the skill of the id typed, of the kinds checked
INT NCMSViewrWnd::OnSearchID()
{
    const TCHAR* Text = IDEdit->CommitText();
    if (Text && appStrlen(Text) > 0)
    {
        INT ID = appAtoi(Text);
        ItemWnd[3]->Clear();
        NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
        TCHAR Type[20];
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            FL2MagicSkillData* Data = Map->Pairs(i).Value;
            if (!Data)
                continue;
            INT SkillID = Data->SkillID;
            INT Level = Data->Level;
            if (SkillID != ID || !SearchKind(this, Data, Type))
                continue;
            AddFoundSkill(this, SkillID, Level, Type);
        }
        TabCtrl->SetTab(3);
    }
    return 1;
}
NW_HOOK_METHOD(0x100712e0, NCMSViewrWnd, OnSearchID)

// 0x1006fb70 - into a tab's grid, unless the info's +0xC8C is set
INT NCMSViewrWnd::AddItem(INT Tab, NItemInfo* Info)
{
    guard(NCMSViewrWnd::AddItem);
    if (!Info->UnknownC8C && Tab >= 0 && Tab < 4 && ItemWnd[Tab])
        ItemWnd[Tab]->AddItem(Info);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1006fb70, NCMSViewrWnd, AddItem)

// 0x1006f210 - the characters: 19 records cleared and given the race, sex and class of the
// table (see GMSViewCharacters), then 18 names "race sex class" in the selection window
void NCMSViewrWnd::AddAllCharacter()
{
    guard(NCMSViewrWnd::AddAllCharacter);
    for (INT i = 0; i < 19; i++)
    {
        memset(&Characters[i], 0, sizeof(User));
        Characters[i].Race = GMSViewCharacters[i].Race;
        Characters[i].Sex = GMSViewCharacters[i].Sex;
        Characters[i].ClassID = GMSViewCharacters[i].ClassID;
    }
    // (race, male / female, fighter / mystic) of each name, in the stock order
    static const BYTE Names[18][3] =
    {
        { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1, 1 },
        { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 1 }, { 1, 1, 1 },
        { 2, 0, 0 }, { 2, 1, 0 }, { 2, 0, 1 }, { 2, 1, 1 },
        { 3, 0, 0 }, { 3, 1, 0 }, { 3, 0, 1 }, { 3, 1, 1 },
        { 4, 0, 0 }, { 4, 1, 0 },
    };
    TCHAR Name[104];
    for (INT n = 0; n < 18; n++)
    {
        appSprintf(Name, TEXT("%s%s%s"), *GL2GameData.SysString[STR_MSViewRace0 + Names[n][0]],
                   *GL2GameData.SysString[STR_MSViewMale + Names[n][1]],
                   *GL2GameData.SysString[STR_MSViewFighter + Names[n][2]]);
        CharSelectWnd->NameCombo->AddItem(Name);
    }
    unguard;
}
NW_HOOK_METHOD(0x1006f210, NCMSViewrWnd, AddAllCharacter)

// 0x10070dd0 - the level-1 skills into the first three grids by kind (magic, active, passive
// and toggles), their capacity raised with each; the icon of their level (its name copied
// without a NULL check) or "NWindow.Darkyellow", "ID = n" as their description
void NCMSViewrWnd::AddAllItem()
{
    guard(NCMSViewrWnd::AddAllItem);
    NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        FL2MagicSkillData* Data = Map->Pairs(i).Value;
        if (!Data)
            continue;
        INT SkillID = Data->SkillID;
        INT Level = Data->Level;
        NItemInfo Info;
        if (Level != 1)
            continue;
        wcscpy(Info.IconName, GConsole->GetSkillIconName(SkillID, Level));
        Info.Icon = FindTexture(GConsole->GetSkillIconName(SkillID, Level));
        if (GConsole->GetSkillIconName(SkillID, 1))
            Info.Icon = FindTexture(GConsole->GetSkillIconName(SkillID, 1));
        if (!Info.Icon)
            Info.Icon = FindTexture(TEXT("NWindow.Darkyellow"));
        if (GConsole->GetSkillName(SkillID, 1))
        {
            swprintf(Info.Description, TEXT("ID = %d"), SkillID);
            wcscpy(Info.Name, GConsole->GetSkillName(SkillID, 1));
        }
        Info.ID = SkillID;
        INT Tab;
        if (Data->bMagic)
            Tab = 0;
        else if (Data->OperateType != 0 && Data->OperateType != 1)
            Tab = 2;
        else
            Tab = 1;
        Info.BodyPart = Tab;
        AddItem(Tab, &Info);
        ItemWnd[Tab]->MaxItems++;
    }
    unguard;
}
NW_HOOK_METHOD(0x10070dd0, NCMSViewrWnd, AddAllItem)

NW_CHECK_VTABLE(NCMSView_InvenItemWnd, 0x100e6ea8, 70)
NW_CHECK_VTABLE(NCMSView_CharacterSelectWnd, 0x100e6b30, 64)
NW_CHECK_VTABLE(NCMSViewrWnd, 0x100e69b8, 64)

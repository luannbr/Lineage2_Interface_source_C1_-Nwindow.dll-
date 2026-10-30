/*=============================================================================
    NCPawnView_CharacterSelectWnd.cpp - the pawn viewer's character window and its
    animation list. Stock: 0x10088140 .. 0x100881ec, 0x10088450 .. 0x10088592,
    0x10089140 .. 0x100892cf, 0x10089f10 .. 0x1008a007, 0x1008c3c0 .. 0x1008c719,
    0x1008cd20 .. 0x1008d13f, 0x1008d960 .. 0x1008dc73; OnDestroy and five of the list's
    slots are bodies shared with other classes.
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

// NWM_COMMAND ids (names ours)
enum
{
    NID_CHARSELECT_FRAME = (INT)0xAFF12011,
    NID_CHARSELECT_BYACTION = 0x101,
    NID_CHARSELECT_NAME = 0x1F04,
    NID_CHARSELECT_ANINAME = 0x1F05,   // handled, no control sends it
};

// UpdateAnimList's groups: the animations whose name holds these come first, in this order
static const TCHAR GAnimActionKeys[5][10] = {
    TEXT("run"), TEXT("walk"), TEXT("atk"), TEXT("wait"), TEXT("social")};
static const TCHAR GAnimWeaponKeys[6][10] = {
    TEXT("1hs"), TEXT("2hs"), TEXT("dual"), TEXT("pole"), TEXT("bow"), TEXT("hand")};

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

/*-----------------------------------------------------------------------------
    NCPawnView_CharacterSelect_List.
-----------------------------------------------------------------------------*/

// 0x1008d070
NCPawnView_CharacterSelect_List::NCPawnView_CharacterSelect_List()
{
}
NW_HOOK_CTOR(0x1008d070, NCPawnView_CharacterSelect_List)

// 0x1008d110 - the debug list's 50 / 10 lines replaced
NCPawnView_CharacterSelect_List::NCPawnView_CharacterSelect_List(INT InMaxLines,
                                                                 INT InVisibleLines)
{
    MaxLines = InMaxLines;
    VisibleLines = InVisibleLines;
    PlayingIndex = -1;
}
NW_HOOK_CTOR_ARGS(0x1008d110, NCPawnView_CharacterSelect_List, (INT A, INT B), (A, B))

// 0x1008d0c0
NW_HOOK_DTOR(0x1008d0c0, NCPawnView_CharacterSelect_List)

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a530)
INT NCPawnView_CharacterSelect_List::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCDebugWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10088450 (vslot 35)
INT NCPawnView_CharacterSelect_List::OnCreate()
{
    guard(NCPawnView_CharacterSelect_List::OnCreate);
    NCDebugWnd::OnCreate();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088450, NCPawnView_CharacterSelect_List, OnCreate)

// 0x10088550 (vslot 36)
INT NCPawnView_CharacterSelect_List::OnDestroy()
{
    guard(NCPawnView_CharacterSelect_List::OnDestroy);
    NCDebugWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10088550, NCPawnView_CharacterSelect_List, OnDestroy)

// 0x100884d0 (vslot 50)
INT NCPawnView_CharacterSelect_List::OnPaint(NCGDevice* Device)
{
    guard(NCPawnView_CharacterSelect_List::OnPaint);
    NCDebugWnd::OnPaint(Device);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100884d0, NCPawnView_CharacterSelect_List, OnPaint)

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a730)
INT NCPawnView_CharacterSelect_List::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCListBox::OnMouseWheel(wParam, lParam);
    return 1;
}

// 0x10089f10 (vslot 66) - the line's animation loops on the player's pawn; the line playing
// goes back to the idle animation of the pawn's weapon (and stays the one playing, so a third
// double click stops it again)
INT NCPawnView_CharacterSelect_List::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCPawnView_CharacterSelect_List::OnLButtonDblClick);
    const TCHAR* AnimName = (const TCHAR*)GetItemParam(Selected);
    APawn* Pawn = NPawnView::PlayerPawn();
    DWORD AnimFlags = Pawn->AnimFlags;
    if (PlayingIndex == Selected)
    {
        Pawn->Unknown13D4 |= 1;
        Pawn->AnimFlags = AnimFlags & ~0x10;
        Pawn->PlayAnim(0, Pawn->GetWaitAnimName(), 1.0f, 0.0f, 1, 0);
    }
    else
    {
        Pawn->Unknown13D4 &= ~1;
        Pawn->AnimFlags = AnimFlags | 0x10;
        Pawn->PlayAnim(0, FName(AnimName, FNAME_Add), Pawn->Unknown694, 0.0f, 1, 0);
        PlayingIndex = Selected;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089f10, NCPawnView_CharacterSelect_List, OnLButtonDblClick)

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a6f0)
INT NCPawnView_CharacterSelect_List::ScrollUp()
{
    NCListBox::ScrollUp();
    return 1;
}

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a700)
INT NCPawnView_CharacterSelect_List::ScrollDown()
{
    NCListBox::ScrollDown();
    return 1;
}

// stock body shared with NCNpcView_CharacterSelect_List's (0x1007a710)
INT NCPawnView_CharacterSelect_List::SetTopIndex(INT Index)
{
    NCListBox::SetTopIndex(Index);
    return 1;
}

NW_CHECK_VTABLE(NCPawnView_CharacterSelect_List, 0x100eb698, 70)

/*-----------------------------------------------------------------------------
    NCPawnView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x10089140
NCPawnView_CharacterSelectWnd::NCPawnView_CharacterSelectWnd()
{
    TitleTex = NULL;
    BackTex = NULL;
    UnknownA8 = NULL;
    UnknownAC = NULL;
    UnknownB0 = NULL;
    UnknownB4 = 0;
    Selected = -1;
}
NW_HOOK_CTOR(0x10089140, NCPawnView_CharacterSelectWnd)

// 0x100891c0
NW_HOOK_DTOR(0x100891c0, NCPawnView_CharacterSelectWnd)

// 0x10088140 (vslot 24)
INT NCPawnView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CHARSELECT_NAME)
            return OnNameComboBox((INT)lParam) != 0;
        if (wParam == NID_CHARSELECT_ANINAME)
            return OnAniNameComboBox((INT)lParam) != 0;
        if (wParam == NID_CHARSELECT_BYACTION)
            return OnByActionCheck() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10088140, NCPawnView_CharacterSelectWnd, WindowProc)

// 0x1008d960 (vslot 35) - a title bar without buttons, the textures, the character combo,
// the animation list (100 lines, 18 shown) and "by action" ("\xC561\xC158\xBCC4"), unchecked
INT NCPawnView_CharacterSelectWnd::OnCreate()
{
    guard(NCPawnView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_CHARSELECT_FRAME;
    FrameCtrl->ID = NID_CHARSELECT_FRAME;
    FrameCtrl->SetWindowRect(2, 3, Width - 6, 20);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    UnknownA8 = FindTexture(TEXT("sek.cbui67"));
    UnknownAC = FindTexture(TEXT("sek.cbui68"));
    UnknownB0 = FindTexture(TEXT("sek.cbui69"));

    NameCombo = new NCComboBox;
    NameCombo->ID = NID_CHARSELECT_NAME;
    NameCombo->SetWindowRect(10, 28, 129, 18);
    CreateChild(NameCombo, NWS_VISIBLE);

    AnimList = new NCPawnView_CharacterSelect_List(100, 18);
    AnimList->SetWindowRect(33, 59, 166, 234);
    CreateChild(AnimList, NWS_VISIBLE);

    ByActionCheck = new NCCheckBox(TEXT("\xC561\xC158\xBCC4"), 0, 0);
    ByActionCheck->ID = NID_CHARSELECT_BYACTION;
    ByActionCheck->SetWindowRect(150, 30, 50, 16);
    CreateChild(ByActionCheck, NWS_VISIBLE);
    if (!ByActionCheck->bDisabled)
        ByActionCheck->bChecked = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008d960, NCPawnView_CharacterSelectWnd, OnCreate)

// stock body shared with NCTrainWnd's (0x10089310): the check box is left alone
INT NCPawnView_CharacterSelectWnd::OnDestroy()
{
    DestroyChild(this, NameCombo);
    DestroyChild(this, AnimList);
    NCWnd::OnDestroy();
    return 1;
}

// 0x10089210 (vslot 50) - the background stretched 20 px past the right edge, the title
// strip; the clip goes on before the origin (the guard names NCCharacterSelectWnd)
INT NCPawnView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCCharacterSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawStretchedTex(0, 0, Width + 20, Height, BackTex);
    if (TitleTex)
        Device->DrawTexture(2, 3, Width - 4, 20, 0, 0, Width - 4, 20, TitleTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10089210, NCPawnView_CharacterSelectWnd, OnPaint)

// 0x1008cd20 (vslot 62) - another default character: a new PlayerController is spawned only
// to own the new pawn and carry its class (it is left in the level), the pawn of that class
// replaces the player's 30 units higher under the player's own controller, falls to the
// ground, is dressed as the record and plays its idle animation
INT NCPawnView_CharacterSelectWnd::OnNameComboBox(INT Index)
{
    guard(NCPawnView_CharacterSelectWnd::OnNameComboBox);
    if (Index > 19 || Index == Selected)
        return 1;
    ULevel* Level = GameLevel();
    APlayerController* PlayerController = ViewActor();
    FName ControllerClass = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* pController = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, *ControllerClass, NULL, 1, NULL));
    check(pController);
    User Character = NPawnView::Viewer()->Characters[Index];
    UClass*& PawnClass = pController->PawnClass;
    PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL,
                                         Character.GetClassNamePointer(), NULL, 0, NULL);
    if (!PawnClass)
        return 1;
    FVector Location = NPawnView::PlayerPawn()->Location;
    Location.Z += 30.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(PawnClass, NAME_None, Location, FRotator(0, 0, 0),
                                            NULL, 1, 1, pController, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(NPawnView::PlayerPawn(), 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = PlayerController;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Selected = Index;
    NPawnView::Viewer()->Characters[Index].Pawn = Pawn;
    NPawnView::Viewer()->Characters[Selected].SetPawnResource();
    NameCombo->SetSelected(Selected);
    Pawn->PlayAnim(0, Pawn->GetWaitAnimName(), 1.0f, 0.0f, 1, 0);
    UpdateAnimList();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008cd20, NCPawnView_CharacterSelectWnd, OnNameComboBox)

// 0x1008d060 (vslot 64)
INT NCPawnView_CharacterSelectWnd::OnByActionCheck()
{
    UpdateAnimList();
    return 1;
}
NW_HOOK_METHOD(0x1008d060, NCPawnView_CharacterSelectWnd, OnByActionCheck)

// 0x1008c3c0 - the animations of the player's pawn mesh into the list: first those whose name
// holds each key (by action, or by weapon), then the rest in the mesh's order; a line's
// param is the name itself
__declspec(noinline) void NCPawnView_CharacterSelectWnd::UpdateAnimList()
{
    TArray<INT> Remaining;
    APawn* Pawn = NPawnView::PlayerPawn();
    if (!Pawn)
        return;
    UMesh* Mesh = Pawn->Mesh;
    if (!Mesh)
        return;
    Mesh->MeshGetInstance(Pawn);
    UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
    INT NumSequences = Anim->AnimSeqs.Num();
    AnimList->Clear();
    for (INT i = 0; i < NumSequences; i++)
        Remaining.AddItem(i);
    const TCHAR(*Keys)[10] = ByActionCheck->bChecked ? GAnimActionKeys : GAnimWeaponKeys;
    INT NumKeys = ByActionCheck->bChecked ? 5 : 6;
    for (INT k = 0; k < NumKeys; k++)
    {
        for (INT i = 0; i < NumSequences; i++)
        {
            FName Name = Anim->AnimSeqs(i).Name;
            INT Index = Remaining.FindItemIndex(i);
            if (Index != -1 && appStristr(*Name, Keys[k]) && AnimList)
            {
                Remaining.Remove(Index);
                AnimList->AddString(*Name, 0xFFFFFFFF, (INT)*Name);
            }
        }
    }
    for (INT i = 0; i < Remaining.Num(); i++)
        AnimList->AddString(*Anim->AnimSeqs(Remaining(i)).Name, 0xFFFFFFFF,
                            (INT)*Anim->AnimSeqs(Remaining(i)).Name);
}
NW_HOOK_METHOD(0x1008c3c0, NCPawnView_CharacterSelectWnd, UpdateAnimList)

NW_CHECK_VTABLE(NCPawnView_CharacterSelectWnd, 0x100ead40, 65)

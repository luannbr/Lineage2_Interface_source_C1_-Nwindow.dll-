/*=============================================================================
    NCNpcViewrWnd.cpp - the NPC viewer, its NPC list and item grids. Stock: 0x1007a1a0 ..
    0x1007a75a, 0x1007a7f0 .. 0x1007b3c4, 0x1007b3e0 .. 0x1007c5b6, 0x1007c5d0 .. 0x1007d7cc
    (the grids' WindowProc also serves the skill viewer's; TMapBase::Rehash, 0x1007a760, is
    left out).
=============================================================================*/
#include "nwindow/NCNpcViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCCheckBox.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NCTabControl.h"
#include "nwindow/NItemInfo.h"
#include "nwindow/NString.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// UObject::StaticLoadClass flags (UE2)
enum { LOAD_NoFail = 0x1 };

// sysstring labels of the selection window's edit lines (names ours)
enum { STR_NpcViewSpeed = 432, STR_NpcViewRate = 316 };

template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the NPCs listed: those with an NPC mesh, or a monster mesh, as checked
static FORCEINLINE UBOOL IsListedNpc(NCNpcView_CharacterSelectWnd* Wnd, NViewNpcData* Npc)
{
    if (Wnd->NpcCheck->bChecked && appStristr(*Npc->MeshName, TEXT("lineagenpcs")))
        return 1;
    return Wnd->MonsterCheck->bChecked && appStristr(*Npc->MeshName, TEXT("lineagemonsters"));
}

/*-----------------------------------------------------------------------------
    NCNpcView_InvenItemWnd.
-----------------------------------------------------------------------------*/

// 0x1007be90
NCNpcView_InvenItemWnd::NCNpcView_InvenItemWnd()
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR(0x1007be90, NCNpcView_InvenItemWnd)

// 0x1007bf30 - the viewer's are 6 x 4 icons of 32 pixels, 80 at first (AddAllItem raises it)
NCNpcView_InvenItemWnd::NCNpcView_InvenItemWnd(INT InColumns, INT InVisibleRows, INT InMaxItems,
                                               INT InCellSize)
    : NCInvenItemWnd(InColumns, InVisibleRows, InMaxItems, InCellSize)
{
    bDropPending = 0;
    bDestroyPending = 0;
}
NW_HOOK_CTOR_ARGS(0x1007bf30, NCNpcView_InvenItemWnd, (INT A, INT B, INT C, INT D), (A, B, C, D))

// 0x1007bee0
NW_HOOK_DTOR(0x1007bee0, NCNpcView_InvenItemWnd)

// 0x1007a1a0 (vslot 24) - NCInvenItemWnd's messages, then the double click (also the skill
// viewer's grids')
INT NCNpcView_InvenItemWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCInvenItemWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_LBUTTONDBLCLK)
        return OnLButtonDblClick(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1007a1a0, NCNpcView_InvenItemWnd, WindowProc)

// 0x1007a7f0 (vslot 58) - the item under the mouse dresses the viewer's pawn: its id in the
// slot of its body part (a two-handed one empties both hands, a one-handed one the two-handed
// slot), the pawn's resources again, then its left hand's class
INT NCNpcView_InvenItemWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_InvenItemWnd::OnRButtonDown);
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (Selected >= 0 && Selected < Items.Num())
    {
        INT Slot = Items(Selected)->BodyPart;
        if (Slot == BODYSLOT_LRHand)
        {
            NpcViewer()->Npcs[0].Equipped[BODYSLOT_RHand] = 0;
            NpcViewer()->Npcs[0].Equipped[BODYSLOT_LHand] = 0;
        }
        else if (Slot == BODYSLOT_RHand)
            NpcViewer()->Npcs[0].Equipped[BODYSLOT_LRHand] = 0;
        else if (Slot == BODYSLOT_LHand)
            NpcViewer()->Npcs[0].Equipped[BODYSLOT_LRHand] = 0;
        NpcViewer()->Npcs[0].Equipped[Items(Selected)->BodyPart] = Items(Selected)->ID;
        NpcViewer()->Npcs[0].Pawn = ViewActor()->Pawn;
        NpcViewer()->Npcs[0].SetPawnResource();
        INT LeftHand = NpcViewer()->Npcs[0].GetItemClassID(BODYSLOT_LHand);
        NpcViewer()->Npcs[0].Pawn->LeftHandItem = LeftHand;
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a7f0, NCNpcView_InvenItemWnd, OnRButtonDown)

// 0x1007a210 (vslot 68) - as the right button
INT NCNpcView_InvenItemWnd::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_InvenItemWnd::OnLButtonDblClick);
    bDragging = 0;
    Selected = HitTest(LOWORD(lParam), HIWORD(lParam));
    UnselectAll();
    OnRButtonDown(wParam, lParam);
    bDragging = 0;
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a210, NCNpcView_InvenItemWnd, OnLButtonDblClick)

/*-----------------------------------------------------------------------------
    NCNpcView_CharacterSelect_List.
-----------------------------------------------------------------------------*/

// 0x1007c5d0
NCNpcView_CharacterSelect_List::NCNpcView_CharacterSelect_List()
{
}
NW_HOOK_CTOR(0x1007c5d0, NCNpcView_CharacterSelect_List)

// 0x1007c670 - NCDebugWnd's defaults, then the sizes
NCNpcView_CharacterSelect_List::NCNpcView_CharacterSelect_List(INT InMaxLines, INT InVisibleLines)
{
    MaxLines = InMaxLines;
    VisibleLines = InVisibleLines;
}
NW_HOOK_CTOR_ARGS(0x1007c670, NCNpcView_CharacterSelect_List, (INT A, INT B), (A, B))

// 0x1007c620
NW_HOOK_DTOR(0x1007c620, NCNpcView_CharacterSelect_List)

// 0x1007a530 (vslot 24) - also NCPawnView_CharacterSelect_List's
INT NCNpcView_CharacterSelect_List::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCDebugWnd::WindowProc(Msg, wParam, lParam) != 0;
}
NW_HOOK_METHOD(0x1007a530, NCNpcView_CharacterSelect_List, WindowProc)

// 0x1007a570 (vslot 35)
INT NCNpcView_CharacterSelect_List::OnCreate()
{
    guard(NCNpcView_CharacterSelect_List::OnCreate);
    NCDebugWnd::OnCreate();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a570, NCNpcView_CharacterSelect_List, OnCreate)

// 0x1007a670 (vslot 36)
INT NCNpcView_CharacterSelect_List::OnDestroy()
{
    guard(NCNpcView_CharacterSelect_List::OnDestroy);
    NCDebugWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a670, NCNpcView_CharacterSelect_List, OnDestroy)

// 0x1007a5f0 (vslot 50)
INT NCNpcView_CharacterSelect_List::OnPaint(NCGDevice* Device)
{
    guard(NCNpcView_CharacterSelect_List::OnPaint);
    NCDebugWnd::OnPaint(Device);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a5f0, NCNpcView_CharacterSelect_List, OnPaint)

// 0x1007a730 (vslot 61) - also NCPawnView_CharacterSelect_List's, as 67 .. 69
INT NCNpcView_CharacterSelect_List::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    NCDebugWnd::OnMouseWheel(wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1007a730, NCNpcView_CharacterSelect_List, OnMouseWheel)

// 0x1007a6f0 (vslot 67)
INT NCNpcView_CharacterSelect_List::ScrollUp()
{
    NCDebugWnd::ScrollUp();
    return 1;
}
NW_HOOK_METHOD(0x1007a6f0, NCNpcView_CharacterSelect_List, ScrollUp)

// 0x1007a700 (vslot 68)
INT NCNpcView_CharacterSelect_List::ScrollDown()
{
    NCDebugWnd::ScrollDown();
    return 1;
}
NW_HOOK_METHOD(0x1007a700, NCNpcView_CharacterSelect_List, ScrollDown)

// 0x1007a710 (vslot 69)
INT NCNpcView_CharacterSelect_List::SetTopIndex(INT Index)
{
    NCDebugWnd::SetTopIndex(Index);
    return 1;
}
NW_HOOK_METHOD(0x1007a710, NCNpcView_CharacterSelect_List, SetTopIndex)

// 0x1007a750
void NCNpcView_CharacterSelect_List::SetScrollRange(INT Total, INT Page)
{
    ScrollBar->SetScrollRange(Total, Page);
}
NW_HOOK_METHOD(0x1007a750, NCNpcView_CharacterSelect_List, SetScrollRange)

// 0x1007acf0 (vslot 66) - the NPC of the line becomes the player's pawn: a player controller
// spawned for it, its pawn class loaded by the NPC's class name (and logged), the pawn spawned
// 60 units above the player's, which it replaces (the old one destroyed; the controller kept
// is the viewport's). The viewer's pawn gets its resources, the selection window's edit lines
// its speed and the factor 1, the animation combo box its animations.
INT NCNpcView_CharacterSelect_List::OnLButtonDblClick(WPARAM wParam, LPARAM lParam)
{
    guard(NCNpcView_CharacterSelect_List::OnLButtonDblClick);
    FName ControllerClassName = NAME_None;
    if (Selected < 0)
        return 1;
    ULevel* Level = GameLevel();
    APlayerController* Viewer = ViewActor();
    ControllerClassName = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* Controller = (AController*)Level->SpawnActor(
        StaticLoadClass(AController::StaticClass(), NULL, *ControllerClassName, NULL, LOAD_NoFail,
                        NULL));
    if (!Controller)
        appFailAssert("pController", "Src\\NNpcViewerWnd.cpp", 563);
    NViewNpcData* Npc = (NViewNpcData*)GetItemParam(Selected);
    NpcViewer()->Npcs[0].NpcID = Npc->ID;
    Controller->PawnClass = StaticLoadClass(APawn::StaticClass(), NULL, *Npc->ClassName, NULL, 0,
                                            NULL);
    if (GLog)
        GLog->Logf2(TEXT("PawnClass = %s"), *Npc->ClassName);
    if (!Controller->PawnClass)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    Location.Z += 60.0f;
    APawn* Pawn = (APawn*)Level->SpawnActor(Controller->PawnClass, NAME_None, Location,
                                            FRotator(0, 0, 0), NULL, 1, 1, Controller, NULL);
    if (!Pawn)
        return 1;
    Level->DestroyActor(ViewActor()->Pawn, 0);
    ViewActor()->Pawn = Pawn;
    Pawn->Controller = Viewer;
    Pawn->Role = ROLE_Authority;
    Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Pawn->NpcID = Npc->ID;
    NpcViewer()->Npcs[0].Pawn = Pawn;
    NpcViewer()->Npcs[0].SetPawnResource();
    NpcViewer()->CharSelectWnd->Speed = (INT)Pawn->Speed;
    TCHAR Speed[128];
    appSprintf(Speed, TEXT("%d"), (INT)Pawn->Speed);
    NpcViewer()->CharSelectWnd->SpeedEdit->ClearText();
    NpcViewer()->CharSelectWnd->SpeedEdit->InsertText(Speed, 0);
    NpcViewer()->CharSelectWnd->Rate = 1.0f;
    NpcViewer()->CharSelectWnd->RateEdit->ClearText();
    NpcViewer()->CharSelectWnd->RateEdit->InsertText(TEXT("1"), 0);
    APawn* ViewPawn = ViewActor()->Pawn;
    if (ViewPawn)
    {
        UMesh* Mesh = ViewPawn->Mesh;
        if (Mesh)
        {
            Mesh->MeshGetInstance(ViewPawn);
            UMeshAnimation* Anim = static_cast<USkeletalMesh*>(Mesh)->DefaultAnim;
            if (Anim)
            {
                INT Num = Anim->AnimSeqs.Num();
                FName Sequence = NAME_None;
                NpcViewer()->CharSelectWnd->AniCombo->ClearItems();
                for (INT i = 0; i < Num; i++)
                {
                    Sequence = Anim->AnimSeqs(i).Name;
                    NpcViewer()->CharSelectWnd->AniCombo->AddItem(*Sequence);
                }
            }
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007acf0, NCNpcView_CharacterSelect_List, OnLButtonDblClick)

// 0x1007b190 - every line's text again: the NPC's mesh, or its name
void NCNpcView_CharacterSelect_List::ToggleNames(UBOOL bMeshNames)
{
    guard(NCNpcView_CharacterSelect_List::ToggleNames);
    for (INT i = 0; i < Items.Num(); i++)
    {
        NListItem& Item = Items(i);
        Item.Text->Empty();
        NViewNpcData* Npc = (NViewNpcData*)Item.Param;
        Item.Text->Insert(bMeshNames ? *Npc->MeshName : *Npc->Name, 0);
    }
    unguard;
}
NW_HOOK_METHOD(0x1007b190, NCNpcView_CharacterSelect_List, ToggleNames)

/*-----------------------------------------------------------------------------
    NCNpcView_CharacterSelectWnd.
-----------------------------------------------------------------------------*/

// 0x1007b240 - the rest is left as allocated
NCNpcView_CharacterSelectWnd::NCNpcView_CharacterSelectWnd()
{
    Rate = 1.0f;
}
NW_HOOK_CTOR(0x1007b240, NCNpcView_CharacterSelectWnd)

// 0x1007b290
NW_HOOK_DTOR(0x1007b290, NCNpcView_CharacterSelectWnd)

// 0x1007bd80 (vslot 24)
INT NCNpcView_CharacterSelectWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_NPCVIEW_SELECT_ANIM)
            return OnAniNameCombo(lParam) != 0;
        if (wParam == NID_NPCVIEW_SELECT_SEARCH)
            return OnSearch() != 0;
        if (wParam == NID_NPCVIEW_SELECT_RESET)
            return OnReset() != 0;
        if (wParam == NID_NPCVIEW_SELECT_APPLY)
            return OnApply() != 0;
        if (wParam == NID_NPCVIEW_SELECT_MESHNAME)
            return OnToggleNames() != 0;
        if (wParam == NID_NPCVIEW_SELECT_SUMMON)
            return OnSummon() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x1007bd80, NCNpcView_CharacterSelectWnd, WindowProc)

// 0x1007c6a0 (vslot 35) - the NPC list (3010 lines, 20 shown) under the animation combo box,
// the search and speed controls below, the kinds listed (monsters, NPCs; by name)
INT NCNpcView_CharacterSelectWnd::OnCreate()
{
    guard(NCNpcView_CharacterSelectWnd::OnCreate);
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(0);
    FrameID = NID_NPCVIEW_SELECT_FRAME;
    FrameCtrl->ID = NID_NPCVIEW_SELECT_FRAME;
    FrameCtrl->SetWindowRect(2, 3, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    AniCombo = new NCComboBox;
    AniCombo->ID = NID_NPCVIEW_SELECT_ANIM;
    AniCombo->SetWindowRect(25, 30, 125, 18);
    CreateChild(AniCombo, NWS_VISIBLE);
    List = new NCNpcView_CharacterSelect_List(3010, 20);
    List->SetWindowRect(10, 55, 300, 280);
    CreateChild(List, NWS_VISIBLE);
    SearchEdit = new NCEditBox(1, NULL, 1, 64);
    SearchEdit->ID = NID_NPCVIEW_SELECT_SEARCHEDIT;
    SearchEdit->SetWindowRect(10, 350, 80, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);
    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_NPCVIEW_SELECT_SEARCH;
    SearchButton->SetWindowRect(95, 350, 28, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(420);
    ResetButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    ResetButton->ID = NID_NPCVIEW_SELECT_RESET;
    ResetButton->SetWindowRect(128, 350, 45, 16);
    CreateChild(ResetButton, NWS_VISIBLE);
    ResetButton->SetTextID(422);
    SpeedEdit = new NCEditBox(1, NULL, 1, 64);
    SpeedEdit->ID = NID_NPCVIEW_SELECT_SPEED;
    SpeedEdit->SetWindowRect(80, 430, 32, 16);
    CreateChild(SpeedEdit, NWS_VISIBLE);
    RateEdit = new NCEditBox(1, NULL, 1, 64);
    RateEdit->ID = NID_NPCVIEW_SELECT_RATE;
    RateEdit->SetWindowRect(80, 450, 32, 16);
    CreateChild(RateEdit, NWS_VISIBLE);
    SummonButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    SummonButton->ID = NID_NPCVIEW_SELECT_SUMMON;
    SummonButton->SetWindowRect(180, 350, 28, 16);
    CreateChild(SummonButton, NWS_VISIBLE);
    SummonButton->SetText(TEXT("\xC18C\xD658"));                        // "Summon"
    MeshNameCheck = new NCCheckBox(TEXT("Mesh\xC774\xB984"), 0, 0);     // "Mesh name"
    MeshNameCheck->ID = NID_NPCVIEW_SELECT_MESHNAME;
    MeshNameCheck->SetWindowRect(180, 32, 100, 16);
    CreateChild(MeshNameCheck, NWS_VISIBLE);
    if (!MeshNameCheck->bDisabled)
        MeshNameCheck->bChecked = 0;
    MonsterCheck = new NCCheckBox(TEXT("Monster"), 0, 0);
    MonsterCheck->SetWindowRect(30, 380, 100, 16);
    CreateChild(MonsterCheck, NWS_VISIBLE);
    if (!MonsterCheck->bDisabled)
        MonsterCheck->bChecked = 1;
    NpcCheck = new NCCheckBox(TEXT("NPC"), 0, 0);
    NpcCheck->SetWindowRect(150, 380, 100, 16);
    CreateChild(NpcCheck, NWS_VISIBLE);
    if (!NpcCheck->bDisabled)
        NpcCheck->bChecked = 1;
    ApplyButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    ApplyButton->ID = NID_NPCVIEW_SELECT_APPLY;
    ApplyButton->SetWindowRect(120, 440, 32, 16);
    CreateChild(ApplyButton, NWS_VISIBLE);
    ApplyButton->SetTextID(218);
    SetTitle(TEXT("NpcViewer"));
    TitleTex = FindTexture(TEXT("sek.cbui168"));
    BackTex = FindTexture(TEXT("sek.cbui144"));
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007c6a0, NCNpcView_CharacterSelectWnd, OnCreate)

// 0x1007b3e0 (vslot 36) - the list is deleted without leaving the child list (and its field
// keeps the pointer)
INT NCNpcView_CharacterSelectWnd::OnDestroy()
{
    DestroyChild(this, AniCombo);
    DestroyChild(this, FrameCtrl);
    DestroyChild(this, ResetButton);
    DestroyChild(this, SearchEdit);
    DestroyChild(this, SearchButton);
    DestroyChild(this, SpeedEdit);
    DestroyChild(this, RateEdit);
    DestroyChild(this, SummonButton);
    DestroyChild(this, MeshNameCheck);
    DestroyChild(this, MonsterCheck);
    DestroyChild(this, NpcCheck);
    DestroyChild(this, ApplyButton);
    delete List;
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1007b3e0, NCNpcView_CharacterSelectWnd, OnDestroy)

// 0x1007b610 (vslot 50) - the title strip and the background stretched, the edit lines'
// labels centered in 64 x 16 beside them
INT NCNpcView_CharacterSelectWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNpcView_CharacterSelectWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (TitleTex)
        Device->DrawStretchedTex(1, 3, 350, 17, 0, 0, 240, 20, TitleTex);
    if (BackTex)
        Device->DrawStretchedTex(0, 20, 350, 480, 0, 0, 242, 156, BackTex);
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_NpcViewSpeed], 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 10, (16 - Size.cy) / 2 + 430, 0xFFFFFFFF,
                     *GL2GameData.SysString[STR_NpcViewSpeed], 0, 0, 0, L2FONT_Normal, 0, 0, 0,
                     0, 0, 0);
    NCGDevice::GetTextExtent(&Size, *GL2GameData.SysString[STR_NpcViewRate], 0, 0, 0, 0, 0);
    Device->DrawText((64 - Size.cx) / 2 + 10, (16 - Size.cy) / 2 + 450, 0xFFFFFFFF,
                     *GL2GameData.SysString[STR_NpcViewRate], 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0,
                     0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007b610, NCNpcView_CharacterSelectWnd, OnPaint)

// 0x1007cfe0 (vslot 62) - the NPCs listed whose name (or mesh, when checked) holds the text
INT NCNpcView_CharacterSelectWnd::OnSearch()
{
    const TCHAR* Text = SearchEdit->CommitText();
    INT Count = 0;
    if (Text)
    {
        List->Clear();
        NDataMap<NViewNpcData>* Map = GL2GameData.NpcTable;
        for (INT i = 0; i < Map->Pairs.Num(); i++)
        {
            NViewNpcData* Npc = Map->Pairs(i).Value;
            if (!Npc || !Npc->HasName() || !IsListedNpc(this, Npc))
                continue;
            if (MeshNameCheck->bChecked && appStristr(*Npc->MeshName, Text))
            {
                Count++;
                List->AddString(*Npc->MeshName, 0xFFFFFFFF, (INT)Npc);
            }
            else if (!MeshNameCheck->bChecked
                     && appStristr(GL2GameData.GetNpcName(Map->Pairs(i).Key), Text))
            {
                Count++;
                List->AddString(GL2GameData.GetNpcName(Map->Pairs(i).Key), 0xFFFFFFFF, (INT)Npc);
            }
        }
        List->SetScrollRange(Count, 20);
        List->TopIndex = 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x1007cfe0, NCNpcView_CharacterSelectWnd, OnSearch)

// 0x1007d210 (vslot 63) - the whole table again
INT NCNpcView_CharacterSelectWnd::OnReset()
{
    List->Clear();
    NpcViewer()->AddAllCharacter();
    return 1;
}
NW_HOOK_METHOD(0x1007d210, NCNpcView_CharacterSelectWnd, OnReset)

// 0x1007b800 (vslot 64) - a factor above 0 goes to the viewer pawn's speed factor and its
// animation's rate (the first channel of its mesh instance, as SetAnimRate does it without the
// channel check); a speed above 0 (truncated) to its speed, divided by the factor when that is
// above 0
INT NCNpcView_CharacterSelectWnd::OnApply()
{
    if (RateEdit->CommitText())
    {
        DOUBLE Value = _wtof(RateEdit->CommitText());
        if (Value > 0.0)
        {
            Rate = (FLOAT)Value;
            NpcViewer()->Npcs[0].Pawn->SpeedRate = (FLOAT)Value;
            USkeletalMeshInstance* MeshInstance =
                static_cast<USkeletalMeshInstance*>(NpcViewer()->Npcs[0].Pawn->MeshInstance);
            if (MeshInstance)
            {
                MeshAnimChannel& Channel = MeshInstance->Blends(0);
                Channel.AnimRate = Channel.Unknown20 * Rate;
            }
        }
    }
    if (SpeedEdit->CommitText())
    {
        DOUBLE Value = _wtof(SpeedEdit->CommitText());
        if (Value > 0.0)
        {
            Speed = (INT)Value;
            DOUBLE NewSpeed = Speed;
            if (Rate > 0.0f)
                NewSpeed /= Rate;
            NpcViewer()->Npcs[0].Pawn->Speed = (FLOAT)NewSpeed;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1007b800, NCNpcView_CharacterSelectWnd, OnApply)

// 0x1007b920 (vslot 65) - the list's lines by mesh or by name, as checked
INT NCNpcView_CharacterSelectWnd::OnToggleNames()
{
    List->ToggleNames(MeshNameCheck->bChecked);
    return 1;
}
NW_HOOK_METHOD(0x1007b920, NCNpcView_CharacterSelectWnd, OnToggleNames)

// 0x1007b940 (vslot 66) - the NPC of the selected line spawned where the player stands, with
// the player's rotation and a player controller of its own (both with the authority role), a
// User record made for its resources; then moved down onto the ground (a box trace from 50
// units above the player's feet down, 30 units longer both ways each try, 30 tries; a hit
// closer than 1.9 units (the time times 12) is raised to the middle of 1.9 .. 2.4) and its
// spawn event run. The class name is read as an engine FName here (the double click reads it
// as an L2FName). Nothing checks that the pawn was spawned.
INT NCNpcView_CharacterSelectWnd::OnSummon()
{
    APlayerController* Viewer = ViewActor();
    if (!Viewer || !Viewer->Pawn || !Viewer->XLevel)
        return 1;
    INT Index = List->Selected;
    if (Index < 0)
        return 1;
    FVector Location = ViewActor()->Pawn->Location;
    ULevel* Level = ViewActor()->XLevel;
    AController* Controller = (AController*)Level->SpawnActor(
        StaticLoadClass(AController::StaticClass(), NULL, TEXT("Engine.PlayerController"), NULL,
                        LOAD_NoFail, NULL),
        NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1);
    NViewNpcData* Npc = (NViewNpcData*)List->GetItemParam(Index);
    FName ClassName = NAME_None;
    if (Npc)
        ClassName = *(FName*)&Npc->ClassName;
    UClass* PawnClass = StaticLoadClass(APawn::StaticClass(), NULL, *ClassName, NULL, 0, NULL);
    if (!PawnClass)
        return 1;
    APawn* Pawn = (APawn*)Level->SpawnActor(PawnClass, NAME_None, Location,
                                            ViewActor()->Pawn->Rotation, NULL, 1, 0, Controller);
    Controller->Pawn = Pawn;
    Pawn->Controller = Controller;
    Controller->Role = ROLE_Authority;
    Controller->Pawn->Role = ROLE_Authority;
    Controller->Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Controller->Pawn->NpcID = Npc->ID;
    User Record;
    Record.Unknown8 = 1;
    Record.NpcID = Npc->ID;
    Record.ObjectID = Npc->ID;
    Record.Pawn = Controller->Pawn;
    Record.SetPawnResource();
    FVector Start = Location;
    FVector End = Location;
    Start.Z += 50.0f;
    for (INT i = 0; i < 30; i++)
    {
        Start.Z += 30.0f;
        End.Z -= 30.0f;
        FCheckResult Hit(1.0f);
        APawn* Summoned = Controller->Pawn;
        Level->SingleLineCheck(Hit, Summoned, End, Start, 0x86,
                               FVector(Summoned->CollisionRadius, Summoned->CollisionRadius,
                                       Summoned->CollisionHeight));
        if (Hit.Location.Z != 0.0f)
        {
            DOUBLE Step = Hit.Time * 12.0;
            if (Step < 1.9f)
                Hit.Location.Z = (FLOAT)(((DOUBLE)2.4f + 1.9f) * 0.5f - Step + Hit.Location.Z);
            Level->FarMoveActor(Controller->Pawn, Hit.Location, 0, 0, 0);
            break;
        }
    }
    Record.Pawn->SpawnEnterEvent();
    return 1;
}
NW_HOOK_METHOD(0x1007b940, NCNpcView_CharacterSelectWnd, OnSummon)

// 0x1007b2e0 - the animation picked (the combo box's notice, lParam) plays looped on the
// player's pawn at the speed factor. Items(Index) is read up to Index == Num (">=" in the
// stock code).
INT NCNpcView_CharacterSelectWnd::OnAniNameCombo(INT Index)
{
    guard(NCNpcView_CharacterSelectWnd::OnAniNameCombo);
    if (AniCombo->Items.Num() >= Index)
    {
        TCHAR* Name = AniCombo->Items(Index)->GetAt(0);
        APawn* Pawn = ViewActor()->Pawn;
        Pawn->Unknown13D4 &= ~1;
        Pawn->AnimFlags |= 0x10;
        Pawn->PlayAnim(0, FName(Name, FNAME_Add), Rate, 0.f, 1, 0);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007b2e0, NCNpcView_CharacterSelectWnd, OnAniNameCombo)

/*-----------------------------------------------------------------------------
    NCNpcViewrWnd.
-----------------------------------------------------------------------------*/

// 0x1007a9a0 - the NPC records built; the selection window and the search controls are left
// as allocated
NCNpcViewrWnd::NCNpcViewrWnd()
{
    TabCtrl = NULL;
    BackTex = NULL;
    OptionBackTex = NULL;
    ItemWnd[0] = NULL;
    ItemWnd[1] = NULL;
    Level = NULL;
}
NW_HOOK_CTOR(0x1007a9a0, NCNpcViewrWnd)

// 0x1007aa60
NW_HOOK_DTOR(0x1007aa60, NCNpcViewrWnd)

// 0x1007a2d0 (vslot 24)
INT NCNpcViewrWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_NPCVIEW_SEARCH)
        return OnSearchBtnClick() != 0;
    return 0;
}
NW_HOOK_METHOD(0x1007a2d0, NCNpcViewrWnd, WindowProc)

// 0x1007d260 (vslot 35) - the two item grids under their tabs ("item", "search"), the search
// line and button, the selection window (top-level), the first NPC record cleared (not a
// player, NPC id 1), then both tables loaded. NCWnd::OnCreate is not called.
INT NCNpcViewrWnd::OnCreate()
{
    guard(NCNpcViewrWnd::OnCreate);
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_NPCVIEW_FRAME;
    FrameCtrl->ID = NID_NPCVIEW_FRAME;
    FrameCtrl->SetWindowRect(0, 0, Width, 25);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    BackTex = FindTexture(TEXT("L2UI.QuestWnd.questBack3"));
    OptionBackTex = FindTexture(TEXT("L2UI.etcWnd.OptionBack"));
    ItemWnd[0] = new NCNpcView_InvenItemWnd(6, 4, 80, 32);
    ItemWnd[0]->SetWindowRect(16, 49, 218, 150);
    CreateChild(ItemWnd[0], NWS_VISIBLE | 0x20);
    ItemWnd[1] = new NCNpcView_InvenItemWnd(6, 4, 80, 32);
    ItemWnd[1]->SetWindowRect(16, 49, 218, 150);
    CreateChild(ItemWnd[1], NWS_VISIBLE | 0x20);
    TabCtrl = new NCTabControl;
    TabCtrl->ID = NID_NPCVIEW_TAB;
    TabCtrl->SetWindowRect(15, 22, 140, 16);
    CreateChild(TabCtrl, NWS_VISIBLE);
    TabCtrl->AddTab(2, ItemWnd[0], 70, 16, TEXT("L2UI.NWindow.Tab2"), TEXT("L2UI.NWindow.Tab1"), 0,
                    -1);
    TabCtrl->AddTab(420, ItemWnd[1], 70, 16, TEXT("L2UI.NWindow.Tab2"),
                    TEXT("L2UI.NWindow.Tab1"), 0, -1);
    SearchEdit = new NCEditBox(1, NULL, 1, 64);
    SearchEdit->SetWindowRect(70, 240, 50, 16);
    CreateChild(SearchEdit, NWS_VISIBLE);
    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_NPCVIEW_SEARCH;
    SearchButton->SetWindowRect(140, 240, 32, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(420);
    for (INT i = 0; i < 19; i++)
        Npcs[i].Pawn = NULL;
    CharSelectWnd = new NCNpcView_CharacterSelectWnd;
    CharSelectWnd->SetWindowRect(34, 34, 350, 480);
    CreateTopWindow(CharSelectWnd, NWS_VISIBLE);
    memset(&Npcs[0], 0, sizeof(User));
    Npcs[0].Unknown8 = 1;
    Npcs[0].NpcID = 1;
    AddAllItem();
    AddAllCharacter();
    Level = GameLevel();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007d260, NCNpcViewrWnd, OnCreate)

// 0x1007abc0 (vslot 36) - the selection window deleted (top-level, not cleared); the search
// line and button are left to NCWnd::OnDestroy
INT NCNpcViewrWnd::OnDestroy()
{
    guard(NCNpcViewrWnd::OnDestroy);
    DestroyChild(this, ItemWnd[0]);
    DestroyChild(this, ItemWnd[1]);
    DestroyChild(this, TabCtrl);
    DestroyChild(this, FrameCtrl);
    delete CharSelectWnd;
    NCWnd::OnDestroy();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007abc0, NCNpcViewrWnd, OnDestroy)

// 0x1007a3d0 (vslot 42) - with the selection window
void NCNpcViewrWnd::ShowWindow()
{
    guard(NCNpcViewrWnd::ShowWindow);
    if (CharSelectWnd)
        CharSelectWnd->ShowWindow();
    NCWnd::ShowWindow();
    TabCtrl->ShowCurrentPage();
    unguard;
}
NW_HOOK_METHOD(0x1007a3d0, NCNpcViewrWnd, ShowWindow)

// 0x1007a340 (vslot 43)
void NCNpcViewrWnd::HideWindow()
{
    guard(NCNpcViewrWnd::HideWindow);
    if (CharSelectWnd)
        CharSelectWnd->HideWindow();
    NCWnd::HideWindow();
    unguard;
}
NW_HOOK_METHOD(0x1007a340, NCNpcViewrWnd, HideWindow)

// 0x1007aab0 (vslot 50) - the backgrounds of the grids and of the search line
INT NCNpcViewrWnd::OnPaint(NCGDevice* Device)
{
    guard(NCNpcViewrWnd::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    if (BackTex)
        Device->DrawTexture(0, 20, 240, 185, 0, 0, 240, 185, BackTex);
    if (OptionBackTex)
        Device->DrawTexture(0, 205, 240, 70, 0, 274, 240, 70, OptionBackTex);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007aab0, NCNpcViewrWnd, OnPaint)

// 0x1007a510 (vslot 53) - Enter runs the selection window's NPC search
INT NCNpcViewrWnd::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_RETURN)
        CharSelectWnd->OnSearch();
    return 1;
}
NW_HOOK_METHOD(0x1007a510, NCNpcViewrWnd, OnKeyDown)

// 0x1007c370 (vslot 62) - the items whose name holds the text (NULL is not checked) into the
// second grid, "ID = n" as their description; the second tab shown
INT NCNpcViewrWnd::OnSearchBtnClick()
{
    guard(NCNpcViewrWnd::OnSearchBtnClick);
    const TCHAR* Text = SearchEdit->CommitText();
    ItemWnd[1]->Clear();
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Item = Map->Pairs(i).Value;
        if (!Item || !appStrstr(*Item->Name, Text) || (Item->Class != 0 && Item->Class != 2))
            continue;
        NItemInfo Info;
        INT ID = Map->Pairs(i).Key;
        if (GConsole->GetIconName(ID))
        {
            wcscpy(Info.IconName, GConsole->GetIconName(ID));
            Info.Icon = FindTexture(GConsole->GetIconName(ID));
        }
        if (!Info.Icon)
            Info.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        if (GConsole->GetItemName(ID))
        {
            appSprintf(Info.Description, TEXT("ID = %d"), ID);
            wcscpy(Info.Name, GConsole->GetItemName(ID));
        }
        Info.ID = ID;
        Info.BodyPart = Item->BodyPart;
        AddItem(1, &Info);
        ItemWnd[1]->MaxItems++;
    }
    TabCtrl->SetTab(1);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007c370, NCNpcViewrWnd, OnSearchBtnClick)

// 0x1007a470 - into a tab's grid. The bound is 8 while the viewer has two grids (the next
// fields would be taken for grids).
INT NCNpcViewrWnd::AddItem(INT Tab, NItemInfo* Info)
{
    guard(NCNpcViewrWnd::AddItem);
    if (Tab >= 0 && Tab < 8)
    {
        NCNpcView_InvenItemWnd* Grid = ItemWnd[Tab];
        if (Grid)
            Grid->AddItem(Info);
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1007a470, NCNpcViewrWnd, AddItem)

// 0x1007bf70 - the NPC table into the selection window's list (as NCNpcView_CharacterSelectWnd::
// OnSearch, without the text)
void NCNpcViewrWnd::AddAllCharacter()
{
    guard(NCNpcViewrWnd::AddAllCharacter);
    INT Count = 0;
    NDataMap<NViewNpcData>* Map = GL2GameData.NpcTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewNpcData* Npc = Map->Pairs(i).Value;
        if (!Npc || !Npc->HasName() || !IsListedNpc(CharSelectWnd, Npc))
            continue;
        const TCHAR* Name;
        if (CharSelectWnd->MeshNameCheck->bChecked)
        {
            Count++;
            Name = *Npc->MeshName;
        }
        else
        {
            Count++;
            Name = GL2GameData.GetNpcName(Map->Pairs(i).Key);
        }
        CharSelectWnd->List->AddString(Name, 0xFFFFFFFF, (INT)Npc);
    }
    CharSelectWnd->List->SetScrollRange(Count, 20);
    CharSelectWnd->List->TopIndex = 0;
    unguard;
}
NW_HOOK_METHOD(0x1007bf70, NCNpcViewrWnd, AddAllCharacter)

// 0x1007c170 - the items of classes 0 and 2 into the first grid (its capacity raised with
// each)
void NCNpcViewrWnd::AddAllItem()
{
    guard(NCNpcViewrWnd::AddAllItem);
    NDataMap<NViewItemData>* Map = GL2GameData.ItemTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        NViewItemData* Item = Map->Pairs(i).Value;
        if (!Item || (Item->Class != 0 && Item->Class != 2))
            continue;
        NItemInfo Info;
        INT ID = Map->Pairs(i).Key;
        if (GConsole->GetIconName(ID))
        {
            wcscpy(Info.IconName, GConsole->GetIconName(ID));
            Info.Icon = FindTexture(GConsole->GetIconName(ID));
        }
        if (!Info.Icon)
            Info.Icon = FindTexture(TEXT("NWindow.BlackTexture"));
        if (GConsole->GetItemName(ID))
            wcscpy(Info.Name, GConsole->GetItemName(ID));
        Info.ID = ID;
        Info.BodyPart = Item->BodyPart;
        AddItem(0, &Info);
        ItemWnd[0]->MaxItems++;
    }
    unguard;
}
NW_HOOK_METHOD(0x1007c170, NCNpcViewrWnd, AddAllItem)

NW_CHECK_VTABLE(NCNpcView_InvenItemWnd, 0x100e8f40, 70)
NW_CHECK_VTABLE(NCNpcView_CharacterSelect_List, 0x100e9148, 70)
NW_CHECK_VTABLE(NCNpcView_CharacterSelectWnd, 0x100e8d78, 67)
NW_CHECK_VTABLE(NCNpcViewrWnd, 0x100e8b20, 63)

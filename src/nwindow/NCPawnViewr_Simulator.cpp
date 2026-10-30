/*=============================================================================
    NCPawnViewr_Simulator.cpp - the pawn viewer's crowd simulator. Stock:
    0x10088770 .. 0x100887d8, 0x1008bc10 .. 0x1008c27d, 0x1008d150 .. 0x1008d935,
    0x1008de40 .. 0x1008e211, and UE2's appSeconds out of line at 0x10016620 (the pawn
    vector's std::vector code is NStdLib.cpp's; the OnDestroy body is NCLobbyWnd's).
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <intrin.h>
#include <string.h>

enum { NID_SIMULATOR_START = 0x113 };   // NWM_COMMAND id (name ours)
enum { STR_Start = 428 };               // sysstring line

// the weapons the new pawns hold (item ids)
enum { ITEM_BowWeapon = 273, ITEM_OneHandWeapon = 1 };

// the 18 default characters of the creation screen (0x101133b8, only the viewer reads it)
struct NDefaultCharacter
{
    INT Race;
    INT Sex;
    INT ClassID;
};
#if NW_HOOK_MODE
#define GDefaultCharacters (reinterpret_cast<const NDefaultCharacter*>(::nw::Addr(0x101133b8)))
#else
extern const NDefaultCharacter GDefaultCharacters[];
#endif

// UE2's appSeconds (UnVcWin32.h): the time stamp counter in seconds, plus 2^24. Inline in the
// stock code (OnPaint here), and one copy out of line that only OnStart calls.
static FORCEINLINE DOUBLE SecondsNow()
{
    unsigned __int64 Cycles = __rdtsc();
    DWORD L = (DWORD)Cycles;
    DWORD H = (DWORD)(Cycles >> 32);
    return ((DOUBLE)L + 4294967296.0 * (DOUBLE)H) * GSecondsPerCycle + 16777216.0;
}
static __declspec(noinline) DOUBLE appSeconds()
{
    return SecondsNow();
}
NW_HOOK_FUNC(0x10016620, appSeconds)

// a label centered in a Width x 16 box at (X, Y) (inline in the stock OnPaint)
static FORCEINLINE void DrawLabel(NCGDevice* Device, const TCHAR* Text, INT BoxW, INT X, INT Y)
{
    SIZE Size;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((BoxW - Size.cx) / 2 + X, (16 - Size.cy) / 2 + Y, 0xFFFFFFFF, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
}

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

// 0x1008e0f0 - the bow share's edit line is left as allocated
NCPawnViewr_Simulator::NCPawnViewr_Simulator()
{
    CountEdit = NULL;
    SkillEdit = NULL;
    CancelEdit = NULL;
    ReplaceEdit = NULL;
    StartButton = NULL;
    Pawns.First = NULL;
    Pawns.Last = NULL;
    Pawns.End = NULL;
}
NW_HOOK_CTOR(0x1008e0f0, NCPawnViewr_Simulator)

// 0x1008e190 - the pawn vector's storage freed (its pawns are not destroyed)
NW_HOOK_DTOR(0x1008e190, NCPawnViewr_Simulator)

// 0x10088770 (vslot 24)
INT NCPawnViewr_Simulator::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND && wParam == NID_SIMULATOR_START)
        return OnStart() != 0;
    return 0;
}
NW_HOOK_METHOD(0x10088770, NCPawnViewr_Simulator, WindowProc)

// 0x1008d150 (vslot 35) - the five edit lines and "Start"; the active skills of level 1 that
// have an animation (SkillIDs holds 2000, unchecked)
INT NCPawnViewr_Simulator::OnCreate()
{
    NCWnd::OnCreate();
    CountEdit = new NCEditBox(1, NULL, 1, 64);
    CountEdit->SetWindowRect(100, 30, 60, 16);
    CreateChild(CountEdit, NWS_VISIBLE);
    SkillEdit = new NCEditBox(1, NULL, 1, 64);
    SkillEdit->SetWindowRect(100, 60, 60, 16);
    CreateChild(SkillEdit, NWS_VISIBLE);
    CancelEdit = new NCEditBox(1, NULL, 1, 64);
    CancelEdit->SetWindowRect(100, 90, 60, 16);
    CreateChild(CancelEdit, NWS_VISIBLE);
    ReplaceEdit = new NCEditBox(1, NULL, 1, 64);
    ReplaceEdit->SetWindowRect(100, 120, 60, 16);
    CreateChild(ReplaceEdit, NWS_VISIBLE);
    BowEdit = new NCEditBox(1, NULL, 1, 64);
    BowEdit->SetWindowRect(70, 150, 30, 16);
    CreateChild(BowEdit, NWS_VISIBLE);
    StartButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    StartButton->ID = NID_SIMULATOR_START;
    StartButton->SetWindowRect(100, 180, 35, 16);
    CreateChild(StartButton, NWS_VISIBLE);
    StartButton->SetTextID(STR_Start);

    NumSkills = 0;
    NDataMap<FL2MagicSkillData>* Map = GL2GameData.SkillTable;
    for (INT i = 0; i < Map->Pairs.Num(); i++)
    {
        FL2MagicSkillData* Skill = Map->Pairs(i).Value;
        if (!Skill)
            continue;
        FName None(TEXT("None"), FNAME_Add);
        if (Skill->AnimName != None && Skill->Level == 1 && Skill->OperateType != 2)
            SkillIDs[NumSkills++] = Skill->SkillID;
    }
    return 1;
}
NW_HOOK_METHOD(0x1008d150, NCPawnViewr_Simulator, OnCreate)

// stock body shared with NCLobbyWnd's (0x1006a510)
INT NCPawnViewr_Simulator::OnDestroy()
{
    DestroyChild(this, CountEdit);
    DestroyChild(this, SkillEdit);
    DestroyChild(this, CancelEdit);
    DestroyChild(this, ReplaceEdit);
    DestroyChild(this, BowEdit);
    DestroyChild(this, StartButton);
    NCWnd::OnDestroy();
    return 1;
}

// 0x1008bc10 (vslot 50) - the labels ("PC count", "skill use %", "skill cancel %", "PC
// delete %", "bow %"), then, while running, each pawn's turn: an action, else a cancel,
// else a replacement. The origin goes on before the clip.
INT NCPawnViewr_Simulator::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewr_Simulator::OnPaint);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    DrawLabel(Device, TEXT("PC\xAC1C\xC218:"), 80, 20, 30);
    DrawLabel(Device, TEXT("\xC2A4\xD0AC\xC0AC\xC6A9%:"), 80, 20, 60);
    DrawLabel(Device, TEXT("\xC2A4\xD0AC\xC911\xB2E8%:"), 80, 20, 90);
    DrawLabel(Device, TEXT("PC\xC0AD\xC81C%:"), 80, 20, 120);
    DrawLabel(Device, TEXT("\xD65C%:"), 45, 25, 150);
    Device->EndPaint();
    Device->ReleaseClip();
    if (bRunning)
    {
        DeltaTime = SecondsNow() - LastTime;
        LastTime = DeltaTime + LastTime;
        for (INT i = 0; i < NumPawns; i++)
        {
            if (!TryAction(i) && !TryCancel(i))
                TryReplace(i);
        }
    }
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008bc10, NCPawnViewr_Simulator, OnPaint)

// 0x1008de40 (vslot 62) - the count and the percentages read; running (never, see bRunning)
// the crowd grows or shrinks, else it is made anew in reserved storage. Growing writes past
// the storage reserved.
INT NCPawnViewr_Simulator::OnStart()
{
    if (!CountEdit->CommitText() || !SkillEdit->CommitText() || !CancelEdit->CommitText() ||
        !ReplaceEdit->CommitText() || !BowEdit->CommitText())
        return 1;
    INT Count = appAtoi(CountEdit->CommitText());
    SkillRate = appAtof(SkillEdit->CommitText());
    CancelRate = appAtof(CancelEdit->CommitText());
    ReplaceRate = appAtof(ReplaceEdit->CommitText());
    BowRate = appAtof(BowEdit->CommitText());
    if (bRunning)
    {
        LastTime = appSeconds();
        if (NumPawns < Count)
        {
            for (INT i = NumPawns; i < Count; i++)
                Pawns.First[i] = SpawnPawn();
            NumPawns = Count;
            return 1;
        }
        for (INT i = Count; i < NumPawns; i++)
            GameLevel()->DestroyActor(Pawns.First[i].Pawn, 0);
        NumPawns = Count;
        return 1;
    }
    NumPawns = Count;
    SkillRate = appAtof(SkillEdit->CommitText());
    CancelRate = appAtof(CancelEdit->CommitText());
    ReplaceRate = appAtof(ReplaceEdit->CommitText());
    BowRate = appAtof(BowEdit->CommitText());
    LastTime = appSeconds();
    Pawns.Reserve(NumPawns);
    for (INT i = 0; i < NumPawns; i++)
        Pawns.First[i] = SpawnPawn();
    return 1;
}
NW_HOOK_METHOD(0x1008de40, NCPawnViewr_Simulator, OnStart)

// 0x1008bf30 (vslot 63) - a random default character up to 300 units from the player (on
// the half circle of angles 0..pi) 50 higher, with a controller of its own: by the bow share
// one of six characters with item 273 in both hands, else any of the 18 with item 1 in the
// right hand. The stock indexes the six with a negated factor.
NSimPawn NCPawnViewr_Simulator::SpawnPawn()
{
    User Character;
    INT BowCharacters[6] = {0, 1, 4, 5, 7, 8};
    NSimPawn Result;
    INT Index;
    memset(&Character, 0, sizeof(User));
    if (appFrand() * 100.0f < BowRate)
    {
        Index = BowCharacters[-(INT)(appFrand() * -5.94f)];
        Result.Kind = 2;
        Character.Equipped[BODYSLOT_LRHand] = ITEM_BowWeapon;
    }
    else
    {
        Index = (INT)(appFrand() * 17.82f);
        Result.Kind = 1;
        Character.Equipped[BODYSLOT_RHand] = ITEM_OneHandWeapon;
    }
    Character.Race = GDefaultCharacters[Index].Race;
    Character.Sex = GDefaultCharacters[Index].Sex;
    Character.ClassID = GDefaultCharacters[Index].ClassID;
    ULevel* Level = GameLevel();
    FVector Location = NPawnView::PlayerPawn()->Location;
    Location.Z += 50.0f;
    DOUBLE Angle = appFrand() * 3.141592653589793;
    DOUBLE Distance = appFrand() * 300.0f;
    Location.X = (FLOAT)(appCos(Angle) * Distance + Location.X);
    Location.Y = (FLOAT)(appSin(Angle) * Distance + Location.Y);
    FName ControllerClass = FName(TEXT("Engine.PlayerController"), FNAME_Add);
    AController* Controller = (AController*)Level->SpawnActor(
        UObject::StaticLoadClass(AController::StaticClass(), NULL, *ControllerClass, NULL, 1, NULL),
        NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 1, NULL, NULL);
    UClass*& PawnClass = Controller->PawnClass;
    PawnClass = UObject::StaticLoadClass(APawn::StaticClass(), NULL,
                                         Character.GetClassNamePointer(), NULL, 0, NULL);
    Controller->Pawn = (APawn*)Level->SpawnActor(
        PawnClass, NAME_None, Location, FRotator(0, 0, 0), NULL, 1, 0, Controller, NULL);
    Controller->Pawn->Controller = Controller;
    Controller->Role = ROLE_Authority;
    Controller->Pawn->Role = ROLE_Authority;
    Controller->Pawn->setPhysics(1, NULL, FVector(0.f, 0.f, 1.f));
    Character.Pawn = Controller->Pawn;
    Character.Unknown8 = 0;
    Character.SetPawnResource();
    APawn* Pawn = Controller->Pawn;
    Pawn->PlayAnim(0, Pawn->GetWaitAnimName(), 1.0f, 0.0f, 1, 0);
    Result.Pawn = Controller->Pawn;
    return Result;
}
NW_HOOK_METHOD(0x1008bf30, NCPawnViewr_Simulator, SpawnPawn)

// 0x1008d590 (vslot 64) - by the skill share of the frame, a pawn not casting picks a skill
// (the first kind: any of the table; the bow kind: one of four, or an attack) and a random
// other pawn to use it on. The target is drawn again until it is another live pawn: with
// no other live pawn the loop never ends. Both random indexes use negated factors.
INT NCPawnViewr_Simulator::TryAction(INT Index)
{
    INT BowSkills[6] = {56, 19, 24, 101, 0, 0};
    INT SkillID = -1;
    if (SkillRate * DeltaTime < appFrand() * 100.0f)
        return 1;
    NSimPawn& Sim = Pawns.First[Index];
    if (Sim.Kind == 0 || Sim.Pawn->ActionType == 5)
        return 1;
    if (Sim.Kind == 1)
        SkillID = SkillIDs[-(INT)(appFrand() * (DOUBLE)NumSkills * -0.99f)];
    else if (Sim.Kind == 2)
        SkillID = BowSkills[-(INT)(appFrand() * -5.94f)];
    INT Target;
    do
        Target = (INT)(appFrand() * (DOUBLE)NumPawns);
    while (Target == Index || Pawns.First[Target].Kind == 0 || Target >= NumPawns);

    if (SkillID == 0)
    {
        // an attack (the pawn's fields of the L2 action)
        APawn* Pawn = Pawns.First[Index].Pawn;
        APawn* TargetPawn = Pawns.First[Target].Pawn;
        Pawn->Controller->Unknown638 |= 2;
        Pawn->Controller->Unknown638 |= 4;
        Pawn->Controller->StopMove();
        Pawn->NActionStop();
        Pawn->ActionTarget = TargetPawn;
        Pawn->ActionType = (Pawn->WeaponType == 5) + 1;   // 5: a bow
        DWORD& Bits = Pawn->Unknown62C;
        Bits = (Bits & ~0xF) | ((((Target % 3 == 0) << 2) | (Target % 2 & 1)) << 1);
        Pawn->Unknown628 = 1;
        Pawn->Unknown63C.Empty(12);
        Pawn->Unknown634 = 0;
        Pawn->Unknown638 = 0;
        Pawn->Unknown5C0 = 1;
        return 1;
    }
    if (SkillID == -1)
        return 1;
    APawn* Pawn = Pawns.First[Index].Pawn;
    APawn* TargetPawn = Pawns.First[Target].Pawn;
    Pawn->Controller->StopMove();
    FL2MagicSkillData* Skill = GL2GameData.GetMSData(SkillID, 1);
    if (!Skill)
        return 1;
    Pawn->SetMagicInfo(SkillID, 1, Skill->HitTime, Skill->HitTime, TargetPawn, 1.0f);
    Pawn->MagicTargets.AddItem(TargetPawn);
    if (Pawn->InitSkillProcess())
    {
        Pawn->ActionTarget = TargetPawn;
        Pawn->ActionType = 5;
        return 1;
    }
    Pawn->ActionType = 0;
    return 1;
}
NW_HOOK_METHOD(0x1008d590, NCPawnViewr_Simulator, TryAction)

// 0x1008d830 (vslot 65) - by the cancel share, the pawn's cast stops
INT NCPawnViewr_Simulator::TryCancel(INT Index)
{
    FLOAT Roll = appFrand() * 100.0f;
    if (CancelRate * DeltaTime > Roll)
    {
        Pawns.First[Index].Pawn->MagicCancel();
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x1008d830, NCPawnViewr_Simulator, TryCancel)

// 0x1008d880 (vslot 66) - by the replacement share, another pawn takes its place (the first
// one never goes); its controller is flagged unless it is the player's
INT NCPawnViewr_Simulator::TryReplace(INT Index)
{
    if (Index == 0)
        return 0;
    FLOAT Roll = appFrand() * 100.0f;
    if (ReplaceRate * DeltaTime > Roll)
    {
        AController* Controller = Pawns.First[Index].Pawn->Controller;
        if (Controller != ViewActor())
            Controller->Unknown98 |= 4;
        GameLevel()->DestroyActor(Pawns.First[Index].Pawn, 0);
        Pawns.First[Index] = SpawnPawn();
    }
    return 1;
}
NW_HOOK_METHOD(0x1008d880, NCPawnViewr_Simulator, TryReplace)

NW_CHECK_VTABLE(NCPawnViewr_Simulator, 0x100eb818, 67)

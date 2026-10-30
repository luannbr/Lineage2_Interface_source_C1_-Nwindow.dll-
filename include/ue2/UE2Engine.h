/*=============================================================================
    UE2Engine.h - the part of Engine.dll (Lineage II C1 build) that NWindow uses.
    Same rules as UE2Core.h: ENGINE_API declarations must match Engine.dll's
    decorated exports, virtual lists follow the vtables in Engine.dll.
=============================================================================*/
#pragma once

#include "UE2Core.h"

class UMaterial;
class UTexture;
class UFont;
class UViewport;
class UClient;
class URenderDevice;
class AActor;
class FLevelSceneNode;
class FRenderInterface;
class FL2ColorFontInfo;
class UL2Font;
struct User;
class ULevel;
class APawn;
class ABrush;
class ALevelInfo;
class APlayerController;
class UNetConnection;
class UChannel;
class FURL;
class FMemStack;
struct FCheckResult;
class AController;
class UCanvas;

// the types the generated virtual lists name (include/ue2/gen/*.vt.inc)
#include "gen/EngineFwd.gen.h"

/*-----------------------------------------------------------------------------
    Math (UnMath.h).
-----------------------------------------------------------------------------*/

class FVector
{
public:
    FLOAT X, Y, Z;
    FVector() {}
    FVector(FLOAT InX, FLOAT InY, FLOAT InZ) : X(InX), Y(InY), Z(InZ) {}
    CORE_API FLOAT Size() const;
    // two Core calls: ?SafeNormal@FVector@@QBE?AV1@XZ (a __thiscall that returns the 12-byte vector through a hidden pointer:
    // the pointer is the first stack argument, `ret 4`, `this` in ecx) and ?GetMax@FVector@@QBEMXZ (the largest component, in ST0). Core.dll's own x87 code: not rewritten
    CORE_API FVector SafeNormal() const;
    CORE_API FLOAT GetMax() const;
    // the Core calls of the polygon code, each of them Core.dll's own x87 code, never rewritten; the decorated name is what the import library resolves
    CORE_API FLOAT SizeSquared() const;                          // ?SizeSquared@FVector@@QBEMXZ: ((x*x + y*y) + z*z), left UNROUNDED in st0 (Core 0x1010C9C0)
    CORE_API INT IsZero() const;                                 // ?IsZero@FVector@@QBEHXZ: the three components == 0.0f (test ah,0x44; jp)
    CORE_API INT Normalize();                                    // ?Normalize@FVector@@QAEHXZ: in place; 0 (and nothing written) when the length is too small
    CORE_API FVector TransformVectorBy(const FCoords& Coords) const;   // ?TransformVectorBy@FVector@@QBE?AV1@ABVFCoords@@@Z: FCoords MUST be a `class` (the 'V' of the name)
    CORE_API INT operator==(const FVector& Other) const;         // ??8FVector@@QBEHABV0@@Z: IEEE, per component (a NaN is different)
    CORE_API INT operator!=(const FVector& Other) const;         // ??9FVector@@QBEHABV0@@Z
};

class FPlane : public FVector
{
public:
    FLOAT W;
    CORE_API FPlane operator*(FLOAT Scale) const;                // ??DFPlane@@QBE?AV0@M@Z: the four components times Scale (a hidden result pointer)
};

// a sphere is a plane's four floats (X, Y, Z and the radius in W): passed BY VALUE to FConvexVolume::SphereCheck
class FSphere : public FPlane
{
};
NW_ASSERT_SIZE(FSphere, 16);

// UE2's FCoords (UnMath.h): an origin and three axes, 0x30 bytes. A `class` in the Engine (the mangled name of FVector::TransformVectorBy says 'V')
class FCoords
{
public:
    FVector Origin, XAxis, YAxis, ZAxis;
};
NW_ASSERT_SIZE(FCoords, 0x30);

// what UModel::Transform builds for FPoly::Transform (Core's ??0FModelCoords@@QAE@XZ + ABrush::BuildCoords): the point transform at +0 and the vector transform at +0x30 are the only
// parts FPoly::Transform reads (VERIFIED in the assembly); the real object may be larger
struct FModelCoords
{
    FCoords PointXform, VectorXform;
};
NW_ASSERT_SIZE(FModelCoords, 0x60);

class FRotator
{
public:
    INT Pitch, Yaw, Roll;
    FRotator() {}
    FRotator(INT InPitch, INT InYaw, INT InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
};
NW_ASSERT_SIZE(FRotator, 12);

// A collision check's result (UnPrim.h, FIteratorActorList's two pointers first; the NPC
// viewer traces the ground with one).
struct FCheckResult
{
    FCheckResult* Next;
    AActor* Actor;
    FVector Location;
    FVector Normal;
    UObject* Primitive;
    FLOAT Time;
    INT Item;
    UObject* Material;

    FCheckResult(FLOAT InTime = 0.f, FCheckResult* InNext = NULL)
        : Next(InNext), Actor(NULL), Location(0, 0, 0), Normal(0, 0, 0), Primitive(NULL),
          Time(InTime), Item(-1), Material(NULL) {}
};
NW_ASSERT_SIZE(FCheckResult, 0x30);

/*-----------------------------------------------------------------------------
    L2 text rendering enums (values still being mapped).
-----------------------------------------------------------------------------*/

enum L2FontType { L2FONT_Normal = 0 };
enum TargetRenderType { TARGETRENDER_Default = 0 };

/*-----------------------------------------------------------------------------
    UTexture. Real chain: UTexture : UBitmapMaterial : URenderedMaterial :
    UMaterial : UObject. Only the fields NWindow reads are placed, the rest is
    padding until decompiled code needs it.
-----------------------------------------------------------------------------*/

// UE2's texture formats (UnTex.h)
enum ETextureFormat
{
    TEXF_P8, TEXF_RGBA7, TEXF_RGB16, TEXF_DXT1, TEXF_RGB8, TEXF_RGBA8, TEXF_NODATA, TEXF_DXT3,
    TEXF_DXT5, TEXF_L8, TEXF_G16, TEXF_RRRGGGBBB,
};

class UTexture : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }
    // mip level Miplevel decoded to 32 bpp BGRA (Data from GMalloc, freed by the caller)
    ENGINE_API ETextureFormat ConvertDXT(INT Miplevel, INT ForceRGBA, INT Use4444, BYTE** Data);

    BYTE _Unknown28[0x38 - 0x28];
    BYTE Format;  // +0x38 ETextureFormat (UBitmapMaterial)
    BYTE _Unknown39[0x40 - 0x39];
    INT USize;   // +0x40 (tentative name: NWindow skips textures where both are 0)
    INT VSize;   // +0x44
    BYTE _Unknown48[0x80 - 0x48];
    DWORD TextureFlags;  // +0x80 (name ours) bits: the cool time frames get bit 1
    BYTE _Unknown84[0xA4 - 0x84];
    FLOAT UnknownA4;     // +0xA4 the shortcut bar sets 9 on its toggle skill frame before each
                         //   draw (an animation rate?)

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(UTexture, Format, 0x38);
NW_ASSERT_OFFSET(UTexture, USize, 0x40);
NW_ASSERT_OFFSET(UTexture, VSize, 0x44);
NW_ASSERT_OFFSET(UTexture, TextureFlags, 0x80);
NW_ASSERT_OFFSET(UTexture, UnknownA4, 0xA4);

// Classes NWindow only names (the pawn viewer finds and loads hair meshes and their skins)
class UMaterial : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

// UE2's bounding box (UnMath.h): what UPrimitive::GetRenderBoundingBox returns
class FBox
{
public:
    FVector Min;
    FVector Max;
    BYTE IsValid;
};
NW_ASSERT_SIZE(FBox, 0x1C);

// the base of the meshes (UE2's; its fields are not declared)
class UPrimitive : public UObject
{
public:
#include "gen/UPrimitive.vt.inc"
};

class UMesh : public UPrimitive
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/UMesh.vt.inc"

private:
    ENGINE_API static UClass PrivateStaticClass;
};

class UMeshAnimation;

class USkeletalMesh : public UMesh
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

    BYTE _Unknown28[0x23C - 0x28];
    UMeshAnimation* DefaultAnim;    // +0x23C (UE2's) its animation set: the constructor clears it,
                                    //   Serialize keeps it, USkeletalMeshInstance plays from it

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(USkeletalMesh, DefaultAnim, 0x23C);

// One sequence of an animation set (UE2's FMeshAnimSeq, 0x40 bytes here: UMeshAnimation::
// GetAnimSeq steps by it), the name first
struct FMeshAnimSeq
{
    FName Name;                     // +0x00
    BYTE _Unknown4[0x40 - 0x4];
};
NW_ASSERT_SIZE(FMeshAnimSeq, 0x40);

// UE2's animation set of a skeletal mesh (its copy constructor copies UT2003's InternalVersion,
// RefBones, Moves and AnimSeqs, in that order); only what NWindow reads
class UMeshAnimation : public UObject
{
public:
    BYTE _Unknown28[0x44 - 0x28];
    TArray<FMeshAnimSeq> AnimSeqs;  // +0x44 (UE2's)
};
NW_ASSERT_OFFSET(UMeshAnimation, AnimSeqs, 0x44);

// One animation channel of a skeletal mesh instance (UE2's MeshAnimChannel, 0x70 bytes here)
struct MeshAnimChannel
{
    BYTE _Unknown0[0xC];
    FLOAT AnimRate;                 // +0x0C (USkeletalMeshInstance::GetActiveAnimRate)
    BYTE _Unknown10[0x20 - 0x10];
    FLOAT Unknown20;                // +0x20 SetAnimRate makes AnimRate the rate times it
    BYTE _Unknown24[0x70 - 0x24];
};
NW_ASSERT_SIZE(MeshAnimChannel, 0x70);

// The instance of a skeletal mesh an actor plays (AActor::MeshInstance); only its channels
class USkeletalMeshInstance : public UObject
{
public:
    BYTE _Unknown28[0x114 - 0x28];
    TArray<MeshAnimChannel> Blends; // +0x114 (UE2's name) its channels (GetAnimChannelCount,
                                    //   SetAnimRate)
};
NW_ASSERT_OFFSET(USkeletalMeshInstance, Blends, 0x114);

/*-----------------------------------------------------------------------------
    Actors, players and the level: only the members NWindow touches. Field
    offsets come from Engine.dll code (noted per field), virtual lists from its
    vtables (tools/engine_vtables.py). Never embed or allocate these.
-----------------------------------------------------------------------------*/

// enum values from UE2 (UT2003 headers), not checked against L2 yet
enum ELevelTick { LEVELTICK_TimeOnly = 0, LEVELTICK_ViewportsOnly = 1, LEVELTICK_All = 2 };
enum ENetRole { ROLE_None, ROLE_DumbProxy, ROLE_SimulatedProxy, ROLE_AutonomousProxy, ROLE_Authority };
enum EDetailMode { DM_Low, DM_High, DM_SuperHigh };
enum ESoundOcclusion { OCCLUSION_Default, OCCLUSION_None, OCCLUSION_BSP, OCCLUSION_StaticMeshes };
enum EAcceptConnection { ACCEPTC_Reject, ACCEPTC_Accept, ACCEPTC_Ignore };

class AActor : public UObject
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/AActor.vt.inc"

    BYTE _Unknown28[0x5C - 0x28];
    INT Unknown5C;      // +0x5C bumped when the chat balloon over a pawn changes (NCConsole)
    BYTE _Unknown60[0x94 - 0x60];
    INT CreatureID;     // +0x94 server id (ULevel::FindActorWithCreatureID)
    DWORD Unknown98;    // +0x98 bits: the pawn simulator sets bit 2 on a pawn's controller
                        //   before destroying the pawn
    BYTE _Unknown9C[0xED - 0x9C];
    BYTE Role;          // +0xED ENetRole: the viewers make what they spawn ROLE_Authority
    BYTE _UnknownEE[0x118 - 0xEE];
    ULevel* XLevel;     // +0x118 (AActor::GetLevel, exported inline)
    BYTE _Unknown11C[0x138 - 0x11C];
    UMesh* Mesh;        // +0x138 (the viewers list its animations)
    BYTE _Unknown13C[0x14C - 0x13C];
    UMesh* HairMeshes[2];        // +0x14C (name ours) the hair change window's second mesh,
                                 //   then its first
    BYTE _Unknown154[0x19C - 0x154];
    UObject* MeshInstance;      // +0x19C the mesh's instance (a USkeletalMeshInstance for the
                                 //   viewers' pawns: the NPC viewer rescales its first channel)
    BYTE _Unknown1A0[0x2C8 - 0x1A0];
    FVector Location;   // +0x2C8 (NWindow: /loc prints the player's)
    FRotator Rotation;  // +0x2D4 (NCPawnSelectWnd turns the characters to the camera)
    BYTE _Unknown2E0[0x418 - 0x2E0];
    UMaterial* HairMaterials[2]; // +0x418 (name ours) the materials of HairMeshes
    BYTE _Unknown420[0x4B0 - 0x420];
    FLOAT CollisionRadius; // +0x4B0 (the NPC viewer's summon traces with it)
    FLOAT CollisionHeight; // +0x4B4 (NCLobbyWnd: the name floats above it)

    ULevel* GetLevel() const { return XLevel; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(AActor, CreatureID, 0x94);
NW_ASSERT_OFFSET(AActor, XLevel, 0x118);
NW_ASSERT_OFFSET(AActor, Location, 0x2C8);
NW_ASSERT_OFFSET(AActor, Rotation, 0x2D4);
NW_ASSERT_OFFSET(AActor, CollisionHeight, 0x4B4);
NW_ASSERT_OFFSET(AActor, Unknown5C, 0x5C);
NW_ASSERT_OFFSET(AActor, Unknown98, 0x98);
NW_ASSERT_OFFSET(AActor, Role, 0xED);
NW_ASSERT_OFFSET(AActor, Mesh, 0x138);
NW_ASSERT_OFFSET(AActor, HairMeshes, 0x14C);
NW_ASSERT_OFFSET(AActor, MeshInstance, 0x19C);
NW_ASSERT_OFFSET(AActor, HairMaterials, 0x418);
NW_ASSERT_OFFSET(AActor, CollisionRadius, 0x4B0);

class APawn : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/APawn.vt.inc"

    BYTE _Unknown4B8[0x578 - 0x4B8];
    INT Unknown578;                 // +0x578 the combined animations set 9 after SetMagicInfo,
    BYTE _Unknown57C[0x580 - 0x57C];
    INT Unknown580;                 // +0x580   and 3, their number of steps (MixAnimSteps)
    BYTE _Unknown584[0x59C - 0x584];
    TArray<AActor*> MagicTargets;   // +0x59C (name ours) the targets of the skill SetMagicInfo
                                    //   sets up (the viewers add theirs before InitSkillProcess)
    BYTE _Unknown5A8[0x5C0 - 0x5A8];
    BYTE Unknown5C0;                // +0x5C0 the simulator's attacks set 1
    BYTE _Unknown5C1[0x624 - 0x5C1];
    INT ActionType;                 // +0x624 (names ours) L2's action: 5 a skill, 1 an attack,
                                    //   2 a bow attack, 0 none (the viewers and the simulator)
    INT Unknown628;                 // +0x628 the attacks set 1
    DWORD Unknown62C;               // +0x62C bits 1..3: which attack
    AActor* ActionTarget;           // +0x630
    INT Unknown634;                 // +0x634 the attacks clear both
    INT Unknown638;                 // +0x638
    FArray Unknown63C;              // +0x63C 12-byte elements, emptied by the attacks
    BYTE _Unknown648[0x65C - 0x648];
    DWORD AnimFlags;                // +0x65C (name ours) bit 4: an animation the viewers chose
                                    //   plays instead of the idle one
    BYTE _Unknown660[0x678 - 0x660];
    BYTE Unknown678;                // +0x678 bit 2: the hair cannot change (hair change window)
    BYTE _Unknown679[0x67C - 0x679];
    INT MeshType;                   // +0x67C (names ours) its race and sex (User::GetMeshType):
                                    //   the prefix of its hair meshes' names
    INT NpcID;                      // +0x680 (the NPC viewer's pawn)
    INT LeftHandItem;               // +0x684 the item class id in its left hand (NPC viewer)
    BYTE _Unknown688[0x690 - 0x688];
    DWORD Unknown690;               // +0x690 bit 0: nothing can be dropped where it stands
    FLOAT Unknown694;               // +0x694 the rate the pawn viewer plays a chosen animation at
    FLOAT SpeedRate;                // +0x698 (names ours) the factor of its speeds (the status
                                    //   window's RunSpeed and WalkSpeed; the NPC viewer's rate)
    BYTE _Unknown69C[0x6D0 - 0x69C];
    FLOAT Speed;                    // +0x6D0 (the NPC viewer's speed line)
    BYTE _Unknown6D4[0x701 - 0x6D4];
    BYTE WeaponType;                // +0x701 (names ours) the weapon in hand, 0..7 (5: a bow):
                                    //   the index into its FName[8] animation tables (from
                                    //   +0x70C every 0x20 bytes, Engine.dll)
    BYTE _Unknown702[0x7EC - 0x702];
    FName WaitAnimName[8];          // +0x7EC by weapon type (APawn::GetWaitAnimName)
    BYTE _Unknown80C[0xDCC - 0x80C];
    FName SocialAnimName[20];       // +0xDCC (name ours) by social action, as
                                    //   UGameEngine::OnSocialAction plays them; the selection
                                    //   plays the third on a second click
    BYTE _UnknownE1C[0x1198 - 0xE1C];
    AController* Controller;        // +0x1198
    BYTE _Unknown119C[0x1310 - 0x119C];
    TArray<FName> MixAnimSteps;     // +0x1310 (name ours) the combined animation's, played from
                                    //   the last
    BYTE _Unknown131C[0x13D4 - 0x131C];
    DWORD Unknown13D4;              // +0x13D4 bit 0: set with the idle animation, cleared while
                                    //   one the viewers chose plays
    BYTE _Unknown13D8[0x142C - 0x13D8];
    TCHAR* ChatBalloon;             // +0x142C (name ours) the chat line drawn over it (NCConsole)
    BYTE _Unknown1430[0x1458 - 0x1430];
    DWORD SelectFlags;              // +0x1458 (names ours) bit 0: one of the selection's
    INT SelectIndex;                // +0x145C   characters, and its slot there (both set by
                                    //   UL2ConsoleWnd::InsertSelectCharacterInfo)

    // exported by Engine.dll too; inline in the headers NWindow was built with (the stock code
    // reads the table)
    FORCEINLINE FName GetWaitAnimName() { return WaitAnimName[WeaponType]; }

    // L2's skill use: the skill viewer's cast (NCMSView_InvenItemWnd::OnRButtonDown), the NPC
    // viewer's summon (NCNpcView_CharacterSelectWnd::OnSummon), the pawn viewer's combined
    // animations and simulator (which also cancels and stops); argument names ours
    ENGINE_API void SetMagicInfo(INT SkillID, INT Level, FLOAT HitTime, FLOAT HitTime2,
                                 AActor* Target, FLOAT P6);
    ENGINE_API INT InitSkillProcess();
    ENGINE_API void SpawnEnterEvent();
    ENGINE_API void MagicCancel();
    ENGINE_API void NActionStop();

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(APawn, Unknown578, 0x578);
NW_ASSERT_OFFSET(APawn, MagicTargets, 0x59C);
NW_ASSERT_OFFSET(APawn, Unknown5C0, 0x5C0);
NW_ASSERT_OFFSET(APawn, ActionType, 0x624);
NW_ASSERT_OFFSET(APawn, Unknown63C, 0x63C);
NW_ASSERT_OFFSET(APawn, AnimFlags, 0x65C);
NW_ASSERT_OFFSET(APawn, MeshType, 0x67C);
NW_ASSERT_OFFSET(APawn, Unknown690, 0x690);
NW_ASSERT_OFFSET(APawn, Speed, 0x6D0);
NW_ASSERT_OFFSET(APawn, WeaponType, 0x701);
NW_ASSERT_OFFSET(APawn, WaitAnimName, 0x7EC);
NW_ASSERT_OFFSET(APawn, SocialAnimName, 0xDCC);
NW_ASSERT_OFFSET(APawn, Controller, 0x1198);
NW_ASSERT_OFFSET(APawn, MixAnimSteps, 0x1310);
NW_ASSERT_OFFSET(APawn, Unknown13D4, 0x13D4);
NW_ASSERT_OFFSET(APawn, ChatBalloon, 0x142C);
NW_ASSERT_OFFSET(APawn, SelectFlags, 0x1458);

class AController : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

#include "gen/AController.vt.inc"

    BYTE _Unknown4B8[0x5DC - 0x4B8];
    APawn* Pawn;                    // +0x5DC the pawn it drives
    BYTE _Unknown5E0[0x634 - 0x5E0];
    AActor* Target;                 // +0x634 (name ours) the player's (the viewers cast at it)
    DWORD Unknown638;               // +0x638 bits: the simulator's attacks set 2 and 4
    BYTE _Unknown63C[0x650 - 0x63C];
    BYTE MoveMode;                  // +0x650 (name ours) 1 running, 0 walking (status window)
    BYTE _Unknown651[0x714 - 0x651];
    UClass* PawnClass;              // +0x714 what it spawns (the viewers load one by name)

private:
    ENGINE_API static UClass PrivateStaticClass;
};
NW_ASSERT_OFFSET(AController, Pawn, 0x5DC);
NW_ASSERT_OFFSET(AController, Target, 0x634);
NW_ASSERT_OFFSET(AController, MoveMode, 0x650);
NW_ASSERT_OFFSET(AController, PawnClass, 0x714);

// an item on the ground (NConsoleWnd::GetPickupUnderMouse)
class AL2Pickup : public AActor
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }

private:
    ENGINE_API static UClass PrivateStaticClass;
};

class APlayerController : public AController
{
};

// UE2's bases: FOutputDevice at +0x28, FExec at +0x2C (the console's commands go to the
// viewport's Exec)
class UPlayer : public UObject, public FOutputDevice, public FExec
{
public:
#include "gen/UPlayer.vt.inc"

    APlayerController* Actor;       // +0x30 (UPlayer::Destroy: Actor->GetLevel()->DestroyActor(Actor, 1))
};
NW_ASSERT_OFFSET(UPlayer, Actor, 0x30);

// UE2 input keys (the ones NWindow tests)
enum EInputKey { IK_Shift = 0x10 };

class UInput : public USubsystem
{
public:
    ENGINE_API BYTE KeyDown(INT Key);
};

class UViewport : public UPlayer
{
public:
#include "gen/UViewport.vt.inc"

    BYTE _Unknown34[0x68 - 0x34];
    UCanvas* Canvas;                // +0x68 (UE2's; UViewport::Destroy deletes it after the input
                                    //   and the console)
    UInput* Input;                  // +0x6C (NWindow: Shift held for /actions)
    URenderDevice* RenDev;          // +0x70 (UClient::Flush calls its virtual +0x7C, Flush)
    BYTE _Unknown74[0x88 - 0x74];
    INT SizeX;                      // +0x88 (NConsoleWnd::AcceptGameStart)
};
NW_ASSERT_OFFSET(UViewport, Input, 0x6C);
NW_ASSERT_OFFSET(UViewport, RenDev, 0x70);
NW_ASSERT_OFFSET(UViewport, Canvas, 0x68);
NW_ASSERT_OFFSET(UViewport, SizeX, 0x88);

class UClient : public USubsystem
{
public:
    TArray<UViewport*> Viewports;   // +0x2C
    BYTE _Unknown38[0x60 - 0x38];
    // the config properties of the client (l2.ini [WinDrv.WindowsClient]); the offsets come from
    // UClient::StaticConstructor (Engine.dll 0x10491F0A..) and from WinDrv's ToggleFullscreen /
    // EndFullscreen, which read them: window -> fullscreen opens the device at the Fullscreen size,
    // fullscreen -> window at the Windowed one
    INT WindowedViewportX;          // +0x60
    INT WindowedViewportY;          // +0x64
    INT FullscreenViewportX;        // +0x68
    INT FullscreenViewportY;        // +0x6C
    BYTE _Unknown70[0x78 - 0x70];
    FLOAT Gamma;                    // +0x78 (the video options; UEngine::UpdateGamma applies it)
};
NW_ASSERT_OFFSET(UClient, Viewports, 0x2C);
NW_ASSERT_OFFSET(UClient, WindowedViewportX, 0x60);
NW_ASSERT_OFFSET(UClient, FullscreenViewportY, 0x6C);
NW_ASSERT_OFFSET(UClient, Gamma, 0x78);

// UE2's URL (0x44 bytes: protocol, host, port, map, options, portal, valid); only built and
// handed to UEngine's Browse (NConsoleWnd::RestartToLobby)
enum ETravelType { TRAVEL_Absolute, TRAVEL_Partial, TRAVEL_Relative };
class FURL
{
public:
    ENGINE_API FURL(const TCHAR* Filename);
    ENGINE_API FURL(FURL* Base, const TCHAR* TextURL, ETravelType Type);
    ENGINE_API FURL(const FURL& Other);
    ENGINE_API ~FURL();
    ENGINE_API void LoadURLConfig(const TCHAR* Section, const TCHAR* Filename);
    ENGINE_API void AddOption(const TCHAR* Str);

private:
    BYTE _Data[0x44];
};
NW_ASSERT_SIZE(FURL, 0x44);

class FNetworkNotify
{
public:
    virtual EAcceptConnection NotifyAcceptingConnection() = 0;
    virtual void NotifyAcceptedConnection(UNetConnection* Connection) = 0;
    virtual UBOOL NotifyAcceptingChannel(UChannel* Channel) = 0;
    virtual ULevel* NotifyGetLevel() = 0;
    virtual void NotifyReceivedText(UNetConnection* Connection, const TCHAR* Text) = 0;
    virtual UBOOL NotifySendingFile(UNetConnection* Connection, FGuid GUID) = 0;
    virtual void NotifyReceivedFile(UNetConnection* Connection, INT PackageIndex, const TCHAR* Error,
                                    UBOOL Skipped) = 0;
    virtual void NotifyProgress(const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds) = 0;
};

class ULevelBase : public UObject, public FNetworkNotify
{
};

// ??_7ULevel@@6BUObject@@@: UObject 24 + 45
class ULevel : public ULevelBase
{
public:
    virtual void Modify(UBOOL DoTransArrays = 0);                                              // 24
    virtual void SetActorCollision(UBOOL bCollision, UBOOL bForce = 0);                        // 25
    virtual void Tick(ELevelTick TickType, FLOAT DeltaSeconds);                                // 26
    virtual void TickNetClient(FLOAT DeltaSeconds);                                            // 27
    virtual void L2TickNetClient(FLOAT DeltaSeconds);                                          // 28
    virtual void TickNetServer(FLOAT DeltaSeconds);                                            // 29
    virtual INT ServerTickClient(UNetConnection* Conn, FLOAT DeltaSeconds);                    // 30
    virtual void ReconcileActors();                                                            // 31
    virtual void RememberActors();                                                             // 32
    virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);                                   // 33
    virtual void ShrinkLevel();                                                                // 34
    virtual void CompactActors();                                                              // 35
    virtual UBOOL Listen(FString& Error);                                                      // 36
    virtual UBOOL IsServer();                                                                  // 37
    virtual UBOOL MoveActor(AActor* Actor, FVector Delta, FRotator NewRotation, FCheckResult& Hit,
                            UBOOL Test = 0, UBOOL IgnorePawns = 0, UBOOL bIgnoreBases = 0,
                            UBOOL bNoFail = 0);                                                // 38
    virtual UBOOL FarMoveActor(AActor* Actor, FVector DestLocation, UBOOL Test = 0,
                               UBOOL bNoCheck = 0, UBOOL bAttachedMove = 0);                   // 39
    virtual UBOOL DestroyActor(AActor* Actor, UBOOL bNetForce = 0);                            // 40
    virtual void CleanupDestroyed(UBOOL bForce);                                               // 41
    virtual AActor* SpawnActor(UClass* Class, FName InName = NAME_None,
                               FVector Location = FVector(0, 0, 0),
                               FRotator Rotation = FRotator(0, 0, 0), AActor* Template = NULL,
                               UBOOL bNoCollisionFail = 0, UBOOL bRemoteOwned = 0,
                               AActor* Owner = NULL, APawn* Instigator = NULL);                // 42
    virtual ABrush* SpawnBrush();                                                              // 43
    virtual void SpawnViewActor(UViewport* Viewport);                                          // 44
    virtual APlayerController* SpawnPlayActor(UPlayer* Viewport, ENetRole RemoteRole,
                                              const FURL& URL, FString& Error);                // 45
    virtual UBOOL FindSpot(FVector Extent, FVector& Location);                                 // 46
    virtual UBOOL CheckSlice(FVector& Location, FVector Extent, INT& bKeepTrying);             // 47
    virtual UBOOL CheckEncroachment(AActor* Actor, FVector TestLocation, FRotator TestRotation,
                                    UBOOL bTouchNotify);                                       // 48
    virtual UBOOL SinglePointCheck(FCheckResult& Hit, FVector Location, FVector Extent,
                                   DWORD ExtraNodeFlags, ALevelInfo* Level, UBOOL bActors);    // 49
    virtual UBOOL EncroachingWorldGeometry(FCheckResult& Hit, FVector Location, FVector Extent,
                                           DWORD ExtraNodeFlags, ALevelInfo* Level);           // 50
    virtual UBOOL SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End,
                                  const FVector& Start, DWORD TraceFlags, FVector Extent);     // 51
    virtual FCheckResult* GetPawnListExtent(FMemStack& Mem, INT P2, INT P3, INT P4, INT P5,
                                            INT P6);                                           // 52
    virtual FCheckResult* GetPawnListExtent2(FMemStack& Mem, AActor* SourceActor,
                                             const FVector& End, const FVector& Start,
                                             FVector Extent);                                  // 53
    virtual FCheckResult* MultiPointCheck(FMemStack& Mem, FVector Location, FVector Extent,
                                          DWORD ExtraNodeFlags, ALevelInfo* Level, UBOOL bActors,
                                          UBOOL bOnlyWorldGeometry, UBOOL bSingleResult);      // 54
    virtual FCheckResult* MultiLineCheck(FMemStack& Mem, FVector End, FVector Start,
                                         FVector Size, ALevelInfo* LevelInfo, DWORD TraceFlags,
                                         AActor* SourceActor);                                 // 55
    virtual void DetailChange(EDetailMode NewDetailMode);                                      // 56
    virtual UBOOL TickDemoRecord(FLOAT DeltaSeconds);                                          // 57
    virtual UBOOL TickDemoPlayback(FLOAT DeltaSeconds);                                        // 58
    virtual void UpdateTime(ALevelInfo* Info);                                                 // 59
    virtual UBOOL IsPaused();                                                                  // 60
    virtual UBOOL L2SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End,
                                    const FVector& Start, DWORD TraceFlags, FVector Extent);   // 61
    virtual FCheckResult* L2MultiLineCheck(FMemStack& Mem, FVector End, FVector Start,
                                           FVector Size, ALevelInfo* LevelInfo, DWORD TraceFlags,
                                           AActor* SourceActor);                               // 62
    virtual AActor* FindActorWithCreatureID(INT CreatureID);                                   // 63
    virtual void ResetActorStaticLight();                                                      // 64
    virtual void WelcomePlayer(UNetConnection* Connection, TCHAR* Optional);                   // 65
    virtual void BuildRenderData();                                                            // 66
    virtual UBOOL IsAudibleAt(FVector SoundLocation, FVector ListenerLocation, AActor* SoundActor,
                              ESoundOcclusion SoundOcclusion);                                 // 67
    virtual FLOAT CalculateRadiusMultiplier(INT P1, INT P2);                                   // 68
};

/*-----------------------------------------------------------------------------
    UCanvas. 64 virtual slots (UObject 24 + 40), order read from Engine.dll's
    ??_7UCanvas@@6B@. Most fields not mapped yet: never embed or allocate a UCanvas.

    MSVC puts overloaded virtuals next to each other in REVERSE declaration
    order, so each overload pair below is declared second-slot first.
-----------------------------------------------------------------------------*/

class UCanvas : public UObject
{
public:
    BYTE _Unknown28[0x98 - 0x28];
    UViewport* Viewport;   // +0x98 (UCanvas::Init)
    BYTE _Unknown9C[0x12C - 0x9C];
    UL2Font* L2Fonts[32];  // +0x12C indexed by L2FontType (UCanvas::GetTextExtent); count not known

    virtual void Init(UViewport* InViewport);                                                   // 24 +0x60
    virtual void Update();                                                                      // 25 +0x64
    virtual void DrawTile(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U,
                          FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z, FPlane Color, FPlane Fog);       // 26 +0x68
    virtual void DrawIcon(UMaterial* Material, FLOAT ScreenX, FLOAT ScreenY, FLOAT XSize,
                          FLOAT YSize, FLOAT Z, FPlane Color, FPlane Fog);                      // 27 +0x6c
    virtual void DrawPattern(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL,
                             FLOAT Scale, FLOAT OrgX, FLOAT OrgY, FLOAT Z, FPlane Color,
                             FPlane Fog);                                                       // 28 +0x70
    virtual void VARARGS WrappedStrLenf(UFont* Font, INT& XL, INT& YL, const TCHAR* Fmt, ...);  // 30 +0x78
    virtual void VARARGS WrappedStrLenf(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL,
                                        INT& YL, const TCHAR* Fmt, ...);                        // 29 +0x74
    virtual void VARARGS WrappedPrintf(UFont* Font, UBOOL Center, const TCHAR* Fmt, ...);       // 32 +0x80
    virtual void VARARGS WrappedPrintf(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center,
                                       const TCHAR* Fmt, ...);                                  // 31 +0x7c
    virtual void ReleaseClip();                                                                 // 33 +0x84
    virtual void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, UTexture* Tex);                 // 35 +0x8c
    virtual void DrawStretchedTex(INT X, INT Y, INT XL, INT YL, INT SrcX, INT SrcY, INT SrcXL,
                                  INT SrcYL, UTexture* Tex);                                    // 34 +0x88
    virtual void DrawTexture(INT X, INT Y, INT XL, INT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL,
                             UTexture* Tex);                                                    // 36 +0x90
    virtual void DrawLine(INT X1, INT Y1, INT X2, INT Y2, DWORD Color, INT Width);              // 37 +0x94
    virtual void DrawActor(AActor* Actor, INT X, INT Y, INT Z);                                 // 38 +0x98
    virtual void DrawRect(INT X, INT Y, INT XL, INT YL, UTexture* Tex, DWORD Color);            // 39 +0x9c
    virtual DWORD DrawText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                           DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11, INT P12,
                           DWORD P13, INT P14, INT P15, INT P16,
                           TArray<FL2ColorFontInfo>* ColorInfo);                                // 40 +0xa0
    virtual DWORD DrawHtmlText(INT X, INT Y, DWORD Color, const TCHAR* Text, DWORD P5, DWORD P6,
                               DWORD P7, FLOAT P8, INT P9, L2FontType FontType, INT P11,
                               INT P12, DWORD P13, INT P14);                                    // 41 +0xa4
    virtual void DrawBrightenTile(UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL,
                                  FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z, FPlane Color,
                                  FPlane Fog);                                                  // 42 +0xa8
    virtual void DrawBrightenTexture(INT X, INT Y, INT XL, INT YL, FLOAT U, FLOAT V, FLOAT UL,
                                     FLOAT VL, UTexture* Tex, DWORD Brightness);                // 43 +0xac
    virtual DWORD GetTextExtent(const TCHAR* Text, SIZE* Size, DWORD P3, DWORD P4, INT P5,
                                L2FontType FontType, INT P7, DWORD P8);                         // 44 +0xb0
    virtual void Draw3DCoordText(FVector Loc, DWORD Color, TCHAR* Text, UTexture* Tex, INT P5,
                                 INT P6, L2FontType FontType, INT P8);                          // 46 +0xb8
    virtual void Draw3DCoordText(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                                 DWORD Color, TCHAR* Text, UTexture* Tex, INT P7, INT P8,
                                 L2FontType FontType, INT P10);                                 // 45 +0xb4
    virtual void DrawTargetName(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                                DWORD Color, User* Target, TargetRenderType RenderType,
                                L2FontType FontType);                                           // 47 +0xbc
    virtual void DrawTargetOptionName(FLevelSceneNode* SceneNode, FRenderInterface* RI,
                                      FVector Loc, DWORD Color, User* Target,
                                      TargetRenderType RenderType, L2FontType FontType);        // 48 +0xc0
    virtual void DrawTargetTex(FLOAT P1, FLOAT P2, FLOAT P3, FLOAT P4, FLOAT P5, FLOAT P6,
                               FLOAT P7, FLOAT P8, FLOAT P9, UTexture* Tex);                    // 49 +0xc4
    virtual void DrawDepthBar(FLevelSceneNode* SceneNode, FRenderInterface* RI, FVector Loc,
                              DWORD P4, DWORD P5, DWORD P6);                                    // 50 +0xc8
    virtual DWORD DrawSpecialDigit(INT DigitWidth, INT X, INT Y, DWORD Color, TCHAR* Text,
                                   UTexture* DigitTexture);                                     // 51 +0xcc
    virtual void ImportPledgeCrest(INT P1, INT P2);                                             // 52 +0xd0
    virtual void ImportAllyCrest(INT P1, INT P2);                                               // 53 +0xd4
    virtual void WrapStringToArray(const TCHAR* Text, TArray<FString>* OutArray, FLOAT Width,
                                   UFont* Font, TCHAR EOL);                                     // 54 +0xd8
    virtual void ClippedStrLen(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL,
                               const TCHAR* Text);                                              // 55 +0xdc
    virtual void ClippedPrint(UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Checked,
                              const TCHAR* Text);                                               // 56 +0xe0
    virtual void DrawTileStretched(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT AWidth,
                                   FLOAT AHeight);                                              // 57 +0xe4
    virtual void DrawTileScaled(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT NewXScale,
                                FLOAT NewYScale);                                               // 58 +0xe8
    virtual void DrawTileBound(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                               FLOAT Height);                                                   // 59 +0xec
    virtual void DrawTileJustified(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                                   FLOAT Height, BYTE Justification);                           // 60 +0xf0
    virtual void DrawTileScaleBound(UMaterial* Material, FLOAT Left, FLOAT Top, FLOAT Width,
                                    FLOAT Height);                                              // 61 +0xf4
    virtual void VARARGS DrawTextJustified(BYTE Justification, FLOAT X1, FLOAT Y1, FLOAT X2,
                                           FLOAT Y2, const TCHAR* Fmt, ...);                    // 62 +0xf8
    virtual void SetClip(INT X, INT Y, INT XL, INT YL);                                         // 63 +0xfc

    // non-virtual L2 additions exported by Engine.dll
    ENGINE_API void DrawRadar();
    ENGINE_API void DrawRadarTarget(INT X, INT Y);
    ENGINE_API void DrawMinimapLoc(INT X, INT Y, UTexture* Tex);
};
NW_ASSERT_OFFSET(UCanvas, Viewport, 0x98);
NW_ASSERT_OFFSET(UCanvas, L2Fonts, 0x12C);

/*-----------------------------------------------------------------------------
    UL2Font: L2's font interface. Abstract in Engine.dll (slots 25-28 pure);
    NWindow's NL2Font implements it (names ours).
-----------------------------------------------------------------------------*/

struct FL2FontChar;

class UL2Font : public USubsystem
{
public:
    static UClass* StaticClass() { return &PrivateStaticClass; }
    ENGINE_API virtual ~UL2Font();

    virtual INT Load(const TCHAR* TextureName, const TCHAR* FontFile) = 0;   // 25
    virtual UTexture* GetTexture() = 0;                                        // 26
    virtual FL2FontChar* GetChar(TCHAR C) = 0;                                 // 27
    virtual void Clear() = 0;                                                  // 28

private:
    ENGINE_API static UClass PrivateStaticClass;
};

/*-----------------------------------------------------------------------------
    FL2TextAlignParser (L2 line breaker). sizeof 0x6C: NWindow allocates it
    with new (NCGDevice::Init). Only Width is declared here, because
    SetWidthParam is inline for NWindow; NWindow touches no other field.
    GetLineBuffer's out value is the offset in Text where the returned line
    starts (stock 0x1049F613: mov [edx], ecx with ecx = [esi+0x14]), not a length.
-----------------------------------------------------------------------------*/

class FL2TextAlignParser
{
public:
    ENGINE_API FL2TextAlignParser();
    ENGINE_API ~FL2TextAlignParser();
    ENGINE_API void Clear();
    ENGINE_API void SetParserParam(TCHAR* Text, DWORD Width, UL2Font* Font, INT P4, INT P5, INT P6);
    ENGINE_API TCHAR* GetLineBuffer(INT& LineStart);
    // exported too, but NWindow writes the field directly: inline in the headers it used
    void SetWidthParam(DWORD InWidth) { Width = InWidth; }

private:
    BYTE _Unknown00[0x48];
    DWORD Width;           // +0x48 line width in pixels
    BYTE _Unknown4C[0x6C - 0x4C];
};
NW_ASSERT_SIZE(FL2TextAlignParser, 0x6C);

/*-----------------------------------------------------------------------------
    The engine and the console base. NWindow talks to the game through the
    FL2NetNotify side of the engine (requests, interface sounds); its virtual
    list is generated from Engine.dll (include/ue2/gen).
-----------------------------------------------------------------------------*/

class FL2NetNotify
{
public:
#include "gen/FL2NetNotify.vt.inc"
};

// The engine's subsystems NWindow calls. Their virtuals are mostly pure in Engine.dll: the
// generated lists name them after the client's implementations (ALAudio.dll's
// UALAudioSubsystem, D3DDrv.dll's UD3DRenderDevice, Network.dll's UNetworkHandler).
class UAudioSubsystem : public USubsystem
{
public:
#include "gen/UAudioSubsystem.vt.inc"
};

// what URenderDevice::GetRenderCaps returns (UE2's; the video options read the pixel shader
// version)
struct FRenderCaps
{
    INT MaxSimultaneousTerrainLayers;  // +0x00
    INT PixelShaderVersion;            // +0x04 12 = 1.2
};

class URenderDevice : public USubsystem
{
public:
#include "gen/URenderDevice.vt.inc"
};

// the connection to the game server: the requests go through it
class UL2NetHandler : public USubsystem
{
public:
#include "gen/UL2NetHandler.vt.inc"
};

// UEngine : USubsystem (+0x00, FExec at +0x28), FL2NetNotify (+0x2C). Only the fields NWindow
// reads are placed.
class UEngine : public USubsystem, public FL2NetNotify
{
public:
#include "gen/UEngine.vt.inc"

    BYTE _Unknown30[0x4C - 0x30];
    UClient* Client;                // +0x4C the viewports
    UAudioSubsystem* Audio;         // +0x50 NULL without sound
    URenderDevice* GRenDev;         // +0x54
    BYTE _Unknown58[0x5C - 0x58];
    UL2NetHandler* NetHandler;      // +0x5C the requests to the server
    BYTE _Unknown60[0x164 - 0x60];
};
NW_ASSERT_OFFSET(UEngine, Client, 0x4C);
NW_ASSERT_OFFSET(UEngine, Audio, 0x50);
NW_ASSERT_OFFSET(UEngine, GRenDev, 0x54);
NW_ASSERT_OFFSET(UEngine, NetHandler, 0x5C);
NW_ASSERT_SIZE(UEngine, 0x164);   // UGameEngine's copy constructor copies its own fields from here

// The game's engine: the console's Engine is one. Only the fields NWindow reads.
class UGameEngine : public UEngine
{
public:
#include "gen/UGameEngine.vt.inc"

    ULevel* GLevel;                 // +0x164 the level being played (UE2's; UT2003's GEntry,
                                    //   GPendingLevel and LastURL follow, the FURL at +0x170)
    BYTE _Unknown168[0x1EC - 0x168];
    FVector MouseHitLocation;       // +0x1EC (names ours) where the line under the mouse hits
                                    //   the world
    BYTE _Unknown1F8[0x208 - 0x1F8];
    AActor* MouseHitActor;          // +0x208 the actor it hits, NULL for none (both as
                                    //   UGameEngine::FindMouseTargetObject leaves them)
};
NW_ASSERT_OFFSET(UGameEngine, GLevel, 0x164);
NW_ASSERT_OFFSET(UGameEngine, MouseHitLocation, 0x1EC);
NW_ASSERT_OFFSET(UGameEngine, MouseHitActor, 0x208);

// UL2ConsoleWnd::State (names ours; only the values decompiled code uses)
enum L2ConsoleState
{
    L2CS_Login = 2,                     // the login screen (back from the server list)
    L2CS_Waiting = 1,                   // (name ours) a request of the character stage sent, the
                                        //   server's answer awaited
    L2CS_None = 0,                      // (name ours) MoveCameraByState's "from anywhere"
    L2CS_Create = 3,                    // (names ours) the character creation
    L2CS_Lobby = 4,                     //   the character selection
    L2CS_InGame = 6,                    //   in the world (AcceptGameStart)
    L2CS_Loading = 7,                   //   the loading screen: SetState keeps the new state in
                                        //   SavedState (phase 6, ReturnToLogin)
};

// the engine's ItemInfo, passed by value to three console slots (NWindow's declaration of it:
// NCItemWnd.h)
struct NItemRequest;

/*-----------------------------------------------------------------------------
    Game data tables (sysstring, items, skills...). Only placed members.
-----------------------------------------------------------------------------*/

// one line of systemmsg-*.dat (name ours, 0x14 bytes)
struct FL2SystemMsgData
{
    INT Unknown0;                       // +0x00 0 when the line does not exist
    FStringNoInit Message;              // +0x04 (copied with the line: NConsoleWnd slot 145)
    INT Type;                           // +0x10 4: shown in a dialog box
};

// /useskill's table (FL2GameData::GetMSCommandData, name ours): only the first field is known
struct FL2MagicSkillCommandData
{
    INT SkillID;                        // +0x00
};

// A name of L2's own table (Engine.dll), kept by the data tables: an index, 4 bytes.
class L2FName
{
public:
    ENGINE_API const TCHAR* operator*() const;

private:
    INT Index;
};
NW_ASSERT_SIZE(L2FName, 4);

// one line of actionname-*.dat (name ours, 0x1C bytes; NCActionItemListWnd::LoadActions)
struct FL2ActionData
{
    INT bValid;                         // +0x00 0: no action here
    INT ActionID;                       // +0x04
    INT Param;                          // +0x08 the social action of the social ones
    INT Category;                       // +0x0C 1 basic, 2 party, 3 social, 4 pet, 5 summon
                                        //   (the pet and summon windows list 4 and 5)
    L2FName Name;                       // +0x10
    L2FName IconName;                   // +0x14
    L2FName Description;                // +0x18
};
NW_ASSERT_SIZE(FL2ActionData, 0x1C);

// one level of a skill (FL2GameData::GetMSData, name ours): only the fields NWindow reads
struct FL2MagicSkillData
{
    BYTE _Unknown0[0x4];
    INT SkillID;                        // +0x04 (the skill viewer walks the table)
    INT Level;                          // +0x08
    BYTE _UnknownC[0x14 - 0xC];
    INT OperateType;                    // +0x14 2: passive, 3: toggle, else active
    INT bMagic;                         // +0x18 shown as "Magic" instead
    INT MpCost;                         // +0x1C
    BYTE _Unknown20[0x2C - 0x20];
    INT Range;                          // +0x2C < 0: none
    BYTE _Unknown30[0x40 - 0x30];
    FLOAT HitTime;                      // +0x40 (APawn::SetMagicInfo, the skill viewer's cast)
    FName AnimName;                     // +0x44 (name ours) its animation (the pawn simulator
                                        //   casts the level-1 skills that have one)
    BYTE _Unknown48[0x4C - 0x48];
    L2FName Name;                       // +0x4C
    L2FName IconName;                   // +0x50
    L2FName Description;                // +0x54
};
NW_ASSERT_OFFSET(FL2MagicSkillData, OperateType, 0x14);
NW_ASSERT_OFFSET(FL2MagicSkillData, Range, 0x2C);
NW_ASSERT_OFFSET(FL2MagicSkillData, HitTime, 0x40);
NW_ASSERT_OFFSET(FL2MagicSkillData, Description, 0x54);
NW_ASSERT_OFFSET(FL2MagicSkillData, AnimName, 0x44);

// one line of questname-*.dat (name ours, 0x14 bytes): a quest's text at one step
struct FL2QuestNameData
{
    INT bValid;                         // +0x00 0: no quest here
    INT QuestID;                        // +0x04
    INT Step;                           // +0x08
    L2FName Name;                       // +0x0C the quest's (NConsoleWnd::AddQuestID)
    L2FName Text;                       // +0x10 the step's
};
NW_ASSERT_SIZE(FL2QuestNameData, 0x14);

// one line of castlename-*.dat (name ours, 0x14 bytes)
struct FL2CastleNameData
{
    INT bValid;                         // +0x00 0: no castle here
    INT CastleID;                       // +0x04
    L2FName Name;                       // +0x08 (the system messages' castle parameter, type 5)
    L2FName Location;                   // +0x0C (names ours, after the columns of the later
    L2FName Description;                // +0x10   clients' castlename files)
};
NW_ASSERT_SIZE(FL2CastleNameData, 0x14);

// one word of the chat filter (obscene-*.dat, FL2GameData::ObsceneDataLoad; name ours)
struct FL2ObsceneData
{
    INT Unknown0;                       // +0x00
    FString Word;                       // +0x04
};

// one line of abnormalgrp.txt + abnormalname-*.txt (name ours, 0x14 bytes): a buff/debuff's icon and text
// (AbnormalDataLoad writes bValid, AbnormalID and the icon; AbnormalNameDataLoad adds name and description
// to the line with the same abnormal_id)
struct FL2AbnormalData
{
    INT bValid;                         // +0x00 0: no abnormal status here
    INT AbnormalID;                     // +0x04 abnormal_id
    L2FName Name;                       // +0x08 name (abnormalname)
    L2FName Description;                // +0x0C desc (abnormalname)
    L2FName IconName;                   // +0x10 icon (abnormalgrp)
};
NW_ASSERT_SIZE(FL2AbnormalData, 0x14);

// the data tables' hash maps and entries (NWindow's declarations: NGameData.h)
template <class T> struct NDataMap;
struct NViewItemData;
struct NViewNpcData;

// The whole object is 0x3AF90 bytes. Its COPY constructor (the first FL2GameData::FL2GameData) copies it
// piece by piece, so every region below, its size and the total come from there: no gap is left. The tables with a fixed size are
// filled by their loader WITHOUT a capacity check (only LoadSymbolInfo compares, with 200): a data file
// with more rows than the array has writes into the next table. The other tables are heap hash maps
// (pointers, created by Load) and have no limit.
class FL2GameData
{
public:
    // reads Length bytes of a data file from Archive into Buffer: decrypts them with BlowFish when the archive says the file
    // is version 0xD3; otherwise (when the strict-file option is on) it tells "Files are corrupted" and asks to exit
    ENGINE_API INT GetTextInformation(FArchive* Archive, BYTE* Buffer, INT Length);

    BYTE _UObject[0x28];                // +0x00 UObject (0x28 bytes in the C1)
    INT Unknown28;                      // +0x28 (NpcDataLoad sets it to 0; nothing else reads it)
    BYTE BlowFish[0x1048];              // +0x2C FBlowFish: the cipher of the data files (18 + 4 * 256
                                        //   dwords of key schedule); GetTextInformation decrypts a
                                        //   file the archive calls version 0xD3 (Lineage2Ver211)
    DWORD HairData[1500];               // +0x1074 hairgrp.txt (HairDataLoad)
    NDataMap<NViewItemData>* ItemTable; // +0x27E4 (names ours) the items, by item id
    NDataMap<NViewNpcData>* NpcTable;   // +0x27E8 the NPCs, by npc id
    BYTE CharData[15][0x274];           // +0x27EC chargrp.txt (CharDataLoad): 15 records
    NDataMap<FL2MagicSkillData>* SkillTable; // +0x4CB8 the skills' levels
    void* MSCommandTable;               // +0x4CBC (name ours) skillname -> /useskill (MSCommandDataLoad)
    void* CommandTable;                 // +0x4CC0 (name ours) commandname-*.txt (CommandDataLoad)
    void* MusicTable;                   // +0x4CC4 (name ours) musicinfo.txt (LoadMusicData)
    TArray<FL2ObsceneData*> ObsceneData; // +0x4CC8 (name ours) the chat filter's words
                                        //   (ObsceneDataLoad, ObsceneDataClear)
    void* NpcSkillAnimTable;            // +0x4CD4 (name ours) mobskillanimgrp.txt (NpcSkillAnimDataLoad)
    FL2ActionData Actions[1000];        // +0x4CD8 actionname-*.dat
    BYTE PetActions[100][0x1C];         // +0xBA38 (name ours) PetActionName (PetActionDataLoad), 100 lines
    FL2QuestNameData QuestNames[1000];  // +0xC528 questname-*.dat
    FL2AbnormalData Abnormals[3000];    // +0x11348 abnormalgrp.txt + abnormalname-*.txt
    FL2SystemMsgData SystemMsg[3000];   // +0x1FDA8 systemmsg-*.dat (NConsoleWnd checks id < 3000)
    void* SkillSoundTable;              // +0x2E808 (name ours) skillsoundgrp.txt (SkillSndDataLoad)
    void* StaticObjectTable;            // +0x2E80C (name ours) staticobject-*.txt (LoadStaticObjectInfo)
    FString SysString[3000];            // +0x2E810 sysstring-*.dat (FL2GameData::SysStringClear)
    FString ClassInfo[100];             // +0x374B0 classinfo-*.dat (name ours): 0 the creation
                                        //   hint, 1..9 the base classes (count from the file); the
                                        //   array has room for 100 (ClassInfoClear empties 100)
    FString ClassType[100];             // +0x37960 (name ours) ClassTypeLoad: "ClassType-k.txt", a
                                        //   fixed file name (the same four columns as classinfo)
    FString NoticeText;                 // +0x37E10 (name ours) the announcements shown before
                                        //   the server list (NConsoleWnd::StartNotice)
    FString WarningNotice;              // +0x37E1C (name ours) the warning page shown after the
                                        //   announcements (WarningNoticeLoad)
    FL2CastleNameData CastleNames[100]; // +0x37E28 castlename-*.dat
    BYTE EnterEvents[200][0x20];        // +0x385F8 (name ours) entereventgrp.txt (EnterEventDataLoad)
    BYTE Symbols[200][0x10];            // +0x39EF8 (name ours) symbolname-*.txt (LoadSymbolInfo, which
                                        //   stops at 200; GetSymbolTexture)
    BYTE _Unknown3AB78[0x418];          // +0x3AB78 262 dwords no loader or getter touches

    ENGINE_API INT GetCommandType(TCHAR* Command);   // commandname-*.dat: code of a chat command
    ENGINE_API FL2MagicSkillCommandData* GetMSCommandData(TCHAR* Command);
    ENGINE_API void Load();                          // reads the tables again (language change)
    ENGINE_API TCHAR* GetStaticObjectName(INT ID);   // staticobject-*.dat (doors...)
    ENGINE_API TCHAR* GetItemName(INT ID);           // itemname-*.dat, by item id
    ENGINE_API TCHAR* GetItemDescription(INT ID);
    ENGINE_API FL2MagicSkillData* GetMSData(INT SkillID, INT Level);   // NULL: no such skill
    ENGINE_API TCHAR* GetNpcName(INT ID);            // npcname-*.dat, by npc id
};
NW_ASSERT_OFFSET(FL2GameData, Actions, 0x4CD8);
NW_ASSERT_OFFSET(FL2GameData, SystemMsg, 0x1FDA8);
NW_ASSERT_OFFSET(FL2GameData, SysString, 0x2E810);
NW_ASSERT_OFFSET(FL2GameData, ClassInfo, 0x374B0);
NW_ASSERT_OFFSET(FL2GameData, QuestNames, 0xC528);
NW_ASSERT_OFFSET(FL2GameData, CastleNames, 0x37E28);
NW_ASSERT_OFFSET(FL2GameData, ItemTable, 0x27E4);
NW_ASSERT_OFFSET(FL2GameData, SkillTable, 0x4CB8);
NW_ASSERT_OFFSET(FL2GameData, ObsceneData, 0x4CC8);
NW_ASSERT_OFFSET(FL2GameData, WarningNotice, 0x37E1C);
NW_ASSERT_OFFSET(FL2GameData, BlowFish, 0x2C);
NW_ASSERT_OFFSET(FL2GameData, HairData, 0x1074);
NW_ASSERT_OFFSET(FL2GameData, NpcTable, 0x27E8);
NW_ASSERT_OFFSET(FL2GameData, CharData, 0x27EC);
NW_ASSERT_OFFSET(FL2GameData, NpcSkillAnimTable, 0x4CD4);
NW_ASSERT_OFFSET(FL2GameData, PetActions, 0xBA38);
NW_ASSERT_OFFSET(FL2GameData, Abnormals, 0x11348);
NW_ASSERT_OFFSET(FL2GameData, SkillSoundTable, 0x2E808);
NW_ASSERT_OFFSET(FL2GameData, ClassType, 0x37960);
NW_ASSERT_OFFSET(FL2GameData, NoticeText, 0x37E10);
NW_ASSERT_OFFSET(FL2GameData, EnterEvents, 0x385F8);
NW_ASSERT_OFFSET(FL2GameData, Symbols, 0x39EF8);
NW_ASSERT_SIZE(FL2GameData, 0x3AF90);

ENGINE_API extern FL2GameData GL2GameData;
ENGINE_API extern FMemCache GCache;           // Core's cache (UEngine::Flush empties it): UE2Core.h

/*-----------------------------------------------------------------------------
    L2ParamStack: the argument list of the requests to the server (16 bytes: buffer, size,
    read and write positions). PushBack takes any 32-bit value.
-----------------------------------------------------------------------------*/

class L2ParamStack
{
public:
    ENGINE_API L2ParamStack(INT Count);
    ENGINE_API ~L2ParamStack();
    ENGINE_API INT PushBack(void* Value);
    ENGINE_API void* Top();             // takes the next value, first pushed first
    ENGINE_API L2ParamStack& operator=(L2ParamStack& Other);

private:
    void** Buffer;
    INT Size;
    INT ReadPos;
    INT WritePos;
};
NW_ASSERT_SIZE(L2ParamStack, 0x10);

/*-----------------------------------------------------------------------------
    ItemInfo: L2's record of an item as the server sends it (Engine.dll "ItemInfo", 0x30
    bytes): the console's item slots get it by reference, three of them by value; NWindow's
    item requests fill one (NItemRequest, NCItemWnd.h). Names ours, after what the console
    makes of the fields (NItemInfo).
-----------------------------------------------------------------------------*/

struct ItemInfo
{
    SWORD Type;                // +0x00 NItemInfo::Param (0..3: the enchant shows)
    SWORD Unknown02;           // +0x02
    INT ObjectID;              // +0x04 the server's id of this one
    INT ItemID;                // +0x08 itemname and item table id
    INT Count;                 // +0x0C
    INT Unknown10;             // +0x10 (NItemInfo +0xC1C: the shops' and stores' lists)
    INT Unknown14;             // +0x14
    INT BodyPart;              // +0x18 slot mask
    SWORD Unknown1C;           // +0x1C (NItemInfo +0xC84)
    SWORD bEquipped;           // +0x1E (NItemInfo +0xC8C)
    INT Enchant;               // +0x20
    INT Unknown24;             // +0x24 (NItemInfo +0xC70)
    INT Unknown28;             // +0x28 (NItemInfo +0xC98: the private stores' price)
    INT Unknown2C;             // +0x2C (NItemInfo +0xC90: the warehouse's and the buy
                               //   stores' lists)
};
NW_ASSERT_OFFSET(ItemInfo, ItemID, 0x8);
NW_ASSERT_OFFSET(ItemInfo, bEquipped, 0x1E);
NW_ASSERT_SIZE(ItemInfo, 0x30);

// The new character NCPawnCreateWnd asks the server for (Network.dll "CharacterInfo": 0x58
// bytes, passed by value to UL2NetHandler::RequestCharacterCreate; field names ours).
// NCPawnSetupWnd::GetCreateInfo fills the name and the choices; the creation window adds the
// stats of the last class template the server sent and the hair and face.
struct CharacterInfo
{
    TCHAR Name[20];            // +0x00
    INT Race;                  // +0x28 0 human, 1 elf, 2 dark elf, 3 orc, 4 dwarf
    INT ClassID;               // +0x2C base class (NCPawnSetupWnd::GetClassID)
    INT Sex;                   // +0x30 0 male, 1 female
    INT Stats[6];              // +0x34 STR, DEX, CON, INT, WIT, MEN
    INT HairStyle;             // +0x4C
    INT HairColor;             // +0x50
    INT Face;                  // +0x54
};
NW_ASSERT_OFFSET(CharacterInfo, Race, 0x28);
NW_ASSERT_OFFSET(CharacterInfo, Stats, 0x34);
NW_ASSERT_OFFSET(CharacterInfo, HairStyle, 0x4C);
NW_ASSERT_SIZE(CharacterInfo, 0x58);

// L2's record of a clan (Engine.dll "PledgeInfo"): the console copies it into NPledgeInfo
// (NCPledgeWnd.h), whose names these follow; the network handler keeps one per clan
// (UL2NetHandler::GetPledgeInfo), with the crest textures. Only the fields NWindow reads.
struct PledgeInfo
{
    INT PledgeID;              // +0x00
    TCHAR Name[32];            // +0x04
    TCHAR LeaderName[32];      // +0x44
    INT Level;                 // +0x84
    UBOOL bHasCastle;          // +0x88
    UBOOL bHasHideout;         // +0x8C
    INT Unknown90;             // +0x90 (NPledgeInfo +0x94)
    INT Unknown94;             // +0x94 (NPledgeInfo +0x98)
    INT Status;                // +0x98 0..4, the status text's
    INT Unknown9C;             // +0x9C (NPledgeInfo +0xE8)
    INT CrestID;               // +0xA0 its texture is "crest.<server>_<id>" (the GM status
                               //   window finds it by that name)
    BYTE _UnknownA4[0xBC - 0xA4];
    TCHAR AllyName[(0x100 - 0xBC) / 2]; // +0xBC (size: up to the next known field)
    UTexture* CrestTex;        // +0x100 NULL = no crest; the character status window draws it
                               //   directly (the engine hands the network handler each crest
                               //   texture it imports)
    UTexture* AllyCrestTex;    // +0x104 (tested the same way before the alliance crest is drawn)
};
NW_ASSERT_OFFSET(PledgeInfo, LeaderName, 0x44);
NW_ASSERT_OFFSET(PledgeInfo, CrestID, 0xA0);
NW_ASSERT_OFFSET(PledgeInfo, AllyName, 0xBC);
NW_ASSERT_OFFSET(PledgeInfo, CrestTex, 0x100);

/*-----------------------------------------------------------------------------
    User: L2's record of a creature the client knows (Engine.dll "User", 0x208 bytes: the
    character selection keeps seven by value and copies them around). Only the fields NWindow
    reads are placed.
-----------------------------------------------------------------------------*/

struct User
{
    BYTE _Unknown0[0x8];
    INT Unknown8;                       // +0x08 non-zero: not a player (invitations refuse it)
    INT Unknown0C;                      // +0x0C with Unknown8: a pet or summon (target window)
    INT Unknown10;                      // +0x10 with Unknown8, without Unknown0C: the target
                                        //   window shows its HP (monsters)
    INT NpcID;                          // +0x14 npcname id (NConsoleWnd::IsSiegeNpc)
    INT ObjectID;                       // +0x18 (attack and target requests)
    BYTE _Unknown1C[0x5C - 0x1C];
    INT Race;                           // +0x5C (names ours) the pawn viewers set them with the
    INT Sex;                            // +0x60   class id
    INT ClassID;                        // +0x64 (NConsoleWnd::GetClassNameString)
    INT Level;                          // +0x68
    INT Exp;                            // +0x6C
    INT Str;                            // +0x70 the six stats (character status window)
    INT Dex;                            // +0x74
    INT Con;                            // +0x78
    INT Int;                            // +0x7C
    INT Wit;                            // +0x80
    INT Men;                            // +0x84
    INT MaxHP;                          // +0x88
    INT HP;                             // +0x8C
    INT MaxMP;                          // +0x90
    INT MP;                             // +0x94
    INT MaxLoad;                        // +0x98 weight limit
    BYTE _Unknown9C[0xA4 - 0x9C];
    INT Equipped[15];                   // +0xA4 (name ours) what it wears, by body part slot:
                                        //   object ids on the player's record (the inventory
                                        //   compares them), item ids on the viewers' records
    BYTE _UnknownE0[0xF0 - 0xE0];
    INT Karma;                          // +0xF0 (also reddens the name in the status window;
                                        //   shown beside "Karma" on the character selection)
    INT RunSpeed;                       // +0xF4 times the pawn's speed factor (+0x698)
    INT WalkSpeed;                      // +0xF8
    BYTE _UnknownFC[0x118 - 0xFC];
    FLOAT MoveSpeedRate;                // +0x118 (name ours) the GM status window multiplies
                                        //   RunSpeed by this one instead of the pawn's
    BYTE _Unknown11C[0x148 - 0x11C];
    APawn* Pawn;                        // +0x148
    INT DeleteDays;                     // +0x14C (name ours) days left until the character is
                                        //   deleted, 0 = not being deleted
    INT Load;                           // +0x150 weight carried
    INT SP;                             // +0x154 (also on the character selection, unlabeled)
    INT Accuracy;                       // +0x158 combat values (character status window)
    INT Critical;                       // +0x15C
    INT PAtk;                           // +0x160
    INT PAtkSpeed;                      // +0x164
    INT PDef;                           // +0x168
    INT Evasion;                        // +0x16C
    INT MAtk;                           // +0x170
    INT MDef;                           // +0x174
    INT CastSpeed;                      // +0x178
    INT HairStyle;                      // +0x17C (names ours) the creation's choices, applied
    INT HairColor;                      // +0x180   by SetPawnResource
    INT Face;                           // +0x184
    INT Unknown188;                     // +0x188 set: the party loot rule cannot be changed
                                        //   (game options)
    INT BuilderLevel;                   // +0x18C GM rights, 0 = none (// and /// commands)
    BYTE _Unknown190[0x1D0 - 0x190];
    INT PledgeID;                       // +0x1D0 (NConsoleWnd::GetPledgeInfo, crests)
    BYTE _Unknown1D4[0x1E0 - 0x1D4];
    DWORD StatusFlags;                  // +0x1E0 (name ours) bit 6: clan leader
    BYTE _Unknown1E4[0x1E8 - 0x1E4];
    INT Unknown1E8;                     // +0x1E8 UNetHandler::IsSurrender compares it with its
                                        //   clan's +0xA8 (the console's attack cursor)
    BYTE _Unknown1EC[0x1F0 - 0x1EC];
    INT bCanCrystallize;                // +0x1F0 (name ours) the inventory's crystallize icon
    BYTE _Unknown1F4[0x1F8 - 0x1F4];
    SWORD PrivateStoreType;             // +0x1F8 (name ours) 1 selling, 3 buying (the console's
                                        //   talk cursor); set: no chat balloon over it
    BYTE _Unknown1FA[0x1FC - 0x1FA];
    INT PKCount;                        // +0x1FC (names ours) shown as "PvP / PK"
    INT PvPCount;                       // +0x200
    BYTE _Unknown204[0x208 - 0x204];

    ENGINE_API User();
    ENGINE_API TCHAR* GetName();
    ENGINE_API TCHAR* GetNickName();    // the title (NULL or empty: none)
    ENGINE_API INT GetMeshType();
    ENGINE_API TCHAR* GetClassNamePointer();
    ENGINE_API void SetPawnResource();
    ENGINE_API INT GetItemClassID(INT Slot);        // the item worn in a body part slot
};
NW_ASSERT_OFFSET(User, NpcID, 0x14);
NW_ASSERT_OFFSET(User, ObjectID, 0x18);
NW_ASSERT_OFFSET(User, ClassID, 0x64);
NW_ASSERT_OFFSET(User, Level, 0x68);
NW_ASSERT_OFFSET(User, Str, 0x70);
NW_ASSERT_OFFSET(User, MaxHP, 0x88);
NW_ASSERT_OFFSET(User, MP, 0x94);
NW_ASSERT_OFFSET(User, MaxLoad, 0x98);
NW_ASSERT_OFFSET(User, Karma, 0xF0);
NW_ASSERT_OFFSET(User, Pawn, 0x148);
NW_ASSERT_OFFSET(User, Load, 0x150);
NW_ASSERT_OFFSET(User, CastSpeed, 0x178);
NW_ASSERT_OFFSET(User, BuilderLevel, 0x18C);
NW_ASSERT_OFFSET(User, PledgeID, 0x1D0);
NW_ASSERT_OFFSET(User, StatusFlags, 0x1E0);
NW_ASSERT_OFFSET(User, PvPCount, 0x200);
NW_ASSERT_OFFSET(User, DeleteDays, 0x14C);
NW_ASSERT_OFFSET(User, HairStyle, 0x17C);
NW_ASSERT_OFFSET(User, Face, 0x184);
NW_ASSERT_OFFSET(User, Race, 0x5C);
NW_ASSERT_OFFSET(User, Equipped, 0xA4);
NW_ASSERT_OFFSET(User, Unknown188, 0x188);
NW_ASSERT_OFFSET(User, bCanCrystallize, 0x1F0);
NW_ASSERT_OFFSET(User, PrivateStoreType, 0x1F8);
NW_ASSERT_OFFSET(User, Unknown1E8, 0x1E8);
NW_ASSERT_SIZE(User, 0x208);

// What the network handler keeps about the player's pet or summon (Network.dll "PetInfo":
// UNetworkHandler::GetPetInfo returns it by value, NConsoleWnd::GetPetInfo passes it on;
// 0xB4 bytes).
struct PetInfo
{
    INT Type;                           // +0x00 2: pet (NCPetStatusWnd), 1: summon
                                        //   (NCSummonedStatusWnd); each window hides otherwise
    INT ObjectID;                       // +0x04
    TCHAR Name[(0x54 - 0x8) / 2];       // +0x08 (size: up to the next known field)
    INT HP;                             // +0x54
    INT MaxHP;                          // +0x58
    INT MP;                             // +0x5C
    INT MaxMP;                          // +0x60
    INT SP;                             // +0x64
    INT Level;                          // +0x68
    INT Exp;                            // +0x6C
    INT LevelExp;                       // +0x70 exp at the start of its level
    INT NextExp;                        // +0x74 exp of the next level (0: no exp bar)
    INT Fed;                            // +0x78 pet's food (the green bar)
    INT MaxFed;                         // +0x7C
    INT Load;                           // +0x80 weight carried (pet window)
    INT MaxLoad;                        // +0x84
    INT PAtk;                           // +0x88 combat values (pet and summon info tabs)
    INT PDef;                           // +0x8C
    INT Accuracy;                       // +0x90
    INT Critical;                       // +0x94
    INT PAtkSpeed;                      // +0x98
    INT MAtk;                           // +0x9C
    INT MDef;                           // +0xA0
    INT Evasion;                        // +0xA4
    INT Speed;                          // +0xA8
    INT CastSpeed;                      // +0xAC
    INT UnknownB0;                      // +0xB0 (the status windows' constructors clear it)
};
NW_ASSERT_OFFSET(PetInfo, HP, 0x54);
NW_ASSERT_OFFSET(PetInfo, Level, 0x68);
NW_ASSERT_OFFSET(PetInfo, Fed, 0x78);
NW_ASSERT_OFFSET(PetInfo, PAtk, 0x88);
NW_ASSERT_OFFSET(PetInfo, CastSpeed, 0xAC);
NW_ASSERT_SIZE(PetInfo, 0xB4);

// An item lying on the ground as the network handler keeps it (UNetHandler::GetItem, by its
// object id). Only the fields NWindow reads.
struct Item
{
    INT ObjectID;                       // +0x00 the server's id (the pet picks it up by it)
    INT ItemID;                         // +0x04 itemname id
    UBOOL bStackable;                   // +0x08 (names ours) set: its name shows the count
    INT Count;                          // +0x0C
};
NW_ASSERT_OFFSET(Item, ItemID, 0x4);
NW_ASSERT_OFFSET(Item, Count, 0xC);

// A door or another object the server placed (UNetHandler::GetStaticObject, by the actor's
// creature id; its constructor and assignment are exported, 0x10 bytes)
struct FServerStaticObject
{
    INT Unknown0;                       // +0x00 -1 when made
    INT StaticObjectID;                 // +0x04 (names ours) staticobject-*.dat id
                                        //   (FL2GameData::GetStaticObjectName)
    INT Unknown8;                       // +0x08
    AActor* Actor;                      // +0x0C (UGameEngine::OnMyTargetSelected targets it)
};
NW_ASSERT_SIZE(FServerStaticObject, 0x10);

// L2's console base (Engine.dll): 247 virtuals after USubsystem's, 228 of them pure. NConsoleWnd
// implements those; their declarations are NConsoleWnd's (db/l2consolewnd_slots.csv, through
// tools/gen_engine_decls.py).
class UL2ConsoleWnd : public USubsystem
{
    ENGINE_API static UClass PrivateStaticClass;   // private, as DECLARE_CLASS has it

public:
    static UClass* StaticClass() { return &PrivateStaticClass; }   // NConsoleWnd's parent class

    INT State;                          // +0x2C L2ConsoleState (UL2ConsoleWnd::GetState)
    INT SavedState;                     // +0x30 (name ours) while State is L2CS_Loading, the state
                                        //   SetState set (it only writes this one then)
    BYTE _Unknown34[0x44 - 0x34];
    UEngine* Engine;                    // +0x44 (UL2ConsoleWnd::PlayIntroMusic: Engine->Audio)
    User DefaultCharacterInfo[18];      // +0x48 (names ours, after LoadDefaultCharacterInfo and
                                        //   InsertSelectCharacterInfo) the creation stage's
                                        //   characters, Pawn their actor
    BYTE _Unknown24D8[0x25F8 - 0x24D8];
    User SelectCharacterInfo[8];        // +0x25F8 the selection's (ResetSelectCharacterInfo
                                        //   empties the eight)
    BYTE _Unknown3638[0x36B8 - 0x3638];

    ENGINE_API UL2ConsoleWnd();
    ENGINE_API virtual ~UL2ConsoleWnd();
    ENGINE_API void SetState(L2ConsoleState NewState);

    ENGINE_API virtual void Tick(FLOAT DeltaTime);                                   //  24 +0x060
#include "gen/UL2ConsoleWnd.vt.inc"
};
NW_ASSERT_OFFSET(UL2ConsoleWnd, State, 0x2C);
NW_ASSERT_OFFSET(UL2ConsoleWnd, SavedState, 0x30);
NW_ASSERT_OFFSET(UL2ConsoleWnd, Engine, 0x44);
NW_ASSERT_OFFSET(UL2ConsoleWnd, DefaultCharacterInfo, 0x48);
NW_ASSERT_OFFSET(UL2ConsoleWnd, SelectCharacterInfo, 0x25F8);
NW_ASSERT_SIZE(UL2ConsoleWnd, 0x36B8);   // its copy constructor copies up to here

// A class template the server sends before the character creation (Engine.dll
// "RecommandedDataStr", 0x50 bytes, passed by value to FL2NetNotify::AddRecommandedData; field
// names ours, laid out as the server's packet sends them: race, class, then 70 / value / 10 for
// each stat). NWindow reads only the values.
struct RecommandedDataStr
{
    INT Race;                           // +0x00
    INT ClassID;                        // +0x04
    struct
    {
        INT Max;
        INT Value;
        INT Min;
    } Stats[6];                         // +0x08 STR, DEX, CON, INT, WIT, MEN
};
NW_ASSERT_SIZE(RecommandedDataStr, 0x50);

/*-----------------------------------------------------------------------------
    FParser (L2 addition): splits a line into tokens at the separator characters. Only the
    size matters here (0x418: flags, position, length, token length, string, the token and
    the separators, 257 characters each).
-----------------------------------------------------------------------------*/

class FParser
{
public:
    ENGINE_API FParser();
    ENGINE_API ~FParser();
    ENGINE_API TCHAR* GetNextToken();
    ENGINE_API void SetSperator(TCHAR* Separators);   // sic
    ENGINE_API TCHAR* SetString(TCHAR* String);       // the first token

private:
    ENGINE_API TCHAR* GetToken();
    ENGINE_API UBOOL IsSeperator(TCHAR C);

    BYTE Data[0x418];
};
NW_ASSERT_SIZE(FParser, 0x418);

/*=============================================================================
    NCPawnViewr_HairChangeWnd.cpp - the pawn viewer's hair page. Stock:
    0x100885d0 .. 0x1008876d, 0x1008a040 .. 0x1008bc0e (the Cast<AActor> template code at
    0x10088800 is inline here).
=============================================================================*/
#include "nwindow/NCPawnViewrWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCComboBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>

// NWM_COMMAND ids (names ours); both hair combos carry 0x104, which nothing handles
enum
{
    NID_HAIR_COMBO = 0x104,
    NID_HAIR_MESHA = 0x105,
    NID_HAIR_MESHB = 0x106,
    NID_HAIR_OK = 0x107,
    NID_HAIR_CLEAR = 0x108,
    NID_HAIR_SEARCH = 0x111,
};

// sysstring lines
enum { STR_OK = 140, STR_Cancel = 141, STR_Select = 183 };

// the mesh names' prefixes by the pawn's mesh type (APawn::MeshType); built on the stack from
// these literals by each stock user
static const TCHAR GHairMeshPrefixes[20][20] = {
    TEXT("MFighter"), TEXT("FFighter"), TEXT("MDarkElf"), TEXT("FDarkElf"), TEXT("MDwarf"),
    TEXT("FDwarf"),   TEXT("MElf"),     TEXT("FElf"),     TEXT("MMagic"),   TEXT("FMagic"),
    TEXT("MOrc"),     TEXT("FOrc"),     TEXT("MShaman"),  TEXT("FShaman")};

// the player's pawn: its hair meshes' prefix, and whether its hair can change
static FORCEINLINE const TCHAR* MeshPrefix(APawn* Pawn)
{
    return GHairMeshPrefixes[Pawn->MeshType];
}
static FORCEINLINE UBOOL HairLocked(APawn* Pawn) { return Pawn->Unknown678 & 4; }

// UObject::StaticFindObject, then StaticLoadObject when not loaded yet (inline each time)
static FORCEINLINE UObject* FindOrLoad(UClass* FindClass, UClass* LoadClass, const TCHAR* Name)
{
    UObject* Object = UObject::StaticFindObject(FindClass, ANY_PACKAGE, Name, 0);
    if (!Object)
        Object = UObject::StaticLoadObject(LoadClass, NULL, Name, NULL, 0, NULL);
    return Object;
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

// 0x1008a040
NCPawnViewr_HairChangeWnd::NCPawnViewr_HairChangeWnd()
{
    HairStyleCombo = NULL;
    MeshACombo = NULL;
    MeshBCombo = NULL;
    OKButton = NULL;
    ClearButton = NULL;
    memset(MeshATexture, 0, sizeof(MeshATexture));
    memset(MeshBTexture, 0, sizeof(MeshBTexture));
}
NW_HOOK_CTOR(0x1008a040, NCPawnViewr_HairChangeWnd)

// 0x1008a120
NW_HOOK_DTOR(0x1008a120, NCPawnViewr_HairChangeWnd)

// 0x100885d0 (vslot 24)
INT NCPawnViewr_HairChangeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_HAIR_MESHA)
            return OnMeshA((INT)lParam) != 0;
        if (wParam == NID_HAIR_MESHB)
            return OnMeshB((INT)lParam) != 0;
        if (wParam == NID_HAIR_OK)
            return OnOK() != 0;
        if (wParam == NID_HAIR_CLEAR)
            return OnClear() != 0;
        if (wParam == NID_HAIR_SEARCH)
            return OnSearch() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100885d0, NCPawnViewr_HairChangeWnd, WindowProc)

// 0x1008a170 (vslot 35) - OK / Cancel, style (0..4) and color (0..1), the two mesh combos and
// "Select" (the search)
INT NCPawnViewr_HairChangeWnd::OnCreate()
{
    TCHAR Text[64];
    NCWnd::OnCreate();
    OKButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                            TEXT("L2UI.Control.DefaultButton_click"), 0);
    OKButton->ID = NID_HAIR_OK;
    OKButton->SetWindowRect(45, 170, 35, 16);
    CreateChild(OKButton, NWS_VISIBLE);
    OKButton->SetTextID(STR_OK);
    ClearButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                               TEXT("L2UI.Control.DefaultButton_click"), 0);
    ClearButton->ID = NID_HAIR_CLEAR;
    ClearButton->SetWindowRect(95, 170, 35, 16);
    CreateChild(ClearButton, NWS_VISIBLE);
    ClearButton->SetTextID(STR_Cancel);

    HairStyleCombo = new NCComboBox;
    HairStyleCombo->ID = NID_HAIR_COMBO;
    HairStyleCombo->SetWindowRect(45, 30, 35, 16);
    CreateChild(HairStyleCombo, NWS_VISIBLE);
    HairColorCombo = new NCComboBox;
    HairColorCombo->ID = NID_HAIR_COMBO;
    HairColorCombo->SetWindowRect(140, 30, 35, 16);
    CreateChild(HairColorCombo, NWS_VISIBLE);
    for (INT i = 0; i < 5; i++)
    {
        appSprintf(Text, TEXT("%d"), i);
        HairStyleCombo->AddItem(Text);
    }
    for (INT i = 0; i < 2; i++)
    {
        appSprintf(Text, TEXT("%d"), i);
        HairColorCombo->AddItem(Text);
    }

    MeshACombo = new NCComboBox;
    MeshACombo->ID = NID_HAIR_MESHA;
    MeshACombo->SetWindowRect(45, 55, 155, 16);
    CreateChild(MeshACombo, NWS_VISIBLE);
    MeshBCombo = new NCComboBox;
    MeshBCombo->ID = NID_HAIR_MESHB;
    MeshBCombo->SetWindowRect(45, 105, 155, 16);
    CreateChild(MeshBCombo, NWS_VISIBLE);

    SearchButton = new NCButton(TEXT("L2UI.Control.DefaultButton"),
                                TEXT("L2UI.Control.DefaultButton_click"), 0);
    SearchButton->ID = NID_HAIR_SEARCH;
    SearchButton->SetWindowRect(180, 30, 35, 16);
    CreateChild(SearchButton, NWS_VISIBLE);
    SearchButton->SetTextID(STR_Select);
    return 1;
}
NW_HOOK_METHOD(0x1008a170, NCPawnViewr_HairChangeWnd, OnCreate)

// 0x1008bb20 (vslot 36) - the color combo and "Select" are left alone
INT NCPawnViewr_HairChangeWnd::OnDestroy()
{
    DestroyChild(this, HairStyleCombo);
    DestroyChild(this, MeshACombo);
    DestroyChild(this, MeshBCombo);
    DestroyChild(this, OKButton);
    DestroyChild(this, ClearButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x1008bb20, NCPawnViewr_HairChangeWnd, OnDestroy)

// 0x1008b7d0 (vslot 50) - the labels, the number of meshes found and the textures chosen,
// each centered in its box; the origin goes on before the clip
INT NCPawnViewr_HairChangeWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnViewr_HairChangeWnd::OnPaint);
    TCHAR Text[64];
    SIZE Size;
    Device->BeginPaint(ScreenX, ScreenY);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    NCGDevice::GetTextExtent(&Size, TEXT("HELM:"), 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 30, 0xFFFFFFFF, TEXT("HELM:"),
                     0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    NCGDevice::GetTextExtent(&Size, TEXT("COLOR:"), 0, 0, 0, 0, 0);
    Device->DrawText((55 - Size.cx) / 2 + 85, (16 - Size.cy) / 2 + 30, 0xFFFFFFFF,
                     TEXT("COLOR:"), 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("AH(%d):"), MeshACombo->Items.Num());
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 55, 0xFFFFFFFF, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    appSprintf(Text, TEXT("BH(%d):"), MeshBCombo->Items.Num());
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    Device->DrawText((40 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 105, 0xFFFFFFFF, Text, 0, 0, 0,
                     L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    if (appStrlen(MeshATexture) > 0)
    {
        NCGDevice::GetTextExtent(&Size, MeshATexture, 0, 0, 0, 0, 0);
        Device->DrawText((220 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 80, 0xFF808000,
                         MeshATexture, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    if (appStrlen(MeshBTexture) > 0)
    {
        NCGDevice::GetTextExtent(&Size, MeshBTexture, 0, 0, 0, 0, 0);
        Device->DrawText((220 - Size.cx) / 2 + 5, (16 - Size.cy) / 2 + 130, 0xFF808000,
                         MeshBTexture, 0, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    }
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1008b7d0, NCPawnViewr_HairChangeWnd, OnPaint)

// 0x1008a670 (vslot 62) - the two meshes chosen and their textures on the player's pawn
// (skeletal meshes loaded when not found). With no mesh line chosen the index is -1 and the
// number read before MeshANumbers is the hair style.
INT NCPawnViewr_HairChangeWnd::OnOK()
{
    TCHAR Name[256];
    UObject* Object = NPawnView::PlayerPawn();
    AActor* Pawn = (Object && Object->IsA(AActor::StaticClass())) ? (AActor*)Object : NULL;
    if (!Pawn)
        return 1;
    if (MeshANumbers[MeshACombo->Selected] < 0 || MeshBNumbers[MeshBCombo->Selected] < 0)
        return 1;
    appSprintf(Name, TEXT("LineageWarriors.%s"), MeshACombo->GetItemText(MeshACombo->Selected));
    Pawn->HairMeshes[1] =
        (UMesh*)FindOrLoad(UMesh::StaticClass(), USkeletalMesh::StaticClass(), Name);
    Pawn->HairMaterials[1] =
        (UMaterial*)FindOrLoad(UMaterial::StaticClass(), UMaterial::StaticClass(), MeshATexture);
    appSprintf(Name, TEXT("LineageWarriors.%s"), MeshBCombo->GetItemText(MeshBCombo->Selected));
    Pawn->HairMeshes[0] =
        (UMesh*)FindOrLoad(UMesh::StaticClass(), USkeletalMesh::StaticClass(), Name);
    Pawn->HairMaterials[0] =
        (UMaterial*)FindOrLoad(UMaterial::StaticClass(), UMaterial::StaticClass(), MeshBTexture);
    return 1;
}
NW_HOOK_METHOD(0x1008a670, NCPawnViewr_HairChangeWnd, OnOK)

// 0x100886d0 (vslot 63) - the search emptied
INT NCPawnViewr_HairChangeWnd::OnClear()
{
    MeshACombo->ClearItems();
    MeshBCombo->ClearItems();
    memset(MeshATexture, 0, sizeof(MeshATexture));
    memset(MeshBTexture, 0, sizeof(MeshBTexture));
    HairStyleCombo->SetSelected(-1);
    HairColorCombo->SetSelected(-1);
    return 1;
}
NW_HOOK_METHOD(0x100886d0, NCPawnViewr_HairChangeWnd, OnClear)

// 0x1008aed0 (vslot 64) - the "a" mesh's texture name, from the mesh number, color and style
INT NCPawnViewr_HairChangeWnd::OnMeshA(INT Index)
{
    if (HairLocked(NPawnView::PlayerPawn()))
        return 1;
    const TCHAR* Prefix = MeshPrefix(NPawnView::PlayerPawn());
    appSprintf(MeshATexture, TEXT("%s.%s_m00%d_t0%d_m0%d_ah"), Prefix, Prefix,
               MeshANumbers[Index], HairColor, HairStyle);
    return 1;
}
NW_HOOK_METHOD(0x1008aed0, NCPawnViewr_HairChangeWnd, OnMeshA)

// 0x1008b350 (vslot 65)
INT NCPawnViewr_HairChangeWnd::OnMeshB(INT Index)
{
    if (HairLocked(NPawnView::PlayerPawn()))
        return 1;
    const TCHAR* Prefix = MeshPrefix(NPawnView::PlayerPawn());
    appSprintf(MeshBTexture, TEXT("%s.%s_m00%d_t0%d_m0%d_bh"), Prefix, Prefix,
               MeshBNumbers[Index], HairColor, HairStyle);
    return 1;
}
NW_HOOK_METHOD(0x1008b350, NCPawnViewr_HairChangeWnd, OnMeshB)

// 0x1008a830 (vslot 66) - the "a" and "b" hair meshes 0..9 of the style chosen that exist in
// LineageWarriors (loaded when needed) into the mesh combos
INT NCPawnViewr_HairChangeWnd::OnSearch()
{
    TCHAR Path[100];
    TCHAR Name[100];
    if (HairLocked(NPawnView::PlayerPawn()))
        return 1;
    if (HairStyleCombo->Selected < 0 || HairColorCombo->Selected < 0)
        return 1;
    MeshACombo->ClearItems();
    MeshBCombo->ClearItems();
    memset(MeshATexture, 0, sizeof(MeshATexture));
    memset(MeshBTexture, 0, sizeof(MeshBTexture));
    HairStyle = HairStyleCombo->Selected;
    HairColor = HairColorCombo->Selected;
    INT* NextA = MeshANumbers;
    INT* NextB = MeshBNumbers;
    for (INT i = 0; i < 10; i++)
    {
        appSprintf(Name, TEXT("%s_m00%d_m0%d_ah"), MeshPrefix(NPawnView::PlayerPawn()), i,
                   HairStyle);
        appSprintf(Path, TEXT("LineageWarriors.%s"), Name);
        if (FindOrLoad(UMesh::StaticClass(), UMesh::StaticClass(), Path))
        {
            MeshACombo->AddItem(Name);
            *NextA++ = i;
        }
        appSprintf(Name, TEXT("%s_m00%d_m0%d_bh"), MeshPrefix(NPawnView::PlayerPawn()), i,
                   HairStyle);
        appSprintf(Path, TEXT("LineageWarriors.%s"), Name);
        if (FindOrLoad(UMesh::StaticClass(), UMesh::StaticClass(), Path))
        {
            MeshBCombo->AddItem(Name);
            *NextB++ = i;
        }
    }
    return 1;
}
NW_HOOK_METHOD(0x1008a830, NCPawnViewr_HairChangeWnd, OnSearch)

NW_CHECK_VTABLE(NCPawnViewr_HairChangeWnd, 0x100eb0e8, 67)

/*=============================================================================
    NCPawnCreateWnd.cpp - the character creation screen: NCPawnCreateWnd, its choices panel
    (NCPawnSetupWnd), the class description (NCPawnInfoWnd) and the selectors (NCSelectCtrl).
    Stock: 0x10083c00 .. 0x1008741d, the four classes interleaved; three bodies are shared
    with other classes (0x10004e90, 0x10077fd0, 0x10083930).
=============================================================================*/
#include "nwindow/NCPawnCreateWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NString.h"
#include "nwindow/NConsoleWnd.h"

#include <string.h>
#include <wchar.h>

// NWM_COMMAND ids (names ours)
enum
{
    NID_SELECT_PREV = 0xF0EF01,        // NCSelectCtrl's side buttons
    NID_SELECT_NEXT = 0xF0EF02,
    NID_SETUP_NAME = 0x1503,           // NCPawnSetupWnd's children
    NID_SETUP_RACE = 0xF0A3B2,
    NID_SETUP_CLASSTYPE = 0xF0A4B3,
    NID_SETUP_SEX = 0xF0A5B4,
    NID_SETUP_HAIRCOLOR = 0xF0A6B5,
    NID_SETUP_HAIRSTYLE = 0xF0A7B6,
    NID_SETUP_FACE = 0xF0A8B7,
    NID_SETUP_FRAME = (INT)0xAFF12011, // the same title bar id as the character selection's
    NID_CREATE_CREATE = 0x1009,        // NCPawnCreateWnd's buttons
    NID_CREATE_PREVIOUS = 0x100A,
    NID_CREATE_LEFT = 0x100D,
    NID_CREATE_RIGHT = 0x100E,
    NID_CREATE_ZOOM = 0x1010,          // zoom in and zoom out alike
};

// systemmsg lines (names ours)
enum
{
    MSG_BadNameLength = 80,            // "Please insert 1~16 English characters."
    MSG_SelectRace = 81,               // "Please select your race."
    MSG_SelectClassType = 82,          // "Please select your occupation."
    MSG_SelectSex = 83,                // "Please select your gender."
    MSG_SelectHairStyle = 91,          // "Please select your hairstyle."
    MSG_IncorrectName = 204,           // "Incorrect name. Please try again. "
    MSG_SelectHairColor = 366,         // "Select hair color."
};

// the race index (names ours)
enum { RACE_Orc = 3, RACE_Dwarf = 4 };

// the choices in panel order (names ours)
enum { CHOICE_Race, CHOICE_ClassType, CHOICE_Sex, CHOICE_HairStyle, CHOICE_HairColor, CHOICE_Face };

// the parts of the lobby map's event names (stock tables, names ours)
#if NW_HOOK_MODE
#define GRaceEventNames (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x101130a8)))      // "HUMAN" .. "DWARF"
#define GClassTypeEventNames (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x101130bc))) // "KNIGHT", "WIZARD"
#define GSexEventNames (reinterpret_cast<const TCHAR* const*>(::nw::Addr(0x101130c4)))       // "MAN", "WOMAN"
#else
extern const TCHAR* const GRaceEventNames[];
extern const TCHAR* const GClassTypeEventNames[];
extern const TCHAR* const GSexEventNames[];
#endif

// the console's creation window, and its 18 default characters of the creation stage (User
// records, Pawn the actor; not placed in NConsoleWnd's declaration yet, name ours)
static NCPawnCreateWnd* ConsolePawnCreateWnd() { return GConsole->PawnCreateWnd; }
static User* ConsoleDefaultCharacters() { return GConsole->DefaultCharacterInfo; }

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

// the text of a systemmsg line
static FORCEINLINE const TCHAR* SystemMessage(INT ID)
{
    return *GL2GameData.SystemMsg[ID].Message;
}

/*-----------------------------------------------------------------------------
    NCSelectCtrl.
-----------------------------------------------------------------------------*/

// 0x10086df0 - the buttons come in OnCreate
NCSelectCtrl::NCSelectCtrl()
{
    Selected = -1;
    Items.Empty();
}
NW_HOOK_CTOR(0x10086df0, NCSelectCtrl)

// 0x10086eb0
NW_HOOK_DTOR(0x10086eb0, NCSelectCtrl)

// 0x100837f0 (vslot 24) - the side buttons step through the items
INT NCSelectCtrl::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SELECT_PREV)
            return SelectPrev() != 0;
        if (wParam == NID_SELECT_NEXT)
            return SelectNext() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x100837f0, NCSelectCtrl, WindowProc)

// 0x10083ed0 (vslot 35) - a push button at each end
INT NCSelectCtrl::OnCreate()
{
    NControl::OnCreate();

    LeftButton = new NCPushButton(TEXT("L2UI.LobbyWnd.LeftBtn"), TEXT("L2UI.LobbyWnd.LeftBtn_click"));
    LeftButton->ID = NID_SELECT_PREV;
    LeftButton->SetWindowRect(0, 1, 27, 15);
    CreateChild(LeftButton, NWS_VISIBLE);

    RightButton = new NCPushButton(TEXT("L2UI.LobbyWnd.rightBtn"), TEXT("L2UI.LobbyWnd.rightBtn_click"));
    RightButton->ID = NID_SELECT_NEXT;
    RightButton->SetWindowRect(Width - 27, 1, 27, 15);
    CreateChild(RightButton, NWS_VISIBLE);
    return 1;
}
NW_HOOK_METHOD(0x10083ed0, NCSelectCtrl, OnCreate)

// 0x10086370 (vslot 36)
INT NCSelectCtrl::OnDestroy()
{
    DestroyChild(this, LeftButton);
    DestroyChild(this, RightButton);
    ClearItems();
    NControl::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10086370, NCSelectCtrl, OnDestroy)

// 0x10084030 (vslot 50) - the chosen item after the left button, nothing while none is. No
// guard; the clip goes before the origin.
INT NCSelectCtrl::OnPaint(NCGDevice* Device)
{
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (Selected >= 0 && Selected < Items.Num())
        Device->DrawText(32, 2, 0xFFFFFFFF, Items(Selected)->GetAt(0), 0, 0, 0, L2FONT_Normal, 0,
                         0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
}
NW_HOOK_METHOD(0x10084030, NCSelectCtrl, OnPaint)

// 0x10083880 (vslot 63) - the previous item, from the first to the last
INT NCSelectCtrl::SelectPrev()
{
    if (Items.Num() > 0 && IsEnabled())
    {
        if (Selected > 0)
            Selected--;
        else
            Selected = Items.Num() - 1;
        NotifyParent(Selected);
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10083880, NCSelectCtrl, SelectPrev)

// 0x100838d0 (vslot 64) - the next item, from the last to the first
INT NCSelectCtrl::SelectNext()
{
    if (Items.Num() > 0 && IsEnabled())
    {
        if (Selected < Items.Num() - 1)
            Selected++;
        else
            Selected = 0;
        NotifyParent(Selected);
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100838d0, NCSelectCtrl, SelectNext)

// 0x100862d0
void NCSelectCtrl::ClearItems()
{
    for (INT i = 0; i < Items.Num(); i++)
    {
        NString* Item = Items(i);
        if (Item)
            delete Item;
    }
    Items.Empty();
    Selected = -1;
}
NW_HOOK_METHOD(0x100862d0, NCSelectCtrl, ClearItems)

// 0x100863f0 - line 0 is never added. Not inlined: the stock callers call it.
__declspec(noinline) void NCSelectCtrl::AddItem(INT SysStringID)
{
    if (SysStringID > 0)
        Items.AddItem(new NString(*GL2GameData.SysString[SysStringID]));
}
NW_HOOK_METHOD(0x100863f0, NCSelectCtrl, AddItem)

NW_CHECK_VTABLE(NCSelectCtrl, 0x100ea578, 65)

/*-----------------------------------------------------------------------------
    NCPawnInfoWnd.
-----------------------------------------------------------------------------*/

// 0x100840d0
NCPawnInfoWnd::NCPawnInfoWnd()
{
    BackTex = NULL;
    Text[0] = 0;
}
NW_HOOK_CTOR(0x100840d0, NCPawnInfoWnd)

// 0x10084130
NW_HOOK_DTOR(0x10084130, NCPawnInfoWnd)

// stock body shared with NControl's (0x10004e90)
INT NCPawnInfoWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    return NCWnd::WindowProc(Msg, wParam, lParam) != 0;
}

// 0x10084180 (vslot 35) - starts with classinfo 0, "Select a race and occupation."
INT NCPawnInfoWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.LobbyWnd.MakingBack2"));
    SetText(*GL2GameData.ClassInfo[0]);
    return 1;
}
NW_HOOK_METHOD(0x10084180, NCPawnInfoWnd, OnCreate)

// stock body shared with NCCompositeBox's (0x10083930)
INT NCPawnInfoWnd::OnDestroy()
{
    NCWnd::OnDestroy();
    return 1;
}

// 0x100841c0 (vslot 50) - the text wrapped at 228 pixels; the clip is released before the
// origin
INT NCPawnInfoWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnInfoWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 0, Width, Height, 0, 0, Width, Height, BackTex);
    const TCHAR* Str = Text;
    if (Str && *Str)
        Device->DrawText(6, 10, 0xFFFFFFFF, Text, 228, 0, 0, L2FONT_Normal, 0, 0, 0, 0, 0, 0);
    Device->ReleaseClip();
    Device->EndPaint();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100841c0, NCPawnInfoWnd, OnPaint)

// 0x10083940 - an empty text leaves the old one. Not inlined: the stock callers call it.
__declspec(noinline) void NCPawnInfoWnd::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
}
NW_HOOK_METHOD(0x10083940, NCPawnInfoWnd, SetText)

// stock body shared with NCMessageWnd's (0x10077fd0, not decompiled yet); kept out of line,
// the stock callers call it
__declspec(noinline) void NCPawnInfoWnd::ClearText()
{
    Text[0] = 0;
}

NW_CHECK_VTABLE(NCPawnInfoWnd, 0x100ea000, 62)

/*-----------------------------------------------------------------------------
    NCPawnSetupWnd.
-----------------------------------------------------------------------------*/

// the creation window this panel belongs to (read without a check in the stock code)
inline NCPawnCreateWnd* NCPawnSetupWnd::CreateWnd()
{
    return static_cast<NCPawnCreateWnd*>(Entry->Parent->Wnd);
}

// the selectors from First on show nothing (inlined in each handler in the stock code)
static void ClearChoicesFrom(NCPawnSetupWnd* Wnd, INT First)
{
    switch (First)
    {
    case CHOICE_Race:
        Wnd->RaceSelect->SetSelected(-1);
    case CHOICE_ClassType:
        Wnd->ClassTypeSelect->SetSelected(-1);
    case CHOICE_Sex:
        Wnd->SexSelect->SetSelected(-1);
    case CHOICE_HairStyle:
        Wnd->HairStyleSelect->SetSelected(-1);
    case CHOICE_HairColor:
        Wnd->HairColorSelect->SetSelected(-1);
    case CHOICE_Face:
        Wnd->FaceSelect->SetSelected(-1);
    }
}

// the hair styles: types A..D, and E, F for women (inlined in the stock code)
static void FillHairStyles(NCSelectCtrl* Select, UBOOL bWoman)
{
    Select->ClearItems();
    Select->AddItem(179);              // "Type A" .. "Type D"
    Select->AddItem(180);
    Select->AddItem(181);
    Select->AddItem(182);
    if (bWoman)
    {
        Select->AddItem(186);          // "Type E", "Type F"
        Select->AddItem(187);
    }
}

// the default character of Type wears the given hair and face (inlined in the stock code). It
// works on a copy of the console's record: only the actor changes.
static FORCEINLINE void DressDefaultCharacter(INT Type, INT HairStyle, INT HairColor, INT Face)
{
    User Preview = ConsoleDefaultCharacters()[Type];
    if (Preview.Pawn)
    {
        Preview.HairStyle = HairStyle;
        Preview.HairColor = HairColor;
        Preview.Face = Face;
        Preview.SetPawnResource();
    }
}

// 0x100842c0 - the hair and face selectors are left as allocated
NCPawnSetupWnd::NCPawnSetupWnd()
{
    RaceSelect = NULL;
    ClassTypeSelect = NULL;
    SexSelect = NULL;
    UnknownAC = 0;
    BackTex = NULL;
    NameEdit = NULL;
    CameraStep = 0;
    Race = -1;
    ClassType = -1;
    Sex = -1;
    HairStyle = -1;
    HairColor = -1;
    Face = -1;
}
NW_HOOK_CTOR(0x100842c0, NCPawnSetupWnd)

// 0x10084360
NW_HOOK_DTOR(0x10084360, NCPawnSetupWnd)

// 0x10083970 (vslot 24) - each selector reports its new index
INT NCPawnSetupWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_SETUP_RACE)
            return OnRace((INT)lParam) != 0;
        if (wParam == NID_SETUP_CLASSTYPE)
            return OnClassType((INT)lParam) != 0;
        if (wParam == NID_SETUP_SEX)
            return OnSex((INT)lParam) != 0;
        if (wParam == NID_SETUP_HAIRSTYLE)
            return OnHairStyle((INT)lParam) != 0;
        if (wParam == NID_SETUP_HAIRCOLOR)
            return OnHairColor((INT)lParam) != 0;
        if (wParam == NID_SETUP_FACE)
            return OnFace((INT)lParam) != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10083970, NCPawnSetupWnd, WindowProc)

// 0x10086f10 (vslot 35) - a title bar, the name box (up to 128 px past its width) and the six
// selectors, 22 px apart
INT NCPawnSetupWnd::OnCreate()
{
    NCWnd::OnCreate();
    BackTex = FindTexture(TEXT("L2UI.LobbyWnd.MakingBack1"));

    FrameCtrl = new NCFrameCtrl(1);
    FrameID = NID_SETUP_FRAME;
    FrameCtrl->ID = NID_SETUP_FRAME;
    FrameCtrl->SetWindowRect(0, 0, 240, 21);
    CreateChild(FrameCtrl, NWS_VISIBLE);

    NameEdit = new NCEditBox(0, NULL, 0, 128);
    NameEdit->ID = NID_SETUP_NAME;
    NameEdit->SetWindowRect(98, 28, 100, 16);
    CreateChild(NameEdit, NWS_VISIBLE);

    RaceSelect = new NCSelectCtrl;
    RaceSelect->ID = NID_SETUP_RACE;
    RaceSelect->SetWindowRect(86, 64, 135, 16);
    CreateChild(RaceSelect, NWS_VISIBLE);

    ClassTypeSelect = new NCSelectCtrl;
    ClassTypeSelect->ID = NID_SETUP_CLASSTYPE;
    ClassTypeSelect->SetWindowRect(86, 86, 135, 16);
    CreateChild(ClassTypeSelect, NWS_VISIBLE);

    SexSelect = new NCSelectCtrl;
    SexSelect->ID = NID_SETUP_SEX;
    SexSelect->SetWindowRect(86, 108, 135, 16);
    CreateChild(SexSelect, NWS_VISIBLE);

    HairStyleSelect = new NCSelectCtrl;
    HairStyleSelect->ID = NID_SETUP_HAIRSTYLE;
    HairStyleSelect->SetWindowRect(86, 130, 135, 16);
    CreateChild(HairStyleSelect, NWS_VISIBLE);

    HairColorSelect = new NCSelectCtrl;
    HairColorSelect->ID = NID_SETUP_HAIRCOLOR;
    HairColorSelect->SetWindowRect(86, 152, 135, 16);
    CreateChild(HairColorSelect, NWS_VISIBLE);

    FaceSelect = new NCSelectCtrl;
    FaceSelect->ID = NID_SETUP_FACE;
    FaceSelect->SetWindowRect(86, 174, 135, 16);
    CreateChild(FaceSelect, NWS_VISIBLE);

    InitSelectors();
    return 1;
}
NW_HOOK_METHOD(0x10086f10, NCPawnSetupWnd, OnCreate)

// 0x100845b0 (vslot 36) - the selectors are enabled again first (no NULL check)
INT NCPawnSetupWnd::OnDestroy()
{
    RaceSelect->EnableWindow(1);
    ClassTypeSelect->EnableWindow(1);
    SexSelect->EnableWindow(1);
    HairStyleSelect->EnableWindow(1);
    HairColorSelect->EnableWindow(1);
    FaceSelect->EnableWindow(1);
    DestroyChild(this, NameEdit);
    DestroyChild(this, RaceSelect);
    DestroyChild(this, ClassTypeSelect);
    DestroyChild(this, SexSelect);
    DestroyChild(this, HairStyleSelect);
    DestroyChild(this, HairColorSelect);
    DestroyChild(this, FaceSelect);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x100845b0, NCPawnSetupWnd, OnDestroy)

// 0x10083a90 (vslot 42) - the name box takes the keyboard
void NCPawnSetupWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    NameEdit->SetFocus();
}
NW_HOOK_METHOD(0x10083a90, NCPawnSetupWnd, ShowWindow)

// 0x100843b0 (vslot 50) - the background under the title bar and the labels "Name", "Race",
// "Class", "Gender", "Hairstyle", "Hair Color", "Face" (sysstring 163..169)
INT NCPawnSetupWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnSetupWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    if (BackTex)
        Device->DrawTexture(0, 21, Width, Height, 0, 0, Width, Height, BackTex);
    Device->DrawText(24, 30, 0xFFFFFFFF, *GL2GameData.SysString[163], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 67, 0xFFFFFFFF, *GL2GameData.SysString[164], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 89, 0xFFFFFFFF, *GL2GameData.SysString[165], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 111, 0xFFFFFFFF, *GL2GameData.SysString[166], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 133, 0xFFFFFFFF, *GL2GameData.SysString[167], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 155, 0xFFFFFFFF, *GL2GameData.SysString[168], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->DrawText(24, 177, 0xFFFFFFFF, *GL2GameData.SysString[169], 0, 0, 0, L2FONT_Normal, 0,
                     0, 0, 0, 0, 0);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100843b0, NCPawnSetupWnd, OnPaint)

// 0x10086630 (vslot 62) - a race: the later choices are cleared, dwarves get the warrior type
// only, the class type is enabled and every default character comes back on the stage
INT NCPawnSetupWnd::OnRace(INT Index)
{
    ResetPreviews();
    Race = Index;
    ClassType = -1;
    Sex = -1;
    HairStyle = -1;
    HairColor = -1;
    Face = -1;
    if (Index == RACE_Orc)
    {
        // the same list as the other races; the stock code tests orcs on their own
        ClassTypeSelect->ClearItems();
        ClassTypeSelect->AddItem(175);           // "Warrior Type"
        ClassTypeSelect->AddItem(176);           // "Spellcaster Type"
    }
    else if (Index == RACE_Dwarf)
    {
        ClassTypeSelect->ClearItems();
        ClassTypeSelect->AddItem(175);
    }
    else
    {
        ClassTypeSelect->ClearItems();
        ClassTypeSelect->AddItem(175);
        ClassTypeSelect->AddItem(176);
    }
    ClassTypeSelect->SetSelected(ClassType);
    SexSelect->SetSelected(Sex);
    HairStyleSelect->SetSelected(HairStyle);
    HairColorSelect->SetSelected(HairColor);
    FaceSelect->SetSelected(Face);
    RaceSelect->EnableWindow(1);
    ClassTypeSelect->EnableWindow(1);
    SexSelect->EnableWindow(0);
    HairStyleSelect->EnableWindow(0);
    HairColorSelect->EnableWindow(0);
    FaceSelect->EnableWindow(0);
    CreateWnd()->ShowViewButtons(0);
    ConsolePawnCreateWnd()->InfoWnd->ClearText();
    ExecCameraEvent(1);
    GConsole->ShowAllDefaultCharacter();
    return 1;
}
NW_HOOK_METHOD(0x10086630, NCPawnSetupWnd, OnRace)

// 0x10086810 (vslot 63) - a class type: the gender is enabled and the description shows the
// base class (classinfo 1..9). A class id outside the nine keeps Index as the classinfo line.
INT NCPawnSetupWnd::OnClassType(INT Index)
{
    ResetPreviews();
    ExecCameraEvent(2);
    ClassType = Index;
    Sex = -1;
    HairStyle = -1;
    HairColor = -1;
    Face = -1;
    SexSelect->SetSelected(Sex);
    HairStyleSelect->SetSelected(HairStyle);
    HairColorSelect->SetSelected(HairColor);
    FaceSelect->SetSelected(Face);
    if (Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        ClassTypeSelect->SetSelected(-1);
        return 1;
    }
    switch (GetClassID())
    {
    case 0:  Index = 1; break;         // human fighter
    case 10: Index = 2; break;         // human mystic
    case 18: Index = 3; break;         // elven fighter
    case 25: Index = 4; break;         // elven mystic
    case 31: Index = 5; break;         // dark fighter
    case 38: Index = 6; break;         // dark mystic
    case 44: Index = 7; break;         // orc fighter
    case 49: Index = 8; break;         // orc mystic
    case 53: Index = 9; break;         // dwarven fighter
    }
    RaceSelect->EnableWindow(1);
    ClassTypeSelect->EnableWindow(1);
    SexSelect->EnableWindow(1);
    HairStyleSelect->EnableWindow(0);
    HairColorSelect->EnableWindow(0);
    FaceSelect->EnableWindow(0);
    CreateWnd()->ShowViewButtons(0);
    ConsolePawnCreateWnd()->InfoWnd->ClearText();
    ConsolePawnCreateWnd()->InfoWnd->SetText(*GL2GameData.ClassInfo[Index]);
    GConsole->ShowAllDefaultCharacter();
    return 1;
}
NW_HOOK_METHOD(0x10086810, NCPawnSetupWnd, OnClassType)

// 0x10086a60 (vslot 64) - a gender: the hair style list for it (orc women get theirs only
// with a class type), hair and face start at type A, everything is enabled, the view buttons
// appear and the stage keeps only the chosen default character
INT NCPawnSetupWnd::OnSex(INT Index)
{
    ResetPreviews();
    CreateWnd()->ShowViewButtons(0);
    if (Race == RACE_Orc)
    {
        if (Index == 0)
            FillHairStyles(HairStyleSelect, 0);
        else if (Index == 1 && (ClassType == 0 || ClassType == 1))
            FillHairStyles(HairStyleSelect, 1);
    }
    else
    {
        if (Index == 0)
            FillHairStyles(HairStyleSelect, 0);
        else if (Index == 1)
            FillHairStyles(HairStyleSelect, 1);
    }
    Sex = Index;
    HairStyle = 0;
    HairColor = 0;
    Face = 0;
    HairStyleSelect->SetSelected(HairStyle);
    HairColorSelect->SetSelected(HairColor);
    FaceSelect->SetSelected(Face);
    if (Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        ClearChoicesFrom(this, CHOICE_ClassType);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (ClassType < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectClassType));
        ClearChoicesFrom(this, CHOICE_Sex);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    RaceSelect->EnableWindow(1);
    ClassTypeSelect->EnableWindow(1);
    SexSelect->EnableWindow(1);
    HairStyleSelect->EnableWindow(1);
    HairColorSelect->EnableWindow(1);
    FaceSelect->EnableWindow(1);
    CreateWnd()->ShowViewButtons(1);
    ExecCameraEvent(3);
    GConsole->ShowAllDefaultCharacter();
    GConsole->ShowOnlyOneDefaultCharacter(GetCharacterType());
    return 1;
}
NW_HOOK_METHOD(0x10086a60, NCPawnSetupWnd, OnSex)

// 0x10084850 (vslot 65) - a hair style, worn at once by the chosen default character
INT NCPawnSetupWnd::OnHairStyle(INT Index)
{
    ResetPreviews();
    HairStyle = Index;
    HairColorSelect->SetSelected(HairColor);
    FaceSelect->SetSelected(Face);
    if (Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        ClearChoicesFrom(this, CHOICE_ClassType);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (ClassType < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectClassType));
        ClearChoicesFrom(this, CHOICE_Sex);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (Sex < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectSex));
        ClearChoicesFrom(this, CHOICE_HairStyle);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    DressDefaultCharacter(GetCharacterType(), HairStyle, HairColor, Face);
    return 1;
}
NW_HOOK_METHOD(0x10084850, NCPawnSetupWnd, OnHairStyle)

// 0x10084b40 (vslot 66) - a hair color
INT NCPawnSetupWnd::OnHairColor(INT Index)
{
    ResetPreviews();
    HairColor = Index;
    FaceSelect->SetSelected(Face);
    if (Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        ClearChoicesFrom(this, CHOICE_ClassType);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (ClassType < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectClassType));
        ClearChoicesFrom(this, CHOICE_Sex);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (Sex < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectSex));
        ClearChoicesFrom(this, CHOICE_HairStyle);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (HairStyle < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectHairStyle));
        return 1;
    }
    DressDefaultCharacter(GetCharacterType(), HairStyle, HairColor, Face);
    return 1;
}
NW_HOOK_METHOD(0x10084b40, NCPawnSetupWnd, OnHairColor)

// 0x10084e40 (vslot 67) - a face
INT NCPawnSetupWnd::OnFace(INT Index)
{
    ResetPreviews();
    Face = Index;
    if (Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        ClearChoicesFrom(this, CHOICE_ClassType);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (ClassType < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectClassType));
        ClearChoicesFrom(this, CHOICE_Sex);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (Sex < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectSex));
        ClearChoicesFrom(this, CHOICE_HairStyle);
        CreateWnd()->ShowViewButtons(0);
        return 1;
    }
    if (HairStyle < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectHairStyle));
        ClearChoicesFrom(this, CHOICE_HairColor);
        return 1;
    }
    if (HairColor < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectHairColor));
        return 1;
    }
    DressDefaultCharacter(GetCharacterType(), HairStyle, HairColor, Index);
    return 1;
}
NW_HOOK_METHOD(0x10084e40, NCPawnSetupWnd, OnFace)

// 0x10083ab0 - index of the default character of the race / class type / gender chosen
INT NCPawnSetupWnd::GetCharacterType()
{
    return (Race * 2 + ClassType) * 2 + Sex;
}
NW_HOOK_METHOD(0x10083ab0, NCPawnSetupWnd, GetCharacterType)

// 0x10083ad0 - every default character back to hair style A and facing the camera; works on
// copies of the console's records, so only the actors change (this is not used)
void NCPawnSetupWnd::ResetPreviews()
{
    for (INT i = 0; i < 18; i++)
    {
        User Preview = ConsoleDefaultCharacters()[i];
        if (Preview.Pawn)
        {
            Preview.HairStyle = 0;
            Preview.SetPawnResource();
            GConsole->ResetDefaultCharacterYaw(i);
        }
    }
}
NW_HOOK_METHOD(0x10083ad0, NCPawnSetupWnd, ResetPreviews)

// 0x10083b50 - the base class of the race and class type, -1 while they are incomplete. The
// stock switch lacks two breaks: dark elves and orcs without a class type get 53 (dwarven
// fighter), so creating one then complains about the gender instead of the class type.
INT NCPawnSetupWnd::GetClassID()
{
    switch (Race)
    {
    case 0:
        if (ClassType == 0)
            return 0;                  // human fighter
        if (ClassType == 1)
            return 10;                 // human mystic
        break;
    case 1:
        if (ClassType == 0)
            return 18;                 // elven fighter
        if (ClassType == 1)
            return 25;                 // elven mystic
        break;
    case 2:
        if (ClassType == 0)
            return 31;                 // dark fighter
        if (ClassType == 1)
            return 38;                 // dark mystic
        // falls through (sic)
    case 3:
        if (ClassType == 0)
            return 44;                 // orc fighter
        if (ClassType == 1)
            return 49;                 // orc mystic
        // falls through (sic)
    case 4:
        return 53;                     // dwarven fighter
    }
    return -1;
}
NW_HOOK_METHOD(0x10083b50, NCPawnSetupWnd, GetClassID)

// 0x10083bf0 - the engine learns the race being made (this is not used). Not inlined: the
// stock caller calls it.
__declspec(noinline) void NCPawnSetupWnd::SetMakingRace(INT InRace)
{
    GConsole->SetCurrentMakingRace(InRace);
}
NW_HOOK_METHOD(0x10083bf0, NCPawnSetupWnd, SetMakingRace)

// 0x10084760 - an empty panel (the console calls it through NCPawnCreateWnd::Reset). Not
// inlined: the stock caller calls it.
__declspec(noinline) void NCPawnSetupWnd::Reset()
{
    CameraStep = 0;
    NameEdit->ClearText();
    Race = -1;
    ClassType = -1;
    Sex = -1;
    HairStyle = -1;
    HairColor = -1;
    Face = -1;
    ClearChoicesFrom(this, CHOICE_Race);
    CreateWnd()->ShowViewButtons(0);
}
NW_HOOK_METHOD(0x10084760, NCPawnSetupWnd, Reset)

// the name typed as the stock code measures it: bytes in the ANSI code page
static FORCEINLINE INT AnsiLength(const TCHAR* Text)
{
    ANSICHAR Ansi[512];
    WideCharToMultiByte(CP_ACP, 0, Text, -1, Ansi, 512, NULL, NULL);
    return (INT)strlen(Ansi);
}

// 0x10085190 - the name and the choices, or a system message and 0. Race, gender and hair
// style are read from the selectors, the class from GetClassID; hair color and face are left
// to the caller.
UBOOL NCPawnSetupWnd::GetCreateInfo(CharacterInfo* Info)
{
    if (!NameEdit->CommitText())
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_BadNameLength));
        return 0;
    }
    INT Length = AnsiLength(NameEdit->CommitText());
    if (Length == 0 || Length > 16)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_BadNameLength));
        NameEdit->SetFocus();
        return 0;
    }
    wcscpy(Info->Name, NameEdit->CommitText());
    Info->Race = RaceSelect->Selected;
    if (Info->Race < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectRace));
        return 0;
    }
    Info->ClassID = GetClassID();
    if (Info->ClassID < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectClassType));
        return 0;
    }
    Info->Sex = SexSelect->Selected;
    if (Info->Sex < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectSex));
        return 0;
    }
    Info->HairStyle = HairStyleSelect->Selected;
    if (Info->HairStyle < 0)
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_SelectHairStyle));
        return 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10085190, NCPawnSetupWnd, GetCreateInfo)

// 0x10085310 - the lobby event that moves the camera after a choice (Step 1 race, 2 class
// type, 3 gender), named from the selectors (already on the new item) and the map's name parts:
//   forward:   every choice so far, "ELF", "ELF_WIZARD", "ELF_WIZARD_WWOMAN";
//   same step: the change, "ELF_KNIGHT_WIZARD" (class type), "ELF_WMAN_WWOMAN" (gender); the
//              old item is the other of the two, taken from the new one;
//   back to 2: "ELF_WIZARD_KWOMAN" played in reverse: new class type, then the old gender
//              view, whose _K / _W again comes from the new class type.
// Dwarves (one class type) skip the class type change and the step back to it.
void NCPawnSetupWnd::ExecCameraEvent(INT Step)
{
    if (Step > CameraStep)
    {
        INT SelClassType = ClassTypeSelect->Selected;
        INT SelRace = RaceSelect->Selected;
        INT SelSex = SexSelect->Selected;
        NString Event;
        if (SelRace >= 0)
        {
            Event.Append(GRaceEventNames[SelRace]);
            CameraStep = 1;
        }
        if (SelClassType >= 0)
        {
            Event.Append(TEXT("_"));
            Event.Append(GClassTypeEventNames[SelClassType]);
            CameraStep = 2;
        }
        if (SelSex >= 0)
        {
            if (SelClassType == 0)
                Event.Append(TEXT("_K"));
            else if (SelClassType == 1)
                Event.Append(TEXT("_W"));
            Event.Append(GSexEventNames[SelSex]);
            CameraStep = 3;
        }
        SetMakingRace(SelRace);
        GConsole->ExecLobbyEvent(Event.GetAt(0), 0);
    }
    else if (Step == CameraStep)
    {
        INT SelRace = RaceSelect->Selected;
        INT SelSex = SexSelect->Selected;
        INT SelClassType = ClassTypeSelect->Selected;
        NString Event;
        if (Step == 1)
        {
            Event.Append(GRaceEventNames[SelRace]);
            SetMakingRace(SelRace);
            GConsole->ExecLobbyEvent(Event.GetAt(0), 0);
        }
        else if (Step == 2)
        {
            if (SelRace != RACE_Dwarf)
            {
                Event.Append(GRaceEventNames[SelRace]);
                Event.Append(TEXT("_"));
                if (SelClassType == 0)
                    Event.Append(GClassTypeEventNames[1]);
                else if (SelClassType == 1)
                    Event.Append(GClassTypeEventNames[0]);
                Event.Append(TEXT("_"));
                Event.Append(GClassTypeEventNames[SelClassType]);
                SetMakingRace(SelRace);
                GConsole->ExecLobbyEvent(Event.GetAt(0), 0);
            }
        }
        else if (Step == 3)
        {
            Event.Append(GRaceEventNames[SelRace]);
            if (SelClassType == 0)
                Event.Append(TEXT("_K"));
            else if (SelClassType == 1)
                Event.Append(TEXT("_W"));
            if (SelSex == 0)
                Event.Append(GSexEventNames[1]);
            else if (SelSex == 1)
                Event.Append(GSexEventNames[0]);
            if (SelClassType == 0)
                Event.Append(TEXT("_K"));
            else if (SelClassType == 1)
                Event.Append(TEXT("_W"));
            Event.Append(GSexEventNames[SelSex]);
            SetMakingRace(SelRace);
            GConsole->ExecLobbyEvent(Event.GetAt(0), 0);
        }
    }
    else if (Step < CameraStep)
    {
        INT SelRace = RaceSelect->Selected;
        INT SelSex = SexSelect->Selected;
        INT SelClassType = ClassTypeSelect->Selected;
        NString Event;
        if (Step == 1)
        {
            Event.Append(GRaceEventNames[SelRace]);
            SetMakingRace(SelRace);
            GConsole->ExecLobbyEvent(Event.GetAt(0), 0);
            CameraStep = Step;
        }
        else if (Step == 2 && SelRace != RACE_Dwarf)
        {
            Event.Append(GRaceEventNames[SelRace]);
            Event.Append(TEXT("_"));
            Event.Append(GClassTypeEventNames[SelClassType]);
            if (SelClassType == 1)
                Event.Append(TEXT("_K"));
            else if (SelClassType == 0)
                Event.Append(TEXT("_W"));
            Event.Append(GSexEventNames[SelSex]);
            SetMakingRace(SelRace);
            GConsole->ExecLobbyEvent(Event.GetAt(0), 1);
            CameraStep = 2;
        }
    }
}
NW_HOOK_METHOD(0x10085310, NCPawnSetupWnd, ExecCameraEvent)

// 0x10086490 - the items of the six selectors (OnRace and OnSex redo the class type and hair
// style lists); only the race is enabled
void NCPawnSetupWnd::InitSelectors()
{
    RaceSelect->AddItem(170);          // "Human", "Elf", "Dark elf", "Orc", "Dwarf"
    RaceSelect->AddItem(171);
    RaceSelect->AddItem(172);
    RaceSelect->AddItem(173);
    RaceSelect->AddItem(174);
    ClassTypeSelect->AddItem(175);     // "Warrior Type", "Spellcaster Type"
    ClassTypeSelect->AddItem(176);
    SexSelect->AddItem(177);           // "Male", "Female"
    SexSelect->AddItem(178);
    HairStyleSelect->AddItem(179);     // "Type A" .. "Type D", "Type E", "Type F"
    HairStyleSelect->AddItem(180);
    HairStyleSelect->AddItem(181);
    HairStyleSelect->AddItem(182);
    HairStyleSelect->AddItem(186);
    HairStyleSelect->AddItem(187);
    HairColorSelect->AddItem(179);     // "Type A", "Type B"
    HairColorSelect->AddItem(180);
    FaceSelect->AddItem(179);
    FaceSelect->AddItem(180);
    RaceSelect->EnableWindow(1);
    ClassTypeSelect->EnableWindow(0);
    SexSelect->EnableWindow(0);
    HairStyleSelect->EnableWindow(0);
    HairColorSelect->EnableWindow(0);
    FaceSelect->EnableWindow(0);
    CameraStep = 0;
}
NW_HOOK_METHOD(0x10086490, NCPawnSetupWnd, InitSelectors)

NW_CHECK_VTABLE(NCPawnSetupWnd, 0x100ea160, 68)

/*-----------------------------------------------------------------------------
    NCPawnCreateWnd.
-----------------------------------------------------------------------------*/

// 0x10085740 - the panels and the view buttons are left as allocated
NCPawnCreateWnd::NCPawnCreateWnd()
{
    CreateButton = NULL;
    PreviousButton = NULL;
    bZoomed = 0;
}
NW_HOOK_CTOR(0x10085740, NCPawnCreateWnd)

// 0x100857a0
NW_HOOK_DTOR(0x100857a0, NCPawnCreateWnd)

// 0x10083c00 (vslot 24) - the turn buttons report 1 when pressed and 0 when released
INT NCPawnCreateWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NWM_COMMAND)
    {
        if (wParam == NID_CREATE_PREVIOUS)
            return OnPrevious() != 0;
        if (wParam == NID_CREATE_CREATE)
            return OnCreateCharacter() != 0;
        if (wParam == NID_CREATE_LEFT)
            return OnTurnLeft((INT)lParam) != 0;
        if (wParam == NID_CREATE_RIGHT)
            return OnTurnRight((INT)lParam) != 0;
        if (wParam == NID_CREATE_ZOOM)
            return OnZoom() != 0;
    }
    return 0;
}
NW_HOOK_METHOD(0x10083c00, NCPawnCreateWnd, WindowProc)

// 0x100857f0 (vslot 35) - the panels on the left, the two buttons in the lower right corner
// like the character selection's, the view buttons under the stage, hidden (Width halved
// unsigned, as in the stock code)
INT NCPawnCreateWnd::OnCreate()
{
    NCWnd::OnCreate();
    bZoomed = 0;

    SetupWnd = new NCPawnSetupWnd;
    SetupWnd->SetWindowRect(34, 34, 240, 203);
    CreateChild(SetupWnd, NWS_NOCLOSE | NWS_VISIBLE);
    SetupWnd->SetTitle(*GL2GameData.SysString[158]);            // "Create"

    InfoWnd = new NCPawnInfoWnd;
    InfoWnd->SetWindowRect(34, 288, 240, 158);
    CreateChild(InfoWnd, NWS_VISIBLE);

    CreateButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    CreateButton->ID = NID_CREATE_CREATE;
    CreateButton->SetWindowRect(Width - 131, Height - 124, 99, 19);
    CreateChild(CreateButton, NWS_VISIBLE);
    CreateButton->SetTextID(162);                               // "Create Character"

    PreviousButton = new NCButton(TEXT("L2UI.ActionWnd.PledgeButten1"),
                                  TEXT("L2UI.ActionWnd.PledgeButten2"), 0);
    PreviousButton->ID = NID_CREATE_PREVIOUS;
    PreviousButton->SetWindowRect(Width - 131, Height - 100, 99, 19);
    CreateChild(PreviousButton, NWS_VISIBLE);
    PreviousButton->SetTextID(161);                             // "Previous"

    LeftButton = new NCExButton(TEXT("l2ui.nwindow.left"), TEXT("l2ui.nwindow.left_click"), 0);
    LeftButton->ID = NID_CREATE_LEFT;
    LeftButton->SetWindowRect(((DWORD)Width >> 1) - 88, Height - 100, 44, 44);
    CreateChild(LeftButton, NWS_VISIBLE);
    LeftButton->HideWindow();

    RightButton = new NCExButton(TEXT("l2ui.nwindow.right"), TEXT("l2ui.nwindow.right_click"), 0);
    RightButton->ID = NID_CREATE_RIGHT;
    RightButton->SetWindowRect(((DWORD)Width >> 1) + 44, Height - 100, 44, 44);
    CreateChild(RightButton, NWS_VISIBLE);
    RightButton->HideWindow();

    ZoomInButton = new NCButton(TEXT("l2ui.nwindow.zoomin"), TEXT("l2ui.nwindow.zoomin_click"), 0);
    ZoomInButton->ID = NID_CREATE_ZOOM;
    ZoomInButton->SetWindowRect(((DWORD)Width >> 1) - 22, Height - 100, 44, 44);
    CreateChild(ZoomInButton, NWS_VISIBLE);
    ZoomInButton->HideWindow();

    ZoomOutButton = new NCButton(TEXT("l2ui.nwindow.zoomout"), TEXT("l2ui.nwindow.zoomout_click"), 0);
    ZoomOutButton->ID = NID_CREATE_ZOOM;
    ZoomOutButton->SetWindowRect(((DWORD)Width >> 1) - 22, Height - 100, 44, 44);
    CreateChild(ZoomOutButton, NWS_VISIBLE);
    ZoomOutButton->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x100857f0, NCPawnCreateWnd, OnCreate)

// 0x10085e60 (vslot 36)
INT NCPawnCreateWnd::OnDestroy()
{
    DestroyChild(this, SetupWnd);
    DestroyChild(this, InfoWnd);
    DestroyChild(this, CreateButton);
    DestroyChild(this, PreviousButton);
    DestroyChild(this, LeftButton);
    DestroyChild(this, RightButton);
    DestroyChild(this, ZoomInButton);
    DestroyChild(this, ZoomOutButton);
    NCWnd::OnDestroy();
    return 1;
}
NW_HOOK_METHOD(0x10085e60, NCPawnCreateWnd, OnDestroy)

// 0x10083e30 (vslot 42) - the view buttons wait for a gender
void NCPawnCreateWnd::ShowWindow()
{
    NCWnd::ShowWindow();
    ShowViewButtons(0);
}
NW_HOOK_METHOD(0x10083e30, NCPawnCreateWnd, ShowWindow)

// 0x10083d40 (vslot 43)
void NCPawnCreateWnd::HideWindow()
{
    NCWnd::HideWindow();
    SetupWnd->ResetPreviews();
}
NW_HOOK_METHOD(0x10083d40, NCPawnCreateWnd, HideWindow)

// 0x10085db0 (vslot 50) - draws nothing (the stage shows through)
INT NCPawnCreateWnd::OnPaint(NCGDevice* Device)
{
    guard(NCPawnCreateWnd::OnPaint);
    Device->SetClip(ScreenX, ScreenY, Width, Height);
    Device->BeginPaint(ScreenX, ScreenY);
    Device->EndPaint();
    Device->ReleaseClip();
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10085db0, NCPawnCreateWnd, OnPaint)

// 0x10083d20 (vslot 62) - "Previous": back to the character selection
INT NCPawnCreateWnd::OnPrevious()
{
    GConsole->ReturnToCharacterSelect();
    return 1;
}
NW_HOOK_METHOD(0x10083d20, NCPawnCreateWnd, OnPrevious)

// 0x10086000 (vslot 63) - "Create Character": the choices must be complete and the name
// valid. The stats sent are the last class template's the server sent (SetRecommendedData keeps
// one).
INT NCPawnCreateWnd::OnCreateCharacter()
{
    CharacterInfo Info;
    if (!SetupWnd->GetCreateInfo(&Info))
        return 1;
    if (Info.Name[0] && (GConsole->HasInvalidChars(Info.Name) || !GConsole->IsValidName(Info.Name)))
    {
        GConsole->ShowMessageWnd(SystemMessage(MSG_IncorrectName));
        SetupWnd->NameEdit->ClearText();
        SetupWnd->NameEdit->SetFocus();
        return 1;
    }
    for (INT i = 0; i < 6; i++)
        Info.Stats[i] = Recommended.Stats[i].Value;
    Info.HairStyle = SetupWnd->HairStyle;
    Info.HairColor = SetupWnd->HairColor;
    Info.Face = SetupWnd->Face;
    GConsole->RequestCharacterCreate(Info);
    return 1;
}
NW_HOOK_METHOD(0x10086000, NCPawnCreateWnd, OnCreateCharacter)

// 0x10083e50 (vslot 64) - the chosen default character turns while the button is held
INT NCPawnCreateWnd::OnTurnLeft(INT bPressed)
{
    INT Type = SetupWnd->GetCharacterType();
    if (bPressed)
        GConsole->DefaultCharacterTurn(Type, 2.0f);
    else
        GConsole->DefaultCharacterStop(Type);
    return 1;
}
NW_HOOK_METHOD(0x10083e50, NCPawnCreateWnd, OnTurnLeft)

// 0x10083e90 (vslot 65)
INT NCPawnCreateWnd::OnTurnRight(INT bPressed)
{
    INT Type = SetupWnd->GetCharacterType();
    if (bPressed)
        GConsole->DefaultCharacterTurn(Type, -2.0f);
    else
        GConsole->DefaultCharacterStop(Type);
    return 1;
}
NW_HOOK_METHOD(0x10083e90, NCPawnCreateWnd, OnTurnRight)

// 0x10086130 (vslot 66) - zoom on the chest and back: the event is the gender view's name
// plus "_CHEST" ("ELF_WWOMAN_CHEST"), played in reverse to zoom out. The stock code builds
// the name in both branches.
INT NCPawnCreateWnd::OnZoom()
{
    bZoomed = !bZoomed;
    INT SelRace = SetupWnd->RaceSelect->Selected;
    INT SelClassType = SetupWnd->ClassTypeSelect->Selected;
    INT SelSex = SetupWnd->SexSelect->Selected;
    NString Event;
    if (bZoomed)
    {
        ZoomOutButton->ShowWindow();
        ZoomInButton->HideWindow();
    }
    else
    {
        ZoomOutButton->HideWindow();
        ZoomInButton->ShowWindow();
    }
    if (SelRace >= 0)
        Event.Append(GRaceEventNames[SelRace]);
    if (SelSex >= 0)
    {
        if (SelClassType == 0)
            Event.Append(TEXT("_K"));
        else if (SelClassType == 1)
            Event.Append(TEXT("_W"));
        Event.Append(GSexEventNames[SelSex]);
        Event.Append(TEXT("_CHEST"));
    }
    GConsole->ExecLobbyEvent(Event.GetAt(0), bZoomed ? 0 : 1);
    return 1;
}
NW_HOOK_METHOD(0x10086130, NCPawnCreateWnd, OnZoom)

// 0x10083d00 - NConsoleWnd::AddRecommandedData passes each template on
void NCPawnCreateWnd::SetRecommendedData(RecommandedDataStr Data)
{
    Recommended = Data;
}
NW_HOOK_METHOD(0x10083d00, NCPawnCreateWnd, SetRecommendedData)

// 0x10083d60 - the turn and zoom buttons (zoom in or out by bZoomed) with only the chosen
// default character on the stage; hiding them also forgets the zoom. Not inlined: the stock
// callers call it.
__declspec(noinline) void NCPawnCreateWnd::ShowViewButtons(UBOOL bShow)
{
    if (bShow)
    {
        LeftButton->ShowWindow();
        RightButton->ShowWindow();
        if (bZoomed)
        {
            ZoomOutButton->ShowWindow();
            ZoomInButton->HideWindow();
        }
        else
        {
            ZoomOutButton->HideWindow();
            ZoomInButton->ShowWindow();
        }
        GConsole->ShowOnlyOneDefaultCharacter(SetupWnd->GetCharacterType());
    }
    else
    {
        LeftButton->HideWindow();
        RightButton->HideWindow();
        ZoomInButton->HideWindow();
        ZoomOutButton->HideWindow();
        bZoomed = 0;
    }
}
NW_HOOK_METHOD(0x10083d60, NCPawnCreateWnd, ShowViewButtons)

// 0x10085fe0 - NConsoleWnd::AcceptNewCharacter empties the screen before showing it
void NCPawnCreateWnd::Reset()
{
    SetupWnd->Reset();
    InfoWnd->ClearText();
}
NW_HOOK_METHOD(0x10085fe0, NCPawnCreateWnd, Reset)

NW_CHECK_VTABLE(NCPawnCreateWnd, 0x100ea2b8, 67)

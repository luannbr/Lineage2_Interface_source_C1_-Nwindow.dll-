// Phase 6: NCRecipeWnd, NCRecipeListWnd and the tool tip of the craft windows
// (include/features/craft/CraftWnd.h). NWindow_C1 src/inject/craft/NwUi.cpp, whose stock
// addresses are our methods here:
//   NCWnd ctor 0x10023160, WindowProc 0x10022b60, OnCreate 0x10023e50, OnDestroy 0x10024020,
//   ShowWindow 0x10022d80, HideWindow 0x10022dc0, OffsetWindow 0x10039080,
//   SetWindowRect 0x10023d90, AddChildWindow 0x10023ca0 + AttachEntry 0x10002a80
//   (NCWnd::CreateChild), BringChildToTop 0x10022e00; NCScrollWnd WindowProc 0x10020f40,
//   SetContentHeight 0x10021050; NCSliderBar::SetValue 0x10021800; NCFrameCtrl(1) 0x1000bfa0;
//   NCButton(Normal, Pushed, 0) 0x10001a20, SetText 0x100011e0; NCObject::FindTexture
//   0x10020e00; NCGDevice SetClip 0x1000d100, ReleaseClip 0x1000d110, DrawTexture 0x1000cd30,
//   DrawStretchedTex 0x1000cc40 / 0x1000ccb0, DrawText 0x1000cb00, GetTextExtent 0x1000c730;
//   NConsoleWnd::GetIconName 0x10054c90 (the item table 0x1004dda0 and the icon's L2FName);
//   NCConsole::AddSystemText 0x1003e6f0; the prompts: NDialogInfo 0x10006230, SetText
//   0x10005b70, SetSystemMessage 0x10005b10, NConsoleWnd::ShowDialog 0x1004a0e0,
//   NCDialogBox::Reset 0x10005d60; the focused entry GFocusEntry (0x10123d54).
#include "features/craft/CraftWnd.h"
#include "features/NwFeatures.h"

#include "nwindow/NCButton.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCDialogBox.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCFrameCtrl.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCSliderBar.h"
#include "nwindow/NDialogInfo.h"

NW_IMPLEMENT_L2_CLASS(NCRecipeListWnd, NCScrollWnd, 0)
NW_IMPLEMENT_L2_CLASS(NCRecipeWnd, NCWnd, CLASS_Abstract)

// names ours
enum
{
    RECIPEWND_FrameID = 0x7AF0,          // the title bar's NWM_COMMAND id (NwUi kFrameId)
    RECIPEWND_TipW = 0xB8,               // the stock item tool tip's width
    RECIPEWND_TipWrap = 0xA8,            // and its text's wrap width
    DIALOGID_Amount = 2,                 // NCDialogBox: the amount (keypad) prompt, or yes / no
                                         //   without the edit line
    DIALOGID_Text = 0x24,                // the text prompt: OK 0xF0000008, Cancel 0xF0000009
    NWM_DIALOG_TEXTOK = (INT)0xF0000008,
    NWM_DIALOG_TEXTCANCEL = (INT)0xF0000009,
};

/*-----------------------------------------------------------------------------
    Helpers.
-----------------------------------------------------------------------------*/

static NCConsole* Desktop()
{
    return GConsole ? GConsole->GetDesktop() : NULL;
}

static UBOOL IsVisible(NCWnd* Wnd)
{
    return Wnd && (Wnd->Style & NWS_VISIBLE) != 0;
}

// Entry is Root's or one of its descendants' (NwUi in_subtree)
// NwUi ran each window callback under __try: a fault in one loses that call, not the game
#define CRAFT_GUARDED(Call)     do                            {                                 try                           {                                 Call;                     }                             catch (...)                   {                             }                         } while (0)

static UBOOL InSubtree(NWndEntry* Entry, NCWnd* Root)
{
    for (INT Guard = 0; Entry && Guard < 64; Guard++)
    {
        if (Entry->Wnd == Root)
            return 1;
        Entry = Entry->Parent;
    }
    return 0;
}

// a hidden window that keeps the focus keeps taking the keys (NwUi kill_focus_in)
static void KillFocusIn(NCWnd* Root)
{
    NWndEntry* Focus = GFocusEntry;
    if (Focus && InSubtree(Focus, Root) && Focus->Wnd)
        Focus->Wnd->KillFocus();
}

// the paint prologue of every stock OnPaint: origin and clip on Wnd (Grow widens the clip: the
// 1 px frame of an icon grid's selection)
static void BeginWndPaint(NCGDevice* Device, NCWnd* Wnd, INT Grow)
{
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX - Grow, Wnd->ScreenY - Grow, Wnd->Width + 2 * Grow,
                    Wnd->Height + 2 * Grow);
}

static void EndWndPaint(NCGDevice* Device)
{
    Device->ReleaseClip();
    Device->EndPaint();
}

// the item icons, per item id (a missing one kept as NULL)
struct NCraftIcon
{
    INT ItemID;
    UTexture* Tex;
};
static NCraftIcon GIcons[512];
static INT GNumIcons;
// the tool tip's textures, loaded again for another console
static UTexture* GTipTop;
static UTexture* GTipMid;
static UTexture* GTipBottom;
static NConsoleWnd* GTipConsole;

/*-----------------------------------------------------------------------------
    NCRecipeWnd: what the paints use.
-----------------------------------------------------------------------------*/

SIZE NCRecipeWnd::Measure(const TCHAR* Text, INT WrapW)
{
    SIZE Size;
    Size.cx = 0;
    Size.cy = 0;
    if (Text && *Text && GCanvas)
        NCGDevice::GetTextExtent(&Size, Text, WrapW, 0, 0, 0, 0);
    return Size;
}

void NCRecipeWnd::PaintText(NCGDevice* Device, INT X, INT Y, DWORD Color, const TCHAR* Text,
                            INT WrapW)
{
    if (Text && *Text)
        Device->DrawText(X, Y, Color, Text, WrapW, 0, 0, (L2FontType)0, 0, 0, 0, 0, 0, 0);
}

void NCRecipeWnd::PaintTextRight(NCGDevice* Device, INT Right, INT Y, DWORD Color,
                                 const TCHAR* Text)
{
    PaintText(Device, Right - Measure(Text).cx, Y, Color, Text);
}

void NCRecipeWnd::PaintTex(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H, INT U,
                           INT V, INT UL, INT VL)
{
    if (Tex)
        Device->DrawTexture(X, Y, W, H, U, V, UL, VL, Tex);
}

void NCRecipeWnd::PaintStretchUV(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H,
                                 INT U, INT V, INT UL, INT VL)
{
    if (Tex && W > 0 && H > 0)
        Device->DrawStretchedTex(X, Y, W, H, U, V, UL, VL, Tex);
}

void NCRecipeWnd::PaintStretch(NCGDevice* Device, UTexture* Tex, INT X, INT Y, INT W, INT H)
{
    if (Tex)
        Device->DrawStretchedTex(X, Y, W, H, Tex);
}

void NCRecipeWnd::PaintIcon(NCGDevice* Device, INT ItemID, INT X, INT Y)
{
    if (UTexture* Tex = ItemIcon(ItemID))
        PaintTex(Device, Tex, X, Y, 32, 32, 0, 0, 32, 32);
}

UTexture* NCRecipeWnd::LoadTex(const TCHAR* Name)
{
    NCConsole* D = Desktop();
    if (!D || !Name)
        return NULL;
    return D->FindTexture(Name);
}

// the icon the inventory draws: the item table's icon name (NConsoleWnd::GetIconName)
UTexture* NCRecipeWnd::ItemIcon(INT ItemID)
{
    for (INT i = 0; i < GNumIcons; i++)
        if (GIcons[i].ItemID == ItemID)
            return GIcons[i].Tex;
    UTexture* Tex = NULL;
    const TCHAR* Name = GConsole ? GConsole->GetIconName(ItemID) : NULL;
    if (Name && *Name)
        Tex = LoadTex(Name);
    if (GNumIcons < (INT)ARRAY_COUNT(GIcons))
    {
        GIcons[GNumIcons].ItemID = ItemID;
        GIcons[GNumIcons].Tex = Tex;
        GNumIcons++;
    }
    return Tex;
}

const TCHAR* NCRecipeWnd::ItemName(INT ItemID)
{
    if (ItemID <= 0)
        return TEXT("");
    const TCHAR* Name = GL2GameData.GetItemName(ItemID);
    return Name ? Name : TEXT("");
}

const TCHAR* NCRecipeWnd::ItemDescription(INT ItemID)
{
    if (ItemID <= 0)
        return TEXT("");
    const TCHAR* Text = GL2GameData.GetItemDescription(ItemID);
    return Text ? Text : TEXT("");
}

void NCRecipeWnd::Sound(INT Index)
{
    if (GConsole && GConsole->Engine)
        GConsole->Engine->OnInterfacePlaySound(Index);
}

void NCRecipeWnd::SystemMessage(const TCHAR* Text)
{
    NCConsole* D = Desktop();
    if (D && Text)
        D->AddSystemText(Text, 0, 0xFFB09B79);
}

INT NCRecipeWnd::ScreenW()
{
    NCConsole* D = Desktop();
    return D ? D->Width : 0;
}

INT NCRecipeWnd::ScreenH()
{
    NCConsole* D = Desktop();
    return D ? D->Height : 0;
}

// the real cursor in the game window's client area (NWindow_C1 WindowFix_CursorClientPos)
UBOOL NCRecipeWnd::CursorPos(INT* X, INT* Y)
{
    POINT P;
    if (!GViewportHWnd || !GetCursorPos(&P) || !ScreenToClient(GViewportHWnd, &P))
        return 0;
    *X = P.x;
    *Y = P.y;
    return 1;
}

void NCRecipeWnd::ForgetCaches()
{
    GNumIcons = 0;
    GTipConsole = NULL;
}

/*-----------------------------------------------------------------------------
    NCRecipeWnd: the window.
-----------------------------------------------------------------------------*/

NCRecipeWnd::NCRecipeWnd()
{
    Title[0] = 0;
    PosKey[0] = 0;
    BackTexName = NULL;
    DefaultFromRight = 290;
    DefaultTop = 205;
    bWantOpen = 0;
    bClosingByCode = 0;
    bCreating = 0;
    DialogKind = CRAFTDLG_None;
    NumChildren = 0;
    NumLists = 0;
    BackTex = NULL;
    MpTex = NULL;
    ItemClickTex = NULL;
    for (INT i = 0; i < 5; i++)
        GradeTex[i] = NULL;
}

NCRecipeWnd::NCRecipeWnd(const TCHAR* InBackTexName, const TCHAR* InTitle, const TCHAR* InPosKey,
                         INT InFromRight, INT InTop)
{
    lstrcpynW(Title, InTitle ? InTitle : TEXT(""), ARRAY_COUNT(Title));
    lstrcpynW(PosKey, InPosKey ? InPosKey : TEXT(""), ARRAY_COUNT(PosKey));
    BackTexName = InBackTexName;
    DefaultFromRight = InFromRight;
    DefaultTop = InTop;
    bWantOpen = 0;
    bClosingByCode = 0;
    bCreating = 0;
    DialogKind = CRAFTDLG_None;
    NumChildren = 0;
    NumLists = 0;
    BackTex = NULL;
    MpTex = NULL;
    ItemClickTex = NULL;
    for (INT i = 0; i < 5; i++)
        GradeTex[i] = NULL;
}

// NwUi CreateNow: at the desktop's origin, the size of every recipe window, hidden until
// Open places it
void NCRecipeWnd::Create(NCWnd* Parent)
{
    SetWindowRect(0, 0, RECIPEWND_Width, RECIPEWND_Height);
    Parent->CreateChild(this, NWS_VISIBLE);   // WM_CREATE: OnCreate
    bClosingByCode = 1;
    bCreating = 1;
    HideWindow();
    bCreating = 0;
    bClosingByCode = 0;
}

void NCRecipeWnd::Open()
{
    bWantOpen = 1;
    if (!IsVisible(this))
        RestorePos();
    ShowWindow();
    if (NCConsole* D = Desktop())
        D->BringChildToTop(Entry);
}

void NCRecipeWnd::Close()
{
    if (!bWantOpen)
        return;
    CancelDialog();
    bWantOpen = 0;
    bClosingByCode = 1;
    HideWindow();
    bClosingByCode = 0;
}

UBOOL NCRecipeWnd::FocusInside()
{
    return GFocusEntry && InSubtree(GFocusEntry, this);
}

UBOOL NCRecipeWnd::Frontmost()
{
    NCConsole* D = Desktop();
    return D && D->ChildHead && D->ChildHead->Entry == Entry;
}

INT NCRecipeWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCWnd::WindowProc(Msg, wParam, lParam))
        return 1;
    switch ((INT)Msg)
    {
    case NWM_COMMAND:
        CRAFT_GUARDED(OnCommand((INT)wParam, (INT)lParam));
        return 1;
    case NWM_DIALOG_RESULT:   // the amount, or 1: yes
    {
        INT Kind = DialogKind;
        DialogKind = CRAFTDLG_None;
        if (bWantOpen)
        {
            if (Kind == CRAFTDLG_Number)
                CRAFT_GUARDED(OnDialogNumber((SQWORD)(DWORD)wParam));
            else if (Kind == CRAFTDLG_Confirm)
                CRAFT_GUARDED(OnDialogConfirm());
        }
        return 1;
    }
    case NWM_DIALOG_TEXTOK:   // the prompt's line, read as it is (Left + Comp + Right)
    {
        TCHAR Text[256];
        INT N = 0;
        NCConsole* D = Desktop();
        NCEditBox* Edit = D && D->MainDialogBox ? D->MainDialogBox->EditBox : NULL;
        if (Edit)
        {
            FString* Parts[3] = {&Edit->Left, &Edit->Comp, &Edit->Right};
            for (INT p = 0; p < 3; p++)
            {
                const TCHAR* Data = Parts[p]->GetCharData();
                INT Num = Parts[p]->Num();
                for (INT i = 0; Data && i < Num - 1 && N < 255; i++)
                    Text[N++] = Data[i];
            }
        }
        Text[N] = 0;
        INT Kind = DialogKind;
        DialogKind = CRAFTDLG_None;
        if (Kind == CRAFTDLG_Text && bWantOpen)
            CRAFT_GUARDED(OnDialogText(Text));
        return 1;   // the prompt closes
    }
    case NWM_DIALOG_TEXTCANCEL:
        DialogKind = CRAFTDLG_None;
        return 1;
    }
    return 0;
}

// the stock base, then the title bar (a decorated NCFrameCtrl: title, close button, drag),
// then the window's controls
INT NCRecipeWnd::OnCreate()
{
    NCWnd::OnCreate();
    FrameCtrl = new NCFrameCtrl(1);
    FrameID = RECIPEWND_FrameID;
    FrameCtrl->ID = RECIPEWND_FrameID;
    FrameCtrl->SetWindowRect(0, 0, Width, RECIPEWND_TitleH);
    CreateChild(FrameCtrl, NWS_VISIBLE);
    SetTitle(Title);
    CRAFT_GUARDED(OnCreateChildren());
    return 1;
}

INT NCRecipeWnd::OnDestroy()
{
    for (INT i = NumChildren - 1; i >= 0; i--)
    {
        RemoveChildWindow(Children[i]->Entry);
        delete Children[i];
        Children[i] = NULL;
    }
    NumChildren = 0;
    NumLists = 0;
    return NCWnd::OnDestroy();   // the title bar
}

// the desktop shows every child when it shows (entering the world, Alt+H): a closed window
// stays hidden
void NCRecipeWnd::ShowWindow()
{
    if (!bWantOpen)
    {
        NCWnd::HideWindow();
        return;
    }
    NCWnd::ShowWindow();
}

// The close button hides the window while the desktop is visible: a close by the user. Alt+H
// and leaving the world hide the desktop first (its bit goes before its children hide): the
// window stays open.
void NCRecipeWnd::HideWindow()
{
    UBOOL bParentVisible = Entry && Entry->Parent && IsVisible(Entry->Parent->Wnd);
    UBOOL bWasVisible = IsVisible(this);
    KillFocusIn(this);
    NCWnd::HideWindow();
    if (bWasVisible && bParentVisible && !bCreating)
        SavePos();
    if (bWantOpen && bParentVisible && !bClosingByCode)
    {
        bWantOpen = 0;
        CancelDialog();
        CRAFT_GUARDED(OnUserClose());
    }
}

INT NCRecipeWnd::OnPaint(NCGDevice* Device)
{
    BeginWndPaint(Device, this, 0);
    CRAFT_GUARDED(PaintWindow(Device));
    EndWndPaint(Device);
    return 1;
}

// the wheel reaches the focused window: a list under the cursor scrolls
INT NCRecipeWnd::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    INT X, Y;
    UBOOL bCursor = CursorPos(&X, &Y);
    for (INT i = 0; i < NumLists; i++)
    {
        NCRecipeListWnd* L = Lists[i];
        if (bCursor && X >= L->ScreenX && Y >= L->ScreenY && X < L->ScreenX + L->Width &&
            Y < L->ScreenY + L->Height)
            return L->OnMouseWheel(wParam, lParam);
    }
    return 0;
}

NCButton* NCRecipeWnd::AddButton(INT ID, INT X, INT Y, INT W, INT H, const TCHAR* Label,
                                 const TCHAR* NormalTex, const TCHAR* PushedTex)
{
    NCButton* Button = new NCButton(NormalTex, PushedTex, 0);
    Button->ID = ID;
    Button->SetWindowRect(X, Y, W, H);
    CreateChild(Button, NWS_VISIBLE);
    if (NumChildren < (INT)ARRAY_COUNT(Children))
        Children[NumChildren++] = Button;
    if (Label)
        SetButtonText(Button, Label);
    return Button;
}

NCRecipeListWnd* NCRecipeWnd::AddList(INT ID, INT X, INT Y, INT W, INT H, INT RowH, INT Cols,
                                      INT CellW, INT PadTop)
{
    NCRecipeListWnd* List = new NCRecipeListWnd(this, ID, RowH > 0 ? RowH : 16,
                                                Cols > 0 ? Cols : 1,
                                                CellW > 0 ? CellW : W - RECIPEWND_ScrollBarW,
                                                PadTop);
    List->SetWindowRect(X, Y, W, H);
    CreateChild(List, NWS_VISIBLE);   // WM_CREATE: the scroll buttons and bar
    if (NumChildren < (INT)ARRAY_COUNT(Children))
        Children[NumChildren++] = List;
    if (NumLists < (INT)ARRAY_COUNT(Lists))
        Lists[NumLists++] = List;
    List->LineStep = List->RowH;
    List->PageStep = H;
    List->SetItems(0);
    return List;
}

// the button's label buffer is 32 characters, copied without a bound
void NCRecipeWnd::SetButtonText(NCButton* Button, const TCHAR* Text)
{
    if (!Button)
        return;
    TCHAR Label[32];
    lstrcpynW(Label, Text ? Text : TEXT(""), ARRAY_COUNT(Label));
    Button->SetText(Label);
}

// The stock prompts (NDialogInfo): ID 2 is the amount prompt (the keypad; without the edit
// line, yes / no), 0x24 the text prompt. bMainBox picks the box with the edit line (+0x148)
// or the other one (+0x14C).
UBOOL NCRecipeWnd::AskNumber(INT SysMsgID)
{
    if (!GConsole || !Desktop())
        return 0;
    NDialogInfo Info;
    Info.ParamText[0] = 0;
    Info.ID = DIALOGID_Amount;
    Info.Type = 3;              // digits, with the keypad
    Info.Owner = Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.bLargeAmount = 1;      // up to 100,000,000
    Info.bProgress = 0;
    Info.SetSystemMessage(SysMsgID);
    GConsole->ShowDialog(&Info);
    DialogKind = CRAFTDLG_Number;
    return 1;
}

UBOOL NCRecipeWnd::AskText(INT SysMsgID, const TCHAR* Initial)
{
    if (!GConsole || !Desktop())
        return 0;
    NDialogInfo Info;
    Info.ParamText[0] = 0;
    Info.ID = DIALOGID_Text;
    Info.Type = 0;
    Info.Owner = Entry;
    Info.bMainBox = 1;
    Info.bInput = 1;
    Info.bLargeAmount = 0;
    Info.bProgress = 0;
    Info.SetSystemMessage(SysMsgID);
    GConsole->ShowDialog(&Info);
    DialogKind = CRAFTDLG_Text;
    // ShowDialog emptied the line
    NCConsole* D = Desktop();
    if (Initial && *Initial && D->MainDialogBox && D->MainDialogBox->EditBox)
        D->MainDialogBox->EditBox->SetText(Initial);
    return 1;
}

UBOOL NCRecipeWnd::AskConfirm(const TCHAR* Text)
{
    if (!GConsole || !Desktop())
        return 0;
    NDialogInfo Info;
    Info.ParamText[0] = 0;
    Info.ID = DIALOGID_Amount;  // without the edit line OK answers NWM_DIALOG_RESULT, 1
    Info.Type = 0;
    Info.Owner = Entry;
    Info.bMainBox = 0;          // the box without the edit line
    Info.bInput = 0;
    Info.bLargeAmount = 0;
    Info.bProgress = 0;
    Info.SetText(Text);
    GConsole->ShowDialog(&Info);
    DialogKind = CRAFTDLG_Confirm;
    return 1;
}

// a prompt of the desktop's that is still waiting for this window goes
void NCRecipeWnd::CancelDialog()
{
    if (!DialogKind)
        return;
    DialogKind = CRAFTDLG_None;
    NCConsole* D = Desktop();
    if (!D || !Entry)
        return;
    NCDialogBox* Boxes[2] = {D->MainDialogBox, D->SecondDialogBox};
    for (INT i = 0; i < 2; i++)
    {
        NCDialogBox* Box = Boxes[i];
        if (Box && Box->Owner == Entry && Box->IsShowWindow())
        {
            Box->Reset();
            Box->HideWindow();
            return;
        }
    }
}

// LineageWindow.ini [CraftUi] <PosKey>X / Y: the text's number, an optional '-' (NwUi ini_get)
static UBOOL ReadPos(const char* Key, INT* Out)
{
    char Text[24];
    nwf::IniString("CraftUi", Key, "", Text, sizeof(Text));
    if (!Text[0])
        return 0;
    INT V = 0;
    INT i = 0;
    UBOOL bNeg = Text[0] == '-';
    if (bNeg)
        i++;
    for (; Text[i] >= '0' && Text[i] <= '9'; i++)
        V = V * 10 + (Text[i] - '0');
    *Out = bNeg ? -V : V;
    return 1;
}

static void PosKeys(const TCHAR* PosKey, char* KeyX, char* KeyY)
{
    char Key[32];
    WideCharToMultiByte(CP_ACP, 0, PosKey, -1, Key, sizeof(Key), NULL, NULL);
    wsprintfA(KeyX, "%sX", Key);
    wsprintfA(KeyY, "%sY", Key);
}

void NCRecipeWnd::SavePos()
{
    char KeyX[40], KeyY[40];
    PosKeys(PosKey, KeyX, KeyY);
    nwf::IniWriteInt("CraftUi", KeyX, LocalX);
    nwf::IniWriteInt("CraftUi", KeyY, LocalY);
}

// where the last window of this key was closed, kept on the screen
void NCRecipeWnd::RestorePos()
{
    NCConsole* D = Desktop();
    if (!D)
        return;
    INT SW = D->Width;
    INT SH = D->Height;
    INT X = SW - DefaultFromRight;
    INT Y = DefaultTop;
    char KeyX[40], KeyY[40];
    PosKeys(PosKey, KeyX, KeyY);
    ReadPos(KeyX, &X);
    ReadPos(KeyY, &Y);
    if (X > SW - Width)
        X = SW - Width;
    if (Y > SH - Height)
        Y = SH - Height;
    if (X < 0)
        X = 0;
    if (Y < 0)
        Y = 0;
    INT DX = X - LocalX;
    INT DY = Y - LocalY;
    if (DX || DY)
        OffsetWindow(DX, DY);
}

/*-----------------------------------------------------------------------------
    NCRecipeWnd: the common look (NWindow_C1 CraftWindows.cpp RecipeWnd).
-----------------------------------------------------------------------------*/

void NCRecipeWnd::LoadCommon()
{
    static const TCHAR* const GradeNames[5] = {TEXT("symbol.grade_d"), TEXT("symbol.grade_c"),
                                               TEXT("symbol.grade_b"), TEXT("symbol.grade_a"),
                                               TEXT("symbol.grade_s")};
    BackTex = LoadTex(BackTexName);
    MpTex = LoadTex(TEXT("L2UI.PlayerStatusWnd.MP"));
    ItemClickTex = LoadTex(TEXT("l2ui.nwindow.item_click"));
    for (INT i = 0; i < 5; i++)
        GradeTex[i] = LoadTex(GradeNames[i]);
}

void NCRecipeWnd::PaintBack(NCGDevice* Device)
{
    PaintTex(Device, BackTex, 0, RECIPEWND_TitleH, RECIPEWND_Width,
             RECIPEWND_Height - RECIPEWND_TitleH, 0, 0, RECIPEWND_Width,
             RECIPEWND_Height - RECIPEWND_TitleH);
}

// cur * 152 / max px of a 1 x 10 texel column of PlayerStatusWnd.MP, stretched
void NCRecipeWnd::PaintMpBar(NCGDevice* Device, INT X, INT Y, INT Cur, INT Max)
{
    enum { BarW = 152 };
    if (Max <= 0)
        return;
    SQWORD Bar = (SQWORD)Cur * BarW / Max;
    if (Bar <= 0)
        return;
    if (Bar > BarW)
        Bar = BarW;
    PaintStretchUV(Device, MpTex, X, Y, (INT)Bar, 10, 0, 0, 1, 10);
}

// C2 draws "`gradeX`" after the product name: the 16 x 16 symbol.grade_X
void NCRecipeWnd::PaintGrade(NCGDevice* Device, INT X, INT Y, INT Grade)
{
    if (Grade >= 1 && Grade <= 5)
        PaintTex(Device, GradeTex[Grade - 1], X, Y, 16, 16, 0, 0, 16, 16);
}

void NCRecipeWnd::PaintSelection(NCGDevice* Device, INT X, INT Y)
{
    PaintTex(Device, ItemClickTex, X - 1, Y - 1, 34, 34, 0, 0, 34, 34);
}

/*-----------------------------------------------------------------------------
    NCRecipeListWnd.
-----------------------------------------------------------------------------*/

NCRecipeListWnd::NCRecipeListWnd()
{
    Owner = NULL;
    ListID = 0;
    RowH = 16;
    Cols = 1;
    CellW = 0;
    PadTop = 0;
    Count = 0;
    Selected = -1;
}

NCRecipeListWnd::NCRecipeListWnd(NCRecipeWnd* InOwner, INT InID, INT InRowH, INT InCols,
                                 INT InCellW, INT InPadTop)
{
    Owner = InOwner;
    ListID = InID;
    RowH = InRowH;
    Cols = InCols;
    CellW = InCellW;
    PadTop = InPadTop;
    Count = 0;
    Selected = -1;
}

// the dispatcher sends the double click like a click (NWM routes 0x203 to the window hit)
INT NCRecipeListWnd::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_LBUTTONDBLCLK)
    {
        Click(lParam, 1);
        return 1;
    }
    return NCScrollWnd::WindowProc(Msg, wParam, lParam);
}

// the rows from the first visible, each cell by the owner, clipped to the list (an icon
// grid's clip 1 px wider: the selection frame)
INT NCRecipeListWnd::OnPaint(NCGDevice* Device)
{
    if (!Owner)
        return 1;
    BeginWndPaint(Device, this, Cols > 1 ? 1 : 0);
    INT Row = (-ScrollY - PadTop) / RowH;
    if (Row < 0)
        Row = 0;
    for (;; Row++)
    {
        INT RY = PadTop + Row * RowH + ScrollY;
        if (RY >= Height || Row * Cols >= Count)
            break;
        for (INT C = 0; C < Cols; C++)
        {
            INT i = Row * Cols + C;
            if (i >= Count)
                break;
            CRAFT_GUARDED(Owner->PaintListItem(this, Device, i, C * CellW, RY, i == Selected));
        }
    }
    EndWndPaint(Device);
    return 1;
}

// NWM_DRAWTOOLTIP, sent to the window under the cursor after the paint: the owner's tool tip of
// the item there, which may cover the whole screen
INT NCRecipeListWnd::OnDrawToolTip(NCGDevice* Device)
{
    INT CX, CY;
    if (!Owner || !NCRecipeWnd::CursorPos(&CX, &CY))
        return 1;
    INT i = ItemAt(CX, CY);
    if (i < 0)
        return 1;
    Device->OriginX = ScreenX;
    Device->OriginY = ScreenY;
    Device->SetClip(0, 0, NCRecipeWnd::ScreenW(), NCRecipeWnd::ScreenH());
    INT Row = i / Cols;
    INT Col = i % Cols;
    CRAFT_GUARDED(Owner->PaintListToolTip(this, Device, i, Col * CellW, PadTop + Row * RowH + ScrollY));
    EndWndPaint(Device);
    return 1;
}

// the hover comes from the cursor when the tool tip pass asks
INT NCRecipeListWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    return 1;
}

INT NCRecipeListWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    Click(lParam, 0);
    return 1;
}

// one row a notch, like the C2 lists
INT NCRecipeListWnd::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    INT Notches = (SWORD)HIWORD(wParam) / 120;
    ScrollBy(-Notches * RowH);
    return 1;
}

INT NCRecipeListWnd::DrawWidth()
{
    return Width - RECIPEWND_ScrollBarW;
}

void NCRecipeListWnd::SetItems(INT N)
{
    Count = N < 0 ? 0 : N;
    if (Selected >= Count)
        Selected = -1;
    INT Rows = (Count + Cols - 1) / Cols;
    SetContentHeight(PadTop + Rows * RowH);
    ScrollBy(0);   // clamp and bar
}

void NCRecipeListWnd::ScrollBy(INT Pixels)
{
    INT Y = ScrollY - Pixels;   // <= 0
    if (Y < MinScrollY)
        Y = MinScrollY;
    if (Y > 0)
        Y = 0;
    ScrollY = Y;
    if (ScrollBar)
        ScrollBar->SetValue(-Y);
}

INT NCRecipeListWnd::ItemAt(INT X, INT Y)
{
    INT LX = X - ScreenX;
    INT VY = Y - ScreenY;
    if (LX < 0 || LX >= DrawWidth() || VY < 0 || VY >= Height)
        return -1;
    INT CY = VY - ScrollY - PadTop;
    if (CY < 0)
        return -1;
    INT Row = CY / RowH;
    INT Col = 0;
    if (Cols > 1)
    {
        Col = LX / CellW;
        if (Col >= Cols)
            return -1;
    }
    INT i = Row * Cols + Col;
    return i < Count ? i : -1;
}

void NCRecipeListWnd::Click(LPARAM Pos, UBOOL bDouble)
{
    if (!Owner)
        return;
    INT i = ItemAt((SWORD)LOWORD(Pos), (SWORD)HIWORD(Pos));
    if (i < 0)
        return;
    Selected = i;
    CRAFT_GUARDED(Owner->OnListClick(this, i, bDouble));
}

/*-----------------------------------------------------------------------------
    NCraftToolTip.
-----------------------------------------------------------------------------*/

NCraftToolTip::NCraftToolTip()
{
    NumLines = 0;
    Used = 0;
    Pool[0] = 0;
}

// the callers keep Used below 1023
INT NCraftToolTip::AddText(const TCHAR* Text)
{
    INT At = Used;
    while (Text && *Text && Used < 1023)
        Pool[Used++] = *Text++;
    Pool[Used++] = 0;
    return At;
}

void NCraftToolTip::Line(const TCHAR* Text, DWORD Color)
{
    if (NumLines >= (INT)ARRAY_COUNT(Lines))
        return;
    NLine& L = Lines[NumLines++];
    L.NumSegs = 0;
    L.bWrap = 0;
    Seg(Text, Color);
}

void NCraftToolTip::Seg(const TCHAR* Text, DWORD Color)
{
    if (!NumLines || Used >= 1023)
        return;
    NLine& L = Lines[NumLines - 1];
    if (L.NumSegs >= (INT)ARRAY_COUNT(L.Segs))
        return;
    L.Segs[L.NumSegs].Text = AddText(Text);
    L.Segs[L.NumSegs].Color = Color;
    L.NumSegs++;
}

void NCraftToolTip::Wrapped(const TCHAR* Text, DWORD Color)
{
    if (!Text || !*Text)
        return;
    Line(Text, Color);
    if (NumLines)
        Lines[NumLines - 1].bWrap = 1;
}

void NCraftToolTip::Draw(NCGDevice* Device, INT AX, INT AY, INT AW, INT AH)
{
    if (!NumLines)
        return;
    if (GTipConsole != GConsole)   // the textures go with the console's packages
    {
        GTipTop = NCRecipeWnd::LoadTex(TEXT("L2UI.Tooltip.Tool02"));
        GTipMid = NCRecipeWnd::LoadTex(TEXT("L2UI.Tooltip.Tool01"));
        GTipBottom = NCRecipeWnd::LoadTex(TEXT("L2UI.Tooltip.Tool03"));
        GTipConsole = GConsole;
    }
    SIZE A = NCRecipeWnd::Measure(TEXT("A"));
    INT LineH = (A.cy > 0 ? A.cy : 12) + 1;
    INT ContentH = 0;
    INT Heights[ARRAY_COUNT(Lines)];
    for (INT i = 0; i < NumLines; i++)
    {
        const NLine& L = Lines[i];
        Heights[i] = LineH;
        if (L.bWrap)
        {
            SIZE S = NCRecipeWnd::Measure(Pool + L.Segs[0].Text, RECIPEWND_TipWrap);
            if (S.cy > LineH)
                Heights[i] = S.cy + 1;
        }
        ContentH += Heights[i];
    }
    INT Strips = (ContentH + LineH - 1) / LineH;
    INT BoxH = 16 + Strips * LineH;
    // above the anchor like the stock item tool tip, below it when there is no room
    INT OX = Device->OriginX;
    INT OY = Device->OriginY;
    INT SW = NCRecipeWnd::ScreenW();
    INT X = AX;
    if (OX + X + RECIPEWND_TipW > SW)
        X = SW - OX - RECIPEWND_TipW;
    if (OX + X < 0)
        X = -OX;
    INT Y = AY - BoxH - 1;
    if (OY + Y < 0)
        Y = AY + AH + 1;
    NCRecipeWnd::PaintTex(Device, GTipTop, X, Y, RECIPEWND_TipW, 8, 0, 0, RECIPEWND_TipW, 8);
    for (INT S = 0; S < Strips; S++)
        NCRecipeWnd::PaintTex(Device, GTipMid, X, Y + 8 + S * LineH, RECIPEWND_TipW, LineH, 0, 0,
                              RECIPEWND_TipW, LineH);
    NCRecipeWnd::PaintTex(Device, GTipBottom, X, Y + 8 + Strips * LineH, RECIPEWND_TipW, 8, 0, 0,
                          RECIPEWND_TipW, 8);
    INT TY = Y + 8;
    for (INT i = 0; i < NumLines; i++)
    {
        const NLine& L = Lines[i];
        if (L.bWrap)
            NCRecipeWnd::PaintText(Device, X + 8, TY, L.Segs[0].Color, Pool + L.Segs[0].Text,
                                   RECIPEWND_TipWrap);
        else
        {
            INT TX = X + 8;
            for (INT s = 0; s < L.NumSegs; s++)
            {
                const TCHAR* T = Pool + L.Segs[s].Text;
                NCRecipeWnd::PaintText(Device, TX, TY, L.Segs[s].Color, T);
                TX += NCRecipeWnd::Measure(T).cx;
            }
        }
        TY += Heights[i];
    }
}

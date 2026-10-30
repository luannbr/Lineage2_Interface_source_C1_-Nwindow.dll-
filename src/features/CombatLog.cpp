/*=============================================================================
    CombatLog.cpp - phase 6: the combat log above the chat (include/features/CombatLog.h).

    Port of NWindow_C1 src/inject/combatlog/CombatLog.cpp (HEAD 4e56326; its docs
    COMBAT_LOG.md and SESSION_2026-07-16_COMBATLOG_UI.md): the same windows, layout, filter,
    texts, combatlog.ini keys (now section CombatLog of the settings: compiled-in default = every
    category, the player's choices in the registry, src/features/NwFeatureConfig.cpp) and "chatlog"
    log lines. NWindow_C1 reached the stock code by
    address; here those are our classes and fields:

      NWindow_C1                                   here
      "ChatBack" list, UClass 0x1012dcf8,          NCDebugWnd (the console's debug log list,
        ctor 0x10057ad0(0x96, 0x14)                  built the same way: new NCDebugWnd(150, 20))
      AddLine 0x10018310, refresh 0x10017bf0       NCListBox::AddString, UpdateScrollBar
      fill texture 0x10057740, list +0x1a0         NCDebugWnd::SetBackTexture, BackTex
      list +0x1a8..+0x1e4                          Items, scroll controls, VisibleLines, MaxLines,
                                                     TopIndex, NumLines, Selected, Unknown1D4
                                                     (row height), TextX, TextY, LineGap,
                                                     bAutoScroll
      SetRect 0x10023d90, +0x30..+0x58             NCWnd::SetWindowRect, Local*, Screen*,
                                                     Width, Height, Style, Entry
      vtable +0x60, +0xa8, +0xac                   WindowProc, ShowWindow, HideWindow
      Register 0x100257d0 on console +0x36bc       (root windows there; here children of the
                                                     desktop, see below)
      button ctor 0x10001a20, +0x1b8/+0x1bc        NCButton::NCButton, NormalTexH/NormalTexW
      console +0x36e0 "chat", +0x36d8 "bottom"     DebugWnd, MessageWnd
      console +0x36dc "UI host", +0x3718           Desktop, bSuspended
      root +0x3c/+0x40 (paint list), entry +4      WndMain->TopLevel, NWndEntry::Wnd
      UL2ConsoleWnd::GetState                      State (GetState returns it)
      the hook on 0x10037b30                       CombatLog_DivertChatMessage, called first by
                                                     NCChatWnd::AddChatMessage
      WindowFix_CursorClientPos / GameHasFocus /   the same functions of our WindowFix port
        CursorInClient                               (features/WindowFix.h)

    Beyond NWindow_C1 (the user's requests after the first in-game test, 27/09/2026): the list
    is one of the desktop's windows (NConsoleWnd::Desktop, the NCConsole that owns the chat),
    made the way NCConsole::OnCreate makes the chat, and the gear and the filter are the list's
    children, as the chat's option button and filter window are the chat's. So the windows the
    player opens cover it, and it hides with the interface (Alt+H) and with the desktop when the
    world goes (the loading screen of a restart, the lobby). NWindow_C1 registered all three as
    root windows, painted after the whole desktop and never hidden by it.

    What differs from NWindow_C1 and why: docs/notas/F6_CombatLog.md.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCVirtualWndMain.h"
#include "nwindow/NCDebugWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCButton.h"
#include "nwindow/NCSliderBar.h"
#include "features/CombatLog.h"
#include "features/WindowFix.h"

/*-----------------------------------------------------------------------------
    Constants.
-----------------------------------------------------------------------------*/

// NCConsole::AddSystemMessage's type (SAY2_System in NCChatWnd.cpp)
static const INT CHAT_System = 5;
// the system sub type of the petition chat, which NCChatWnd::AddChatMessage sends to the petition windows
static const INT CHAT_SubPetition = 6;

static const INT StableTicks = 60;      // in the world this many ticks before the list is made
static const INT Gap = 6;               // between the list and the top of the chat
static const INT OptBtnSize = 16;       // the gear when its texture has no usable size

// the list: painted, clicks go through (the chat's own frame and text lists are
// NWS_NOHITTEST too); its scroll controls and the gear take clicks
static const DWORD StyleList = NWS_VISIBLE | NWS_NOHITTEST;
static const DWORD StyleCfg = NWS_VISIBLE;                    // painted and clickable
static const INT CfgHeight = 132;                             // ten lines of 12 and a margin

// NWindow_C1 looked for these in order and kept the first found (FindTexture falls back to
// NWindow.BlackTexture, so it is the first one)
static const TCHAR* const FillTexNames[] = {
    TEXT("L2UI.etcwnd.NPCHtml_BACK"),
    TEXT("L2UI.etcWnd.npcHtml_back"),
    TEXT("L2UI.etcWnd.NPCHtml_BACK"),
    TEXT("L2UI.HTML.SquareGray"),
    TEXT("L2UI.HTML.SquareBlack"),
};
static const INT FillTexCount = ARRAY_COUNT(FillTexNames);

// the filter's categories
enum
{
    CAT_ExpSp = 1 << 0,
    CAT_Drop = 1 << 1,
    CAT_Damage = 1 << 2,
    CAT_DmgRecv = 1 << 3,
    CAT_Kill = 1 << 4,
    CAT_Other = 1 << 5,   // missed, skills, buffs, other combat lines
    CAT_All = CAT_ExpSp | CAT_Drop | CAT_Damage | CAT_DmgRecv | CAT_Kill | CAT_Other,
};

struct FCombatCategory
{
    DWORD Bit;
    const TCHAR* Label;
    const char* IniKey;   // combatlog.ini [filter] key, now setting CombatLog/<key>
    DWORD Color;          // of its lines in the list
};

static const FCombatCategory Cats[] = {
    {CAT_ExpSp, TEXT("Exp / SP"), "exp_sp", 0xFFFFFF00},
    {CAT_Drop, TEXT("Drop / Adena"), "drop", 0xFF80FF80},
    {CAT_Damage, TEXT("Damage (dealt)"), "damage", 0xFFFF8080},
    {CAT_DmgRecv, TEXT("Damage received"), "dmg_recv", 0xFFFFA040},
    {CAT_Kill, TEXT("Kill / Level"), "kill", 0xFF80C0FF},
    {CAT_Other, TEXT("Skills / Other"), "other", 0xFFC0C0C0},
};
static const INT CatCount = ARRAY_COUNT(Cats);

// the filter window's lines: title, one per category, all on, all off, hint
static const INT CfgLineCat0 = 1;
static const INT CfgLineAllOn = CfgLineCat0 + CatCount;
static const INT CfgLineAllOff = CfgLineAllOn + 1;

/*-----------------------------------------------------------------------------
    State (all on the game thread: the console tick and the chat).
-----------------------------------------------------------------------------*/

static NCDebugWnd* GPanel;          // the combat list
static NCDebugWnd* GCfg;            // the filter window under it
static NCButton* GOptBtn;           // the gear (the stock chat's option button textures)
static NConsoleWnd* GLastConsole;
static UTexture* GFillTex;          // the lists' background
static UBOOL GReady;                // the list was made (or is being made)
static INT GStable;                 // ticks in the world
static INT GLastState = -1;

static UBOOL GHasUserPos;           // dragged: stays there (not saved)
static INT GUserX;
static INT GUserY;
static UBOOL GDragging;
static INT GDragOX;
static INT GDragOY;

static UBOOL GCfgOpen;
static UBOOL GCfgClickArmed;        // the left button was already down last tick
static UBOOL GHotkeyArmed;          // Alt+L was already down last tick
static DWORD GFilter = CAT_All;     // every category shown until the player turns one off

static UBOOL GDivertOn;             // NWindow_C1's hook on 0x10037b30 was in: set when the first
                                    //   list is made, never cleared
static INT GDivertCount;
static INT GUiHidden;               // last state of the game's interface logged (Alt+H...)

// the last size of the "chat" (the console's debug window) seen open
static INT GChatX = 0;
static INT GChatW = 360;
static INT GChatH = 200;

/*-----------------------------------------------------------------------------
    Helpers.
-----------------------------------------------------------------------------*/

// Still a live object: not freed by a garbage collection (NWindow_C1 uobject_alive).
static UBOOL IsAlive(UObject* Obj)
{
    if (!Obj)
        return 0;
    __try
    {
        if (IsBadReadPtr(Obj, 8))
            return 0;
        void** Vtbl = *(void***)Obj;
        if (!Vtbl || IsBadReadPtr(Vtbl, sizeof(void*)))
            return 0;
        if (Obj->UObject::IsPendingKill())
            return 0;
        if (!Obj->IsValid())
            return 0;
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

static UBOOL IsShown(NCWnd* Wnd)
{
    return Wnd && (Wnd->Style & NWS_VISIBLE) != 0;
}

// The pointers dropped so that the next tick makes the windows again (filter and dragged
// position kept).
static void AbandonPanel(const char* Reason)
{
    nwf::Log("chatlog", "panel abandon (%s) — will recreate", Reason ? Reason : "?");
    GPanel = NULL;
    GCfg = NULL;
    GOptBtn = NULL;
    GFillTex = NULL;
    GCfgOpen = 0;
    GDragging = 0;
    GReady = 0;
    GStable = 0;
}

static void RerootWindows()
{
    if (IsAlive(GPanel))
        GPanel->AddToRoot();
    if (IsAlive(GOptBtn))
        GOptBtn->AddToRoot();
    else
        GOptBtn = NULL;
    if (IsAlive(GCfg))
        GCfg->AddToRoot();
    else
    {
        GCfg = NULL;
        GCfgOpen = 0;
    }
    if (IsAlive(GFillTex))
        GFillTex->AddToRoot();
    else
        GFillTex = NULL;
}

// Wnd to (X, Y) on the screen, W x H, with its subtree (NWindow_C1's set_rect: SetWindowRect,
// then every rectangle field again). SetWindowRect takes the place in the parent and offsets
// the children by the new place minus the old screen one, right only under a parent at (0, 0),
// and the gear and the filter are the list's children: here the subtree moves by the screen
// offset (OffsetScreen) and Local* is the place in the parent.
static void PlaceWnd(NCWnd* Wnd, INT X, INT Y, INT W, INT H)
{
    if (!Wnd || W < 16 || H < 16)
        return;
    Wnd->OffsetScreen(X - Wnd->ScreenX, Y - Wnd->ScreenY);
    NCWnd* Parent = (Wnd->Entry && Wnd->Entry->Parent) ? Wnd->Entry->Parent->Wnd : NULL;
    Wnd->LocalX = Parent ? X - Parent->ScreenX : X;
    Wnd->LocalY = Parent ? Y - Parent->ScreenY : Y;
    Wnd->Width = W;
    Wnd->Height = H;
    Wnd->LocalW = W;
    Wnd->LocalH = H;
}

// A scroll control of Parent at (X, Y) on the screen, W x H, written directly (NWindow_C1's
// write_xywh; Local* in the parent's space here).
static void WriteRect(NCWnd* Wnd, NCWnd* Parent, INT X, INT Y, INT W, INT H)
{
    if (!Wnd)
        return;
    Wnd->ScreenX = X;
    Wnd->ScreenY = Y;
    Wnd->Width = W;
    Wnd->Height = H;
    Wnd->LocalX = X - Parent->ScreenX;
    Wnd->LocalY = Y - Parent->ScreenY;
    Wnd->LocalW = W;
    Wnd->LocalH = H;
}

// The list paints VisibleLines rows from TopIndex: the constructor's 20 are a whole chat
// window, the combat list has room for 8 to 11. Made to fit the height; with bPinToBottom
// the newest lines are shown (after adding lines only, or the player could not scroll up).
static void SyncListMetrics(NCListBox* List, INT PanelH, UBOOL bPinToBottom)
{
    if (!List || PanelH < 24)
        return;
    INT LineH = List->Unknown1D4;
    if (LineH < 8 || LineH > 32)
        LineH = 12;
    INT Extra = List->LineGap;
    if (Extra < 0 || Extra > 16)
        Extra = 0;
    INT Row = LineH + Extra;
    if (Row < 8)
        Row = 12;
    INT TopPad = List->TextY;
    if (TopPad < 0 || TopPad > 16)
        TopPad = 3;

    // a couple of pixels left so the last row does not touch the bottom border
    INT Vis = (PanelH - TopPad - 4) / Row;
    if (Vis < 3)
        Vis = 3;
    if (Vis > 40)
        Vis = 40;
    List->VisibleLines = Vis;
    List->bAutoScroll = 1;

    INT Count = List->Items.Num();
    if (Count < 0)
        Count = 0;
    INT MaxTop = Count - Vis;
    if (MaxTop < 0)
        MaxTop = 0;
    if (bPinToBottom)
        List->TopIndex = MaxTop;
    else
    {
        INT Top = List->TopIndex;
        if (Top < 0)
            Top = 0;
        if (Top > MaxTop)
            Top = MaxTop;
        List->TopIndex = Top;
    }
    List->NumLines = Count;
    List->UpdateScrollBar();
}

// About how many characters fit a row (the C1 UI font is about 6 pixels wide).
static INT MaxCharsForPanel(INT PanelW, INT TextIndent)
{
    if (PanelW < 40)
        return 40;
    INT Usable = PanelW - TextIndent - 18;   // border and gear
    if (Usable < 24)
        Usable = 24;
    INT MaxChars = Usable / 6;
    if (MaxChars < 12)
        MaxChars = 12;
    if (MaxChars > 200)
        MaxChars = 200;
    return MaxChars;
}

// The next row of Src into Out (OutChars with the terminator), broken at the last space or
// punctuation of the row when there is one; returns the characters used (0: nothing left).
static INT TakeWrapChunk(const TCHAR* Src, TCHAR* Out, INT OutChars, INT MaxChars)
{
    if (!Src || !Src[0] || !Out || OutChars < 2 || MaxChars < 4)
        return 0;
    if (MaxChars >= OutChars - 1)
        MaxChars = OutChars - 2;

    INT Len = 0;
    while (Src[Len])
        ++Len;
    if (Len <= MaxChars)
    {
        INT N = Len;
        if (N > OutChars - 1)
            N = OutChars - 1;
        for (INT i = 0; i < N; ++i)
            Out[i] = Src[i];
        Out[N] = 0;
        return N;
    }

    INT Cut = MaxChars;
    for (INT i = MaxChars; i > MaxChars / 3; --i)
    {
        TCHAR C = Src[i];
        if (C == ' ' || C == '\t' || C == '-' || C == ':' || C == ',' || C == '.')
        {
            Cut = (C == ' ' || C == '\t') ? i : i + 1;
            if (Cut > MaxChars)
                Cut = MaxChars;
            break;
        }
    }
    if (Cut < 1)
        Cut = MaxChars;
    if (Cut > OutChars - 1)
        Cut = OutChars - 1;
    for (INT i = 0; i < Cut; ++i)
        Out[i] = Src[i];
    Out[Cut] = 0;

    // the spaces at the start of the rest are skipped
    INT Consumed = Cut;
    while (Src[Consumed] == ' ' || Src[Consumed] == '\t')
        ++Consumed;
    return Consumed;
}

/*-----------------------------------------------------------------------------
    Filter: the categories of a line (by its text) and the player's choices.
-----------------------------------------------------------------------------*/

static void LoadFilters()
{
    DWORD Filter = 0;
    UBOOL bAny = 0;
    for (INT i = 0; i < CatCount; ++i)
    {
        INT Value = nwf::IniInt("CombatLog", Cats[i].IniKey, -1);
        if (Value < 0)
            continue;
        bAny = 1;
        if (Value)
            Filter |= Cats[i].Bit;
    }
    if (bAny)
        GFilter = Filter;
    nwf::Log("chatlog", "filter=0x%X", GFilter);
}

static void SaveFilters()
{
    for (INT i = 0; i < CatCount; ++i)
        nwf::IniWriteInt("CombatLog", Cats[i].IniKey, (GFilter & Cats[i].Bit) ? 1 : 0);
}

// Needle in Hay, A-Z matched without case.
static UBOOL HasText(const TCHAR* Hay, const TCHAR* Needle)
{
    if (!Hay || !Needle || !Needle[0])
        return 0;
    for (const TCHAR* P = Hay; *P; ++P)
    {
        const TCHAR* A = P;
        const TCHAR* B = Needle;
        while (*A && *B)
        {
            TCHAR CA = *A;
            TCHAR CB = *B;
            if (CA >= 'A' && CA <= 'Z')
                CA = (TCHAR)(CA + 32);
            if (CB >= 'A' && CB <= 'Z')
                CB = (TCHAR)(CB + 32);
            if (CA != CB)
                break;
            ++A;
            ++B;
        }
        if (!*B)
            return 1;
    }
    return 0;
}

// "Earned Bone Gaiters." / "Earned Recipe: ..." (the C1 loot lines), not exp / SP.
static UBOOL IsEarnedItemLine(const TCHAR* Text)
{
    if (!Text || !Text[0])
        return 0;
    // not "recebeu": that one matches the damage received lines
    if (!HasText(Text, TEXT("earned")) && !HasText(Text, TEXT("ganhou")))
        return 0;
    if (HasText(Text, TEXT("experience")) || HasText(Text, TEXT("experienc")) ||
        HasText(Text, TEXT("experiencia")) || HasText(Text, TEXT("experiência")) ||
        HasText(Text, TEXT("skill point")) || HasText(Text, TEXT("skill points")) ||
        HasText(Text, TEXT("pontos de habilidade")) || HasText(Text, TEXT("ponto de habilidade")))
        return 0;
    // "earned 150 SP"
    if (HasText(Text, TEXT(" sp")) || HasText(Text, TEXT("sp.")) || HasText(Text, TEXT(" sp.")))
        return 0;
    return 1;
}

static DWORD ClassifyMessage(const TCHAR* Text)
{
    if (!Text || !Text[0])
        return 0;
    DWORD Cat = 0;

    // exp / SP ("you have earned ... experience / SP" stays here, not with the drops)
    if (HasText(Text, TEXT("experience")) || HasText(Text, TEXT("experienc")) ||
        HasText(Text, TEXT("experiencia")) || HasText(Text, TEXT("experiência")) ||
        HasText(Text, TEXT("skill point")) || HasText(Text, TEXT("skill points")) ||
        HasText(Text, TEXT("pontos de habilidade")) || HasText(Text, TEXT("ponto de habilidade")) ||
        // bare "exp" / "SP" words (not item names with "exp" in them)
        HasText(Text, TEXT(" exp")) || HasText(Text, TEXT("exp ")) || HasText(Text, TEXT("exp.")) ||
        HasText(Text, TEXT(" sp")) || HasText(Text, TEXT("sp.")) ||
        (HasText(Text, TEXT("you have earned")) &&
         (HasText(Text, TEXT("experience")) || HasText(Text, TEXT(" sp")) || HasText(Text, TEXT("sp")))))
        Cat |= CAT_ExpSp;

    // drops, adena, items ("Earned <item>." / "Earned Recipe: ..." in C1, not "obtained")
    if (HasText(Text, TEXT("obtained")) || HasText(Text, TEXT("picked up")) ||
        HasText(Text, TEXT("acquired")) || HasText(Text, TEXT("adena")) ||
        HasText(Text, TEXT("drop")) || HasText(Text, TEXT("loot")) ||
        HasText(Text, TEXT("adquiriu")) || HasText(Text, TEXT("pegou")) ||
        HasText(Text, TEXT("obteve")) || HasText(Text, TEXT("foi adicionado")) ||
        HasText(Text, TEXT("has been added")) || HasText(Text, TEXT("you have obtained")) ||
        HasText(Text, TEXT("you picked up")) || HasText(Text, TEXT("recipe")) ||
        HasText(Text, TEXT("recipe:")) || IsEarnedItemLine(Text))
        Cat |= CAT_Drop;

    // kills, levels
    if ((HasText(Text, TEXT("level")) && HasText(Text, TEXT("up"))) ||
        HasText(Text, TEXT("has been killed")) || HasText(Text, TEXT("you have defeated")) ||
        HasText(Text, TEXT("foi morto")) || HasText(Text, TEXT("derrotou")) ||
        HasText(Text, TEXT("level up")))
        Cat |= CAT_Kill;

    // damage received
    if (HasText(Text, TEXT("received")) || HasText(Text, TEXT("you receive")) ||
        HasText(Text, TEXT("recebeu dano")) || HasText(Text, TEXT("sofreu")) ||
        HasText(Text, TEXT("hit you")) || HasText(Text, TEXT("hits you")) ||
        HasText(Text, TEXT("damage to you")) || HasText(Text, TEXT("you have received")))
        Cat |= CAT_DmgRecv;

    // damage dealt
    if (!(Cat & CAT_DmgRecv))
    {
        if (HasText(Text, TEXT("damage")) || HasText(Text, TEXT("critical")) ||
            HasText(Text, TEXT("hit for")) || HasText(Text, TEXT("hits for")) ||
            HasText(Text, TEXT("you did")) || HasText(Text, TEXT("you hit")) ||
            HasText(Text, TEXT("over-hit")) || HasText(Text, TEXT("over hit")) ||
            HasText(Text, TEXT("dano")) || HasText(Text, TEXT("critico")) ||
            HasText(Text, TEXT("crítico")) || HasText(Text, TEXT("atingiu")) ||
            HasText(Text, TEXT("causou")))
            Cat |= CAT_Damage;
    }

    // skills and other combat lines (missed, "Use Mortal Blow", spirits, reuse...)
    if (HasText(Text, TEXT("missed")) || HasText(Text, TEXT("miss ")) ||
        HasText(Text, TEXT("use ")) || HasText(Text, TEXT("not available")) ||
        HasText(Text, TEXT("prepared for reuse")) || HasText(Text, TEXT("spirits")) ||
        HasText(Text, TEXT("enabled")) || HasText(Text, TEXT("disabled")) ||
        HasText(Text, TEXT("resist")) || HasText(Text, TEXT("blocked")) ||
        HasText(Text, TEXT("evaded")) || HasText(Text, TEXT("perfect")) ||
        HasText(Text, TEXT("soulshot")) || HasText(Text, TEXT("spiritshot")) ||
        HasText(Text, TEXT("mp ")) || HasText(Text, TEXT(" hp")))
        Cat |= CAT_Other;

    return Cat;
}

static UBOOL ShouldShowCombat(const TCHAR* Text)
{
    DWORD Cat = ClassifyMessage(Text);
    if (Cat == 0)
        return 1;   // a system line of no category is shown
    return (Cat & GFilter) != 0;
}

static DWORD ColorForMessage(const TCHAR* Text)
{
    DWORD Cat = ClassifyMessage(Text);
    for (INT i = 0; i < CatCount; ++i)
        if (Cat & Cats[i].Bit)
            return Cats[i].Color;
    return 0xFFC0C0C0;
}

// Text into the list, wrapped over as many rows as it needs, the newest row shown.
static void PanelAddLine(NCDebugWnd* List, const TCHAR* Text, DWORD Color)
{
    if (!List || !Text)
        return;
    __try
    {
        INT PanelW = List->Width;
        INT Indent = List->TextX;
        if (Indent < 0 || Indent > 64)
            Indent = 0x1E;
        const INT MaxChars = MaxCharsForPanel(PanelW, Indent);
        const TCHAR* P = Text;
        INT Rows = 0;
        while (P && P[0] && Rows < 16)
        {
            TCHAR Chunk[256];
            Chunk[0] = 0;
            INT Consumed = TakeWrapChunk(P, Chunk, 256, MaxChars);
            if (Consumed <= 0)
                break;
            List->AddString(Chunk, Color, -1);
            P += Consumed;
            ++Rows;
        }
        SyncListMetrics(List, List->Height, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

/*-----------------------------------------------------------------------------
    The windows: list, gear, filter; layout and mouse.
-----------------------------------------------------------------------------*/

// The chat's size while it is open (the console's debug window, NWindow_C1's "chat").
static void RememberOpenChat(NCWnd* Chat)
{
    if (!Chat)
        return;
    if ((Chat->Style & NWS_VISIBLE) && Chat->Width >= 180 && Chat->Width <= 600 &&
        Chat->Height >= 100 && Chat->Height <= 350)
    {
        GChatX = Chat->ScreenX;
        GChatW = Chat->Width;
        GChatH = Chat->Height;
    }
}

// The list's scroll controls along its left edge (NCListBox::OnCreate puts them on the right).
// Their visibility is the list's (the desktop shows and hides the whole tree).
static void PlaceScrollLeft(NCListBox* List, INT PanelX, INT PanelY, INT PanelH)
{
    if (!List || PanelH < 32)
        return;
    NCWnd* Up = List->ScrollUpButton;
    NCWnd* Down = List->ScrollDownButton;
    NCWnd* Bar = List->ScrollBar;
    const INT BarH = PanelH - 0x18;
    if (BarH < 4)
        return;
    if (Up)
        WriteRect(Up, List, PanelX, PanelY, 12, 12);
    if (Down)
        WriteRect(Down, List, PanelX, PanelY + PanelH - 12, 12, 12);
    if (Bar)
        WriteRect(Bar, List, PanelX, PanelY + 12, 12, BarH);
}

static void HideScrollKids(NCListBox* List)
{
    if (!List)
        return;
    NCWnd* Kids[3] = {List->ScrollUpButton, List->ScrollDownButton, List->ScrollBar};
    for (INT i = 0; i < 3; ++i)
    {
        if (!Kids[i])
            continue;
        Kids[i]->Style &= ~NWS_VISIBLE;
        Kids[i]->HideWindow();
    }
}

// The background of the lists, kept through the garbage collection.
static UBOOL ApplyFillTexture(NCDebugWnd* List)
{
    if (!List)
        return 0;
    UTexture* Tex = NULL;
    for (INT i = 0; i < FillTexCount; i++)
    {
        List->SetBackTexture(FillTexNames[i]);
        Tex = List->BackTex;
        if (Tex)
            break;
    }
    if (Tex)
    {
        List->BackTex = Tex;
        GFillTex = Tex;
        Tex->AddToRoot();
    }
    nwf::Log("chatlog", "fill tex=%p", Tex);
    return Tex != NULL;
}

// Above the chat, 50 pixels narrower, 55 % of its height (110 to 150), or where it was dragged.
static void ComputeRect(NConsoleWnd* Console, INT* X, INT* Y, INT* W, INT* H)
{
    NCWnd* Chat = (NCWnd*)Console->DebugWnd;
    NCWnd* Bottom = (NCWnd*)Console->MessageWnd;
    RememberOpenChat(Chat);

    INT CW = GChatW;
    INT CX = GChatX;
    INT CH = GChatH;
    INT ChatTop = 0;
    if (Chat)
    {
        if ((Chat->Style & NWS_VISIBLE) && Chat->Width >= 180 && Chat->Width <= 600 &&
            Chat->Height >= 100 && Chat->Height <= 350)
        {
            CW = Chat->Width;
            CH = Chat->Height;
            CX = Chat->ScreenX;
            if (Chat->ScreenY > 40)
                ChatTop = Chat->ScreenY;
        }
    }
    if (ChatTop < 40 && Bottom)
    {
        INT BY = Bottom->ScreenY;
        if (BY > 200 && BY < 2000)
            ChatTop = BY - CH;
    }
    if (ChatTop < 80)
        ChatTop = 470;

    *W = (CW > 0 ? CW : 360) - 50;
    if (*W < 200)
        *W = 200;
    *H = (CH * 55) / 100;
    if (*H < 110)
        *H = 110;
    if (*H > 150)
        *H = 150;
    if (*H > CH - 40 && CH > 140)
        *H = CH - 40;

    if (GHasUserPos)
    {
        *X = GUserX;
        *Y = GUserY;
    }
    else
    {
        *X = CX >= 0 ? CX : 0;
        *Y = ChatTop - *H - Gap;
        if (*Y < 8)
            *Y = 8;
    }
}

static void RebuildCfgLines()
{
    if (!GCfg)
        return;
    GCfg->Clear();
    PanelAddLine(GCfg, TEXT("  Combat filter"), 0xFFE8E8C0);
    for (INT i = 0; i < CatCount; ++i)
    {
        TCHAR Line[96];
        const UBOOL bOn = (GFilter & Cats[i].Bit) != 0;
        appSprintf(Line, TEXT("  %s  %s"), bOn ? TEXT("[x]") : TEXT("[ ]"), Cats[i].Label);
        PanelAddLine(GCfg, Line, bOn ? Cats[i].Color : 0xFF606060);
    }
    PanelAddLine(GCfg, TEXT("  [ All on ]"), 0xFF90C090);
    PanelAddLine(GCfg, TEXT("  [ All off ]"), 0xFFC09090);
    PanelAddLine(GCfg, TEXT("  click gear / Alt+L close"), 0xFF707070);
    GCfg->Selected = -1;
    GCfg->TopIndex = 0;
}

// Closed, the filter is out of the list's child list (it stays registered with the window
// manager): the desktop's NCWnd::ShowWindow, which shows every child again (entering the
// world, Alt+H), and the painting walk the child lists, so a closed filter never comes back
// with them. Opened, it goes back at the end of the list (ToggleCfg).
static void HideCfg()
{
    if (GCfg)
    {
        GCfg->HideWindow();
        if (GPanel && GCfg->Entry)
            GPanel->RemoveChildNode(GCfg->Entry);   // nothing when already out
    }
    GCfgOpen = 0;
}

// The combat log in front of the desktop's other windows: what NCWnd::SetFocus does for a
// clicked window (BringChildToTop up the parents). A click on the scroll controls, the gear or
// the filter gets there through the stock click routing; this is for Alt+L and the drag.
static void BringToFront(NConsoleWnd* Console)
{
    NCWnd* Desktop = Console->Desktop;
    if (GPanel && GPanel->Entry && Desktop && Desktop->ChildHead)
        Desktop->BringChildToTop(GPanel->Entry);
}

// Something is drawn at the point: Wnd, or one of the windows under it (a window paints when
// its own NWS_VISIBLE is set: NCVirtualWndMain::PaintChildren walks every child).
static UBOOL CoversPoint(NCWnd* Wnd, INT X, INT Y)
{
    if (Wnd->IsInWindow(X, Y))
        return 1;
    for (NWndListNode* Node = Wnd->ChildHead; Node; Node = Node->Next)
        if (CoversPoint(Node->Entry->Wnd, X, Y))
            return 1;
    return 0;
}

// The combat log is what is on screen at (X, Y): no shown top window registered after the
// desktop (painted after it) and no desktop window in front of the list covers the point.
static UBOOL IsFrontAt(NConsoleWnd* Console, INT X, INT Y)
{
    NCWnd* Desktop = Console->Desktop;
    NCVirtualWndMain* Root = Console->WndMain;
    if (!Desktop || !Root || !GPanel)
        return 0;
    UBOOL bAfterDesktop = 0;
    for (INT i = 0; i < Root->TopLevel.Num(); i++)
    {
        NCWnd* Top = Root->TopLevel(i)->Wnd;
        if (Top == Desktop)
            bAfterDesktop = 1;
        else if (bAfterDesktop && (Top->Style & NWS_VISIBLE) && CoversPoint(Top, X, Y))
            return 0;
    }
    for (NWndListNode* Node = Desktop->ChildHead; Node; Node = Node->Next)
    {
        NCWnd* Wnd = Node->Entry->Wnd;
        if (Wnd == GPanel)
            return 1;
        if (CoversPoint(Wnd, X, Y))
            return 0;
    }
    return 0;
}

// The filter under the list, as wide, without scroll controls.
static void PlaceCfg(INT MainX, INT MainY, INT MainW, INT MainH)
{
    if (!GCfg)
        return;
    const INT CW = MainW;
    const INT CH = CfgHeight;
    INT CX = MainX;
    INT CY = MainY + MainH + 3;
    if (CX < 4)
        CX = 4;
    if (CY < 4)
        CY = 4;
    PlaceWnd(GCfg, CX, CY, CW, CH);
    GCfg->TextX = 8;
    GCfg->TextY = 3;
    GCfg->LineGap = 0;
    GCfg->bAutoScroll = 0;
    GCfg->Selected = -1;
    if (GFillTex)
        GCfg->BackTex = GFillTex;
}

// The gear at the top right corner of the list.
static void PlaceOptBtn(INT MainX, INT MainY, INT MainW)
{
    if (!GOptBtn)
        return;
    INT BW = OptBtnSize;
    INT BH = OptBtnSize;
    // NWindow_C1 took +0x1B8 (NormalTexH) as the width and +0x1BC (NormalTexW) as the height
    INT TW = GOptBtn->NormalTexH;
    INT TH = GOptBtn->NormalTexW;
    if (TW >= 8 && TW <= 32 && TH >= 8 && TH <= 32)
    {
        BW = TW;
        BH = TH;
    }
    const INT BX = MainX + MainW - BW - 14;
    const INT BY = MainY + 3;
    PlaceWnd(GOptBtn, BX, BY, BW, BH);
}

// The stock chat's option button, a child of the list as the chat's is of the chat
// (NCChatWnd::OnCreate); PlaceOptBtn moves it to its corner.
static UBOOL CreateOptBtn()
{
    if (GOptBtn)
        return 1;
    if (!GPanel)
        return 0;
    NCButton* Btn = new NCButton(TEXT("L2UI.ChatWnd.OptionBtn"), TEXT("L2UI.ChatWnd.OptionBtn_click"), 0);
    Btn->AddToRoot();
    Btn->SetWindowRect(GPanel->Width - OptBtnSize - 14, 3, OptBtnSize, OptBtnSize);
    GPanel->CreateChild(Btn, NWS_VISIBLE);
    GOptBtn = Btn;
    nwf::Log("chatlog", "OptionBtn OK");
    return 1;
}

// The filter: a list like the combat log, a child of it placed under it, as the chat's filter
// window is the chat's child (placed above the chat). Made hidden.
static UBOOL CreateCfgPanel()
{
    if (GCfg)
        return 1;
    if (!GPanel)
        return 0;
    NCDebugWnd* List = new NCDebugWnd(0x20, 0x14);
    List->AddToRoot();
    // placed before its creation, as the stock code does (its scroll controls take the size)
    List->SetWindowRect(0, GPanel->Height + 3, GPanel->Width, CfgHeight);
    GPanel->CreateChild(List, StyleCfg);
    GCfg = List;
    if (GFillTex)
        List->BackTex = GFillTex;
    else
        ApplyFillTexture(List);
    RebuildCfgLines();
    HideCfg();   // made closed: hidden and out of the list's children
    nwf::Log("chatlog", "config panel ok");
    return 1;
}

static void ToggleCfg(NConsoleWnd* Console)
{
    if (!GCfgOpen)
    {
        if (!CreateCfgPanel())
            return;
        RebuildCfgLines();   // the marks of the current filter
        GCfgOpen = 1;
        GPanel->AddChildNode(GCfg->Entry);   // back among the list's children (HideCfg)
        GCfg->ShowWindow();
        HideScrollKids(GCfg);   // ShowWindow shows the children too
        // over the chat, which the filter overlaps
        BringToFront(Console);
        nwf::Log("chatlog", "config OPEN");
    }
    else
    {
        HideCfg();
        nwf::Log("chatlog", "config CLOSE");
    }
}

// A click on a filter line (the list selects the line under a click) toggles it.
static void HandleCfgClicks()
{
    if (!GCfg || !GCfgOpen)
        return;
    INT Sel = GCfg->Selected;
    if (Sel < 0)
        return;
    UBOOL bChanged = 0;
    if (Sel >= CfgLineCat0 && Sel < CfgLineCat0 + CatCount)
    {
        GFilter ^= Cats[Sel - CfgLineCat0].Bit;
        bChanged = 1;
    }
    else if (Sel == CfgLineAllOn)
    {
        GFilter = CAT_All;
        bChanged = 1;
    }
    else if (Sel == CfgLineAllOff)
    {
        GFilter = 0;
        bChanged = 1;
    }
    GCfg->Selected = -1;
    if (bChanged)
    {
        SaveFilters();
        RebuildCfgLines();
        nwf::Log("chatlog", "filter now 0x%X", GFilter);
    }
}

// Alt+L, the gear and the drag, read from the keyboard and mouse state: the list itself takes
// no clicks (NWS_NOHITTEST). The list's rectangle is in the game window's client space. A window
// drawn over the list takes its clicks: the gear and the drag act only where the combat log is
// the window on screen (IsFrontAt).
static void HandleUserDragAndOption(NConsoleWnd* Console)
{
    if (!GPanel)
        return;
    POINT Pt = {0, 0};
    if (!nwf::WindowFix_CursorClientPos(&Pt))
        GetCursorPos(&Pt);   // no viewport known (or WindowFix off): the stock UI's own space
    const UBOOL bFocused = nwf::WindowFix_GameHasFocus();
    const UBOOL bLmb = bFocused && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    // Alt+L, never F1-F12
    const UBOOL bAlt = bFocused && (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const UBOOL bKeyL = (GetAsyncKeyState('L') & 0x8000) != 0;
    if (bAlt && bKeyL)
    {
        if (!GHotkeyArmed)
        {
            GHotkeyArmed = 1;
            ToggleCfg(Console);
        }
    }
    else
        GHotkeyArmed = 0;

    const INT PX = GPanel->ScreenX;
    const INT PY = GPanel->ScreenY;
    const INT PW = GPanel->Width;
    const INT PH = GPanel->Height;

    UBOOL bOverBtn = 0;
    if (GOptBtn)
    {
        const INT BX = GOptBtn->ScreenX;
        const INT BY = GOptBtn->ScreenY;
        bOverBtn = Pt.x >= BX && Pt.x < BX + GOptBtn->Width && Pt.y >= BY &&
                   Pt.y < BY + GOptBtn->Height;
    }

    if (bLmb)
    {
        // a press on the title bar or on another window does not take the list
        const UBOOL bStartOk = nwf::WindowFix_CursorInClient();
        if (!GCfgClickArmed)
        {
            GCfgClickArmed = 1;
            if (bOverBtn && !GDragging && bStartOk && IsFrontAt(Console, Pt.x, Pt.y))
            {
                ToggleCfg(Console);
                return;
            }
        }
        if (!GDragging)
        {
            // the body, not the scroll controls on the left or the gear
            if (bStartOk && !bOverBtn && Pt.x >= PX + 14 && Pt.x < PX + PW && Pt.y >= PY &&
                Pt.y < PY + PH && IsFrontAt(Console, Pt.x, Pt.y))
            {
                GDragging = 1;
                GDragOX = Pt.x - PX;
                GDragOY = Pt.y - PY;
                // a dragged window comes to the front, as the stock title bar drag does
                BringToFront(Console);
            }
        }
        else
        {
            GUserX = Pt.x - GDragOX;
            GUserY = Pt.y - GDragOY;
            if (GUserX < 0)
                GUserX = 0;
            if (GUserY < 0)
                GUserY = 0;
            GHasUserPos = 1;
            PlaceWnd(GPanel, GUserX, GUserY, PW, PH);
            PlaceScrollLeft(GPanel, GUserX, GUserY, PH);
        }
    }
    else
    {
        GDragging = 0;
        GCfgClickArmed = 0;
    }
}

static void ApplyLayout(NConsoleWnd* Console)
{
    if (!GPanel || !Console)
        return;
    __try
    {
        HandleUserDragAndOption(Console);
        HandleCfgClicks();

        INT X, Y, W, H;
        ComputeRect(Console, &X, &Y, &W, &H);
        if (!GDragging)
        {
            if (GPanel->ScreenX != X || GPanel->ScreenY != Y || GPanel->Width != W ||
                GPanel->Height != H)
                PlaceWnd(GPanel, X, Y, W, H);
            PlaceScrollLeft(GPanel, X, Y, H);
        }
        else
        {
            X = GUserX;
            Y = GUserY;
            PlaceScrollLeft(GPanel, X, Y, H);
        }
        GPanel->TextX = 0x1E;
        GPanel->TextY = 3;
        GPanel->LineGap = 0;
        GPanel->bAutoScroll = 1;
        GPanel->Selected = -1;
        // the visible rows must fit the height (SyncListMetrics)
        SyncListMetrics(GPanel, H, 0);
        if (GFillTex)
            GPanel->BackTex = GFillTex;

        if (GOptBtn)
            PlaceOptBtn(X, Y, W);
        if (GCfg && GCfgOpen)
        {
            PlaceCfg(X, Y, W, H);
            if (!IsShown(GCfg))
                GCfg->ShowWindow();
            HideScrollKids(GCfg);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        nwf::Log("chatlog", "apply_layout EXCEPTION");
    }
}

// The list: a debug log list like the console's own, one of the desktop's windows as the chat
// is (NCConsole::OnCreate: SetWindowRect in the parent's space, then CreateChild with
// NWS_VISIBLE | NWS_NOHITTEST). CreateChild puts it at the end of the desktop's list, under the
// windows there: they cover it until a click on its controls, a drag or the filter brings it
// forward (BringChildToTop), as a click on the chat's edit box or tabs does for the chat.
static UBOOL CreatePanel(NConsoleWnd* Console)
{
    NCWnd* Desktop = Console->Desktop;
    if (!Desktop || !Desktop->Entry)
    {
        nwf::Log("chatlog", "no desktop");
        return 0;
    }
    NCDebugWnd* List = new NCDebugWnd(0x96, 0x14);
    // not freed by a garbage collection while it is in the window lists
    List->AddToRoot();
    nwf::Log("chatlog", "AddToRoot ok");

    INT X, Y, W, H;
    ComputeRect(Console, &X, &Y, &W, &H);
    List->SetWindowRect(X - Desktop->ScreenX, Y - Desktop->ScreenY, W, H);
    Desktop->CreateChild(List, StyleList);   // WM_CREATE: the scroll controls
    List->LineGap = 0;
    List->TextX = 0x1E;
    List->TextY = 3;
    List->bAutoScroll = 1;
    List->Selected = -1;

    PlaceScrollLeft(List, X, Y, H);
    // the constructor's 20 visible rows, made to fit before the first line comes
    SyncListMetrics(List, H, 1);

    GPanel = List;
    ApplyFillTexture(List);
    if (GFillTex)
        List->BackTex = GFillTex;

    // from now on NCChatWnd::AddChatMessage hands its system and combat lines over
    GDivertOn = 1;

    CreateOptBtn();
    PlaceOptBtn(X, Y, W);

    nwf::Log("chatlog", "panel=%p wh=%d,%d filter=0x%X (37b30 divert)", List, W, H, GFilter);
    return 1;
}

// The game's interface is not on screen: Alt+H (NCConsole::ToggleInterface), the loading
// screen of a restart (NConsoleWnd::RestartResetWindows), the lobby... The desktop's HideWindow
// hid the combat log's windows with its others, and a hidden top window's tree is not painted.
static UBOOL IsInterfaceHidden(NConsoleWnd* Console)
{
    return Console->bSuspended || !Console->Desktop || !IsShown(Console->Desktop);
}

// Every tick with the interface shown: layout, mouse, keyboard. Showing and hiding the
// windows is the desktop's (NWindow_C1 showed its root windows again every tick).
static void UpdatePanel(NConsoleWnd* Console)
{
    if (!GPanel)
        return;
    if (!IsAlive(GPanel))
    {
        AbandonPanel("dead in show_panel");
        return;
    }
    ApplyLayout(Console);
    // a FLUSH can clear the texture
    UTexture* Tex = GPanel->BackTex;
    if (!Tex || !IsAlive(Tex))
    {
        ApplyFillTexture(GPanel);
        if (GFillTex)
            GPanel->BackTex = GFillTex;
    }
    if (!GOptBtn || !IsAlive(GOptBtn))
    {
        // made again when the garbage collection took it
        GOptBtn = NULL;
        CreateOptBtn();
    }
}

static void TrySpawn(NConsoleWnd* Console)
{
    if (Console != GLastConsole)
    {
        nwf::Log("chatlog", "console %p -> %p", GLastConsole, Console);
        GLastConsole = Console;
        GPanel = NULL;
        GCfg = NULL;
        GOptBtn = NULL;
        GFillTex = NULL;
        GCfgOpen = 0;
        GHotkeyArmed = 0;
        GHasUserPos = 0;
        GDragging = 0;
        GDivertCount = 0;
        GReady = 0;
        GStable = 0;
        GLastState = -1;
    }

    const INT State = Console->State;
    if (State != GLastState)
    {
        nwf::Log("chatlog", "state %d -> %d", GLastState, State);
        const INT Prev = GLastState;
        GLastState = State;
        // leaving the world (relog): the lines do not follow into the next character (the
        // windows went with the desktop already)
        if (Prev == L2CS_InGame && State != L2CS_InGame)
        {
            if (IsAlive(GPanel))
                GPanel->Clear();
            GDivertCount = 0;
            HideCfg();
            GStable = 0;
            nwf::Log("chatlog", "cleared (left world / relog)");
        }
        else if (State == L2CS_InGame && Prev != L2CS_InGame && Prev >= 0)
        {
            // entering the world after the character selection: an empty list
            if (IsAlive(GPanel))
                GPanel->Clear();
            GDivertCount = 0;
            GStable = 0;
            nwf::Log("chatlog", "cleared (enter world)");
        }
        else if (State != L2CS_InGame)
        {
            HideCfg();
            GStable = 0;
        }
    }

    if (State != L2CS_InGame)
    {
        GStable = 0;
        return;
    }
    if (!Console->DebugWnd)
        return;
    // collected while in use (garbage collection, a bad FLUSH): made again
    if (GReady && !IsAlive(GPanel))
        AbandonPanel("dead in TrySpawn");

    ++GStable;
    if (GStable < StableTicks)
        return;

    // the interface hidden (Alt+H, a restart's loading screen): the windows are hidden with
    // the desktop; the filter closes, as NWindow_C1's did
    const INT Hidden = IsInterfaceHidden(Console) ? 1 : 0;
    if (Hidden != GUiHidden)
    {
        GUiHidden = Hidden;
        nwf::Log("chatlog", "stock UI %s (ALT+H sync)", Hidden ? "HIDDEN" : "shown");
    }
    if (Hidden)
    {
        if (GReady)
            HideCfg();
        return;
    }

    if (!GReady)
    {
        GReady = 1;
        LoadFilters();
        nwf::Log("chatlog", "create (divert system->combat, chat stays bottom)");
        if (!CreatePanel(Console))
        {
            GReady = 0;
            GStable = StableTicks / 2;
            nwf::Log("chatlog", "create FAILED");
            return;
        }
        nwf::Log("chatlog", "create OK");
    }
    else
        UpdatePanel(Console);
}

/*-----------------------------------------------------------------------------
    Entry points (include/features/CombatLog.h).
-----------------------------------------------------------------------------*/

namespace nwf {

void CombatLog_TrySpawn(NConsoleWnd* Console)
{
    if (!Console)
        return;
    __try
    {
        TrySpawn(Console);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

UBOOL CombatLog_DivertChatMessage(const TCHAR* Text, DWORD Color, INT Type, INT SubType)
{
    if (!GDivertOn)
        return 0;
    __try
    {
        // Only the system lines: type 5, which NCConsole::AddSystemMessage / AddSystemText give to
        // every systemmsg line (damage, exp, drops, "Missed target"...). What a player types comes
        // from NCConsole::Say with its Say2 type (0 normal, 1 shout, 2 tell, 3 party, 4 clan, 8
        // trade, 9 ally) and stays in the chat whatever its words. NWindow_C1 also took any line
        // that read like a combat one ("use ", "drop", "damage"...): "mouse bugs" or "the cause
        // of" left the chat for the combat log, and vanished with that category filtered out. The
        // petition chat (sub type 6) goes on to the petition windows.
        if (Type == CHAT_System && SubType != CHAT_SubPetition && Text && Text[0])
        {
            if (GReady && GPanel && ShouldShowCombat(Text))
            {
                PanelAddLine(GPanel, Text, ColorForMessage(Text));
                ++GDivertCount;
                if (GDivertCount <= 40 && nwf::LogEnabled())
                {
                    char Ansi[100];
                    Ansi[0] = 0;
                    WideCharToMultiByte(CP_ACP, 0, Text, -1, Ansi, 94, NULL, NULL);
                    Ansi[94] = 0;
                    nwf::Log("chatlog", "DIVERT t=%d #%d \"%s\"", Type, GDivertCount, Ansi);
                }
            }
            // taken: not in the chat, whether shown or filtered out
            return 1;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return 0;
}

void CombatLog_OnAfterCacheClean()
{
    if (!GReady || !GPanel)
        return;   // TrySpawn makes it when it can
    __try
    {
        if (!IsAlive(GPanel))
        {
            AbandonPanel("dead after cache clean");
            return;
        }
        RerootWindows();

        // FLUSH often drops the render cache and the material links
        ApplyFillTexture(GPanel);
        if (GFillTex)
        {
            GPanel->BackTex = GFillTex;
            GFillTex->AddToRoot();
        }

        // NWindow_C1 showed its root windows again here (unless Alt+H was on); the desktop's
        // windows keep the desktop's visibility, so only the lost pointers go
        if (!GOptBtn || !IsAlive(GOptBtn))
            GOptBtn = NULL;   // the next tick makes it again
        if (GCfg && IsAlive(GCfg))
        {
            if (GFillTex)
                GCfg->BackTex = GFillTex;
        }
        else
        {
            GCfg = NULL;
            GCfgOpen = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

}  // namespace nwf

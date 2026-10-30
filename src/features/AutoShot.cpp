/*=============================================================================
    AutoShot.cpp - phase 6: automatic soulshot / spiritshot on the shortcut bar, and moving a
    shortcut onto another slot (include/features/AutoShot.h).

    C1 has none of Interlude's plumbing (no RequestAutoSoulShot, no ExAutoSoulShot), but it has
    the two pieces that matter: IsShotItem (0x10047120), the same id list C3 uses, and the
    shortcut bar's ToggleTex, the animated mark C3 puts over an automatic shot. Its right button
    is free (NCWnd's no-op). The state lives on the server, as in C3: nothing is toggled here
    before the server says so, and a shot the server turns off (none left) shows off.

    NWindow_C1 src/inject/AutoShot.cpp cloned the bar's vtable at run time (paint, right button
    down, left button up over the stock slots); its craft/CraftUi.cpp sent "ss_ui 1" from an
    Engine.dll hook on OnUserInfo and took the data messages from one on OnNpcHtmlMessage. Here:
    calls in NCShortCutWnd's own OnPaint / OnLButtonUp, an OnRButtonDown override, and
    NConsoleWnd::Tick / NpcHtmlMessage. Protocol (AUTO_SHOT.md, part F): "ss_ui 1" on entering
    the world, "ss_set <item> <0|1>" on a right click, back "L2W shot 1 state\non=<ids>\nmsg=..".
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCShortCutWnd.h"
#include "features/AutoShot.h"
#include "features/ManufactureCmd.h"

namespace nwf {

/*-----------------------------------------------------------------------------
    State.
-----------------------------------------------------------------------------*/

enum { AUTOSHOT_MaxOn = 32, AUTOSHOT_MsgLen = 192 };

static INT GAutoShot = -1;               // [Shots] AutoShot
static INT GShotsOn[AUTOSHOT_MaxOn];     // item ids with automatic use on (the server's word)
static INT GNumShotsOn = 0;
static TCHAR GShotMsg[AUTOSHOT_MsgLen];  // the server's "msg=" line, shown by the next tick
static UBOOL bShotMsg = 0;
// the server id of the player "ss_ui 1" was sent for. Not the User pointer: GetMyUser() returns
// the record in the game data's user table, which moves when other users come into view, and
// every new pointer re-sent "ss_ui 1" - which makes the server reset the automatic shots
static INT GAnnouncedID = 0;

UBOOL AutoShot_Enabled()
{
    if (GAutoShot < 0)
    {
        GAutoShot = IniInt("Shots", "AutoShot", 1) != 0;
        Log("autoshot", "%s", GAutoShot ? "enabled" : "disabled");
    }
    return GAutoShot;
}

static UBOOL IsShotOn(INT ItemID)
{
    for (INT i = 0; i < GNumShotsOn; i++)
        if (GShotsOn[i] == ItemID)
            return 1;
    return 0;
}

static UBOOL IsShot(INT ItemID)
{
    return ItemID > 0 && GConsole->IsShotItem(ItemID);
}

// a slot of the bar (absolute: page * Cols + index), NULL out of range
static NShortCutItem* SlotItem(NCShortCutWnd* Wnd, INT Slot)
{
    if (!Wnd->Items || Slot < 0 || Slot >= Wnd->Cols * Wnd->Pages)
        return NULL;
    return &Wnd->Items[Slot];
}

/*-----------------------------------------------------------------------------
    The server's side: announce, data messages.
-----------------------------------------------------------------------------*/

// "L2W shot 1 state\non=1463,2509\nmsg=..." - on= replaces the whole list (no line: none on)
static void OnShotData(const TCHAR* Text)
{
    INT On[AUTOSHOT_MaxOn];
    INT Num = 0;
    const TCHAR* P = Text;
    while (*P)
    {
        const TCHAR* Line = P;
        while (*P && *P != '\n')
            P++;
        INT Len = (INT)(P - Line);
        if (Len && Line[Len - 1] == '\r')
            Len--;
        if (Len > 3 && Line[0] == 'o' && Line[1] == 'n' && Line[2] == '=')
        {
            for (INT i = 3; i < Len && Num < AUTOSHOT_MaxOn;)
            {
                DWORD Value = 0;
                UBOOL bDigits = 0;
                for (; i < Len && Line[i] >= '0' && Line[i] <= '9'; i++)
                {
                    Value = Value * 10 + (Line[i] - '0');
                    bDigits = 1;
                }
                if (bDigits)
                    On[Num++] = (INT)Value;
                while (i < Len && Line[i] != ',')
                    i++;
                if (i < Len)
                    i++;
            }
        }
        else if (Len > 4 && Line[0] == 'm' && Line[1] == 's' && Line[2] == 'g' && Line[3] == '=')
        {
            INT Count = Len - 4 < AUTOSHOT_MsgLen - 1 ? Len - 4 : AUTOSHOT_MsgLen - 1;
            for (INT i = 0; i < Count; i++)
                GShotMsg[i] = Line[4 + i];
            GShotMsg[Count] = 0;
            bShotMsg = 1;
        }
        if (*P)
            P++;
    }
    for (INT i = 0; i < Num; i++)
        GShotsOn[i] = On[i];
    GNumShotsOn = Num;
    Log("autoshot", "state from server: %d shot(s) on (first=%d)", Num, Num ? On[0] : 0);
}

// Text starts with Prefix; bIgnoreCase: ASCII letters without case (the markers are ASCII)
static UBOOL HasPrefix(const TCHAR* Text, const TCHAR* Prefix, UBOOL bIgnoreCase)
{
    for (; *Prefix; Text++, Prefix++)
    {
        TCHAR A = *Text;
        TCHAR B = *Prefix;
        if (bIgnoreCase)
        {
            if (A >= 'A' && A <= 'Z')
                A = (TCHAR)(A - 'A' + 'a');
            if (B >= 'A' && B <= 'Z')
                B = (TCHAR)(B - 'A' + 'a');
        }
        if (A != B)
            return 0;  // the text's end included
    }
    return 1;
}

UBOOL AutoShot_OnNpcHtml(const TCHAR* Html)
{
    if (!AutoShot_Enabled() || !Html)
        return 0;
    const TCHAR* Data = Html;
    if (HasPrefix(Data, TEXT("<html><body>"), 1))  // a server path that forces <html>
        Data += 12;
    if (!HasPrefix(Data, TEXT("L2W "), 0))
        return 0;
    if (HasPrefix(Data, TEXT("L2W mfg"), 0))  // the craft windows' (CraftUi)
        return 0;
    OnShotData(Data);
    return 1;
}

void AutoShot_OnTick(NConsoleWnd* Console)
{
    if (!AutoShot_Enabled())
        return;
    // the first tick in the world with a new player: NWindow_C1 announced on the first UserInfo
    // of a new User (an Engine.dll hook; UserInfo does not reach NWindow) and forgot it on the
    // restart / logout answers; here leaving the world forgets it (the lobby, the login screen),
    // not the loading screen (L2CS_Loading), which comes and goes without a new player
    if (Console->State != L2CS_InGame)
    {
        if (Console->State != L2CS_Loading)
            GAnnouncedID = 0;
    }
    else
    {
        User* Me = Console->GetMyUser();
        if (Me && Me->ObjectID && Me->ObjectID != GAnnouncedID)
        {
            GNumShotsOn = 0;  // the server forgets the automatic shots on every relog
            if (SendBypassToServer(TEXT("ss_ui 1")))
            {
                GAnnouncedID = Me->ObjectID;
                Log("autoshot", "announced auto shots (player %d)", Me->ObjectID);
            }
        }
    }
    if (bShotMsg)
    {
        bShotMsg = 0;
        NCConsole* Desktop = Console->GetDesktop();
        if (Desktop)
            Desktop->AddSystemText(GShotMsg, 0, 0xFFB09B79);
    }
}

/*-----------------------------------------------------------------------------
    The bar.
-----------------------------------------------------------------------------*/

// the slots of the current page as the stock OnPaint places them: 31 on, IconSize + 5 apart,
// 6 across; the mark as the stock one over a toggle skill (its UnknownA4 set to 9 first)
void AutoShot_OnPaint(NCShortCutWnd* Wnd, NCGDevice* Device)
{
    if (!AutoShot_Enabled() || !GNumShotsOn)
        return;
    UTexture* Mark = Wnd->ToggleTex;
    if (!Mark)
        return;
    Device->BeginPaint(Wnd->ScreenX, Wnd->ScreenY);
    Device->SetClip(Wnd->ScreenX, Wnd->ScreenY, Wnd->Width, Wnd->Height);
    INT Size = Wnd->IconSize;
    INT Pos = 31;
    for (INT i = 0; i < Wnd->Cols; i++, Pos += Size + 5)
    {
        NShortCutItem* Item = SlotItem(Wnd, Wnd->Page * Wnd->Cols + i);
        if (!Item || !Item->bUsed || Item->Type != SHORTCUT_Item)
            continue;
        if (!IsShotOn(Item->ID) || !IsShot(Item->ID))
            continue;
        Mark->UnknownA4 = 9.0f;
        if (Wnd->bHorizontal)
            Device->DrawTexture(Pos, 6, Size, Size, 0, 0, Size, Size, Mark);
        else
            Device->DrawTexture(6, Pos, Size, Size, 0, 0, Size, Size, Mark);
    }
    Device->ReleaseClip();
    Device->EndPaint();
}

UBOOL AutoShot_OnRButtonDown(NCShortCutWnd* Wnd, WPARAM wParam, LPARAM lParam)
{
    if (!AutoShot_Enabled())
        return 0;
    INT Slot = Wnd->SlotAt((SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
    NShortCutItem* Item = SlotItem(Wnd, Slot);
    if (!Item || !Item->bUsed || Item->Type != SHORTCUT_Item)
        return 0;  // not an item: the click goes on (the camera), as on the stock bar
    INT ID = Item->ID;
    if (!IsShot(ID))
        return 0;
    UBOOL bOn = IsShotOn(ID);
    TCHAR Cmd[48];
    appSprintf(Cmd, TEXT("ss_set %d %d"), ID, bOn ? 0 : 1);
    UBOOL bSent = SendBypassToServer(Cmd);  // the server answers with the new state
    Log("autoshot", "right click slot item=%d was=%s -> %s (sent=%d)", ID, bOn ? "on" : "off",
        bOn ? "off" : "on", bSent ? 1 : 0);
    GConsole->Engine->OnInterfacePlaySound(0);  // the click
    return 1;  // no mouse look from a right click on a shot
}

// C3: a shortcut dropped on another slot moves there. The list is the server's, so these are
// the requests the stock bar sends for a drop from the inventory or the skill window.
static void MoveShortCut(NCShortCutWnd* Wnd, INT From, INT To)
{
    NShortCutItem* Src = SlotItem(Wnd, From);
    NShortCutItem* Dst = SlotItem(Wnd, To);
    if (!Src || !Dst)
        return;
    INT Type = Src->Type;
    INT ID = Type == SHORTCUT_Item ? Src->ObjectID : Src->ID;
    if (Type < SHORTCUT_Item || !ID)
        return;
    if (Dst->bUsed)
    {
        // a swap: the other one takes the slot left
        INT DstType = Dst->Type;
        INT DstID = DstType == SHORTCUT_Item ? Dst->ObjectID : Dst->ID;
        if (DstType >= SHORTCUT_Item && DstID)
            SendShortCutReg(DstType, From, DstID, 1);
    }
    else
    {
        SendShortCutDel(From);  // the server does not answer a deletion: cleared here
        Src->Clear();
    }
    SendShortCutReg(Type, To, ID, 1);  // the server answers with the slot
    Log("autoshot", "moved shortcut %d -> %d (type %d id %d)", From, To, Type, ID);
}

// after the stock OnLButtonUp, which used the slot released on its own pressed slot and
// deleted one dropped off the bar; a drop on another slot it leaves as it was
void AutoShot_OnLButtonUp(NCShortCutWnd* Wnd, LPARAM lParam)
{
    if (!AutoShot_Enabled())
        return;
    INT From = Wnd->PressedSlot;
    if (From < 0)
        return;
    INT To = Wnd->SlotAt((SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
    NShortCutItem* Src = SlotItem(Wnd, From);
    if (To < 0 || To == From || !Src || !Src->bUsed)
        return;
    MoveShortCut(Wnd, From, To);
    Wnd->PressedSlot = -1;
    Wnd->bPressed = 0;
}

}  // namespace nwf

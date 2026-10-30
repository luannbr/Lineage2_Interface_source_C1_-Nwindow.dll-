/*=============================================================================
    ManufactureCmd.cpp - phase 6: /dwarvenmanufacture, action ids 37 and 52, and the requests
    the features send (include/features/ManufactureCmd.h).

    NWindow_C1 src/inject/ManufactureCmd.cpp got there by patching: a detour on Engine.dll's
    FL2GameData::GetCommandType, the three action id dispatches of NWindow.dll (the shortcut bar's
    two jump tables, the action window's remap table) widened from id 31 to 52 at their stock
    addresses, and detours on Network.dll only to capture the network handler. Here the command
    is checked by NCommandParser::GetCommand, the dispatches call ManufactureCmd_UseAction, and
    the requests go through the console to GConsole->Engine->NetHandler as the stock ones.
=============================================================================*/
#include "nwindow/NWindow.h"
#include "nwindow/NConsoleWnd.h"
#include "features/ManufactureCmd.h"

namespace nwf {

/*-----------------------------------------------------------------------------
    The requests.
-----------------------------------------------------------------------------*/

// the network handler the stock requests go through (NWindow_C1 waited for its capture)
static UBOOL NetReady()
{
    return GConsole && GConsole->Engine && GConsole->Engine->NetHandler;
}

UBOOL SendRequestActionUse(INT ActionID)
{
    if (!NetReady())
    {
        Log("mfgcmd", "send FAIL no handler");
        return 0;
    }
    // the values themselves in the slots, as the stock requests push them: a pointer to a local
    // put a stack address on the wire (MANUFACTURE_37_ACTIONID_BUG.md)
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)ActionID);
    Params.PushBack((void*)0);  // Ctrl
    Params.PushBack((void*)0);  // Shift
    GConsole->RequestActionUse(Params);
    Log("mfgcmd", "RequestActionUse(%d) OK h=%p (int-as-slot)", ActionID,
        GConsole->Engine->NetHandler);
    return 1;
}

// C2S 0x21 "cS", what an HTML link sends
static UBOOL SendBypassNow(const TCHAR* Cmd)
{
    if (!NetReady())
    {
        Log("mfgcmd", "bypass FAIL no handler yet");
        return 0;
    }
    L2ParamStack Params(4);
    Params.PushBack((void*)Cmd);
    GConsole->RequestBypassToServer(Params);
    return 1;
}

// the server (packetfilter.xml: RequestBypassToServer count=1 perMs=100) drops a bypass that
// comes too soon after another, answering only ActionFailed: two of ours in one frame (the craft
// and the automatic shots' announcements) would lose the second
// BYPASS_MaxLen: NWindow_C1 had 200, which refused the craft window's "mfg_start" once a shop
// had about 15-20 offers (the Start was silently not sent and the shop did not open). 1024 is
// the stock HTML bypass buffer (NConsoleWnd::RunHtmlCommand), so Network.dll already sends that
// much, and the server reads the string without a limit; its 40 offers need about 660.
enum { BYPASS_Interval = 120, BYPASS_QueueMax = 8, BYPASS_MaxLen = 1024 };
static TCHAR GBypassQueue[BYPASS_QueueMax][BYPASS_MaxLen];
static INT GBypassHead = 0;
static INT GBypassCount = 0;
static DWORD GLastBypass = 0;   // GetTickCount of the last one sent, 0 = none yet

UBOOL SendBypassToServer(const TCHAR* Cmd)
{
    if (!Cmd || !*Cmd || appStrlen(Cmd) >= BYPASS_MaxLen)
        return 0;
    DWORD Now = GetTickCount();
    if (!GBypassCount && (!GLastBypass || Now - GLastBypass >= BYPASS_Interval))
    {
        if (!SendBypassNow(Cmd))
            return 0;
        GLastBypass = Now;
        return 1;
    }
    if (GBypassCount >= BYPASS_QueueMax)
    {
        Log("mfgcmd", "bypass queue full");
        return 0;
    }
    appStrncpy(GBypassQueue[(GBypassHead + GBypassCount) % BYPASS_QueueMax], Cmd, BYPASS_MaxLen);
    GBypassCount++;
    return 1;
}

// a queued bypass is sent once, even when that fails
void BypassFlush()
{
    if (!GBypassCount)
        return;
    DWORD Now = GetTickCount();
    if (GLastBypass && Now - GLastBypass < BYPASS_Interval)
        return;
    const TCHAR* Cmd = GBypassQueue[GBypassHead];
    UBOOL bSent = SendBypassNow(Cmd);
    if (bSent)
        GLastBypass = Now;
    if (LogEnabled())
    {
        char Text[BYPASS_MaxLen * 2];   // a double-byte code page needs up to two per character
        if (!WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)Cmd, -1, Text, sizeof(Text), NULL, NULL))
            Text[0] = 0;
        Text[sizeof(Text) - 1] = 0;
        Log("mfgcmd", "queued bypass '%s' sent=%d", Text, bSent ? 1 : 0);
    }
    GBypassHead = (GBypassHead + 1) % BYPASS_QueueMax;
    GBypassCount--;
}

UBOOL SendShortCutReg(INT Type, INT Slot, INT ID, INT Extra)
{
    if (!NetReady())
        return 0;
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Type);
    Params.PushBack((void*)(INT_PTR)Slot);
    Params.PushBack((void*)(INT_PTR)ID);
    Params.PushBack((void*)(INT_PTR)Extra);
    // the net handler itself, as NWindow_C1 (Network.dll's export): the console's
    // RequestShortCutReg (0x10044830) also plays the pick-up sound, which a move must not
    GConsole->Engine->NetHandler->RequestShortCutReg(Params);
    return 1;
}

UBOOL SendShortCutDel(INT Slot)
{
    if (!NetReady())
        return 0;
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)Slot);
    GConsole->RequestShortCutDel(Params);
    return 1;
}

/*-----------------------------------------------------------------------------
    /dwarvenmanufacture and the actions.
-----------------------------------------------------------------------------*/

// ASCII letters without case, the rest as is
static TCHAR FoldAscii(TCHAR C)
{
    return (C >= 'A' && C <= 'Z') ? (TCHAR)(C - 'A' + 'a') : C;
}

UBOOL ManufactureCmd_OnCommand(const TCHAR* Command)
{
    if (!Command)
        return 0;
    if (Command[0] == '/')
        Command++;
    const TCHAR* Name = TEXT("dwarvenmanufacture");
    while (*Command && *Name && FoldAscii(*Command) == FoldAscii(*Name))
    {
        Command++;
        Name++;
    }
    if (*Command || *Name)
        return 0;
    // no race check here: the server answers a non-dwarf with a message
    if (SendRequestActionUse(ACTION_DwarvenManufacture))
        Log("mfgcmd", "/dwarvenmanufacture -> 37 OK");
    else
        Log("mfgcmd", "/dwarvenmanufacture send failed");
    return 1;
}

UBOOL ManufactureCmd_UseAction(INT ActionID, UBOOL bActionWindow)
{
    // the stock cases (0 .. 31) stay the switch's; past 52 nothing, as past 31 in the stock
    if (ActionID < 32 || ActionID > ACTION_MaxExtended)
        return 0;
    if (ActionID == ACTION_DwarvenManufacture)
    {
        UBOOL bSent = SendRequestActionUse(ACTION_DwarvenManufacture);
        Log("mfgcmd", "%s click id=37 %s", bActionWindow ? "Actions" : "ActionBar",
            bSent ? "OK" : "send FAIL");
    }
    else if (ActionID == ACTION_ServitorReturn)
    {
        // C1 has no servitor Return; the server unsummons on 52 (id 19 is the pet's)
        if (SendRequestActionUse(ACTION_ServitorReturn))
            Log("mfgcmd", "summon Return id=52 OK");
        else
            Log("mfgcmd", "summon Return id=52 send FAIL");
    }
    else
    {
        // NWindow_C1 pointed the other new ids at the case of id 0: the stock RequestActionUse
        // group, with Ctrl (force) and Shift (don't move)
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)ActionID);
        Params.PushBack((void*)(INT_PTR)GConsole->IsCtrlDown());
        Params.PushBack(
            (void*)(INT_PTR)GConsole->Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
        GConsole->RequestActionUse(Params);
    }
    return 1;
}

}  // namespace nwf

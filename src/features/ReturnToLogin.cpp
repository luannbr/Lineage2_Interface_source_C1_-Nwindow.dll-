/*=============================================================================
    ReturnToLogin.cpp - phase 6: back to the login screen instead of a stuck server list or
    a closed game, ported from NWindow_C1 src/inject/ReturnToLogin.cpp (HEAD 4e56326; RE notes
    in its docs/C1_DISCONNECT_RE.md sections 2-5, ours in docs/notas/F6_ReturnToLogin.md).
    Every piece below is a stock C1 routine; this file only decides when to run them.

    1. Failed game connect after server select (RE section 2)
       The PlayOk handler closes the login socket, starts an async connect to the game server and
       sets the network phase to 2 without telling the UI (console state stays 2). Every failure
       of that connect is dropped, and a second click on the list sends RequestServerLogin on the
       closed login socket. The two game-socket handlers are wrapped to note a connect error or a
       close; the Tick then runs the server list's own Cancel (AuthReconnect, login window,
       SetState(2)) when that happens or after the timeout, and prints "Unable to connect to
       server.".

    2. Disconnect popup (RE sections 3-4)
       Dialog 0x1D ("disconnected from the server", systemmsg 127) calls console slot 273 on OK,
       which saves WindowsInfo.ini and calls appRequestExit. That slot (NConsoleWnd::QuitGame)
       now only requests the transition, and Esc on the same dialog does too (stock Esc just
       closed it over a dead world). From the Tick: in game, the stock Restart teardown (hide the
       game UI, ContinueServerClose, clear the per-character caches, Browse "Lobby?L2Restart" to
       character select); then, once character select is up, the stock character-select Back
       (logout, AuthReconnect, login screen). Both halves are the paths the client already runs
       for Restart and for Back.
       Also: a close without error on the game socket (server crash, proxy drop) was never
       noticed; after 1 s without a ServerClose it is reported as OnServerClose(1), the stock
       popup.

    Never reconnect automatically right after AuthReconnect: it leaves the old receive thread
    alive for up to ~100 ms (RE section 5.4). A human re-login takes seconds.

    What NWindow_C1 patched in NWindow (console slot +0x444, the dialog box's Cancel slot, its
    Tick port) and the stock routines it called by address are direct calls of our code here;
    Network.dll and Engine.dll keep their runtime patches, checked at install.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCServerSelectWnd.h"
#include "features/ReturnToLogin.h"

namespace {

// ---- Network.dll RVAs. It prefers 0x10000000 like NWindow.dll, so one of the two is relocated:
// always module base + RVA.
const DWORD NET_GameSockVt = 0x33D58;        // CClientSocket vtable
const DWORD NET_HandlerVt = 0x33798;         // UNetworkHandler vtable
const DWORD SOCKVT_OnConnect = 0x64 / 4;     // FD_CONNECT result
const DWORD SOCKVT_OnNetMsg = 0x68 / 4;      // FD_READ / FD_OOB / FD_CLOSE
const DWORD NET_OnConnect = 0x2320;
const DWORD NET_OnNetMsg = 0x10370;
const DWORD NETVT_ClearObjectMap = 0xC0 / 4; // UL2NetHandler::ClearObjectMap, slot 48
const DWORD NET_ClearObjectMap = 0x218C0;
// Per-character caches on the game socket that RestartResponse clears (fastcall, ecx = socket).
const DWORD NET_CacheClear[5] = {0x125E0, 0x12710, 0x12760, 0x12790, 0x12800};
// UNetworkHandler and socket fields (Network.dll's objects, not declared by the engine headers)
const DWORD SOCK_Handle = 0x2C;   // SOCKET, 0 = closed
const DWORD NET_GameSock = 0x3C;
const DWORD NET_Phase = 0x44;     // 2 = game-server phase, set by the PlayOk handler
const DWORD NET_Notify = 0x48;    // FL2NetNotify* (the UGameEngine sub-object)

// ---- Engine.dll (an /INCREMENTAL build: vtables hold ILT thunks)
const DWORD ENG_ObjVt = 0x487198;   // UGameEngine UObject vtable (??_7UGameEngine@@6BUObject@@@)
const DWORD ENGVT_ContinueServerClose = 0xE0 / 4;
const DWORD ENG_ContinueServerCloseThunk = 0x10B9F;
const DWORD ENG_ContinueServerClose = 0x11F430;
const DWORD ENG_OnServerCloseThunk = 0x78F6;   // FL2NetNotify +0x230 OnServerClose(reason)
const DWORD ENG_OnServerClose = 0x11F3E0;

const INT MSG_UnableToConnect = 90;            // systemmsg: "Unable to connect to server."
const INT DIALOG_Disconnected = 0x1D;          // NConsoleWnd::ServerClose's box

// WSAAsyncSelect lParam: low word = event, high word = error (no winsock headers needed).
const WORD FD_CLOSE_EVENT = 0x20;

typedef INT(__thiscall* SockHandlerFn)(void* Sock, WPARAM w, LPARAM l);
typedef void(__thiscall* ThisIntFn)(void* Self, INT Arg);
typedef void(__fastcall* FastVoidFn)(void* Self);

enum EInst
{
    INST_Pending,
    INST_On,
    INST_Off,
};

enum EDisc
{
    DISC_Idle,
    DISC_Start,
    DISC_WaitCharSelect,
};

bool GReady = false;   // modules resolved, config read
EInst GConnect = INST_Pending;
EInst GDisconnect = INST_Pending;
BYTE* GNetDll = NULL;
BYTE* GEngineDll = NULL;
bool GCfgConnect = true;
bool GCfgDisconnect = true;
DWORD GTimeoutMs = 25000;

SockHandlerFn GOrigConnect = NULL;
SockHandlerFn GOrigNetMsg = NULL;
ThisIntFn GOrigServerClose = NULL;

// 1. connect watchdog
volatile LONG GConnectFailed = 0;   // connect error or close of the game socket
bool GArmed = false;
DWORD GArmedAt = 0;

// 2. disconnect
EDisc GDisc = DISC_Idle;
DWORD GDiscAt = 0;
bool GServerCloseSeen = false;    // OnServerClose ran since the login screen
volatile LONG GGracefulAt = 0;    // GetTickCount of a close without error on the game socket

UL2NetHandler* NetOf(NConsoleWnd* Console)
{
    return Console->Engine ? Console->Engine->NetHandler : NULL;
}

// Only events of the socket the handler currently owns (not a late message for an old one).
bool IsCurrent(void* Sock, WPARAM w)
{
    return Sock && w != 0 && w == *(WPARAM*)((BYTE*)Sock + SOCK_Handle);
}

/*-----------------------------------------------------------------------------
    Hooks (Network.dll, Engine.dll).
-----------------------------------------------------------------------------*/

INT __fastcall HookOnConnect(void* Sock, void*, WPARAM w, LPARAM l)
{
    if (HIWORD(l) != 0 && IsCurrent(Sock, w))
        InterlockedExchange(&GConnectFailed, 1);
    return GOrigConnect(Sock, w, l);
}

INT __fastcall HookOnNetMsg(void* Sock, void*, WPARAM w, LPARAM l)
{
    if (LOWORD(l) == FD_CLOSE_EVENT && IsCurrent(Sock, w))
    {
        InterlockedExchange(&GConnectFailed, 1);
        if (HIWORD(l) == 0)   // stock ignores this one: the server just closed the connection
            InterlockedExchange(&GGracefulAt, (LONG)(GetTickCount() | 1));
    }
    return GOrigNetMsg(Sock, w, l);
}

void __fastcall HookOnServerClose(void* Notify, void*, INT Reason)
{
    GServerCloseSeen = true;
    GOrigServerClose(Notify, Reason);
}

/*-----------------------------------------------------------------------------
    Stock routines.
-----------------------------------------------------------------------------*/

// the text of a system message (GL2GameData's systemmsg table), NULL when empty
const TCHAR* MsgText(INT Id)
{
    const TCHAR* Text = GL2GameData.SystemMsg[Id].Message.GetCharData();
    return (Text && *Text) ? Text : NULL;
}

// Server list Cancel: AuthReconnect (both sockets closed, phase 0), login window, SetState(2).
bool ServerListCancel(NConsoleWnd* Console)
{
    NCServerSelectWnd* List = Console->LoginWnd ? Console->LoginWnd->ServerSelectWnd : NULL;
    if (!List)
    {
        nwf::Log("returntologin", "server list not found - left as is");
        return false;
    }
    List->OnCancel();
    return true;
}

// ContinueServerClose follows Client->Viewports(0)->Actor->Pawn without null checks.
bool PawnChainOk(UEngine* Eng)
{
    UClient* Client = Eng->Client;
    UViewport** Viewports = Client ? (UViewport**)Client->Viewports.GetData() : NULL;
    UViewport* Viewport = Viewports ? Viewports[0] : NULL;
    APlayerController* Actor = Viewport ? Viewport->Actor : NULL;
    return Actor && Actor->Pawn;
}

// Every world object gone, as in the stock Restart (ContinueServerClose does exactly that).
void ClearWorld(NConsoleWnd* Console)
{
    UEngine* Eng = Console->Engine;
    if (!Eng)
        return;
    if (*(BYTE**)Eng == GEngineDll + ENG_ObjVt && PawnChainOk(Eng))
    {
        static_cast<UGameEngine*>(Eng)->ContinueServerClose();
        return;
    }
    UL2NetHandler* Net = Eng->NetHandler;
    if (Net && *(BYTE**)Net == GNetDll + NET_HandlerVt)
        Net->ClearObjectMap(NULL);
    nwf::Log("returntologin", "no pawn - ClearObjectMap(0) only");
}

void ClearCharCaches(NConsoleWnd* Console)
{
    UL2NetHandler* Net = NetOf(Console);
    BYTE* Sock = Net ? *(BYTE**)((BYTE*)Net + NET_GameSock) : NULL;
    if (!Sock || *(BYTE**)Sock != GNetDll + NET_GameSockVt)
        return;
    for (DWORD i = 0; i < sizeof(NET_CacheClear) / sizeof(NET_CacheClear[0]); i++)
        ((FastVoidFn)(GNetDll + NET_CacheClear[i]))(Sock);
}

void ToLogin(NConsoleWnd* Console, INT State)
{
    if (State == L2CS_None || State == L2CS_Login)
    {
        ServerListCancel(Console);
        return;
    }
    if (State == L2CS_Create)
        Console->ReturnToCharacterSelect();
    // Logout packet (dropped on the dead socket), AuthReconnect, login screen, SetState(2).
    Console->ReturnToLogin();
    nwf::Log("returntologin", "disconnected - back to the login screen");
}

/*-----------------------------------------------------------------------------
    Tick parts.
-----------------------------------------------------------------------------*/

void DisconnectStep(NConsoleWnd* Console)
{
    const INT Raw = Console->State;
    const INT State = Raw == L2CS_Loading ? Console->SavedState : Raw;

    if (GDisc == DISC_Start)
    {
        Console->SaveWindowPositions();   // what the stock OK did first
        if (State == L2CS_InGame)
        {
            Console->RestartResetWindows();   // console slot 142 (+0x238)
            ClearWorld(Console);
            ClearCharCaches(Console);
            Console->RestartToLobby();        // console slot 143 (+0x23C)
            GDisc = DISC_WaitCharSelect;
            GDiscAt = GetTickCount();
            nwf::Log("returntologin", "disconnected in game - world down, character select");
            return;
        }
        GDisc = DISC_Idle;
        ToLogin(Console, State);
        return;
    }

    // WaitCharSelect: the Back to login runs on the next ticks, once the lobby is really up
    // (state 4 itself, not 7 = loading), as when the player presses Back on character select.
    if (Raw == L2CS_Lobby || GetTickCount() - GDiscAt > 10000)
    {
        GDisc = DISC_Idle;
        ToLogin(Console, L2CS_Lobby);
    }
}

void GracefulCloseCheck(NConsoleWnd* Console, UL2NetHandler* Net, INT Raw)
{
    const LONG At = GGracefulAt;
    // Wait 1 s: a ServerClose packet that arrived just before the FIN is dispatched from the
    // network queue and shows the popup itself.
    if (!At || GetTickCount() - (DWORD)At < 1000)
        return;
    InterlockedExchange(&GGracefulAt, 0);
    const INT State = Raw == L2CS_Loading ? Console->SavedState : Raw;
    if (GServerCloseSeen || GIsRequestingExit || !Net || (State != L2CS_InGame && State != L2CS_Lobby))
        return;
    FL2NetNotify* Notify = *(FL2NetNotify**)((BYTE*)Net + NET_Notify);
    if (!Notify)
        return;
    nwf::Log("returntologin", "server closed the connection without ServerClose - disconnect popup");
    // Through the stock filter (UGameEngine::OnServerClose): in game / char select -> popup.
    Notify->OnServerClose(1);
}

void ConnectWatchdog(NConsoleWnd* Console, UL2NetHandler* Net, INT Raw)
{
    const INT Phase = Net ? *(INT*)((BYTE*)Net + NET_Phase) : 0;
    // Watch only between PlayOk and character select: still on the login / server list screen,
    // network already in the game-server phase.
    if (!((Raw == L2CS_None || Raw == L2CS_Login) && Phase == 2))
    {
        GArmed = false;
        InterlockedExchange(&GConnectFailed, 0);
        return;
    }
    if (!GArmed)
    {
        GArmed = true;
        GArmedAt = GetTickCount();
    }
    if (!GConnectFailed && GetTickCount() - GArmedAt < GTimeoutMs)
        return;

    GArmed = false;
    InterlockedExchange(&GConnectFailed, 0);
    if (!ServerListCancel(Console))
        return;
    const TCHAR* Text = MsgText(MSG_UnableToConnect);
    Console->ShowMessageWnd(Text ? Text : TEXT("Unable to connect to server."));
    nwf::Log("returntologin", "game server unreachable - back to the login screen");
}

void TickBody(NConsoleWnd* Console)
{
    if (GDisc != DISC_Idle)
    {
        DisconnectStep(Console);
        return;
    }
    const INT Raw = Console->State;
    UL2NetHandler* Net = NetOf(Console);
    if (Raw == L2CS_None || Raw == L2CS_Login)
    {
        GServerCloseSeen = false;
        InterlockedExchange(&GGracefulAt, 0);
    }
    if (GDisconnect == INST_On)
        GracefulCloseCheck(Console, Net, Raw);
    if (GConnect == INST_On)
        ConnectWatchdog(Console, Net, Raw);
}

/*-----------------------------------------------------------------------------
    Install.
-----------------------------------------------------------------------------*/

void LoadConfig()
{
    GCfgConnect = nwf::IniInt("Connect", "ReturnToLogin", 1) != 0;
    GCfgDisconnect = nwf::IniInt("Connect", "DisconnectToLogin", 1) != 0;
    UINT Secs = (UINT)nwf::IniInt("Connect", "TimeoutSeconds", 25);   // unsigned, as NWindow_C1 read it
    if (Secs < 5)
        Secs = 5;
    if (Secs > 120)
        Secs = 120;
    GTimeoutMs = Secs * 1000;
}

bool BytesMatch(const BYTE* P, const BYTE* Sig, DWORD Count)
{
    for (DWORD i = 0; i < Count; ++i)
        if (P[i] != Sig[i])
            return false;
    return true;
}

bool PatchPtr(void** Slot, void* Value)
{
    DWORD Old = 0;
    if (!VirtualProtect(Slot, sizeof(void*), PAGE_READWRITE, &Old))
        return false;
    *Slot = Value;
    VirtualProtect(Slot, sizeof(void*), Old, &Old);
    return true;
}

// Engine.dll's ILT thunks (Engine.dll is an /INCREMENTAL build): "E9 rel32" -> body
BYTE* IltTarget(BYTE* Mod, DWORD ThunkRva)
{
    BYTE* Thunk = Mod + ThunkRva;
    if (Thunk[0] != 0xE9)
        return NULL;
    return Thunk + 5 + *(INT*)(Thunk + 1);
}

// The thunk points at Mod+BodyRva whose first bytes are Sig - or is already hooked to Hook.
bool IltCheck(BYTE* Mod, DWORD ThunkRva, DWORD BodyRva, const void* Hook, const BYTE* Sig, DWORD SigLen)
{
    BYTE* Target = IltTarget(Mod, ThunkRva);
    if (!Target)
        return false;
    if (Target == (const BYTE*)Hook)
        return true;
    if (Target != Mod + BodyRva)
        return false;
    return BytesMatch(Target, Sig, SigLen);
}

bool IltRetarget(BYTE* Mod, DWORD ThunkRva, const void* Hook)
{
    BYTE* Thunk = Mod + ThunkRva;
    DWORD Old = 0;
    if (!VirtualProtect(Thunk, 5, PAGE_EXECUTE_READWRITE, &Old))
        return false;
    *(INT*)(Thunk + 1) = (INT)((const BYTE*)Hook - (Thunk + 5));
    VirtualProtect(Thunk, 5, Old, &Old);
    FlushInstructionCache(GetCurrentProcess(), Thunk, 5);
    return true;
}

// Signatures skip absolute addresses: they differ in the relocated module.
const BYTE SIG_OnConnect[10] = {0x8B, 0x44, 0x24, 0x08, 0x83, 0xEC, 0x08, 0xC1, 0xE8, 0x10};
const BYTE SIG_OnNetMsg[6] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68};

// Both features share the game-socket wrappers; they go in when either feature is wanted.
bool InstallSocketHooks()
{
    void** Vt = (void**)(GNetDll + NET_GameSockVt);
    const bool bConnectOurs = Vt[SOCKVT_OnConnect] == (void*)&HookOnConnect;
    const bool bNetMsgOurs = Vt[SOCKVT_OnNetMsg] == (void*)&HookOnNetMsg;
    if (bConnectOurs && bNetMsgOurs)
        return true;
    // one slot ours: an earlier try saved both originals and failed on the second patch (NWindow_C1
    // tested only the first slot and took this for installed); finish it
    if (bConnectOurs || bNetMsgOurs)
        return (bConnectOurs || PatchPtr(&Vt[SOCKVT_OnConnect], (void*)&HookOnConnect)) &&
               (bNetMsgOurs || PatchPtr(&Vt[SOCKVT_OnNetMsg], (void*)&HookOnNetMsg));
    if (Vt[SOCKVT_OnConnect] != GNetDll + NET_OnConnect || Vt[SOCKVT_OnNetMsg] != GNetDll + NET_OnNetMsg ||
        !BytesMatch(GNetDll + NET_OnConnect, SIG_OnConnect, sizeof(SIG_OnConnect)) ||
        !BytesMatch(GNetDll + NET_OnNetMsg, SIG_OnNetMsg, sizeof(SIG_OnNetMsg)))
        return false;
    GOrigConnect = (SockHandlerFn)Vt[SOCKVT_OnConnect];
    GOrigNetMsg = (SockHandlerFn)Vt[SOCKVT_OnNetMsg];
    return PatchPtr(&Vt[SOCKVT_OnConnect], (void*)&HookOnConnect) &&
           PatchPtr(&Vt[SOCKVT_OnNetMsg], (void*)&HookOnNetMsg);
}

// NWindow_C1 also checked the stock NWindow routines it called by address (the server list's
// Cancel, the status text); here they are our code.
bool InstallConnect()
{
    return InstallSocketHooks();
}

bool InstallDisconnect()
{
    static const BYTE SIG_ContinueServerClose[9] = {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x4C, 0x8B, 0x48, 0x2C};
    static const BYTE SIG_OnServerClose[9] = {0x8B, 0x49, 0x2C, 0x8B, 0x41, 0x2C, 0x83, 0xF8, 0x06};
    static const BYTE SIG_Cache0[6] = {0x8B, 0x81, 0x5C, 0x4D, 0x00, 0x00};
    static const BYTE SIG_Cache4[6] = {0x8B, 0x81, 0xAC, 0x4D, 0x00, 0x00};

    // NWindow_C1 checked the NWindow side too (console slot +0x444, the dialog box's Cancel,
    // the Restart slots, SaveWinInfo, the Back routines); here that is our code, which calls
    // this feature when it is on.
    void** EngVt = (void**)(GEngineDll + ENG_ObjVt);
    void** NetVt = (void**)(GNetDll + NET_HandlerVt);
    if (EngVt[ENGVT_ContinueServerClose] != GEngineDll + ENG_ContinueServerCloseThunk ||
        IltTarget(GEngineDll, ENG_ContinueServerCloseThunk) != GEngineDll + ENG_ContinueServerClose ||
        !BytesMatch(GEngineDll + ENG_ContinueServerClose, SIG_ContinueServerClose, 9) ||
        !IltCheck(GEngineDll, ENG_OnServerCloseThunk, ENG_OnServerClose, (const void*)&HookOnServerClose,
                  SIG_OnServerClose, 9) ||
        NetVt[NETVT_ClearObjectMap] != GNetDll + NET_ClearObjectMap ||
        !BytesMatch(GNetDll + NET_CacheClear[0], SIG_Cache0, 6) ||
        !BytesMatch(GNetDll + NET_CacheClear[4], SIG_Cache4, 6))
        return false;
    if (!InstallSocketHooks())
        return false;

    GOrigServerClose = (ThisIntFn)(GEngineDll + ENG_OnServerClose);
    return IltRetarget(GEngineDll, ENG_OnServerCloseThunk, (const void*)&HookOnServerClose);
}

}  // namespace

namespace nwf {

bool ReturnToLogin_Install()
{
    if (GConnect != INST_Pending && GDisconnect != INST_Pending)
        return GConnect == INST_On || GDisconnect == INST_On;
    if (!GReady)
    {
        HMODULE Net = GetModuleHandleW(L"Network.dll");
        HMODULE Eng = GetModuleHandleW(L"Engine.dll");
        if (!Net || !Eng)
            return false;   // not mapped yet: retried from the Tick
        GNetDll = (BYTE*)Net;
        GEngineDll = (BYTE*)Eng;
        LoadConfig();
        GReady = true;
    }
    // Another client build keeps its stock behaviour.
    if (GConnect == INST_Pending)
    {
        GConnect = GCfgConnect && InstallConnect() ? INST_On : INST_Off;
        Log("returntologin", GConnect == INST_On ? "connect watchdog on"
                                                 : "connect watchdog off (config or other build)");
    }
    if (GDisconnect == INST_Pending)
    {
        GDisconnect = GCfgDisconnect && InstallDisconnect() ? INST_On : INST_Off;
        Log("returntologin", GDisconnect == INST_On ? "disconnect -> login on"
                                                    : "disconnect -> login off (config or other build)");
    }
    return GConnect == INST_On || GDisconnect == INST_On;
}

void ReturnToLogin_OnTick(NConsoleWnd* Console)
{
    if (!Console)
        return;
    if (GConnect == INST_Pending || GDisconnect == INST_Pending)
        ReturnToLogin_Install();
    __try
    {
        TickBody(Console);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        GArmed = false;
        GDisc = DISC_Idle;
        Log("returntologin", "EXCEPTION in Tick");
    }
}

bool ReturnToLogin_QuitGame()
{
    // NWindow_C1 replaced console slot +0x444 only once the disconnect part was installed
    if (GDisconnect != INST_On)
        return false;
    if (GDisc == DISC_Idle)
        GDisc = DISC_Start;   // the transition runs from the Tick, once the box's OK has finished
    return true;
}

void ReturnToLogin_DialogCancelled(INT DialogID)
{
    if (GDisconnect == INST_On && DialogID == DIALOG_Disconnected && GDisc == DISC_Idle)
        GDisc = DISC_Start;
}

}  // namespace nwf

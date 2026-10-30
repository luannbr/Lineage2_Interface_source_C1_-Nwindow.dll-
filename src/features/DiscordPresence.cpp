/*=============================================================================
    DiscordPresence.cpp - phase 6: Discord Rich Presence (include/features/DiscordPresence.h).

    The Discord desktop client listens on a local named pipe, \\.\pipe\discord-ipc-0 .. -9. Every
    message is a frame: opcode (DWORD), payload length (DWORD), JSON payload.
      1. HANDSHAKE (0) {"v":1,"client_id":"<application id>"} -> FRAME (1) with "evt":"READY",
         or CLOSE (2) when the id is not an application
      2. FRAME {"cmd":"SET_ACTIVITY","args":{"pid":..,"activity":{..}},"nonce":".."}
      3. then keep the pipe open (Discord clears the activity when it closes) and answer each
         PING (3) with a PONG (4) carrying the same payload.
    Discord shows "Playing <application name>", the details / state lines, the time since
    timestamps.start, the assets (or the application icon when there are none) and up to two
    buttons (seen by the others on the profile, never by oneself).

    The thread starts in DllMain, before Core has its allocator (new / delete go through GMalloc
    here, src/re/NwMemory.cpp): no heap, only fixed buffers and kernel32.
=============================================================================*/
#include "features/DiscordPresence.h"

#include <string.h>

namespace nwf {
namespace {

// --- what presence.ini carried, compiled in (the owner's data, 29/09/2026) ---------------------
// The application (id 1154460536088567829) is "L2GIRAN.COM" in the Discord Developer Portal: that
// name is the "Playing ..." line and its icon is the picture. Its only Art Asset is another
// server's logo (l2skelth), so no asset is named here.
const char AppId[] = "1154460536088567829";
const char Details[] = "Play L2GIRAN.COM chronicle 1";
const char State[] = "";
const char LargeImage[] = "";     // Art Asset keys uploaded to the application; empty: app icon
const char LargeText[] = "";
const char SmallImage[] = "";
const char SmallText[] = "";
const char Button1Label[] = "Site";
const char Button1Url[] = "https://l2giran.com";
const char Button2Label[] = "Discord";
const char Button2Url[] = "https://discord.gg/uzWj8M4gcm";

const DWORD OP_HANDSHAKE = 0;
const DWORD OP_FRAME = 1;
const DWORD OP_CLOSE = 2;
const DWORD OP_PING = 3;
const DWORD OP_PONG = 4;

const DWORD RETRY_MS = 5000;          // Discord not running / connection lost
const DWORD REFUSED_MS = 60000;       // Discord answered CLOSE (unknown application id)

volatile LONG GStarted = 0;
HANDLE GPipe = INVALID_HANDLE_VALUE;
LONGLONG GStartTime = 0;              // unix seconds when the game started: the elapsed timer
DWORD GNonce = 0;

bool IsSet(const char* S)
{
    return S && S[0];
}

// bounded text (JSON) builder
struct FPresenceText
{
    char Buf[2048];
    INT Len;

    FPresenceText() : Len(0) { Buf[0] = 0; }

    void Char(char C)
    {
        if (Len < (INT)sizeof(Buf) - 1)
        {
            Buf[Len++] = C;
            Buf[Len] = 0;
        }
    }
    void Raw(const char* S)
    {
        while (*S)
            Char(*S++);
    }
    void Num(LONGLONG V)
    {
        char T[24];
        INT N = 0;
        const bool bNeg = V < 0;
        ULONGLONG U = bNeg ? (ULONGLONG)(-V) : (ULONGLONG)V;
        do
        {
            T[N++] = (char)('0' + U % 10);
            U /= 10;
        } while (U && N < (INT)sizeof(T));
        if (bNeg)
            Char('-');
        while (N)
            Char(T[--N]);
    }
    // a JSON string: quotes, backslashes and control characters escaped
    void Str(const char* S)
    {
        static const char Hex[] = "0123456789abcdef";
        Char('"');
        for (; *S; ++S)
        {
            const unsigned char C = (unsigned char)*S;
            if (C == '"' || C == '\\')
            {
                Char('\\');
                Char((char)C);
            }
            else if (C < 0x20)
            {
                Raw("\\u00");
                Char(Hex[C >> 4]);
                Char(Hex[C & 15]);
            }
            else
                Char((char)C);
        }
        Char('"');
    }
    // "key":"value" after a comma when not the first member
    void Member(const char* Key, const char* Value, bool& bFirst)
    {
        if (!IsSet(Value))
            return;
        if (!bFirst)
            Char(',');
        bFirst = false;
        Str(Key);
        Char(':');
        Str(Value);
    }
};

LONGLONG UnixNow()
{
    FILETIME Ft;
    GetSystemTimeAsFileTime(&Ft);
    const ULONGLONG Ticks = ((ULONGLONG)Ft.dwHighDateTime << 32) | Ft.dwLowDateTime;   // 100 ns since 1601
    return (LONGLONG)((Ticks - 116444736000000000ULL) / 10000000ULL);
}

bool Connect()
{
    char Name[32] = "\\\\.\\pipe\\discord-ipc-0";
    const INT Digit = lstrlenA(Name) - 1;
    for (char N = '0'; N <= '9'; ++N)
    {
        Name[Digit] = N;
        HANDLE Pipe = CreateFileA(Name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (Pipe != INVALID_HANDLE_VALUE)
        {
            GPipe = Pipe;
            return true;
        }
    }
    return false;
}

bool WriteAll(const void* Data, DWORD Size)
{
    const char* P = (const char*)Data;
    while (Size)
    {
        DWORD Done = 0;
        if (!WriteFile(GPipe, P, Size, &Done, NULL) || !Done)
            return false;
        P += Done;
        Size -= Done;
    }
    return true;
}

bool ReadAll(void* Data, DWORD Size)
{
    char* P = (char*)Data;
    while (Size)
    {
        DWORD Done = 0;
        if (!ReadFile(GPipe, P, Size, &Done, NULL) || !Done)
            return false;
        P += Done;
        Size -= Done;
    }
    return true;
}

bool WriteFrame(DWORD Op, const char* Payload, DWORD Len)
{
    const DWORD Header[2] = {Op, Len};
    return WriteAll(Header, sizeof(Header)) && (!Len || WriteAll(Payload, Len));
}

// one frame; the payload is kept up to OutSize - 1 bytes (NUL-terminated), the rest read and dropped
bool ReadFrame(DWORD& Op, char* Out, DWORD OutSize, DWORD& Len)
{
    DWORD Header[2];
    if (!ReadAll(Header, sizeof(Header)))
        return false;
    Op = Header[0];
    Len = Header[1];
    const DWORD Keep = Len < OutSize - 1 ? Len : OutSize - 1;
    if (Keep && !ReadAll(Out, Keep))
        return false;
    Out[Keep] = 0;
    for (DWORD Left = Len - Keep; Left;)
    {
        char Skip[256];
        const DWORD N = Left < sizeof(Skip) ? Left : (DWORD)sizeof(Skip);
        if (!ReadAll(Skip, N))
            return false;
        Left -= N;
    }
    if (Len > Keep)
        Len = Keep;
    return true;
}

bool SendActivity()
{
    FPresenceText T;
    T.Raw("{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":");
    T.Num(GetCurrentProcessId());
    T.Raw(",\"activity\":{");
    bool bFirst = true;
    T.Member("details", Details, bFirst);
    T.Member("state", State, bFirst);
    if (!bFirst)
        T.Char(',');
    T.Raw("\"timestamps\":{\"start\":");
    T.Num(GStartTime);
    T.Char('}');
    if (IsSet(LargeImage) || IsSet(LargeText) || IsSet(SmallImage) || IsSet(SmallText))
    {
        T.Raw(",\"assets\":{");
        bool bAsset = true;
        T.Member("large_image", LargeImage, bAsset);
        T.Member("large_text", LargeText, bAsset);
        T.Member("small_image", SmallImage, bAsset);
        T.Member("small_text", SmallText, bAsset);
        T.Char('}');
    }
    const char* Labels[2] = {Button1Label, Button2Label};
    const char* Urls[2] = {Button1Url, Button2Url};
    bool bButtons = false;
    for (INT i = 0; i < 2; ++i)
    {
        if (!IsSet(Labels[i]) || !IsSet(Urls[i]))
            continue;
        T.Raw(bButtons ? "," : ",\"buttons\":[");
        bButtons = true;
        T.Raw("{\"label\":");
        T.Str(Labels[i]);
        T.Raw(",\"url\":");
        T.Str(Urls[i]);
        T.Char('}');
    }
    if (bButtons)
        T.Char(']');
    T.Raw("}},\"nonce\":\"");
    T.Num(++GNonce);
    T.Raw("\"}");
    return WriteFrame(OP_FRAME, T.Buf, (DWORD)T.Len);
}

// one connection: handshake, activity, then pings until the pipe closes; returns the wait before
// the next try
DWORD Session()
{
    FPresenceText Hs;
    Hs.Raw("{\"v\":1,\"client_id\":");
    Hs.Str(AppId);
    Hs.Char('}');
    if (!WriteFrame(OP_HANDSHAKE, Hs.Buf, (DWORD)Hs.Len))
        return RETRY_MS;

    char In[1024];
    DWORD Op = 0, Len = 0;
    if (!ReadFrame(Op, In, sizeof(In), Len))
        return RETRY_MS;
    if (Op == OP_CLOSE)
    {
        Log("presence", "Discord refused the handshake: %s", In);
        return REFUSED_MS;
    }
    if (Op != OP_FRAME || !SendActivity())
        return RETRY_MS;
    Log("presence", "activity sent");

    while (ReadFrame(Op, In, sizeof(In), Len))
    {
        if (Op == OP_PING)
        {
            if (!WriteFrame(OP_PONG, In, Len))
                break;
        }
        else if (Op == OP_CLOSE)
            break;
        else if (Op == OP_FRAME && strstr(In, "\"evt\":\"ERROR\""))
            Log("presence", "Discord: %s", In);
    }
    return RETRY_MS;
}

DWORD WINAPI Worker(LPVOID)
{
    for (;;)
    {
        DWORD Wait = RETRY_MS;
        if (Connect())
        {
            Wait = Session();
            CloseHandle(GPipe);
            GPipe = INVALID_HANDLE_VALUE;
        }
        Sleep(Wait);
    }
}

// the game client, not the smoke / diff tools that load NWindow.dll too
bool InGameProcess()
{
    char Path[MAX_PATH];
    const DWORD Len = GetModuleFileNameA(NULL, Path, MAX_PATH);
    if (!Len || Len >= MAX_PATH)
        return false;
    const char* Name = strrchr(Path, '\\');
    Name = Name ? Name + 1 : Path;
    return lstrcmpiA(Name, "L2.exe") == 0;
}

}  // namespace

void DiscordPresence_Start()
{
    if (!InGameProcess() || !IniInt("Presence", "Enable", 1))
        return;
    if (InterlockedExchange(&GStarted, 1))
        return;
    GStartTime = UnixNow();
    HANDLE Thread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
    if (Thread)
        CloseHandle(Thread);
    Log("presence", Thread ? "thread started" : "CreateThread failed");
}

}  // namespace nwf

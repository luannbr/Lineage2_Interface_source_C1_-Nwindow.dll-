// presence_test.cpp - the Discord Rich Presence frames of src/features/DiscordPresence.cpp, outside
// the game and without Discord: the same source is included here, its pipe is a file, and
// presence_test.py reads the frames back (opcode, length, JSON) and checks them.
//
// Build + run: python tools/presence/presence_test.py   (MSVC x86 via vcvars32)
#define NW_FEATURES 1
#include "../../src/features/DiscordPresence.cpp"

#include <stdio.h>

// what the feature takes from NwFeatureConfig.cpp
namespace nwf {
void Log(const char*, const char*, ...) {}
INT IniInt(const char*, const char*, INT Default) { return Default; }
}  // namespace nwf

// A fake Discord on a pipe of our own name (the real one may hold discord-ipc-0): it reads the
// handshake, answers READY, reads SET_ACTIVITY, sends a PING and expects the PONG with the same
// payload, then closes. Session() must go through all of it and give up only when the pipe closes.
static char GFakeName[64];
static volatile LONG GFakeResult = -1;     // 1 = the whole exchange as Discord expects

static bool FakeRead(HANDLE P, DWORD& Op, char* Buf, DWORD Size)
{
    DWORD H[2], Got = 0;
    if (!ReadFile(P, H, 8, &Got, NULL) || Got != 8 || H[1] >= Size)
        return false;
    Op = H[0];
    if (H[1] && (!ReadFile(P, Buf, H[1], &Got, NULL) || Got != H[1]))
        return false;
    Buf[H[1]] = 0;
    return true;
}

static bool FakeWrite(HANDLE P, DWORD Op, const char* S)
{
    DWORD H[2] = {Op, (DWORD)lstrlenA(S)}, Put = 0;
    return WriteFile(P, H, 8, &Put, NULL) && WriteFile(P, S, H[1], &Put, NULL);
}

static DWORD WINAPI FakeDiscord(LPVOID Pipe)
{
    HANDLE P = (HANDLE)Pipe;
    char Buf[4096];
    DWORD Op = 99;
    LONG Ok = 0;
    if (ConnectNamedPipe(P, NULL) || GetLastError() == ERROR_PIPE_CONNECTED)
    {
        if (FakeRead(P, Op, Buf, sizeof(Buf)) && Op == 0 && strstr(Buf, "1154460536088567829") &&
            FakeWrite(P, 1, "{\"cmd\":\"DISPATCH\",\"data\":{\"v\":1},\"evt\":\"READY\",\"nonce\":null}") &&
            FakeRead(P, Op, Buf, sizeof(Buf)) && Op == 1 && strstr(Buf, "SET_ACTIVITY") &&
            FakeWrite(P, 3, "{\"ping\":42}") &&
            FakeRead(P, Op, Buf, sizeof(Buf)) && Op == 4 && !lstrcmpA(Buf, "{\"ping\":42}"))
            Ok = 1;
    }
    GFakeResult = Ok;
    DisconnectNamedPipe(P);
    CloseHandle(P);
    return 0;
}

static bool TestSession()
{
    using namespace nwf;
    wsprintfA(GFakeName, "\\\\.\\pipe\\presence-test-%lu", GetCurrentProcessId());
    HANDLE Server = CreateNamedPipeA(GFakeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                     1, 4096, 4096, 0, NULL);
    if (Server == INVALID_HANDLE_VALUE)
        return false;
    HANDLE Thread = CreateThread(NULL, 0, FakeDiscord, Server, 0, NULL);
    GPipe = CreateFileA(GFakeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (GPipe == INVALID_HANDLE_VALUE)
        return false;
    const DWORD Wait = Session();            // returns when the fake closes the pipe
    CloseHandle(GPipe);
    GPipe = INVALID_HANDLE_VALUE;
    WaitForSingleObject(Thread, 5000);
    CloseHandle(Thread);
    printf("session wait %lu fake %ld\n", Wait, GFakeResult);
    return Wait == RETRY_MS && GFakeResult == 1;
}

int main(int argc, char** argv)
{
    using namespace nwf;
    if (argc < 2)
        return 2;
    if (!TestSession())
        return 4;
    GPipe = CreateFileA(argv[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (GPipe == INVALID_HANDLE_VALUE)
        return 3;
    GStartTime = UnixNow();

    FPresenceText Hs;                       // the handshake Session() writes
    Hs.Raw("{\"v\":1,\"client_id\":");
    Hs.Str(AppId);
    Hs.Char('}');
    const bool bOk = WriteFrame(OP_HANDSHAKE, Hs.Buf, (DWORD)Hs.Len) && SendActivity();
    CloseHandle(GPipe);

    FPresenceText E;                        // escaping
    E.Str("a\"b\\c\nd\x01");
    printf("escape %s\n", E.Buf);
    printf("start %lld\n", (long long)GStartTime);
    printf("ingame %d\n", InGameProcess() ? 1 : 0);
    return bOk ? 0 : 1;
}

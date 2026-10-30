// nwd_smoke --l2-patches <System>: the runtime patches of the NWindowL2 build (phase 6,
// docs/notas/Fase6.md), outside the game (tools/smoke_standalone.py --l2 runs it).
//
// The features that stay runtime patches of other modules (WindowFix: four ILT thunks of
// WinDrv.dll; ReturnToLogin: two CClientSocket vtable slots of Network.dll and an ILT thunk of
// Engine.dll) install from our DllMain after checking the target bytes. In the game WinDrv.dll
// (and with it Window.dll, Network.dll, Engine.dll) is loaded long before NWindow.dll, so here
// they are loaded first, then NWindow.dll with plain LoadLibrary (its DllMain runs); each
// patched place is then read back: "hooked" when it leads into NWindow.dll, "stock" when it
// still leads where the client file has it. Nothing is called through the patches.
// The craft balloon (CraftBalloon) patches six sites of Engine.dll's FDynamicActor::Render on the
// console's first tick, which never comes outside the game: those sites are read too, against
// the bytes its install checks before it writes its jumps (or already ours). Its label pair
// carries an absolute address that only matches with Engine.dll at its preferred base; relocated
// (as in the game) the install leaves that pair stock, which is reported as such.
//
// The folder holds the client's engine DLLs, our NWindow.dll and a LineageWindow.ini with
// [Window] Log=1: LineageWindow.log is printed at the end (the features' install lines).
#include <windows.h>
#include <stdio.h>
#include <string.h>

bool SmokeCoreStandIns();   // nwd_smoke_standalone.cpp

namespace {

HMODULE GNwL2;

const char* Owner(const BYTE* Target)
{
    if (!Target)
        return "nothing";
    BYTE* Nw = (BYTE*)GNwL2;
    IMAGE_NT_HEADERS* Nt = (IMAGE_NT_HEADERS*)(Nw + ((IMAGE_DOS_HEADER*)Nw)->e_lfanew);
    if (Target >= Nw && Target < Nw + Nt->OptionalHeader.SizeOfImage)
        return "hooked (NWindow.dll)";
    return "stock";
}

// "E9 rel32" at Mod+ThunkRva -> its target, NULL when it is not a jump
const BYTE* IltTarget(HMODULE Mod, DWORD ThunkRva)
{
    const BYTE* Thunk = (const BYTE*)Mod + ThunkRva;
    if (Thunk[0] != 0xE9)
        return NULL;
    return Thunk + 5 + *(const INT*)(Thunk + 1);
}

void Thunk(const char* Name, HMODULE Mod, DWORD ThunkRva, DWORD BodyRva)
{
    const BYTE* Target = IltTarget(Mod, ThunkRva);
    const char* What = Owner(Target);
    if (Target && Target == (const BYTE*)Mod + BodyRva)
        What = "stock";
    else if (Target && !strcmp(What, "stock"))
        What = "ELSEWHERE";
    printf("  %s: %s\n", Name, What);
}

// A site CraftBalloon replaces with a 5-byte jmp: ours already, or the stock bytes its install
// checks (src/features/CraftBalloon.cpp: SigTypeA/B, SigBuyLabel, SigBuyColor). Relocated: the
// stock instruction with Engine.dll's relocated immediate - the label sites' check compares the
// immediate with its preferred-base value, so CraftBalloon (as NWindow_C1) leaves that group stock.
void BalloonSite(const char* Name, HMODULE Mod, DWORD Rva, const BYTE* Sig, int SigLen,
                 const BYTE* Relocated = NULL)
{
    const BYTE* P = (const BYTE*)Mod + Rva;
    const char* What;
    if (!memcmp(P, Sig, SigLen))
        What = "stock bytes, as CraftBalloon checks them";
    else if (Relocated && !memcmp(P, Relocated, SigLen))
        What = "stock, relocated immediate: CraftBalloon leaves this group stock";
    else if (P[0] == 0xE9)
        What = !strcmp(Owner(P + 5 + *(const INT*)(P + 1)), "hooked (NWindow.dll)") ? "hooked (NWindow.dll)"
                                                                                  : "ELSEWHERE";
    else
        What = "UNEXPECTED bytes";
    printf("  craft balloon %s: %s\n", Name, What);
}

void Slot(const char* Name, HMODULE Mod, DWORD VtableRva, DWORD Offset, DWORD BodyRva)
{
    const BYTE* Target = *(const BYTE* const*)((const BYTE*)Mod + VtableRva + Offset);
    const char* What = Owner(Target);
    if (Target == (const BYTE*)Mod + BodyRva)
        What = "stock";
    else if (!strcmp(What, "stock"))
        What = "ELSEWHERE";
    printf("  %s: %s\n", Name, What);
}

}  // namespace

int L2PatchesMain(int argc, wchar_t** argv)
{
    if (argc < 3)
    {
        fwprintf(stderr, L"usage: nwd_smoke --l2-patches <System folder>\n");
        return 2;
    }
    printf("System: %ls\n", argv[2]);
    if (!SetCurrentDirectoryW(argv[2]) || !SetDllDirectoryW(argv[2]))
    {
        printf("bad folder (%lu)\n", GetLastError());
        return 2;
    }
    // as in the game: Core first (L2.exe), WinDrv (with Window.dll, Network.dll, Engine.dll)
    // bound long before NWindow
    HMODULE Core = LoadLibraryW(L"Core.dll");
    // Network.dll's initialization needs what appInit sets up in the game (GMalloc...): the
    // standalone smoke's stand-ins
    if (Core)
        SmokeCoreStandIns();
    HMODULE WinDrv = Core ? LoadLibraryW(L"WinDrv.dll") : NULL;
    HMODULE Network = GetModuleHandleW(L"Network.dll");
    HMODULE Engine = GetModuleHandleW(L"Engine.dll");
    printf("  Core.dll %s, WinDrv.dll %s, Network.dll %s, Engine.dll %s\n", Core ? "loaded" : "MISSING",
           WinDrv ? "loaded" : "MISSING", Network ? "loaded" : "MISSING", Engine ? "loaded" : "MISSING");
    if (!Core || !WinDrv || !Network || !Engine)
    {
        printf("NOT RUN (%lu) - the engine DLLs one by one:\n", GetLastError());
        const wchar_t* Deps[] = {L"MSVCR70.dll", L"ogg.dll", L"vorbis.dll", L"vorbisfile.dll", L"IFC23.dll",
                                 L"Core.dll", L"Engine.dll", L"Window.dll", L"Network.dll", L"DINPUT8.dll",
                                 L"WinDrv.dll"};
        for (int i = 0; i < (int)(sizeof(Deps) / sizeof(Deps[0])); i++)
        {
            HMODULE h = LoadLibraryW(Deps[i]);
            printf("  %-16ls %s (%lu)\n", Deps[i], h ? "ok" : "FAILED", h ? 0 : GetLastError());
        }
        fflush(stdout);
        TerminateProcess(GetCurrentProcess(), 1);
    }
    GNwL2 = LoadLibraryW(L"NWindow.dll");
    printf("  NWindow.dll %s\n", GNwL2 ? "loaded (DllMain ran)" : "FAILED");
    if (!GNwL2)
    {
        fflush(stdout);
        TerminateProcess(GetCurrentProcess(), 1);
    }
    // the RVAs the features check (src/features/WindowFix.cpp, ReturnToLogin.cpp)
    Thunk("WinDrv ResizeViewport (ILT 0x100a)", WinDrv, 0x100A, 0x6570);
    Thunk("WinDrv OpenWindow (ILT 0x1136)", WinDrv, 0x1136, 0x4310);
    Thunk("WinDrv CauseInputEvent (ILT 0x11a9)", WinDrv, 0x11A9, 0x51F0);
    Thunk("WinDrv ToggleFullscreen (ILT 0x101e)", WinDrv, 0x101E, 0x6440);
    Slot("Network CClientSocket OnConnect (vtable 0x33d58 +0x64)", Network, 0x33D58, 0x64, 0x2320);
    Slot("Network CClientSocket OnUserNetmessage (vtable 0x33d58 +0x68)", Network, 0x33D58, 0x68, 0x10370);
    Thunk("Engine UGameEngine::OnServerClose (ILT 0x78f6)", Engine, 0x78F6, 0x11F3E0);
    // CraftBalloon's sites (patched on the first console tick). The label's immediate is the
    // stock FString "<Private Store - Buy>" at Engine.dll's preferred base (0x10300000 +
    // 0x67692C), as its install compares it; Engine.dll is relocated whenever something sits in
    // its 63 MB range - in the game always, L2.exe's own image (0x10900000, no relocations) does.
    static const BYTE TypeA[] = {0x66, 0x8B, 0x87, 0xF8, 0x01, 0x00, 0x00};   // mov ax, [edi+1F8h]
    static const BYTE TypeB[] = {0x66, 0x8B, 0xBF, 0xF8, 0x01, 0x00, 0x00};   // mov di, [edi+1F8h]
    static const BYTE Label[] = {0xB9, 0x2C, 0x69, 0x97, 0x10};               // mov ecx, 1097692Ch
    static const BYTE Color[] = {0x68, 0xA5, 0xFE, 0xFD, 0xFF};               // push 0FFFDFEA5h
    BYTE LabelHere[5] = {0xB9};                                               // mov ecx, <base+67692Ch>
    *(DWORD*)(LabelHere + 1) = (DWORD)(DWORD_PTR)Engine + 0x67692C;
    printf("  Engine.dll at %p (preferred 10300000)\n", (void*)Engine);
    BalloonSite("type-load A (Engine 0x27fb37)", Engine, 0x27FB37, TypeA, sizeof(TypeA));
    BalloonSite("type-load B (Engine 0x27fc47)", Engine, 0x27FC47, TypeB, sizeof(TypeB));
    BalloonSite("buy label A (Engine 0x27fb4a)", Engine, 0x27FB4A, Label, sizeof(Label), LabelHere);
    BalloonSite("buy label B (Engine 0x27fc5a)", Engine, 0x27FC5A, Label, sizeof(Label), LabelHere);
    BalloonSite("buy colour A (Engine 0x27fb9f)", Engine, 0x27FB9F, Color, sizeof(Color));
    BalloonSite("buy colour B (Engine 0x27fc72)", Engine, 0x27FC72, Color, sizeof(Color));
    printf("LineageWindow.log:\n");
    FILE* F = _wfopen(L"LineageWindow.log", L"rb");
    if (F)
    {
        char Line[1024];
        while (fgets(Line, sizeof(Line), F))
        {
            size_t N = strlen(Line);
            while (N && (Line[N - 1] == '\n' || Line[N - 1] == '\r'))
                Line[--N] = 0;
            printf("  | %s\n", Line);
        }
        fclose(F);
    }
    else
        printf("  (none)\n");
    printf("DONE\n");
    fflush(stdout);
    // no unloading: the engine DLLs are not meant to be freed outside the game
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}

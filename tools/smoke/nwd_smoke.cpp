// nwd_smoke - loads a System folder's NWindow.dll outside the game.
//
// The patched stock NWindow.dll imports NWindowDecomp.dll, so loading it runs our DllMain:
// stock image check + hook installation, reported in NWindowDecomp.log. Core.dll and
// Engine.dll load too (their static initializers only), nothing else of the engine runs.
//
//   nwd_smoke.exe "<...>\System - NWindow Teste"
//   nwd_smoke.exe --standalone <System> ...   a NWindow.dll without NWindowDecomp.dll, loaded
//                                             and registered the engine's way
//                                             (nwd_smoke_standalone.cpp)
//   nwd_smoke.exe --l2-patches <System>       the NWindowL2 build's runtime patches of WinDrv,
//                                             Network and Engine, read back after loading it
//                                             (nwd_smoke_l2.cpp)
#include <windows.h>
#include <stdio.h>

int StandaloneMain(int argc, wchar_t** argv);
int L2PatchesMain(int argc, wchar_t** argv);

static void Try(const wchar_t* name)
{
    HMODULE h = LoadLibraryW(name);
    printf("  %-20ls %s (%lu)\n", name, h ? "ok" : "FAILED", h ? 0 : GetLastError());
}

int wmain(int argc, wchar_t** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    // the DLLs read and the tests write their switches under this root, never under the owner's HKCU\Software\Lineage2C1\NWindow
    // (src/features/NwFeatureConfig.cpp: RegRootNow)
    // NWD_SMOKE_SUFFIX (optional): "..._<suffix>", so that runs from several worktrees at once keep to a root of their own
    {
        wchar_t Suffix[64] = {0};
        wchar_t Root[160] = L"Software\\Lineage2C1\\NWindowSmoke";
        if (GetEnvironmentVariableW(L"NWD_SMOKE_SUFFIX", Suffix, 64) && Suffix[0])
        {
            lstrcatW(Root, L"_");
            lstrcatW(Root, Suffix);
        }
        SetEnvironmentVariableW(L"NWD_INI_ROOT", Root);
    }
    if (argc >= 2 && !wcscmp(argv[1], L"--standalone"))
        return StandaloneMain(argc, argv);
    if (argc >= 2 && !wcscmp(argv[1], L"--l2-patches"))
        return L2PatchesMain(argc, argv);
    if (argc < 2)
    {
        fwprintf(stderr, L"usage: nwd_smoke <System folder>\n");
        return 2;
    }
    printf("System: %ls\n", argv[1]);
    if (!SetCurrentDirectoryW(argv[1]) || !SetDllDirectoryW(argv[1]))
    {
        printf("bad folder (%lu)\n", GetLastError());
        return 2;
    }
    // NWindowDecomp.dll must come from the System folder through NWindow.dll's import (loading
    // it first would run its DllMain before NWindow.dll exists). This exe is built into its own
    // folder so the application directory holds no other copy.
    HMODULE stock = LoadLibraryW(L"NWindow.dll");
    if (!stock)
    {
        printf("LoadLibrary(NWindow.dll) failed: %lu - dependencies one by one:\n", GetLastError());
        const wchar_t* deps[] = {L"MSVCR70.dll", L"ogg.dll", L"vorbis.dll", L"vorbisfile.dll", L"IFC23.dll",
                                 L"Core.dll", L"Engine.dll", L"NWindowDecomp.dll"};
        for (const wchar_t* d : deps)
            Try(d);
        return 1;
    }
    HMODULE decomp = GetModuleHandleW(L"NWindowDecomp.dll");
    printf("NWindow.dll at %p, NWindowDecomp.dll at %p\n", (void*)stock, (void*)decomp);

    FILE* f = _wfopen(L"NWindowDecomp.log", L"rb");
    if (f)
    {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
            fwrite(buf, 1, n, stdout);
        fclose(f);
    }
    else
        printf("no NWindowDecomp.log\n");
    fflush(stdout);
    // No FreeLibrary: unloading the engine DLLs outside the game is not supported.
    ExitProcess(decomp ? 0 : 1);
}

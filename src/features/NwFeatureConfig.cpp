// Phase 6: the settings and the log of the features (include/features/NwFeatures.h).
//
// No files next to NWindow.dll (the owner's request, 28/09/2026: "combatlog.ini ... LineageWindow.ini
// ... fique salvo dentro do dll, o usuario so vai alterar essas config dentro do jogo"):
// - what LineageWindow.ini used to carry is compiled in (GBuiltIn, the values the owner played with);
// - what the player changes in the game (window position, craft window positions, combat log
//   filters) is kept under HKCU\Software\Lineage2C1\NWindow\<Section>, value <Key> (REG_SZ) -- a loaded
//   DLL cannot rewrite itself, and the registry keeps no file in the System folder;
// - the log (LineageWindow.log) exists only in builds with NW_FEATURE_LOG=1.
// A read looks at the registry first, then GBuiltIn, then the caller's default.
#include "nwindow/NWindow.h"
#include "features/NwFeatures.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef NW_FEATURE_LOG
#define NW_FEATURE_LOG 0
#endif

namespace nwf {

namespace {

const WCHAR RegRootDefault[] = L"Software\\Lineage2C1\\NWindow";

// The registry root. The test harness sets NWD_INI_ROOT (a path under HKCU) so that it can write and read switches without ever
// touching the owner's real ones (an earlier harness run that died in the middle left the owner's switches changed); the game
// never has the variable.
const WCHAR* RegRootNow(WCHAR* Buffer, DWORD Chars)
{
    const DWORD N = GetEnvironmentVariableW(L"NWD_INI_ROOT", Buffer, Chars);
    return (N > 0 && N < Chars) ? Buffer : RegRootDefault;
}

struct FSetting
{
    const char* Section;
    const char* Key;
    const char* Value;
};

// LineageWindow.ini as the owner had it (System - NWindow Teste, 27/09/2026); keys not listed keep the
// default the feature passes (Connect, Hotkeys, Shots, BuffTimer: NWindow_C1's defaults)
const FSetting GBuiltIn[] = {
    {"Window", "Enable", "1"},
    {"Window", "Mode", "Auto"},
    {"Window", "FixUiMouse", "1"},
    {"Window", "FilterClicks", "1"},
    {"Window", "ConfineDrag", "1"},
    {"Window", "ConfineFullscreen", "1"},
    {"Window", "SafeMinimize", "1"},
    {"Window", "ConfirmClose", "1"},
    {"Craft", "TextureTest", "0"},
    {"Craft", "NativeUI", "1"},
};

// the ASCII section / key / value texts the features pass, as the wide registry API wants them
void Widen(const char* In, WCHAR* Out, INT OutCount)
{
    if (!MultiByteToWideChar(CP_ACP, 0, In ? In : "", -1, Out, OutCount))
        Out[0] = 0;
    Out[OutCount - 1] = 0;
}

// HKCU\Software\Lineage2C1\NWindow\<Section> value <Key>, as text; false when absent
bool RegRead(const char* Section, const char* Key, char* Out, DWORD OutSize)
{
    WCHAR Sub[160], K[64], Value[512];
    WCHAR S[64];
    Widen(Section, S, 64);
    Widen(Key, K, 64);
    WCHAR RootBuf[160];
    wsprintfW(Sub, L"%s\\%s", RegRootNow(RootBuf, 160), S);
    HKEY Handle;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, Sub, 0, KEY_QUERY_VALUE, &Handle) != ERROR_SUCCESS)
        return false;
    DWORD Type = 0, Bytes = sizeof(Value) - sizeof(WCHAR);
    const LONG Result = RegQueryValueExW(Handle, K, NULL, &Type, (BYTE*)Value, &Bytes);
    RegCloseKey(Handle);
    if (Result != ERROR_SUCCESS || Type != REG_SZ)
        return false;
    Value[Bytes / sizeof(WCHAR)] = 0;
    if (!WideCharToMultiByte(CP_ACP, 0, Value, -1, Out, (INT)OutSize, NULL, NULL))
        Out[0] = 0;
    Out[OutSize - 1] = 0;
    return true;
}

const char* BuiltIn(const char* Section, const char* Key)
{
    for (INT i = 0; i < (INT)(sizeof(GBuiltIn) / sizeof(GBuiltIn[0])); i++)
        if (!lstrcmpiA(GBuiltIn[i].Section, Section) && !lstrcmpiA(GBuiltIn[i].Key, Key))
            return GBuiltIn[i].Value;
    return NULL;
}

// GetPrivateProfileInt's reading of a text: optional sign, then digits; anything else is Default
INT ParseInt(const char* Text, INT Default)
{
    const char* P = Text;
    while (*P == ' ' || *P == '\t')
        ++P;
    const bool bNeg = (*P == '-');
    if (*P == '-' || *P == '+')
        ++P;
    if (*P < '0' || *P > '9')
        return Default;
    INT Value = 0;
    for (; *P >= '0' && *P <= '9'; ++P)
        Value = Value * 10 + (*P - '0');
    return bNeg ? -Value : Value;
}

}  // namespace

// <folder of our NWindow.dll>\<Name>
void ModuleSibling(const char* Name, char* Out, DWORD OutSize)
{
    Out[0] = 0;
    DWORD Len = GetModuleFileNameA(GNWindowInstance, Out, OutSize);
    char* Slash = Len ? strrchr(Out, '\\') : NULL;
    if (Slash && (DWORD)(Slash + 1 - Out) + lstrlenA(Name) < OutSize)
        lstrcpyA(Slash + 1, Name);
    else
        lstrcpynA(Out, Name, OutSize);
}

INT IniInt(const char* Section, const char* Key, INT Default)
{
    char Text[64];
    if (RegRead(Section, Key, Text, sizeof(Text)))
        return ParseInt(Text, Default);
    if (const char* Built = BuiltIn(Section, Key))
        return ParseInt(Built, Default);
    return Default;
}

void IniString(const char* Section, const char* Key, const char* Default, char* Out, DWORD OutSize)
{
    if (!OutSize)
        return;
    if (RegRead(Section, Key, Out, OutSize))
        return;
    const char* Built = BuiltIn(Section, Key);
    lstrcpynA(Out, Built ? Built : (Default ? Default : ""), (INT)OutSize);
}

void IniWriteInt(const char* Section, const char* Key, INT Value)
{
    char Text[16];
    wsprintfA(Text, "%d", Value);
    IniWriteString(Section, Key, Text);
}

void IniWriteString(const char* Section, const char* Key, const char* Value)
{
    WCHAR S[64], K[64], V[512], Sub[160];
    Widen(Section, S, 64);
    Widen(Key, K, 64);
    Widen(Value, V, 512);
    WCHAR RootBuf[160];
    wsprintfW(Sub, L"%s\\%s", RegRootNow(RootBuf, 160), S);
    HKEY Handle;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, Sub, 0, NULL, 0, KEY_SET_VALUE, NULL, &Handle, NULL) != ERROR_SUCCESS)
        return;
    RegSetValueExW(Handle, K, 0, REG_SZ, (const BYTE*)V, (DWORD)((lstrlenW(V) + 1) * sizeof(WCHAR)));
    RegCloseKey(Handle);
}

bool LogEnabled()
{
    return NW_FEATURE_LOG != 0;
}

void Log(const char* Tag, const char* Fmt, ...)
{
#if NW_FEATURE_LOG
    char Line[1024];
    INT Len = _snprintf_s(Line, sizeof(Line), _TRUNCATE, "%s: ", Tag);
    if (Len < 0)
        Len = 0;
    va_list Args;
    va_start(Args, Fmt);
    INT More = _vsnprintf_s(Line + Len, sizeof(Line) - Len, _TRUNCATE, Fmt, Args);
    va_end(Args);
    Len = More < 0 ? lstrlenA(Line) : Len + More;

    char Path[MAX_PATH];
    ModuleSibling("LineageWindow.log", Path, MAX_PATH);
    HANDLE File = CreateFileA(Path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
        return;
    DWORD Written = 0;
    WriteFile(File, Line, (DWORD)Len, &Written, NULL);
    WriteFile(File, "\r\n", 2, &Written, NULL);
    CloseHandle(File);
#else
    (void)Tag;
    (void)Fmt;
#endif
}

}  // namespace nwf

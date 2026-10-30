/*=============================================================================
    NCChineseIME.cpp - IME of the Chinese client. Stock: 0x1000ef80 .. 0x1000f087,
    0x1000f090 .. 0x1000f0b4, 0x1000f0c0 .. 0x1000f2ad, 0x1000f390 .. 0x1000f3fb,
    0x1000fbf0 .. 0x10010206, 0x10013300 .. 0x100134be, 0x100134c0 .. 0x10014637,
    0x10014b60 .. 0x1001522e (the window procedure and FreeCandList are the Japanese IME's
    bodies).
=============================================================================*/
#include "nwindow/NCChineseIME.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCCompositeBox.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NConsoleWnd.h"

#include <stdlib.h>

// a child window goes (repeated inline in the stock OnDestroys)
template <class T> static FORCEINLINE void DestroyChild(NCWnd* Parent, T*& Child)
{
    if (Child)
    {
        Parent->RemoveChildWindow(Child->Entry);
        delete Child;
        Child = NULL;
    }
}

// the IME versions GetImeId knows (names ours)
enum
{
    IMEID_CHS_VER41 = 0x40001,         // Microsoft Pinyin 1.5
    IMEID_CHS_VER42 = 0x40002,         // Microsoft Pinyin 2
    IMEID_CHS_VER53 = 0x50003,         // Microsoft Pinyin 3
};

// 0x10014b60 - CandList and the boxes are left as allocated
NCChineseIME::NCChineseIME(INT InMode)
    : NCIME(0)
{
    Mode = InMode;
}
NW_HOOK_CTOR_ARGS(0x10014b60, NCChineseIME, (INT A), (A))

// 0x10014c10
NW_HOOK_DTOR(0x10014c10, NCChineseIME)

// 0x1000f360 - the class's operator new (NW_DECLARE_CLASS), out of line for the manager's
// OnCreate
static void* NewChineseIME(size_t Size, UObject* Outer, FName Name, DWORD SetFlags)
{
    return NCChineseIME::operator new(Size, Outer, Name, SetFlags);
}
NW_HOOK_FUNC(0x1000f360, NewChineseIME)

// stock body shared with the Japanese IME's (0x1000ee70)
INT NCChineseIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCIME::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_IME_NOTIFY)
        return OnImeNotify(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITION)
        return OnImeComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_ENDCOMPOSITION)
        return OnImeEndComposition(wParam, lParam) != 0;
    if (Msg == WM_IME_SETCONTEXT)
        return OnImeSetContext(wParam, lParam) != 0;
    if (Msg == WM_INPUTLANGCHANGE)
        return OnInputLangChange(wParam, lParam) != 0;
    if (Msg == NIM_CARET)
        return OnCaret((INT)wParam, (INT)lParam) != 0;
    return 0;
}

// 0x1000ef80 - GChineseGetReadingString from the current IME's file, NULL without it (the
// library is not freed)
void NCChineseIME::LoadReadingString()
{
    ANSICHAR FileName[MAX_PATH];
    HKL Layout = GetKeyboardLayout(0);
    GChineseGetReadingString = NULL;
    if (ImmGetIMEFileNameA(Layout, FileName, MAX_PATH) > 0)
    {
        HMODULE IME = LoadLibraryA(FileName);
        if (IME)
            GChineseGetReadingString = (NGetReadingString)GetProcAddress(IME, "GetReadingString");
    }
}
NW_HOOK_FUNC(0x1000ef80, NCChineseIME::LoadReadingString)

// 0x1000efe0 - a Chinese layout with a Simplified Chinese IME: layouts E00E, E001, E004 and
// E025, or the files PINTLGNT, winpy, winabc and PWBX32 (this is not used)
UBOOL NCChineseIME::IsChineseIME()
{
    ANSICHAR FileName[260];
    HKL Layout = GetKeyboardLayout(0);
    ImmGetIMEFileNameA(Layout, FileName, 259);
    if (((UINT_PTR)Layout & 0x3FF) != LANG_CHINESE)
        return 0;
    WORD Device = HIWORD((UINT_PTR)Layout);
    if (Device == 0xE00E || Device == 0xE001 || Device == 0xE004 || Device == 0xE025
        || !lstrcmpiA(FileName, "PINTLGNT.IME") || !lstrcmpiA(FileName, "winpy.ime")
        || !lstrcmpiA(FileName, "winabc.ime") || !lstrcmpiA(FileName, "PWBX32.IME"))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1000efe0, NCChineseIME, IsChineseIME)

// stock body shared with the Japanese IME's (0x1000e6a0)
void NCChineseIME::FreeCandList()
{
    if (CandList)
    {
        CANDIDATELIST* List = CandList;
        CandList = NULL;
        appFree(List);
    }
}

// 0x1000f090 (vslot 42) - both boxes hidden
void NCChineseIME::ShowWindow()
{
    NCWnd::ShowWindow();
    CandidateBox->HideWindow();
    CompositeBox->HideWindow();
}
NW_HOOK_METHOD(0x1000f090, NCChineseIME, ShowWindow)

// 0x1000f0c0 - the version of the old Microsoft Pinyin IMEs, which keep the reading string in
// their private data: their layout (E00E), their file (unless the IME has GetReadingString,
// PINTLGNT or MSSCIPYA), a known version for a Chinese layout (any version with
// GetReadingString); 0 otherwise
DWORD NCChineseIME::GetImeId()
{
    ANSICHAR FileName[MAX_PATH];
    HKL Layout = GetKeyboardLayout(0);
    DWORD Lang = (DWORD)(UINT_PTR)Layout & 0x3FF;
    if ((UINT_PTR)Layout != 0xE00E0804 || ImmGetIMEFileNameA(Layout, FileName, MAX_PATH - 1) <= 0)
        return 0;
    if (!GChineseGetReadingString && lstrcmpiA(FileName, "PINTLGNT.IME")
        && lstrcmpiA(FileName, "MSSCIPYA.IME"))
        return 0;
    DWORD Handle;
    DWORD Size = GetFileVersionInfoSizeA(FileName, &Handle);
    if (!Size)
        return 0;
    void* Buffer = malloc(Size);
    if (Buffer && GetFileVersionInfoA(FileName, Handle, Size, Buffer))
    {
        VS_FIXEDFILEINFO* Info;
        UINT InfoSize;
        if (VerQueryValueA(Buffer, "\\", (LPVOID*)&Info, &InfoSize))
        {
            DWORD Version = Info->dwFileVersionMS & 0xFF00FF;
            if (GChineseGetReadingString
                || (Lang == LANG_CHINESE
                    && (Version == IMEID_CHS_VER41 || Version == IMEID_CHS_VER42
                        || Version == IMEID_CHS_VER53)))
            {
                free(Buffer);
                return Version;
            }
        }
    }
    free(Buffer);
    return 0;
}
NW_HOOK_FUNC(0x1000f0c0, NCChineseIME::GetImeId)

// 0x1000f1f0 (vslot 67) - the IME types 11 and 12 also empty the composition window and hide
// the candidates
INT NCChineseIME::OnImeEndComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 1;
    if (GConsole->GetIMEType() == 11 || GConsole->GetIMEType() == 12)
    {
        FString Text;
        Text.Empty();
        CompositeBox->SetText(*Text);
        CandidateBox->HideWindow();
    }
    SendToParent(NIM_ENDCOMPOSITION, 0, 0);
    return 1;
}
NW_HOOK_METHOD(0x1000f1f0, NCChineseIME, OnImeEndComposition)

// 0x1000f390 - the system is NT (the version is read once)
bool NCChineseIME::IsNT()
{
    if (!GChineseNTChecked)
    {
        if (!GChineseOSVersionRead)
        {
            GChineseOSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
#pragma warning(suppress : 4996)  // deprecated in current SDKs; the stock code calls it
            GetVersionExA(&GChineseOSVersion);
            GChineseOSVersionRead = true;
        }
        OSVERSIONINFOA Info = GChineseOSVersion;
        if (Info.dwPlatformId == VER_PLATFORM_WIN32_NT)
            GChineseNT = true;
        GChineseNTChecked = true;
    }
    return GChineseNT;
}
NW_HOOK_FUNC(0x1000f390, NCChineseIME::IsNT)

// 0x1000fbf0 (vslot 35) - the candidate box and the composition window, hidden, then the
// reading string API, imm32's input context locks and the code page
INT NCChineseIME::OnCreate()
{
    NCIME::OnCreate();
    CandidateBox = new NCCandidateBox(9, 15);
    CandidateBox->SetWindowRect(0, 0, 100, 135);
    CreateChild(CandidateBox, NWS_VISIBLE);
    CandidateBox->HideWindow();
    LastComp.Empty();
    bCandOpened = 0;
    bOpenResultSent = 0;
    CompositeBox = new NCCompositeBox(100, 15, 0, 0, 0);
    CompositeBox->SetWindowRect(0, 0, 100, 15);
    CreateChild(CompositeBox, NWS_VISIBLE);
    CompositeBox->HideWindow();
    LoadReadingString();
    LoadImmFunctions();
    UpdateCodePage();
    return 1;
}
NW_HOOK_METHOD(0x1000fbf0, NCChineseIME, OnCreate)

// 0x1000fd60 - the Taiwanese IME's (see there) with the Pinyin IMEs' private data: 1.5 and 3 in
// Unicode, 2 in Unicode on NT only (else ANSI, with twice as much room before the lengths)
void NCChineseIME::ReadReadingString()
{
    DWORD Id = GetImeId();
    if (!Id || !hIMC)
        return;
    WCHAR Local[262];
    INT Err = 0;
    bool bUnicode = false;
    UINT Len = 0;
    WCHAR* Str = NULL;
    if (GChineseGetReadingString)
    {
        BOOL bVertical;
        UINT MaxUiLen;
        Len = GChineseGetReadingString(hIMC, 0, NULL, &Err, &bVertical, &MaxUiLen);
        if (Len)
        {
            Str = (WCHAR*)malloc(Len);
            if (!Str)
            {
                ImeReleaseContext(hIMC);
                return;
            }
            Len = GChineseGetReadingString(hIMC, Len, Str, &Err, &bVertical, &MaxUiLen);
        }
        bUnicode = true;
    }
    else
    {
        BYTE* IC = GImmLockIMC(hIMC);
        BYTE* p;
        switch (Id)
        {
        case IMEID_CHS_VER41:
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 0x20);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x9C);
            Err = *(DWORD*)(p + 0xA0);
            if ((UINT)Err >= Len)
                Err = Len;
            Str = (WCHAR*)(p + 0x38);
            bUnicode = true;
            break;
        case IMEID_CHS_VER42:
        {
            INT CharSize = IsNT() ? 2 : 1;
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 0x20);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x40 + 16 * CharSize);
            Err = *(DWORD*)(p + 0x44 + 16 * CharSize);
            bUnicode = IsNT();
            if (bUnicode)
                Str = (WCHAR*)(p + 0x40);
            else
                MultiByteToWideChar(CodePage, 0, (ANSICHAR*)(p + 0x40), -1, Local, Len);
            break;
        }
        case IMEID_CHS_VER53:
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 4);
            if (!p)
                break;
            p = *(BYTE**)(p + 0x18);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x60);
            Err = *(DWORD*)(p + 0x64);
            Str = (WCHAR*)(p + 0x40);
            bUnicode = true;
            break;
        }
        GImmUnlockIMCC(IMCPrivate(IC));
        GImmUnlockIMC(hIMC);
    }
    if (Len > 0)
    {
        ReadingStr[Len] = 0;
        for (INT i = 0; i < (INT)Len; i++)
            ReadingStr[i] = bUnicode ? Str[i] : Local[i];
        SendToParent(NIM_READING, Len, Err);
    }
}
NW_HOOK_METHOD(0x1000fd60, NCChineseIME, ReadReadingString)

// 0x1000ffa0 - the Japanese IME's (see there) with 10 candidates a page for the IME type 9
// (the box 150 px high then)
void NCChineseIME::FillCandidateBox()
{
    SIZE Size;
    SIZE ItemSize;
    CandidateBox->Clear();
    INT PageSize = GConsole->GetIMEType() == 9 ? 10 : 9;
    DWORD i = CandList->dwPageStart;
    TCHAR* Text = (TCHAR*)((BYTE*)CandList + CandList->dwOffset[i]);
    CandidateBox->AddItem(Text, -1);
    if (i == CandList->dwSelection)
        CandidateBox->Selected = i % PageSize;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    for (i = CandList->dwPageStart + 1; i < CandList->dwCount; i++)
    {
        if (i >= CandList->dwPageSize + CandList->dwPageStart)
            break;
        Text = (TCHAR*)((BYTE*)CandList + CandList->dwOffset[i]);
        CandidateBox->AddItem(Text, -1);
        NCGDevice::GetTextExtent(&ItemSize, Text, 0, 0, 0, 0, 0);
        if (i == CandList->dwSelection)
            CandidateBox->Selected = i % PageSize;
        if (ItemSize.cx > Size.cx)
            Size.cx = ItemSize.cx;
    }
    CandidateBox->SetScroll(CandList->dwCount, CandList->dwSelection);
    CandidateBox->PageSize = CandList->dwPageSize;
    CandidateBox->SetSize(Size.cx + 20, PageSize * 15);
    SendToParent(NIM_GETCARET, (WPARAM)Entry, 0);
}
NW_HOOK_METHOD(0x1000ffa0, NCChineseIME, FillCandidateBox)

// 0x10010130 (vslot 74) - moved to the caret when hidden, then laid out by the IME type (9 and
// 12: layout 5, 10 a page, 100x150; else 0, 9 a page, 100x135) and shown. The SetWindowRect
// puts the box back at the IME's origin, as in the stock code, so the caret is not followed.
INT NCChineseIME::OnCaret(INT X, INT Y)
{
    if (!CandidateBox->IsShowWindow())
    {
        CandidateBox->MoveWindow(-CandidateBox->LocalX, -CandidateBox->LocalY);
        CandidateBox->MoveWindow(X, Y);
    }
    if (GConsole->GetIMEType() == 9 || GConsole->GetIMEType() == 12)
    {
        CandidateBox->SetMode(5);
        CandidateBox->PageSize = 10;
        CandidateBox->SetWindowRect(0, 0, 100, 150);
    }
    else
    {
        CandidateBox->SetMode(0);
        CandidateBox->PageSize = 9;
        CandidateBox->SetWindowRect(0, 0, 100, 135);
    }
    CandidateBox->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x10010130, NCChineseIME, OnCaret)

// 0x10013300 (vslot 36) - NCWnd::OnDestroy is not called
INT NCChineseIME::OnDestroy()
{
    Clauses.Empty();
    if (CandList)
        appFree(CandList);
    CandList = NULL;
    ImmDestroyContext(hIMC);
    DestroyChild(this, CandidateBox);
    DestroyChild(this, CompositeBox);
    return 1;
}
NW_HOOK_METHOD(0x10013300, NCChineseIME, OnDestroy)

// 0x100133b0 - the Japanese IME's (see there) with the IME type's page (10 for the type 9),
// the page start always sent to the IME and kept in CandList for the type 10; a selection past
// the list stops early, without releasing the context
void NCChineseIME::ReadCandList()
{
    DWORD Size = ImmGetCandidateListW(hIMC, 0, NULL, 0);
    FreeCandList();
    if (Size)
    {
        CandList = (CANDIDATELIST*)appMalloc(Size, TEXT("new"));
        CandList->dwSize = Size;
        ImmGetCandidateListW(hIMC, 0, CandList, Size);
        if (CandList->dwSelection >= CandList->dwCount)
            return;
        DWORD PageStart = 0;
        DWORD Step;
        if (GConsole->GetIMEType() == 9)
            Step = CandList->dwPageSize == 10 || !CandList->dwPageSize ? CandList->dwPageSize : 10;
        else
            Step = CandList->dwPageSize == 9 || !CandList->dwPageSize ? CandList->dwPageSize : 9;
        if (CandList->dwSelection > 0 && CandList->dwPageSize && Step <= CandList->dwSelection)
        {
            do
                PageStart += Step;
            while (Step + PageStart <= CandList->dwSelection);
        }
        if (GConsole->GetIMEType() == 10)
            CandList->dwPageStart = PageStart;
        ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, 0, PageStart);
        FillCandidateBox();
    }
    ImmReleaseContext(GViewportHWnd, hIMC);
}
NW_HOOK_METHOD(0x100133b0, NCChineseIME, ReadCandList)

// 0x100134c0 (vslot 69) - NCJapaneseIME::OnImeComposition's copy (see there), with its own
// guard names, nothing read unless the IME is Chinese (GetImeId is called first, its result
// unused), and for the IME type 10: LastComp keeps the composition string before each change
// and the clause data is not read. The clauses are rebuilt without telling the parent anything
// more.
INT NCChineseIME::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
    guard(NCChineseIME::OnImeComposition);
    TCHAR Clause[1024];
    NImeText Item;
    WCHAR Wide[0x1000];
    ANSICHAR Ansi[0x801];
    UBOOL bClauses = 0;
    INT Bytes;
    UINT Len = 0;
    ImeIsUIMessage(WM_IME_SETCONTEXT, 1, 0);
    ImeIsUIMessage(WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
    GetImeId();
    // the CJK clients with another IME: nothing (and the context is not released)
    if ((GLanguageType == 2 || GLanguageType == 3 || GLanguageType == 4) && !IsChineseIME())
        return 1;

    if (lParam & GCS_RESULTSTR)
    {
        if (lParam & GCS_RESULTCLAUSE)
        {
            if (IsAnsiIME())
            {
                guard(NCChineseIME::OnImeComposition::GCS_RESULTCLAUSE::NotUnicode);
                Bytes = ImmGetCompositionStringW(hIMC, GCS_RESULTCLAUSE, Ansi, 0x800);
                Ansi[Bytes] = 0;
                Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
                if (Len)
                    Len = Len * 2 - 2;
                ResultClauseLen = Len >> 1;
                appMemset(ResultClause, 0, sizeof(ResultClause));
                appMemcpy(ResultClause, Wide, ResultClauseLen);
                ResultClause[ResultClauseLen] = 0;
                unguard;
            }
            else
            {
                guard(NCChineseIME::OnImeComposition::GCS_RESULTCLAUSE::Unicode);
                appMemset(ResultClause, 0, sizeof(ResultClause));
                ResultClauseLen =
                    ImmGetCompositionStringW(hIMC, GCS_RESULTCLAUSE, ResultClause, sizeof(ResultClause));
                unguard;
            }
        }
        else
        {
            appMemset(ResultClause, 0, sizeof(ResultClause));
            ResultClauseLen = 0;
        }
        if (IsAnsiIME())
        {
            guard(NCChineseIME::OnImeComposition::GCS_RESULTSTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            ResultStrLen = Len >> 1;
            appMemset(ResultStr, 0, sizeof(ResultStr));
            ResultStr[0] = 0;
            appStrcpy(ResultStr, Wide);
            ResultStr[ResultStrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCChineseIME::OnImeComposition::GCS_RESULTSTR::Unicode);
            appMemset(ResultStr, 0, sizeof(ResultStr));
            ResultStr[0] = 0;
            ResultStrLen =
                (DWORD)ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, ResultStr, sizeof(ResultStr)) >> 1;
            ResultStr[ResultStrLen] = 0;
            unguard;
        }
        Clauses.Empty();
        SendToParent(NIM_RESULT, 0, 0);
        SendToParent(NIM_ATTRIBUTE, 1, 1);
    }
    else
    {
        ResultStrLen = 0;
        appMemset(ResultStr, 0, sizeof(ResultStr));
        ResultStr[0] = 0;
        appMemset(ResultClause, 0, sizeof(ResultClause));
        ResultClauseLen = 0;
    }

    if (lParam & GCS_COMPSTR)
    {
        if (IsAnsiIME())
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPSTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            CompStrLen = Len >> 1;
            if (GConsole->GetIMEType() == 10)
                LastComp = FString(CompStr);
            appMemset(CompStr, 0, sizeof(CompStr));
            CompStr[0] = 0;
            appStrcpy(CompStr, Wide);
            CompStr[CompStrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPSTR::Unicode);
            if (GConsole->GetIMEType() == 10)
                LastComp = FString(CompStr);
            appMemset(CompStr, 0, sizeof(CompStr));
            CompStr[0] = 0;
            CompStrLen = (DWORD)ImmGetCompositionStringW(hIMC, GCS_COMPSTR, CompStr, sizeof(CompStr)) >> 1;
            CompStr[CompStrLen] = 0;
            unguard;
        }
        // the character being composed (read before the string when it is empty, as in the
        // stock code)
        SendToParent(NIM_COMPOSITION, CompStr[CompStrLen - 1], 0);
    }
    else
    {
        CompStrLen = 0;
        appMemset(CompStr, 0, sizeof(CompStr));
        CompStr[0] = 0;
        SendToParent(NIM_COMPOSITION, 0, 0);
        if (GConsole->GetIMEType() == 10)
            LastComp = FString(CompStr);
    }

    if (lParam & GCS_COMPATTR)
    {
        if (IsAnsiIME())
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPATTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            CompAttrLen = Len >> 1;
            appMemset(CompAttr, 0, sizeof(CompAttr));
            appMemcpy(CompAttr, Wide, CompAttrLen);
            CompAttr[CompAttrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPATTR::Unicode);
            appMemset(CompAttr, 0, sizeof(CompAttr));
            CompAttrLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, CompAttr, sizeof(CompAttr));
            unguard;
        }
        if (!CompAttr[0])
        {
            SendToParent(NIM_ATTRIBUTE, 0, 0);
            Clauses.Empty();
            bClauses = 0;
        }
        else
            bClauses = 1;
    }
    else
    {
        appMemset(CompAttr, 0, sizeof(CompAttr));
        CompAttrLen = 0;
    }

    if (GConsole->GetIMEType() != 10)
    {
        if (lParam & GCS_COMPCLAUSE)
        {
            if (IsAnsiIME())
            {
                guard(NCChineseIME::OnImeComposition::GCS_COMPCLAUSE::NotUnicode);
                Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPCLAUSE, Ansi, 0x800);
                Ansi[Bytes] = 0;
                Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
                if (Len)
                    Len = Len * 2 - 2;
                CompClauseLen = Len >> 1;
                appMemset(CompClause, 0, sizeof(CompClause));
                appMemcpy(CompClause, Wide, CompClauseLen);
                CompClause[CompClauseLen] = 0;
                unguard;
            }
            else
            {
                guard(NCChineseIME::OnImeComposition::GCS_COMPCLAUSE::Unicode);
                appMemset(CompClause, 0, sizeof(CompClause));
                CompClauseLen =
                    (DWORD)ImmGetCompositionStringW(hIMC, GCS_COMPCLAUSE, CompClause, sizeof(CompClause)) >> 2;
                unguard;
            }
        }
        else
        {
            appMemset(CompClause, 0, sizeof(CompClause));
            CompClauseLen = 0;
        }
    }

    if (lParam & GCS_CURSORPOS)
    {
        UBOOL bTyping = 0;
        if (IsAnsiIME())
        {
            for (INT i = 0; i < CompAttrLen; i++)
                if (CompAttr[i] == ATTR_INPUT)
                    bTyping = 1;
            if (bTyping)
                CursorPos = Len >> 1;
            else
                CursorPos = CompStrLen;
        }
        else
        {
            for (INT i = 0; i < CompAttrLen; i++)
                if (CompAttr[i] == ATTR_INPUT)
                    bTyping = 1;
            if (bTyping)
                CursorPos = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0);
            else
                CursorPos = CompStrLen;
        }
        SendToParent(NIM_CURSORPOS, CursorPos, 0);
    }

    if (lParam & GCS_COMPREADSTR)
    {
        if (IsAnsiIME())
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPREADSTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPREADSTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            CompReadStrLen = Len >> 1;
            appMemset(CompReadStr, 0, sizeof(CompReadStr));
            CompReadStr[0] = 0;
            appStrcpy(CompReadStr, Wide);
            CompReadStr[CompReadStrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPREADSTR::Unicode);
            appMemset(CompReadStr, 0, sizeof(CompReadStr));
            CompReadStr[0] = 0;
            Len = ImmGetCompositionStringW(hIMC, GCS_COMPREADSTR, CompReadStr, sizeof(CompReadStr));
            CompReadStrLen = Len >> 1;
            CompReadStr[Len >> 1] = 0;
            unguard;
        }
    }
    else
    {
        CompReadStrLen = 0;
        appMemset(CompReadStr, 0, sizeof(CompReadStr));
        CompReadStr[0] = 0;
    }

    if (GConsole->GetIMEType() != 10)
    {
        if (lParam & GCS_COMPREADCLAUSE)
        {
            if (IsAnsiIME())
            {
                guard(NCChineseIME::OnImeComposition::GCS_COMPREADCLAUSE::NotUnicode);
                Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPREADCLAUSE, Ansi, 0x800);
                Ansi[Bytes] = 0;
                Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
                if (Len)
                    Len = Len * 2 - 2;
                CompReadClauseLen = Len >> 1;
                appMemset(CompReadClause, 0, sizeof(CompReadClause));
                appMemcpy(CompReadClause, Wide, CompReadClauseLen);
                CompReadClause[CompReadClauseLen] = 0;
                unguard;
            }
            else
            {
                guard(NCChineseIME::OnImeComposition::GCS_COMPREADCLAUSE::Unicode);
                appMemset(CompReadClause, 0, sizeof(CompReadClause));
                CompReadClauseLen = ImmGetCompositionStringW(hIMC, GCS_COMPREADCLAUSE, CompReadClause,
                                                             sizeof(CompReadClause));
                unguard;
            }
        }
        else
        {
            CompReadClauseLen = 0;
            appMemset(CompReadClause, 0, sizeof(CompReadClause));
        }
    }

    if (lParam & GCS_COMPREADATTR)
    {
        if (IsAnsiIME())
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPREADATTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPREADATTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            CompReadAttrLen = Len >> 1;
            appMemset(CompReadAttr, 0, sizeof(CompReadAttr));
            appMemcpy(CompReadAttr, Wide, CompReadAttrLen);
            CompReadAttr[CompReadAttrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCChineseIME::OnImeComposition::GCS_COMPREADATTR::Unicode);
            appMemset(CompReadAttr, 0, sizeof(CompReadAttr));
            CompReadAttrLen =
                ImmGetCompositionStringW(hIMC, GCS_COMPREADATTR, CompReadAttr, sizeof(CompReadAttr));
            unguard;
        }
    }
    else
    {
        CompReadAttrLen = 0;
        appMemset(CompReadAttr, 0, sizeof(CompReadAttr));
    }

    // one entry per clause of the composition (none without clause data)
    if (bClauses)
    {
        Clauses.Empty();
        if (CompStrLen && CompClauseLen && CompAttrLen)
        {
            CompClause[255] = 0;
            for (INT i = 1; CompClause[i] && CompClause[i - 1] < (DWORD)CompStrLen; i++)
            {
                INT Count = CompClause[i] - CompClause[i - 1];
                appMemcpy(Clause, &CompStr[CompClause[i - 1]], Count * 2);
                Clause[Count] = 0;
                Item.Text[0] = 0;
                Item.Flag = 0;
                if (CompAttr[CompClause[i - 1]] & 1)
                    Item.Flag = 1;
                if (Clause[0])
                    wcscpy(Item.Text, Clause);
                Clauses(Clauses.Add()) = Item;
            }
        }
    }
    ImmReleaseContext(GViewportHWnd, hIMC);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x100134c0, NCChineseIME, OnImeComposition)

// 0x10014530 (vslot 64) - the Japanese IME's (see there) with the chat line's Chinese IME; both
// boxes are hidden when the candidates were shown
void NCChineseIME::Deactivate()
{
    bActive = 0;
    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmNotifyIME(hIMC, NI_CLOSECANDIDATE, 0, 0);
    OpenStatus = ImmGetOpenStatus(hIMC);
    hIMC = ImmGetContext(GViewportHWnd);
    if (GIsEnterChatting)
        ImmSetOpenStatus(hIMC, 0);
    else if (Mode != 1)
        ImmAssociateContext(GViewportHWnd, GDesktop->ChatWnd->EditBox->IMEManager->ChineseIME->hIMC);
    FreeCandList();
    Clauses.Empty();
    if (CandidateBox->IsShowWindow())
    {
        CandidateBox->HideWindow();
        CompositeBox->HideWindow();
    }
    bNotComposing = 1;
}
NW_HOOK_METHOD(0x10014530, NCChineseIME, Deactivate)

// 0x10014c90 (vslot 65) - only while active with a Chinese IME. The candidate list is read when
// it opens (after releasing the context; the IME type 11 follows its changes only from then on,
// the type 12 sends the first character of the page's second candidate as the result on every
// other opening) and closed with both boxes (always for the types 11 and 12). IMN_PRIVATE: for
// the types 11 and 12 the keys held build the composition window's text (letters added in
// lower case, Backspace, Space empties it) and the type 12 takes the candidate of the digit
// held (1..9, 0 for the tenth) as the result, without its first character; then the reading
// string is read while no candidate is shown (always for the type 8), and the private
// notifications the old IMEs send for their own reading window are kept from the IME (4.x: 1
// and 2; 5.3: 16, 17, 26, 27, 28). Not handled (0) otherwise.
INT NCChineseIME::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    if (!bActive || !IsChineseIME())
        return 0;
    switch (wParam)
    {
    case IMN_CLOSESTATUSWINDOW:
    case IMN_OPENSTATUSWINDOW:
    case IMN_SETOPENSTATUS:
        break;
    case IMN_OPENCANDIDATE:
        ImmReleaseContext(GViewportHWnd, hIMC);
        if (GConsole->GetIMEType() == 11)
            bCandOpened = 1;
        ReadCandList();
        if (GConsole->GetIMEType() == 12)
        {
            if (!bOpenResultSent)
            {
                bOpenResultSent = 1;
                appMemset(ResultStr, 0, sizeof(ResultStr));
                ResultStr[0] = 0;
                if (CandList)
                {
                    appMemcpy(ResultStr, (BYTE*)CandList + CandList->dwOffset[CandList->dwPageStart + 1], 2);
                    ResultStr[1] = 0;
                    SendToParent(NIM_RESULT, 0, 0);
                }
            }
            else
                bOpenResultSent = 0;
        }
        break;
    case IMN_CHANGECANDIDATE:
        if (GConsole->GetIMEType() != 11 || bCandOpened)
            ReadCandList();
        break;
    case IMN_CLOSECANDIDATE:
        if (lParam == 1 || GConsole->GetIMEType() == 11 || GConsole->GetIMEType() == 12)
        {
            FreeCandList();
            CandidateBox->HideWindow();
            CompositeBox->HideWindow();
            bOpenResultSent = 0;
            bCandOpened = 0;
        }
        break;
    case IMN_PRIVATE:
        if (GConsole->GetIMEType() == 11 || GConsole->GetIMEType() == 12)
        {
            BYTE KeyState[256];
            GetKeyboardState(KeyState);
            FString Text;
            for (INT Key = 'A'; Key <= 'Z'; Key++)
            {
                if (KeyState[Key] & 0x80)
                {
                    TCHAR Letter[2];
                    Letter[0] = (TCHAR)(Key + ('a' - 'A'));
                    Letter[1] = 0;
                    Text = CompositeBox->GetText() + FString(Letter);
                    CompositeBox->SetText(*Text);
                }
            }
            if (KeyState[VK_BACK] & 0x80)
            {
                Text = CompositeBox->GetText();
                if (Text.Len())
                {
                    Text.Delete(Text.Len() - 1, 1);
                    CompositeBox->SetText(*Text);
                }
            }
            if (KeyState[VK_SPACE] & 0x80)
            {
                FString None;
                None.Empty();
                CompositeBox->SetText(*None);
            }
            if (bCandOpened)
                bCandOpened = 0;
            if (GConsole->GetIMEType() == 12 && CandList)
            {
                appMemset(ResultStr, 0, sizeof(ResultStr));
                DWORD Start = CandList->dwPageStart;
                BYTE* List = (BYTE*)CandList;
                UBOOL bPicked = 0;
                TCHAR* Cand = NULL;
                // the last digit held wins, 0 over all (tested one by one in the stock code)
                for (INT Digit = 1; Digit <= 9; Digit++)
                {
                    if (KeyState['0' + Digit] & 0x80)
                    {
                        Cand = (TCHAR*)(List + CandList->dwOffset[Start + Digit - 1]);
                        bPicked = 1;
                    }
                }
                if (KeyState['0'] & 0x80)
                {
                    Cand = (TCHAR*)(List + CandList->dwOffset[Start + 9]);
                    bPicked = 1;
                }
                if (bPicked && Cand)
                {
                    INT CandLen = appStrlen(Cand);
                    appMemcpy(ResultStr, Cand + 1, CandLen * 2 - 2);
                    ResultStr[CandLen - 1] = 0;
                    FreeCandList();
                    Clauses.Empty();
                    bOpenResultSent = 0;
                    SendToParent(NIM_RESULT, 0, 0);
                    SendToParent(NIM_ATTRIBUTE, 1, 1);
                }
            }
        }
        if (!CandidateBox->IsShowWindow() || GConsole->GetIMEType() == 8)
            ReadReadingString();
        switch (GetImeId())
        {
        case IMEID_CHS_VER41:
        case IMEID_CHS_VER42:
            if (lParam == 1 || lParam == 2)
                return 0;
            break;
        case IMEID_CHS_VER53:
            if (lParam == 16 || lParam == 17 || lParam == 26 || lParam == 27 || lParam == 28)
                return 0;
            break;
        }
        return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x10014c90, NCChineseIME, OnImeNotify)

NW_CHECK_VTABLE(NCChineseIME, 0x100dc4b8, 75)

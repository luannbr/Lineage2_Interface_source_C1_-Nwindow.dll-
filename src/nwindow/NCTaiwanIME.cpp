/*=============================================================================
    NCTaiwanIME.cpp - IME of the Taiwanese client. Stock: 0x1000e6f0 .. 0x1000e7d9,
    0x1000e7e0 .. 0x1000e832, 0x1000e890 .. 0x1000ed5a (up to OnImeSetContext's end),
    0x1000ed90 .. 0x1000ee6a, 0x1000f890 .. 0x1000f953, 0x1000fad0 .. 0x1000fbea,
    0x10012310 .. 0x100131ff, 0x10013210 .. 0x100132fe, 0x10014860 .. 0x10014b57 (OnDestroy,
    ShowWindow and the candidate box filling are the Japanese IME's bodies).
=============================================================================*/
#include "nwindow/NCTaiwanIME.h"
#include "nwindow/NCChatWnd.h"
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
    IMEID_CHT_VER42 = 0x40002,         // New Phonetic / ChangJie, IME 98
    IMEID_CHT_VER43 = 0x40003,         // IME 98a
    IMEID_CHT_VER44 = 0x40004,         // New ChangJie, IME 98b
    IMEID_CHT_VER50 = 0x50000,
    IMEID_CHT_VER51 = 0x50001,         // IME 2002
    IMEID_CHT_VER52 = 0x50002,
    IMEID_CHT_VER60 = 0x60000,
};

// 0x10014860 - CandList and the candidate box are left as allocated
NCTaiwanIME::NCTaiwanIME(INT InMode)
    : NCIME(0)
{
    Mode = InMode;
}
NW_HOOK_CTOR_ARGS(0x10014860, NCTaiwanIME, (INT A), (A))

// 0x100148d0
NW_HOOK_DTOR(0x100148d0, NCTaiwanIME)

// 0x1000f330 - the class's operator new (NW_DECLARE_CLASS), out of line for the manager's
// OnCreate
static void* NewTaiwanIME(size_t Size, UObject* Outer, FName Name, DWORD SetFlags)
{
    return NCTaiwanIME::operator new(Size, Outer, Name, SetFlags);
}
NW_HOOK_FUNC(0x1000f330, NewTaiwanIME)

// 0x1000e6f0 (vslot 24) - as the Japanese IME's, without the end of the composition
INT NCTaiwanIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NCIME::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == WM_IME_NOTIFY)
        return OnImeNotify(wParam, lParam) != 0;
    if (Msg == WM_IME_COMPOSITION)
        return OnImeComposition(wParam, lParam) != 0;
    if (Msg == WM_INPUTLANGCHANGE)
        return OnInputLangChange(wParam, lParam) != 0;
    if (Msg == WM_IME_SETCONTEXT)
        return OnImeSetContext(wParam, lParam) != 0;
    if (Msg == NIM_CARET)
        return OnCaret((INT)wParam, (INT)lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x1000e6f0, NCTaiwanIME, WindowProc)

// 0x1000e7e0 - GTaiwanGetReadingString from the current IME's file, NULL without it (the
// library is not freed)
void NCTaiwanIME::LoadReadingString()
{
    ANSICHAR FileName[MAX_PATH];
    HKL Layout = GetKeyboardLayout(0);
    GTaiwanGetReadingString = NULL;
    if (ImmGetIMEFileNameA(Layout, FileName, MAX_PATH) > 0)
    {
        HMODULE IME = LoadLibraryA(FileName);
        if (IME)
            GTaiwanGetReadingString = (NGetReadingString)GetProcAddress(IME, "GetReadingString");
    }
}
NW_HOOK_FUNC(0x1000e7e0, NCTaiwanIME::LoadReadingString)

// 0x1000e890 - the version of the old Taiwanese IMEs (New Phonetic, New ChangJie) that keep the
// reading string in their private data: their layout, their file (unless the IME has
// GetReadingString, one of TINTLGNT, CINTLGNT, MSTCIPHA), a known version for a Chinese layout
// (any version with GetReadingString); 0 otherwise
DWORD NCTaiwanIME::GetImeId()
{
    ANSICHAR FileName[MAX_PATH];
    HKL Layout = GetKeyboardLayout(0);
    DWORD Lang = (DWORD)(UINT_PTR)Layout & 0x3FF;
    if ((UINT_PTR)Layout != 0xE0080404 && (UINT_PTR)Layout != 0xE0090404)
        return 0;
    if (ImmGetIMEFileNameA(Layout, FileName, MAX_PATH - 1) <= 0)
        return 0;
    if (!GTaiwanGetReadingString && lstrcmpiA(FileName, "TINTLGNT.IME")
        && lstrcmpiA(FileName, "CINTLGNT.IME") && lstrcmpiA(FileName, "MSTCIPHA.IME"))
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
            if (GTaiwanGetReadingString
                || (Lang == LANG_CHINESE
                    && (Version == IMEID_CHT_VER42 || Version == IMEID_CHT_VER43
                        || Version == IMEID_CHT_VER44 || Version == IMEID_CHT_VER50
                        || Version == IMEID_CHT_VER51 || Version == IMEID_CHT_VER52
                        || Version == IMEID_CHT_VER60)))
            {
                free(Buffer);
                return Version;
            }
        }
    }
    free(Buffer);
    return 0;
}
NW_HOOK_FUNC(0x1000e890, NCTaiwanIME::GetImeId)

// 0x1000ea00 - the reading string into ReadingStr and the parent told (NIM_READING, length,
// error position): from GetReadingString (the buffer is allocated in bytes for the character
// count and never freed), else from the private data of the old IMEs by version (the 5.0 one
// in ANSI); nothing without a known IME
void NCTaiwanIME::ReadReadingString()
{
    DWORD Id = GetImeId();
    if (!Id || !hIMC)
        return;
    WCHAR Local[262];
    INT Err = 0;
    UBOOL bUnicode = 0;
    UINT Len = 0;
    WCHAR* Str = NULL;
    if (GTaiwanGetReadingString)
    {
        BOOL bVertical;
        UINT MaxUiLen;
        Len = GTaiwanGetReadingString(hIMC, 0, NULL, &Err, &bVertical, &MaxUiLen);
        if (Len)
        {
            Str = (WCHAR*)malloc(Len);
            if (!Str)
            {
                ImeReleaseContext(hIMC);
                return;
            }
            Len = GTaiwanGetReadingString(hIMC, Len, Str, &Err, &bVertical, &MaxUiLen);
        }
        bUnicode = 1;
    }
    else
    {
        BYTE* IC = GImmLockIMC(hIMC);
        BYTE* p;
        switch (Id)
        {
        case IMEID_CHT_VER42:
        case IMEID_CHT_VER43:
        case IMEID_CHT_VER44:
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 0x18);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x9C);
            Err = *(DWORD*)(p + 0xA0);
            Str = (WCHAR*)(p + 0x38);
            bUnicode = 1;
            break;
        case IMEID_CHT_VER50:
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 0xC);
            if (!p)
                break;
            p = *(BYTE**)(p + 0x20);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x50) >> 1;
            Err = *(DWORD*)(p + 0x54) >> 1;
            MultiByteToWideChar(CodePage, 0, (ANSICHAR*)(p + 0x40), -1, Local, Len);
            bUnicode = 0;
            break;
        case IMEID_CHT_VER51:
        case IMEID_CHT_VER52:
            p = *(BYTE**)((BYTE*)GImmLockIMCC(IMCPrivate(IC)) + 4);
            if (!p)
                break;
            p = *(BYTE**)(p + 0x18);
            if (!p)
                break;
            Len = *(DWORD*)(p + 0x60);
            Err = *(DWORD*)(p + 0x64);
            Str = (WCHAR*)(p + 0x40);
            bUnicode = 1;
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
NW_HOOK_METHOD(0x1000ea00, NCTaiwanIME, ReadReadingString)

// 0x1000ec20 - a Traditional Chinese layout with a Taiwanese IME: layouts E002, E006, E008 and
// E020, or the files TINTLGNT, CINTLGNT and MSTCIPHA (this is not used)
UBOOL NCTaiwanIME::IsTaiwanIME()
{
    ANSICHAR FileName[260];
    HKL Layout = GetKeyboardLayout(0);
    ImmGetIMEFileNameA(Layout, FileName, 259);
    if (((UINT_PTR)Layout & 0x3FF) != LANG_CHINESE)
        return 0;
    WORD Device = HIWORD((UINT_PTR)Layout);
    if (Device == 0xE002 || Device == 0xE006 || Device == 0xE008 || Device == 0xE020
        || !lstrcmpiA(FileName, "TINTLGNT.IME") || !lstrcmpiA(FileName, "CINTLGNT.IME")
        || !lstrcmpiA(FileName, "MSTCIPHA.IME"))
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1000ec20, NCTaiwanIME, IsTaiwanIME)

// 0x1000ecc0 (vslot 68) - the conversion mode and the open status kept (0 without a context)
INT NCTaiwanIME::OnImeSetContext(WPARAM wParam, LPARAM lParam)
{
    guard(NCTaiwanIME::OnImeSetContext);
    if (!ImeGetContext())
        return 0;
    ImmGetConversionStatus(hIMC, &Conversion, &Sentence);
    OpenStatus = ImmGetOpenStatus(hIMC);
    ImmReleaseContext(GViewportHWnd, hIMC);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x1000ecc0, NCTaiwanIME, OnImeSetContext)

// 0x1000ed90 (vslot 64) - the Japanese IME's (see there) with the chat line's Taiwanese IME, the
// clauses kept
void NCTaiwanIME::Deactivate()
{
    bActive = 0;
    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmNotifyIME(hIMC, NI_CLOSECANDIDATE, 0, 0);
    OpenStatus = ImmGetOpenStatus(hIMC);
    hIMC = ImmGetContext(GViewportHWnd);
    if (GIsEnterChatting)
        ImmSetOpenStatus(hIMC, 0);
    else if (Mode != 1)
        ImmAssociateContext(GViewportHWnd, GDesktop->ChatWnd->EditBox->IMEManager->TaiwanIME->hIMC);
    FreeCandList();
    if (CandidateBox->IsShowWindow())
        CandidateBox->HideWindow();
    bNotComposing = 1;
}
NW_HOOK_METHOD(0x1000ed90, NCTaiwanIME, Deactivate)

// 0x1000f890 (vslot 35) - the candidate box, hidden, then the reading string API, imm32's input
// context locks and the code page
INT NCTaiwanIME::OnCreate()
{
    NCIME::OnCreate();
    CandidateBox = new NCCandidateBox(9, 15);
    CandidateBox->SetWindowRect(0, 0, 100, 135);
    CreateChild(CandidateBox, NWS_VISIBLE);
    CandidateBox->HideWindow();
    LoadReadingString();
    LoadImmFunctions();
    UpdateCodePage();
    return 1;
}
NW_HOOK_METHOD(0x1000f890, NCTaiwanIME, OnCreate)

// 0x1000fad0 (vslot 74) - shown and placed only when hidden: moved back to the IME's origin,
// then by (X, -Y) with the layout and page of the IME type (6: 1, 9 a page; 4: 2, 9 a page;
// 5 and 7: 3, 10 a page); other types leave it at the origin
INT NCTaiwanIME::OnCaret(INT X, INT Y)
{
    if (!CandidateBox->IsShowWindow())
    {
        CandidateBox->MoveWindow(-CandidateBox->LocalX, -CandidateBox->LocalY);
        if (GConsole->GetIMEType() == 6)
        {
            CandidateBox->MoveWindow(X, -Y);
            CandidateBox->SetMode(1);
            CandidateBox->PageSize = 9;
        }
        else if (GConsole->GetIMEType() == 4)
        {
            CandidateBox->MoveWindow(X, -Y);
            CandidateBox->SetMode(2);
            CandidateBox->PageSize = 9;
        }
        else if (GConsole->GetIMEType() == 5 || GConsole->GetIMEType() == 7)
        {
            CandidateBox->MoveWindow(X, -Y);
            CandidateBox->SetMode(3);
            CandidateBox->PageSize = 10;
        }
        CandidateBox->ShowWindow();
    }
    return 1;
}
NW_HOOK_METHOD(0x1000fad0, NCTaiwanIME, OnCaret)

// stock body shared with the Japanese IME's (0x1000e680)
void NCTaiwanIME::ShowWindow()
{
    NCWnd::ShowWindow();
    CandidateBox->HideWindow();
}

// stock body shared with the Japanese IME's (0x1000e6a0)
void NCTaiwanIME::FreeCandList()
{
    if (CandList)
    {
        CANDIDATELIST* List = CandList;
        CandList = NULL;
        appFree(List);
    }
}

// stock body shared with the Japanese IME's (0x1000f960)
void NCTaiwanIME::FillCandidateBox()
{
    SIZE Size;
    SIZE ItemSize;
    CandidateBox->Clear();
    DWORD i = CandList->dwPageStart;
    TCHAR* Text = (TCHAR*)((BYTE*)CandList + CandList->dwOffset[i]);
    CandidateBox->AddItem(Text, -1);
    if (i == CandList->dwSelection)
        CandidateBox->Selected = i % 9;
    NCGDevice::GetTextExtent(&Size, Text, 0, 0, 0, 0, 0);
    for (i = CandList->dwPageStart + 1; i < CandList->dwCount; i++)
    {
        if (i >= CandList->dwPageSize + CandList->dwPageStart)
            break;
        Text = (TCHAR*)((BYTE*)CandList + CandList->dwOffset[i]);
        CandidateBox->AddItem(Text, -1);
        NCGDevice::GetTextExtent(&ItemSize, Text, 0, 0, 0, 0, 0);
        if (i == CandList->dwSelection)
            CandidateBox->Selected = i % 9;
        if (ItemSize.cx > Size.cx)
            Size.cx = ItemSize.cx;
    }
    CandidateBox->SetScroll(CandList->dwCount, CandList->dwSelection);
    CandidateBox->PageSize = CandList->dwPageSize;
    CandidateBox->SetSize(Size.cx + 20, 135);
    SendToParent(NIM_GETCARET, (WPARAM)Entry, 0);
}

// stock body shared with the Japanese IME's (0x10011120)
INT NCTaiwanIME::OnDestroy()
{
    Clauses.Empty();
    if (CandList)
        appFree(CandList);
    CandList = NULL;
    DestroyChild(this, CandidateBox);
    ImmDestroyContext(hIMC);
    return 1;
}

// 0x10012310 (vslot 69) - the Japanese IME's copy (see there) with its own guard names, nothing
// read unless the IME is a Taiwanese one, and the Korean IME's cursor position (the clauses
// are added through TArray::AddItem, called out of line here)
INT NCTaiwanIME::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
    guard(NCTaiwanIME::OnImeComposition);
    TCHAR Clause[1024];
    NImeText Item;
    WCHAR Wide[0x1000];
    ANSICHAR Ansi[0x801];
    UBOOL bClauses = 0;
    INT Bytes;
    UINT Len = 0;
    ImeIsUIMessage(WM_IME_SETCONTEXT, 1, 0);
    ImeIsUIMessage(WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
    // the CJK clients with another IME: nothing (and the context is not released)
    if ((GLanguageType == 2 || GLanguageType == 3 || GLanguageType == 4) && !IsTaiwanIME())
        return 1;

    if (lParam & GCS_RESULTSTR)
    {
        if (lParam & GCS_RESULTCLAUSE)
        {
            if (IsAnsiIME())
            {
                guard(NCTaiwanIME::OnImeComposition::GCS_RESULTCLAUSE::NotUnicode);
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
                guard(NCTaiwanIME::OnImeComposition::GCS_RESULTCLAUSE::Unicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_RESULTSTR::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_RESULTSTR::Unicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPSTR::NotUnicode);
            Bytes = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, Ansi, 0x800);
            Ansi[Bytes] = 0;
            Len = MultiByteToWideChar(CodePage, 0, Ansi, -1, Wide, 0x1000);
            if (Len)
                Len = Len * 2 - 2;
            CompStrLen = Len >> 1;
            appMemset(CompStr, 0, sizeof(CompStr));
            CompStr[0] = 0;
            appStrcpy(CompStr, Wide);
            CompStr[CompStrLen] = 0;
            unguard;
        }
        else
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPSTR::Unicode);
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
    }

    if (lParam & GCS_COMPATTR)
    {
        if (IsAnsiIME())
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPATTR::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPATTR::Unicode);
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

    if (lParam & GCS_COMPCLAUSE)
    {
        if (IsAnsiIME())
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPCLAUSE::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPCLAUSE::Unicode);
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

    if (lParam & GCS_CURSORPOS)
    {
        if (IsAnsiIME())
            CursorPos = Len >> 1;
        else
            CursorPos = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0);
        SendToParent(NIM_CURSORPOS, CursorPos, 0);
    }

    if (lParam & GCS_COMPREADSTR)
    {
        if (IsAnsiIME())
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADSTR::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADSTR::Unicode);
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

    if (lParam & GCS_COMPREADCLAUSE)
    {
        if (IsAnsiIME())
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADCLAUSE::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADCLAUSE::Unicode);
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

    if (lParam & GCS_COMPREADATTR)
    {
        if (IsAnsiIME())
        {
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADATTR::NotUnicode);
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
            guard(NCTaiwanIME::OnImeComposition::GCS_COMPREADATTR::Unicode);
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

    // one entry per clause of the composition; without clause data the parent is told
    // whether there is a composition at all
    if (bClauses)
    {
        Clauses.Empty();
        if (CompStrLen == 0)
            SendToParent(NIM_COMPOSITION, 0, 0);
        else if (CompClauseLen == 0 || CompAttrLen == 0)
            SendToParent(NIM_COMPOSITION, 1, 0);
        else
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
                Clauses.AddItem(Item);
            }
        }
    }
    ImmReleaseContext(GViewportHWnd, hIMC);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10012310, NCTaiwanIME, OnImeComposition)

// 0x10013210 - the Japanese IME's (see there); an empty list hides the candidate box for the
// IME type 7
void NCTaiwanIME::ReadCandList()
{
    DWORD Size = ImmGetCandidateListW(hIMC, 0, NULL, 0);
    FreeCandList();
    if (Size)
    {
        CandList = (CANDIDATELIST*)appMalloc(Size, TEXT("new"));
        CandList->dwSize = Size;
        ImmGetCandidateListW(hIMC, 0, CandList, Size);
        DWORD Step = CandList->dwPageSize == 9 || !CandList->dwPageSize ? CandList->dwPageSize : 9;
        DWORD PageStart = 0;
        if (CandList->dwSelection > 0 && CandList->dwPageSize && Step <= CandList->dwSelection)
        {
            do
                PageStart += Step;
            while (Step + PageStart <= CandList->dwSelection);
        }
        if (CandList->dwPageStart != PageStart)
            ImmNotifyIME(hIMC, NI_SETCANDIDATE_PAGESTART, 0, PageStart);
        FillCandidateBox();
    }
    else if (GConsole->GetIMEType() == 7)
        CandidateBox->HideWindow();
    ImmReleaseContext(GViewportHWnd, hIMC);
}
NW_HOOK_METHOD(0x10013210, NCTaiwanIME, ReadCandList)

// 0x10014930 (vslot 65) - only while active with a Taiwanese IME (handled otherwise): the
// conversion mode and the open status followed, the candidate list read (after releasing the
// context when it opens) and closed with its box; IMN_PRIVATE reads the reading string while
// no candidate is shown and keeps from the IME the private notifications the old IMEs send
// for their own reading window (4.x: 1 and 2; 5.x and 6.0: 16, 17, 26, 27, 28)
INT NCTaiwanIME::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    guard(NCTaiwanIME::OnImeNotify);
    if (!bActive || !IsTaiwanIME())
        return 1;
    switch (wParam)
    {
    case IMN_SETCONVERSIONMODE:
        ImmGetConversionStatus(hIMC, &Conversion, &Sentence);
        return 0;
    case IMN_SETOPENSTATUS:
        OpenStatus = ImmGetOpenStatus(hIMC);
        return 0;
    case IMN_OPENCANDIDATE:
    {
        guard(NCTaiwanIME::OnImeNotify::IMN_OPENCANDIDATE);
        ImmReleaseContext(GViewportHWnd, hIMC);
        unguard;
    }
        // fall through
    case IMN_CHANGECANDIDATE:
        ReadCandList();
        return 0;
    case IMN_CLOSECANDIDATE:
        if (lParam == 1)
        {
            FreeCandList();
            CandidateBox->HideWindow();
        }
        return 0;
    case IMN_PRIVATE:
        if (!CandidateBox->IsShowWindow())
            ReadReadingString();
        switch (GetImeId())
        {
        case IMEID_CHT_VER42:
        case IMEID_CHT_VER43:
        case IMEID_CHT_VER44:
            if (lParam == 1 || lParam == 2)
                return 0;
            break;
        case IMEID_CHT_VER50:
        case IMEID_CHT_VER51:
        case IMEID_CHT_VER52:
        case IMEID_CHT_VER60:
            if (lParam == 16 || lParam == 17 || lParam == 26 || lParam == 27 || lParam == 28)
                return 0;
            break;
        }
        return 1;
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x10014930, NCTaiwanIME, OnImeNotify)

NW_CHECK_VTABLE(NCTaiwanIME, 0x100dc300, 75)

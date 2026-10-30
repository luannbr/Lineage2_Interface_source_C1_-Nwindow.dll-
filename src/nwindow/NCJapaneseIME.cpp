/*=============================================================================
    NCJapaneseIME.cpp - IME of the Japanese client. Stock: 0x1000e660 .. 0x1000e6ed,
    0x1000ee70 .. 0x1000ef7a, 0x1000f760 .. 0x1000f88c, 0x1000f960 .. 0x1000fac1,
    0x10011120 .. 0x1001220f, 0x10012210 .. 0x10012309, 0x10014710 .. 0x1001485f (the
    window procedure, OnDestroy, ShowWindow and the candidate list helpers are shared with the
    Taiwanese and Chinese IMEs, hooked here).
=============================================================================*/
#include "nwindow/NCJapaneseIME.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCEditBox.h"
#include "nwindow/NCGDevice.h"
#include "nwindow/NCIMEManager.h"
#include "nwindow/NConsoleWnd.h"

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

// 0x10014710 - CandList and the candidate box are left as allocated
NCJapaneseIME::NCJapaneseIME(INT InMode)
    : NCIME(0)
{
    Mode = InMode;
}
NW_HOOK_CTOR_ARGS(0x10014710, NCJapaneseIME, (INT A), (A))

// 0x10014780
NW_HOOK_DTOR(0x10014780, NCJapaneseIME)

// 0x1000ee70 (vslot 24) - also the Chinese IME's: NCIME's messages again after NCIME, and the
// caret the edit box sends back
INT NCJapaneseIME::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
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
NW_HOOK_METHOD(0x1000ee70, NCJapaneseIME, WindowProc)

// 0x1000e660 - the keyboard layout is Japanese (this is not used)
UBOOL NCJapaneseIME::IsJapaneseLayout()
{
    return PRIMARYLANGID(LOWORD(GetKeyboardLayout(0))) == LANG_JAPANESE;
}
NW_HOOK_METHOD(0x1000e660, NCJapaneseIME, IsJapaneseLayout)

// 0x1000e680 (vslot 42) - also the Taiwanese IME's
void NCJapaneseIME::ShowWindow()
{
    NCWnd::ShowWindow();
    CandidateBox->HideWindow();
}
NW_HOOK_METHOD(0x1000e680, NCJapaneseIME, ShowWindow)

// 0x1000e6a0 - also the Taiwanese and Chinese IMEs'
void NCJapaneseIME::FreeCandList()
{
    if (CandList)
    {
        CANDIDATELIST* List = CandList;
        CandList = NULL;
        appFree(List);
    }
}
NW_HOOK_METHOD(0x1000e6a0, NCJapaneseIME, FreeCandList)

// 0x1000e6d0 (vslot 67)
INT NCJapaneseIME::OnImeEndComposition(WPARAM wParam, LPARAM lParam)
{
    bNotComposing = 1;
    SendToParent(NIM_ENDCOMPOSITION, 0, 0);
    return 1;
}
NW_HOOK_METHOD(0x1000e6d0, NCJapaneseIME, OnImeEndComposition)

// 0x1000f760 (vslot 35) - the candidate box, hidden
INT NCJapaneseIME::OnCreate()
{
    NCIME::OnCreate();
    CandidateBox = new NCCandidateBox(9, 15);
    CandidateBox->SetWindowRect(0, 0, 100, 135);
    CreateChild(CandidateBox, NWS_VISIBLE);
    CandidateBox->HideWindow();
    return 1;
}
NW_HOOK_METHOD(0x1000f760, NCJapaneseIME, OnCreate)

// 0x1000f820 (vslot 74) - the candidate box shown at the caret (moved only when it was hidden)
INT NCJapaneseIME::OnCaret(INT X, INT Y)
{
    if (!CandidateBox->IsShowWindow())
    {
        CandidateBox->MoveWindow(-CandidateBox->LocalX, -CandidateBox->LocalY);
        CandidateBox->MoveWindow(X, Y);
    }
    CandidateBox->SetMode(0);
    CandidateBox->ShowWindow();
    return 1;
}
NW_HOOK_METHOD(0x1000f820, NCJapaneseIME, OnCaret)

// 0x1000f960 - also the Taiwanese IME's: the page of CandList (the selection among them, 9 a
// page) into the candidate box, sized to its widest candidate, then the edit box is asked for
// its caret
void NCJapaneseIME::FillCandidateBox()
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
NW_HOOK_METHOD(0x1000f960, NCJapaneseIME, FillCandidateBox)

// 0x10011120 (vslot 36) - also the Taiwanese IME's (NCWnd::OnDestroy is not called)
INT NCJapaneseIME::OnDestroy()
{
    Clauses.Empty();
    if (CandList)
        appFree(CandList);
    CandList = NULL;
    DestroyChild(this, CandidateBox);
    ImmDestroyContext(hIMC);
    return 1;
}
NW_HOOK_METHOD(0x10011120, NCJapaneseIME, OnDestroy)

// 0x100111a0 - the IME's candidate list copied, the IME told to start its page where the
// selection's page of 9 starts, the box filled; the context is released in any case
void NCJapaneseIME::ReadCandList()
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
    ImmReleaseContext(GViewportHWnd, hIMC);
}
NW_HOOK_METHOD(0x100111a0, NCJapaneseIME, ReadCandList)

// 0x10011270 (vslot 69) - NCKoreanIME::OnImeComposition's copy (see there), with its own guard
// names, nothing read unless the keyboard layout is Japanese, and the cursor position: from
// the IME only while a character still has attribute 0 (being typed), else the end of the
// composition. The stock code's guards around the cursor position compiled to nothing.
INT NCJapaneseIME::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
    guard(NCJapaneseIME::OnImeComposition);
    TCHAR Clause[1024];
    NImeText Item;
    WCHAR Wide[0x1000];
    ANSICHAR Ansi[0x801];
    UBOOL bClauses = 0;
    INT Bytes;
    UINT Len = 0;
    ImeIsUIMessage(WM_IME_SETCONTEXT, 1, 0);
    ImeIsUIMessage(WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
    // the CJK clients with another layout: nothing (and the context is not released)
    if ((GLanguageType == 2 || GLanguageType == 3 || GLanguageType == 4) && !IsJapaneseLayout())
        return 1;

    if (lParam & GCS_RESULTSTR)
    {
        if (lParam & GCS_RESULTCLAUSE)
        {
            if (IsAnsiIME())
            {
                guard(NCJapaneseIME::OnImeComposition::GCS_RESULTCLAUSE::NotUnicode);
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
                guard(NCJapaneseIME::OnImeComposition::GCS_RESULTCLAUSE::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_RESULTSTR::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_RESULTSTR::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPSTR::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPSTR::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPATTR::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPATTR::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPCLAUSE::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPCLAUSE::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADSTR::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADSTR::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADCLAUSE::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADCLAUSE::Unicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADATTR::NotUnicode);
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
            guard(NCJapaneseIME::OnImeComposition::GCS_COMPREADATTR::Unicode);
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
                Clauses(Clauses.Add()) = Item;
            }
        }
    }
    ImmReleaseContext(GViewportHWnd, hIMC);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10011270, NCJapaneseIME, OnImeComposition)

// 0x10012210 (vslot 64) - the composition cancelled and the candidate list closed, the open
// status kept, and the game window's current context taken; leaving the chat line closes the
// IME, otherwise (unless Mode is 1) the game window gets the chat line's context back. The
// candidates go.
void NCJapaneseIME::Deactivate()
{
    bActive = 0;
    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmNotifyIME(hIMC, NI_CLOSECANDIDATE, 0, 0);
    OpenStatus = ImmGetOpenStatus(hIMC);
    hIMC = ImmGetContext(GViewportHWnd);
    if (GIsEnterChatting)
        ImmSetOpenStatus(hIMC, 0);
    else if (Mode != 1)
        ImmAssociateContext(GViewportHWnd, GDesktop->ChatWnd->EditBox->IMEManager->JapaneseIME->hIMC);
    FreeCandList();
    Clauses.Empty();
    if (CandidateBox->IsShowWindow())
        CandidateBox->HideWindow();
    bNotComposing = 1;
}
NW_HOOK_METHOD(0x10012210, NCJapaneseIME, Deactivate)

// 0x100147e0 (vslot 65) - while active: the candidate list read when it opens (after
// releasing the context) or changes, closed with its box; never handled (0)
INT NCJapaneseIME::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    if (!bActive)
        return 0;
    switch (wParam)
    {
    case IMN_OPENCANDIDATE:
        ImmReleaseContext(GViewportHWnd, hIMC);
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
        break;
    }
    return 0;
}
NW_HOOK_METHOD(0x100147e0, NCJapaneseIME, OnImeNotify)

NW_CHECK_VTABLE(NCJapaneseIME, 0x100dc1d0, 75)

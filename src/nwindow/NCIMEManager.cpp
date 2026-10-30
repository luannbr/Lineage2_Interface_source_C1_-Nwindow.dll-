/*=============================================================================
    NCIMEManager.cpp - the IME of an edit box. Stock: 0x1000dd60 .. 0x1000ded9,
    0x1000f400 .. 0x1000f6e4, 0x100152a0 .. 0x100154e9; WindowProc body shared with NCEditBox.
=============================================================================*/
#include "nwindow/NCIMEManager.h"
#include "nwindow/NCChineseIME.h"
#include "nwindow/NCJapaneseIME.h"
#include "nwindow/NCKoreanIME.h"
#include "nwindow/NCTaiwanIME.h"

// 0x1000f400 - NControl's constructor is inlined in the stock code
NCIMEManager::NCIMEManager(INT InMode)
{
    KoreanIME = NULL;
    JapaneseIME = NULL;
    TaiwanIME = NULL;
    ChineseIME = NULL;
    Mode = InMode;
}
NW_HOOK_CTOR_ARGS(0x1000f400, NCIMEManager, (INT A), (A))

// 0x1000f490
NW_HOOK_DTOR(0x1000f490, NCIMEManager)

// 0x10007f80 (vslot 24) - also NCEditBox's (identical body)
INT NCIMEManager::WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg != WM_PAINT && !IsEnabled())
        return 0;
    if (NControl::WindowProc(Msg, wParam, lParam))
        return 1;
    if (Msg == NIM_COMPOSITION)
        return OnComposition(wParam, lParam) != 0;
    if (Msg == NIM_RESULT)
        return OnResult(wParam, lParam) != 0;
    if (Msg == NIM_ATTRIBUTE)
        return OnAttribute(wParam, lParam) != 0;
    if (Msg == NIM_READING)
        return OnReading(wParam, lParam) != 0;
    if (Msg == NIM_14)
        return On14(wParam, lParam) != 0;
    if (Msg == NIM_GETCARET)
        return OnGetCaret(wParam, lParam) != 0;
    if (Msg == NIM_ENDCOMPOSITION)
        return OnEndComposition(wParam, lParam) != 0;
    if (Msg == NIM_CURSORPOS)
        return OnCursorPos(wParam, lParam) != 0;
    return 0;
}
NW_HOOK_METHOD(0x10007f80, NCIMEManager, WindowProc)

// 0x100152a0 (vslot 35) - one IME for the client language (NControl::OnCreate is not called)
INT NCIMEManager::OnCreate()
{
    switch (GLanguageType)
    {
    case 0:
    case 1:
        KoreanIME = new NCKoreanIME(Mode);
        CreateChild(KoreanIME, NWS_VISIBLE);
        break;
    case 2:
        JapaneseIME = new NCJapaneseIME(Mode);
        JapaneseIME->AttachEntry(AddChildWindow(JapaneseIME, NWS_VISIBLE));
        break;
    case 3:
        TaiwanIME = new NCTaiwanIME(Mode);
        TaiwanIME->AttachEntry(AddChildWindow(TaiwanIME, NWS_VISIBLE));
        break;
    case 4:
        ChineseIME = new NCChineseIME(Mode);
        ChineseIME->AttachEntry(AddChildWindow(ChineseIME, NWS_VISIBLE));
        break;
    }
    return 1;
}
NW_HOOK_METHOD(0x100152a0, NCIMEManager, OnCreate)

// 0x1000f4e0 (vslot 36)
INT NCIMEManager::OnDestroy()
{
    switch (GLanguageType)
    {
    case 0:
    case 1:
        if (KoreanIME)
        {
            RemoveChildWindow(KoreanIME->Entry);
            delete KoreanIME;
            KoreanIME = NULL;
        }
        break;
    case 2:
        if (JapaneseIME)
        {
            RemoveChildWindow(JapaneseIME->Entry);
            delete JapaneseIME;
            JapaneseIME = NULL;
        }
        break;
    case 3:
        if (TaiwanIME)
        {
            RemoveChildWindow(TaiwanIME->Entry);
            delete TaiwanIME;
            TaiwanIME = NULL;
        }
        break;
    case 4:
        if (ChineseIME)
        {
            RemoveChildWindow(ChineseIME->Entry);
            delete ChineseIME;
            ChineseIME = NULL;
        }
        break;
    }
    return 1;
}
NW_HOOK_METHOD(0x1000f4e0, NCIMEManager, OnDestroy)

// 0x1000dd60 (vslot 63)
void NCIMEManager::Activate()
{
    if (KoreanIME)
        KoreanIME->Activate();
    else if (JapaneseIME)
        JapaneseIME->Activate();
    else if (TaiwanIME)
        TaiwanIME->Activate();
    else if (ChineseIME)
        ChineseIME->Activate();
}
NW_HOOK_METHOD(0x1000dd60, NCIMEManager, Activate)

// 0x1000ddb0 (vslot 64)
void NCIMEManager::Deactivate()
{
    if (KoreanIME)
        KoreanIME->Deactivate();
    else if (JapaneseIME)
        JapaneseIME->Deactivate();
    else if (TaiwanIME)
        TaiwanIME->Deactivate();
    else if (ChineseIME)
        ChineseIME->Deactivate();
}
NW_HOOK_METHOD(0x1000ddb0, NCIMEManager, Deactivate)

// 0x1000de00 (vslot 65) - 65..72 pass the IME's messages on to the edit box
INT NCIMEManager::OnComposition(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_COMPOSITION, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000de00, NCIMEManager, OnComposition)

// 0x1000de20 (vslot 66)
INT NCIMEManager::OnResult(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_RESULT, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000de20, NCIMEManager, OnResult)

// 0x1000de40 (vslot 67)
INT NCIMEManager::OnAttribute(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_ATTRIBUTE, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000de40, NCIMEManager, OnAttribute)

// 0x1000de60 (vslot 68)
INT NCIMEManager::OnReading(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_READING, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000de60, NCIMEManager, OnReading)

// 0x1000de80 (vslot 70)
INT NCIMEManager::OnGetCaret(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_GETCARET, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000de80, NCIMEManager, OnGetCaret)

// 0x1000dea0 (vslot 71)
INT NCIMEManager::OnEndComposition(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_ENDCOMPOSITION, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000dea0, NCIMEManager, OnEndComposition)

// 0x1000dec0 (vslot 72)
INT NCIMEManager::OnCursorPos(WPARAM wParam, LPARAM lParam)
{
    SendToParent(NIM_CURSORPOS, wParam, lParam);
    return 1;
}
NW_HOOK_METHOD(0x1000dec0, NCIMEManager, OnCursorPos)

// 0x1000f5f0
TCHAR* NCIMEManager::GetCompStr()
{
    NCIME* IME = GetIME();
    return IME ? IME->GetCompStr() : NULL;
}
NW_HOOK_METHOD(0x1000f5f0, NCIMEManager, GetCompStr)

// 0x1000f630
TCHAR* NCIMEManager::GetResultStr()
{
    NCIME* IME = GetIME();
    return IME ? IME->GetResultStr() : NULL;
}
NW_HOOK_METHOD(0x1000f630, NCIMEManager, GetResultStr)

// 0x1000f670
TCHAR* NCIMEManager::GetReadingStr()
{
    NCIME* IME = GetIME();
    return IME ? IME->GetReadingStr() : NULL;
}
NW_HOOK_METHOD(0x1000f670, NCIMEManager, GetReadingStr)

// 0x1000f6b0
UBOOL NCIMEManager::IsNotComposing()
{
    NCIME* IME = GetIME();
    return IME ? IME->IsNotComposing() : -1;
}
NW_HOOK_METHOD(0x1000f6b0, NCIMEManager, IsNotComposing)

NW_CHECK_VTABLE(NCIMEManager, 0x100da100, 73)

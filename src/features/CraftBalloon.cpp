// Phase 6: CraftBalloon (include/features/CraftBalloon.h) - NWindow_C1 src/inject/CraftBalloon.cpp
// ("v6"): the same six Engine.dll sites, checks, caves and texts. Differences: installed from
// the console's first tick (NWindow_C1: at DLL attach, then again from a thread), the log is
// nwf::Log ("craftballoon" lines, [Window] Log=1), and the draw no longer hands the User to
// ManufactureCmd_NoteUser (its client-side dwarf check was not ported: the server decides).
//
// FDynamicActor::Render (Engine.dll RVA 0x27D680), where it draws a store owner's balloon:
//   path A  27FB37  mov ax, [edi+1F8h]          the owner's store type
//           27FB4A  mov ecx, 1097692Ch          FString "<Private Store - Buy>" (types 1, 2
//                                               jump to the sell one at 10976938h)
//           ...     appSprintf(buf, "%s\n%s", *label, *title)
//           27FB9F  push 0FFFDFEA5h             the buy colour, [esp] = buf
//   path B  27FC47  mov di, [edi+1F8h]          (no title: the label alone)
//           27FC5A  mov ecx, 1097692Ch
//           27FC72  push 0FFFDFEA5h             [esp] = the label's text
// A cave at each: the type sites note whether the owner drawn is a manufacture store (type 5),
// the label and colour sites then put "<Private Workshop - Craft>" and cyan in place of the buy
// ones (path A keeps the buffer's second line, the title).
#include "nwindow/NWindow.h"
#include "features/CraftBalloon.h"

enum
{
    RVA_LoadTypeA = 0x27FB37,
    RVA_LoadTypeB = 0x27FC47,
    RVA_BuyLabelA = 0x27FB4A,
    RVA_BuyLabelB = 0x27FC5A,
    RVA_BuyColorA = 0x27FB9F,
    RVA_BuyColorB = 0x27FC72,
    RVA_BuyLabelFString = 0x67692C,   // 1097692Ch at the preferred base 10300000h
    OFF_StoreType = 0x1F8,            // User: the private store type (a word, the low byte used)
    STORETYPE_Manufacture = 5,
};

// FString's layout, for the label the Engine takes the text of (FString::operator*)
struct NCraftBalloonLabel
{
    TCHAR* Data;
    INT Num;
    INT Max;
};

static TCHAR GCraftLabel[] = TEXT("<Private Workshop - Craft>");
enum { CRAFTLABEL_Num = 27 };   // with the terminator (Num and Max of an FString)
static NCraftBalloonLabel GCraftLabelFString = {GCraftLabel, CRAFTLABEL_Num, CRAFTLABEL_Num};

static UBOOL GInstallTried;
static volatile LONG GApplyCount;
static volatile LONG GTypeLogs = 12;

// read by the caves
static BYTE* GRetTypeA;
static BYTE* GRetTypeB;
static BYTE* GRetLabelA;
static BYTE* GRetLabelB;
static BYTE* GRetColorA;
static BYTE* GRetColorB;
static DWORD GBuyLabelStock;             // the stock FString, at Engine.dll's real base
static DWORD GColorBuy = 0xFFFDFEA5;
static DWORD GColorCraft = 0xFF66EEFF;
static volatile LONG GForceCraft;        // the owner being drawn has a manufacture store
static volatile LONG GLabelHits;
static volatile LONG GColorHits;
static volatile LONG GTextHits;
static TCHAR* GCraftLabelPtr = GCraftLabel;
static NCraftBalloonLabel* GCraftLabelFStringPtr = &GCraftLabelFString;

// a 5-byte jmp to Dst at Src, the rest of the NeedBytes nops
static UBOOL WriteJmp(BYTE* Src, void* Dst, INT NeedBytes)
{
    DWORD Old = 0;
    if (!Src || !Dst || NeedBytes < 5 ||
        !VirtualProtect(Src, (SIZE_T)NeedBytes, PAGE_EXECUTE_READWRITE, &Old))
        return 0;
    Src[0] = 0xE9;
    *(INT*)(Src + 1) = (INT)((BYTE*)Dst - (Src + 5));
    for (INT i = 5; i < NeedBytes; i++)
        Src[i] = 0x90;
    DWORD Ignored = 0;
    VirtualProtect(Src, (SIZE_T)NeedBytes, Old, &Ignored);
    FlushInstructionCache(GetCurrentProcess(), Src, (SIZE_T)NeedBytes);
    return 1;
}

static UBOOL SigTypeA(const BYTE* P)   // 66 8B 87 F8 01 00 00  mov ax, [edi+1F8h]
{
    return P[0] == 0x66 && P[1] == 0x8B && P[2] == 0x87 && P[3] == 0xF8 && P[4] == 0x01 &&
           P[5] == 0x00 && P[6] == 0x00;
}

static UBOOL SigTypeB(const BYTE* P)   // 66 8B BF F8 01 00 00  mov di, [edi+1F8h]
{
    return P[0] == 0x66 && P[1] == 0x8B && P[2] == 0xBF && P[3] == 0xF8 && P[4] == 0x01 &&
           P[5] == 0x00 && P[6] == 0x00;
}

static UBOOL SigBuyLabel(const BYTE* P)   // B9 2C 69 97 10  mov ecx, 1097692Ch
{
    return P[0] == 0xB9 && P[1] == 0x2C && P[2] == 0x69 && P[3] == 0x97 && P[4] == 0x10;
}

static UBOOL SigBuyColor(const BYTE* P)   // 68 A5 FE FD FF  push 0FFFDFEA5h
{
    return P[0] == 0x68 && P[1] == 0xA5 && P[2] == 0xFE && P[3] == 0xFD && P[4] == 0xFF;
}

// path A's buffer is "label\ntitle": the first line replaced, the rest (the title) kept
static void __stdcall ForceCraftBuffer(TCHAR* Buf)
{
    if (!Buf || IsBadWritePtr(Buf, 4))
        return;
    __try
    {
        TCHAR* NewLine = Buf;
        for (INT Guard = 0; *NewLine && *NewLine != '\n' && Guard < 512; Guard++)
            NewLine++;
        TCHAR Tail[384];
        INT TailLen = 0;
        if (*NewLine == '\n')
            while (NewLine[TailLen] && TailLen < 382)
            {
                Tail[TailLen] = NewLine[TailLen];
                TailLen++;
            }
        Tail[TailLen] = 0;
        INT i = 0;
        while (GCraftLabel[i] && i < 300)
        {
            Buf[i] = GCraftLabel[i];
            i++;
        }
        for (INT j = 0; j < TailLen && i < 380; j++)
            Buf[i++] = Tail[j];
        Buf[i] = 0;
        if (InterlockedIncrement(&GTextHits) == 1)
            nwf::Log("craftballoon", "label <Private Workshop - Craft> + keep title line");
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// the type sites, for each store owner drawn
static void __stdcall OnDrawUser(BYTE* User)
{
    InterlockedExchange(&GForceCraft, 0);
    if (!User)
        return;
    __try
    {
        INT Type = *(_WORD*)(User + OFF_StoreType) & 0xFF;
        if (Type != 0 && GTypeLogs > 0)
        {
            InterlockedDecrement(&GTypeLogs);
            nwf::Log("craftballoon", "draw storeType=%d user=%p", Type, User);
        }
        if (Type == STORETYPE_Manufacture)
        {
            InterlockedExchange(&GForceCraft, 1);
            if (InterlockedIncrement(&GApplyCount) == 1)
                nwf::Log("craftballoon", "type=5 craft visual armed");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

__declspec(naked) static void CaveTypeA()
{
    __asm {
        pushad
        push edi
        call OnDrawUser
        popad
        mov ax, word ptr [edi + 0x1F8]
        jmp dword ptr [GRetTypeA]
    }
}

__declspec(naked) static void CaveTypeB()
{
    __asm {
        pushad
        push edi
        call OnDrawUser
        popad
        mov di, word ptr [edi + 0x1F8]
        jmp dword ptr [GRetTypeB]
    }
}

__declspec(naked) static void CaveLabelA()
{
    __asm {
        cmp dword ptr [GForceCraft], 0
        je stock_a
        mov ecx, GCraftLabelFStringPtr
        inc dword ptr [GLabelHits]
        jmp dword ptr [GRetLabelA]
    stock_a:
        mov ecx, GBuyLabelStock
        jmp dword ptr [GRetLabelA]
    }
}

__declspec(naked) static void CaveLabelB()
{
    __asm {
        cmp dword ptr [GForceCraft], 0
        je stock_b
        mov ecx, GCraftLabelFStringPtr
        inc dword ptr [GLabelHits]
        jmp dword ptr [GRetLabelB]
    stock_b:
        mov ecx, GBuyLabelStock
        jmp dword ptr [GRetLabelB]
    }
}

// path A: [esp] is the text buffer (pushed): its first line becomes ours, then the cyan colour
__declspec(naked) static void CaveColorA()
{
    __asm {
        cmp dword ptr [GForceCraft], 0
        je stock_col_a
        pushad
        push dword ptr [esp + 32]
        call ForceCraftBuffer
        popad
        push dword ptr [GColorCraft]
        inc dword ptr [GColorHits]
        jmp dword ptr [GRetColorA]
    stock_col_a:
        push dword ptr [GColorBuy]
        jmp dword ptr [GRetColorA]
    }
}

// path B: [esp] is the label's text (no second line): ours instead, then the cyan colour
__declspec(naked) static void CaveColorB()
{
    __asm {
        cmp dword ptr [GForceCraft], 0
        je stock_col_b
        mov eax, GCraftLabelPtr
        mov dword ptr [esp], eax
        push dword ptr [GColorCraft]
        inc dword ptr [GColorHits]
        inc dword ptr [GTextHits]
        jmp dword ptr [GRetColorB]
    stock_col_b:
        push dword ptr [GColorBuy]
        jmp dword ptr [GRetColorB]
    }
}

// each group of sites only where its bytes are the expected ones; nothing when none is
static void Install()
{
    BYTE* Engine = (BYTE*)GetModuleHandleW(TEXT("Engine.dll"));
    if (!Engine)
    {
        nwf::Log("craftballoon", "Engine.dll not loaded");
        return;
    }
    BYTE* TypeA = Engine + RVA_LoadTypeA;
    BYTE* TypeB = Engine + RVA_LoadTypeB;
    BYTE* LabelA = Engine + RVA_BuyLabelA;
    BYTE* LabelB = Engine + RVA_BuyLabelB;
    BYTE* ColorA = Engine + RVA_BuyColorA;
    BYTE* ColorB = Engine + RVA_BuyColorB;
    UBOOL bType = SigTypeA(TypeA) && SigTypeB(TypeB);
    UBOOL bLabel = SigBuyLabel(LabelA) && SigBuyLabel(LabelB);
    UBOOL bColor = SigBuyColor(ColorA) && SigBuyColor(ColorB);
    if (!bType)
        nwf::Log("craftballoon", "type-load sig mismatch");
    if (!bLabel)
        nwf::Log("craftballoon", "buy-label sig mismatch");
    if (!bColor)
        nwf::Log("craftballoon", "buy-color sig mismatch");
    if (!bType && !bLabel && !bColor)
    {
        nwf::Log("craftballoon", "Engine.dll is not the expected C1 build: balloon left stock");
        return;
    }
    GBuyLabelStock = (DWORD)(Engine + RVA_BuyLabelFString);
    INT Ok = 0;
    if (bType)
    {
        if (WriteJmp(TypeA, (void*)&CaveTypeA, 7))
        {
            GRetTypeA = TypeA + 7;
            Ok++;
            nwf::Log("craftballoon", "siteA type-load OK");
        }
        if (WriteJmp(TypeB, (void*)&CaveTypeB, 7))
        {
            GRetTypeB = TypeB + 7;
            Ok++;
            nwf::Log("craftballoon", "siteB type-load OK");
        }
    }
    if (bLabel)
    {
        if (WriteJmp(LabelA, (void*)&CaveLabelA, 5))
        {
            GRetLabelA = LabelA + 5;
            Ok++;
            nwf::Log("craftballoon", "buyLabelA OK");
        }
        if (WriteJmp(LabelB, (void*)&CaveLabelB, 5))
        {
            GRetLabelB = LabelB + 5;
            Ok++;
            nwf::Log("craftballoon", "buyLabelB OK");
        }
    }
    if (bColor)
    {
        if (WriteJmp(ColorA, (void*)&CaveColorA, 5))
        {
            GRetColorA = ColorA + 5;
            Ok++;
            nwf::Log("craftballoon", "colorA text+cyan OK");
        }
        if (WriteJmp(ColorB, (void*)&CaveColorB, 5))
        {
            GRetColorB = ColorB + 5;
            Ok++;
            nwf::Log("craftballoon", "colorB text+cyan OK");
        }
    }
    if (!Ok)
        nwf::Log("craftballoon", "all sites FAIL");
    else
        nwf::Log("craftballoon", "install OK v6 (%d hooks, <Private Workshop - Craft> + title)", Ok);
}

void CraftBalloon_OnTick()
{
    if (GInstallTried)
        return;
    GInstallTried = 1;
    Install();
}

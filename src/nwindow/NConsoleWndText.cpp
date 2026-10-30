/*=============================================================================
    NConsoleWndText.cpp - the console's text services: the keyboard IME's kind, the checks
    of names and HTML / dialog input (bad words, characters the font lacks, '&'), the escaped
    newlines of the tool tips, the "$s1" and Korean particle ("%이") formatting of the system
    messages and the dialogs, and the clan and alliance crest uploads.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NViewerData.h"

#include <imm.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>

// system message colors and lines
enum
{
    STR_CrestPledgeFormat = 211,        // "Can only register 16*12 sized bmp files of 256 colors."
    STR_FileNotFound = 528,
    STR_CrestAllyFormat = 529,          // "You may only register 8x12 *.bmp files with 256 colors."
};
static const DWORD SYSMSG_COLOR = 0xFFB09B79;

// MakeSystemMessage's parameter types (names ours)
enum
{
    SYSMSGPARAM_Text,
    SYSMSGPARAM_Number,
    SYSMSGPARAM_Npc,                    // npc id + 1000000
    SYSMSGPARAM_Item,
    SYSMSGPARAM_Skill,                  // skill id, level
    SYSMSGPARAM_Castle,
};


/*-----------------------------------------------------------------------------
    The keyboard.
-----------------------------------------------------------------------------*/

// 0x10042810 - the kind of the keyboard layout's IME: -1 for a language NWindow does not
// handle, English 1; Korean 1 in alphanumeric mode and 0 in hangul (or without a conversion
// status); Japanese 2 and Chinese 4 .. 12 by input method (the traditional ones by
// GLanguageType), 1 while their IME is closed. The input context is never released.
INT NConsoleWnd::GetIMEType()
{
    char FileName[0x104];
    DWORD Conversion, Sentence;
    INT Type = 1;
    HKL Layout = GetKeyboardLayout(0);
    ImmGetIMEFileNameA(Layout, FileName, 0x103);
    HIMC Context = ImmGetContext(GViewportHWnd);
    switch (PRIMARYLANGID(LOWORD((DWORD_PTR)Layout)))
    {
    case LANG_KOREAN:
        Type = 0;
        if (ImmGetConversionStatus(Context, &Conversion, &Sentence) &&
            !(Conversion & IME_CMODE_NATIVE))
            Type = 1;
        return Type;
    case LANG_ENGLISH:
        return 1;
    case LANG_JAPANESE:
        Type = 2;
        break;
    case LANG_CHINESE:
    {
        WORD Device = HIWORD((DWORD_PTR)Layout);
        if (Device == 0xE002)
            Type = 4;
        else if (Device == 0xE006)
            Type = 5;
        else if (Device == 0xE008)
            Type = 6;
        else if (!lstrcmpiA(FileName, "CINTLGNT.IME"))
            Type = 4;
        else if (GLanguageType == 3 && Device == 0xE020)
            Type = 7;
        else if (Device == 0xE00E)
            Type = 8;
        else if (Device == 0xE001)
            Type = 9;
        else if (Device == 0xE004)
            Type = 10;
        else if (GLanguageType == 4 && (Device == 0xE020 || Device == 0xE025))
            Type = 12;
        break;
    }
    default:
        return -1;
    }
    if (!ImmGetOpenStatus(Context))
        Type = 1;
    return Type;
}
NW_HOOK_METHOD(0x10042810, NConsoleWnd, GetIMEType)

/*-----------------------------------------------------------------------------
    Checks of what the player types.
-----------------------------------------------------------------------------*/

// 0x100462c0 - 1 for an empty text or one with a character the default font lacks
UBOOL NConsoleWnd::HasInvalidChars(TCHAR* Text)
{
    if (!Text || !*Text)
        return 1;
    UL2Font** Fonts = GCanvas->L2Fonts;
    if (Fonts)
    {
        UL2Font* Font = Fonts[L2FONT_Normal];
        for (; *Text; Text++)
            if (!Font->GetChar(*Text))
                return 1;
    }
    return 0;
}
NW_HOOK_METHOD(0x100462c0, NConsoleWnd, HasInvalidChars)

// 0x10046320 - 0 when the text holds a '&'
UBOOL NConsoleWnd::HasNoAmpersand(TCHAR* Text)
{
    if (Text)
        for (; *Text; Text++)
            if (*Text == '&')
                return 0;
    return 1;
}
NW_HOOK_METHOD(0x10046320, NConsoleWnd, HasNoAmpersand)

// 0x10046360 - the characters the default font lacks taken out of the text (the rest moves
// up over each; the length is not updated, the moves stop at the old end)
void NConsoleWnd::RemoveInvalidChars(TCHAR* Text)
{
    if (!Text)
        return;
    UL2Font** Fonts = GCanvas->L2Fonts;
    UL2Font* Font = *Fonts;
    if (!Fonts)
        return;
    INT Len = wcslen(Text);
    INT Next = 1;
    for (TCHAR* P = Text; *P;)
    {
        if (Font->GetChar(*P))
        {
            Next++;
            P++;
            continue;
        }
        *P = 0;
        INT i = Next - 1;
        for (INT j = Next; j < Len; j++, i++)
            Text[i] = Text[i + 1];
        Text[i] = 0;
    }
}
NW_HOOK_METHOD(0x10046360, NConsoleWnd, RemoveInvalidChars)

// 0x1004a190 - the word filter: the characters the font lacks are taken out of the text,
// then each bad word found in it is replaced by "-_-ㆀ", in place (the text grows: the
// callers' buffers are large); 1 when one was
UBOOL NConsoleWnd::HasBadWord(TCHAR* Text)
{
    UBOOL bFound = 0;
    if (!Text)
        return 0;
    RemoveInvalidChars(Text);
    for (INT i = 0; i < GL2GameData.ObsceneData.Num(); i++)
    {
        FL2ObsceneData* Bad = GL2GameData.ObsceneData(i);
        if (!Bad)
            continue;
        const TCHAR* Word = *Bad->Word;
        INT WordLen = wcslen(*Bad->Word);
        if (!Word || !*Word)
            continue;
        for (TCHAR* Found = appStrstr(Text, Word); Found; Found = appStrstr(Text, Word))
        {
            FString Before;
            FString After;
            Before = Text;
            After = Text;
            INT Pos = Found - Text;
            INT RightLen = After.Len() - Pos - WordLen;
            if (Pos < 0 || RightLen < 0 || Pos > Before.Len() || RightLen > After.Len())
                break;
            appSprintf(Text, TEXT("%s%s%s"), *Before.Left(Pos), TEXT("-_-\x3180"),
                       *After.Right(RightLen));
            bFound = 1;
        }
    }
    return bFound;
}
NW_HOOK_METHOD(0x1004a190, NConsoleWnd, HasBadWord)

// 0x10056fa0 - a character or pet name: not empty, no bad word (which also takes the
// characters the font lacks out of it) and no NPC's name, whatever the case
UBOOL NConsoleWnd::IsValidName(TCHAR* Name)
{
    if (!Name || !*Name || HasBadWord(Name))
        return 0;
    NDataMap<NViewNpcData>* Npcs = GL2GameData.NpcTable;
    for (INT i = 0; i < Npcs->Pairs.Num(); i++)
    {
        TCHAR* NpcName = GL2GameData.GetNpcName(Npcs->Pairs(i).Key);
        if (NpcName && !_wcsicmp(Name, NpcName))
            return 0;
    }
    return 1;
}
NW_HOOK_METHOD(0x10056fa0, NConsoleWnd, IsValidName)

/*-----------------------------------------------------------------------------
    Formatting.
-----------------------------------------------------------------------------*/

// 0x10046de0 - the text with its escaped newlines ("\n", two characters) turned into CR LF
void NConsoleWnd::ConvertNewLines(TCHAR* Text, FString& Result)
{
    Result = Text;
    INT Len = Result.Len();
    INT Pos = Result.InStr(TEXT("\\n"));
    while (Pos != -1)
    {
        FString Line(Result);
        Result = Line.Left(Pos);
        Result += TEXT("\r\n");
        Result += Line.Mid(Pos + 2, Len - Pos - 2);
        Pos = Result.InStr(TEXT("\\n"));
    }
}
NW_HOOK_METHOD(0x10046de0, NConsoleWnd, ConvertNewLines)

// 0x10046120 - a system message's text with its parameters: the first "$..." (three
// characters) gets Param1, the second Param2 (a "$" left over is dropped); "%" and a particle
// puts the particle's form for the character before (anything else after "%" is dropped with
// it). Out may not be Format.
void NConsoleWnd::FormatSystemMessage(TCHAR* Out, const TCHAR* Format, const TCHAR* Param1,
                                      const TCHAR* Param2)
{
    INT Len = 0;
    if (Format)
        while (Format[Len])
            Len++;
    INT OutLen = 0;
    UBOOL bParam1 = 0;
    UBOOL bParam2 = 0;
    for (INT i = 0; i < Len; i++)
    {
        TCHAR C = Format[i];
        if (C == '$')
        {
            if (!bParam1 && Param1)
            {
                for (const TCHAR* P = Param1; *P; P++)
                    Out[OutLen++] = *P;
                i += 2;
                bParam1 = 1;
            }
            else if (!bParam2 && Param2)
            {
                for (const TCHAR* P = Param2; *P; P++)
                    Out[OutLen++] = *P;
                i += 2;
                bParam2 = 1;
            }
        }
        else if (C == '%')
        {
            TCHAR Form = ParticleForm(Format[i + 1], HasNoFinalConsonant(Out[OutLen - 1]));
            if (Form)
                Out[OutLen++] = Form;
            i++;
        }
        else
            Out[OutLen++] = C;
    }
    Out[OutLen] = 0;
}
NW_HOOK_METHOD(0x10046120, NConsoleWnd, FormatSystemMessage)

// 0x10047380 - the parameter's text: a copy of InText and its length; nothing for NULL
__declspec(noinline) void NSysMsgParam::Set(const TCHAR* InText)
{
    if (!InText)
        return;
    Text = new TCHAR[appStrlen(InText) + 1];
    appStrcpy(Text, InText);
    Len = appStrlen(Text);
}
NW_HOOK_METHOD(0x10047380, NSysMsgParam, Set)

// 0x100473d0 - the parameter's text: the number
__declspec(noinline) void NSysMsgParam::SetNumber(INT Value)
{
    TCHAR Number[1024];
    appSprintf(Number, TEXT("%d"), Value);
    Text = new TCHAR[appStrlen(Number) + 1];
    appStrcpy(Text, Number);
    Len = appStrlen(Text);
}
NW_HOOK_METHOD(0x100473d0, NSysMsgParam, SetNumber)

// 0x10057070 - a system message with its parameters into Out (FormatDateTime): the message
// id, the count of parameters, then each as its type and value(s) (SYSMSGPARAM_*), gathered
// in SysMsgParams as the engine's parameter packets are (AddSystemMessageParam). "$sN" puts
// the N-th parameter (no check below 1); "%" and a particle puts the particle's form for the
// last parameter put - the first "%" before any "$" reads the parameter before the array.
// A parameter without text gets the forms 이, 를, 는, 와. Out is left alone for a message
// the table does not have; the parameters are freed either way.
void NConsoleWnd::MakeSystemMessage(L2ParamStack& Params, TCHAR* Out)
{
    INT MsgID = (INT)Params.Top();
    INT Count = (INT)Params.Top();
    for (INT n = Count; n > 0; n--)
    {
        NSysMsgParam Param;
        Param.Text = NULL;
        Param.Len = 0;
        switch ((INT)Params.Top())
        {
        case SYSMSGPARAM_Text:
            Param.Set((const TCHAR*)Params.Top());
            break;
        case SYSMSGPARAM_Number:
            Param.SetNumber((INT)Params.Top());
            break;
        case SYSMSGPARAM_Npc:
            Param.Set(GL2GameData.GetNpcName((INT)Params.Top() - 1000000));
            break;
        case SYSMSGPARAM_Item:
            Param.Set(GetItemName((INT)Params.Top()));
            break;
        case SYSMSGPARAM_Skill:
        {
            INT SkillID = (INT)Params.Top();
            INT Level = (INT)Params.Top();
            Param.Set(GetSkillName(SkillID, Level));
            break;
        }
        case SYSMSGPARAM_Castle:
            Param.Set(GetCastleName((INT)Params.Top()));
            break;
        }
        SysMsgParams.AddItem(Param);
    }

    TCHAR Buffer[1024] = {0};
    INT NumParams = SysMsgParams.Num();
    FL2SystemMsgData& Line = GL2GameData.SystemMsg[MsgID];
    INT OutLen = 0;
    INT LastParam = -1;
    if (Line.Unknown0 && MsgID >= 0 && MsgID < 3000)
    {
        FL2SystemMsgData Msg = Line;
        const TCHAR* Format = *Msg.Message;
        for (INT i = 0; i < Msg.Message.Len(); i++)
        {
            TCHAR C = Format[i];
            if (C == '$')
            {
                INT Index = Format[i + 2] - '1';
                if (Index < NumParams && SysMsgParams(Index).Text)
                {
                    appStrcpy(&Buffer[OutLen], SysMsgParams(Index).Text);
                    OutLen += SysMsgParams(Index).Len;
                }
                LastParam = Index;
                i += 2;
            }
            else if (C == '%')
            {
                NSysMsgParam& Last = SysMsgParams(LastParam);
                TCHAR Particle = Format[i + 1];
                TCHAR Form;
                if (Last.Text)
                    Form = ParticleForm(Particle, HasNoFinalConsonant(Last.Text[Last.Len - 1]));
                else
                    Form = (Particle == 0xC774 || Particle == 0xC640) ? Particle
                                                                      : ParticleForm(Particle, 1);
                if (Form)
                    Buffer[OutLen++] = Form;
                i++;
            }
            else
                Buffer[OutLen++] = C;
        }
        Buffer[OutLen] = 0;
        appStrcpy(Out, Buffer);
    }
    for (INT j = 0; j < NumParams; j++)
    {
        NSysMsgParam& Param = SysMsgParams(j);
        if (Param.Text)
        {
            appFree(Param.Text);
            Param.Text = NULL;
        }
        Param.Len = 0;
    }
    SysMsgParams.Empty();
}
NW_HOOK_METHOD(0x10057070, NConsoleWnd, MakeSystemMessage)

/*-----------------------------------------------------------------------------
    Crests.
-----------------------------------------------------------------------------*/

// 0x100563c0 - the clan crest from a .bmp file (the path given): at most 0xC00 bytes, 16 x 12
// and 8 bits per pixel or less, sent as a 256-color 16 x 16 bmp whose 12 rows of 16 bytes
// are copied from the file as if it had 256 colors (the four rows left are 0xFF). A file that
// does not fit is not freed, nor its reader.
void NConsoleWnd::RegisterPledgeCrest(L2ParamStack& Params)
{
    FArchive* Reader = GFileManager->CreateFileReader((const TCHAR*)Params.Top(), 0, GNull);
    if (!Reader)
    {
        ShowSystemMessage(STR_FileNotFound, SYSMSG_COLOR);
        return;
    }
    INT Size = Reader->TotalSize();
    BYTE* File = (BYTE*)malloc(Size);
    Reader->Serialize(File, Size);
    if (File && (*(WORD*)File != 0x4D42 || ((BITMAPFILEHEADER*)File)->bfSize > 0xC00))
    {
        ShowSystemMessage(STR_CrestPledgeFormat, SYSMSG_COLOR);
        return;
    }
    BITMAPINFOHEADER* Info = (BITMAPINFOHEADER*)(File + sizeof(BITMAPFILEHEADER));
    if (Info && (Info->biWidth != 16 || Info->biHeight != 12 || Info->biBitCount > 8))
    {
        ShowSystemMessage(STR_CrestPledgeFormat, SYSMSG_COLOR);
        return;
    }
    BYTE* Crest = (BYTE*)malloc(0x536);
    appMemset(Crest, 0xFF, 0x536);
    ((BITMAPFILEHEADER*)File)->bfSize = 0x536;
    Info->biWidth = 16;
    Info->biHeight = 16;
    Info->biSizeImage = 0x100;
    appMemcpy(Crest, File, sizeof(BITMAPFILEHEADER));
    appMemcpy(Crest + 0xE, Info, sizeof(BITMAPINFOHEADER));
    appMemcpy(Crest + 0x36, File + 0x36, 0x400);
    for (INT Row = 0; Row < 12; Row++)
        appMemcpy(Crest + 0x436 + Row * 16, File + 0x436 + Row * 16, 16);
    Engine->NetHandler->RequestSetPledgeCrest(0x536, (ANSICHAR*)Crest);
    free(File);
    free(Crest);
    delete Reader;
}
NW_HOOK_METHOD(0x100563c0, NConsoleWnd, RegisterPledgeCrest)

// 0x10056570 - the alliance crest, as the clan's but 8 x 12 (sent 8 x 16, rows of 8 bytes)
void NConsoleWnd::RegisterAllyCrest(L2ParamStack& Params)
{
    FArchive* Reader = GFileManager->CreateFileReader((const TCHAR*)Params.Top(), 0, GNull);
    if (!Reader)
    {
        ShowSystemMessage(STR_FileNotFound, SYSMSG_COLOR);
        return;
    }
    INT Size = Reader->TotalSize();
    BYTE* File = (BYTE*)malloc(Size);
    Reader->Serialize(File, Size);
    if (File && (*(WORD*)File != 0x4D42 || ((BITMAPFILEHEADER*)File)->bfSize > 0xC00))
    {
        ShowSystemMessage(STR_CrestAllyFormat, SYSMSG_COLOR);
        return;
    }
    BITMAPINFOHEADER* Info = (BITMAPINFOHEADER*)(File + sizeof(BITMAPFILEHEADER));
    if (Info && (Info->biWidth != 8 || Info->biHeight != 12 || Info->biBitCount > 8))
    {
        ShowSystemMessage(STR_CrestAllyFormat, SYSMSG_COLOR);
        return;
    }
    BYTE* Crest = (BYTE*)malloc(0x4B6);
    appMemset(Crest, 0xFF, 0x4B6);
    ((BITMAPFILEHEADER*)File)->bfSize = 0x4B6;
    Info->biWidth = 8;
    Info->biHeight = 16;
    Info->biSizeImage = 0x80;
    appMemcpy(Crest, File, sizeof(BITMAPFILEHEADER));
    appMemcpy(Crest + 0xE, Info, sizeof(BITMAPINFOHEADER));
    appMemcpy(Crest + 0x36, File + 0x36, 0x400);
    for (INT Row = 0; Row < 12; Row++)
        appMemcpy(Crest + 0x436 + Row * 8, File + 0x436 + Row * 8, 8);
    Engine->NetHandler->RequestSetAllyCrest(0x4B6, (ANSICHAR*)Crest);
    free(File);
    free(Crest);
    delete Reader;
}
NW_HOOK_METHOD(0x10056570, NConsoleWnd, RegisterAllyCrest)

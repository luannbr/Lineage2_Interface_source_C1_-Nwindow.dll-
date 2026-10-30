/*=============================================================================
    NDialogInfo.cpp - dialog box parameters. Stock: 0x10005ab0 .. 0x10005be6,
    0x10006230 .. 0x1000628c.
=============================================================================*/
#include "nwindow/NDialogInfo.h"

// 0x10006230
NDialogInfo::NDialogInfo()
{
    ID = -1;
    Type = 3;
    Owner = NULL;
    bMainBox = 1;
    bInput = 0;
    bLargeAmount = 0;
    Text[0] = 0;
    Text2[0] = 0;
    Button1[0] = 0;
    Button2[0] = 0;
    Param1 = 0;
    Param2 = 0;
    Unknown1624 = 0;
    Unknown1628 = 0;
    bProgress = 0;
}
NW_HOOK_CTOR(0x10006230, NDialogInfo)

// 0x10005b70
void NDialogInfo::SetText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(Text, InText);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x10005b70, NDialogInfo, SetText)

// 0x10005b10
void NDialogInfo::SetSystemMessage(INT MsgID)
{
    if (*GL2GameData.SystemMsg[MsgID].Message)
        appStrcpy(Text, *GL2GameData.SystemMsg[MsgID].Message);
    else
        Text[0] = 0;
}
NW_HOOK_METHOD(0x10005b10, NDialogInfo, SetSystemMessage)

// 0x10005ab0
void NDialogInfo::SetButtons(const TCHAR* InButton1, const TCHAR* InButton2)
{
    if (InButton1 && *InButton1)
        appStrcpy(Button1, InButton1);
    else
        Button1[0] = 0;
    if (InButton2 && *InButton2)
        appStrcpy(Button2, InButton2);
    else
        Button2[0] = 0;
}
NW_HOOK_METHOD(0x10005ab0, NDialogInfo, SetButtons)

// 0x10005ba0
void NDialogInfo::SetParamText(const TCHAR* InText)
{
    if (InText && *InText)
        appStrcpy(ParamText, InText);
    else
        ParamText[0] = 0;
}
NW_HOOK_METHOD(0x10005ba0, NDialogInfo, SetParamText)

// 0x10005bd0
void NDialogInfo::SetParams(INT InParam1, INT InParam2)
{
    Param1 = InParam1;
    Param2 = InParam2;
}
NW_HOOK_METHOD(0x10005bd0, NDialogInfo, SetParams)

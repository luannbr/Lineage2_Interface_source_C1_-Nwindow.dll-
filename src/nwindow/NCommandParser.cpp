/*=============================================================================
    NCommandParser.cpp - chat command recognition. Stock: 0x100338e0 .. 0x1003397c.
=============================================================================*/
#include "nwindow/NCommandParser.h"

#if NW_FEATURES
// phase 6: ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp)
#include "features/ManufactureCmd.h"
#endif

// 0x100338e0
NCommandParser::NCommandParser()
{
    SetSperator(GCommandSeparators);
}
NW_HOOK_CTOR(0x100338e0, NCommandParser)

// 0x10033940 - a line starting with '/' (or 0xA3AF, the full-width slash of the Korean code
// page) is a command, looked up by its first token; anything else is chat
INT NCommandParser::GetCommand(TCHAR* Line)
{
    if (!Line)
        return NCMD_None;
    if (Line[0] != '/' && Line[0] != 0xA3AF)
        return NCMD_Chat;
#if NW_FEATURES
    // phase 6: ManufactureCmd (NWindow_C1 src/inject/ManufactureCmd.cpp): /dwarvenmanufacture,
    // which C1's command table lacks, sends RequestActionUse(37). SetString starts the parser
    // over, so the stock line below gets the same first token again.
    if (nwf::ManufactureCmd_OnCommand(SetString(Line)))
        return NCMD_None;
#endif
    return GL2GameData.GetCommandType(SetString(Line));
}
NW_HOOK_METHOD(0x10033940, NCommandParser, GetCommand)

// the stock linker's stubs for FParser imports called without dllimport
NW_HOOK_DTOR(0x10033930, FParser)
NW_HOOK_METHOD(0x10033980, FParser, GetNextToken)

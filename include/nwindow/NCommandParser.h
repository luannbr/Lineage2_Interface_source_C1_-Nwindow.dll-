#pragma once
// NCommandParser - the FParser that recognizes chat commands (name ours; 0x418 bytes, plain
// C++): NCChatWnd keeps one for its input line, NCGMPetitionWnd builds one on the stack.
// GetCommand gives the commandname-*.dat code of a "/command" line, the tokens after it come
// from GetNextToken.

#include "nwindow/NWindow.h"

// separators given to every parser (name ours); FParser::SetString puts its own back
#if NW_HOOK_MODE
#define GCommandSeparators (reinterpret_cast<TCHAR*>(::nw::Addr(0x1010c8f0)))   // L" ,\t\n"
#else
extern TCHAR GCommandSeparators[];
#endif

// GetCommand results: the action codes of commandname-*.dat (named after its actions) and
// the special ones
enum
{
    NCMD_Loc, NCMD_Petition, NCMD_PetitionCancel, NCMD_GMList, NCMD_Sit, NCMD_Stand, NCMD_Walk,
    NCMD_Run, NCMD_Attack, NCMD_AttackForce, NCMD_AttackStand, NCMD_Trade, NCMD_Target,
    NCMD_TargetNext, NCMD_PickUp, NCMD_Assist, NCMD_PartyInvite, NCMD_PartyLeave,
    NCMD_PartyDismiss, NCMD_Vendor, NCMD_PartyMatching, NCMD_SocialHello, NCMD_SocialVictory,
    NCMD_SocialCharge, NCMD_UseSkill, NCMD_UseSkillForce, NCMD_UseSkillStand, NCMD_UseShortcut,
    NCMD_UseShortcutForce, NCMD_UseShortcutStand, NCMD_AllyInvite, NCMD_AllyLeave, NCMD_AllyOust,
    NCMD_AllyDismiss, NCMD_AllyCrest, NCMD_AllyInfo, NCMD_FriendInvite, NCMD_FriendList,
    NCMD_FriendDel, NCMD_Buy, NCMD_AllyWarStart, NCMD_AllyWarStop, NCMD_AllyWarSurrender,
    NCMD_BlockList, NCMD_Block, NCMD_Unblock, NCMD_SocialNo, NCMD_SocialYes, NCMD_SocialBow,
    NCMD_SocialUnaware, NCMD_SocialWaiting, NCMD_SocialLaugh, NCMD_Escape,
    NCMD_DwarvenManufacture,                    // 0x35

    NCMD_None = -1,                             // no line, or an unknown /command
    NCMD_Builder = 0x10000000,                  // "//...": a builder command for the server
    NCMD_Exec = 0x10000001,                     // "///...": a console command run here
    NCMD_Chat = 0x20000000,                     // not a command: plain chat
};

class NCommandParser : public FParser
{
public:
    NCommandParser();                         // 0x100338e0
    // ~NCommandParser: FParser's (0x10033930 is the import thunk, hooked)

    INT GetCommand(TCHAR* Line);              // 0x10033940 (name ours)
};
NW_ASSERT_SIZE(NCommandParser, 0x418);

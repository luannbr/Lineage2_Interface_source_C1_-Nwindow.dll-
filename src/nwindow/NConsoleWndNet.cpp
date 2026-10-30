/*=============================================================================
    NConsoleWndNet.cpp - the console's requests to the server, and what they act on (the
    player, its target, what the network handler knows). Almost all of them only pass their
    arguments on to the engine's network handler (Engine->NetHandler, a UL2NetHandler whose 228
    virtuals are pure in Engine.dll and have no names yet: the slot is given at each call).
    Within a family the slots follow the protocol's packet order (e.g. 195..201 are the private
    store packets 0x73..0x79), which names the requests no chat command names.
=============================================================================*/
#include "nwindow/NConsoleWnd.h"
#include "nwindow/NCConsole.h"
#include "nwindow/NCChatWnd.h"
#include "nwindow/NCLobbyWnd.h"
#include "nwindow/NCLoginWnd.h"
#include "nwindow/NCLoadingWnd.h"
#include "nwindow/NCMessageWnd.h"
#include "nwindow/NCRestartMenuWnd.h"
#include "nwindow/NCItemWnd.h"

// interface sounds (FL2NetNotify::OnInterfacePlaySound; names ours)
enum { SOUND_UseShortcut = 1, SOUND_PickUp = 3 };

// the pawn the player's controller drives, and the actor under the mouse
static FORCEINLINE APawn* PlayerPawn(UEngine* Engine)
{
    return Engine->Client->Viewports(0)->Actor->Pawn;
}
static FORCEINLINE AActor* HoveredActor(UEngine* Engine)
{
    return static_cast<UGameEngine*>(Engine)->MouseHitActor;
}

// 0x10038540 - where the player's pawn stands
FVector NConsoleWnd::GetMyLocation()
{
    return PlayerPawn(Engine)->Location;
}
NW_HOOK_METHOD(0x10038540, NConsoleWnd, GetMyLocation)

// 0x10042b80 - a chat line (NetHandler slot 146: type, text, whisper target); with
// GIsOnlyL2Console (no server) it is only shown in the chat window. Returns 0.
UBOOL NConsoleWnd::SendChatMsgToServer(INT Type, TCHAR* Text, TCHAR* Target)
{
    guard(NConsoleWnd::SendChatMsgToServer);
    UL2NetHandler* NetHandler = Engine->NetHandler;
    if (GIsOnlyL2Console)
        static_cast<NCConsole*>(Desktop)->ChatWnd->AddChatMessage(Text, 0xFFFFFFFF, 0, 0);
    else
    {
        L2ParamStack Params(10);
        Params.PushBack((void*)(INT_PTR)Type);
        Params.PushBack(Text);
        Params.PushBack(Target);
        NetHandler->Say2(Params);
    }
    return 0;
    unguard;
}
NW_HOOK_METHOD(0x10042b80, NConsoleWnd, SendChatMsgToServer)

// 0x10042c70 - NetHandler slot 68
UBOOL NConsoleWnd::RequestAuthLogin(const TCHAR* Account, const TCHAR* Password)
{
    guard(NConsoleWnd::RequestAuthLogin);
    return Engine->NetHandler->RequestAuthLogin((TCHAR*)Account, (TCHAR*)Password) != 0;
    unguard;
}
NW_HOOK_METHOD(0x10042c70, NConsoleWnd, RequestAuthLogin)

// 0x10042d00 - NetHandler slot 147
void NConsoleWnd::RequestLogout()
{
    Engine->NetHandler->SendLogOutPacket();
}
NW_HOOK_METHOD(0x10042d00, NConsoleWnd, RequestLogout)

// 0x10042d10 - NetHandler slot 71; when sent, the loading screen replaces the character
// selection until the server answers (AcceptGameStart)
UBOOL NConsoleWnd::RequestGameStart(INT Index)
{
    guard(NConsoleWnd::RequestGameStart);
    if (!Engine->NetHandler->RequestGameStart(Index))
        return 0;
    LobbyWnd->HideWindow();
    MessageWnd->HideWindow();
    LoadingWnd->SetLoadingTexture(LoadingScreenID);
    LoadingWnd->ShowWindow();
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10042d10, NConsoleWnd, RequestGameStart)

// 0x10042e00 - NetHandler slot 72
UBOOL NConsoleWnd::RequestCharacterDelete(INT Index)
{
    guard(NConsoleWnd::RequestCharacterDelete);
    if (!Engine->NetHandler->RequestCharacterDelete(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10042e00, NConsoleWnd, RequestCharacterDelete)

// 0x10042eb0 - NetHandler slot 73
UBOOL NConsoleWnd::RequestCharacterRestore(INT Index)
{
    guard(NConsoleWnd::RequestCharacterRestore);
    if (!Engine->NetHandler->RequestCharacterRestore(Index))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10042eb0, NConsoleWnd, RequestCharacterRestore)

// 0x10042f60 - NetHandler slot 74
void NConsoleWnd::RequestNewCharacter()
{
    guard(NConsoleWnd::RequestNewCharacter);
    Engine->NetHandler->RequestNewCharacter();
    unguard;
}
NW_HOOK_METHOD(0x10042f60, NConsoleWnd, RequestNewCharacter)

// 0x10042fe0 - NetHandler slot 75 (the creation's choices, by value)
UBOOL NConsoleWnd::RequestCharacterCreate(CharacterInfo Info)
{
    guard(NConsoleWnd::RequestCharacterCreate);
    if (!Engine->NetHandler->RequestCharacterCreate(Info))
        return 0;
    SetState(L2CS_Waiting);
    return 1;
    unguard;
}
NW_HOOK_METHOD(0x10042fe0, NConsoleWnd, RequestCharacterCreate)

// 0x100430a0 - NetHandler slot 159
void NConsoleWnd::RequestRestart(L2ParamStack& Params)
{
    Engine->NetHandler->RequestRestart(Params);
}
NW_HOOK_METHOD(0x100430a0, NConsoleWnd, RequestRestart)

// 0x100430b0 - NetHandler slot 160
void NConsoleWnd::RequestRestartPoint(L2ParamStack& Params)
{
    Engine->NetHandler->RequestRestartPoint(Params);
}
NW_HOOK_METHOD(0x100430b0, NConsoleWnd, RequestRestartPoint)

// 0x10043200 - NetHandler slot 70
void NConsoleWnd::RequestServerLogin(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestServerLogin);
    Engine->NetHandler->RequestServerLogin(Params);
    unguard;
}
NW_HOOK_METHOD(0x10043200, NConsoleWnd, RequestServerLogin)

// 0x100439f0 - NetHandler slot 77
void NConsoleWnd::RequestItemList()
{
    guard(NConsoleWnd::RequestItemList);
    Engine->NetHandler->RequestItemList();
    unguard;
}
NW_HOOK_METHOD(0x100439f0, NConsoleWnd, RequestItemList)

// 0x10043a70 - NetHandler slot 86
void NConsoleWnd::RequestUseItem(INT ObjectID)
{
    guard(NConsoleWnd::RequestUseItem);
    Engine->NetHandler->RequestUseItem(ObjectID);
    unguard;
}
NW_HOOK_METHOD(0x10043a70, NConsoleWnd, RequestUseItem)

// 0x10043af0 - NetHandler slot 153
void NConsoleWnd::RequestUnEquipItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestUnEquipItem(Params);
}
NW_HOOK_METHOD(0x10043af0, NConsoleWnd, RequestUnEquipItem)

// 0x10043de0 - NetHandler slot 82
void NConsoleWnd::RequestTrade(INT CreatureID)
{
    guard(NConsoleWnd::RequestTrade);
    Engine->NetHandler->RequestTrade(CreatureID);
    unguard;
}
NW_HOOK_METHOD(0x10043de0, NConsoleWnd, RequestTrade)

// 0x10043e60 - NetHandler slot 83 (by value)
void NConsoleWnd::RequestAddTradeItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestAddTradeItem);
    Engine->NetHandler->RequestAddTradeItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x10043e60, NConsoleWnd, RequestAddTradeItem)

// 0x10043ef0 - NetHandler slot 84
void NConsoleWnd::RequestTradeDone(INT bConfirm)
{
    guard(NConsoleWnd::RequestTradeDone);
    Engine->NetHandler->RequestTradeDone(bConfirm);
    unguard;
}
NW_HOOK_METHOD(0x10043ef0, NConsoleWnd, RequestTradeDone)

// 0x10044020 - NetHandler slot 89
void NConsoleWnd::RequestBuyItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestBuyItem);
    Engine->NetHandler->RequestBuyItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x10044020, NConsoleWnd, RequestBuyItem)

// 0x100440a0 - NetHandler slot 88
void NConsoleWnd::RequestSellItem(L2ParamStack& Params)
{
    guard(NConsoleWnd::RequestSellItem);
    Engine->NetHandler->RequestSellItem(Params);
    unguard;
}
NW_HOOK_METHOD(0x100440a0, NConsoleWnd, RequestSellItem)

// 0x10044250 - NetHandler slot 138: the warehouse window's list in its deposit mode
void NConsoleWnd::RequestWarehouseDeposit(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseDepositList(Params);
}
NW_HOOK_METHOD(0x10044250, NConsoleWnd, RequestWarehouseDeposit)

// 0x10044260 - NetHandler slot 139: the same in its withdraw mode
void NConsoleWnd::RequestWarehouseWithdraw(L2ParamStack& Params)
{
    Engine->NetHandler->SendWareHouseWithdrawList(Params);
}
NW_HOOK_METHOD(0x10044260, NConsoleWnd, RequestWarehouseWithdraw)

// 0x10044750 - NetHandler slot 145
void NConsoleWnd::RequestMagicSkillUse(L2ParamStack& Params)
{
    Engine->NetHandler->RequestMagicSkillUse(Params);
}
NW_HOOK_METHOD(0x10044750, NConsoleWnd, RequestMagicSkillUse)

// 0x10044760 - NetHandler slot 154
void NConsoleWnd::RequestSkillList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSkillItem(Params);
}
NW_HOOK_METHOD(0x10044760, NConsoleWnd, RequestSkillList)

// 0x10044830 - NetHandler slot 140, with the item sound
void NConsoleWnd::RequestShortCutReg(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutReg(Params);
    Engine->OnInterfacePlaySound(SOUND_PickUp);
}
NW_HOOK_METHOD(0x10044830, NConsoleWnd, RequestShortCutReg)

// 0x10044860 - NetHandler slot 141, with its sound
void NConsoleWnd::UseShortcut(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutUse(Params);
    Engine->OnInterfacePlaySound(SOUND_UseShortcut);
}
NW_HOOK_METHOD(0x10044860, NConsoleWnd, UseShortcut)

// 0x10044890 - NetHandler slot 142
void NConsoleWnd::RequestShortCutDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShortCutDel(Params);
}
NW_HOOK_METHOD(0x10044890, NConsoleWnd, RequestShortCutDel)

// 0x10044940 - NetHandler slot 143
void NConsoleWnd::RequestTargetCancel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTargetCancel(Params);
}
NW_HOOK_METHOD(0x10044940, NConsoleWnd, RequestTargetCancel)

// 0x10044950 - NetHandler slot 157
void NConsoleWnd::RequestActionUse(L2ParamStack& Params)
{
    Engine->NetHandler->RequestActionUse(Params);
}
NW_HOOK_METHOD(0x10044950, NConsoleWnd, RequestActionUse)

// 0x10044960 - NetHandler slot 52 (target, origin, stand still): 0
void NConsoleWnd::RequestAttackForce(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Attack(ObjectID, Origin, 0);
}
NW_HOOK_METHOD(0x10044960, NConsoleWnd, RequestAttackForce)

// 0x100449a0 - the same, standing still
void NConsoleWnd::RequestAttackStand(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Attack(ObjectID, Origin, 1);
}
NW_HOOK_METHOD(0x100449a0, NConsoleWnd, RequestAttackStand)

// 0x100449e0 - NetHandler slot 57
void NConsoleWnd::RequestSocialAction(INT Action)
{
    Engine->NetHandler->SocialAction(Action);
}
NW_HOOK_METHOD(0x100449e0, NConsoleWnd, RequestSocialAction)

// 0x100449f0 - NetHandler slot 59
void NConsoleWnd::RequestChangeWaitType(UBOOL bStand)
{
    Engine->NetHandler->ChangeWaitType(bStand);
}
NW_HOOK_METHOD(0x100449f0, NConsoleWnd, RequestChangeWaitType)

// 0x10044a00 - NetHandler slot 58
void NConsoleWnd::RequestChangeMoveType(UBOOL bRun)
{
    Engine->NetHandler->ChangeMoveType(bRun);
}
NW_HOOK_METHOD(0x10044a00, NConsoleWnd, RequestChangeMoveType)

// 0x10044a30 - NetHandler slot 102
void NConsoleWnd::RequestPledgeMemberList()
{
    Engine->NetHandler->RequestPledgeMemberList();
}
NW_HOOK_METHOD(0x10044a30, NConsoleWnd, RequestPledgeMemberList)

// 0x10044a40 - NetHandler slot 103
void NConsoleWnd::RequestNet103(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCreatePledge(Params);
}
NW_HOOK_METHOD(0x10044a40, NConsoleWnd, RequestNet103)

// 0x10044a50 - NetHandler slot 104
void NConsoleWnd::RequestAnswerJoinPledge(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinPledge(Params);
}
NW_HOOK_METHOD(0x10044a50, NConsoleWnd, RequestAnswerJoinPledge)

// 0x10044a60 - NetHandler slot 105
void NConsoleWnd::RequestOustPledgeMember(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustPledgeMember(Params);
}
NW_HOOK_METHOD(0x10044a60, NConsoleWnd, RequestOustPledgeMember)

// 0x10044a70 - NetHandler slot 106
void NConsoleWnd::RequestNet106()
{
    Engine->NetHandler->RequestDismissPledge();
}
NW_HOOK_METHOD(0x10044a70, NConsoleWnd, RequestNet106)

// 0x10044a80 - NetHandler slot 109
void NConsoleWnd::RequestStartPledgeWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStartPledgeWar(Params);
}
NW_HOOK_METHOD(0x10044a80, NConsoleWnd, RequestStartPledgeWar)

// 0x10044a90 - NetHandler slot 110
void NConsoleWnd::RequestReplyStartPledgeWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestReplyStartPledgeWar(Params);
}
NW_HOOK_METHOD(0x10044a90, NConsoleWnd, RequestReplyStartPledgeWar)

// 0x10044aa0 - NetHandler slot 111
void NConsoleWnd::RequestStopPledgeWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStopPledgeWar(Params);
}
NW_HOOK_METHOD(0x10044aa0, NConsoleWnd, RequestStopPledgeWar)

// 0x10044ab0 - NetHandler slot 112
void NConsoleWnd::RequestReplyStopPledgeWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestReplyStopPledgeWar(Params);
}
NW_HOOK_METHOD(0x10044ab0, NConsoleWnd, RequestReplyStopPledgeWar)

// 0x10044ac0 - NetHandler slot 113
void NConsoleWnd::RequestSurrenderPledgeWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSurrenderPledgeWar(Params);
}
NW_HOOK_METHOD(0x10044ac0, NConsoleWnd, RequestSurrenderPledgeWar)

// 0x10044ad0 - NetHandler slot 115
void NConsoleWnd::RequestSurrenderPersonally(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSurrenderPersonally(Params);
}
NW_HOOK_METHOD(0x10044ad0, NConsoleWnd, RequestSurrenderPersonally)

// 0x10044ae0 - NetHandler slot 116
void NConsoleWnd::RequestGiveNickName(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGiveNickName(Params);
}
NW_HOOK_METHOD(0x10044ae0, NConsoleWnd, RequestGiveNickName)

// 0x10044af0 - NetHandler slot 107
void NConsoleWnd::RequestJoinPledge(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinPledge(Params);
}
NW_HOOK_METHOD(0x10044af0, NConsoleWnd, RequestJoinPledge)

// 0x10044b00 - NetHandler slot 108
void NConsoleWnd::RequestWithdrawalPledge()
{
    Engine->NetHandler->RequestWithDrawalPledge();
}
NW_HOOK_METHOD(0x10044b00, NConsoleWnd, RequestWithdrawalPledge)

// 0x10044cf0 - NetHandler slot 119
void NConsoleWnd::RequestAllyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinAlly(Params);
}
NW_HOOK_METHOD(0x10044cf0, NConsoleWnd, RequestAllyInvite)

// 0x10044d00 - NetHandler slot 120
void NConsoleWnd::RequestAnswerJoinAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinAlly(Params);
}
NW_HOOK_METHOD(0x10044d00, NConsoleWnd, RequestAnswerJoinAlly)

// 0x10044d10 - NetHandler slot 121
void NConsoleWnd::RequestOustAlly(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustAlly(Params);
}
NW_HOOK_METHOD(0x10044d10, NConsoleWnd, RequestOustAlly)

// 0x10044d20 - NetHandler slot 124
void NConsoleWnd::RequestAllyInfo()
{
    Engine->NetHandler->RequestAllyInfo();
}
NW_HOOK_METHOD(0x10044d20, NConsoleWnd, RequestAllyInfo)

// 0x10044d30 - NetHandler slot 122
void NConsoleWnd::RequestWithdrawAlly()
{
    Engine->NetHandler->RequestWithdrawAlly();
}
NW_HOOK_METHOD(0x10044d30, NConsoleWnd, RequestWithdrawAlly)

// 0x10044d40 - NetHandler slot 123
void NConsoleWnd::RequestDismissAlly()
{
    Engine->NetHandler->RequestDismissAlly();
}
NW_HOOK_METHOD(0x10044d40, NConsoleWnd, RequestDismissAlly)

// 0x10044ed0 - NetHandler slot 125
void NConsoleWnd::RequestFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendInvite(Params);
}
NW_HOOK_METHOD(0x10044ed0, NConsoleWnd, RequestFriendInvite)

// 0x10044ee0 - NetHandler slot 126
void NConsoleWnd::RequestFriendDel(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendDel(Params);
}
NW_HOOK_METHOD(0x10044ee0, NConsoleWnd, RequestFriendDel)

// 0x10044ef0 - NetHandler slot 128
void NConsoleWnd::RequestAnswerFriendInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestFriendAddReply(Params);
}
NW_HOOK_METHOD(0x10044ef0, NConsoleWnd, RequestAnswerFriendInvite)

// 0x10044f00 - NetHandler slot 127
void NConsoleWnd::RequestFriendList()
{
    Engine->NetHandler->RequestFriendList();
}
NW_HOOK_METHOD(0x10044f00, NConsoleWnd, RequestFriendList)

// 0x10044f10 - NetHandler slot 129
void NConsoleWnd::RequestBlock(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBlock(Params);
}
NW_HOOK_METHOD(0x10044f10, NConsoleWnd, RequestBlock)

// 0x10044fc0 - NetHandler slot 130
void NConsoleWnd::RequestPartyInvite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinParty(Params);
}
NW_HOOK_METHOD(0x10044fc0, NConsoleWnd, RequestPartyInvite)

// 0x10044fd0 - NetHandler slot 131
void NConsoleWnd::RequestAnswerJoinParty(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAnswerJoinParty(Params);
}
NW_HOOK_METHOD(0x10044fd0, NConsoleWnd, RequestAnswerJoinParty)

// 0x10044fe0 - NetHandler slot 132
void NConsoleWnd::RequestPartyLeave(L2ParamStack& Params)
{
    Engine->NetHandler->RequestWithDrawalParty(Params);
}
NW_HOOK_METHOD(0x10044fe0, NConsoleWnd, RequestPartyLeave)

// 0x10044ff0 - NetHandler slot 133
void NConsoleWnd::RequestPartyDismiss(L2ParamStack& Params)
{
    Engine->NetHandler->RequestOustPartyMember(Params);
}
NW_HOOK_METHOD(0x10044ff0, NConsoleWnd, RequestPartyDismiss)

// 0x10045000 - NetHandler slot 135
void NConsoleWnd::RequestPartyMatchConfig(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPartyMatchConfig(Params);
}
NW_HOOK_METHOD(0x10045000, NConsoleWnd, RequestPartyMatchConfig)

// 0x10045010 - NetHandler slot 136
void NConsoleWnd::RequestPartyMatchList(INT Status)
{
    Engine->NetHandler->RequestPartyMatchList(Status);
}
NW_HOOK_METHOD(0x10045010, NConsoleWnd, RequestPartyMatchList)

// 0x10045020 - NetHandler slot 137
void NConsoleWnd::RequestPartyMatchDetail(INT ObjectID)
{
    Engine->NetHandler->RequestPartyMatchDetail(ObjectID);
}
NW_HOOK_METHOD(0x10045020, NConsoleWnd, RequestPartyMatchDetail)

// 0x100452a0 - NetHandler slot 91
void NConsoleWnd::RequestLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestLinkHtml(Params);
}
NW_HOOK_METHOD(0x100452a0, NConsoleWnd, RequestLinkHtml)

// 0x100452b0 - NetHandler slot 93
void NConsoleWnd::RequestBypassToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBypassToServer(Params);
}
NW_HOOK_METHOD(0x100452b0, NConsoleWnd, RequestBypassToServer)

// 0x100452c0 - NetHandler slot 92
void NConsoleWnd::RequestBBSWrite(L2ParamStack& Params)
{
    Engine->NetHandler->RequestBBSWrite(Params);
}
NW_HOOK_METHOD(0x100452c0, NConsoleWnd, RequestBBSWrite)

// 0x100452d0 - NetHandler slot 163
void NConsoleWnd::RequestShowBoard(L2ParamStack& Params)
{
    Engine->NetHandler->RequestShowboard(Params);
}
NW_HOOK_METHOD(0x100452d0, NConsoleWnd, RequestShowBoard)

// 0x10045440 - NetHandler slot 94
void NConsoleWnd::RequestTutorialLinkHtml(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialLinkHtml(Params);
}
NW_HOOK_METHOD(0x10045440, NConsoleWnd, RequestTutorialLinkHtml)

// 0x10045450 - NetHandler slot 95
void NConsoleWnd::RequestTutorialPassCmdToServer(L2ParamStack& Params)
{
    Engine->NetHandler->RequestTutorialPassCmdToServer(Params);
}
NW_HOOK_METHOD(0x10045450, NConsoleWnd, RequestTutorialPassCmdToServer)

// 0x10045460 - NetHandler slot 96
void NConsoleWnd::RequestTutorialQuestionMark(INT ID)
{
    Engine->NetHandler->RequestTutorialQuestionMarkPressed(ID);
}
NW_HOOK_METHOD(0x10045460, NConsoleWnd, RequestTutorialQuestionMark)

// 0x100454d0 - the item nearest to the pet within Range (NetHandler slot 210, a float) is
// picked up by it (slot 189 with the item's object id, its first field). The stock also tests
// the address of the item's field +0x10 for NULL, which it never is.
void NConsoleWnd::PetPickUp(INT Range, INT Param)
{
    Item* Found = Engine->NetHandler->GetPetNearestItem((FLOAT)Range, Param);
    if (Found)
        Engine->NetHandler->RequestPetGetItem(Found->ObjectID);
}
NW_HOOK_METHOD(0x100454d0, NConsoleWnd, PetPickUp)

// 0x10045510 - NetHandler slot 190
void NConsoleWnd::RequestPetUseItem(INT ObjectID)
{
    Engine->NetHandler->RequestPetUseItem(ObjectID);
}
NW_HOOK_METHOD(0x10045510, NConsoleWnd, RequestPetUseItem)

// 0x10045520 - NetHandler slot 191
void NConsoleWnd::RequestGiveItemToPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGiveItemToPet(Params);
}
NW_HOOK_METHOD(0x10045520, NConsoleWnd, RequestGiveItemToPet)

// 0x10045530 - NetHandler slot 192
void NConsoleWnd::RequestGetItemFromPet(L2ParamStack& Params)
{
    Engine->NetHandler->RequestGetItemFromPet(Params);
}
NW_HOOK_METHOD(0x10045530, NConsoleWnd, RequestGetItemFromPet)

// 0x10045540 - NetHandler slot 194
void NConsoleWnd::RequestChangePetName(TCHAR* Name)
{
    Engine->NetHandler->RequestChangePetName(Name);
}
NW_HOOK_METHOD(0x10045540, NConsoleWnd, RequestChangePetName)

// 0x100456a0 - NetHandler slot 78
void NConsoleWnd::RequestPetition(TCHAR* Text)
{
    Engine->NetHandler->RequestPetition(Text);
}
NW_HOOK_METHOD(0x100456a0, NConsoleWnd, RequestPetition)

// 0x100456b0 - NetHandler slot 79
void NConsoleWnd::RequestPetitionCancel()
{
    Engine->NetHandler->RequestPetitionCancel();
}
NW_HOOK_METHOD(0x100456b0, NConsoleWnd, RequestPetitionCancel)

// 0x100456c0 - NetHandler slot 80
void NConsoleWnd::RequestGMList()
{
    Engine->NetHandler->RequestGMList();
}
NW_HOOK_METHOD(0x100456c0, NConsoleWnd, RequestGMList)

// The GM window's queries about a character: NetHandler slot 81 (name, what).

// 0x100456d0
void NConsoleWnd::RequestGMInventory(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 5);
}
NW_HOOK_METHOD(0x100456d0, NConsoleWnd, RequestGMInventory)

// 0x100456f0
void NConsoleWnd::RequestGMSkills(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 3);
}
NW_HOOK_METHOD(0x100456f0, NConsoleWnd, RequestGMSkills)

// 0x10045710
void NConsoleWnd::RequestGMStatus(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 1);
}
NW_HOOK_METHOD(0x10045710, NConsoleWnd, RequestGMStatus)

// 0x10045730
void NConsoleWnd::RequestGMQuests(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 4);
}
NW_HOOK_METHOD(0x10045730, NConsoleWnd, RequestGMQuests)

// 0x10045750
void NConsoleWnd::RequestGMWarehouse(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 6);
}
NW_HOOK_METHOD(0x10045750, NConsoleWnd, RequestGMWarehouse)

// 0x10045770
void NConsoleWnd::RequestGMPledge(TCHAR* Name)
{
    Engine->NetHandler->RequestGMCommand(Name, 2);
}
NW_HOOK_METHOD(0x10045770, NConsoleWnd, RequestGMPledge)

// The private stores: selling (NetHandler slots 195..201, packets 0x73..0x79) and buying
// (202..208, packets 0x90..0x96).

// 0x10045bc0 - "/vendor": manage the selling store
void NConsoleWnd::RequestVendor()
{
    Engine->NetHandler->RequestPrivateStoreManage();
}
NW_HOOK_METHOD(0x10045bc0, NConsoleWnd, RequestVendor)

// 0x10045bd0 - the managed selling list (item, amount, price...)
void NConsoleWnd::RequestSetPrivateStoreListSell(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreList(Params);
}
NW_HOOK_METHOD(0x10045bd0, NConsoleWnd, RequestSetPrivateStoreListSell)

// 0x10045be0 - the managing window closed without opening the store
void NConsoleWnd::RequestPrivateStoreManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreManageCancel();
}
NW_HOOK_METHOD(0x10045be0, NConsoleWnd, RequestPrivateStoreManageCancel)

// 0x10045bf0
void NConsoleWnd::RequestPrivateStoreQuitSell()
{
    Engine->NetHandler->RequestPrivateStoreQuit();
}
NW_HOOK_METHOD(0x10045bf0, NConsoleWnd, RequestPrivateStoreQuitSell)

// 0x10045c00
void NConsoleWnd::SetPrivateStoreMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreMsg(Text);
}
NW_HOOK_METHOD(0x10045c00, NConsoleWnd, SetPrivateStoreMsg)

// 0x10045c10 - buying from another player's store
void NConsoleWnd::RequestPrivateStoreBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x10045c10, NConsoleWnd, RequestPrivateStoreBuy)

// 0x10045c60 - "/buy": manage the buying store
void NConsoleWnd::RequestBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyManage();
}
NW_HOOK_METHOD(0x10045c60, NConsoleWnd, RequestBuy)

// 0x10045c70
void NConsoleWnd::RequestSetPrivateStoreListBuy(L2ParamStack& Params)
{
    Engine->NetHandler->SetPrivateStoreBuyList(Params);
}
NW_HOOK_METHOD(0x10045c70, NConsoleWnd, RequestSetPrivateStoreListBuy)

// 0x10045c80
void NConsoleWnd::RequestPrivateStoreBuyManageCancel()
{
    Engine->NetHandler->RequestPrivateStoreBuyManageCancel();
}
NW_HOOK_METHOD(0x10045c80, NConsoleWnd, RequestPrivateStoreBuyManageCancel)

// 0x10045c90
void NConsoleWnd::RequestPrivateStoreQuitBuy()
{
    Engine->NetHandler->RequestPrivateStoreBuyQuit();
}
NW_HOOK_METHOD(0x10045c90, NConsoleWnd, RequestPrivateStoreQuitBuy)

// 0x10045ca0
void NConsoleWnd::SetPrivateStoreBuyMsg(TCHAR* Text)
{
    Engine->NetHandler->SetPrivateStoreBuyMsg(Text);
}
NW_HOOK_METHOD(0x10045ca0, NConsoleWnd, SetPrivateStoreBuyMsg)

// 0x10045cb0 - selling to another player's buying store
void NConsoleWnd::RequestPrivateStoreSell(L2ParamStack& Params)
{
    Engine->NetHandler->SendPrivateStoreBuyBuyList(Params);
}
NW_HOOK_METHOD(0x10045cb0, NConsoleWnd, RequestPrivateStoreSell)

// 0x10045ed0 - NetHandler slot 224
void NConsoleWnd::RequestDwarvenManufacture()
{
    Engine->NetHandler->RequestRemainTime();
}
NW_HOOK_METHOD(0x10045ed0, NConsoleWnd, RequestDwarvenManufacture)

// 0x10045ef0 - "Previous" on the character selection: back to the login screen (NetHandler
// slots 147, the logout, and 168)
void NConsoleWnd::ReturnToLogin()
{
    LobbyWnd->ClearCharacters();
    LobbyWnd->HideWindow();
    Engine->NetHandler->SendLogOutPacket();
    Engine->NetHandler->AuthReconnect();
    LoginWnd->ResetWindows();
    LoginWnd->ShowWindow();
    MessageWnd->SetText(TEXT("Copyright 2002 NCsoft Corporation. All Rights Reserved."));
    MoveCameraByState(L2CS_None, L2CS_Login);
    SetState(L2CS_Login);
}
NW_HOOK_METHOD(0x10045ef0, NConsoleWnd, ReturnToLogin)

// 0x10046480 - NetHandler slot 30
INT NConsoleWnd::GetServerID()
{
    return Engine->NetHandler->GetServerID();
}
NW_HOOK_METHOD(0x10046480, NConsoleWnd, GetServerID)

// 0x10046490 - NetHandler slot 35: an item lying on the ground
Item* NConsoleWnd::FindItemByID(INT ObjectID)
{
    return Engine->NetHandler->GetItem(ObjectID);
}
NW_HOOK_METHOD(0x10046490, NConsoleWnd, FindItemByID)

// 0x100464a0 - NetHandler slot 31
PetInfo NConsoleWnd::GetPetInfo()
{
    return Engine->NetHandler->GetPetInfo();
}
NW_HOOK_METHOD(0x100464a0, NConsoleWnd, GetPetInfo)

// 0x100464c0 - NetHandler slot 32
User* NConsoleWnd::FindUserByName(TCHAR* Name)
{
    return Engine->NetHandler->GetUser(Name);
}
NW_HOOK_METHOD(0x100464c0, NConsoleWnd, FindUserByName)

// 0x100464d0 - NetHandler slot 33 (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::FindUserByID(INT ObjectID)
{
    return Engine->NetHandler->GetUser(ObjectID);
}
NW_HOOK_METHOD(0x100464d0, NConsoleWnd, FindUserByID)

// 0x100464e0 - NetHandler slot 34
FServerStaticObject* NConsoleWnd::FindStaticObjectInfo(INT CreatureID)
{
    return Engine->NetHandler->GetStaticObject(CreatureID);
}
NW_HOOK_METHOD(0x100464e0, NConsoleWnd, FindStaticObjectInfo)

// 0x100464f0 - the pawn under the mouse; with bCheck, only a player's (its User record,
// NetHandler slot 33, without Unknown8)
AActor* NConsoleWnd::GetTargetedActor(UBOOL bCheck)
{
    guard(NConsoleWnd::GetTargetedActor);
    if (!Engine || !HoveredActor(Engine) || !HoveredActor(Engine)->IsA(APawn::StaticClass()))
        return NULL;
    AActor* Actor = HoveredActor(Engine);
    if (bCheck)
    {
        User* Info = Engine->NetHandler->GetUser(Actor->CreatureID);
        if (!Info || Info->Unknown8)
            return NULL;
    }
    return Actor;
    unguard;
}
NW_HOOK_METHOD(0x100464f0, NConsoleWnd, GetTargetedActor)

// 0x10046620 - NetHandler slot 38
PledgeInfo* NConsoleWnd::GetPledgeInfo(INT PledgeID)
{
    return Engine->NetHandler->GetPledgeInfo(PledgeID);
}
NW_HOOK_METHOD(0x10046620, NConsoleWnd, GetPledgeInfo)

// 0x10046630 - NetHandler slot 39
UTexture* NConsoleWnd::GetPledgeCrestTexture(INT PledgeID)
{
    return Engine->NetHandler->GetPledgeCrestTex(PledgeID);
}
NW_HOOK_METHOD(0x10046630, NConsoleWnd, GetPledgeCrestTexture)

// 0x10046640 - NetHandler slot 40
UTexture* NConsoleWnd::GetAllyCrestTexture(INT AllyID)
{
    return Engine->NetHandler->GetAllyCrestTex(AllyID);
}
NW_HOOK_METHOD(0x10046640, NConsoleWnd, GetAllyCrestTexture)

// 0x10048200 - NetHandler slot 87 (by value)
void NConsoleWnd::RequestDropItem(NItemRequest Request, FVector Location)
{
    guard(NConsoleWnd::RequestDropItem);
    Engine->NetHandler->RequestDropItem(Request, Location);
    unguard;
}
NW_HOOK_METHOD(0x10048200, NConsoleWnd, RequestDropItem)

// The actions on a target: NetHandler slot 53 (object id, where the player stands, Shift held)
// selects it, slot 52 (same) attacks it.

// 0x100489b0 - a click on the player's status window
void NConsoleWnd::TargetMyself()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(PlayerPawn(Engine)->CreatureID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x100489b0, NConsoleWnd, TargetMyself)

// 0x10048a40
void NConsoleWnd::RequestTarget(INT ObjectID)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return;
    FVector Origin = Pawn->Location;
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048a40, NConsoleWnd, RequestTarget)

// 0x10048ac0 - the item nearest to the player within Range (NetHandler slot 211, a float); the
// stock tests the address of its field +0x10 too, never NULL
void NConsoleWnd::RequestPickUp(INT Range, INT ObjectID)
{
    Item* Found = Engine->NetHandler->GetNearestItem((FLOAT)Range, ObjectID);
    if (!Found)
        return;
    Engine->NetHandler->Action(Found->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048ac0, NConsoleWnd, RequestPickUp)

// 0x10048b50 - the next creature within Range after CurrentID (NetHandler slot 214)
void NConsoleWnd::RequestTargetNext(INT Range, INT CurrentID)
{
    User* Next = Engine->NetHandler->GetNextEnemy((FLOAT)Range, CurrentID);
    if (!Next || !Next->Pawn)
        return;
    Engine->NetHandler->Action(Next->ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048b50, NConsoleWnd, RequestTargetNext)

// 0x10048be0 - what the party member targets; its User record is not tested for NULL
void NConsoleWnd::AssistMember(INT ObjectID, FVector Origin)
{
    User* Member = FindUserByID(ObjectID);
    if (!Member || !Member->Pawn)
        return;
    AController* Controller = Member->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    User* Target = FindUserByID(Controller->Target->CreatureID);
    Engine->NetHandler->Action(Target->ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048be0, NConsoleWnd, AssistMember)

// 0x10048c70 - a click on the pet / summon status window
void NConsoleWnd::RequestAction(INT ObjectID, FVector Origin)
{
    Engine->NetHandler->Action(ObjectID, Origin,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048c70, NConsoleWnd, RequestAction)

// 0x10048cd0 - attack with Ctrl held (slot 52), else select (slot 53)
void NConsoleWnd::RequestAttack(INT ObjectID, FVector Origin)
{
    if (IsCtrlDown())
        Engine->NetHandler->Attack(ObjectID, Origin,
                                   (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
    else
        Engine->NetHandler->Action(ObjectID, Origin,
                                   (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10048cd0, NConsoleWnd, RequestAttack)

// 0x1004ce70 - the player's User record (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetMyUser()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (!Pawn)
        return NULL;
    return FindUserByID(Pawn->CreatureID);
}
NW_HOOK_METHOD(0x1004ce70, NConsoleWnd, GetMyUser)

// 0x1004cea0 - the User record of the player's target (not inlined: the stock calls it)
__declspec(noinline) User* NConsoleWnd::GetSelectedActor()
{
    guard(NConsoleWnd::GetSelectedActor);
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return FindUserByID(Pawn->Controller->Target->CreatureID);
    return NULL;
    unguard;
}
NW_HOOK_METHOD(0x1004cea0, NConsoleWnd, GetSelectedActor)

// 0x1004cf50 - the player's target as an actor (doors and the other static objects have no
// User record)
AActor* NConsoleWnd::GetSelectedStaticActor()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return Pawn->Controller->Target;
    return NULL;
}
NW_HOOK_METHOD(0x1004cf50, NConsoleWnd, GetSelectedStaticActor)

// 0x1004cf90 - the player has a target (the shortcut keys)
UBOOL NConsoleWnd::HasSelectedTarget()
{
    APawn* Pawn = PlayerPawn(Engine);
    if (Engine && Pawn && Pawn->Controller && Pawn->Controller->Target)
        return 1;
    return 0;
}
NW_HOOK_METHOD(0x1004cf90, NConsoleWnd, HasSelectedTarget)

// 0x1004d250 - move there (NetHandler slot 90: the player's id, 3 and the point in whole units,
// or 2 and no height)
UBOOL NConsoleWnd::MoveToLocation(FVector Loc, UBOOL bNoZ)
{
    APawn* Pawn = PlayerPawn(Engine);
    if (bNoZ)
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 2, (INT)Loc.X, (INT)Loc.Y, 0);
    else
        Engine->NetHandler->RequestTeleport(Pawn->CreatureID, 3, (INT)Loc.X, (INT)Loc.Y,
                                            (INT)Loc.Z);
    return 1;
}
NW_HOOK_METHOD(0x1004d250, NConsoleWnd, MoveToLocation)

// 0x10051d00 - what the target targets; its User record is not tested for NULL
void NConsoleWnd::RequestAssist()
{
    User* Target = GetSelectedActor();
    if (!Target || !Target->Pawn)
        return;
    AController* Controller = Target->Pawn->Controller;
    if (!Controller || !Controller->Target)
        return;
    INT ObjectID = FindUserByID(Controller->Target->CreatureID)->ObjectID;
    Engine->NetHandler->Action(ObjectID, PlayerPawn(Engine)->Location,
                               (UBOOL)Engine->Client->Viewports(0)->Input->KeyDown(IK_Shift));
}
NW_HOOK_METHOD(0x10051d00, NConsoleWnd, RequestAssist)


/*-----------------------------------------------------------------------------
    The virtual requests: UL2ConsoleWnd slots the engine leaves pure and the console's own
    (272..284), which the windows call through the vtable.
-----------------------------------------------------------------------------*/

// 0x10043180 - NetHandler slot 69
void NConsoleWnd::RequestServerList()
{
    guard(NConsoleWnd::RequestServerList);
    Engine->NetHandler->RequestServerList();
    unguard;
}
NW_HOOK_METHOD(0x10043180, NConsoleWnd, RequestServerList)

// 0x10043820 - the answer to a resurrection offer (NetHandler slot 76: the answer and the
// offer's two values); refused, the restart menu comes back
void NConsoleWnd::AnswerReviveRequest(UBOOL bYes, INT Param1, INT Param2)
{
    if (!bYes)
        RestartMenuWnd->ShowWindow();
    L2ParamStack Params(10);
    Params.PushBack((void*)(INT_PTR)bYes);
    Params.PushBack((void*)(INT_PTR)Param1);
    Params.PushBack((void*)(INT_PTR)Param2);
    Engine->NetHandler->ReviveReply(Params);
}
NW_HOOK_METHOD(0x10043820, NConsoleWnd, AnswerReviveRequest)

// 0x10043c50 - NetHandler slot 171: the item the enchant scroll goes on
void NConsoleWnd::RequestEnchantItem(INT ObjectID)
{
    guard(NConsoleWnd::RequestEnchantItem);
    Engine->NetHandler->RequestEnchantItem(ObjectID);
    unguard;
}
NW_HOOK_METHOD(0x10043c50, NConsoleWnd, RequestEnchantItem)

// 0x10043d40 - NetHandler slot 165
void NConsoleWnd::RequestDestroyItem(NItemRequest Request)
{
    guard(NConsoleWnd::RequestDestroyItem);
    Engine->NetHandler->RequestDestroyItem(Request);
    unguard;
}
NW_HOOK_METHOD(0x10043d40, NConsoleWnd, RequestDestroyItem)

// 0x10043dd0 - NetHandler slot 166
void NConsoleWnd::RequestCrystallizeItem(INT ObjectID, INT Count)
{
    Engine->NetHandler->RequestCrystallizeItem(ObjectID, Count);
}
NW_HOOK_METHOD(0x10043dd0, NConsoleWnd, RequestCrystallizeItem)

// 0x10043f70 - NetHandler slot 156: the answer to a trade offer
void NConsoleWnd::AnswerTradeRequest(L2ParamStack& Params)
{
    Engine->NetHandler->AnswerTradeRequest(Params);
}
NW_HOOK_METHOD(0x10043f70, NConsoleWnd, AnswerTradeRequest)

// 0x10044380 - NetHandler slot 169
UBOOL NConsoleWnd::RequestQuestList()
{
    guard(NConsoleWnd::RequestQuestList);
    if (GIsOnlyL2Console)
        return 0;
    return Engine->NetHandler->RequestQuestList();
    unguard;
}
NW_HOOK_METHOD(0x10044380, NConsoleWnd, RequestQuestList)

// 0x10044420 - NetHandler slot 170
void NConsoleWnd::RequestDestroyQuest(INT QuestID)
{
    guard(NConsoleWnd::RequestDestroyQuest);
    Engine->NetHandler->RequestDestroyQuest(QuestID);
    unguard;
}
NW_HOOK_METHOD(0x10044420, NConsoleWnd, RequestDestroyQuest)

// 0x10044710 - NetHandler slot 172: what a skill of the trainer needs
void NConsoleWnd::RequestAcquireSkillInfo(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkillInfo(Params);
}
NW_HOOK_METHOD(0x10044710, NConsoleWnd, RequestAcquireSkillInfo)

// 0x10044720 - NetHandler slot 173
void NConsoleWnd::RequestAcquireSkill(L2ParamStack& Params)
{
    Engine->NetHandler->RequestAcquireSkill(Params);
}
NW_HOOK_METHOD(0x10044720, NConsoleWnd, RequestAcquireSkill)

// 0x10044e10 - NetHandler slot 175
void NConsoleWnd::RequestStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x10044e10, NConsoleWnd, RequestStartAllianceWar)

// 0x10044e20 - NetHandler slot 176
void NConsoleWnd::RequestReplyStartAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStartAllianceWar(Params);
}
NW_HOOK_METHOD(0x10044e20, NConsoleWnd, RequestReplyStartAllianceWar)

// 0x10044e30 - NetHandler slot 177
void NConsoleWnd::RequestStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x10044e30, NConsoleWnd, RequestStopAllianceWar)

// 0x10044e40 - NetHandler slot 178
void NConsoleWnd::RequestReplyStopAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->ReplyStopAllianceWar(Params);
}
NW_HOOK_METHOD(0x10044e40, NConsoleWnd, RequestReplyStopAllianceWar)

// 0x10044e50 - NetHandler slot 179
void NConsoleWnd::RequestSurrenderAllianceWar(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSurrenderAllianceWar(Params);
}
NW_HOOK_METHOD(0x10044e50, NConsoleWnd, RequestSurrenderAllianceWar)

// 0x10044e60 - NetHandler slot 187: the freight
void NConsoleWnd::RequestPackageSend(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSend(Params);
}
NW_HOOK_METHOD(0x10044e60, NConsoleWnd, RequestPackageSend)

// 0x10044e70 - NetHandler slot 188
void NConsoleWnd::RequestPackageSendableItemList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestPackageSendableItemList(Params);
}
NW_HOOK_METHOD(0x10044e70, NConsoleWnd, RequestPackageSendableItemList)

// 0x10044f20 - NetHandler slot 180 (no caller in NWindow)
void NConsoleWnd::vfn184(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeInfo(Params);
}
NW_HOOK_METHOD(0x10044f20, NConsoleWnd, vfn184)

// 0x10044f30 - NetHandler slot 181
void NConsoleWnd::RequestSiegeAttackerList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeAttackerList(Params);
}
NW_HOOK_METHOD(0x10044f30, NConsoleWnd, RequestSiegeAttackerList)

// 0x10044f40 - NetHandler slot 182
void NConsoleWnd::RequestSiegeDefenderList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestCastleSiegeDefenderList(Params);
}
NW_HOOK_METHOD(0x10044f40, NConsoleWnd, RequestSiegeDefenderList)

// 0x10044f50 - NetHandler slot 183
void NConsoleWnd::RequestJoinSiege(L2ParamStack& Params)
{
    Engine->NetHandler->RequestJoinCastleSiege(Params);
}
NW_HOOK_METHOD(0x10044f50, NConsoleWnd, RequestJoinSiege)

// 0x10044f60 - NetHandler slot 184
void NConsoleWnd::RequestConfirmSiegeWaitingList(L2ParamStack& Params)
{
    Engine->NetHandler->RequestConfirmCastleSiegeWaitingList(Params);
}
NW_HOOK_METHOD(0x10044f60, NConsoleWnd, RequestConfirmSiegeWaitingList)

// 0x10044f70 - NetHandler slot 185: the siege date
void NConsoleWnd::RequestSetSiegeTime(L2ParamStack& Params)
{
    Engine->NetHandler->RequestSetCastleSiegeTime(Params);
}
NW_HOOK_METHOD(0x10044f70, NConsoleWnd, RequestSetSiegeTime)

// 0x10045470 - the tutorial's client events: each pending one (TutorialEnableClientEvent) is
// sent once (NetHandler slot 97)
void NConsoleWnd::SendTutorialEvent(INT Event)
{
    if (TutorialEvents & Event)
    {
        Engine->NetHandler->RequestTutorialClientEvent(Event);
        TutorialEvents ^= Event;
    }
}
NW_HOOK_METHOD(0x10045470, NConsoleWnd, SendTutorialEvent)

// 0x10045d40 - NetHandler slot 186
void NConsoleWnd::RequestBuyVIPItem(L2ParamStack& Params)
{
    Engine->NetHandler->RequestMultiSellChoose(Params);
}
NW_HOOK_METHOD(0x10045d40, NConsoleWnd, RequestBuyVIPItem)

// 0x10045ee0 - NetHandler slot 164: a GM command
void NConsoleWnd::BuilderCommand(L2ParamStack& Params)
{
    Engine->NetHandler->SendBypassBuildCmd(Params);
}
NW_HOOK_METHOD(0x10045ee0, NConsoleWnd, BuilderCommand)

// 0x10047060 - NetHandler slot 174 (no caller in NWindow). The stock tail-jumps into the slot
// with Params still on the stack: the headers it was built with gave the slot one argument,
// but the client's UNetworkHandler::RequestSkillCoolTime takes none (and pops none). Called
// through the slot to keep that, not through the declaration.
void NConsoleWnd::vfn176(L2ParamStack& Params)
{
    nw::VirtualCall<void>(Engine->NetHandler, 174, &Params);
}
NW_HOOK_METHOD(0x10047060, NConsoleWnd, vfn176)

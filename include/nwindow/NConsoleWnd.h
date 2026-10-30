#pragma once
// NConsoleWnd - the game console (UClass "ConsoleWnd", CLASS_Config, sizeof 0x3740): implements
// UL2ConsoleWnd for the engine, owns the window manager and every window, runs the timers.
// The whole vtable is declared (285 slots, 0x100e2978): the 228 slots the engine leaves pure,
// the engine's own and 13 of the console's; docs/notas/NConsoleWnd.md maps them.

#include "nwindow/NCObject.h"

class NCWnd;
class NCConsole;
class NCVirtualWndMain;
class NCDebugWnd;
struct NDialogInfo;
struct CharacterInfo;
class NCHtmlViewer;
class NCRestartMenuWnd;
class NCOptionWnd;
class NCLoginWnd;
class NCLobbyWnd;
class NCPawnCreateWnd;
class NCLoadingWnd;
class NCMessageWnd;
class NCDialogBox;
struct NItemRequest;

// one SetTimer record (NConsoleWnd::NSetTimer)
struct NTimer
{
    UINT ID;              // +0x00
    UINT Elapse;          // +0x04 milliseconds
    UBOOL bKilled;        // +0x08 set by NKillTimer, dropped by the next Tick
    FLOAT Elapsed;        // +0x0C seconds since the last WM_TIMER
    NCObject* Owner;      // +0x10 gets WindowProc(WM_TIMER, ID, 0)
};
NW_ASSERT_SIZE(NTimer, 0x14);

// a parameter of the system message being received (names ours; AddSystemMessageParam): its
// text, made from a string or a number
struct NSysMsgParam
{
    TCHAR* Text;          // +0x00 GMalloc'd copy
    INT Len;              // +0x04 appStrlen(Text)

    void Set(const TCHAR* InText);    // 0x10047380 a copy of InText; nothing for NULL
    void SetNumber(INT Value);        // 0x100473d0 "%d"
};
NW_ASSERT_SIZE(NSysMsgParam, 0x8);

// The Korean particles of the system messages ("%" and a particle after a "$sN"): whether
// a Hangul syllable ends without a final consonant (any other character counts so; stock
// 0x100427d0, which takes the character in AX: not hooked, its own calling convention),
// and the particle's form after it: i / ga, eul / reul, eun / neun, gwa / wa (0 for
// another character: nothing is put). MakeSystemMessage and EndSystemMessageParam.
FORCEINLINE UBOOL HasNoFinalConsonant(TCHAR C)
{
    if (C < 0xAC00 || C > 0xD7A3)
        return 1;
    return (C - 0xAC00) % (21 * 28) % 28 == 0;
}
FORCEINLINE TCHAR ParticleForm(TCHAR Particle, UBOOL bNoFinal)
{
    switch (Particle)
    {
    case 0xC774: return bNoFinal ? 0xAC00 : 0xC774;
    case 0xC744: return bNoFinal ? 0xB97C : 0xC744;
    case 0xC740: return bNoFinal ? 0xB294 : 0xC740;
    case 0xC640: return bNoFinal ? 0xC640 : 0xACFC;
    }
    return 0;
}

class NConsoleWnd : public UL2ConsoleWnd
{
    NW_DECLARE_CLASS(NConsoleWnd, UL2ConsoleWnd, CLASS_Config, 0x1012d7c8)

    UBOOL bInitialized;               // +0x36B8 (name ours) set at the end of Initialize
    NCVirtualWndMain* WndMain;        // +0x36BC (NConsoleWnd::SetCursor)
    // the windows of the login and character stages (Initialize; names ours)
    NCRestartMenuWnd* RestartMenuWnd; // +0x36C0 after dying
    NCOptionWnd* OptionWnd;           // +0x36C4
    NCLoginWnd* LoginWnd;             // +0x36C8
    NCLobbyWnd* LobbyWnd;             // +0x36CC the character selection
    NCPawnCreateWnd* PawnCreateWnd;   // +0x36D0 the character creation
    NCLoadingWnd* LoadingWnd;         // +0x36D4
    NCMessageWnd* MessageWnd;         // +0x36D8 the line under the login screens
    NCWnd* Desktop;                   // +0x36DC (name ours) the screen-sized NCConsole window,
                                      //   root of the in-game UI (Initialize)
    NCDebugWnd* DebugWnd;             // +0x36E0 (NCObject::Debugf)
    NCWnd* UnusedWnd;                 // +0x36E4 (name ours) a top window never created
                                      //   (Finalize destroys it)
    NCDialogBox* DialogBox;           // +0x36E8 (name ours) the login stages' dialog box
    NCWnd* PetitionWnd;               // +0x36EC (name ours) top-level NCPetitionWnd, the GM
                                      //   chat window (Initialize)
    TArray<NSysMsgParam> SysMsgParams;// +0x36F0 (name ours) of the system message being received
    TArray<NTimer> Timers;            // +0x36FC
    INT MouseWheelDelta;              // +0x3708 (name ours) of the last wheel event
    FLOAT DeltaTime;                  // +0x370C seconds of the last Tick
    UBOOL bCtrlDown;                  // +0x3710 (name ours) PressCtrl / ReleaseCtrl
    UBOOL bCtrlDownAlt;               // +0x3714 (name ours) IsCtrlDown's other half, never set
    UBOOL bSuspended;                 // +0x3718 (name ours) no painting, no input; never set
    UBOOL bWheelHandled;              // +0x371C (name ours) the last wheel step went to a window:
                                      //   its release is eaten (MasterConsoleEventProcess)
    INT Unknown3720;                  // +0x3720
    INT TutorialEvents;               // +0x3724 (name ours) the pending tutorial event bits
    INT LoadingScreenID;              // +0x3728 (name ours) shown when entering the game (10001)
    INT PartyItemDistribution;        // +0x372C (name ours) sent with party invitations
    UBOOL bNoChatOnEnter;             // +0x3730 (name ours) Enter leaves the chat line alone
                                      //   (HookShortCutKey); never set
    NCWnd* PawnViewerWnd[3];          // +0x3734 (name ours) the pawn viewer's windows

    NConsoleWnd();                                                   // 0x100557c0
    NCConsole* GetDesktop() const;                                   // Desktop as what it is
                                                                     //   (defined in NCConsole.h)

    // Decompiled in NConsoleWndNet.cpp / NConsoleWndData.cpp, not in the groups of the windows
    // below (names ours: the requests after the protocol's packets)
    void RequestBypassToServer(L2ParamStack& Params);                // 0x100452b0 HTML "bypass"
    void RequestTutorialPassCmdToServer(L2ParamStack& Params);       // 0x10045450 the tutorial's
    Item* FindItemByID(INT ObjectID);                                // 0x10046490 on the ground
    UTexture* GetAllyCrestTexture(INT AllyID);                       // 0x10046640 8x12 crest
    void RequestAddTradeItem(NItemRequest Request);                  // 0x10043e60 (guard name)
    void RequestDropItem(NItemRequest Request, FVector Location);    // 0x10048200 (guard name)
    const TCHAR* GetSkillIconName(INT SkillID, INT Level);           // 0x10046650
    const TCHAR* GetActionIconName(INT ActionID);                    // 0x100467e0
    const TCHAR* GetCastleName(INT CastleID);                        // 0x10046cf0 castlename-*.dat
    const TCHAR* GetCastleLocation(INT CastleID);                    // 0x10046d40 its 2nd text
    const TCHAR* GetCastleDescription(INT CastleID);                 // 0x10046d90 its 3rd text
    INT GetWeaponEnchantSteps(INT Enchant);                          // 0x10047040 past +3: x2
    INT GetArmorEnchantSteps(INT Enchant);                           // 0x10047050 past +3: x3
    const TCHAR* GetIconName(INT ItemID);                            // 0x10054c90 (guard name)
    UBOOL HasSelectedTarget();                                       // 0x1004cf90 (shortcut keys)
    void FormatDateTime(INT Time, TCHAR* Out);                       // 0x100575c0 "yyyy/mm/dd hh:mm"
                                                                     //   (the siege windows)
    void MakeSystemMessage(L2ParamStack& Params, TCHAR* Out);        // 0x10057070 (NConsoleWndText.cpp)
                                                                     //   systemmsg id, count, then
                                                                     //   (type, value) each

    // Declared by the windows that use them. Decompiled: the requests to the server
    // (NConsoleWndNet.cpp) and the data getters (NConsoleWndData.cpp); the rest in the other
    // NConsoleWnd*.cpp files. Names ours unless a guard string gave them; the
    // Request* ones pass the call on to the engine's network handler (UL2NetHandler at Engine
    // +0x5C) and are named after the chat command that uses them.
    INT GetIMEType();                 // 0x10042810 kind of the keyboard layout's IME: 1 closed
                                      //   or none, 2..12 by layout (9, 10: Chinese), -1 other
    UBOOL SendChatMsgToServer(INT Type, TCHAR* Text, TCHAR* Target); // 0x10042b80 (guard name) 0
    void RequestTrade(INT CreatureID);                               // 0x10043de0 (guard name)
    User* GetSelectedActor();                                        // 0x1004cea0 (guard name) target
    User* GetMyUser();                                               // 0x1004ce70 the player
    User* FindUserByName(TCHAR* Name);                               // 0x100464c0
    FVector GetMyLocation();                                         // 0x10038540 the player's pawn
    void ShowSystemMessage(INT ID, DWORD Color);                     // 0x10054b60 systemmsg line
    void ShowDialog(NDialogInfo* Info);                              // 0x1004a0e0
    void ExecCommand(TCHAR* Command);                                // 0x1004a0b0 viewport Exec
    void RequestPetition(TCHAR* Text);                               // 0x100456a0
    void RequestPetitionCancel();                                    // 0x100456b0
    void RequestGMList();                                            // 0x100456c0
    void RequestSocialAction(INT Action);                            // 0x100449e0
    void RequestChangeMoveType(UBOOL bRun);                          // 0x10044a00
    void RequestChangeWaitType(UBOOL bStand);                        // 0x100449f0
    void RequestAttack(INT ObjectID, FVector Origin);                // 0x10048cd0
    void RequestAttackForce(INT ObjectID, FVector Origin);           // 0x10044960
    void RequestAttackStand(INT ObjectID, FVector Origin);           // 0x100449a0
    void RequestTarget(INT ObjectID);                                // 0x10048a40
    void RequestTargetNext(INT Range, INT CurrentID);                // 0x10048b50
    void RequestPickUp(INT Range, INT ObjectID);                     // 0x10048ac0
    void RequestAssist();                                            // 0x10051d00
    void RequestPartyInvite(L2ParamStack& Params);                   // 0x10044fc0
    void RequestPartyLeave(L2ParamStack& Params);                    // 0x10044fe0
    void RequestPartyDismiss(L2ParamStack& Params);                  // 0x10044ff0
    void RequestVendor();                                            // 0x10045bc0
    void RequestMagicSkillUse(L2ParamStack& Params);                 // 0x10044750
    void UseShortcut(L2ParamStack& Params);                          // 0x10044860
    void RequestAllyInvite(L2ParamStack& Params);                    // 0x10044cf0
    void RequestAllyInfo();                                          // 0x10044d20
    void RequestFriendInvite(L2ParamStack& Params);                  // 0x10044ed0
    void RequestFriendDel(L2ParamStack& Params);                     // 0x10044ee0
    void RequestFriendList();                                        // 0x10044f00
    void RequestBlock(L2ParamStack& Params);                         // 0x10044f10
    void RequestBuy();                                               // 0x10045c60
    void RequestDwarvenManufacture();                                // 0x10045ed0
    // used by the dialog box (NCDialogBox::OnOK / OnCancel)
    void ConvertNewLines(TCHAR* Text, FString& Result);              // 0x10046de0 escaped newlines
    UBOOL HasInvalidChars(TCHAR* Text);                              // 0x100462c0 empty, or a
                                                                     //   character the font lacks
    UBOOL HasBadWord(TCHAR* Text);                                   // 0x1004a190 word filter
    UBOOL IsValidName(TCHAR* Text);                                  // 0x10056fa0
    void RegisterPledgeCrest(L2ParamStack& Params);                  // 0x100563c0 bmp file path
    void RegisterAllyCrest(L2ParamStack& Params);                    // 0x10056570
    void SetPrivateStoreMsg(TCHAR* Text);                            // 0x10045c00
    void SetPrivateStoreBuyMsg(TCHAR* Text);                         // 0x10045ca0
    void RequestChangePetName(TCHAR* Name);                          // 0x10045540
    void RequestAnswerJoinParty(L2ParamStack& Params);               // 0x10044fd0
    void RequestAnswerJoinPledge(L2ParamStack& Params);              // 0x10044a50
    void RequestAnswerJoinAlly(L2ParamStack& Params);                // 0x10044d00
    void RequestAnswerFriendInvite(L2ParamStack& Params);            // 0x10044ef0
    void RequestGiveNickName(L2ParamStack& Params);                  // 0x10044ae0 member title
    void RequestOustPledgeMember(L2ParamStack& Params);              // 0x10044a60
    void RequestWithdrawalPledge();                                  // 0x10044b00
    void RequestStartPledgeWar(L2ParamStack& Params);                // 0x10044a80
    void RequestReplyStartPledgeWar(L2ParamStack& Params);           // 0x10044a90
    void RequestStopPledgeWar(L2ParamStack& Params);                 // 0x10044aa0
    void RequestReplyStopPledgeWar(L2ParamStack& Params);            // 0x10044ab0
    void RequestSurrenderPledgeWar(L2ParamStack& Params);            // 0x10044ac0
    void RequestSurrenderPersonally(L2ParamStack& Params);           // 0x10044ad0
    void RequestOustAlly(L2ParamStack& Params);                      // 0x10044d10
    void RequestWithdrawAlly();                                      // 0x10044d30
    void RequestDismissAlly();                                       // 0x10044d40
    void RequestActionUse(L2ParamStack& Params);                     // 0x10044950
    void RequestNet103(L2ParamStack& Params);                        // 0x10044a40 (dialog 0xB,
    void RequestNet106();                                            // 0x10044a70  0xC: opened
                                                                     //   nowhere in NWindow)
    // used by the login screens (NCAuthWnd, NCLoginWnd, NCServerSelectWnd)
    UBOOL RequestAuthLogin(const TCHAR* Account, const TCHAR* Password); // 0x10042c70 (guard name)
    void RequestServerLogin(L2ParamStack& Params);                   // 0x10043200 (guard name)
    void ShowMessageWnd(const TCHAR* Text);                          // 0x10046070 the message
                                                                     //   window at +0x36D8
    void ToggleOptionWnd();                                          // 0x100460c0 +0x36C4
    // used by the clan and party matching windows
    UTexture* GetPledgeCrestTexture(INT PledgeID);                   // 0x10046630 16x12 crest
    void RequestJoinPledge(L2ParamStack& Params);                    // 0x10044af0 invitation
    void RequestPartyMatchConfig(L2ParamStack& Params);              // 0x10045000
    void RequestPartyMatchDetail(INT ObjectID);                      // 0x10045020
    const TCHAR* GetClassNameString(INT ClassID);                    // 0x10046f00 SysString[247 + ID]
    // used by the menus and the option windows
    void RequestRestartPoint(L2ParamStack& Params);                  // 0x100430b0 restart menu
                                                                     //   choice (0 village .. 4 fixed)
    void RequestRestart(L2ParamStack& Params);                       // 0x100430a0 back to the
                                                                     //   character selection
    void RequestLogout();                                            // 0x10042d00 quit the game
    void ResetWindowPositions();                                     // 0x1004c060 every window back
                                                                     //   to its default place
    void ReloadGameTexts();                                          // 0x100463f0 after the item /
                                                                     //   skill texts were reloaded
    // used by the status windows (player, target, party, buffs)
    void TargetMyself();                                             // 0x100489b0 click on the
                                                                     //   player window
    AActor* GetSelectedStaticActor();                                // 0x1004cf50 door or other
                                                                     //   static target
    FServerStaticObject* FindStaticObjectInfo(INT CreatureID);       // 0x100464e0
    PetInfo GetPetInfo();                                            // 0x100464a0
    BYTE IsSiegeNpc(INT NpcID);                                      // 0x10047070 castle guards,
                                                                     //   teleporters, mercenaries
    PledgeInfo* GetPledgeInfo(INT PledgeID);                         // 0x10046620
    void RequestTargetCancel(L2ParamStack& Params);                  // 0x10044940
    void AssistMember(INT ObjectID, FVector Origin);                 // 0x10048be0 targets what
                                                                     //   the member targets
    INT GetSkillRange(INT SkillID, INT Level);                       // 0x100467b0 skill data +0x2C
    TCHAR* GetSkillName(INT SkillID, INT Level);                     // 0x10046680
    TCHAR* GetSkillDesc(INT SkillID, INT Level);                     // 0x100466b0
    INT GetServerID();                                               // 0x10046480 NetHandler slot
                                                                     //   30 (GM status: crest names)
    // used by the pet, summon and minimap windows
    void RequestAction(INT ObjectID, FVector Origin);                // 0x10048c70 net handler
                                                                     //   slot 53 (click on it)
    void LockCursor();                                               // 0x10045fd0
    void UnlockCursor();                                             // 0x10045fe0
    // used by the action window
    void RequestPledgeMemberList();                                  // 0x10044a30 NetHandler slot
                                                                     //   102 (shown with a clan)
    void RequestShortCutReg(L2ParamStack& Params);                   // 0x10044830 (type, slot,
                                                                     //   id...) + the drop sound
    void PetPickUp(INT Range, INT Param);                            // 0x100454d0 nearest item
                                                                     //   (slot 210) to slot 189
    // used by the character selection and creation (NCLobbyWnd, NCPawnSelectWnd,
    // NCPawnCreateWnd) and the announcements (NCNoticeWnd)
    UBOOL RequestGameStart(INT Index);                               // 0x10042d10 (guard name)
    UBOOL RequestCharacterDelete(INT Index);                         // 0x10042e00 (guard name)
    UBOOL RequestCharacterRestore(INT Index);                        // 0x10042eb0 (guard name)
    void RequestNewCharacter();                                      // 0x10042f60 (guard name)
    void ReturnToLogin();                                            // 0x10045ef0 "Previous" on
                                                                     //   the character selection
    void ReturnToCharacterSelect();                                  // 0x10045f70 creation cancelled
    void ShowWarningWnd();                                           // 0x100430c0 after the
                                                                     //   announcements, when any
    void FormatSystemMessage(TCHAR* Out, const TCHAR* Format, const TCHAR* Param1,
                             const TCHAR* Param2);                   // 0x10046120 $s1 / $s2
    UBOOL RequestCharacterCreate(CharacterInfo Info);                // 0x10042fe0 (guard name)
    // used by the quest and item windows
    const TCHAR* GetQuestText(INT QuestID, INT Step);                // 0x10044600 (name ours; it
                                                                     //   uses AddQuestID's guard)
    void GetItemSlotText(INT Type, INT BodyPart, FString& Result, INT SubType); // 0x100469a0 the
                                                                     //   slot line of a tool tip
    // used by the shortcut bar and the skill windows
    void RequestUseItem(INT ObjectID);                               // 0x10043a70 (guard name)
    void RequestPetUseItem(INT ObjectID);                            // 0x10045510 NetHandler slot 190
    void RequestShortCutDel(L2ParamStack& Params);                   // 0x10044890 (slot)
    void RequestSkillList(L2ParamStack& Params);                     // 0x10044760 NetHandler slot 154
    const TCHAR* GetItemName(INT ItemID);                            // 0x10054d40 (guard name)
    const TCHAR* GetActionName(INT ActionID);                        // 0x10046830 actionname-*.dat
    UBOOL IsToggleSkill(INT SkillID, INT Level);                     // 0x100466e0 skill data +0x14 == 3
    UBOOL HasAbnormal(INT SkillID);                                  // 0x1004d2e0 the skill's effect
                                                                     //   is on the player (buffs)
    UBOOL IsShotItem(INT ItemID);                                    // 0x10047120 soul / spirit shots
    INT GetSkillMpCost(INT SkillID, INT Level);                      // 0x10046780 skill data +0x1C
    const TCHAR* GetSkillTypeText(INT SkillID, INT Level);           // 0x10046710 "Active Skill",
                                                                     //   "Passive Skill" or "Magic"
    // used by the HTML viewer
    UBOOL HasNoAmpersand(TCHAR* Text);                               // 0x10046320 0 when Text
                                                                     //   holds a '&'
    void RequestTutorialQuestionMark(INT ID);                        // 0x10045460 the tutorial
                                                                     //   question mark clicked
    // used by the desktop (NCConsole)
    UBOOL MoveToLocation(FVector Loc, UBOOL bNoZ);                   // 0x1004d250 NetHandler slot 90
    User* FindUserByID(INT ObjectID);                                // 0x100464d0 NetHandler slot 33
    void RequestItemList();                                          // 0x100439f0 (guard name)
    void RequestPartyMatchList(INT Status);                          // 0x10045010 3: leaves the room
    void RequestLinkHtml(L2ParamStack& Params);                      // 0x100452a0 HTML "Link"
    void RequestBBSWrite(L2ParamStack& Params);                      // 0x100452c0 HTML "Write"
    void RequestShowBoard(L2ParamStack& Params);                     // 0x100452d0 community board
    void RequestTutorialLinkHtml(L2ParamStack& Params);              // 0x10045440
    void RequestPrivateStoreQuitSell();                              // 0x10045bf0
    void RequestPrivateStoreQuitBuy();                               // 0x10045c90
    void RequestGMInventory(TCHAR* Name);                            // 0x100456d0 GM command 5
    void RequestGMSkills(TCHAR* Name);                               // 0x100456f0 GM command 3
    void RequestGMStatus(TCHAR* Name);                               // 0x10045710 GM command 1
    void RequestGMQuests(TCHAR* Name);                               // 0x10045730 GM command 4
    void RequestGMWarehouse(TCHAR* Name);                            // 0x10045750 GM command 6
    void RequestGMPledge(TCHAR* Name);                               // 0x10045770 GM command 2
    // used by the trade window and the pet's inventory
    void RequestTradeDone(INT bConfirm);                             // 0x10043ef0 (guard name) OK 1,
                                                                     //   cancel 0
    void RequestGetItemFromPet(L2ParamStack& Params);                // 0x10045530 NetHandler slot
                                                                     //   192 (object id, amount, 0)
    // used by the inventory (NCInventoryWnd, NCInvenItemWnd, NCEquipItemWnd)
    void RequestUnEquipItem(L2ParamStack& Params);                   // 0x10043af0 body part bits
    void RequestGiveItemToPet(L2ParamStack& Params);                 // 0x10045520 object id, amount
    AActor* GetTargetedActor(UBOOL bCheck);                          // 0x100464f0 (guard name) the
                                                                     //   pawn under the mouse
    INT GetEquipSlot(INT BodyPart, INT ObjectID);                    // 0x1004d030 paperdoll slot,
                                                                     //   14 / 15: two-slot items
    const TCHAR* GetGradeText(INT Grade);                            // 0x10046f20 crystal grade
    UBOOL IsPetCollar(INT ItemID);                                   // 0x10046880 2375, 3500..3502
    UBOOL IsMagicArmor(INT ItemID);                                  // 0x10054df0 armor type 3 (MP)
    const TCHAR* GetWeaponTypeName(INT Type);                        // 0x100468b0 sysstring 43..504
    const TCHAR* GetAttackSpeedText(INT Speed);                      // 0x10046c70 sysstring 306..310
    INT GetEnchantedDefense(INT Grade, INT Enchant, INT Base);       // 0x1004d000 base + bonus
    INT GetEnchantedAttack(INT Grade, INT Enchant, INT Base);        // 0x1004cfd0 (weapons)
    // used by the shop and warehouse windows (NCShopWnd, NCStoreWnd): (id, count) lists
    void RequestBuyItem(L2ParamStack& Params);                       // 0x10044020 (guard name)
    void RequestSellItem(L2ParamStack& Params);                      // 0x100440a0 (guard name)
    void RequestWarehouseDeposit(L2ParamStack& Params);              // 0x10044250 NetHandler slot 138
    void RequestWarehouseWithdraw(L2ParamStack& Params);             // 0x10044260 NetHandler slot 139
    // used by the private stores (names ours, after the packets; the quit requests, NetHandler
    // slots 198 and 205, are with the desktop's above)
    void RequestSetPrivateStoreListSell(L2ParamStack& Params);       // 0x10045bd0 NetHandler slot
                                                                     //   196: count, then object id,
                                                                     //   count and price of each
    void RequestPrivateStoreManageCancel();                          // 0x10045be0 NetHandler slot 197
    void RequestPrivateStoreBuy(L2ParamStack& Params);               // 0x10045c10 NetHandler slot
                                                                     //   201: the owner's object id,
                                                                     //   then as above
    void RequestSetPrivateStoreListBuy(L2ParamStack& Params);        // 0x10045c70 NetHandler slot
                                                                     //   203: count, then item id,
                                                                     //   enchant, count and price
    void RequestPrivateStoreBuyManageCancel();                       // 0x10045c80 NetHandler slot 204
    void RequestPrivateStoreSell(L2ParamStack& Params);              // 0x10045cb0 NetHandler slot
                                                                     //   208: the owner's object id,
                                                                     //   count, then object id and
                                                                     //   the above for each
    // the console's own window management (names ours unless a guard string gave them)
    UBOOL Initialize(HWND hWnd);                                     // 0x1004de70 every window
    void LoadWindowPositions();                                      // 0x1004b020 WindowsInfo.ini
    void SaveWindowPositions();                                      // 0x1004a3d0 WindowsInfo.ini
    void Finalize();                                                 // 0x1004eb30
    UBOOL InitializePawnViewer(HWND hWnd);                           // 0x1004d6c0
    INT NSetTimer(UINT ID, UINT Elapse, NCObject* Owner);            // 0x1004ef90 (NCObject::SetTimer)
    UBOOL NKillTimer(UINT ID, NCObject* Owner);                      // 0x10047440
    void SetCursor(INT Type, UTexture* Texture);                     // 0x10045fb0 unless locked
    void UpdateCursor();                                             // 0x10054e20 every frame
    void UpdateTargetWindow();                                       // 0x10054c20 every tick
    INT DispatchWndMsgX(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam); // 0x10055940
    UBOOL HookShortCutKey(UINT Msg, WPARAM wParam, LPARAM lParam);   // 0x1004f050
    // the console's own text services and actions (names ours)
    void RemoveInvalidChars(TCHAR* Text);                            // 0x10046360 the characters
                                                                     //   the font lacks
    AActor* GetPickupUnderMouse();                                   // 0x100465e0 an AL2Pickup
    void UseAction(INT ActionID);                                    // 0x100551c0 an action slot
    void UseShortCut(INT Slot);                                      // 0x100561f0 a shortcut slot
                                                                     //   (the keys, slot 263)

    // UObject's and FExec's
    virtual void Destroy();                                             // 10 0x100551a0
    virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) { return 0; } // FExec 0x100016f0

    // UL2ConsoleWnd's, implemented here: called by the engine. The UGameEngine::On* handler of
    // FL2NetNotify that calls a slot names it (without "On") unless a guard string does; names
    // ours otherwise (vfnNNN: not known). The login and the character stages
    // timers, DeltaTime (+0x370C)
    virtual void Tick(FLOAT DeltaTime);                                 // 24 0x10055cf0
    // WndMain->IsOverWindow
    virtual UBOOL IsOverWindow(INT X, INT Y);                           // 26 0x10042990
    // also OnAcceptLogOn; empty, folded
    virtual void AuthLoginSuccess() {}                                  // 27 0x10045870
    virtual void AuthLoginFail(INT Reason);                             // 28 0x10047790
    // the announcements (NCNoticeWnd)
    virtual void StartNotice(L2ParamStack& Params);                     // 29 0x10047a70
    virtual void CharacterCreateFail(INT Reason);                       // 30 0x10047b90
    virtual void ReceiveCharacterInfo();                                // 31 0x100434a0
    virtual void AddCharacterInfo(User Info);                           // 32 0x10043580
    virtual void AcceptGameStart(INT Result);                           // 33 0x10047c80
    virtual void AcceptCharacterDelete(INT Result);                     // 34 0x10047d90
    virtual void AcceptNewCharacter(INT Result);                        // 35 0x10047e40
    virtual void AcceptCharacterCreate(INT Result);                     // 36 0x10047f30
    // the same body as QuitGame (273)
    virtual void AcceptLogOut();                                        // 37 0x1004f820
    virtual void ServerClose(INT Reason);                               // 38 0x1004f6b0
    virtual void ReviveRequest(INT Param1, INT Param2);                 // 39 0x1004f830
    // OnStartServerList
    virtual void ReceiveServerList(INT LastServerID);                   // 40 0x10043280
    virtual void AddServerList(L2ParamStack& Params);                   // 41 0x10043390
    virtual void EndServerList();                                       // 42 0x10043420

    // the target, the inventory, the trade, the NPC shops and the warehouse
    virtual void MyTargetSelected(INT LevelDiff);                       // 43 0x10044a10
    virtual void ReceiveItemList(INT bShow);                            // 44 0x10043b00
    virtual void ReceiveUpdateItemList();                               // 45 0x10043bb0 no handler
    virtual void ReceiveTradeStart(INT ObjectID);                       // 46 0x1004ffb0
    // the engine's ItemInfo, by value
    virtual void ReceiveAddTradeInventoryItem(NItemRequest Item);       // 47 0x100500c0
    virtual void ReceiveAddTradeOwnItem(NItemRequest Item);             // 48 0x10050380
    virtual void ReceiveAddTradeOtherItem(NItemRequest Item);           // 49 0x10050640
    virtual void ReceiveTradeUpdate(INT Change, ItemInfo& Item);        // 50 0x10050900
    virtual void ReceiveTradeDoneMsg(INT Result);                       // 51 0x10043f80
    virtual void AddRecommandedData(RecommandedDataStr Data);           // 52 0x100438e0
    virtual void EndRecommandedData();                                  // 53 0x10043970
    virtual void AddInventoryItem(ItemInfo& Item);                      // 54 0x1004f8c0
    // OnReceiveUpdateItemList
    virtual void UpdateInventoryItem(INT Change, ItemInfo& Item);       // 55 0x1004fbb0
    virtual void StartBuyList(INT Adena, INT ListID);                   // 56 0x10044120 NPC shop
    virtual void AddBuyList(ItemInfo& Item);                            // 57 0x10050bc0
    virtual void StartSellList(INT Adena, INT ListID);                  // 58 0x100441a0
    virtual void AddSellList(ItemInfo& Item);                           // 59 0x10050e70
    virtual void StartDepositList(INT Adena);                           // 60 0x10044270 warehouse
    virtual void AddDepositList(ItemInfo& Item);                        // 61 0x10051120
    virtual void StartWithdrawList(INT Adena);                          // 62 0x10044310
    virtual void AddWithdrawList(ItemInfo& Item);                       // 63 0x100513d0
    // OnReceiveWareHouseDonePacket
    virtual void ReceiveWareHouseDone(L2ParamStack& Params);            // 64 0x100561c0
    virtual void UpdatePlayerInfo(L2ParamStack& Params);                // 65 0x10045e70

    // HTML pages, the tutorial, the radar
    virtual void NpcHtmlMessage(L2ParamStack& Params);                  // 66 0x100452e0
    virtual void TutorialShowHtml(TCHAR* Html);                         // 67 0x10045340
    virtual void TutorialShowQuestionMark(INT ID);                      // 68 0x10049990
    virtual void TutorialEnableClientEvent(INT Events);                 // 69 0x100454a0
    virtual void TutorialCloseHtml();                                   // 70 0x100454b0
    // CheckTutorialClientEvent; once per pending bit (+0x3724)
    virtual void SendTutorialEvent(INT Event);                          // 71 0x10045470
    virtual void ShowRadar(INT X, INT Y, INT Z);                        // 72 0x10045b20
    virtual void DeleteRadar(INT X, INT Y, INT Z);                      // 73 0x10045b70

    // the clan, the alliance, the friends, the party
    // empty, folded
    virtual void ReceivePledgeExtendedInfo(PledgeInfo& Info) {}         // 74 0x10001700
    virtual void ReceivePledgeInfo(PledgeInfo& Info);                   // 75 0x10048d70
    virtual void ReceivePledgeInfoUpdate(PledgeInfo& Info);             // 76 0x10048fe0
    virtual void ReceivePledgeMemberList();                             // 77 0x10044b10
    virtual void AddPledgeMember(L2ParamStack& Params);                 // 78 0x100491f0
    virtual void UpdatePledgeMember(L2ParamStack& Params);              // 79 0x100492f0
    virtual void DeletePledgeMember(TCHAR* Name);                       // 80 0x10044ba0
    virtual void DeleteAllPledgeMember();                               // 81 0x10044c30
    virtual void ReceiveCreatePledge(L2ParamStack& Params);             // 82 0x10056750
    virtual void ReceiveAskJoinPledge(L2ParamStack& Params);            // 83 0x10051e10
    virtual void ReceiveOustPledgeMember(L2ParamStack& Params);         // 84 0x10056790
    virtual void ReceiveDismissPledge(L2ParamStack& Params);            // 85 0x100567d0
    virtual void ReceiveJoinPledge(L2ParamStack& Params);               // 86 0x10056720
    // OnReceiveWidthdrawalPledge
    virtual void ReceiveWithdrawalPledge(L2ParamStack& Params);         // 87 0x10056810
    virtual void ReceiveOustPledgeMessage(L2ParamStack& Params);        // 88 0x10056850
    virtual void ReceiveDismissPledgeMessage(L2ParamStack& Params);     // 89 0x10056870
    virtual void ReceiveStartPledgeWar(L2ParamStack& Params);           // 90 0x10051f10
    virtual void ReceiveReplyStartPledgeWar(L2ParamStack& Params);      // 91 0x100568b0
    virtual void ReceiveStopPledgeWar(L2ParamStack& Params);            // 92 0x10052010
    virtual void ReceiveReplyStopPledgeWar(L2ParamStack& Params);       // 93 0x100568f0
    virtual void ReceiveSurrenderPledgeWar(L2ParamStack& Params);       // 94 0x10056930
    virtual void ReceiveSurrenderPersonally(L2ParamStack& Params);      // 95 0x10056960
    virtual void ReceiveAskJoinParty(L2ParamStack& Params);             // 96 0x10052790
    virtual void ReceiveJoinParty(L2ParamStack& Params);                // 97 0x10056a10
    virtual void ReceiveWithdrawalParty(L2ParamStack& Params);          // 98 0x10045030
    virtual void ReceiveOustPartyMember(L2ParamStack& Params);          // 99 0x10056a40
    virtual void ReceiveOustPartyMessage(L2ParamStack& Params);         // 100 0x10056a70
    virtual void ReceiveDismissParty(L2ParamStack& Params);             // 101 0x10045070
    // OnRecevieDismissPartyMessage
    virtual void ReceiveDismissPartyMessage(L2ParamStack& Params);      // 102 0x10056a90
    virtual void ReceiveAskJoinAlly(L2ParamStack& Params);              // 103 0x10052110
    virtual void ReceiveDismissAlly(L2ParamStack& Params);              // 104 0x10044dd0
    virtual void ReceiveJoinAlly(L2ParamStack& Params);                 // 105 0x10056990
    virtual void ReceiveOustAlly(L2ParamStack& Params);                 // 106 0x10044d90
    virtual void ReceiveWithdrawAlly(L2ParamStack& Params);             // 107 0x10044d50
    virtual void ReceiveFriendAddRequestResult(L2ParamStack& Params);   // 108 0x100569d0
    virtual void ReceiveFriendAddRequest(L2ParamStack& Params);         // 109 0x10052710
    // empty, folded
    virtual void ReceiveFriendAdd(L2ParamStack& Params) {}              // 110 0x10001700
    // empty, folded
    virtual void ReceiveFriendRemove(L2ParamStack& Params) {}           // 111 0x10001700
    // empty, folded
    virtual void ReceiveFriendList(L2ParamStack& Params) {}             // 112 0x10001700
    // empty, folded
    virtual void ReceiveFriendStatus(L2ParamStack& Params) {}           // 113 0x10001700
    // empty, folded
    virtual void ReceiveBlockList(L2ParamStack& Params) {}              // 114 0x10001700

    // shortcuts, buffs, quests, chat, party, skills, system messages
    virtual void ReceiveShortCutList();                                 // 115 0x100448a0
    virtual void AddShortCutItem(L2ParamStack& Params);                 // 116 0x100485b0
    virtual void UpdateShortCutItem(INT ObjectID);                      // 117 0x100448c0
    virtual void UpdatePetShortCutItem(INT ObjectID);                   // 118 0x100448e0
    virtual void ReceiveShortCutDelete(INT Slot);                       // 119 0x10044900
    // OnReceiveMagicSkillListPacket
    virtual void ReceiveMagicSkillList(L2ParamStack& Params);           // 120 0x10044770
    virtual void ReceiveAbnormalStatus();                               // 121 0x10044920
    virtual void AddAbnormalStatus(L2ParamStack& Params);               // 122 0x10048860
    virtual void ReceiveQuestList();                                    // 123 0x100444a0
    virtual void AddQuestID(INT QuestID, INT Step);                     // 124 0x10044530
    virtual void AddQuestItem(ItemInfo& Item);                          // 125 0x10051680
    // a chat line
    virtual UBOOL Say2(L2ParamStack& Params);                           // 126 0x10054a30
    // no handler; the debug window
    virtual void VARARGS DebugLogf(INT Category, const TCHAR* Fmt, ...);
                                                                        // 127 0x10045e00
    virtual void ReceiveVersionCheck(L2ParamStack& Params);             // 128 0x10048050
    virtual void UpdateEquipItem(L2ParamStack& Params);                 // 129 0x10043d20
    // OnPartyAddMemberPacket
    virtual void PartyAddMember(L2ParamStack& Params);                  // 130 0x100450b0
    virtual void PartyUpdateMember(L2ParamStack& Params);               // 131 0x10045150
    virtual void PartyDeleteMember(L2ParamStack& Params);               // 132 0x100451d0
    virtual void PartyDeleteAll(L2ParamStack& Params);                  // 133 0x10045210
    virtual void ReceivePartyMatchList();                               // 134 0x10045240
    virtual void AddPartyMatchMember(L2ParamStack& Params);             // 135 0x10045260
    virtual void ReceivePartyMatchDetail(L2ParamStack& Params);         // 136 0x10045280
    // empty, folded
    virtual void ShowMagicWnd() {}                                      // 137 0x10045870
    virtual void ShowSkillWnd();                                        // 138 0x10044780
    // OnSkillListPacket
    virtual void ReceiveSkillList(L2ParamStack& Params);                // 139 0x10048470
    // OnMagicListPacket; empty, folded
    virtual void ReceiveMagicList(L2ParamStack& Params) {}              // 140 0x10001700
    virtual void ReceiveTradeDemand(L2ParamStack& Params);              // 141 0x1004feb0
    // OnRestartResponse, first
    virtual void RestartResetWindows();                                 // 142 0x10043640
    // OnRestartResponse, then
    virtual void RestartToLobby();                                      // 143 0x100436a0
    virtual void AddSystemMessageParam(L2ParamStack& Params);           // 144 0x10056ab0
    virtual void EndSystemMessageParam(L2ParamStack& Params);           // 145 0x10056ba0

    // board, enchant, skill trainer, restart, VIP shop, chat channels, pets
    // OnShowboardPacket
    virtual void ShowBoard(L2ParamStack& Params);                       // 146 0x10045380
    // NetHandler 164
    virtual void BuilderCommand(L2ParamStack& Params);                  // 147 0x10045ee0
    virtual void SelectItemToEnchant();                                 // 148 0x10043c30
    virtual void ReceiveEnchantResult(INT Result);                      // 149 0x10043cd0
    virtual void ReceiveAction(INT Action);                             // 150 0x10051da0
    virtual void ShowTrainListWnd();                                    // 151 0x100446d0
    virtual void ShowTrainInfoWnd(L2ParamStack& Params);                // 152 0x10051820
    virtual void AddAcquireSkillInfo(L2ParamStack& Params);             // 153 0x100482b0
    virtual void AddAcquireSkillExtendedInfo(L2ParamStack& Params);     // 154 0x10051a90
    // no handler; the restart points after dying
    virtual void ShowRestartMenu(L2ParamStack& Params);                 // 155 0x100480f0
    virtual void TeleportToLocation();                                  // 156 0x100438d0
    virtual void HideTrainWnd();                                        // 157 0x10044730
    virtual void SetVIPShopID(INT ShopID);                              // 158 0x10045cf0
    virtual void AddVIPItemList(INT ProductID, INT ItemID, INT BodyPart, INT Type, INT Count);
                                                                        // 159 0x10054760
    virtual void AddVIPNeedItemList(INT ProductID, INT ItemID, INT Count);
                                                                        // 160 0x10054970
    virtual void VIPItemListEnd();                                      // 161 0x10045d10
    // NetHandler 186
    virtual void RequestBuyVIPItem(L2ParamStack& Params);               // 162 0x10045d40
    virtual void EnablePartyChat(bool bEnable);                         // 163 0x10045d50
    virtual void EnablePledgeChat(bool bEnable);                        // 164 0x10045d70
    virtual void EnableAllyChat(bool bEnable);                          // 165 0x10045d90
    virtual void ShowPetStatusWnd(INT Type);                            // 166 0x10045550
    virtual void ShowPetPartyWnd(INT Type);                             // 167 0x100455d0
    virtual void HideSummonWnd();                                       // 168 0x100499f0 no handler
    virtual void HidePetWnd();                                          // 169 0x10049a90
    virtual void ShowPetNameBtn(INT bShow);                             // 170 0x10045640
    virtual void ReceivePetStatusInfo(INT Type, L2ParamStack& Params);  // 171 0x10045660
    virtual void ReceivePetItemList();                                  // 172 0x10045680
    virtual void AddPetInventoryItem(ItemInfo& Item);                   // 173 0x100528f0
    virtual void UpdatePetInventoryItem(INT Change, ItemInfo& Item);    // 174 0x10052bb0
    // OnSkillCoolTimePacket
    virtual void SkillCoolTime(INT Param1, INT Param2, INT Param3, INT Param4);
                                                                        // 175 0x1004d330

    // alliance wars and sieges (the Request* pass the call on to the network handler)
    // NetHandler 174; no caller in NWindow
    virtual void vfn176(L2ParamStack& Params);                          // 176 0x10047060
    // NetHandler 175
    virtual void RequestStartAllianceWar(L2ParamStack& Params);         // 177 0x10044e10
    // NetHandler 176
    virtual void RequestReplyStartAllianceWar(L2ParamStack& Params);    // 178 0x10044e20
    // NetHandler 177
    virtual void RequestStopAllianceWar(L2ParamStack& Params);          // 179 0x10044e30
    // NetHandler 178
    virtual void RequestReplyStopAllianceWar(L2ParamStack& Params);     // 180 0x10044e40
    // NetHandler 179
    virtual void RequestSurrenderAllianceWar(L2ParamStack& Params);     // 181 0x10044e50
    virtual void StartAllianceWar(TCHAR* AllianceName, TCHAR* Name);    // 182 0x10052210
    virtual void StopAllianceWar(TCHAR* AllianceName, TCHAR* Name);     // 183 0x10052310
    // NetHandler 180; no caller in NWindow
    virtual void vfn184(L2ParamStack& Params);                          // 184 0x10044f20
    // NetHandler 181
    virtual void RequestSiegeAttackerList(L2ParamStack& Params);        // 185 0x10044f30
    // NetHandler 182
    virtual void RequestSiegeDefenderList(L2ParamStack& Params);        // 186 0x10044f40
    // NetHandler 183
    virtual void RequestJoinSiege(L2ParamStack& Params);                // 187 0x10044f50
    // NetHandler 184
    virtual void RequestConfirmSiegeWaitingList(L2ParamStack& Params);  // 188 0x10044f60
    // NetHandler 185
    virtual void RequestSetSiegeTime(L2ParamStack& Params);             // 189 0x10044f70
    virtual void ReceiveSiegeInfo(L2ParamStack& Params);                // 190 0x100493f0
    virtual void ReceiveSiegeAttackerList(L2ParamStack& Params);        // 191 0x10049500
    virtual void ReceiveSiegeDefenderList(L2ParamStack& Params);        // 192 0x100495f0
    virtual void AddSelectableTime(INT Time);                           // 193 0x100496f0
    virtual void EndSiegeAttackerList();                                // 194 0x10049710
    virtual void EndSiegeDefenderList();                                // 195 0x10049730
    virtual void ClearAttackerList();                                   // 196 0x10044f80
    virtual void ClearDefenderList();                                   // 197 0x10044fa0

    // packages, GM views, minimap, private stores
    // NetHandler 187
    virtual void RequestPackageSend(L2ParamStack& Params);              // 198 0x10044e60
    // NetHandler 188
    virtual void RequestPackageSendableItemList(L2ParamStack& Params);  // 199 0x10044e70
    virtual void AddPackageToList(L2ParamStack& Params);                // 200 0x10044e80
    virtual void AddSendableItemList(INT Index, INT TargetID, INT Adena, ItemInfo& Item);
                                                                        // 201 0x10052410
    virtual void GMViewCharacterInfo(User& Info);                       // 202 0x10045790
    virtual void GMViewReceivePledgeInfo(PledgeInfo& Info);             // 203 0x10049b30
    // guard ReceivePledgeMemberList
    virtual void GMViewReceivePledgeMemberList();                       // 204 0x100457c0
    // guard AddPledgeMember
    virtual void GMViewAddPledgeMember(L2ParamStack& Params);           // 205 0x10049d60
    virtual void GMViewShowSkillWnd();                                  // 206 0x10045850
    // OnGMViewSkillListPacket
    virtual void GMViewSkillList(L2ParamStack& Params);                 // 207 0x10049e50
    // empty, folded
    virtual void GMViewShowMagicWnd() {}                                // 208 0x10045870
    // OnGMViewMagicListPacket
    virtual void GMViewMagicList(L2ParamStack& Params);                 // 209 0x10049f80
    // guard ReceiveQuestList
    virtual void GMViewReceiveQuestList();                              // 210 0x10045880
    // guard AddQuestID
    virtual void GMViewAddQuestID(INT QuestID, INT Step);               // 211 0x10045900
    // guard AddQuestItem
    virtual void GMViewAddQuestItem(ItemInfo& Item);                    // 212 0x10052e90
    // guard ReceiveItemList
    virtual void GMViewReceiveItemList(INT bShow);                      // 213 0x100459d0
    // guard AddInventoryItem
    virtual void GMViewAddInventoryItem(ItemInfo& Item);                // 214 0x10053010
    virtual void GMViewStartWithdrawList(INT Adena);                    // 215 0x10045a80
    virtual void GMViewAddWithdrawList(ItemInfo& Item);                 // 216 0x10053300
    // OnShowMinimap
    virtual void ShowMapItem(INT ItemID);                               // 217 0x10045ae0
    // the same body as 219
    virtual void StartPrivateStoreManageList();                         // 218 0x10045c20
    virtual void StartPrivateStoreList();                               // 219 0x10045c20
    virtual void PrivateStoreManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem,
                                        INT OwnerID, INT Adena);        // 220 0x10053590
    virtual void PrivateStoreList(ItemInfo& Item, INT OwnerID, INT Adena);
                                                                        // 221 0x10053b50
    // also OnPrivateStoreBuyMsg; empty
    virtual void PrivateStoreMsg(TCHAR* Text, INT Param) {}             // 222 0x10045c50
    // the same body as 224
    virtual void StartPrivateStoreBuyManageList();                      // 223 0x10045cc0
    virtual void StartPrivateStoreBuyList();                            // 224 0x10045cc0
    virtual void PrivateStoreBuyManageList(ItemInfo& InventoryItem, ItemInfo& StoreItem,
                                           INT OwnerID, INT Adena);     // 225 0x10053e80
    virtual void PrivateStoreBuyList(ItemInfo& Item, INT OwnerID, INT Adena);
                                                                        // 226 0x10054420

    // the rest of the interface
    virtual void BlockedAccountWithMsg(TCHAR* Msg);                     // 227 0x10047490
    virtual void BlockedAccount(INT Reason);                            // 228 0x100475c0
    virtual void SetSummonRemainTime(INT Remaining, INT Total);         // 229 0x100499c0
    // +0x3710 || +0x3714
    virtual UBOOL IsCtrlDown();                                         // 230 0x10046050
    virtual void VARARGS MessageBoxf(const TCHAR* Caption, const TCHAR* Fmt, ...);
                                                                        // 231 0x10045db0 no handler
    virtual UBOOL Init(UEngine* InEngine);                              // 236 0x1004eee0
    virtual INT ConsolePostRender(UCanvas* Canvas);                     // 237 0x10055e50
    virtual INT DispatchWndMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
                                                                        // 238 0x10057500
    virtual UBOOL NpcSay(L2ParamStack& Params);                         // 239 0x100429b0
    virtual UBOOL MasterConsoleEventProcess(INT Key, INT Action, FLOAT Delta);
                                                                        // 240 0x10055f20
    // "L2Debug", "L2DebugWindow"
    virtual UBOOL DebugCommand(const TCHAR* Cmd);                       // 241 0x10047190
    virtual UBOOL InitForPawnViewer(UEngine* InEngine);                 // 253 0x100550f0
    virtual void PawnViewerFunction(INT Function);                      // 254 0x10046f50
    // GMalloc'd array of the item ids
    virtual INT* GetInventoryItemIDs(INT& Count);                       // 257 0x1004d190
    // folded with operator delete
    virtual void Free(void* Ptr) { appFree(Ptr); }                      // 258 0x10046f40
    virtual void SetServerPing(INT ServerID, DWORD Ping);               // 260 0x10047170
    virtual void ToggleInventory();                                     // 261 0x100447a0 no handler
    // no handler; Desktop->ToggleCharacter(0)
    virtual void ShowCharacterStatus();                                 // 262 0x100447e0
    virtual void UseShortCutSlot(INT Index);                            // 263 0x10057590 no handler
    virtual void ChangeShortCutPage(INT bNext);                         // 264 0x100447f0 no handler
    // no handler; the pawn after (or before) CreatureID among the pawns in an area (ULevel slot 52)
    virtual User* FindNextPawnInArea(FLOAT Radius, INT CreatureID, INT bNext, INT P4, INT P5,
                                     INT P6, INT P7);                   // 265 0x1004d4c0
    // Desktop +0x170..0x17C
    virtual void SetViewRect(INT Left, INT Top, INT Right, INT Bottom); // 266 0x1004d680
    // +0x3710 = 1
    virtual void PressCtrl();                                           // 267 0x10045ff0
    // +0x3710 = 0
    virtual void ReleaseCtrl();                                         // 268 0x10046020
    // ShortCutWnd->bPadShift1
    virtual void SetPadShift1(BYTE bShift);                             // 269 0x10047b50
    // ShortCutWnd->bPadShift2
    virtual void SetPadShift2(BYTE bShift);                             // 270 0x10047b70
    // empty, folded
    virtual void vfn271(INT Param) {}                                   // 271 0x10001700

    // NConsoleWnd's own
    // NetHandler 69
    virtual void RequestServerList();                                   // 272 0x10043180
    // the same body as AcceptLogOut (37)
    virtual void QuitGame();                                            // 273 0x1004f820
    virtual void ReceiveServerClose();                                  // 274 0x10047fc0
    // NetHandler 76
    virtual void AnswerReviveRequest(UBOOL bYes, INT Param1, INT Param2);
                                                                        // 275 0x10043820
    // NetHandler 171
    virtual void RequestEnchantItem(INT ObjectID);                      // 276 0x10043c50
    // NetHandler 165
    virtual void RequestDestroyItem(NItemRequest Request);              // 277 0x10043d40
    // NetHandler 166
    virtual void RequestCrystallizeItem(INT ObjectID, INT Count);       // 278 0x10043dd0
    // NetHandler 156
    virtual void AnswerTradeRequest(L2ParamStack& Params);              // 279 0x10043f70
    // NetHandler 169; 0 without a server
    virtual UBOOL RequestQuestList();                                   // 280 0x10044380
    // NetHandler 170
    virtual void RequestDestroyQuest(INT QuestID);                      // 281 0x10044420
    // NetHandler 172
    virtual void RequestAcquireSkillInfo(L2ParamStack& Params);         // 282 0x10044710
    // NetHandler 173
    virtual void RequestAcquireSkill(L2ParamStack& Params);             // 283 0x10044720
    // "bypass [-h] ..."; 2: close the page
    virtual INT RunHtmlCommand(TCHAR* Command, NCHtmlViewer* Viewer);   // 284 0x10049750
};
NW_ASSERT_OFFSET(NConsoleWnd, WndMain, 0x36BC);
NW_ASSERT_OFFSET(NConsoleWnd, Desktop, 0x36DC);
NW_ASSERT_OFFSET(NConsoleWnd, DebugWnd, 0x36E0);
NW_ASSERT_OFFSET(NConsoleWnd, PetitionWnd, 0x36EC);
NW_ASSERT_OFFSET(NConsoleWnd, Timers, 0x36FC);
NW_ASSERT_OFFSET(NConsoleWnd, DeltaTime, 0x370C);
NW_ASSERT_OFFSET(NConsoleWnd, PartyItemDistribution, 0x372C);
NW_ASSERT_OFFSET(NConsoleWnd, RestartMenuWnd, 0x36C0);
NW_ASSERT_OFFSET(NConsoleWnd, MessageWnd, 0x36D8);
NW_ASSERT_OFFSET(NConsoleWnd, LoadingScreenID, 0x3728);
NW_ASSERT_OFFSET(NConsoleWnd, bInitialized, 0x36B8);
NW_ASSERT_OFFSET(NConsoleWnd, SysMsgParams, 0x36F0);
NW_ASSERT_OFFSET(NConsoleWnd, TutorialEvents, 0x3724);
NW_ASSERT_OFFSET(NConsoleWnd, PawnViewerWnd, 0x3734);
NW_ASSERT_SIZE(NConsoleWnd, 0x3740);

// Cast<APawn> out of line (0x10047230, name ours): NULL unless Object is a pawn
APawn* CastToPawn(UObject* Object);

// FL2NetNotify: 296 slots from Engine.dll ??_7FL2NetNotify@@6B@ (tools/gen_engine_decls.py, do not edit)
virtual INT OnVersionCheck();                                                           //   0 +0x000
virtual INT OnMoveToLocation(User*, FVector, FVector);                                  //   1 +0x004
virtual INT OnMoveToLocationInVehicle(FVehicle*, User*, FVector, FVector);              //   2 +0x008
virtual INT OnMoveBackwardToLocation(User*, FVector, FVector);                          //   3 +0x00c
virtual INT OnMoveToPawn(User*, User*, INT, FVector);                                   //   4 +0x010
virtual INT OnValidateLocation(User*, FVector, INT);                                    //   5 +0x014
virtual INT OnValidateLocationInVehicle(FVehicle*, User*, FVector, INT);                //   6 +0x018
virtual INT OnFlyToLocation(User*, FVector, FVector);                                   //   7 +0x01c
virtual INT OnEarthQuake(FVector, INT, INT, INT);                                       //   8 +0x020
virtual INT OnStartRotating(User*, INT, INT);                                           //   9 +0x024
virtual INT OnFinishRotating(User*, INT);                                               //  10 +0x028
virtual INT OnNpcSay(L2ParamStack&);                                                    //  11 +0x02c
virtual INT OnCharInfo(User*, FVector, INT, L2ParamStack&);                             //  12 +0x030
virtual INT OnUserInfo(User*, FVector, INT, INT);                                       //  13 +0x034
virtual INT OnAttack(User*, User*, INT, INT, INT, INT, INT, FVector);                   //  15 +0x03c
virtual INT OnAttack(User*, INT, INT, INT, INT, INT, FVector);                          //  14 +0x038
virtual INT OnAttacked(User*, User*);                                                   //  17 +0x044
virtual INT OnAttacked(User*, INT);                                                     //  16 +0x040
virtual INT OnAttackCanceled(User*);                                                    //  18 +0x048
virtual INT OnDie(User*, L2ParamStack&);                                                //  19 +0x04c
virtual INT OnRide(User*, INT, INT);                                                    //  20 +0x050
virtual INT OnReviveRequest(INT, INT);                                                  //  21 +0x054
virtual INT OnRevive(User*);                                                            //  22 +0x058
virtual INT OnAttackOutofRange(User*);                                                  //  23 +0x05c
virtual INT OnAttackinCoolTime(User*);                                                  //  24 +0x060
virtual INT OnAttackDeadTarget(User*);                                                  //  25 +0x064
virtual INT OnLeaveWorld(User*);                                                        //  26 +0x068
virtual INT OnAuthLoginSuccess();                                                       //  27 +0x06c
virtual INT OnStartServerList(INT);                                                     //  28 +0x070
virtual INT OnAddServerList(INT, TCHAR*, INT, INT, INT, INT);                           //  30 +0x078
virtual INT OnAddServerList(L2ParamStack&);                                             //  29 +0x074
virtual INT OnEndServerList(INT);                                                       //  31 +0x07c
virtual INT OnAuthLoginFail(INT);                                                       //  32 +0x080
virtual INT OnBlockedAccountWithMsg(TCHAR*);                                            //  33 +0x084
virtual INT OnBlockedAccount(INT);                                                      //  34 +0x088
virtual INT OnCharacterCreateFail(INT);                                                 //  35 +0x08c
virtual INT OnAuthServerSelectSuccess();                                                //  36 +0x090
virtual INT OnAuthServerSelectFail(INT);                                                //  37 +0x094
virtual INT OnSpawnItem(Item*, FVector, FRotator);                                      //  38 +0x098
virtual INT OnDropItem(User*, Item*, FVector, FRotator);                                //  39 +0x09c
virtual INT OnEquipItemPlaySound(INT);                                                  //  40 +0x0a0
virtual INT OnGetItem(User*, Item*, FVector);                                           //  41 +0x0a4
virtual INT OnEquipItem(User*, Item*, INT);                                             //  42 +0x0a8
virtual INT OnUnequipItem(User*, Item*, INT);                                           //  43 +0x0ac
virtual INT OnClassUpdate(User*, INT);                                                  //  44 +0x0b0
virtual INT OnLevelUpdate(User*, INT);                                                  //  45 +0x0b4
virtual INT OnExpUpdate(User*, INT);                                                    //  46 +0x0b8
virtual INT OnStrUpdate(User*, INT);                                                    //  47 +0x0bc
virtual INT OnDexUpdate(User*, INT);                                                    //  48 +0x0c0
virtual INT OnConUpdate(User*, INT);                                                    //  49 +0x0c4
virtual INT OnIntUpdate(User*, INT);                                                    //  50 +0x0c8
virtual INT OnWitUpdate(User*, INT);                                                    //  51 +0x0cc
virtual INT OnMenUpdate(User*, INT);                                                    //  52 +0x0d0
virtual INT OnHpUpdate(User*, INT);                                                     //  53 +0x0d4
virtual INT OnMaxHpUpdate(User*, INT);                                                  //  54 +0x0d8
virtual INT OnMpUpdate(User*, INT);                                                     //  55 +0x0dc
virtual INT OnMaxMpUpdate(User*, INT);                                                  //  56 +0x0e0
virtual INT OnSPUpdate(User*, INT);                                                     //  57 +0x0e4
virtual INT OnCarringWeightUpdate(User*, INT);                                          //  58 +0x0e8
virtual INT OnCarryWeightUpdate(User*, INT);                                            //  59 +0x0ec
virtual INT OnAttackRangeUpdate(User*, INT);                                            //  60 +0x0f0
virtual INT OnPhysicalAttack(User*, INT);                                               //  61 +0x0f4
virtual INT OnPhysicalAttackSpeed(User*, INT);                                          //  62 +0x0f8
virtual INT OnPhysicalDefense(User*, INT);                                              //  63 +0x0fc
virtual INT OnPhysicalAvoid(User*, INT);                                                //  64 +0x100
virtual INT OnHitRate(User*, INT);                                                      //  65 +0x104
virtual INT OnCriticalRate(User*, INT);                                                 //  66 +0x108
virtual INT OnMagicalAttack(User*, INT);                                                //  67 +0x10c
virtual INT OnMagicCastingSpeed(User*, INT);                                            //  68 +0x110
virtual INT OnMagicDefense(User*, INT);                                                 //  69 +0x114
virtual INT OnGuiltyUpdate(User*, INT);                                                 //  70 +0x118
virtual INT OnCriminalRateUpdate(User*, INT);                                           //  71 +0x11c
virtual INT OnNpcHtmlMessage(User*, TCHAR*);                                            //  72 +0x120
virtual INT OnTutorialShowHtml(TCHAR*);                                                 //  73 +0x124
virtual INT OnTutorialShowQuestionMark(INT);                                            //  74 +0x128
virtual INT OnTutorialEnableClientEvent(INT);                                           //  75 +0x12c
virtual INT OnTutorialCloseHtml();                                                      //  76 +0x130
virtual INT CheckTutorialClientEvent(INT);                                              //  77 +0x134
virtual INT OnShowRadar(INT, INT, INT);                                                 //  78 +0x138
virtual INT OnDeleteRadar(INT, INT, INT);                                               //  79 +0x13c
virtual INT OnStartSellList(INT, INT);                                                  //  80 +0x140
virtual INT OnAddSellList(ItemInfo&);                                                   //  81 +0x144
virtual INT OnStartBuyList(INT, INT);                                                   //  82 +0x148
virtual INT OnAddBuyList(ItemInfo&);                                                    //  83 +0x14c
virtual INT OnStartDepositList(INT);                                                    //  84 +0x150
virtual INT OnAddDepositList(ItemInfo&);                                                //  85 +0x154
virtual INT OnStartWithdrawList(INT);                                                   //  86 +0x158
virtual INT OnAddWithdrawList(ItemInfo&);                                               //  87 +0x15c
virtual INT OnDeleteUser(User*, INT);                                                   //  88 +0x160
virtual INT OnDeleteItem(Item*);                                                        //  89 +0x164
virtual INT OnGMHide(INT);                                                              //  90 +0x168
virtual INT OnServerObjectInfo(User*, FVector, INT, INT, INT);                          //  91 +0x16c
virtual INT OnNpcInfo(User*, FVector, INT, L2ParamStack&);                              //  92 +0x170
virtual INT OnActionFail();                                                             //  93 +0x174
virtual INT OnTeleportToLocation(User*, FVector);                                       //  94 +0x178
virtual INT OnTargetSelected(User*, INT, User*, Item*, FServerStaticObject*, FVector);  //  95 +0x17c
virtual INT OnMyTargetSelected(User*, INT, Item*, FServerStaticObject*, INT);           //  96 +0x180
virtual INT OnTargetUnselected(User*, FVector);                                         //  97 +0x184
virtual INT OnAutoAttackStart(User*);                                                   //  98 +0x188
virtual INT OnAutoAttackStop(User*);                                                    //  99 +0x18c
virtual INT OnSocialAction(User*, INT);                                                 // 100 +0x190
virtual INT OnChangeMoveType(User*, INT);                                               // 101 +0x194
virtual INT OnChangeWaitType(User*, INT, FVector);                                      // 102 +0x198
virtual INT OnStopMove(User*, FVector, INT);                                            // 103 +0x19c
virtual INT OnStopMoveInVehicle(FVehicle*, User*, FVector, INT);                        // 104 +0x1a0
virtual INT OnStopMoveWithLocation(User*, FVector);                                     // 105 +0x1a4
virtual INT OnDoorInfo(L2ParamStack&, AActor**);                                        // 106 +0x1a8
virtual INT OnDoorStatusUpdate(L2ParamStack&, AActor*);                                 // 107 +0x1ac
virtual INT OnVehicleInfo(FVehicle*, FVector, INT);                                     // 108 +0x1b0
virtual INT OnDeleteVehicle(FVehicle*);                                                 // 109 +0x1b4
virtual INT OnVehicleDeparture(FVehicle*, INT, INT, FVector);                           // 110 +0x1b8
virtual INT OnVehicleCheckLocation(FVehicle*, FVector, INT);                            // 111 +0x1bc
virtual INT OnGetOnVehicle(User*, FVehicle*, FVector);                                  // 112 +0x1c0
virtual INT OnGetOffVehicle(User*, FVehicle*, FVector);                                 // 113 +0x1c4
virtual INT OnSpecialCamera(User*, FLOAT, INT, INT, FLOAT, FLOAT);                      // 114 +0x1c8
virtual INT OnNormalCamera();                                                           // 115 +0x1cc
virtual INT OnEventTrigger(INT, INT);                                                   // 116 +0x1d0
virtual INT OnSkillRemainSec(User*, User*, INT, INT, INT, INT, FLOAT);                  // 117 +0x1d4
virtual void OnAcceptLogOn();                                                           // 118 +0x1d8
virtual void OnAcceptLogOut();                                                          // 119 +0x1dc
virtual void OnStartNotice(L2ParamStack&);                                              // 120 +0x1e0
virtual void OnReceiveCharacterInfo();                                                  // 121 +0x1e4
virtual void AddCharacterInfo(User);                                                    // 122 +0x1e8
virtual void OnAcceptGameStart(INT);                                                    // 123 +0x1ec
virtual void OnAcceptCharacterDelete(INT);                                              // 124 +0x1f0
virtual void OnAcceptNewCharacter(INT);                                                 // 125 +0x1f4
virtual void OnAcceptCharacterCreate(INT);                                              // 126 +0x1f8
virtual void OnReceiveItemList(INT);                                                    // 127 +0x1fc
virtual void OnReceiveUpdateItemList(INT, ItemInfo&);                                   // 128 +0x200
virtual void OnReceiveTradeStart(INT);                                                  // 129 +0x204
virtual void OnReceiveAddTradeInventoryItem(ItemInfo&);                                 // 130 +0x208
virtual void OnReceiveAddTradeOwnItem(ItemInfo&);                                       // 131 +0x20c
virtual void OnReceiveAddTradeOtherItem(ItemInfo&);                                     // 132 +0x210
virtual void OnReceiveTradeUpdate(INT, ItemInfo&);                                      // 133 +0x214
virtual void OnReceiveTradeDoneMsg(INT);                                                // 134 +0x218
virtual void OnReceiveCharacterSelectedPacket(INT, INT, INT);                           // 135 +0x21c
virtual void AddRecommandedData(RecommandedDataStr);                                    // 136 +0x220
virtual void EndRecommandedData();                                                      // 137 +0x224
virtual void AddInventoryItem(ItemInfo&);                                               // 138 +0x228
virtual void OnUpdatePlayerInfo(L2ParamStack&);                                         // 139 +0x22c
virtual void OnServerClose(INT);                                                        // 140 +0x230
virtual void OnReceiveHtmlMessage(L2ParamStack&);                                       // 141 +0x234
virtual void OnReceivePledgeExtendedInfo(PledgeInfo&);                                  // 142 +0x238
virtual void OnReceivePledgeInfo(PledgeInfo&);                                          // 143 +0x23c
virtual void OnReceivePledgeInfoUpdate(PledgeInfo&);                                    // 144 +0x240
virtual void OnReceivePledgeMemberList();                                               // 145 +0x244
virtual void OnAddPledgeMember(L2ParamStack&);                                          // 146 +0x248
virtual void OnUpdatePledgeMember(L2ParamStack&);                                       // 147 +0x24c
virtual void OnDeletePledgeMember(TCHAR*);                                              // 148 +0x250
virtual void OnDeleteAllPledgeMember();                                                 // 149 +0x254
virtual void OnReceiveCreatePledge(L2ParamStack&);                                      // 150 +0x258
virtual void OnRegisterPledgeCrest(INT, INT);                                           // 151 +0x25c
virtual void OnReceiveAskJoinPledge(L2ParamStack&);                                     // 152 +0x260
virtual void OnReceiveOustPledgeMember(L2ParamStack&);                                  // 153 +0x264
virtual void OnReceiveDismissPledge(L2ParamStack&);                                     // 154 +0x268
virtual void OnReceiveJoinPledge(L2ParamStack&);                                        // 155 +0x26c
virtual void OnReceiveWidthdrawalPledge(L2ParamStack&);                                 // 156 +0x270
virtual void OnReceiveOustPledgeMessage(L2ParamStack&);                                 // 157 +0x274
virtual void OnReceiveDismissPledgeMessage(L2ParamStack&);                              // 158 +0x278
virtual void OnReceiveStartPledgeWar(L2ParamStack&);                                    // 159 +0x27c
virtual void OnReceiveReplyStartPledgeWar(L2ParamStack&);                               // 160 +0x280
virtual void OnReceiveStopPledgeWar(L2ParamStack&);                                     // 161 +0x284
virtual void OnReceiveReplyStopPledgeWar(L2ParamStack&);                                // 162 +0x288
virtual void OnReceiveSurrenderPledgeWar(L2ParamStack&);                                // 163 +0x28c
virtual void OnReceiveReplySurrenderPledgeWar(L2ParamStack&);                           // 164 +0x290
virtual void OnReceiveSurrenderPersonally(L2ParamStack&);                               // 165 +0x294
virtual void OnRegisterAllyCrest(INT, INT);                                             // 166 +0x298
virtual void OnReceiveAskJoinAlly(L2ParamStack&);                                       // 167 +0x29c
virtual void OnReceiveOustAlly(L2ParamStack&);                                          // 168 +0x2a0
virtual void OnReceiveDismissAlly(L2ParamStack&);                                       // 169 +0x2a4
virtual void OnReceiveJoinAlly(L2ParamStack&);                                          // 170 +0x2a8
virtual void OnReceiveWithdrawAlly(L2ParamStack&);                                      // 171 +0x2ac
virtual void OnReceiveFriendAddRequestResult(L2ParamStack&);                            // 172 +0x2b0
virtual void OnReceiveFriendAddRequest(L2ParamStack&);                                  // 173 +0x2b4
virtual void OnReceiveFriendAdd(L2ParamStack&);                                         // 174 +0x2b8
virtual void OnReceiveFriendRemove(L2ParamStack&);                                      // 175 +0x2bc
virtual void OnReceiveFriendList(L2ParamStack&);                                        // 176 +0x2c0
virtual void OnReceiveFriendStatus(L2ParamStack&);                                      // 177 +0x2c4
virtual void OnReceiveBlockList(L2ParamStack&);                                         // 178 +0x2c8
virtual void OnReceiveAskJoinParty(L2ParamStack&);                                      // 179 +0x2cc
virtual void OnReceiveJoinParty(L2ParamStack&);                                         // 180 +0x2d0
virtual void OnReceiveWithdrawalParty(L2ParamStack&);                                   // 181 +0x2d4
virtual void OnReceiveOustPartyMember(L2ParamStack&);                                   // 182 +0x2d8
virtual void OnReceiveOustPartyMessage(L2ParamStack&);                                  // 183 +0x2dc
virtual void OnReceiveDismissParty(L2ParamStack&);                                      // 184 +0x2e0
virtual void OnRecevieDismissPartyMessage(L2ParamStack&);                               // 185 +0x2e4
virtual void OnReceiveWareHouseDonePacket(L2ParamStack&);                               // 186 +0x2e8
virtual void OnReceiveShortCutList();                                                   // 187 +0x2ec
virtual void AddShortCutItem(L2ParamStack&);                                            // 188 +0x2f0
virtual void OnUpdateShortCutItem(INT);                                                 // 189 +0x2f4
virtual void OnUpdatePetShortCutItem(INT);                                              // 190 +0x2f8
virtual void OnReceiveShortCutDelete(INT);                                              // 191 +0x2fc
virtual void OnReceiveMagicSkillListPacket(L2ParamStack&);                              // 192 +0x300
virtual void OnSay2(L2ParamStack&);                                                     // 193 +0x304
virtual void OnReceiveVersionCheck(L2ParamStack&);                                      // 194 +0x308
virtual void OnReceiveAbnormalStatus();                                                 // 195 +0x30c
virtual void AddAbnormalStatus(L2ParamStack&);                                          // 196 +0x310
virtual void OnReceiveQuestList();                                                      // 197 +0x314
virtual void AddQuestID(INT, INT);                                                      // 198 +0x318
virtual void AddQuestItem(ItemInfo&);                                                   // 199 +0x31c
virtual void OnUpdateEquipItem(L2ParamStack&);                                          // 200 +0x320
virtual void ReceivePartyMatchList();                                                   // 201 +0x324
virtual void AddPartyMatchMember(L2ParamStack&);                                        // 202 +0x328
virtual void OnReceivePartyMatchDetail(L2ParamStack&);                                  // 203 +0x32c
virtual void OnPartyAddMemberPacket(L2ParamStack&);                                     // 204 +0x330
virtual void OnPartyUpdateMemberPacket(L2ParamStack&);                                  // 205 +0x334
virtual void OnPartyDeleteMemberPacket(L2ParamStack&);                                  // 206 +0x338
virtual void OnPartyDeleteAllPacket(L2ParamStack&);                                     // 207 +0x33c
virtual void OnShowMagicWnd();                                                          // 208 +0x340
virtual void OnShowSkillWnd();                                                          // 209 +0x344
virtual void OnSkillListPacket(L2ParamStack&);                                          // 210 +0x348
virtual void OnMagicListPacket(L2ParamStack&);                                          // 211 +0x34c
virtual void OnReceiveTradeDemand(L2ParamStack&);                                       // 212 +0x350
virtual void OnRestartResponse(L2ParamStack&);                                          // 213 +0x354
virtual void OnAddSystemMessageParam(L2ParamStack&);                                    // 214 +0x358
virtual void OnEndSystemMessageParam(L2ParamStack&);                                    // 215 +0x35c
virtual void OnSetupGaugePacket(L2ParamStack&);                                         // 216 +0x360
virtual void OnShowboardPacket(L2ParamStack&);                                          // 217 +0x364
virtual void OnSelectItemToEnchant();                                                   // 218 +0x368
virtual void OnReceiveEnchantResult(INT);                                               // 219 +0x36c
virtual void OnReceiveAction(INT);                                                      // 220 +0x370
virtual void OnShowTrainListWnd();                                                      // 221 +0x374
virtual void OnShowTrainInfoWnd(L2ParamStack&);                                         // 222 +0x378
virtual void AddAcquireSkillInfo(L2ParamStack&);                                        // 223 +0x37c
virtual void AddAcquireSkillExtendedInfo(L2ParamStack&);                                // 224 +0x380
virtual void OnHideTrainWnd();                                                          // 225 +0x384
virtual void EnablePartyChat(bool);                                                     // 226 +0x388
virtual void EnablePledgeChat(bool);                                                    // 227 +0x38c
virtual void EnableAllyChat(bool);                                                      // 228 +0x390
virtual void OnShowPetStatusWnd(INT);                                                   // 229 +0x394
virtual void OnShowPetPartyWnd(INT);                                                    // 230 +0x398
virtual void OnHidePetWnd(INT);                                                         // 231 +0x39c
virtual void OnShowPetNameBtn(INT);                                                     // 232 +0x3a0
virtual void OnReceivePetStatusInfo(INT, L2ParamStack&);                                // 233 +0x3a4
virtual void OnReceivePetItemList();                                                    // 234 +0x3a8
virtual void AddPetInventoryItem(ItemInfo&);                                            // 235 +0x3ac
virtual void UpdatePetInventoryItem(INT, ItemInfo&);                                    // 236 +0x3b0
virtual void OnGMViewCharacterInfo(User&);                                              // 237 +0x3b4
virtual void OnGMViewReceivePledgeInfo(PledgeInfo&);                                    // 238 +0x3b8
virtual void OnGMViewReceivePledgeMemberList();                                         // 239 +0x3bc
virtual void OnGMViewAddPledgeMember(L2ParamStack&);                                    // 240 +0x3c0
virtual void OnGMViewShowSkillWnd();                                                    // 241 +0x3c4
virtual void OnGMViewSkillListPacket(L2ParamStack&);                                    // 242 +0x3c8
virtual void OnGMViewShowMagicWnd();                                                    // 243 +0x3cc
virtual void OnGMViewMagicListPacket(L2ParamStack&);                                    // 244 +0x3d0
virtual void OnGMViewReceiveQuestList();                                                // 245 +0x3d4
virtual void GMViewAddQuestID(INT, INT);                                                // 246 +0x3d8
virtual void GMViewAddQuestItem(ItemInfo&);                                             // 247 +0x3dc
virtual void OnGMViewReceiveItemList(INT);                                              // 248 +0x3e0
virtual void GMViewAddInventoryItem(ItemInfo&);                                         // 249 +0x3e4
virtual void OnGMViewStartWithdrawList(INT);                                            // 250 +0x3e8
virtual void OnGMViewAddWithdrawList(ItemInfo&);                                        // 251 +0x3ec
virtual void AddPackageToList(L2ParamStack&);                                           // 252 +0x3f0
virtual void AddSendableItemList(INT, INT, INT, ItemInfo&);                             // 253 +0x3f4
virtual void OnShowMinimap(INT);                                                        // 254 +0x3f8
virtual void OnStartPrivateStoreManageList();                                           // 255 +0x3fc
virtual void OnStartPrivateStoreList();                                                 // 256 +0x400
virtual void OnPrivateStoreManageList(ItemInfo&, ItemInfo&, INT, INT);                  // 257 +0x404
virtual void OnPrivateStoreList(ItemInfo&, INT, INT);                                   // 258 +0x408
virtual void OnPrivateStoreMsg(TCHAR*, INT);                                            // 259 +0x40c
virtual void OnStartPrivateStoreBuyManageList();                                        // 260 +0x410
virtual void OnStartPrivateStoreBuyList();                                              // 261 +0x414
virtual void OnPrivateStoreBuyManageList(ItemInfo&, ItemInfo&, INT, INT);               // 262 +0x418
virtual void OnPrivateStoreBuyList(ItemInfo&, INT, INT);                                // 263 +0x41c
virtual void OnPrivateStoreBuyMsg(TCHAR*, INT);                                         // 264 +0x420
virtual void OnPlaySound(L2ParamStack&);                                                // 265 +0x424
virtual void OnInterfacePlaySound(INT);                                                 // 266 +0x428
virtual INT OnStaticObjectInfo(L2ParamStack&, AActor**);                                // 267 +0x42c
virtual void OnDeleteStaticObject(FServerStaticObject*);                                // 268 +0x430
virtual INT OnMoveToActor(User*, AActor*, INT, FVector);                                // 269 +0x434
virtual void OnReceiveMagicSkillUse(User*, User*, L2ParamStack&);                       // 271 +0x43c
virtual void OnReceiveMagicSkillUse(User*, INT, L2ParamStack&);                         // 270 +0x438
virtual void OnReceiveMagicSkillCanceled(User*);                                        // 272 +0x440
virtual INT OnReceiveMagicSkillLaunched(User*, User*, INT, INT, INT);                   // 274 +0x448
virtual INT OnReceiveMagicSkillLaunched(User*, INT, INT, INT, INT);                     // 273 +0x444
virtual INT OnAssociatedAttack(User*, User*, INT, INT, INT, INT, INT, FVector);         // 275 +0x44c
virtual void OnVehicleStartPacket(L2ParamStack&);                                       // 276 +0x450
virtual void OnRequestTimeCheckPacket(L2ParamStack&);                                   // 277 +0x454
virtual void OnReceiveServerTime(INT);                                                  // 278 +0x458
virtual void OnSkillCoolTimePacket(L2ParamStack&);                                      // 279 +0x45c
virtual void OnStartAllianceWar(L2ParamStack&);                                         // 280 +0x460
virtual void OnStopAllianceWar(L2ParamStack&);                                          // 281 +0x464
virtual void OnReceiveSiegeInfo(L2ParamStack&);                                         // 282 +0x468
virtual void OnReceiveSiegeAttackerList(L2ParamStack&);                                 // 283 +0x46c
virtual void OnReceiveSiegeDefenderList(L2ParamStack&);                                 // 284 +0x470
virtual void OnAddSelectableTime(INT);                                                  // 285 +0x474
virtual void OnEndSiegeAttackerlist();                                                  // 286 +0x478
virtual void OnEndSiegeDefenderlist();                                                  // 287 +0x47c
virtual void ClearAttackerList();                                                       // 288 +0x480
virtual void ClearDefenderList();                                                       // 289 +0x484
virtual void OnSetVIPShopID(INT);                                                       // 290 +0x488
virtual void OnAddVIPItemList(INT, INT, INT, INT, INT);                                 // 291 +0x48c
virtual void OnAddVIPNeedItemList(INT, INT, INT);                                       // 292 +0x490
virtual void OnVIPItemListEnd();                                                        // 293 +0x494
virtual void OnSetSummonRemainTime(INT, INT);                                           // 294 +0x498
virtual void SetServerPing(INT, DWORD);                                                 // 295 +0x49c

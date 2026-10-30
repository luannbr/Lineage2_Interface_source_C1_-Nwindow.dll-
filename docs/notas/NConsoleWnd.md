# NConsoleWnd — mapa (rodada 5, 26/09/2026)

O console do jogo (UClass "ConsoleWnd", `CLASS_Config`, 0x3740 bytes, vtable `0x100e2978`, 285
slots). Implementa para o Engine o `UL2ConsoleWnd`: recebe os pacotes do servidor já decodificados
(cada `UGameEngine::On*` do `FL2NetNotify` chama um slot do console), guarda as janelas de topo e
repassa os pedidos das janelas ao objeto de rede do Engine. Este mapa agrupa todas as funções para
planejar as rodadas; os números vêm de `scratchpad` (conmap), conferidos à mão nos grupos 1 e 2.

## Estado na rodada 6 (26/09/2026)

**Grupos 1e e 3 completos**: a vtable inteira está declarada (285 slots, `NW_CHECK_VTABLE`) e
todos os callbacks do Engine e os pedidos virtuais têm corpo decompilado — os 1e no
`NConsoleWndNet.cpp`, os callbacks em três arquivos por assunto: `NConsoleWndLogin.cpp` (login,
seleção, criação, restart, logout, servidor fechado), `NConsoleWndItems.cpp` (inventário, troca,
lojas, armazém, entrega, lojas privadas, VIP, itens de quest e do treinador) e
`NConsoleWndEvents.cpp` (alvo, HTML, tutorial, radar, clã, aliança, amigos, party, chat,
atalhos, buffs, skills, treinador, pet, cerco, visões de GM, mensagens de sistema com
parâmetros). Os nomes da coluna "nome" abaixo são os do `NConsoleWnd.h` (✓ = decompilado).
Os grupos 4 e 5 passaram para outro trabalhador (arquivos `NConsoleWndWindows.cpp`,
`NConsoleWndText.cpp`, `NConsoleWndActions.cpp`); `ShowSystemMessage` foi feito aqui.

Tipos do Engine declarados nesta rodada: `ItemInfo` (o `NItemRequest` do NWindow deriva dele),
`PledgeInfo`, `FURL`, `UViewport::SizeX`, `FL2GameData::NoticeText`; os estados do console
3 (criação), 4 (seleção) e 6 (no mundo). A tabela de itens (`FL2GameData` +0x27E4) está descrita
como `NItemData` no `NConsoleWndItems.cpp`.

## Estado ao fim da rodada 5

**156 funções decompiladas** (✓ nas tabelas): todo o grupo 1 menos os slots virtuais (1a–1d,
120 em `src/nwindow/NConsoleWndNet.cpp`, com os getters do jogador e do alvo), o grupo 2 e
`FormatDateTime` (31 em `NConsoleWndData.cpp`) e mais 5 do grupo 5. Os nomes das lojas e do
armazém são os que o master deu (lotes clan e status). Continuam no original: os slots
virtuais (grupo 1e e o grupo 3 inteiro, esperam a declaração da vtable; os que as janelas usam
têm `nw::VirtualCall` no header, agora também 185/186/189 do cerco e 282/283 do treinador), o
grupo 4 e o resto do 5. No `NConsoleWnd.cpp` sobram 21 stubs `NW_STOCK`: `GetIMEType`,
`ShowSystemMessage`, `ShowDialog`, `ExecCommand`, `ConvertNewLines`, `HasInvalidChars`,
`HasNoAmpersand`, `HasBadWord`, `IsValidName`, `FormatSystemMessage`, `MakeSystemMessage`,
`RegisterPledgeCrest`, `RegisterAllyCrest`, `ShowMessageWnd`, `ShowWarningWnd`,
`ToggleOptionWnd`, `ReturnToCharacterSelect`, `ResetWindowPositions`, `ReloadGameTexts`,
`LockCursor` e `UnlockCursor`.

## Números

- **267** funções da classe no banco: 265 na faixa `0x10042800`–`0x100575c0`, `vfn074`
  (`0x10001700`, `ret 4` dobrado com outras classes) e o inicializador estático (`0x100d1d10`);
  mais 168 funclets de EH.
- Na mesma faixa há mais **169** funções sem classe no banco que são do console (os stubs com nome
  e os `sub_`), duas logo fora dela (`0x100427d0`, `0x100575c0`) e `GetMyLocation`
  (`0x10038540`). **Total mapeado aqui: 436 funções, ~77,6 KB.**
- **285 slots virtuais**: 0–23 `UObject`; 24–271 `UL2ConsoleWnd` (248, dos quais 230 puros no
  Engine.dll e implementados aqui; os outros 18 ficam com o Engine, ver apêndice); 272–284 do
  próprio `NConsoleWnd`.
- Quem chama:
  - os slots 26–231 são quase todos chamados pelos `UGameEngine::On*` (um pacote do servidor, um
    slot). O Engine guarda o console em `UEngine+0x58` (`FL2NetNotify` this + 0x2C);
  - os slots 230, 247–284 também são chamados pelo próprio NWindow (`nw::VirtualCall` no header);
  - as funções não virtuais são chamadas pelas janelas (`NCChatWnd`, `NCDialogBox`, as de item...).

| grupo | funções | bytes |
|---|---|---|
| 1. pedidos ao servidor (NetHandler) | 147 | ~5,6 KB |
| 2. getters de dados | 31 | ~3,0 KB |
| 3. callbacks do Engine (slots) | 178 | ~37,8 KB |
| 4. criação e gestão das janelas | 50 | ~24,4 KB |
| 5. o resto (texto, atores, ações, brasões) | 30 | ~6,9 KB |

## Layout

Campos do `NConsoleWnd` conhecidos (✓ = declarado no `NConsoleWnd.h`). Os tipos das janelas vêm
do `Initialize` (`new` + construtor + gravação no campo).

| off | campo | tipo | usado por |
|---|---|---|---|
| +0x2C | `State` ✓ | `L2ConsoleState` (UL2ConsoleWnd) | seleção de personagem, `DispatchWndMsgX`, `HookShortCutKey` |
| +0x44 | `Engine` ✓ | `UEngine*` (UL2ConsoleWnd) | todos os pedidos ao servidor (`Engine->NetHandler`, +0x5C) |
| +0x48 | personagens padrão da criação | `User[]` (do UL2ConsoleWnd, lidos pelo `NCPawnCreateWnd`) | Engine |
| +0x25F8 | personagens da seleção | `User[8]` (idem, `NCLobbyWnd`) | Engine |
| +0x36B8 | já inicializado | `UBOOL` (`Initialize` põe 1 no fim) | Initialize, Finalize, ConsolePostRender |
| +0x36BC | `WndMain` ✓ | `NCVirtualWndMain*` | todas as janelas (AttachEntry), cursor |
| +0x36C0 | `RestartMenuWnd` ✓ | `NCRestartMenuWnd*` | slots 39, 142, 155, 156, 275 |
| +0x36C4 | `OptionWnd` ✓ | `NCOptionWnd*` | `ToggleOptionWnd`, lista de servidores |
| +0x36C8 | `LoginWnd` ✓ | `NCLoginWnd*` | fluxo de login, `ReturnToLogin` |
| +0x36CC | `LobbyWnd` ✓ | `NCLobbyWnd*` (seleção de personagem) | fluxo da seleção |
| +0x36D0 | `PawnCreateWnd` ✓ | `NCPawnCreateWnd*` (criação) | fluxo da criação |
| +0x36D4 | `LoadingWnd` ✓ | `NCLoadingWnd*` | `RequestGameStart`, `AcceptGameStart` |
| +0x36D8 | `MessageWnd` ✓ | `NCMessageWnd*` (a linha sob as telas de login) | `ShowMessageWnd` |
| +0x36DC | `Desktop` ✓ | `NCConsole*` (declarado `NCWnd*`; macro `GDesktop`) | 114 funções |
| +0x36E0 | `DebugWnd` ✓ | `NCDebugWnd*` | slots 127, 241 |
| +0x36E4 | ? | zerado no construtor | Finalize |
| +0x36E8 | caixa de diálogo do login | `NCDialogBox*` | slot 38 |
| +0x36EC | `PetitionWnd` ✓ | janela de petição | Initialize, slot 142 |
| +0x36F0 | parâmetros da mensagem do sistema | `TArray` de 8 bytes (`{TCHAR* Text; INT Len}`) | slots 144, 145; destrutor `0x10055660` |
| +0x36FC | `Timers` ✓ | `TArray<NTimer>` | Tick, NSetTimer, NKillTimer; destrutor `0x100556f0` |
| +0x3708 | ? | | `MasterConsoleEventProcess` |
| +0x370C | `DeltaTime` ✓ | `FLOAT` | Tick |
| +0x3710 | Ctrl pressionado | `UBOOL` | slots 230, 267, 268 |
| +0x3714 | ? | `UBOOL` | slot 230 (`IsCtrlDown`) |
| +0x3718 | ? | | ConsolePostRender, DispatchWndMsgX |
| +0x371C | ? | | `MasterConsoleEventProcess` |
| +0x3720 | ? | (Initialize: "StartupFullscreen"?) | AcceptGameStart |
| +0x3724 | bits pendentes do tutorial | | slots 69, 71 |
| +0x3728 | `LoadingScreenID` ✓ | `INT`, 10001 no construtor | `RequestGameStart` (a tela de carregamento) |
| +0x372C | `PartyItemDistribution` ✓ | `INT` | `0x100551c0` (ações) |
| +0x3730 | ? | | `HookShortCutKey` |
| +0x3734–0x373C | visualizador de pawns | | `InitializePawnViewer`, Finalize |

O `NCConsole` (desktop) está decompilado e tem todos os filhos nomeados
(`include/nwindow/NCConsole.h`): é por ele que os callbacks do grupo 3 chegam às janelas do jogo.

## Grupo 1 — pedidos ao servidor

O objeto de rede é `Engine->NetHandler` (`UEngine+0x5C`, um `UL2NetHandler` de 228 virtuais puros;
a implementação não está em nenhum dos binários que temos, então os slots não têm nome). Os nomes
das funções vêm dos comandos de chat que as usam ou dos pacotes do protocolo C1: dentro de cada
família os slots seguem a ordem dos opcodes (ex.: 195–201 = 0x73–0x79 da loja privada de venda,
202–208 = 0x90–0x96 da de compra, 94–97 = 0x7B–0x7E do tutorial, 138/139 = 0x31/0x32 do armazém).
`*` = nome nosso, novo nesta rodada.

**Precisa do layout:** só `Engine` (+0x44). Os de 1d precisam também das janelas do login
(+0x36C8–+0x36D8), de +0x3728 e do `Desktop`.

**1a. Repasses puros** — `mov eax,[ecx+44h]; mov ecx,[eax+5Ch]; mov edx,[ecx]; jmp [edx+4*slot]`
(14 bytes; os argumentos passam direto, a contagem foi conferida nos chamadores do original):

| VA | B | nome | NH | args | nota | chamadores |
|---|---|---|---|---|---|---|
| `10042d00` | 14 | RequestLogout ✓ | 147 | - | sair do jogo | NCSystemMenuWnd |
| `100430a0` | 14 | RequestRestart ✓ | 159 | L2ParamStack& | voltar à seleção de personagem | NCSystemMenuWnd |
| `100430b0` | 14 | RequestRestartPoint ✓ | 160 | L2ParamStack& | menu de reinício (0 vila .. 4 fixo) | NCRestartMenuWnd x5 |
| `10043af0` | 14 | RequestUnEquipItem ✓ | 153 | L2ParamStack& | bits da parte do corpo | NCEquipItemWnd x3 |
| `10044250` | 14 | RequestWarehouseDeposit ✓ | 138 | L2ParamStack& | armazém, modo 0 (depositar) | NCStoreWnd |
| `10044260` | 14 | RequestWarehouseWithdraw ✓ | 139 | L2ParamStack& | armazém, modo 1 (retirar) | NCStoreWnd |
| `10044750` | 14 | RequestMagicSkillUse ✓ | 145 | L2ParamStack& |  | NCShortCutWnd x2, NCChatWnd, console, NCSkillItemWnd |
| `10044760` | 14 | RequestSkillList ✓ | 154 | L2ParamStack& |  | NCMagicSkillWnd |
| `10044890` | 14 | RequestShortCutDel ✓ | 142 | L2ParamStack& | slot | NCShortCutWnd |
| `10044940` | 14 | RequestTargetCancel ✓ | 143 | L2ParamStack& |  | NConsoleWnd x2, NCTargetStatusWnd |
| `10044950` | 14 | RequestActionUse ✓ | 157 | L2ParamStack& |  | NCShortCutWnd x2, NCDialogBox, NCActionItemWnd, console |
| `100449e0` | 14 | RequestSocialAction ✓ | 57 | INT |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `100449f0` | 14 | RequestChangeWaitType ✓ | 59 | UBOOL | sentar / levantar | NCChatWnd |
| `10044a00` | 14 | RequestChangeMoveType ✓ | 58 | UBOOL | andar / correr | NCChatWnd |
| `10044a30` | 14 | RequestPledgeMemberList ✓ | 102 | - |  | NCActionWnd |
| `10044a40` | 14 | RequestNet103 ✓ | 103 | L2ParamStack& | diálogo 0xB (não aberto no NWindow) | NCDialogBox |
| `10044a50` | 14 | RequestAnswerJoinPledge ✓ | 104 | L2ParamStack& |  | NCDialogBox x2, NConsoleWnd |
| `10044a60` | 14 | RequestOustPledgeMember ✓ | 105 | L2ParamStack& |  | NCDialogBox |
| `10044a70` | 14 | RequestNet106 ✓ | 106 | - | diálogo 0xC (idem) | NCDialogBox |
| `10044a80` | 14 | RequestStartPledgeWar ✓ | 109 | L2ParamStack& |  | NCDialogBox |
| `10044a90` | 14 | RequestReplyStartPledgeWar ✓ | 110 | L2ParamStack& |  | NCDialogBox x2, NConsoleWnd |
| `10044aa0` | 14 | RequestStopPledgeWar ✓ | 111 | L2ParamStack& |  | NCDialogBox |
| `10044ab0` | 14 | RequestReplyStopPledgeWar ✓ | 112 | L2ParamStack& |  | NCDialogBox x2, NConsoleWnd |
| `10044ac0` | 14 | RequestSurrenderPledgeWar ✓ | 113 | L2ParamStack& |  | NCDialogBox |
| `10044ad0` | 14 | RequestSurrenderPersonally ✓ | 115 | L2ParamStack& |  | NCDialogBox |
| `10044ae0` | 14 | RequestGiveNickName ✓ | 116 | L2ParamStack& | título do membro | NCDialogBox |
| `10044af0` | 14 | RequestJoinPledge ✓ | 107 | L2ParamStack& | convite | NCPledgeWnd |
| `10044b00` | 14 | RequestWithdrawalPledge ✓ | 108 | - |  | NCDialogBox |
| `10044cf0` | 14 | RequestAllyInvite ✓ | 119 | L2ParamStack& |  | NCChatWnd |
| `10044d00` | 14 | RequestAnswerJoinAlly ✓ | 120 | L2ParamStack& |  | NCDialogBox x2, NConsoleWnd |
| `10044d10` | 14 | RequestOustAlly ✓ | 121 | L2ParamStack& |  | NCDialogBox |
| `10044d20` | 14 | RequestAllyInfo ✓ | 124 | - |  | NCChatWnd |
| `10044d30` | 14 | RequestWithdrawAlly ✓ | 122 | - |  | NCDialogBox |
| `10044d40` | 14 | RequestDismissAlly ✓ | 123 | - |  | NCDialogBox |
| `10044ed0` | 14 | RequestFriendInvite ✓ | 125 | L2ParamStack& |  | NCChatWnd |
| `10044ee0` | 14 | RequestFriendDel ✓ | 126 | L2ParamStack& |  | NCChatWnd |
| `10044ef0` | 14 | RequestAnswerFriendInvite ✓ | 128 | L2ParamStack& |  | NCDialogBox x2 |
| `10044f00` | 14 | RequestFriendList ✓ | 127 | - |  | NCChatWnd |
| `10044f10` | 14 | RequestBlock ✓ | 129 | L2ParamStack& |  | NCChatWnd |
| `10044fc0` | 14 | RequestPartyInvite ✓ | 130 | L2ParamStack& |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console, +1 |
| `10044fd0` | 14 | RequestAnswerJoinParty ✓ | 131 | L2ParamStack& |  | NCDialogBox x2, NConsoleWnd |
| `10044fe0` | 14 | RequestPartyLeave ✓ | 132 | L2ParamStack& |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `10044ff0` | 14 | RequestPartyDismiss ✓ | 133 | L2ParamStack& |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `10045000` | 14 | RequestPartyMatchConfig ✓ | 135 | L2ParamStack& |  | NCConsole, NCMatchOptionWnd |
| `10045010` | 14 | RequestPartyMatchList ✓ | 136 | INT | 3: sai da sala | NCConsole x2, NCFrameCtrl |
| `10045020` | 14 | RequestPartyMatchDetail ✓ | 137 | INT |  | NCMatchListCtrl |
| `100452a0` | 14 | RequestLinkHtml (stub: RequestBypassToServer) ✓ | 91 | L2ParamStack& | "Link" das páginas HTML (NCConsole) | NCConsole |
| `100452b0` | 14 | RequestBypassToServer* ✓ | 93 | L2ParamStack& | "bypass" (RunHtmlCommand, slot 284) | NConsoleWnd |
| `100452c0` | 14 | RequestBBSWrite ✓ | 92 | L2ParamStack& | "Write" do quadro | NCConsole |
| `100452d0` | 14 | RequestShowBoard ✓ | 163 | L2ParamStack& |  | NCConsole |
| `10045440` | 14 | RequestTutorialLinkHtml ✓ | 94 | L2ParamStack& | "Link" do tutorial | NCConsole |
| `10045450` | 14 | RequestTutorialPassCmdToServer* ✓ | 95 | L2ParamStack& | "bypass" do tutorial | NConsoleWnd |
| `10045460` | 14 | RequestTutorialQuestionMark ✓ | 96 | INT |  | NCTutorialQuestionWnd |
| `10045510` | 14 | RequestPetUseItem ✓ | 190 | INT |  | NCPetInvenItemWnd x2, NCShortCutWnd |
| `10045520` | 14 | RequestGiveItemToPet ✓ | 191 | L2ParamStack& |  | NCEquipItemWnd x2, NCInvenItemWnd x2 |
| `10045530` | 14 | RequestGetItemFromPet ✓ | 192 | L2ParamStack& |  | NCPetInvenItemWnd x2 |
| `10045540` | 14 | RequestChangePetName ✓ | 194 | TCHAR* |  | NCDialogBox |
| `100456a0` | 14 | RequestPetition ✓ | 78 | TCHAR* |  | NCChatWnd |
| `100456b0` | 14 | RequestPetitionCancel ✓ | 79 | - |  | NCChatWnd, NCPetitionWnd |
| `100456c0` | 14 | RequestGMList ✓ | 80 | - |  | NCChatWnd |
| `10045bc0` | 14 | RequestVendor ✓ | 195 | - | /vendor: gerir a loja (venda) | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `10045bd0` | 14 | RequestSetPrivateStoreListSell ✓ | 196 | L2ParamStack& | lista da loja (venda) | NCPrivateShopWnd |
| `10045be0` | 14 | RequestPrivateStoreManageCancel ✓ | 197 | - | fechou a gestão sem abrir | NCPrivateShopWnd |
| `10045bf0` | 14 | RequestPrivateStoreQuitSell ✓ | 198 | - |  | NCFrameCtrl, NCConsole, NCPrivateShopWnd |
| `10045c00` | 14 | SetPrivateStoreMsg ✓ | 199 | TCHAR* |  | NCDialogBox |
| `10045c10` | 14 | RequestPrivateStoreBuy ✓ | 201 | L2ParamStack& | compra na loja de outro | NCPrivateShopWnd x2 |
| `10045c60` | 14 | RequestBuy ✓ | 202 | - | /buy: gerir a loja (compra) | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `10045c70` | 14 | RequestSetPrivateStoreListBuy ✓ | 203 | L2ParamStack& |  | NCPrivateBuyWnd |
| `10045c80` | 14 | RequestPrivateStoreBuyManageCancel ✓ | 204 | - |  | NCPrivateBuyWnd |
| `10045c90` | 14 | RequestPrivateStoreQuitBuy ✓ | 205 | - |  | NCFrameCtrl, NCConsole, NCPrivateBuyWnd |
| `10045ca0` | 14 | SetPrivateStoreBuyMsg ✓ | 206 | TCHAR* |  | NCDialogBox |
| `10045cb0` | 14 | RequestPrivateStoreSell ✓ | 208 | L2ParamStack& | venda para a loja de outro | NCPrivateBuyWnd x2 |
| `10045ed0` | 14 | RequestDwarvenManufacture ✓ | 224 | - |  | NCChatWnd |

Achado: o stub `RequestBypassToServer` (`0x100452a0`, slot 91) é o comando "Link" das páginas
(o `NCConsole` o usa; no tutorial vai para o 94, `RequestTutorialLinkHtml`); o "bypass" de verdade
(`RunHtmlCommand`, slot 284) usa o slot 93 (`0x100452b0`), ou o 95 no tutorial. Pela ordem dos
pacotes (0x20 `RequestLinkHtml`, 0x21 `RequestBypassToServer`), o 91 é `RequestLinkHtml`.

**1b. Consultas ao NetHandler** (mesma forma, devolvem um valor):

| VA | B | nome | NH | args | nota | chamadores |
|---|---|---|---|---|---|---|
| `10046480` | 11 | GetServerID ✓ | 30 | - | INT (nome das texturas de brasão) | NCGMDetailStatusWnd, NCGMPledgeWnd |
| `100464a0` | 23 | GetPetInfo ✓ | 31 | - | FL2PetInfo por valor (ponteiro oculto) | NCPetStatusWnd x2, NCSummonedStatusWnd x2, NCPetDetailStatusWnd, NCPetWnd, +2 |
| `100464c0` | 14 | FindUserByName ✓ | 32 | TCHAR* | User* | NCChatWnd |
| `100464d0` | 14 | FindUserByID ✓ | 33 | INT | User* | NConsoleWnd x7, console x4, NCConsole x2 |
| `100464e0` | 14 | FindStaticObjectInfo ✓ | 34 | INT | +4: id do staticobject | console, NCTargetStatusWnd |
| `10046490` | 14 | FindItemByID* ✓ | 35 | INT | item do chão (+4 id, +8/+0xC quantidade) | console |
| `10046620` | 14 | GetPledgeInfo ✓ | 38 | INT | FL2PledgeInfo* | NCSiegePartInfoWnd x2, NCGMDetailStatusWnd, NCTargetStatusWnd, NCDetailStatusWnd |
| `10046630` | 14 | GetPledgeCrestTexture ✓ | 39 | INT | UTexture* 16x12 | NCGDevice, NCPledgeWnd, NCListCtrl |
| `10046640` | 14 | GetAllyCrestTexture ✓ | 40 | INT | UTexture* 8x12 | NCGDevice, NCListCtrl |

**1c. Repasses com um passo a mais:**

| VA | B | nome | NH | args | nota | chamadores |
|---|---|---|---|---|---|---|
| `10044830` | 46 | RequestShortCutReg ✓ | 140 | L2ParamStack& | e toca o som 3 (FL2NetNotify slot 266) | NCActionItemWnd, NCEquipItemWnd, NCInvenItemWnd, NCSkillItemWnd, +1 |
| `10044860` | 46 | UseShortcut ✓ | 141 | L2ParamStack& | e toca o som 1 | NCChatWnd |
| `10044960` | 51 | RequestAttackForce ✓ | 52 | INT, FVector | slot 52 (id, origem, 0) | NCChatWnd |
| `100449a0` | 51 | RequestAttackStand ✓ | 52 | INT, FVector | slot 52 (id, origem, 1) | NCChatWnd |
| `100454d0` | 62 | PetPickUp ✓ | 210, 189 | INT, INT | item mais próximo (210) para o pet (189) | NCShortCutWnd x2, NCActionItemWnd, console |
| `100456d0` | 24 | RequestGMInventory ✓ | 81 | TCHAR* | comando GM 5 | NCConsole |
| `100456f0` | 24 | RequestGMSkills ✓ | 81 | TCHAR* | comando GM 3 | NCConsole |
| `10045710` | 24 | RequestGMStatus ✓ | 81 | TCHAR* | comando GM 1 | NCConsole |
| `10045730` | 24 | RequestGMQuests ✓ | 81 | TCHAR* | comando GM 4 | NCConsole |
| `10045750` | 24 | RequestGMWarehouse ✓ | 81 | TCHAR* | comando GM 6 | NCConsole |
| `10045770` | 24 | RequestGMPledge ✓ | 81 | TCHAR* | comando GM 2 | NCConsole |

**1d. Pedidos com lógica** (guard, validações, estado das janelas):

| VA | B | nome | NH | args | nota | chamadores |
|---|---|---|---|---|---|---|
| `10042b80` | 182 | SendChatMsgToServer ✓ | 146 | INT, TCHAR*, TCHAR* | (guard) | NCChatWnd, NCPetitionWnd, NCGMPetitionWnd |
| `10042c70` | 90 | RequestAuthLogin ✓ | 68 | TCHAR*, TCHAR* | (guard) | NCAutoLogWnd, NCAuthWnd |
| `10043200` | 77 | RequestServerLogin ✓ | 70 | L2ParamStack& | (guard) | NCServerSelectWnd |
| `10042d10` | 179 | RequestGameStart ✓ | 71 | INT | (guard) esconde a seleção, tela de carregamento (+0x3728) | NCLobbyWnd |
| `10042e00` | 119 | RequestCharacterDelete ✓ | 72 | INT | (guard) | NCLobbyWnd |
| `10042eb0` | 119 | RequestCharacterRestore ✓ | 73 | INT | (guard) | NCPawnSelectWnd |
| `10042f60` | 71 | RequestNewCharacter ✓ | 74 | - | (guard) | NCLobbyWnd |
| `10042fe0` | 132 | RequestCharacterCreate ✓ | 75 | NCharacterCreateInfo | (guard) struct de 0x58 por valor | NCPawnCreateWnd |
| `10045ef0` | 123 | ReturnToLogin ✓ | 168 | - | +0x36C8/CC/D8 | NCLobbyWnd |
| `100439f0` | 71 | RequestItemList ✓ | 77 | - | (guard) | NCConsole, NConsoleWnd |
| `10043a70` | 77 | RequestUseItem ✓ | 86 | INT | (guard) | NCInvenItemWnd x3, NCShortCutWnd x2, console |
| `10043de0` | 77 | RequestTrade ✓ | 82 | INT | (guard) | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console |
| `10043e60` | 90 | RequestAddTradeItem ✓ | 83 | NItemRequest | (guard) 0x30 por valor | NCTradeItemWnd x3, NCTradeWnd |
| `10043ef0` | 77 | RequestTradeDone ✓ | 84 | INT | (guard) | NCTradeWnd x2, NCFrameCtrl, NCConsole |
| `10044020` | 77 | RequestBuyItem ✓ | 89 | L2ParamStack& | (guard) NCShopWnd | NCShopWnd |
| `100440a0` | 77 | RequestSellItem ✓ | 88 | L2ParamStack& | (guard) NCShopWnd | NCShopWnd |
| `10048200` | 112 | RequestDropItem ✓ | 87 | NItemRequest, FVector | (guard) | NCEquipItemWnd, NCInvenItemWnd, NCPetInvenItemWnd |
| `1004d250` | 144 | MoveToLocation ✓ | 90 | FVector, UBOOL |  | NCConsole |
| `100464f0` | 191 | GetTargetedActor ✓ | 33 | UBOOL | (guard) pawn sob o mouse | console, NCEquipItemWnd, NCInvenItemWnd |
| `100489b0` | 134 | TargetMyself ✓ | - | - | clique na janela do jogador | NCPlayerStatusWnd |
| `10048a40` | 115 | RequestTarget ✓ | - | INT |  | NCChatWnd, NCMatchWnd, NCTargetStatusWnd, NCPartyStatusWnd |
| `10048ac0` | 137 | RequestPickUp ✓ | 211 | INT, INT |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, NConsoleWnd, +1 |
| `10048b50` | 141 | RequestTargetNext ✓ | 53, 214 | INT, INT |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, NConsoleWnd, +1 |
| `10048be0` | 143 | AssistMember ✓ | 53 | INT, FVector |  | NCPartyStatusWnd |
| `10048c70` | 82 | RequestAction ✓ | 53 | INT, FVector |  | NCPetStatusWnd, NCSummonedStatusWnd |
| `10048cd0` | 151 | RequestAttack ✓ | 52, 53 | INT, FVector |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, console, +2 |
| `10051d00` | 153 | RequestAssist ✓ | 53 | - |  | NCShortCutWnd x2, NCActionItemWnd, NCChatWnd, NConsoleWnd, +1 |

**1e. Slots virtuais que repassam** (ficam para a rodada da vtable: hoje são chamados por
`nw::VirtualCall` com esses nomes no header):

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10045470` | 44 | 71 | SendTutorialEvent ✓ | CheckTutorialClientEvent | NH 97 uma vez por bit pendente (+0x3724) |
| `10045ee0` | 14 | 147 | BuilderCommand ✓ | - | NH 164 |
| `10045d40` | 14 | 162 | RequestBuyVIPItem ✓ | - | NH 186 |
| `10047060` | 14 | 176 | vfn176 ✓ | - | NH 174 |
| `10044e10` | 14 | 177 | RequestStartAllianceWar ✓ | - | NH 175 |
| `10044e20` | 14 | 178 | RequestReplyStartAllianceWar ✓ | - | NH 176 |
| `10044e30` | 14 | 179 | RequestStopAllianceWar ✓ | - | NH 177 |
| `10044e40` | 14 | 180 | RequestReplyStopAllianceWar ✓ | - | NH 178 |
| `10044e50` | 14 | 181 | RequestSurrenderAllianceWar ✓ | - | NH 179 |
| `10044f20` | 14 | 184 | vfn184 ✓ | - | NH 180 |
| `10044f30` | 14 | 185 | RequestSiegeAttackerList ✓ | - | NH 181 |
| `10044f40` | 14 | 186 | RequestSiegeDefenderList ✓ | - | NH 182 |
| `10044f50` | 14 | 187 | RequestJoinSiege ✓ | - | NH 183 |
| `10044f60` | 14 | 188 | RequestConfirmSiegeWaitingList ✓ | - | NH 184 |
| `10044f70` | 14 | 189 | RequestSetSiegeTime ✓ | - | NH 185: a data do cerco |
| `10044e60` | 14 | 198 | RequestPackageSend ✓ | - | NH 187: a entrega |
| `10044e70` | 14 | 199 | RequestPackageSendableItemList ✓ | - | NH 188 |
| `10043180` | 71 | 272 | RequestServerList ✓ | - | (guard) NH 69 |
| `10043820` | 161 | 275 | AnswerReviveRequest ✓ | - | NH 76; +0x36C0 |
| `10043c50` | 77 | 276 | RequestEnchantItem ✓ | - | (guard) NH 171 |
| `10043d40` | 90 | 277 | RequestDestroyItem ✓ | - | (guard) |
| `10043dd0` | 14 | 278 | RequestCrystallizeItem ✓ | - | NH 166 |
| `10043f70` | 14 | 279 | AnswerTradeRequest ✓ | - | NH 156 |
| `10044380` | 100 | 280 | RequestQuestList ✓ | - | (guard) |
| `10044420` | 77 | 281 | RequestDestroyQuest ✓ | - | (guard) NH 170 |
| `10044710` | 14 | 282 | RequestAcquireSkillInfo ✓ | - | NH 172: o treinador |
| `10044720` | 14 | 283 | RequestAcquireSkill ✓ | - | NH 173 |

## Grupo 2 — getters de dados

Leem as tabelas do `GL2GameData` (import) e não tocam no console (`this` não é usado). Tabelas:
`GetMSData(id, nível)` (import, skills), `Actions[1000]` (+0x4CD8), `SysString[3000]` (+0x2E810),
a tabela de itens (+0x27E4, hash), a de castelos (+0x37E28, 100 entradas de 0x14 bytes:
`{bValid, ID, 3 L2FName}`), e uma tabela de textos do próprio NWindow (`0x1010f1f4`, graus).

**Precisa do layout:** nada do console.

| VA | B | nome | o que faz | chamadores |
|---|---|---|---|---|
| `10046650` | 43 | GetSkillIconName* ✓ | GetMSData(id, nível) +0x50 (L2FName) | NConsoleWnd x7, NCMSViewrWnd x3 |
| `10046680` | 43 | GetSkillName ✓ | GetMSData +0x4C | NConsoleWnd x8, NCMSViewrWnd x3, console, NCMagicSkillWnd, +2 |
| `100466b0` | 43 | GetSkillDesc ✓ | GetMSData +0x54 | NConsoleWnd x6, NCMagicSkillWnd, NCAbnormalStatusWnd |
| `100466e0` | 45 | IsToggleSkill ✓ | GetMSData +0x14 == 3 | NCShortCutWnd x2, NCSkillItemWnd |
| `10046710` | 101 | GetSkillTypeText ✓ | "Active Skill" / "Passive Skill" / "Magic" | NConsoleWnd x2, NCSkillItemWnd x2 |
| `10046780` | 37 | GetSkillMpCost ✓ | GetMSData +0x1C | NConsoleWnd x2, NCSkillItemWnd x2 |
| `100467b0` | 37 | GetSkillRange ✓ | GetMSData +0x2C | NCSkillItemWnd x2, NConsoleWnd, NCAbnormalStatusWnd |
| `100467e0` | 70 | GetActionIconName* ✓ | Actions[i].IconName, busca linear por ActionID | NConsoleWnd |
| `10046830` | 70 | GetActionName ✓ | Actions[i].Name | NConsoleWnd, NCShortCutWnd |
| `10046cf0` | 68 | GetCastleName* ✓ | castlename: tabela de 100 x 0x14 em +0x37E28, 1º nome (param. tipo 5 das mensagens) | NCSiegePartInfoWnd x2, sub_1002d840, NConsoleWnd, console, +1 |
| `10046d40` | 68 | GetCastleText2* ✓ | 2º nome da mesma tabela | sub_1002d840 |
| `10046d90` | 68 | GetCastleText3* ✓ | 3º nome da mesma tabela | sub_1002d840 |
| `10046f00` | 29 | GetClassNameString ✓ | SysString[247 + ID] | NCMatchListCtrl x2, NCGMDetailStatusWnd, NCPawnSelectWnd, NCDetailStatusWnd |
| `10046f20` | 23 | GetGradeText ✓ | tabela de TCHAR* 0x1010f1f4 (NULL se < 0) | NCDeliverItemWnd, NCEquipItemWnd, NCInvenItemWnd, sub_1006bbd0, +6 |
| `100468b0` | 194 | GetWeaponTypeName ✓ | sysstring 43..504 por tipo | NCEquipItemWnd x2, NCInvenItemWnd x2, NCPetInvenItemWnd x2, NCTradeItemWnd x2, +12 |
| `10046c70` | 113 | GetAttackSpeedText ✓ | sysstring 306..310 | NCEquipItemWnd x2, NCInvenItemWnd x2, NCPetInvenItemWnd x2, NCTradeItemWnd x2, +11 |
| `100469a0` | 714 | GetItemSlotText ✓ | linha de slot da dica (tipo, parte do corpo, subtipo) | NCEquipItemWnd x2, NCInvenItemWnd x2, NCPetInvenItemWnd x2, NCQuestItemWnd x2, +13 |
| `10046880` | 38 | IsPetCollar ✓ | 2375, 3500..3502 | NCInvenItemWnd x2, NCPetInvenItemWnd x2, NCTradeItemWnd x2, sub_10057bb0, +7 |
| `10047120` | 66 | IsShotItem ✓ | soulshots / spiritshots por faixa de id | NCShortCutWnd |
| `10054df0` | 48 | IsMagicArmor ✓ | item table (+0x27E4) +0x1F4 == 3 | NCEquipItemWnd x2, NCInvenItemWnd x2, NCPetInvenItemWnd x2, NCTradeItemWnd x2, +11 |
| `10054d40` | 121 | GetItemName ✓ | (guard) nome do item (+ "?") | console x2, NCNpcViewrWnd x2, NCPawnViewrWnd x2, NConsoleWnd, +1 |
| `10054c90` | 121 | GetIconName ✓ | (guard) ícone do item | NCNpcViewrWnd x2, NCPawnViewrWnd x2 |
| `10047040` | 16 | GetWeaponEnchantSteps* ✓ | enchant > 3: 2*e - 3 | console |
| `10047050` | 16 | GetArmorEnchantSteps* ✓ | enchant > 3: 3*e - 6 | console |
| `1004cfd0` | 43 | GetEnchantedAttack ✓ | base + passos * tabela[grau] (double, _ftol) | NCDeliverItemWnd, NCEquipItemWnd, NCInvenItemWnd, sub_1006bbd0, +6 |
| `1004d000` | 43 | GetEnchantedDefense ✓ | idem, tabela das armaduras | NCDeliverItemWnd, NCEquipItemWnd, NCInvenItemWnd, sub_1006bbd0, +6 |
| `10044600` | 50 | AddQuestID (não virtual) ✓ | (guard) texto da quest no passo (GL2GameData, questname) | NCQuestInfoWnd x3 |
| `10047070` | 169 | IsSiegeNpc ✓ | guardas, teleportadores e mercenários do castelo (por NpcID) | NCTargetStatusWnd |
| `1004d2e0` | 78 | HasAbnormal ✓ | o efeito da skill está no jogador (buffs) | NCSkillItemWnd, NCShortCutWnd |
| `1004d030` | 344 | GetEquipSlot ✓ | slot do paperdoll; 14/15 = itens de dois slots | NCEquipItemWnd x7 |
| `1004dda0` | 69 | (TMap find) | hash id -> entrada (12 B: next, key, value); tabela de itens do GL2GameData | NConsoleWnd x26, console, NCEquipItemWnd, NCInvenItemWnd, +2 |

## Grupo 3 — callbacks do Engine

Slots chamados pelos `UGameEngine::On*` (coluna do meio). Quase todos só repassam para uma janela
filha do `Desktop` ou abrem um diálogo (`NDialogInfo` + `ShowDialog`); os "mensagem" chamam
`ShowSystemMessage` com um id fixo.

**Precisa do layout:** `Desktop` (+0x36DC) e as janelas do login (+0x36C0–+0x36D8, +0x36E8).
Do lado das janelas: os filhos do `NCConsole` e as classes delas (troca, lojas, armazém, clã,
party, pet, cerco, GM). Os de itens constroem `NItemInfo` a partir da tabela de itens
(`0x1004dda0`).

**Login, seleção e criação**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10045870` | 1 | 27/137/208/42/64 | AuthLoginSuccess / ShowMagicWnd / GMViewShowMagicWnd ✓ | OnAuthLoginSuccess, OnAcceptLogOn, OnShowMagicWnd, OnGMViewShowMagicWnd, OnEndServerList, OnReceiveWareHouseDonePacket | slots 27 / 137 / 208 (e de outras classes: ICF) |
| `10047790` | 638 | 28 | AuthLoginFail ✓ | OnAuthLoginFail | falha no login: ShowMessageWnd (+0x36C8) |
| `10047a70` | 217 | 29 | StartNotice ✓ | OnStartNotice | avisos (NCNoticeWnd) |
| `10047b90` | 223 | 30 | CharacterCreateFail ✓ | OnCharacterCreateFail | falha na criação (NCPawnSetupWnd) |
| `100434a0` | 167 | 31 | ReceiveCharacterInfo ✓ | OnReceiveCharacterInfo | (guard) +0x36C4..D0 |
| `10043580` | 130 | 32 | AddCharacterInfo ✓ | AddCharacterInfo | (guard) NCLobbyWnd::AddCharacter |
| `10047c80` | 219 | 33 | AcceptGameStart ✓ | OnAcceptGameStart | (guard) +0x36D4, +0x3720 |
| `10047d90` | 127 | 34 | AcceptCharacterDelete ✓ | OnAcceptCharacterDelete | (guard) |
| `10047e40` | 186 | 35 | AcceptNewCharacter ✓ | OnAcceptNewCharacter | (guard) +0x36CC/D0 |
| `10047f30` | 94 | 36 | AcceptCharacterCreate ✓ | OnAcceptCharacterCreate | (guard) |
| `1004f820` | 15 | 37/273 | AcceptLogOut / QuitGame ✓ | OnAcceptLogOut | logout e QuitGame (273): chama 0x1004a3d0 |
| `1004f6b0` | 362 | 38 | ServerClose ✓ | OnServerClose | servidor fechou: caixa +0x36E8 |
| `1004f830` | 129 | 39 | ReviveRequest ✓ | OnReviveRequest | pedido de ressurreição: diálogo |
| `10043280` | 215 | 40 | ReceiveServerList ✓ | OnStartServerList | (guard) +0x36C4/C8/D8 |
| `10043390` | 80 | 41 | AddServerList ✓ | OnAddServerList | (guard) |
| `10043420` | 74 | 42 | EndServerList ✓ | OnEndServerList | (guard) |
| `100438e0` | 87 | 52 | AddRecommandedData ✓ | AddRecommandedData | (guard) +0x36D0 |
| `10043970` | 68 | 53 | EndRecommandedData ✓ | EndRecommandedData | (guard) |
| `10043640` | 85 | 142 | RestartResetWindows ✓ | OnRestartResponse | resposta ao restart (1): +0x36C0/C4/D4/DC/EC |
| `100436a0` | 381 | 143 | RestartToLobby ✓ | OnRestartResponse | resposta ao restart (2): volta à seleção |
| `10047490` | 289 | 227 | BlockedAccountWithMsg ✓ | OnBlockedAccountWithMsg | conta bloqueada (com texto) |
| `100475c0` | 458 | 228 | BlockedAccount ✓ | OnBlockedAccount | conta bloqueada (código) |
| `10047170` | 24 | 260 | SetServerPing ✓ | SetServerPing | ping do servidor (NCServerSelectWnd::SetPing) |
| `10047fc0` | 85 | 274 | ReceiveServerClose ✓ | - | (guard) slot 274 |
| `10048050` | 158 | 128 | ReceiveVersionCheck ✓ | OnReceiveVersionCheck | versão do protocolo |

**Itens: inventário, troca, lojas, armazém, entrega**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10043b00` | 122 | 44 | ReceiveItemList ✓ | OnReceiveItemList | (guard) inventário |
| `10043bb0` | 74 | 45 | ReceiveUpdateItemList ✓ | - | (guard) slot 45, sem handler |
| `1004f8c0` | 688 | 54 | AddInventoryItem ✓ | AddInventoryItem | (guard) |
| `1004fbb0` | 759 | 55 | UpdateInventoryItem ✓ | OnReceiveUpdateItemList | atualiza item do inventário |
| `10043d20` | 29 | 129 | UpdateEquipItem ✓ | OnUpdateEquipItem | só consome 3 parâmetros |
| `10043c30` | 20 | 148 | SelectItemToEnchant ✓ | OnSelectItemToEnchant | escolher item para encantar |
| `10043cd0` | 74 | 149 | ReceiveEnchantResult ✓ | OnReceiveEnchantResult | resultado do encantamento |
| `1004ffb0` | 210 | 46 | ReceiveTradeStart ✓ | OnReceiveTradeStart | (guard) |
| `100500c0` | 698 | 47 | ReceiveAddTradeInventoryItem ✓ | OnReceiveAddTradeInventoryItem | troca: item do inventário |
| `10050380` | 698 | 48 | ReceiveAddTradeOwnItem ✓ | OnReceiveAddTradeOwnItem | troca: meu item |
| `10050640` | 698 | 49 | ReceiveAddTradeOtherItem ✓ | OnReceiveAddTradeOtherItem | troca: item do outro |
| `10050900` | 703 | 50 | ReceiveTradeUpdate ✓ | OnReceiveTradeUpdate | troca: atualização |
| `10043f80` | 98 | 51 | ReceiveTradeDoneMsg ✓ | OnReceiveTradeDoneMsg | (guard) |
| `1004feb0` | 244 | 141 | ReceiveTradeDemand ✓ | OnReceiveTradeDemand | pedido de troca: diálogo |
| `10044120` | 127 | 56 | StartBuyList ✓ | OnStartBuyList | loja NPC: início da lista de compra |
| `10050bc0` | 685 | 57 | AddBuyList ✓ | OnAddBuyList | loja NPC: item de compra |
| `100441a0` | 171 | 58 | StartSellList ✓ | OnStartSellList | loja NPC: início da lista de venda |
| `10050e70` | 685 | 59 | AddSellList ✓ | OnAddSellList | loja NPC: item de venda |
| `10044270` | 149 | 60 | StartDepositList ✓ | OnStartDepositList | armazém: início (depósito) |
| `10051120` | 685 | 61 | AddDepositList ✓ | OnAddDepositList | armazém: item (depósito) |
| `10044310` | 105 | 62 | StartWithdrawList ✓ | OnStartWithdrawList | armazém: início (retirada) |
| `100513d0` | 685 | 63 | AddWithdrawList ✓ | OnAddWithdrawList | armazém: item (retirada) |
| `100561c0` | 38 | 64 | ReceiveWareHouseDone ✓ | OnReceiveWareHouseDonePacket | armazém: pronto (mensagem) |
| `100447a0` | 56 | 261 | ToggleInventory ✓ | - | (sem handler) pede a lista de itens |
| `10044e80` | 80 | 200 | AddPackageToList ✓ | AddPackageToList | entrega: pacote na lista |
| `10052410` | 768 | 201 | AddSendableItemList ✓ | AddSendableItemList | entrega: itens enviáveis |
| `10045c20` | 41 | 218/219 | StartPrivateStoreManageList / StartPrivateStoreList ✓ | OnStartPrivateStoreManageList, OnStartPrivateStoreList | loja privada (venda): início (slots 218 e 219) |
| `10053590` | 1462 | 220 | PrivateStoreManageList ✓ | OnPrivateStoreManageList | loja privada (venda): gestão |
| `10053b50` | 808 | 221 | PrivateStoreList ✓ | OnPrivateStoreList | loja privada (venda): lista |
| `10045c50` | 3 | 222 | PrivateStoreMsg ✓ | OnPrivateStoreMsg, OnPrivateStoreBuyMsg | mensagem da loja privada: vazio (ret 8) |
| `10045cc0` | 41 | 223/224 | StartPrivateStoreBuyManageList / StartPrivateStoreBuyList ✓ | OnStartPrivateStoreBuyManageList, OnStartPrivateStoreBuyList | loja privada (compra): início (223 e 224) |
| `10053e80` | 1430 | 225 | PrivateStoreBuyManageList ✓ | OnPrivateStoreBuyManageList | loja privada (compra): gestão |
| `10054420` | 818 | 226 | PrivateStoreBuyList ✓ | OnPrivateStoreBuyList | loja privada (compra): lista |
| `10045cf0` | 17 | 158 | SetVIPShopID ✓ | OnSetVIPShopID | loja VIP: id |
| `10054760` | 517 | 159 | AddVIPItemList ✓ | OnAddVIPItemList | loja VIP: item |
| `10054970` | 189 | 160 | AddVIPNeedItemList ✓ | OnAddVIPNeedItemList | loja VIP: item necessário |
| `10045d10` | 41 | 161 | VIPItemListEnd ✓ | OnVIPItemListEnd | loja VIP: fim da lista |

**Clã, aliança, amigos, party, chat**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10048d70` | 543 | 75 | ReceivePledgeInfo ✓ | OnReceivePledgeInfo | (guard) |
| `10048fe0` | 445 | 76 | ReceivePledgeInfoUpdate ✓ | OnReceivePledgeInfoUpdate | (guard) |
| `10044b10` | 86 | 77 | ReceivePledgeMemberList ✓ | OnReceivePledgeMemberList | (guard) |
| `100491f0` | 196 | 78 | AddPledgeMember ✓ | OnAddPledgeMember | (guard) |
| `100492f0` | 196 | 79 | UpdatePledgeMember ✓ | OnUpdatePledgeMember | (guard) |
| `10044ba0` | 92 | 80 | DeletePledgeMember ✓ | OnDeletePledgeMember | (guard) |
| `10044c30` | 140 | 81 | DeleteAllPledgeMember ✓ | OnDeleteAllPledgeMember | (guard) |
| `10056750` | 52 | 82 | ReceiveCreatePledge ✓ | OnReceiveCreatePledge | clã criado: mensagem |
| `10051e10` | 245 | 83 | ReceiveAskJoinPledge ✓ | OnReceiveAskJoinPledge | convite para o clã: diálogo |
| `10056790` | 52 | 84 | ReceiveOustPledgeMember ✓ | OnReceiveOustPledgeMember | membro expulso: mensagem |
| `100567d0` | 52 | 85 | ReceiveDismissPledge ✓ | OnReceiveDismissPledge | clã dissolvido: mensagem |
| `10056720` | 38 | 86 | ReceiveJoinPledge ✓ | OnReceiveJoinPledge | entrou no clã: mensagem |
| `10056810` | 52 | 87 | ReceiveWithdrawalPledge ✓ | OnReceiveWidthdrawalPledge | saiu do clã: mensagem |
| `10056850` | 18 | 88 | ReceiveOustPledgeMessage ✓ | OnReceiveOustPledgeMessage | mensagem |
| `10056870` | 57 | 89 | ReceiveDismissPledgeMessage ✓ | OnReceiveDismissPledgeMessage | mensagem |
| `10051f10` | 253 | 90 | ReceiveStartPledgeWar ✓ | OnReceiveStartPledgeWar | guerra de clãs (início): diálogo |
| `100568b0` | 61 | 91 | ReceiveReplyStartPledgeWar ✓ | OnReceiveReplyStartPledgeWar | mensagem |
| `10052010` | 253 | 92 | ReceiveStopPledgeWar ✓ | OnReceiveStopPledgeWar | guerra de clãs (fim): diálogo |
| `100568f0` | 61 | 93 | ReceiveReplyStopPledgeWar ✓ | OnReceiveReplyStopPledgeWar | mensagem |
| `10056930` | 38 | 94 | ReceiveSurrenderPledgeWar ✓ | OnReceiveSurrenderPledgeWar | mensagem |
| `10056960` | 38 | 95 | ReceiveSurrenderPersonally ✓ | OnReceiveSurrenderPersonally | mensagem |
| `10052110` | 254 | 103 | ReceiveAskJoinAlly ✓ | OnReceiveAskJoinAlly | convite para a aliança: diálogo |
| `10044dd0` | 56 | 104 | ReceiveDismissAlly ✓ | OnReceiveDismissAlly | mensagem no chat |
| `10056990` | 52 | 105 | ReceiveJoinAlly ✓ | OnReceiveJoinAlly | mensagem |
| `10044d90` | 56 | 106 | ReceiveOustAlly ✓ | OnReceiveOustAlly | mensagem no chat |
| `10044d50` | 56 | 107 | ReceiveWithdrawAlly ✓ | OnReceiveWithdrawAlly | mensagem no chat |
| `10052210` | 254 | 182 | StartAllianceWar ✓ | OnStartAllianceWar | guerra de alianças (início): diálogo |
| `10052310` | 246 | 183 | StopAllianceWar ✓ | OnStopAllianceWar | guerra de alianças (fim): diálogo |
| `100569d0` | 52 | 108 | ReceiveFriendAddRequestResult ✓ | OnReceiveFriendAddRequestResult | amizade: resultado |
| `10052710` | 119 | 109 | ReceiveFriendAddRequest ✓ | OnReceiveFriendAddRequest | pedido de amizade: diálogo |
| `10052790` | 350 | 96 | ReceiveAskJoinParty ✓ | OnReceiveAskJoinParty | convite para a party: diálogo |
| `10056a10` | 38 | 97 | ReceiveJoinParty ✓ | OnReceiveJoinParty | mensagem |
| `10045030` | 56 | 98 | ReceiveWithdrawalParty ✓ | OnReceiveWithdrawalParty | mensagem no chat |
| `10056a40` | 38 | 99 | ReceiveOustPartyMember ✓ | OnReceiveOustPartyMember | mensagem |
| `10056a70` | 18 | 100 | ReceiveOustPartyMessage ✓ | OnReceiveOustPartyMessage | mensagem |
| `10045070` | 56 | 101 | ReceiveDismissParty ✓ | OnReceiveDismissParty | mensagem no chat |
| `10056a90` | 18 | 102 | ReceiveDismissPartyMessage ✓ | OnRecevieDismissPartyMessage | mensagem |
| `100450b0` | 153 | 130 | PartyAddMember ✓ | OnPartyAddMemberPacket | party: membro novo |
| `10045150` | 126 | 131 | PartyUpdateMember ✓ | OnPartyUpdateMemberPacket | party: membro atualizado |
| `100451d0` | 52 | 132 | PartyDeleteMember ✓ | OnPartyDeleteMemberPacket | party: membro saiu |
| `10045210` | 43 | 133 | PartyDeleteAll ✓ | OnPartyDeleteAllPacket | party: todos saíram |
| `10045240` | 17 | 134 | ReceivePartyMatchList ✓ | ReceivePartyMatchList | party matching: lista |
| `10045260` | 17 | 135 | AddPartyMatchMember ✓ | AddPartyMatchMember | party matching: jogador |
| `10045280` | 17 | 136 | ReceivePartyMatchDetail ✓ | OnReceivePartyMatchDetail | party matching: detalhe |
| `10045d50` | 17 | 163 | EnablePartyChat ✓ | EnablePartyChat | chat de party liberado |
| `10045d70` | 17 | 164 | EnablePledgeChat ✓ | EnablePledgeChat | chat de clã liberado |
| `10045d90` | 17 | 165 | EnableAllyChat ✓ | EnableAllyChat | chat de aliança liberado |
| `10054a30` | 289 | 126 | Say2 ✓ | OnSay2 | Say2: chat (filtro de palavrões, balões) |
| `100429b0` | 407 | 239 | NpcSay ✓ | OnNpcSay | (guard) |

**Personagem: alvo, atalhos, buffs, skills, treinador, quests, mensagens, HTML, tutorial, radar**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10045e70` | 94 | 65 | UpdatePlayerInfo ✓ | OnUpdatePlayerInfo | copia a pilha de parâmetros e descarta |
| `10044a10` | 17 | 43 | MyTargetSelected ✓ | OnMyTargetSelected | alvo selecionado: diferença de nível |
| `100438d0` | 14 | 156 | TeleportToLocation ✓ | OnTeleportToLocation | teleporte: esconde o menu de reinício |
| `100480f0` | 270 | 155 | ShowRestartMenu ✓ | - | (sem handler) morte: menu de reinício, buffs |
| `10051da0` | 107 | 150 | ReceiveAction ✓ | OnReceiveAction | ação recebida (ataque / alvo automático) |
| `100448a0` | 17 | 115 | ReceiveShortCutList ✓ | OnReceiveShortCutList | atalhos: lista |
| `100485b0` | 676 | 116 | AddShortCutItem ✓ | AddShortCutItem | atalhos: item |
| `100448c0` | 24 | 117 | UpdateShortCutItem ✓ | OnUpdateShortCutItem | atalhos: atualiza |
| `100448e0` | 24 | 118 | UpdatePetShortCutItem ✓ | OnUpdatePetShortCutItem | atalhos: atualiza (pet) |
| `10044900` | 24 | 119 | ReceiveShortCutDelete ✓ | OnReceiveShortCutDelete | atalhos: apaga |
| `10044770` | 13 | 120 | ReceiveMagicSkillList ✓ | OnReceiveMagicSkillListPacket | lista de magias: vazio (ret 4) |
| `10044920` | 17 | 121 | ReceiveAbnormalStatus ✓ | OnReceiveAbnormalStatus | buffs: limpa |
| `10048860` | 331 | 122 | AddAbnormalStatus ✓ | AddAbnormalStatus | buffs: adiciona |
| `1004d330` | 385 | 175 | SkillCoolTime ✓ | OnSkillCoolTimePacket | tempo de recarga das skills |
| `10044780` | 29 | 138 | ShowSkillWnd ✓ | OnShowSkillWnd | janela de skills: limpa |
| `10048470` | 308 | 139 | ReceiveSkillList ✓ | OnSkillListPacket | janela de skills: skill |
| `100446d0` | 58 | 151 | ShowTrainListWnd ✓ | OnShowTrainListWnd | treinador: lista |
| `10051820` | 617 | 152 | ShowTrainInfoWnd ✓ | OnShowTrainInfoWnd | treinador: informação |
| `100482b0` | 448 | 153 | AddAcquireSkillInfo ✓ | AddAcquireSkillInfo | treinador: skill a aprender |
| `10051a90` | 614 | 154 | AddAcquireSkillExtendedInfo ✓ | AddAcquireSkillExtendedInfo | treinador: itens necessários |
| `10044730` | 20 | 157 | HideTrainWnd ✓ | OnHideTrainWnd | treinador: esconde |
| `100444a0` | 80 | 123 | ReceiveQuestList ✓ | OnReceiveQuestList | (guard) |
| `10044530` | 53 | 124 | AddQuestID ✓ | AddQuestID | (guard) |
| `10051680` | 352 | 125 | AddQuestItem ✓ | AddQuestItem | (guard) |
| `10056ab0` | 203 | 144 | AddSystemMessageParam ✓ | OnAddSystemMessageParam | parâmetro de mensagem do sistema (lista +0x36F0) |
| `10056ba0` | 1009 | 145 | EndSystemMessageParam ✓ | OnEndSystemMessageParam | fim dos parâmetros: monta e mostra (+0x36F4) |
| `100452e0` | 81 | 66 | NpcHtmlMessage ✓ | OnNpcHtmlMessage | HTML do NPC |
| `10045340` | 49 | 67 | TutorialShowHtml ✓ | OnTutorialShowHtml | tutorial: página |
| `10049990` | 45 | 68 | TutorialShowQuestionMark ✓ | OnTutorialShowQuestionMark | tutorial: interrogação |
| `100454a0` | 13 | 69 | TutorialEnableClientEvent ✓ | OnTutorialEnableClientEvent | tutorial: liga eventos do cliente (+0x3724) |
| `100454b0` | 20 | 70 | TutorialCloseHtml ✓ | OnTutorialCloseHtml | tutorial: fecha |
| `10045b20` | 65 | 72 | ShowRadar ✓ | OnShowRadar | radar: alvo |
| `10045b70` | 65 | 73 | DeleteRadar ✓ | OnDeleteRadar | radar: remove |
| `10045ae0` | 57 | 217 | ShowMapItem ✓ | OnShowMinimap | minimapa (ShowMapItem) |
| `10045380` | 181 | 146 | ShowBoard ✓ | OnShowboardPacket | quadro da comunidade |

**Pet e summon**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10045550` | 120 | 166 | ShowPetStatusWnd ✓ | OnShowPetStatusWnd | pet: janela de status |
| `100455d0` | 102 | 167 | ShowPetPartyWnd ✓ | OnShowPetPartyWnd | pet: janela da party |
| `100499f0` | 148 | 168 | HideSummonWnd ✓ | - | (sem handler) esconde pet / summon |
| `10049a90` | 148 | 169 | HidePetWnd ✓ | OnHidePetWnd | esconde pet / summon |
| `10045640` | 17 | 170 | ShowPetNameBtn ✓ | OnShowPetNameBtn | pet: botão de nome |
| `10045660` | 25 | 171 | ReceivePetStatusInfo ✓ | OnReceivePetStatusInfo | pet: status |
| `10045680` | 17 | 172 | ReceivePetItemList ✓ | OnReceivePetItemList | pet: inventário (limpa) |
| `100528f0` | 650 | 173 | AddPetInventoryItem ✓ | AddPetInventoryItem | (guard) |
| `10052bb0` | 724 | 174 | UpdatePetInventoryItem ✓ | UpdatePetInventoryItem | pet: atualiza item |
| `100499c0` | 48 | 229 | SetSummonRemainTime ✓ | OnSetSummonRemainTime | summon: tempo restante |

**Cerco e visualização de GM**

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `100493f0` | 268 | 190 | ReceiveSiegeInfo ✓ | OnReceiveSiegeInfo | cerco: informação |
| `10049500` | 239 | 191 | ReceiveSiegeAttackerList ✓ | OnReceiveSiegeAttackerList | cerco: atacantes |
| `100495f0` | 250 | 192 | ReceiveSiegeDefenderList ✓ | OnReceiveSiegeDefenderList | cerco: defensores |
| `100496f0` | 23 | 193 | AddSelectableTime ✓ | OnAddSelectableTime | cerco: horário selecionável |
| `10049710` | 23 | 194 | EndSiegeAttackerList ✓ | OnEndSiegeAttackerlist | cerco: fim dos atacantes |
| `10049730` | 23 | 195 | EndSiegeDefenderList ✓ | OnEndSiegeDefenderlist | cerco: fim dos defensores |
| `10044f80` | 23 | 196 | ClearAttackerList ✓ | ClearAttackerList | cerco: limpa atacantes |
| `10044fa0` | 23 | 197 | ClearDefenderList ✓ | ClearDefenderList | cerco: limpa defensores |
| `10045790` | 45 | 202 | GMViewCharacterInfo ✓ | OnGMViewCharacterInfo | GM: personagem |
| `10049b30` | 488 | 203 | GMViewReceivePledgeInfo ✓ | OnGMViewReceivePledgeInfo | (guard) |
| `100457c0` | 80 | 204 | GMViewReceivePledgeMemberList ✓ | OnGMViewReceivePledgeMemberList | (guard) |
| `10049d60` | 190 | 205 | GMViewAddPledgeMember ✓ | OnGMViewAddPledgeMember | (guard) |
| `10045850` | 23 | 206 | GMViewShowSkillWnd ✓ | OnGMViewShowSkillWnd | GM: skills (limpa) |
| `10049e50` | 302 | 207 | GMViewSkillList ✓ | OnGMViewSkillListPacket | GM: skill |
| `10049f80` | 291 | 209 | GMViewMagicList ✓ | OnGMViewMagicListPacket | GM: magia |
| `10045880` | 74 | 210 | GMViewReceiveQuestList ✓ | OnGMViewReceiveQuestList | (guard) |
| `10045900` | 49 | 211 | GMViewAddQuestID ✓ | GMViewAddQuestID | (guard) |
| `10052e90` | 330 | 212 | GMViewAddQuestItem ✓ | GMViewAddQuestItem | (guard) |
| `100459d0` | 122 | 213 | GMViewReceiveItemList ✓ | OnGMViewReceiveItemList | (guard) |
| `10053010` | 688 | 214 | GMViewAddInventoryItem ✓ | GMViewAddInventoryItem | (guard) |
| `10045a80` | 85 | 215 | GMViewStartWithdrawList ✓ | OnGMViewStartWithdrawList | GM: armazém (início) |
| `10053300` | 646 | 216 | GMViewAddWithdrawList ✓ | OnGMViewAddWithdrawList | GM: armazém (item) |

Também vazio: `0x10001700` (`ret 4`, slots 74, 110–114, 140 e 271: informação estendida do clã, as
listas de amigos e bloqueio, a lista de magias), dobrado com outras classes.

## Grupo 4 — criação e gestão das janelas

**Precisa do layout:** todos os campos de janela (+0x36B8–+0x36EC), `Timers`, +0x3708–+0x3730,
o visualizador (+0x3734–+0x373C). O `Initialize` cria, na ordem: `NCVirtualWndMain` (+0x36BC),
`NCMessageWnd` (+0x36D8), `NCLoginWnd` (+0x36C8), `NCRestartMenuWnd` (+0x36C0), `NCLobbyWnd`
(+0x36CC), `NCPawnCreateWnd` (+0x36D0), `NCLoadingWnd` (+0x36D4), `NCDebugWnd` (+0x36E0),
`NCDialogBox` (+0x36E8), a janela de petição (+0x36EC), `NCConsole` (+0x36DC) e `NCOptionWnd`
(+0x36C4); depois lê o `AutoLog.txt` e põe +0x36B8 = 1.

Virtuais:

| VA | B | slot | nome | UGameEngine:: | o que faz |
|---|---|---|---|---|---|
| `10055890` | 36 | 3 | `scalar deleting destructor' | - | slot 3: não fazer gancho |
| `100551a0` | 17 | 10 | Destroy | - | slot 10: Finalize |
| `1004eee0` | 121 | 236 | Init | - | slot 236: Initialize |
| `100550f0` | 121 | 253 | InitForPawnViewer | - | slot 253: InitializePawnViewer |
| `10046f50` | 161 | 254 | PawnViewerFunction | - | slot 254 |
| `10055cf0` | 290 | 24 | Tick | - | slot 24: timers (+0x36FC), DeltaTime, NCShortCutWnd::Tick |
| `10055e50` | 152 | 237 | ConsolePostRender | - | slot 237: desenha (+0x36B8/BC, +0x3718) |
| `10057500` | 80 | 238 | DispatchWndMsg | - | slot 238 |
| `10055f20` | 363 | 240 | MasterConsoleEventProcess | - | slot 240 (+0x3708, +0x371C) |
| `10045ff0` | 44 | 267 | vfn267 | - | Ctrl pressionado (+0x3710): cursor |
| `10046020` | 41 | 268 | vfn268 | - | Ctrl solto |
| `10046050` | 29 | 230 | IsCtrlDown (slot 230) | - | +0x3710 || +0x3714 |
| `10047190` | 147 | 241 | vfn241 | - | comandos "L2Debug" / "L2DebugWindow" (+0x36E0) |
| `10045e00` | 88 | 127 | vfn127 | - | log de depuração (NCDebugWnd::AddLog) |
| `10045db0` | 75 | 231 | vfn231 | - | MessageBox com texto formatado |
| `100447e0` | 14 | 262 | vfn262 | - | Desktop->ToggleCharacter(0) |
| `100447f0` | 53 | 264 | vfn264 | - | barra de atalhos: página seguinte / anterior |
| `10057590` | 37 | 263 | vfn263 | - | atalho N (0x100561f0) |
| `10047b50` | 26 | 269 | vfn269 | - | ShortCutWnd +0x1E8 = byte |
| `10047b70` | 26 | 270 | vfn270 | - | ShortCutWnd +0x1EC = byte |
| `1004d190` | 186 | 257 | vfn257 | - | (+0x36DC) |
| `1004d4c0` | 437 | 265 | vfn265 | - | (FindUserByID, _ftol) |
| `1004d680` | 59 | 266 | vfn266 | - | Desktop +0x170..0x17C = retângulo |
| `10042990` | 20 | 26 | vfn026 | - | NCVirtualWndMain::IsOverWindow |
| `10049750` | 573 | 284 | RunHtmlCommand (slot 284) | - | "bypass [-h]": NH 93, ou 95 no tutorial |

Não virtuais:

| VA | B | nome | o que faz | chamadores |
|---|---|---|---|---|
| `100557c0` | 200 | NConsoleWnd (ctor) | zera os campos +0x36B8..+0x3730, TArrays | - |
| `100558c0` | 120 | ~NConsoleWnd | destrói +0x36F0/+0x36F4 (0x10055660 / 0x100556f0) e os Timers | NConsoleWnd |
| `1004de70` | 3212 | Initialize | cria todas as janelas de topo (tabela de campos acima), lê Option.ini, AutoLogOn | NConsoleWnd |
| `1004b020` | 4160 | 0x1004b020 | 4160 B, chamada só pelo Initialize: posições das janelas? (+0x36DC) | NConsoleWnd |
| `1004eb30` | 884 | Finalize | destrói as janelas de topo | NConsoleWnd x2 |
| `1004a3d0` | 3151 | 0x1004a3d0 | 3151 B, no logout (slot 37): grava as posições? (+0x36DC) | NConsoleWnd |
| `1004c060` | 3598 | ResetWindowPositions | todas as janelas no lugar padrão (+0x36C0..D0, +0x36DC) | NCGameOptionWnd, NCVideoOptionWnd |
| `1004d6c0` | 1572 | InitializePawnViewer | modo visualizador (+0x3734..3C) | NConsoleWnd |
| `1004dd20` | 118 | (TArray<NTimer>::Remove) | Tick: remove os timers mortos | NConsoleWnd |
| `1004ef90` | 129 | NSetTimer | NCObject::SetTimer | NCObject |
| `10047440` | 78 | NKillTimer | NCObject::KillTimer | NCObject |
| `10054e20` | 720 | (cursor por quadro) | cursor de ataque / pegar item e nome do item no chão; NH 45/46 (guerra?) | NConsoleWnd |
| `10054c20` | 105 | (janela do alvo por tick) | mostra / esconde NCTargetStatusWnd | NConsoleWnd |
| `10055940` | 851 | DispatchWndMsgX | mensagens do Windows para o NCVirtualWndMain | NConsoleWnd x2 |
| `1004f050` | 1355 | HookShortCutKey | teclas de atalho (+0x3730; slots 267/268) | NConsoleWnd |
| `10045fb0` | 23 | SetCursor | WndMain->SetCursorType se o cursor não estiver travado | NConsoleWnd x3, NCObject, console |
| `10045fd0` | 11 | LockCursor | global 0x1012d7bc = 1 | NCObject, NConsoleWnd, NCMinimapWnd |
| `10045fe0` | 11 | UnlockCursor | global 0x1012d7bc = 0 | NConsoleWnd x2, NCObject, NCMinimapWnd |
| `10046070` | 80 | ShowMessageWnd | janela de mensagem +0x36D8 | NConsoleWnd x9, NCPawnSetupWnd x6, NCLobbyWnd x2, NCAuthWnd, +1 |
| `100460c0` | 82 | ToggleOptionWnd | +0x36C4 | NCSystemMenuWnd |
| `100430c0` | 183 | ShowWarningWnd | depois dos avisos (+0x36C8) | NCNoticeWnd x2 |
| `10045f70` | 57 | ReturnToCharacterSelect | criação cancelada (+0x36CC/D0) | NCPawnCreateWnd |
| `1004a0e0` | 166 | ShowDialog | caixa de diálogo (Desktop->MainDialogBox / Second) | NConsoleWnd x12, NCPledgeWnd x8, NCPrivateBuyItemWnd x4, NCPrivateBuyWnd x4, +23 |
| `10054b60` | 189 | ShowSystemMessage | linha de systemmsg no chat (id < 3000) | NConsoleWnd x18, NCGMWnd x6, NCPledgeWnd x4, NCMatchWnd x3, +6 |
| `100463f0` | 135 | ReloadGameTexts | depois de recarregar os textos (+0x36DC) | NCGameOptionWnd |

## Grupo 5 — o resto

**Precisa do layout:** quase nada (`Engine` para os atores; +0x372C nas ações).

| VA | B | nome | o que faz | chamadores |
|---|---|---|---|---|
| `10042810` | 344 | GetIMEType | tipo do IME do teclado | NCMultiEdit x8, NCEditBox x5, NCChineseIME x4, sub_10008180, +11 |
| `100427d0` | 54 | 0x100427d0 | sílaba hangul com consoante final (argumento em AX: não fazer gancho) | - |
| `10046120` | 404 | FormatSystemMessage | $s1 / $s2 (partículas coreanas: 0x100427d0) | console, NCPawnSelectWnd, NCLobbyWnd |
| `10046de0` | 279 | ConvertNewLines | \\n escapado para quebra de linha | NCActionItemWnd x2, NCEquipItemWnd x2, NCInvenItemWnd x2, NCSkillItemWnd x2, +17 |
| `100462c0` | 83 | HasInvalidChars | vazio, ou caractere que a fonte não tem | NCDialogBox, NCHtmlViewer, NCBBSHtmlViewer, NCLobbyWnd, +1 |
| `10046320` | 49 | HasNoAmpersand | 0 se o texto tem '&' | NCHtmlViewer |
| `1004a190` | 574 | HasBadWord | filtro de palavrões (GL2GameData) | NCDialogBox, NCBBSHtmlViewer, NConsoleWnd, console |
| `10046360` | 138 | 0x10046360 | ajudante do HasBadWord | console |
| `10056fa0` | 195 | IsValidName | nome de personagem / pet | NCDialogBox, NCPawnCreateWnd |
| `10057070` | 1116 | MakeSystemMessage* | texto de uma systemmsg com parâmetros (tipos 0..5); stub declarado | sub_100575c0 |
| `100575c0` | 261 | FormatDateTime ✓ | "yyyy/mm/dd hh:mm": systemmsg 713 com appConvertIntToTime | - |
| `10047380` | 74 | (param. de texto) | struct {TCHAR* Text; INT Len} a partir de texto | NConsoleWnd, console |
| `100473d0` | 105 | (param. numérico) | a partir de um número ("%d") | NConsoleWnd, console |
| `10045e60` | 9 | (dtor) | FString em +4 (desenrolar do slot 145) | sub_100cda40, sub_100cda80 |
| `10055660` | 144 | (TArray dtor) | +0x36F0 (destrutor e EH) | NConsoleWnd, sub_100cd9a9, sub_100cd9d9 |
| `100556f0` | 144 | (TArray dtor) | +0x36F4 | NConsoleWnd, sub_100cd9b7, sub_100cd9e7 |
| `10055780` | 50 | (helper) | genérico, 11 chamadores (EH, IsValidName) | console, sub_100cda60, sub_100ce310, sub_100ce330, +7 |
| `10046f40` | 12 | operator delete | global (GMalloc->Free), ICF com o slot 258 | NString x2 |
| `10038540` | 53 | GetMyLocation ✓ | Location do pawn do jogador (fora da faixa) | NCChatWnd |
| `1004ce70` | 40 | GetMyUser ✓ | FindUserByID(pawn do jogador->CreatureID) | NCEquipItemWnd x4, console x3, NCPledgeWnd x2, NConsoleWnd x2, +19 |
| `1004cea0` | 119 | GetSelectedActor ✓ | (guard) User do alvo (pawn +0x1198 -> +0x634) | console x4, NCPledgeWnd x2, NCShortCutWnd x2, NCDialogBox, +4 |
| `1004cf50` | 51 | GetSelectedStaticActor ✓ | porta ou outro alvo estático | console, NCTargetStatusWnd |
| `1004cf90` | 57 | HasSelectedTarget* ✓ | UBOOL do anterior (HookShortCutKey) | NConsoleWnd |
| `100465e0` | 57 | 0x100465e0 | AL2Pickup sob o mouse (Engine +0x208) | console |
| `10047230` | 46 | 0x10047230 | IsA(APawn) | NConsoleWnd |
| `1004a0b0` | 46 | ExecCommand | Exec no viewport | NCChatWnd |
| `100561f0` | 455 | 0x100561f0 | usa o atalho N (skill, item, ação, macro) | NConsoleWnd |
| `100551c0` | 1085 | 0x100551c0 | executa uma ação da janela de ações (+0x372C) | console |
| `100563c0` | 424 | RegisterPledgeCrest | caminho do bmp | NCDialogBox |
| `10056570` | 427 | RegisterAllyCrest |  | NCDialogBox |

## Para as próximas rodadas

- **Não fazer gancho**: `0x100427d0` (recebe o caractere em `AX`, convenção própria do LTCG),
  `0x10045870` (`ret`) e `0x10001700` (`ret 4`), que são de muitas classes (ICF), e o
  `scalar deleting destructor`.
- **Registradores (regcheck)**: nenhum problema com os 156 desta rodada (inclusive as folhas
  `IsPetCollar`, `IsShotItem`, `GetGradeText`, `IsSiegeNpc` e os passos do encantamento). Faltam
  `LockCursor`/`UnlockCursor` (não mexem em registrador nenhum) e `HasNoAmpersand` (só eax e
  ecx), que têm chamadores ainda no original: conferir com o `regcheck` ao fazer.
- **Campos da engine ainda sem declaração** (usados por `nw::Field` no `NConsoleWndNet.cpp`, como
  no `NCMinimapWnd`): o pawn do controlador do jogador (+0x5DC), o controlador de um pawn
  (+0x1198), o alvo de um controlador (+0x634) e o ator sob o mouse (`UEngine` +0x208).
- **Rodada da vtable**: declarar os 285 slots (a lista abaixo dá nome a quase todos pelos
  `UGameEngine::On*`); então os slots do grupo 1e e do grupo 3 viram virtuais de verdade e os
  `nw::VirtualCall` do header somem.
- **Por assunto**: os grupos 3 e 4 andam junto com as janelas que usam (troca, lojas, armazém,
  clã, party, pet, cerco, GM); as maiores são as de itens (`0x10053590` 1462 B, `0x10053e80`
  1430 B) e as de gestão (`Initialize` 3212 B, `0x1004b020` 4160 B, `ResetWindowPositions`
  3598 B, `0x1004a3d0` 3151 B).

## Apêndice — os 285 slots

`<-` = chamado por esse `UGameEngine::On*`; "(guard)" = nome da string de `guard`; "(header)" =
nome do `nw::VirtualCall` no `NConsoleWnd.h`; "Engine" = não sobrescrito (fica com o
`UL2ConsoleWnd`). Slots 0–23: `UObject` (3 = `scalar deleting destructor`, 10 = `Destroy`).

- 24 `10055cf0` **Tick** (UL2ConsoleWnd)
- 25 Engine **SetCurrentMakingRace** (UL2ConsoleWnd)
- 26 `10042990` ?
- 27 `10045870` (ret) <- OnAuthLoginSuccess, OnAcceptLogOn
- 28 `10047790` <- OnAuthLoginFail
- 29 `10047a70` <- OnStartNotice
- 30 `10047b90` <- OnCharacterCreateFail
- 31 `100434a0` ReceiveCharacterInfo (guard); <- OnReceiveCharacterInfo
- 32 `10043580` AddCharacterInfo (guard); <- AddCharacterInfo
- 33 `10047c80` AcceptGameStart (guard); <- OnAcceptGameStart
- 34 `10047d90` AcceptCharacterDelete (guard); <- OnAcceptCharacterDelete
- 35 `10047e40` AcceptNewCharacter (guard); <- OnAcceptNewCharacter
- 36 `10047f30` AcceptCharacterCreate (guard); <- OnAcceptCharacterCreate
- 37 `1004f820` <- OnAcceptLogOut
- 38 `1004f6b0` <- OnServerClose
- 39 `1004f830` <- OnReviveRequest
- 40 `10043280` ReceiveServerList (guard); <- OnStartServerList
- 41 `10043390` AddServerList (guard); <- OnAddServerList
- 42 `10043420` EndServerList (guard); <- OnEndServerList
- 43 `10044a10` <- OnMyTargetSelected
- 44 `10043b00` ReceiveItemList (guard); <- OnReceiveItemList
- 45 `10043bb0` ReceiveUpdateItemList (guard)
- 46 `1004ffb0` ReceiveTradeStart (guard); <- OnReceiveTradeStart
- 47 `100500c0` <- OnReceiveAddTradeInventoryItem
- 48 `10050380` <- OnReceiveAddTradeOwnItem
- 49 `10050640` <- OnReceiveAddTradeOtherItem
- 50 `10050900` <- OnReceiveTradeUpdate
- 51 `10043f80` ReceiveTradeDoneMsg (guard); <- OnReceiveTradeDoneMsg
- 52 `100438e0` AddRecommandedData (guard); <- AddRecommandedData
- 53 `10043970` EndRecommandedData (guard); <- EndRecommandedData
- 54 `1004f8c0` AddInventoryItem (guard); <- AddInventoryItem
- 55 `1004fbb0` <- OnReceiveUpdateItemList
- 56 `10044120` <- OnStartBuyList
- 57 `10050bc0` <- OnAddBuyList
- 58 `100441a0` <- OnStartSellList
- 59 `10050e70` <- OnAddSellList
- 60 `10044270` <- OnStartDepositList
- 61 `10051120` <- OnAddDepositList
- 62 `10044310` <- OnStartWithdrawList
- 63 `100513d0` <- OnAddWithdrawList
- 64 `100561c0` <- OnReceiveWareHouseDonePacket
- 65 `10045e70` <- OnUpdatePlayerInfo
- 66 `100452e0` <- OnNpcHtmlMessage
- 67 `10045340` <- OnTutorialShowHtml
- 68 `10049990` <- OnTutorialShowQuestionMark
- 69 `100454a0` <- OnTutorialEnableClientEvent
- 70 `100454b0` <- OnTutorialCloseHtml
- 71 `10045470` SendTutorialEvent (header); <- CheckTutorialClientEvent
- 72 `10045b20` <- OnShowRadar
- 73 `10045b70` <- OnDeleteRadar
- 74 `10001700` (ret 4, ICF) <- OnReceivePledgeExtendedInfo
- 75 `10048d70` ReceivePledgeInfo (guard); <- OnReceivePledgeInfo
- 76 `10048fe0` ReceivePledgeInfoUpdate (guard); <- OnReceivePledgeInfoUpdate
- 77 `10044b10` ReceivePledgeMemberList (guard); <- OnReceivePledgeMemberList
- 78 `100491f0` AddPledgeMember (guard); <- OnAddPledgeMember
- 79 `100492f0` UpdatePledgeMember (guard); <- OnUpdatePledgeMember
- 80 `10044ba0` DeletePledgeMember (guard); <- OnDeletePledgeMember
- 81 `10044c30` DeleteAllPledgeMember (guard); <- OnDeleteAllPledgeMember
- 82 `10056750` <- OnReceiveCreatePledge
- 83 `10051e10` <- OnReceiveAskJoinPledge
- 84 `10056790` <- OnReceiveOustPledgeMember
- 85 `100567d0` <- OnReceiveDismissPledge
- 86 `10056720` <- OnReceiveJoinPledge
- 87 `10056810` <- OnReceiveWidthdrawalPledge
- 88 `10056850` <- OnReceiveOustPledgeMessage
- 89 `10056870` <- OnReceiveDismissPledgeMessage
- 90 `10051f10` <- OnReceiveStartPledgeWar
- 91 `100568b0` <- OnReceiveReplyStartPledgeWar
- 92 `10052010` <- OnReceiveStopPledgeWar
- 93 `100568f0` <- OnReceiveReplyStopPledgeWar
- 94 `10056930` <- OnReceiveSurrenderPledgeWar
- 95 `10056960` <- OnReceiveSurrenderPersonally
- 96 `10052790` <- OnReceiveAskJoinParty
- 97 `10056a10` <- OnReceiveJoinParty
- 98 `10045030` <- OnReceiveWithdrawalParty
- 99 `10056a40` <- OnReceiveOustPartyMember
- 100 `10056a70` <- OnReceiveOustPartyMessage
- 101 `10045070` <- OnReceiveDismissParty
- 102 `10056a90` <- OnRecevieDismissPartyMessage
- 103 `10052110` <- OnReceiveAskJoinAlly
- 104 `10044dd0` <- OnReceiveDismissAlly
- 105 `10056990` <- OnReceiveJoinAlly
- 106 `10044d90` <- OnReceiveOustAlly
- 107 `10044d50` <- OnReceiveWithdrawAlly
- 108 `100569d0` <- OnReceiveFriendAddRequestResult
- 109 `10052710` <- OnReceiveFriendAddRequest
- 110 `10001700` (ret 4, ICF) <- OnReceiveFriendAdd
- 111 `10001700` (ret 4, ICF) <- OnReceiveFriendRemove
- 112 `10001700` (ret 4, ICF) <- OnReceiveFriendList
- 113 `10001700` (ret 4, ICF) <- OnReceiveFriendStatus
- 114 `10001700` (ret 4, ICF) <- OnReceiveBlockList
- 115 `100448a0` <- OnReceiveShortCutList
- 116 `100485b0` <- AddShortCutItem
- 117 `100448c0` <- OnUpdateShortCutItem
- 118 `100448e0` <- OnUpdatePetShortCutItem
- 119 `10044900` <- OnReceiveShortCutDelete
- 120 `10044770` <- OnReceiveMagicSkillListPacket
- 121 `10044920` <- OnReceiveAbnormalStatus
- 122 `10048860` <- AddAbnormalStatus
- 123 `100444a0` ReceiveQuestList (guard); <- OnReceiveQuestList
- 124 `10044530` AddQuestID (guard); <- AddQuestID
- 125 `10051680` AddQuestItem (guard); <- AddQuestItem
- 126 `10054a30` <- OnSay2
- 127 `10045e00` ?
- 128 `10048050` <- OnReceiveVersionCheck
- 129 `10043d20` <- OnUpdateEquipItem
- 130 `100450b0` <- OnPartyAddMemberPacket
- 131 `10045150` <- OnPartyUpdateMemberPacket
- 132 `100451d0` <- OnPartyDeleteMemberPacket
- 133 `10045210` <- OnPartyDeleteAllPacket
- 134 `10045240` <- ReceivePartyMatchList
- 135 `10045260` <- AddPartyMatchMember
- 136 `10045280` <- OnReceivePartyMatchDetail
- 137 `10045870` (ret) <- OnShowMagicWnd
- 138 `10044780` <- OnShowSkillWnd
- 139 `10048470` <- OnSkillListPacket
- 140 `10001700` (ret 4, ICF) <- OnMagicListPacket
- 141 `1004feb0` <- OnReceiveTradeDemand
- 142 `10043640` <- OnRestartResponse
- 143 `100436a0` <- OnRestartResponse
- 144 `10056ab0` <- OnAddSystemMessageParam
- 145 `10056ba0` <- OnEndSystemMessageParam
- 146 `10045380` <- OnShowboardPacket
- 147 `10045ee0` BuilderCommand (header)
- 148 `10043c30` <- OnSelectItemToEnchant
- 149 `10043cd0` <- OnReceiveEnchantResult
- 150 `10051da0` <- OnReceiveAction
- 151 `100446d0` <- OnShowTrainListWnd
- 152 `10051820` <- OnShowTrainInfoWnd
- 153 `100482b0` <- AddAcquireSkillInfo
- 154 `10051a90` <- AddAcquireSkillExtendedInfo
- 155 `100480f0` ?
- 156 `100438d0` <- OnTeleportToLocation
- 157 `10044730` <- OnHideTrainWnd
- 158 `10045cf0` <- OnSetVIPShopID
- 159 `10054760` <- OnAddVIPItemList
- 160 `10054970` <- OnAddVIPNeedItemList
- 161 `10045d10` <- OnVIPItemListEnd
- 162 `10045d40` ?
- 163 `10045d50` <- EnablePartyChat
- 164 `10045d70` <- EnablePledgeChat
- 165 `10045d90` <- EnableAllyChat
- 166 `10045550` <- OnShowPetStatusWnd
- 167 `100455d0` <- OnShowPetPartyWnd
- 168 `100499f0` ?
- 169 `10049a90` <- OnHidePetWnd
- 170 `10045640` <- OnShowPetNameBtn
- 171 `10045660` <- OnReceivePetStatusInfo
- 172 `10045680` <- OnReceivePetItemList
- 173 `100528f0` AddPetInventoryItem (guard); <- AddPetInventoryItem
- 174 `10052bb0` <- UpdatePetInventoryItem
- 175 `1004d330` <- OnSkillCoolTimePacket
- 176 `10047060` ?
- 177 `10044e10` RequestStartAllianceWar (header)
- 178 `10044e20` RequestReplyStartAllianceWar (header)
- 179 `10044e30` RequestStopAllianceWar (header)
- 180 `10044e40` RequestReplyStopAllianceWar (header)
- 181 `10044e50` RequestSurrenderAllianceWar (header)
- 182 `10052210` <- OnStartAllianceWar
- 183 `10052310` <- OnStopAllianceWar
- 184 `10044f20` ?
- 185 `10044f30` RequestSiegeAttackerList (header)
- 186 `10044f40` RequestSiegeDefenderList (header)
- 187 `10044f50` RequestJoinSiege (header)
- 188 `10044f60` RequestConfirmSiegeWaitingList (header)
- 189 `10044f70` RequestSetSiegeTime (header)
- 190 `100493f0` <- OnReceiveSiegeInfo
- 191 `10049500` <- OnReceiveSiegeAttackerList
- 192 `100495f0` <- OnReceiveSiegeDefenderList
- 193 `100496f0` <- OnAddSelectableTime
- 194 `10049710` <- OnEndSiegeAttackerlist
- 195 `10049730` <- OnEndSiegeDefenderlist
- 196 `10044f80` <- ClearAttackerList
- 197 `10044fa0` <- ClearDefenderList
- 198 `10044e60` RequestPackageSend (header)
- 199 `10044e70` RequestPackageSendableItemList (header)
- 200 `10044e80` <- AddPackageToList
- 201 `10052410` <- AddSendableItemList
- 202 `10045790` <- OnGMViewCharacterInfo
- 203 `10049b30` GMViewReceivePledgeInfo (guard); <- OnGMViewReceivePledgeInfo
- 204 `100457c0` ReceivePledgeMemberList (guard); <- OnGMViewReceivePledgeMemberList
- 205 `10049d60` AddPledgeMember (guard); <- OnGMViewAddPledgeMember
- 206 `10045850` <- OnGMViewShowSkillWnd
- 207 `10049e50` <- OnGMViewSkillListPacket
- 208 `10045870` (ret) <- OnGMViewShowMagicWnd
- 209 `10049f80` <- OnGMViewMagicListPacket
- 210 `10045880` ReceiveQuestList (guard); <- OnGMViewReceiveQuestList
- 211 `10045900` AddQuestID (guard); <- GMViewAddQuestID
- 212 `10052e90` AddQuestItem (guard); <- GMViewAddQuestItem
- 213 `100459d0` ReceiveItemList (guard); <- OnGMViewReceiveItemList
- 214 `10053010` AddInventoryItem (guard); <- GMViewAddInventoryItem
- 215 `10045a80` <- OnGMViewStartWithdrawList
- 216 `10053300` <- OnGMViewAddWithdrawList
- 217 `10045ae0` ShowMapItem (header); <- OnShowMinimap
- 218 `10045c20` <- OnStartPrivateStoreManageList
- 219 `10045c20` <- OnStartPrivateStoreList
- 220 `10053590` <- OnPrivateStoreManageList
- 221 `10053b50` <- OnPrivateStoreList
- 222 `10045c50` <- OnPrivateStoreMsg, OnPrivateStoreBuyMsg
- 223 `10045cc0` <- OnStartPrivateStoreBuyManageList
- 224 `10045cc0` <- OnStartPrivateStoreBuyList
- 225 `10053e80` <- OnPrivateStoreBuyManageList
- 226 `10054420` <- OnPrivateStoreBuyList
- 227 `10047490` <- OnBlockedAccountWithMsg
- 228 `100475c0` <- OnBlockedAccount
- 229 `100499c0` <- OnSetSummonRemainTime
- 230 `10046050` IsCtrlDown (header)
- 231 `10045db0` ?
- 232 Engine **L2KeyRemapping** (UL2ConsoleWnd)
- 233 Engine **ExecLobbyEvent** (UL2ConsoleWnd)
- 234 Engine **ShowOnlyOneDefaultCharacter** (UL2ConsoleWnd)
- 235 Engine **ShowAllDefaultCharacter** (UL2ConsoleWnd)
- 236 `1004eee0` Init (guard)
- 237 `10055e50` ConsolePostRender (guard)
- 238 `10057500` DispatchWndMsg (guard)
- 239 `100429b0` NpcSay (guard); <- OnNpcSay
- 240 `10055f20` MasterConsoleEventProcess (guard)
- 241 `10047190` ?
- 242 Engine **MoveCameraByState** (UL2ConsoleWnd)
- 243 Engine **MoveCameraByCharacter** (UL2ConsoleWnd)
- 244 Engine **LoadDefaultCharacterInfo** (UL2ConsoleWnd)
- 245 Engine **SpawnDefaultCharacter** (UL2ConsoleWnd)
- 246 Engine **DestroyDefaultCharacter** (UL2ConsoleWnd); <- OnReceiveCharacterSelectedPacket
- 247 Engine **InsertSelectCharacterInfo** (UL2ConsoleWnd)
- 248 Engine **ResetSelectCharacterInfo** (UL2ConsoleWnd); <- OnReceiveCharacterSelectedPacket
- 249 Engine **SelectedCharacterNum** (UL2ConsoleWnd)
- 250 Engine **DefaultCharacterTurn** (UL2ConsoleWnd)
- 251 Engine **DefaultCharacterStop** (UL2ConsoleWnd)
- 252 Engine **ResetDefaultCharacterYaw** (UL2ConsoleWnd)
- 253 `100550f0` InitForPawnViewer (guard)
- 254 `10046f50` PawnViewerFunction (guard)
- 255 Engine **PlayIntroMusic** (UL2ConsoleWnd)
- 256 Engine **StopIntroMusic** (UL2ConsoleWnd)
- 257 `1004d190` ?
- 258 `10046f40` ?
- 259 Engine **RestoreLobbyMusic** (UL2ConsoleWnd)
- 260 `10047170` <- SetServerPing
- 261 `100447a0` ?
- 262 `100447e0` ?
- 263 `10057590` ?
- 264 `100447f0` ?
- 265 `1004d4c0` ?
- 266 `1004d680` ?
- 267 `10045ff0` ?
- 268 `10046020` ?
- 269 `10047b50` ?
- 270 `10047b70` ?
- 271 `10001700` (ret 4, ICF) ?
- 272 `10043180` RequestServerList (guard)
- 273 `1004f820` QuitGame (header)
- 274 `10047fc0` ReceiveServerClose (guard)
- 275 `10043820` AnswerReviveRequest (header)
- 276 `10043c50` RequestEnchantItem (guard)
- 277 `10043d40` RequestDestroyItem (guard)
- 278 `10043dd0` RequestCrystallizeItem (header)
- 279 `10043f70` AnswerTradeRequest (header)
- 280 `10044380` RequestQuestList (guard)
- 281 `10044420` RequestDestroyQuest (guard)
- 282 `10044710` RequestAcquireSkillInfo (header)
- 283 `10044720` RequestAcquireSkill (header)
- 284 `10049750` RunHtmlCommand (header)

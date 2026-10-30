# Rede: o protocolo cliente -> servidor do Lineage II C1 (Network.dll), enquadramento, cifra e login

Feito em 04/10/2026, **só análise de leitura**: nada foi compilado, instalado nem rodado, o registro e o jogo não foram tocados, o Ghidra não foi usado, nenhum outro arquivo do
repositório mudou (este é o único arquivo novo). Ferramentas: capstone e pefile sobre `orig/Network.dll` (md5 `f6d590082999cf527b1505841a070a13`, TimeDateStamp `0x40DBD380`,
**idêntico byte a byte ao `Network.dll` do cliente de teste** `System - Teste Acessorio`), `orig/Engine.dll` (md5 `e705066c...`, o stock) e `orig/NWindow.dll`; o código-fonte
reconstruído `src/nwindow/*` (que o projeto verifica contra o stock); `analysis/ghidra/functions.tsv`, `include/ue2/gen/UL2NetHandler.vt.inc`,
`FL2NetNotify.vt.inc`. Scripts descartáveis ficaram no scratchpad (um **executor simbólico linear de x86** que resolve os argumentos de cada chamada ao `Send`, um varredor de
chamadas ao `UL2NetHandler` em Engine.dll/NWindow.dll e um extrator do servidor de referência), não no repositório.

Convenção: **VERIFICADO** = lido no assembly (endereço citado), nas fontes do projeto ou nos arquivos do cliente. **INFERIDO** = raciocínio sobre o que foi lido.
**NÃO VERIFICADO** = só rodar o jogo, um servidor ou a harness diria. **REF** = o servidor `L2J_Mobius_C1_HarbingersOfWar`
(`../L2J_Mobius_C1_HarbingersOfWar`, lido só para comparar nomes e campos): é **um servidor C1 de referência, NÃO o servidor do dono**; o código do
servidor Lucera-C1 do dono **não está nesta máquina** e eu não o li. Endereços do `Network.dll`: base `0x10000000`.
Quando digo "Engine.dll" o endereço é o preferido `0x10300000`.

## 0. Resumo: o que o dono aprende e pode fazer com isto

1. **Quem fala o quê (VERIFICADO).** O `Network.dll` não é chamado pelo NWindow nem pelo Engine por importação: o **único** módulo que o importa é o `WinDrv.dll`, e só a função
   `DispatchSocketWindowMsg` (a janela do viewport repassa toda mensagem do Windows a ela). Todo o resto é por **objeto**: o `UEngine` guarda o `UL2NetHandler` em `+0x5C`
   (`UEngine::NetHandler`), e as 203 virtuais dele (`UL2NetHandler.vt.inc`) são os "pedidos ao servidor". Dentro do `Network.dll` há **quatro classes**: `UNetworkHandler`
   (global `0x10055308`), `CClientSocket` (jogo; vtable `0x10033D58`), `CAuthSocket` (login; vtable `0x10033DD0`, deriva de `CClientSocket`) e `CPingSocket` (vtable `0x100310F0`).
2. **Duas conexões TCP em série, mais pings (VERIFICADO).** Login: `ServerAddr` do ini, porta **2106 fixa no código** (`push 0x83A` em `0x10003411`), cifra **Blowfish ECB com chave estática**.
   Jogo: IP e porta vêm da lista de servidores, cifra **XOR rolante de 8 bytes** com chave que o servidor manda no primeiro pacote. Mais, ao receber a lista, uma conexão curta a cada
   servidor para medir o ping, que manda um pacote `ProtocolVersion(-2)`.
3. **Enquadramento (seção 2.1).** `[u16 LE: tamanho total, incluindo estes 2 bytes][u8 opcode][campos]`, no máximo `0x1FFE` bytes de corpo; o corpo é montado por letras
   (`c h d f S s b`); só o corpo é cifrado.
4. **Tabela dos pacotes (seção 1): 154 opcodes no socket de jogo**, em 170 sítios de envio, mais 3 opcodes de login (`0x00`, `0x02`, `0x05`) e o ping. Para cada um: função do
   `UL2NetHandler` (slot e offset de vtable), formato do fio, de onde vem cada valor, **quem dispara** (janela do NWindow, comando de chat, verbo do `UInput::Exec`, timer do
   Engine) e se o REF lê o mesmo formato. **28 opcodes que o cliente manda não existem no REF** (seção 7) e 9 têm formato diferente [corrigido pela revisão: a nota dizia 27 e 11; ver seção 7].
5. **Detalhes que mordem quem escreve um servidor (seções 2 e 3).**
   - O **bloco da chave** do jogo: o cliente lê do `KeyPacket` um `d` de chave e um `d` que só serve de **flag** (`!= 0` liga a cifra); a metade alta da chave **não vem do
     servidor**: o cliente a fixa em `0x87546CA1` (ou `0xFFFFFFFF` se o primeiro `d` tiver o bit de sinal ligado). Servidor e cliente só concordam com `d1 >= 0` e bytes 4..7 = `A1 6C 54 87`.
   - `d2 = 0` no `KeyPacket` desliga a cifra do jogo inteira (tráfego em claro): útil para depurar.
   - Conta e senha do login vão em **ANSI** (`CP_ACP`), 14 e 16 bytes, preenchidos com zero; o `AuthLogin` do jogo leva a conta em UTF-16.
   - O 1o pacote do servidor de login (`Init`) só tem **2 `d` lidos** (`sessionId` e uma revisão que o cliente **ignora**); o resto é ignorado.
6. **Defeitos do cliente stock que o servidor vê no fio (seção 4, VERIFICADO no assembly):** `FinishRotating` manda um `d` de lixo; `RequestPetitionCancel` e
   `RequestTutorialPassCmdToServer` mandam uma `S` **cujo ponteiro é lixo da pilha** (em `0x80`, o endereço de retorno: bytes de código do NWindow até o primeiro `00 00`; em `0x7C`, o `esi` salvo do
   chamador); o `bypass` de uma página de **tutorial** nunca chega ao servidor; `RequestAttack` (slot 158) e `MoveBackwardToLocation` mandam menos campos que o REF espera.
7. **O que quebra o cliente (seção 5):** tamanho de pacote `< 2` no cabeçalho (INFERIDO: cópia com tamanho negativo), tamanho `> ~17,6 KB` (INFERIDO: estouro do buffer do socket), dados
   urgentes TCP (`MessageBoxW` modal "FD_OOB"), host que não resolve (`MessageBoxW` modal), pacote mais curto que o formato (o leitor **não conhece o tamanho**: lê lixo da heap).
   Um `FIN` limpo **não** avisa o cliente; só `RST`/erro (`FD_CLOSE` com código) chama `OnServerClose`.
8. **Para acrescentar um pacote cliente -> servidor (seção 6):** a via mais barata é um `bypass` (opcode `0x21`) com texto próprio, como o nosso `ManufactureCmd`; para um opcode novo,
   chame o slot `0x60` do `CClientSocket` com formato por letras. Opcodes livres nos dois lados (jogo): `0x13 0x18 0x19 0x3A 0x3B 0x3D 0x40 0x4C 0x54 0x56 0x5A 0x65 0x8D` e `0xAA..0xFF`.

## 1. A tabela dos pacotes cliente -> servidor

Legenda do fio (as letras do empacotador `0x100026E0`, VERIFICADAS na tabela de saltos `0x100028AC`): `c` 1 byte (o byte baixo do inteiro empurrado), `h` 2 bytes, `d` 4 bytes,
`f` 8 bytes (double), `S` string UTF-16LE terminada em zero (ponteiro nulo = 2 bytes zero), `s` string ANSI terminada em zero, `b`/`x` bloco cru (um inteiro de tamanho e um ponteiro),
qualquer outra letra não escreve nada. O primeiro `c` é sempre o **opcode**. `A1, A2...` = o argumento C++ da função, em dwords da pilha (`FVector` ocupa 3, `ItemInfo` 12, `CharacterInfo`
22); `ftol` = float convertido por `_ftol` (`0x100229B4`: **trunca**). `P1, P2...` = o n-ésimo valor que o chamador **empurrou** no `L2ParamStack` (o `Top()` devolve na ordem do
`PushBack`: VERIFICADO em `Say2` e `RequestServerLogin`). Os nomes dos campos vêm do REF quando o formato bate; a **origem** (A/P) é VERIFICADA no assembly. "Gatilho": **NW** = o código do
NWindow (`src/nwindow`, lido), **Eng** = Engine.dll (varredura heurística de `call [vt+off]` em registradores que vêm de `[x+0x5C]`; pode ter falsos positivos, e um ponteiro
guardado na pilha escaparia: marcada "heur."); "nenhum chamador achado" = nenhuma das duas varreduras viu (NÃO é prova de código morto). Coluna **REF**: `=` o REF lê o mesmo formato,
`≠ REF "..."` formato do REF, `ausente` o REF não trata o opcode.

### 1.1 Socket de login (`CAuthSocket`, Blowfish; estado `[handler+0x44] = 1`)

| Op | Função (slot) | Fio | Campos no fio (origem) | Gatilho | REF |
|---|---|---|---|---|---|
| 0x00 | `sub_100075F0` (manipulador do op 0 do login, `0x100075F0`; dois sítios `0x10007706` / `0x10007744`) | `cxxdh` | `x`(14): conta ANSI (`WideCharToMultiByte(CP_ACP,0,0x100567CC,14,...)`); `x`(16): senha ANSI (`0x100567EA`, 16); `d` = 8 (constante); `h` = `atoi` do conteúdo de `package.dat` (0 se o arquivo não existe: o sítio `0x10007744`) | resposta imediata ao 1o pacote do servidor de login | reflete `RequestAuthLogin` do REF (conta em `[1..14]`, senha em `[15..28]`) |
| 0x05 | `RequestServerList` (69 `+0x114`) | `cddc` | `d` loginOk1 (global `0x100567AC`); `d` loginOk2 (`0x100567B0`); `c` = 3 (constante). Só sai se o socket de login está aberto (`[auth+0x2C] != 0`) | NW: NCNoticeWnd (3x), NCWarningWnd (2x) | `=` (`key1,key2,byte`) |
| 0x02 | `RequestServerLogin` (70 `+0x118`) | `cddc` | `d` loginOk1; `d` loginOk2; `c` serverId = P1 (só o byte baixo). P2 = IP (`in_addr` como dword) e P3 = porta ficam em `0x100567BC` / `0x100567C0` para a conexão ao jogo. Só sai se o socket de login está aberto | NW: NCServerSelectWnd::OnOK (uma vez: `bRequested`) | `=` |
| ping | `0x10010520` + envio `0x10002900` (socket próprio, `CPingSocket`) | `cd` | `c` = 0; `d` = **-2** (`0xFFFFFFFE`), enquadrado (`07 00 00 FE FF FF FF`), sem cifra | thread do `EndServerPingList` (`0x10011B30`): uma conexão TCP **bloqueante** a cada servidor da lista; o ping é o tempo do `connect` (9999 em falha) | REF: `ProtocolVersion` com `-2` = "ping do cliente novo", desconecta |

### 1.2 Socket de jogo (`CClientSocket`, XOR rolante depois do `KeyPacket`; estado `[handler+0x44] = 2`)

Nome da função = a exportação do `Network.dll` (`UNetworkHandler::...`); `(slot +off)` = índice e offset na vtable do `UL2NetHandler`. Os pacotes `0x00` e `0x08` não são métodos do handler: saem do
`OnConnect` do socket e do manipulador do `KeyPacket`. Há 162 linhas porque alguns opcodes têm mais de uma função (`0x01`, `0x03`, `0x04`, `0x12`, `0x39`, `0x59`) e algumas funções têm mais de um formato.

| Op | Função (slot +off) | Fio | Campos no fio (origem) | Gatilho | REF |
|---|---|---|---|---|---|
| 0x00 | `ProtocolVersion: CClientSocket::OnConnect` (—) | `cd` | d protocolo = 419 (de "$Revision: 419 $" por sscanf `%cRevision: %d %c`; global `0x10041134`) | CClientSocket::OnConnect (FD_CONNECT sem erro). Não cifrado | = |
| 0x01 | `MoveBackwardToLocation` (50 `+0x0C8`) | `cdddddd` | tx,ty,tz = ftol(A1..A3); ox,oy,oz = ftol(A4..A6). SEM o 7o int (modo) que o REF lê | Eng(heur.): UInput::Exec (bloco L2, `0x104EACAF`) | ≠ REF `ddddddd` |
| 0x01 | `MoveToLocation` (49 `+0x0C4`) | `cddddddd` | tx,ty,tz = ftol(A2..A4) alvo; ox,oy,oz = ftol(A5..A7) origem; d = (A9==0). Variante de veículo: ver 0x5C | Eng(heur.): ALineagePlayerController::Tick (3 sítios), ResetKeyobardMoving, ResetJoypadMoving | = |
| 0x02 | `Say` (55 `+0x0DC`) | `cS` | S text=A1 | nenhum chamador achado (opcode 2 no socket de JOGO) | ausente |
| 0x03 | `RequestEnterWorldPacket` (85 `+0x154`) | `c` | só o opcode | Eng: UGameEngine::Tick `0x104D559F`, quando o contador `[0x109F5A44]` chega a 4 com `[0x109F53DC]` ligado (ver 3.2) | = |
| 0x03 | `EnterWorld` (56 `+0x0E0`) | `cS` | S = L"narcasse" (literal no código, `0x10035954`) | nenhum chamador achado; cria um `User` com `[URL] CharName` e o registra (ver 4) | ≠ REF (nada) |
| 0x04 | `Action` (53 `+0x0D4`) | `cddddc` | d objectId=A1; d,d,d = ftol(A2..A4) posição do jogador; c = A5 & 0xFF (shift) | NW: wrappers TargetMyself, RequestTarget, RequestPickUp, RequestTargetNext, AssistMember, RequestAction, RequestAttack, RequestAssist; Eng: UInput::Exec (GETITEM e TARGETCHANGE; sítio achado `0x104E5B2C`), PLAYERPAWNMOVETO (`0x104EA32C`), ExecJoyPad | = |
| 0x04 | `RequestAttack` (158 `+0x278`) | `cdddd` | d objectId=A1; d,d,d = ftol(A2..A4). SEM o `c` final de Action | nenhum chamador achado (slot 158 `+0x278`) | ≠ REF `ddddc` |
| 0x08 | `AuthLogin: handler do op 0 (KeyPacket)` (—) | `cSdddd` | S account (UTF-16, `0x100567CC`); d playOk2 (`[0x100567C4]`); d playOk1 (`[0x100567C8]`); d loginOk1; d loginOk2 | handler do op 0 do jogo (VersionCheck/KeyPacket), `0x10007F40`, logo depois de `ReceiveVersionCheck` | = |
| 0x09 | `SendLogOutPacket` (147 `+0x24C`) | `c` | só o opcode | NW: NCSystemMenuWnd (OK do diálogo Exit), NConsoleWnd::ReturnToLogin ("Previous" da seleção), ReceiveVersionCheck (versão recusada), NCDialogBox | = |
| 0x0A | `Attack` (52 `+0x0D0`) | `cddddc` | d objectId=A1; d,d,d = ftol(A2..A4); c = A5 (0 = RequestAttackForce, 1 = RequestAttackStand) | NW: RequestAttack/Force/Stand (chat /attack, /attackforce, /attackstand, barra de atalhos, painel de ações); Eng: UInput::Exec (`0x104EA41C`) | = |
| 0x0B | `RequestCharacterCreate` (75 `+0x12C`) | `cSdddddddddddd` | S name (CharacterInfo+0x00, 20 TCHAR); d race(+0x28); d sex(+0x30); d classId(+0x2C); d INT; d STR; d CON; d MEN; d DEX; d WIT (Stats[0..5] = STR,DEX,CON,INT,WIT,MEN); d hairStyle(+0x4C); d hairColor(+0x50); d face(+0x54) | NW: NCPawnCreateWnd (OK da criação) | = |
| 0x0C | `RequestCharacterDelete` (72 `+0x120`) | `cd` | d slot=A1 (índice na lista) | NW: NCLobbyWnd | = |
| 0x0D | `RequestGameStart` (71 `+0x11C`) | `cd` | d slot=A1 | NW: NCLobbyWnd (Start) | = |
| 0x0E | `RequestNewCharacter` (74 `+0x128`) | `c` | só o opcode | NW: NCLobbyWnd (Create) | = |
| 0x0F | `RequestItemList` (77 `+0x134`) | `c` | só o opcode | NW: NCConsole (abre o inventário), NConsoleWndItems; Eng: UInput::Exec (`0x104EA519`) | = |
| 0x10 | `RequestEquipItem` (152 `+0x260`) | `cdd` | d P1; d P2 | nenhum chamador achado (o equipar sai por 0x14) | ausente |
| 0x11 | `RequestUnEquipItem` (153 `+0x264`) | `cd` | d bodyPart=P1 (máscara de slot: `Item->BodyPart`, ou `0x10`/`0x20` quando o item é o do slot 5) | NW: NCEquipItemWnd (3x) | = |
| 0x12 | `RequestDropItem` (87 `+0x15C`) | `cddddd` | d objectId (ItemInfo+4=A2); d count (ItemInfo+0xC=A4); d,d,d = ftol(FVector) posição de queda | NW: NConsoleWnd::RequestDropItem (arrastar item ao chão; NCItemWnd.h) | = |
| 0x12 | `RequestDropItemFromPet` (193 `+0x304`) | `cddddd` | idem 0x12 (mesmo opcode que o do jogador!) | nenhum chamador achado (slot 193) | = |
| 0x14 | `RequestUseItem` (86 `+0x158`) | `cd` | d objectId=A1 [corrigido pela revisão: a nota dizia "SEM o `ctrlPressed` que o REF lê"; em `UseItem.java` a linha `_ctrlPressed = readInt()` está **comentada**, o REF lê só `d`] | NW: NCInvenItemWnd (3x), NCShortCutWnd (2x) | `=` [corrigido pela revisão: era "≠ REF `dd`"] |
| 0x15 | `RequestTrade` (82 `+0x148`) | `cd` | d creatureId=A1 | NW: chat /trade, NCActionItemWnd, NCShortCutWnd | = |
| 0x16 | `RequestAddTradeItem` (83 `+0x14C`) | `cddd` | d tradeId = 1 (constante); d objectId (ItemInfo+4); d count (ItemInfo+0xC) | NW: NCTradeWnd (2x) | = |
| 0x17 | `RequestTradeDone` (84 `+0x150`) | `cd` | d A1 (1 confirma, 0 cancela) | NW: NCTradeWnd, NCConsole, NCFrameCtrl | = |
| 0x1A | `RequestTeleport` (90 `+0x168`) | `cdddd` / `cddddd` | d creatureId=A1; d modo (2 ou 3, constante); d x=A3; d y=A4; [d z=A5 só no modo 3]. Outro modo: nada sai | NW: NConsoleWnd::MoveToLocation (`0x1004D250`) <- NCConsole::OnMoveTo, mensagem `NWM_MOVETO` (0xF0000001); NENHUM remetente da mensagem achado no NWindow | ausente |
| 0x1B | `SocialAction` (57 `+0x0E4`) | `cd` | d actionId=A1 (2 Hello, 3 Victory, 4 Charge, 5 No, 6 Yes, 7 Bow, 8 Unaware, 9 Waiting, 10 Laugh nos comandos de chat) | NW: chat /social*, NCShortCutWnd (9), NCActionItemWnd | = |
| 0x1C | `ChangeMoveType` (58 `+0x0E8`) | `cd` | d A1 (0 anda, 1 corre) | NW: chat /walk, /run; Eng: UInput::Exec MOVEMODECHANGE (4 sítios `0x104E6148..625E`) | = |
| 0x1D | `ChangeWaitType` (59 `+0x0EC`) | `cd` | d A1 (0 senta, 1 levanta) | NW: chat /sit, /stand; Eng: UInput::Exec WAITMODECHANGE (`0x104E603C`, `0x104E605A`) | = |
| 0x1E | `RequestSellItem` (88 `+0x160`) | `cddb` | d listId=P1; d N=P2; depois N x (d objectId, d itemId, d count) = 3 Top() por item. `b` = bloco cru dessas triplas | NW: NCShopWnd (Sell) | = (lista) |
| 0x1F | `RequestBuyItem` (89 `+0x164`) | `cddb` | d listId=P1; d N=P2; depois N x (d itemId, d count) = 2 Top() por item | NW: NCShopWnd (Buy) | = (lista) |
| 0x20 | `RequestLinkHtml` (91 `+0x16C`) | `cS` | S link=P1 | NW: NCConsole (clique em link de página HTML do servidor) | = |
| 0x21 | `RequestBypassToServer` (93 `+0x174`) | `cS` | S command=P1 (tokens do `bypass` unidos por espaço; `-h` é retirado; `$nome` vira o valor do controle da página; buffer de 1024 TCHAR) | NW: NConsoleWnd::RunHtmlCommand (link `bypass [-h] ...` de uma página HTML do servidor, `0x10049750`); nosso ManufactureCmd (SendBypassToServer) | = |
| 0x22 | `RequestBBSWrite` (92 `+0x170`) | `cSSSSSS` | S url,arg1..arg5 = P1..P6 | NW: NCConsole (2x, o quadro de avisos) | = |
| 0x23 | `RequestCreatePledge` (103 `+0x19C`) | `cS` | S name=P1 | nenhum chamador achado (wrapper `RequestNet103`) | ausente |
| 0x24 | `RequestJoinPledge` (107 `+0x1AC`) | `cd` | d target=P1 (objectId) [corrigido pela revisão: a nota dizia "SEM o `pledgeType` que o REF lê"; em `RequestJoinPledge.java` o `_pledgeType = readInt()` está **comentado**, o REF lê só `d`] | NW: NCPledgeWnd | `=` [corrigido pela revisão: era "≠ REF `dd`"] |
| 0x25 | `RequestAnswerJoinPledge` (104 `+0x1A0`) | `cd` | d answer=P1 | nenhum chamador achado | = |
| 0x26 | `RequestWithDrawalPledge` (108 `+0x1B0`) | `c` | só o opcode | NW: NCDialogBox (confirmação) | = |
| 0x27 | `RequestOustPledgeMember` (105 `+0x1A4`) | `cS` | S name=P1 | NW: NCDialogBox | = |
| 0x28 | `RequestDismissPledge` (106 `+0x1A8`) | `c` | só o opcode | NW: NCDialogBox (wrapper `RequestNet106`) | ausente |
| 0x29 | `RequestJoinParty` (130 `+0x208`) | `cdd` | d objectId=P1 (`Pawn->CreatureID` do alvo selecionado); d P2 = `GConsole->PartyItemDistribution` (modo de divisão de itens) | NW: chat /partyinvite, NCActionItemWnd, NCShortCutWnd, NCMatchWnd | = |
| 0x2A | `RequestAnswerJoinParty` (131 `+0x20C`) | `cd` | d response=P1 | nenhum chamador achado | = |
| 0x2B | `RequestWithDrawalParty` (132 `+0x210`) | `c` | só o opcode | NW: chat /partyleave, NCActionItemWnd, NCShortCutWnd | = |
| 0x2C | `RequestOustPartyMember` (133 `+0x214`) | `cS` | S name=P1 | NW: chat /partydismiss, NCActionItemWnd, NCShortCutWnd | = |
| 0x2D | `RequestDismissParty` (134 `+0x218`) | `c` | só o opcode | nenhum chamador achado | ausente |
| 0x2E | `RequestMagicSkillList` (144 `+0x240`) | `c` | só o opcode | nenhum chamador achado (slot 144) | ausente |
| 0x2F | `RequestMagicSkillUse` (145 `+0x244`) | `cddc` | d skillId=P1; d ctrl=P2; c shift=P3 (atalho de skill: `IsCtrlDown()` e `KeyDown(IK_Shift)`; /useskill: force e stand) | NW: NCChatWnd (/useskill*), NCShortCutWnd, NCSkillItemWnd | = |
| 0x30 | `SendAppearingPacket` (60 `+0x0F0`) | `c` | só o opcode | Eng: UGameEngine::ScreenFadeDone `0x104CB354` (fim de um fade de tela com `arg1 != 0` e `arg2 == 1`) | = |
| 0x31 | `SendWareHouseDepositList` (138 `+0x228`) | `cdb` | d N=P1; N x (d objectId, d count) | NW: NCStoreWnd (modo depósito) | = (lista) |
| 0x32 | `SendWareHouseWithdrawList` (139 `+0x22C`) | `cdb` | d N=P1; N x (d objectId, d count) | NW: NCStoreWnd (modo retirada) | = (lista) |
| 0x33 | `RequestShortCutReg` (140 `+0x230`) | `cdddd` | d type=P1 (1 = item); d slot=P2; d id=P3 (objectId no caso do item); d P4 (1 no atalho de item) | NW: NCInvenItemWnd, NCEquipItemWnd, NCSkillItemWnd, NCActionItemWnd, NCPetInventoryWnd, ManufactureCmd | = |
| 0x34 | `RequestShortCutUse` (141 `+0x234`) | `cddc` | d slot=P1 ((Page*10)+Slot-11 no /useshortcut); d ctrl=P2; c shift=P3 | NW: wrapper `UseShortcut` só pelo chat `/useshortcut*`. As teclas de atalho (`NConsoleWnd::UseShortCut` `0x100561F0`) mandam o pedido do que está no atalho (0x2F, 0x14, 0x45...), NÃO o 0x34 | ausente |
| 0x35 | `RequestShortCutDel` (142 `+0x238`) | `cd` | d slot=P1 | NW: NCShortCutWnd, ManufactureCmd | = |
| 0x36 | `CanNotMoveAnymore` (61 `+0x0F4`) | `cdddd` | d,d,d = ftol(A2..A4) x,y,z; d yaw=A5. Na variante normal não sai se A6 != 0 | Eng(heur.): ALineagePlayerController::HitWallNotify, ResetKeyobardMoving, ResetJoypadMoving, UInput::Exec, APawn::stepUp | = |
| 0x37 | `RequestTargetCancel` (143 `+0x23C`) | `c` | só o opcode | NW: NCStatusWnd | = |
| 0x38 | `Say2` (146 `+0x248`) | `cSd` / `cSdS` | S text=P2; d type=P1 (0 normal, 1 shout, 2 tell, 3 party, 4 pledge, 5 sistema, 6, 7, 8 market, 9 ally); com type==2 a variante `cSdS` acrescenta S target=P3. Texto nulo = nada sai | NW: NConsoleWnd::SendChatMsgToServer <- NCChatWnd (linha digitada, prefixos), NCPetitionWnd | = (tipo 2 acrescenta S) |
| 0x39 | `SendCharacterInfo` (148 `+0x250`) | `ccS` | c sub=0; S P1 | nenhum chamador achado | ausente |
| 0x39 | `SendSummonCmd` (149 `+0x254`) | `ccS` | c sub=1; S P1 | nenhum chamador achado | ausente |
| 0x39 | `SendL2ParamSetting` (151 `+0x25C`) | `ccdd` | c sub=3; d P1; d P2 | nenhum chamador achado | ausente |
| 0x39 | `SendServerStatus` (150 `+0x258`) | `cc` | c sub=2 | nenhum chamador achado | ausente |
| 0x3C | `RequestPledgeMemberList` (102 `+0x198`) | `c` | só o opcode | NW: NCActionWnd | = |
| 0x3E | `RequestMagicItem` (155 `+0x26C`) | `c` | só o opcode | nenhum chamador achado (slot 155) | ausente |
| 0x3F | `RequestSkillItem` (154 `+0x268`) | `c` | só o opcode | NW: NCMagicSkillWnd (abre a janela de skills) | = |
| 0x41 | `MoveWithDelta` (62 `+0x0F8`) | `cddd` | d dx,dy,dz = ftol(A1..A3) | nenhum chamador achado | = |
| 0x42 | `GetOnVehicle` (63 `+0x0FC`) | `cdddd` | d vehicleId=A1; d,d,d = ftol(A2..A4) | Eng(heur.): APawn::SetBase `0x1048B3A4` | = |
| 0x43 | `GetOffVehicle` (64 `+0x100`) | `cdddd` | d vehicleId=A1; d,d,d = ftol(A2..A4) | Eng(heur.): APawn::SetBase `0x1048B2CC` | = |
| 0x44 | `AnswerTradeRequest` (156 `+0x270`) | `cd` | d response=P1 | NW: NCDialogBox (resposta do convite de troca, por slot do console `NConsoleWnd::vfn141`) | = |
| 0x45 | `RequestActionUse` (157 `+0x274`) | `cddc` | d actionId=P1; d ctrl=P2; c shift=P3 | NW: NCActionItemWnd, NCShortCutWnd, NCDialogBox; nosso ManufactureCmd (ids 32..52) | = |
| 0x46 | `RequestRestart` (159 `+0x27C`) | `c` | só o opcode | NW: NCSystemMenuWnd (OK do diálogo Restart); Eng: UInput::Exec L2Restart (`0x104E570E`) | = |
| 0x47 | `RequestSiegeInfo` (161 `+0x284`) | `c` | só o opcode | nenhum chamador achado | = |
| 0x48 | `ValidatePosition` (65 `+0x104`) | `cddddd` | d,d,d = ftol(A1..A3); d heading=A4; d A5 (id do veículo ou 0) | Eng: ALineagePlayerController::Tick, a cada 1,2 s (constante `0x10797598`) enquanto o pawn tem velocidade != 0 e `GIsL2NetLogin`; AController::StopMove / execPollMoveTo (ao parar) | = |
| 0x49 | `RequestSEKCustom` (162 `+0x288`) | `cdd` | d P1; d P2 | nenhum chamador achado (slot 162) | ausente |
| 0x4A | `StartRotating` (66 `+0x108`) | `cdd` | d degree=A1; d side=A2 | Eng: UInput::Exec LEFT/RIGHTTURNINGSTART (`0x104EB1A4`, `0x104EB3DE`) | = |
| 0x4B | `FinishRotating` (67 `+0x10C`) | `cdd` | d degree=A1; **d = lixo** (o `esi` salvo do chamador, ver 4) | Eng: UInput::Exec LEFT/RIGHTTURNINGFINISH (`0x104EB28E`) | = |
| 0x4D | `RequestStartPledgeWar` (109 `+0x1B4`) | `cS` | S pledgeName=P1 | nenhum chamador achado | = |
| 0x4E | `RequestReplyStartPledgeWar` (110 `+0x1B8`) | `cSd` | S name=P1; d answer=P2 | nenhum chamador achado | = |
| 0x4F | `RequestStopPledgeWar` (111 `+0x1BC`) | `cS` | S pledgeName=P1 | nenhum chamador achado | = |
| 0x50 | `RequestReplyStopPledgeWar` (112 `+0x1C0`) | `cSd` | S name=P1; d answer=P2 | nenhum chamador achado | = |
| 0x51 | `RequestSurrenderPledgeWar` (113 `+0x1C4`) | `cS` | S pledgeName=P1 | nenhum chamador achado | = |
| 0x52 | `RequestReplySurrenderPledgeWar` (114 `+0x1C8`) | `cSd` | S name=P1; d answer=P2 | nenhum chamador achado | = |
| 0x53 | `RequestSetPledgeCrest` (99 `+0x18C`) | `cdb` | d length=A1; b(len=A1, ptr=A2) | NW: o enviador do brasão (`0x100563C0`) | = |
| 0x55 | `RequestGiveNickName` (116 `+0x1D0`) | `cSS` | S target=P1; S title=P2 | NW: NCDialogBox | = |
| 0x57 | `RequestShowboard` (163 `+0x28C`) | `cd` | d = 1 (constante) | NW: NCConsole (abre o board) | = |
| 0x58 | `RequestEnchantItem` (171 `+0x2AC`) | `cd` | d objectId=A1 | NW: NCInvenItemWnd, NCEquipItemWnd | = |
| 0x59 | `SendDestroyItem` (167 `+0x29C`) | `cdd` | d A1; d A2 (objectId, count) | nenhum chamador achado (slot 167) | = |
| 0x59 | `RequestDestroyItem` (165 `+0x294`) | `cdd` | d objectId (ItemInfo+4); d count (ItemInfo+0xC) | NW: NConsoleWnd::RequestDestroyItem (NCItemWnd.h) | = |
| 0x5B | `SendBypassBuildCmd` (164 `+0x290`) | `cS` | S command=P1 (linha após `//`, sem as barras) | NW: NCChatWnd (`//texto` se `BuilderLevel != 0`), NCPetitionWnd (3x) | = |
| 0x5C | `MoveToLocation` (49 `+0x0C4`) | `cddddddd` | d vehicleId (`[veículo+0x94]`); 6 d = alvo e origem em coordenadas relativas ao veículo (RotateAngleAxis) | variante de 0x01 quando o pawn está em veículo | = |
| 0x5D | `CanNotMoveAnymore` (61 `+0x0F4`) | `cddddd` | d vehicleId (`[veículo+0x94]`); d,d,d x,y,z relativos ao veículo; d yaw=A5 | variante de 0x36 quando o `AActor*` A1 está num veículo | = |
| 0x5E | `RequestFriendInvite` (125 `+0x1F4`) | `cS` | S name=P1 | nenhum chamador achado | = |
| 0x5F | `RequestFriendAddReply` (128 `+0x200`) | `cd` | d response=P1 | nenhum chamador achado | = |
| 0x60 | `RequestFriendList` (127 `+0x1FC`) | `c` | só o opcode | NW: chat /friendlist | = |
| 0x61 | `RequestFriendDel` (126 `+0x1F8`) | `cS` | S name=P1 | nenhum chamador achado | = |
| 0x62 | `RequestCharacterRestore` (73 `+0x124`) | `cd` | d slot=A1 | NW: NCPawnSelectWnd | = |
| 0x63 | `RequestQuestList` (169 `+0x2A4`) | `c` | só o opcode | NW: NCQuestWnd (abre) | = |
| 0x64 | `RequestDestroyQuest` (170 `+0x2A8`) | `cd` | d questId=A1 | NW: NCQuestWnd | = |
| 0x66 | `RequestPledgeInfo` (98 `+0x188`) | `cd` | d pledgeId=A1 (só se `GetRequestedPledgeID` ainda não marcou) | interno Network.dll: chamado de `0x100126EF` (manipulador de pacote do servidor; INFERIDO: clã desconhecido) [corrigido pela revisão: a nota citava também `0x1001D9F8`, mas ali o `call [edx+0x188]` é sobre `[handler+0x48]` = o `FL2NetNotify` (`OnAutoAttackStart`, slot 98), não sobre o `UL2NetHandler`] | = |
| 0x67 | `RequestPledgeExtendedInfo` (101 `+0x194`) | `cS` | S P1 (nome) | nenhum chamador achado | = |
| 0x68 | `RequestPledgeCrest` (100 `+0x190`) | `cd` | d crestId=A1 | interno Network.dll: `0x10002575` (se `..\PledgeCrest\%d_%d.bmp` não existe) [corrigido pela revisão: a nota citava também `0x1001DC42`, mas ali o `call [edx+0x190]` é sobre `[handler+0x48]` = o `FL2NetNotify` (`OnSocialAction`, slot 100), não `RequestPledgeCrest`] | = |
| 0x69 | `RequestSurrenderPersonally` (115 `+0x1CC`) | `cS` | S name=P1 | nenhum chamador achado | ausente |
| 0x6A | `Ride` (54 `+0x0D8`) | `cdd` | d bRide=A1; d rideType=A2 | nenhum chamador achado (a montaria vem do servidor, no pacote servidor->cliente 0x9F `RidePacket`) | ausente |
| 0x6B | `RequestAcquireSkillInfo` (172 `+0x2B0`) | `cdd` | d skillId=P1; d level=P2 | NW: NCTrainWnd | = |
| 0x6C | `RequestAcquireSkill` (173 `+0x2B4`) | `cdd` | d skillId=P1; d level=P2 | NW: NCTrainWnd (Learn) | = |
| 0x6D | `RequestRestartPoint` (160 `+0x280`) | `cd` | d tipo=P1 (ponto de ressurreição) | NW: NCRestartMenuWnd | = |
| 0x6E | `RequestGMCommand` (81 `+0x144`) | `cSdd` | S name=A1; d command=A2 (1 status, 2 pledge, 3 skills, 4 quests, 5 inventory, 6 warehouse); d 0 (constante) | NW: wrappers RequestGMInventory/Skills/Status/Quests/Warehouse/Pledge (sem chamador na fonte; INFERIDO: a janela do GM) | ≠ REF `Sd` [corrigido pela revisão: era "="; em `RequestGMCommand.java` o 3o campo (`// _unknown = readInt()`) está comentado: o `d` = 0 do cliente sobra e o REF o ignora] |
| 0x6F | `RequestPartyMatchConfig` (135 `+0x21C`) | `cdddS` | d P1; d P2; d P3; S memo=P4 | NW: NCMatchOptionWnd, NCConsole | = |
| 0x70 | `RequestPartyMatchList` (136 `+0x220`) | `cd` | d status=A1 | NW: NCConsole (3x), NCFrameCtrl | = |
| 0x71 | `RequestPartyMatchDetail` (137 `+0x224`) | `cd` | d objectId=A1 | NW: NCMatchListCtrl | = |
| 0x72 | `RequestCrystallizeItem` (166 `+0x298`) | `cdd` | d objectId=A1; d count=A2 | NW: NCInvenItemWnd, NCEquipItemWnd | = |
| 0x73 | `RequestPrivateStoreManage` (195 `+0x30C`) | `c` | só o opcode | NW: chat /vendor, NCActionItemWnd, NCShortCutWnd, NConsoleWndActions | `=` [corrigido pela revisão: era "≠ REF `dq`"; em `RequestPrivateStoreManageSell.java` o `readInt()` e o `readLong()` estão **comentados**, o REF não lê nada] |
| 0x74 | `SetPrivateStoreList` (196 `+0x310`) | `cdb` | d N=P1; N x (d id do item, d count, d price) = 3 Top() por item | NW: NCPrivateShopWnd (Start) | = (lista) |
| 0x75 | `RequestPrivateStoreManageCancel` (197 `+0x314`) | `c` | só o opcode | NW: NCPrivateShopWnd | ausente |
| 0x76 | `RequestPrivateStoreQuit` (198 `+0x318`) | `c` | só o opcode | NW: NCPrivateShopWnd, NCConsole, NCFrameCtrl | = |
| 0x77 | `SetPrivateStoreMsg` (199 `+0x31C`) | `cS` | S msg=A1 | NW: NCDialogBox | = |
| 0x78 | `RequestPrivateStoreList` (200 `+0x320`) | `cd` | d objectId=A1 | nenhum chamador achado (o clique no vendedor sai por 0x04) | ausente |
| 0x79 | `SendPrivateStoreBuyList` (201 `+0x324`) | `cddb` | d storeOwnerId=P1; d N=P2; N x (d objectId, d count, d price) | NW: NCPrivateShopWnd (Buy) | = (lista) |
| 0x7A | `ReviveReply` (76 `+0x130`) | `cd` | d P1 (resposta); P2 e P3 são lidos e descartados | NW: NCDialogBox (2x) | ausente |
| 0x7B | `RequestTutorialLinkHtml` (94 `+0x178`) | `cS` | S link=P1 | NW: NCConsole (tutorial) | = |
| 0x7C | `RequestTutorialPassCmdToServer` (95 `+0x17C`) | `cS` | **S = lixo**: o `Top()` é descartado e a `S` lê o `esi` salvo do prólogo (ver 4) | NW: NConsoleWnd::RunHtmlCommand quando o viewer é o `NCTutorialHtmlViewer` (um `bypass` na página de tutorial) | = |
| 0x7D | `RequestTutorialQuestionMarkPressed` (96 `+0x180`) | `cd` | d number=A1 | NW: NCTutorialViewerWnd | = |
| 0x7E | `RequestTutorialClientEvent` (97 `+0x184`) | `cd` | d eventId=A1 | NW: NCQuestWnd (SendTutorialEvent) | = |
| 0x7F | `RequestPetition` (78 `+0x138`) | `cS` | S text=A1. SEM o `type` que o REF lê | NW: NCChatWnd (/petition) | ≠ REF `Sd` |
| 0x80 | `RequestPetitionCancel` (79 `+0x13C`) | `cS` | **S = lixo** (ver 4) | NW: NCChatWnd, NCPetitionWnd | ≠ REF (nada) |
| 0x81 | `RequestGMList` (80 `+0x140`) | `c` | só o opcode | NW: NCChatWnd (/gmlist) | = |
| 0x82 | `RequestJoinAlly` (119 `+0x1DC`) | `cd` | d objectId=P1 | NW: chat /allyinvite | = |
| 0x83 | `RequestAnswerJoinAlly` (120 `+0x1E0`) | `cd` | d response=P1 | nenhum chamador achado | = |
| 0x84 | `RequestWithdrawAlly` (122 `+0x1E8`) | `c` | só o opcode | NW: NCDialogBox | = |
| 0x85 | `RequestOustAlly` (121 `+0x1E4`) | `cS` | S pledgeName=P1 | nenhum chamador achado | = |
| 0x86 | `RequestDismissAlly` (123 `+0x1EC`) | `c` | só o opcode | NW: NCDialogBox | = |
| 0x87 | `RequestSetAllyCrest` (117 `+0x1D4`) | `cdb` | d length=A1; b(len=A1, ptr=A2) | NW: o enviador do brasão de aliança (`0x10056570`) | = |
| 0x88 | `RequestAllyCrest` (118 `+0x1D8`) | `cd` | d crestId=A1 | interno Network.dll: `0x10002628` (se `AllyCrest/<serverId>_<crestId>.bmp` não existe; só com protocolo >= 363) | = |
| 0x89 | `RequestChangePetName` (194 `+0x308`) | `cS` | S name=A1 | NW: NCDialogBox (OK do diálogo de nome) | = |
| 0x8A | `RequestPetUseItem` (190 `+0x2F8`) | `cd` | d objectId=A1 | NW: NCPetInventoryWnd (2x), NCShortCutWnd | `=` [corrigido pela revisão: era "≠ REF `dqd`"; em `RequestPetUseItem.java` o `readLong()` e o 2o `readInt()` estão **comentados**, o REF lê só `d`] |
| 0x8B | `RequestGiveItemToPet` (191 `+0x2FC`) | `cdd` | d objectId=P1; d count=P2 | NW: NCInvenItemWnd (2x), NCEquipItemWnd (2x) | = |
| 0x8C | `RequestGetItemFromPet` (192 `+0x300`) | `cddd` | d objectId=P1; d count=P2; d P3 | NW: NCPetInventoryWnd (2x) | = |
| 0x8E | `RequestAllyInfo` (124 `+0x1F0`) | `c` | só o opcode | NW: chat /allyinfo | = |
| 0x8F | `RequestPetGetItem` (189 `+0x2F4`) | `cd` | d objectId=A1 (item no chão) | NW: NConsoleWnd::PetPickUp (barra de atalhos, painel de ações) | = |
| 0x90 | `RequestPrivateStoreBuyManage` (202 `+0x328`) | `c` | só o opcode | NW: chat /buy, NCActionItemWnd, NCShortCutWnd | = |
| 0x91 | `SetPrivateStoreBuyList` (203 `+0x32C`) | `cdb` | d N=P1; N x (d itemId, h enchant, d count, d price) = 4 Top() por item | NW: NCPrivateBuyWnd | = (lista) |
| 0x92 | `RequestPrivateStoreBuyManageCancel` (204 `+0x330`) | `c` | só o opcode | NW: NCPrivateBuyWnd | ausente |
| 0x93 | `RequestPrivateStoreBuyQuit` (205 `+0x334`) | `c` | só o opcode | NW: NCPrivateBuyWnd, NCConsole, NCFrameCtrl | = |
| 0x94 | `SetPrivateStoreBuyMsg` (206 `+0x338`) | `cS` | S msg=A1 | NW: NCDialogBox | = |
| 0x95 | `RequestPrivateStoreBuyList` (207 `+0x33C`) | `cd` | d objectId=A1 | nenhum chamador achado | ausente |
| 0x96 | `SendPrivateStoreBuyBuyList` (208 `+0x340`) | `cddb` | d buyerId=P1; d N=P2; N x (d objectId, d itemId, h enchant, d count, d price) = 5 Top() por item (18 B por item) | NW: NCPrivateBuyWnd (Sell) | ≠ REF [corrigido pela revisão: era "= (lista)"]: `RequestPrivateStoreSell.java` lê por item `d d h h d d` (**20 B**, dois `readShort`) e exige `N*20 == bytes restantes` antes de ler; com os 18 B do cliente a conta não fecha e o REF **descarta o pacote** (vender a uma loja de compra não funciona no REF) |
| 0x97 | `SendTimeCheckPacket` (209 `+0x344`) | `cdd` | d requestedTime = Engine+0x2D8 (o `d` do pedido 0xD4); d elapsed = Engine+0x2DC (ms contados) | Eng: UGameEngine::Tick `0x104D52FB`, depois do `RequestTimeCheck` (0xD4) do servidor armar `[+0x2D4]`, com o console no estado 6 | ausente |
| 0x98 | `RequestStartAllianceWar` (175 `+0x2BC`) | `cS` | S P1 | nenhum chamador achado na fonte (mas há chamadas por slot do console: ver 10, item 8) | ausente |
| 0x99 | `ReplyStartAllianceWar` (176 `+0x2C0`) | `cSd` | S P1; d P2 | idem | ausente |
| 0x9A | `RequestStopAllianceWar` (177 `+0x2C4`) | `cS` | S P1 | idem | ausente |
| 0x9B | `ReplyStopAllianceWar` (178 `+0x2C8`) | `cSd` | S P1; d P2 | idem | ausente |
| 0x9C | `RequestSurrenderAllianceWar` (179 `+0x2CC`) | `cS` | S P1 | idem | ausente |
| 0x9D | `RequestSkillCoolTime` (174 `+0x2B8`) | `c` | só o opcode | nenhum chamador achado (slot 174) | ausente [corrigido pela revisão: era "="; a entrada `REQUEST_SKILL_COOL_TIME(0x9D, ...)` do `ClientPackets.java` está **comentada**, o REF ignora o opcode] |
| 0x9E | `RequestPackageSendableItemList` (188 `+0x2F0`) | `cd` | d objectId=P1 | NW: NCDeliverWnd | = |
| 0x9F | `RequestPackageSend` (187 `+0x2EC`) | `cddb` | d objectId=P1; d N=P2; N x (d objectId, d count) | NW: NCDeliverWnd | = (lista) |
| 0xA0 | `RequestBlock` (129 `+0x204`) | `cd` / `cdS` | d tipo (0 bloquear, 1 desbloquear, 2 lista) = P1; [S nome = P2 nos tipos 0 e 1] | NW: chat /block, /unblock, /blocklist | = (tipo 2 sem nome) |
| 0xA1 | `RequestCastleSiegeInfo` (180 `+0x2D0`) | `cd` | d castleId=P1 | wrapper do console `0x10044F20` (sem nome na fonte); chamador não identificado | ausente |
| 0xA2 | `RequestCastleSiegeAttackerList` (181 `+0x2D4`) | `cd` | d castleId=P1 | NW: NCSiegeInfoMainWnd | = |
| 0xA3 | `RequestCastleSiegeDefenderList` (182 `+0x2D8`) | `cd` | d castleId=P1 | NW: NCSiegeInfoMainWnd | = |
| 0xA4 | `RequestJoinCastleSiege` (183 `+0x2DC`) | `cddd` | d castleId=P1; d P2; d P3 (INFERIDO pelo REF: isAttacker, isJoining) | NW: NCDialogBox (por slot do console) | = |
| 0xA5 | `RequestConfirmCastleSiegeWaitingList` (184 `+0x2E0`) | `cddd` | d castleId=P1; d clanId=P2; d approved=P3 | NW: NCDialogBox (por slot do console) | = |
| 0xA6 | `RequestSetCastleSiegeTime` (185 `+0x2E4`) | `cdd` | d P1; d P2 | NW: NCSiegeInfoMainWnd | ausente |
| 0xA7 | `RequestMultiSellChoose` (186 `+0x2E8`) | `cdd` | d listId=P1; d entry=P2. SEM o `amount` que o REF lê | NW: NCVIPShopWnd (RequestBuyVIPItem) | ≠ REF `ddd` |
| 0xA8 | `NetPing` (223 `+0x37C`) | `cddd` | d id=A1; d,d = ftol de valores derivados de `[handler+0x78]` (tempo médio de quadro) e `GL2PawnCR` | nenhum chamador achado | ≠ REF (nada) |
| 0xA9 | `RequestRemainTime` (224 `+0x380`) | `c` | só o opcode | NW: chat (NCMD_DwarvenManufacture), NConsoleWnd::RequestDwarvenManufacture | ausente |

Notas sobre linhas da tabela (todas VERIFICADAS no assembly salvo onde marcado):

- **Listas** (`0x1E 0x1F 0x31 0x32 0x74 0x79 0x91 0x96 0x9F`): a função lê `N` e depois repete um `Top()` por campo, empacotando cada item com `0x10002990` (empacotador que devolve 0 em estouro) num
  buffer local de `0x2400` bytes; o `b` do envio é esse bloco cru. O **`N` que sai no fio é o `P` lido, não a quantidade de itens que coube**: se o buffer estourar, os itens que sobram são
  descartados e o `N` continua grande. Formatos por item: Sell `ddd` (objectId, itemId, count; o log do cliente diz `ServerID/ClassID/Num`), Buy `dd` (itemId, count), Warehouse `dd`
  (objectId, count), PackageSend `dd`, SetPrivateStoreList `ddd` (id, count, price), SendPrivateStoreBuyList `ddd`, SetPrivateStoreBuyList `dhdd` (itemId, enchant em 16 bits com
  `movsx`, count, price), SendPrivateStoreBuyBuyList `ddhdd`.
- `RequestSetPledgeCrest` / `RequestSetAllyCrest` (`cdb`): o tamanho vai **duas vezes** (um `d` e o inteiro de tamanho do `b`), depois os bytes do brasão.
- `RequestDropItemFromPet` usa o **mesmo opcode `0x12`** de `RequestDropItem`: o servidor não distingue o jogador do pet.
- `0x39` é um **opcode com subopcode** (`c`): 0 `SendCharacterInfo`, 1 `SendSummonCmd`, 2 `SendServerStatus`, 3 `SendL2ParamSetting`. Nenhum chamador foi achado; o REF não trata `0x39`.
- `MoveToLocation` tem duas saídas: a normal (`0x01`, 7 inteiros depois do opcode) e a de veículo (`0x5C`); a escolha usa uma busca de `User` (`0x10007380`) sobre campos do `AActor*` dos argumentos. `CanNotMoveAnymore`
  idem (`0x36` / `0x5D`); na variante normal **nada sai se `A6 != 0`**.
- `ReviveReply` lê 3 valores do `L2ParamStack` mas só o 1o vai ao fio. `RequestShortCutUse`, `RequestMagicSkillUse` e `RequestActionUse` terminam com um `c` (`ctrl` é `d`, `shift` é `c`).
- **Ping, `ProtocolVersion(-2)`:** o REF desconecta; o servidor do dono precisa tolerar uma conexão que manda `07 00 00 FE FF FF FF` e fecha, a cada abertura da lista de servidores.

### 1.3 Quem recebe o quê no cliente depois de cada pedido (só para o servidor saber a resposta esperada)

Não é o foco (as respostas do servidor estão em `Rede_Protocolo_Servidor_Cliente.md`), mas três pedidos têm **efeito no cliente antes da resposta**: `RequestGameStart` (`NConsoleWnd::RequestGameStart`,
`NConsoleWndNet.cpp:78`) esconde a seleção, mostra a tela de carga e põe o estado `L2CS_Waiting` até o `AcceptGameStart`; `RequestCharacterDelete/Restore/Create` põem `L2CS_Waiting`; `RequestServerLogin` marca
`bRequested` e **não deixa o jogador pedir de novo** (o botão OK da lista fica inerte; só `AuthReconnect`/Cancel libera).

## 2. Enquadramento, empacotador e cifras (VERIFICADO, `Network.dll`)

### 2.1 O pacote

`CClientSocket::Send` (`0x10002B70`, slot `+0x60`, `__cdecl` com o **socket primeiro**: `Send(sock, fmt, op, args...)`): entra na `CRITICAL_SECTION` do socket (`+0x4D2C`), usa um buffer de pilha de `0x2000` bytes,
chama o empacotador `0x100026E0(buf, 0x1FFE, fmt, va_list)`; se devolveu 0 (formato vazio **ou estouro de `0x1FFE`**) **não envia nada**; senão calcula `tamanho = corpo + 2`, grava `[tamanho & 0xFF][tamanho >> 8]`
**antes** do corpo, **cifra o corpo** se `[socket+0x4E70] != 0` (chave em `+0x4E78`) e faz `send(sock, buf, tamanho, 0)`. O retorno do `send` é **ignorado** (os contadores `+0x496C/+0x4970` somam `tamanho` mesmo
assim): como o socket é não bloqueante (`WSAAsyncSelect`), uma escrita parcial ou `WSAEWOULDBLOCK` perderia o pacote em silêncio (INFERIDO).

O `CAuthSocket::Send` (`0x10003230`, vtable `0x10033DD0 +0x60`) é igual, com a cifra do login (2.4) e um buffer de `0x2004` bytes.

O **leitor** `0x10002A00(ctx, buf, fmt, &out...)` entende `c h d f S s` (tabela `0x10002B30`/`0x10002B4C`); qualquer outra letra grava o log `unknown type %c in %s` (`OutputDebugStringW`) e não consome nada. **Não recebe o tamanho
do pacote**: nenhuma leitura é conferida contra o fim do corpo. `S`/`s` copiam até o zero para um destino de tamanho dado (trunca e põe zero no fim).

### 2.2 Cifra do socket de jogo (`CClientSocket`, slots `+0x70` cifra e `+0x74` decifra: `0x10002430` e `0x10002480`)

```
cifrar(buf, key[8], n):   buf[0] ^= key[0]
                          para i = 1..n-1:  buf[i] ^= key[i & 7] ^ buf[i-1]      // buf[i-1] já cifrado
                          key64 += n                                           // soma de 64 bits; n = tamanho do CORPO (sem os 2 bytes)
decifrar:                 out[i] = in[i] ^ key[i & 7] ^ in[i-1]  (in[-1] = 0); key64 += n
```

- **Duas chaves independentes** por socket: envio em `+0x4E78`, recepção em `+0x4E80`, cada uma avançando pelo tamanho do corpo do que passou por ela. O 1o pacote do servidor (`KeyPacket`) vem **em claro**
  e não avança a chave de recepção; o 1o do cliente (`ProtocolVersion`) também.
- A chave nasce no manipulador do op 0 do jogo (`0x10007F40`): lê `"cdd"` = `c` resultado da versão, `d1`, `d2`; faz `[socket+0x4E70] = (d2 != 0)`; `chave = d1 | (0x87546CA1 | sign(d1)) << 32` e grava **a mesma chave** em
  `+0x4E78` e `+0x4E80` (`cdq; or edx,0x87546CA1` em `0x10007FA4..0x10007FC4`). Ou seja: os 4 bytes altos da chave **não vêm do servidor**; com `d1` negativo viram `FF FF FF FF`.
- **REF concorda** (`Encryption.java`: `_inKey/_outKey[8]`, `i & 7`, soma de 32 bits nos 4 primeiros bytes, 1o `encrypt` não cifra; `CRYPT_KEY = 94 35 00 00 A1 6C 54 87`, "the last 4 bytes are fixed"). A soma de 64 bits do cliente e a de 32 do REF só divergem depois de 4 GiB de tráfego.
- O pacote que o servidor manda com o resultado: `c` = 1 versão aceita / 0 recusada (o cliente usa só o byte: `ReceiveVersionCheck`, seção 3.1), `d1` = bytes 0..3 da chave, `d2` = qualquer valor `!= 0` para cifrar.

### 2.3 O `ProtocolVersion` que o cliente manda

`CClientSocket::OnConnect` (`0x10002320`): faz `sscanf`-like `"%cRevision: %d %c"` sobre a string literal **`"$Revision: 419 $"`** (`0x1002E8D4`) e envia `cd` = `0x00`, **419** (`0x1A3`), guardando 419 na global
`0x10041134` (usada depois: a busca do brasão de aliança só roda com protocolo `>= 0x16B`). O `Init` do servidor de login do REF manda a revisão `0x0000C621`; o cliente **não a compara** (ele lê o `d` e o descarta).

### 2.4 Cifra do socket de login (`CAuthSocket`, slots `+0x78` cifra `0x100032E0` e `+0x7C` decifra `0x10003330`)

- **Blowfish** padrão (P e S de pi: `P[0] = 0x243F6A88`, `S0[0] = 0xD1310BA6`, em `0x10040070` e `0x100400B8`), blocos de 8 bytes em ECB, palavras de 32 bits lidas direto da memória (little-endian: a variante do L2),
  chave **`"[;'.]94-31==-%&@!^+]"` + `0x00`**, 21 bytes (`0x10041138`, tamanho `0x15`), expandida por `0x10001880` na construção do `CAuthSocket` (`0x10014120`). O REF usa a mesma (`new NewCrypt("[;'.]94-31==-%&@!^+]\000")`).
- **Enviar** (`0x100032E0`): `L = (n + 7) & ~7` (n = tamanho do corpo); `chk = XOR de todos os dwords de [0, L)`; grava `chk` em `[L]` (o dword seguinte, `[L+4]`, já é 0 porque o buffer foi zerado); tamanho final `L + 8`; Blowfish sobre `L + 8`.
  O cabeçalho do pacote vai com `L + 8 + 2`.
- **Receber** (`0x10003150` + `0x10003330`): se `[socket+0x4E70] != 0` decifra `buf+2` com tamanho `len-2` **só se `(len-2) & 7 == 0`** (senão passa em claro e dá lixo); **não confere o checksum**; o corpo entregue ao
  manipulador ainda inclui o preenchimento e o checksum no fim.
- **Quando a cifra liga:** `RequestAuthLogin` põe `[auth+0x4E70] = 0`; o **1o pacote do servidor de login (`Init`) chega em claro**; o manipulador do op 0 (`0x100075F0`, em `0x10007636`) liga a flag e só então manda o
  `cxxdh`. Os manipuladores de falha (`LoginFail`, `AccountKicked`, `PlayFail`, `ServerFail`, `BlockedAccountWithMsg`) desligam a flag e fecham o socket; o de `PlayOk` (`0x100119B0`) fecha o socket **e deixa a flag em 1** (`0x10011A20`;
  irrelevante, o próximo `RequestAuthLogin` zera).

### 2.5 Recepção, fila e despacho

- **Jogo:** o `FD_READ` (mensagem `0x503`) só faz `ReleaseSemaphore`; quem lê é uma **thread** (`0x1000FD60`, criada em `Connect` `0x10010020` por `_beginthread`): `recv(sock, 0x10051300, 0x4000, 0)` e uma máquina de estados em `+0x561C`
  (0 = juntando os 2 bytes do tamanho em `+0x844`, 1 = juntando o corpo; `+0x5614` = tamanho esperado, `+0x5618` = acumulado). Com o pacote completo chama o slot `+0x6C` (`0x10010240`): decifra o corpo
  (`+0x74`) se a flag liga, **copia o corpo menos o opcode para um bloco novo de `len-2` bytes** (`NetCmd`, com **um** byte zero a mais no fim) e enfileira `{opcode, bloco}` por `UNetworkHandler::AddNetworkQueue` (slot `+0x74` do handler).
- **Login:** o `FD_READ` lê **na própria mensagem** (`0x10002F00`): mesma máquina de estados, `recv` em laço até esvaziar.
- `UNetworkHandler::Tick` (`0x10010730`) esvazia a fila **no thread principal**, todos os pacotes de cada quadro: estado `[handler+0x44] == 1`: opcode `>= 0x0A` vira `0x0A` e chama `[0x10041150 + op*4]`
  (11 entradas: `0x100075F0, 0x10007760, 0x10007830, 0x100078F0, 0x100079F0, 0x10007BD0, 0x10007CA0, 0x100119B0, 0x10007D70, 0x10007E30` e o `Dummy` `0x10002130`); estado `2`: opcode `>= 0xED` vira `0xED` e chama
  `[0x10041180 + op*0x104]` (a tabela de 238 entradas, `Rede_Protocolo_Servidor_Cliente.md` §5); estado `0`: o pacote é **descartado sem liberar a memória**. Depois do manipulador libera o bloco e o `NetworkPacket`.
- Manipuladores do login: 0 `Init` (`dd`), 1 `LoginFail` (`c`), 2 `AccountKicked` (`d`), 3 `LoginOk` (`dddddddd`), 4 `ServerList` (`cc` + n x `cddcchhcd`), 5 `ServerFail` (`c`), 6 `PlayFail` (`c`), 7 `PlayOk` (`dd`),
  8 `AccountKicked` (outro, `c`), 9 `BlockedAccountWithMsg` (`c` + n x `hS`), 10 `Dummy`; os nomes são os do log `(Receive)...` de cada um (VERIFICADO).

### 2.6 Mensagens do Windows e `ServerAddr`

`DispatchSocketWindowMsg(hwnd, msg, wParam, lParam)` (`0x10022160`) é chamada pelo procedimento de janela do viewport (`WinDrv.dll` `0x1110781C`, no começo, **só com `GIsEditor == 0`**; se devolvesse diferente de 0 a mensagem acabaria ali, mas a função devolve sempre 0).
`msg == 1` (WM_CREATE): `0x10022020` lê `GConfig [URL] ServerAddr` e o copia (ANSI) para `+0x484C` dos **dois** sockets, e guarda o `hwnd` em `+0x28`. `msg == 0x502` (WM_USER+0x102, `FD_CONNECT`): slot `+0x64` do socket
**ativo**; `0x503`: slot `+0x68`. Socket ativo = `[handler+0x3C]` (jogo) se o estado é 2, `[handler+0x40]` (login) se o estado é 0 ou 1. `WSAAsyncSelect` pede `FD_CONNECT` (`0x10`) na conexão e `FD_READ|FD_OOB|FD_CLOSE` (`0x25`) depois dela.
O arquivo do ini (`L2.ini`, Ver212 cifrado) **não foi lido** (seção 9).

## 3. Os fluxos

### 3.1 Login até o jogo

| # | Direção | Bytes / efeito | Onde (VERIFICADO) |
|--:|---|---|---|
| 1 | UI | `NCAuthWnd::OnOK` valida que há conta e senha, chama `NConsoleWnd::RequestAuthLogin` -> `UNetworkHandler::RequestAuthLogin` (`0x100033A0`): copia a conta (até 14 `wchar`) em `0x100567CC` e a senha (até 16) em `0x100567EA` (`0x10022974` = `wcsncpy`), põe `[auth+0x4E70] = 0`, chama `Connect(ServerAddr, 2106)` (`0x10002C50`) e `[handler+0x44] = 1`. **Devolve sempre 1** (a "falha de conexão" da janela nunca dispara) | `0x100033A0..0x1000345D` |
| 2 | TCP | `FD_CONNECT` sem erro -> `CAuthSocket::OnConnect` (`0x10002E80`) só pede `FD_READ|FD_OOB|FD_CLOSE`; **não manda nada** | `0x10002E80` |
| 3 | S -> C | `Init` (op 0, `dd`: `sessionId`, revisão), **em claro**; o manipulador liga a cifra e **responde** | `0x100075F0` |
| 4 | C -> S | `0x00` `cxxdh` (Blowfish): conta[14], senha[16], 8, `atoi(package.dat)` | `0x10007706` |
| 5 | S -> C | `LoginOk` (op 3): `d1 d2` viram loginOk1/2 (`0x100567AC/B0`), `d3 d4` vão a `0x100567B4/B8`; o manipulador chama `OnStartNotice` (slot 120 do `FL2NetNotify`) | `0x100078F0` |
| 6 | C -> S | `0x05` `RequestServerList` (loginOk1, loginOk2, 3), depois que a janela de aviso/termo é aceita | `0x10003460` |
| 7 | S -> C | `ServerList` (op 4): `OnStartServerList`, uma `OnAddServerList` por servidor, `OnEndServerList`. O `NCServerSelectWnd` chama `AddServerPingList(serverId-1, ip, porta)` para cada um e `EndServerPingList()`: **uma conexão curta a cada servidor** (ping) | `0x100079F0`, `NCServerSelectWnd.cpp:572` |
| 8 | C -> S | `0x02` `RequestServerLogin` (loginOk1, loginOk2, serverId) quando o jogador clica OK | `0x100034C0` |
| 9 | S -> C | `PlayOk` (op 7, `dd`: playOk1, playOk2 -> `0x100567C8` e `0x100567C4`). O manipulador zera o estado, **fecha o socket de login**, e chama `Connect(inet_ntoa(ip), porta)` no socket de jogo (`0x10010020`), `[handler+0x44] = 2` | `0x100119B0` |
| 10 | TCP | `FD_CONNECT` -> `CClientSocket::OnConnect`: `WSAAsyncSelect`, e **C -> S `0x00` `cd` 419** (em claro) | `0x10002320` |
| 11 | S -> C | `KeyPacket`/`VersionCheck` (op 0, `cdd`): liga a cifra, calcula as chaves (2.2); chama `FL2NetNotify::OnReceiveVersionCheck` (slot 194, `[vt+0x308]`) = `NConsoleWnd::ReceiveVersionCheck` | `0x10007F40` |
| 12 | C -> S | `0x08` `cSdddd` (cifrado): conta UTF-16, **playOk2, playOk1**, loginOk1, loginOk2 | `0x1000806A` |

`NConsoleWnd::ReceiveVersionCheck` (`0x10048050`, `NConsoleWndLogin.cpp:297`): `c == 0` (versão recusada) com `[URL] L2VersionCheck` ligado (padrão 1, `GetBool`) manda `SendLogOutPacket` (`0x09`) e mostra `MSG_ProtocolQuit`; com `L2VersionCheck=0`
só mostra `MSG_ProtocolContinue`. **O `AuthLogin` (passo 12) sai de qualquer jeito** logo depois (o manipulador não olha o resultado).

### 3.2 Da seleção de personagem ao mundo

1. `RequestNewCharacter` `0x0E` (NCLobbyWnd, botão Create) -> o servidor manda os modelos de classe; `RequestCharacterCreate` `0x0B` (NCPawnCreateWnd) -> `OnAcceptCharacterCreate`; `RequestCharacterDelete` `0x0C` / `RequestCharacterRestore` `0x62` (índice do slot).
2. `RequestGameStart` `0x0D` (slot do personagem) -> o servidor manda `CharacterSelected` (op `0x21`); `OnReceiveCharacterSelectedPacket(x,y,z)` carrega o mapa por **posição**.
3. **`RequestEnterWorldPacket` `0x03` sai do `UGameEngine::Tick`** (`0x104D559F`), e não de uma janela: um contador estático `[0x109F5A44]` conta quadros enquanto `[0x109F53DC]` está ligado: com o contador em 0 chama o virtual `+0x9C` do objeto `[Engine+0x164]` com o pawn e uma posição (INFERIDO: `ULevel::FarMoveActor`),
   em 3 chama `OnAcceptGameStart(1)` no próprio Engine (slot 123), e com o contador `>= 4` (o 5o quadro) manda o `0x03`, desliga a flag e volta o contador a 1 (INFERIDO: `[0x109F53DC]` = "mapa carregando";
   o disparo do envio é VERIFICADO).
4. `SendAppearingPacket` `0x30` sai de `UGameEngine::ScreenFadeDone` (`0x104CB354`) quando o fade termina com o pawn do jogador existente.
5. Depois disso o servidor manda `UserInfo` etc. e o tráfego normal começa: `0x48 ValidatePosition` a cada 1,2 s enquanto o vetor em `[pawn+0x2E0]` (INFERIDO: `Velocity`) é diferente de zero e `GIsL2NetLogin` está ligado; `0x0F`, `0x3F`, `0x63`... quando o jogador abre inventário/skills/quests; `0x97 SendTimeCheckPacket` quando o servidor manda `RequestTimeCheck` (`0xD4`): o Engine arma `[Engine+0x2D4]` e o
   contador `[+0x2DC] += ftol(dt*1000)` ms; ao chegar em `[+0x2D8]` manda `(requestedTime, elapsed)` (só com o console no estado 6) e zera o flag.

### 3.3 Sair e voltar ao login (`AuthReconnect`)

- `SendLogOutPacket` `0x09` é só o aviso. **Quem fecha as conexões é `UNetworkHandler::AuthReconnect` (`0x10003530`)**: `[handler+0x44] = 0`, fecha o socket de login (`0x10002E30`: `closesocket` e `DeleteCriticalSection`, zera `+0x2C`,
  `+0x4844/48`, flag de cifra) **e o de jogo** (`0x100022D0`: idem, mais `[+0x4950] = 0` e a global `[0x1005530C] = 0`, que faz a thread de leitura terminar na próxima espera de 100 ms), e zera a flag do jogo.
- `NConsoleWnd::ReturnToLogin` (`0x10045EF0`: o "Previous" da seleção) faz `SendLogOutPacket` + `AuthReconnect` + volta à janela de login; o Cancel da lista de servidores (`NCServerSelectWnd.cpp:382`) chama só `AuthReconnect`.
- `RequestRestart` `0x46` (menu do sistema, comando `L2Restart`) **não** mexe nos sockets: o servidor responde e o cliente volta à seleção pelos eventos.
- O que o nosso `ReturnToLogin` acrescenta (`F6_ReturnToLogin.md`): observa os slots `+0x64` (`OnConnect`) e `+0x68` (`OnUserNetmessage`) da vtable `0x33D58`, para detectar erro de conexão e `FIN` sem erro; confere com o que li (`8B 44 24 08 83 EC 08 C1 E8 10` e `55 8B EC 6A FF 68` são os começos dos dois, VERIFICADO nos endereços `0x10002320` e `0x10010370`).

## 4. Defeitos do cliente stock que aparecem no fio (VERIFICADO no assembly; o efeito no servidor é INFERIDO)

| Pacote | O que o código faz | O que o servidor recebe |
|---|---|---|
| `0x4B FinishRotating` (`0x10003B90`) | formato `cdd`, mas só `A1` é empurrado; o 2o `d` lê o **`esi` salvo do chamador** (o `push esi` do prólogo) | o 2o `d` é lixo de registrador; o REF o lê e descarta |
| `0x80 RequestPetitionCancel` (`0x10006350`) | formato `cS`, **nenhuma** string empurrada; a `S` pega o próximo slot da pilha = o **endereço de retorno** (um ponteiro para código do NWindow) | o corpo é `80` + os bytes do código do NWindow a partir desse endereço até o primeiro par `00 00` (se não houver em `0x1FFE` bytes o empacotador falha e **nada** sai). Um servidor que exija corpo vazio recusa |
| `0x7C RequestTutorialPassCmdToServer` (`0x10004560`) | chama `Top()` e **descarta**; empurra só o opcode; a `S` pega o slot seguinte = o **`esi` salvo no prólogo** (`push esi` em `0x10004560`), o `esi` do chamador (INFERIDO: o `this` do console, em `RunHtmlCommand`) | o `bypass` de uma página de **tutorial** (`NCTutorialHtmlViewer`, `RunHtmlCommand`) **nunca chega**; chegam bytes de memória apontada pelo `esi` até o primeiro `00 00` (INFERIDO: o início do objeto `NConsoleWnd`, que começa pelo ponteiro da vtable). O caminho que funciona para o tutorial é `0x7B RequestTutorialLinkHtml` (`link`) |
| `0x04 RequestAttack` (slot 158) | só 4 `d`, sem o `c` final do `Action` | nenhum chamador achado, então só importa se o dono ou um recurso o chamar |
| `0x01 MoveBackwardToLocation` (slot 50) | 6 `d` (alvo e origem), sem o 7o (modo) do `MoveToLocation` | o REF lê 7 inteiros; o 7o some. Quem o chama: o bloco de teclado do `UInput::Exec` (heur.) |
| `0x03 EnterWorld(INT)` (slot 56) | manda `cS` com a **string literal `L"narcasse"`** (`0x10035954`) e antes cria um `User` com `[URL] CharName` | nenhum chamador achado; o caminho real é `RequestEnterWorldPacket` (mesmo opcode, corpo vazio) |
| `0x12 RequestDropItemFromPet` (slot 193) | mesmo opcode do `RequestDropItem` | o servidor não sabe se foi o pet |
| `0x7F RequestPetition` | só o texto; o REF lê `type` depois | o `type` falta |
| `0xA7 MultiSellChoose` (e `0x7F`, `0x01`, `0x04` acima) | menos campos que o REF | o REF lê além do fim; o servidor do dono deve ler só o que o cliente manda [corrigido pela revisão: `0x14`, `0x24` e `0x8A` saíram desta linha; as leituras extras do REF estão comentadas e o formato é igual] |
| `0x96 SendPrivateStoreBuyBuyList` | 18 B por item (`ddhdd`) | o REF espera 20 B por item (`ddhhdd`) e confere `N*20 == restante`: **recusa** o pacote inteiro [acrescentado pela revisão] |
| lista com muitos itens | `N` é o `P` lido, o bloco é limitado a `0x2400` bytes | com mais de 768 itens de 12 B (ou 1152 de 8 B) o `N` não bate com o bloco |
| `0xA8 NetPing` | `d` id + dois inteiros de tempo (`[handler+0x78]`, `GL2PawnCR`) | o REF não lê nada; nenhum chamador achado |

## 5. O que o cliente faz com o que o servidor manda de errado (e com a conexão que cai)

| Situação | O que o cliente faz | Prova |
|---|---|---|
| 1o pacote do jogo não é o `KeyPacket` | só o manipulador do opcode `0` (`0x10007F40`) liga a cifra; se ele não vier, a flag fica em 0 e o cliente continua **mandando e lendo em claro**; o que o servidor mandar cifrado vira lixo | `0x10007F40`; INFERIDO |
| opcode `>= 0xED` (jogo) ou `>= 0x0A` (login) | vira `0xED` / `0x0A` e cai no `Dummy` (só grava um log). Os 12 opcodes de reserva da tabela do jogo também são manipuladores vazios | `Tick` `0x10010970`, `0x100109C7` |
| pacote com estado `[handler+0x44] == 0` | descartado, **sem liberar** (vaza) | `Tick` |
| pacote mais curto que o formato | o leitor não confere o tamanho; lê o que houver depois do bloco (um byte zero, depois lixo da heap). `S` sem zero lê além | `0x10002A00` |
| tamanho do cabeçalho `< 2` | grava "packet size is under 2 something wrong!" e **segue**; no estado de corpo calcula `len - acumulado` negativo e copia com tamanho negativo (INFERIDO: falha de proteção) | `0x1000FFD2`, `0x1000FE36` |
| tamanho `== 2` | `OnPacket` loga "Critical Bug : Bad Packet Size(size=2) [Src\ClientSocket.cpp:7765]" e **descarta**; sem queda | `0x10010298` |
| tamanho `> 0x44E8` (17.640) | o buffer `+0x844..+0x4D2C` do socket não é conferido: estouro sobre a `CRITICAL_SECTION` e os campos seguintes (INFERIDO) | `0x1000FE5F` |
| TCP urgente (`FD_OOB`) | `MessageBoxW` modal com o texto `FD_OOB`, que trava a thread da UI até alguém fechar | `0x10010464` |
| `FIN` limpo do servidor | **nada**: `FD_CLOSE` com código de erro 0 vai à saída da função sem avisar; a thread de leitura só vê `recv == 0` e volta a esperar. A conexão "morre" e o cliente não percebe até um `send` falhar (e o retorno do `send` também é ignorado) | tabela `0x100104F8` (`FD_CLOSE` -> `0x10010406`) |
| `RST` / erro de rede | `FD_CLOSE` com código != 0 e `wParam == socket`: `FL2NetNotify::OnServerClose(1)` (jogo) ou `(0)` (login). `UGameEngine::OnServerClose` (`0x1041F3E0`): estado do console 6 ou 4 com argumento 1 ou 2, ou fora de 6 e 0 com argumento 0, chama o slot `+0x98` do console (a caixa de desconexão id `0x1D`); em jogo (6) com argumento 0 ou no estado 0 não faz nada | `0x10010481`, `0x100030BF`, `0x1041F3E0` |
| `connect` recusado (`WSAECONNREFUSED` `0x274D`) | só `OutputDebugStringW("Connection refused by server")`; nenhuma mensagem na UI (o nosso `ReturnToLogin` compensa) | `0x10002408` |
| `connect` com erro que não é `WSAEWOULDBLOCK` | só debug | `0x10002DA6` |
| `ServerAddr` não resolve | **`MessageBoxW` modal "ERROR / Can not resolve hostname"** | `0x10002DE6`, `0x10002E00` |
| winsock ausente / versão baixa / `socket` falha | `MessageBoxW` modal ("Can't find winsock.dll", "Socket version is low", "INVALID_SOCKET") | `0x10002C7A..0x10002E27` |
| `LoginFail`/`AccountKicked`/`PlayFail`/`ServerFail`/`Blocked*` | estado 0, fecha o socket de login e chama `OnAuthLoginFail`/`OnBlockedAccount`/`OnBlockedAccountWithMsg` (slots 32, 34, 33 do `FL2NetNotify`); o jogador volta a digitar | manipuladores 1, 2, 5, 6, 8, 9 |
| Login recebido com tamanho de corpo não múltiplo de 8 | a decifração é **pulada** e o corpo é tratado em claro | `0x10003334` |

## 6. Como acrescentar um pacote cliente -> servidor

1. **Sem protocolo novo (o mais barato):** use o `bypass` (`0x21 cS`, texto livre até 1.023 TCHAR; vide `RunHtmlCommand`): o nosso `ManufactureCmd` já manda `SendBypassToServer` por uma fila de 8 itens, **um a cada 120 ms** (o servidor
   do dono descarta bypass a menos de 100 ms do anterior: `F6_ManufactureCmd.md`). O `RequestActionUse(id, 0, 0)` (`0x45`) com um `id` novo é outra via sem protocolo novo: os ids 32..52 já saem pela nossa barra de atalhos e pela janela de ações (`F6_ManufactureCmd.md`).
2. **Opcode novo no socket de jogo:** do NWindow, pegue `GConsole->Engine->NetHandler` (`UEngine+0x5C`), o socket é `*(void**)((BYTE*)NetHandler + 0x3C)`, a função de envio é `(*(void***)Socket)[0x60/4]`, `__cdecl`, **socket primeiro**:
   `Send(Socket, "cdd", 0xB5, a, b)`. Ele já trava o lock do socket, empacota, **cifra** e envia (e devolve sem enviar se o formato estoura `0x1FFE`). Argumentos de `f` são `double` (8 bytes), `b` é (tamanho, ponteiro).
3. **No servidor:** uma entrada nova na tabela de pacotes do `ConnectionState.IN_GAME` (no REF: `ClientPackets.java` + uma classe com `readImpl`). Um opcode **desconhecido** no REF é **ignorado em silêncio** (`GamePacketHandler.handlePacket` devolve `null` quando `PACKET_ARRAY[op]` é nulo ou o estado não bate; a conexão fica) [corrigido pela revisão: a nota dizia "cai em `onUnknownPacket()`"; esse nome é do servidor Lucera do dono, que não foi lido aqui].
4. **Opcodes sem uso** nos dois lados (jogo): `0x13 0x18 0x19 0x3A 0x3B 0x3D 0x40 0x4C 0x54 0x56 0x5A 0x65 0x8D 0xAA..0xFF` (nenhum desses é mandado pelo `Network.dll` stock nem tratado pelo REF; `0x05..0x07` também estão livres no jogo mas `0x05` é do login).
5. **Cuidados:** o corpo é cifrado como um bloco só, então um opcode novo não muda a cifra; o contador da chave avança com o tamanho do corpo, **então um envio perdido desincroniza tudo dali em diante**; nunca mande por fora do `Send` (o contador e o lock são dele).
6. **Pacote servidor -> cliente** novo: fora do escopo (tabela de 238 entradas, `Rede_Protocolo_Servidor_Cliente.md` §5).

## 7. Comparação com o servidor de referência (REF) e o que muda para o servidor do dono

Os dois lados concordam em **opcodes e formato** para quase tudo que o jogo normal usa (movimento, ação, ataque, chat, itens, lojas, party, clã, aliança, perícias, criação de personagem). A diferença que importa:

- **O cliente manda, o REF não trata** (28 opcodes no jogo [corrigido pela revisão: eram 27, faltava `0x9D`]): `0x02` (Say), `0x10`, `0x1A` (teleport), `0x23`, `0x28`, `0x2D`, `0x2E`, `0x34` (RequestShortCutUse), `0x39`, `0x3E`, `0x49`, `0x69` (entrada com manipulador `null`), `0x6A` (Ride), `0x75`, `0x78`, `0x7A` (ReviveReply),
  `0x92`, `0x95`, `0x97` (TimeCheck), `0x98..0x9C` (guerra de aliança), `0x9D` (RequestSkillCoolTime: entrada comentada), `0xA1`, `0xA6`, `0xA9`. O servidor do dono (Lucera) pode tratar vários (o `0xA9` é o que o tipo de comando `NCMD_DwarvenManufacture` do chat stock manda; a nossa `/dwarvenmanufacture` usa `0x45` com id 37; o `0x7A` é a resposta do pedido de ressurreição).
- [corrigido pela revisão: a lista original de "11 opcodes" misturava os dois sentidos e contava leituras **comentadas** do REF em `0x14`, `0x24`, `0x73`, `0x8A`, que na verdade são iguais] **O REF lê mais que o cliente manda** (5): `0x01` `MoveBackwardToLocation` (o 7o `d`, modo), `0x04` `RequestAttack` (o `c` final), `0x7F` (type), `0xA7` (amount), `0x96` (20 B por item contra 18: o REF **recusa**).
  **O cliente manda mais que o REF lê** (4; o REF ignora o excesso): `0x03` `EnterWorld(INT)` (a `S` "narcasse", caminho morto), `0x6E` (o 3o `d` = 0), `0x80` (a `S` de lixo), `0xA8` (os 3 `d`).
- **Tudo o que o REF trata, o cliente stock manda** (nenhum opcode do REF fica sem remetente).
- O **login** do REF e o do cliente concordam: `Init` (`00 sessionId revisão ...`), `RequestAuthLogin` (op 0, conta nos bytes 1..14, senha nos bytes 15..28), `LoginOk` (`03 d d 0 0 0x3EA 0 0 2`), `RequestServerList` (op 5, `d d c`), `RequestServerLogin` (op 2, `d d c`), `PlayOk` (`07 d d`),
  `AuthLogin` do jogo (`08 S playKey2 playKey1 loginKey1 loginKey2`). O REF envia `KeyPacket` (`00 c result, 8 bytes de chave, d serverId, d 1`): o cliente lê `c`, `d1` (bytes 0..3 da chave), `d2` (bytes 4..7, só como flag) e ignora o resto.

## 8. Como o cliente chega a cada pedido (resumo dos gatilhos)

- **Comandos de chat** (`NCChatWnd::OnEditCommand`, `NCChatWnd.cpp:826..1237`; os nomes `/...` vêm do `CommandName-e.txt` cifrado, **não lido**): `/attack`->0x04 ou 0x0A, `/attackforce`, `/attackstand`->0x0A, `/trade`->0x15, `/target`->0x04, `/targetnext`->0x04, `/pickup`->0x04, `/assist`->0x04,
  `/sit` `/stand`->0x1D, `/walk` `/run`->0x1C, `/social*`->0x1B, `/partyinvite` `/partyleave` `/partydismiss`->0x29 `0x2B` `0x2C`, `/allyinvite`->0x82, `/allyinfo`->0x8E, `/friendlist`->0x60, `/block` `/unblock` `/blocklist`->0xA0, `/vendor`->0x73,
  `/buy`->0x90, `/useskill*`->0x2F, `/useshortcut*`->0x34, `/petition` `/petitioncancel`->0x7F `0x80`, `/gmlist`->0x81, o tipo `NCMD_DwarvenManufacture`->0xA9, e `//texto` (com `BuilderLevel != 0`)->0x5B.
  Texto sem comando vira `Say2` (0x38): o 1o caractere escolhe o canal (0 normal, 1 shout, 2 tell, 3 party, 4 pledge, 8 market, 9 ally; 5 é do sistema; 6 e 7 existem na tabela de prefixos).
- **Verbos do `UInput::Exec`** (binding de tecla do `User.ini`): `L2Restart`->0x46, `GetItem` e `TargetChange`->0x04, `WaitModeChange`->0x1D, `MoveModeChange`->0x1C, `Left/RightTurningStart/Finish`->0x4A / 0x4B,
  `PlayerPawnMoveTo` (clique no mundo)->0x04 e, pelo controlador, 0x01. O jogo **não tem** `exec function` do UnrealScript que mande pedido de rede (nos `.uc` extraídos só há câmera e afins): os pacotes saem de C++.
- **Timers e eventos do Engine:** `ValidatePosition` 0x48 a cada 1,2 s com o pawn em movimento (`ALineagePlayerController::Tick`, VERIFICADO), `SendTimeCheckPacket` 0x97, `EnterWorld` 0x03, `Appearing` 0x30 (seção 3.2); `MoveToLocation` 0x01 / 0x5C e `CanNotMoveAnymore` 0x36 / 0x5D do
  controlador do jogador (clique, teclado, parede, degrau); `GetOnVehicle/GetOffVehicle` 0x42 / 0x43 de `APawn::SetBase` (heur.).
- **Automáticos dentro do `Network.dll`:** `RequestPledgeInfo` 0x66 (chamado de `0x100126EF`), `RequestPledgeCrest` 0x68 (`0x10002575`: o brasão `..\PledgeCrest\%d_%d.bmp` não está no disco) e `RequestAllyCrest` 0x88 (`0x10002628`, só com protocolo >= 363) [corrigido pela revisão: `0x1001D9F8` e `0x1001DC42` chamam o `FL2NetNotify` (`[handler+0x48]`: `OnAutoAttackStart` e `OnSocialAction`), não o `UL2NetHandler`; os três sítios restantes carregam `[0x10055308]` e chamam a vtable dele, VERIFICADO].
  Ou seja, o servidor recebe pedidos de brasão **sem ação do jogador** na 1a vez que um clã aparece.
- **Páginas HTML do servidor:** `link ...` -> 0x20 (`NCConsole`), `bypass [-h] ...` -> 0x21 (ou 0x7C no tutorial, defeituoso), `bbs...` -> 0x22/0x57 (quadro).

## 9. O que li e o que não li

| Li | Não li |
|---|---|
| `Network.dll` inteiro desmontado (62.104 linhas): os 170 sítios de envio (resolvidos pelo executor simbólico e conferidos à mão em movimento, listas, `Say2`, `RequestBlock`, `RequestTeleport`, `CharacterCreate`), o empacotador e o leitor, o `Send`/`OnConnect`/`OnUserNetmessage` dos dois sockets, a thread de leitura, as duas cifras (Blowfish: constantes de pi conferidas; **não rodei** o código nem comparei contra um vetor de teste), o `Tick`, a tabela do login, o `KeyPacket`, `PlayOk`, `AuthReconnect`, o ping | O corpo dos manipuladores `0x10001xxx..0x1002Bxxx` do **servidor -> cliente** em detalhe (estão em `Rede_Protocolo_Servidor_Cliente.md`); o instalador de dados dos manipuladores do jogo (`0x1002C100`); a classe `CPingSocket` além de `Connect`/envio |
| Engine.dll: `UGameEngine::Tick` (envio de `SendTimeCheck`, `EnterWorld`), `ScreenFadeDone`, `OnServerClose`/`ContinueServerClose`, o prólogo do `ALineagePlayerController::Tick` (timer de 1,2 s); varredura heurística de chamadas ao `UL2NetHandler` | os sítios do `UInput::Exec` e do `ALineagePlayerController` achados por varredura (não li cada um); `AController::StopMove`, `execPollMoveTo`, `APawn::SetBase`, `stepUp` só pelo nome da função que contém a chamada (o CSV acaba a função no 1o `Catch@`, então o nome pode estar errado); falsos positivos que **eu retirei** da tabela: `UGameEngine::Tick 0x104D55F3`, `Init 0x104D01C7` (objeto `[Engine+0x164]`, não o handler) |
| NWindow: o código-fonte reconstruído (`NConsoleWndNet.cpp` inteiro, `NCChatWnd.cpp` 740..1240, `NConsoleWndActions.cpp` `RunHtmlCommand`, `NCServerSelectWnd`, `NCAuthWnd`, `NConsoleWndLogin.cpp` `ReceiveVersionCheck`, `NCConsole.cpp` `OnMoveTo`), por `grep` os chamadores de cada embrulho | o NWindow.dll em assembly (confio na reconstrução verificada do projeto); janelas que chamam por slot do console (alguns embrulhos sem chamador na fonte têm chamadas por `call [console+off]`: vide 3 deles: alianças, cerco, `AnswerTradeRequest`) |
| Notas do repositório: `F6_ReturnToLogin.md`, `F6_ManufactureCmd.md`, `F6_Craft.md` | `NConsoleWnd.md` (70 KB) só por busca |
| O servidor de referência `L2J_Mobius_C1_HarbingersOfWar`: `ClientPackets.java`, `Encryption.java`, `GameClient.java` (chave), `KeyPacket`, `ProtocolVersion`, `AuthLogin`, `LoginPacketHandler`, os 3 pacotes do login, `Init`/`LoginOk`/`PlayOk`; extração automática da ordem de `readX()` por opcode | o servidor Lucera-C1 do dono (**não encontrado nesta máquina**); outras duas pastas de fontes de servidor (`<server-source>`, outros protocolos) |
| O cliente de teste: `Network.dll` (md5), a listagem de `System - Teste Acessorio`, `L2.log`/`LineageWindow.log` (sem linhas `(Send)`/`(Receive)`: INFERIDO que o `GNetworkLog` não grava em arquivo neste build) | `l2.ini` (Ver212, cifrado: o `[URL] ServerAddr`, `L2VersionCheck` e `CharName` estão aí e **não foram lidos**), `CommandName-e.txt` (cifrado), o `package.dat` (**não existe** na pasta de teste: então o `h` vai 0) |

## 10. Não verificado / perguntas em aberto

1. **Nada foi visto rodando.** Nenhum pacote foi capturado; as ordens de campos vêm do assembly e do REF. Uma captura com `PACKETCOUNTSTART` (`UInput::Exec`) ou um proxy confirmaria o fio byte a byte.
2. **A cifra do jogo e a Blowfish não foram executadas.** Li os dois algoritmos e conferi constantes e o REF; **não** comparei a saída com um vetor de teste. O teste barato: mapear o `Network.dll` e chamar `0x10002430` / `0x100032E0` / `0x10001880`.
3. **`[URL] ServerAddr`, `L2VersionCheck`, `CharName`** estão no `l2.ini` cifrado: o arquivo e a chave não foram usados; o **ini padrão** da `GConfig` (`GetString(Section, Key, File=NULL)`) é INFERIDO como o `L2.ini`.
4. **O servidor Lucera-C1 do dono.** Se as classes `CGModule*` e o `GamePacketHandler` do `server.jar` estiverem em algum disco, vale repetir a diferença da seção 7 contra elas (o REF é Mobius, outro código).
5. **O `d` = 8 do `cxxdh` e os `d3`/`d4` do `LoginOk`**: o cliente manda um 8 constante e guarda `d3`/`d4` do `LoginOk` em `0x100567B4/B8`; **não achei quem usa** `0x100567B4/B8` nem o significado do 8 (o REF ignora tudo depois da senha).
6. **A revisão `0xC621` do `Init`**: o cliente a ignora (VERIFICADO), e o `419` da global `0x10041134` é o único número de protocolo que o cliente usa; não sei se o Lucera-C1 o confere (o REF confere com `PROTOCOL_LIST`).
7. **Quem manda a mensagem `NWM_MOVETO`** (`0xF0000001`, que vira o `0x1A RequestTeleport`): nenhum remetente no NWindow; pode ser outro DLL ou código morto.
8. **Wrappers do console sem chamador na fonte** (vários de clã/aliança/amigos/guerra): pelo menos os de alianças e cerco são chamados por slot do console (`NCDialogBox` e `NConsoleWnd::vfn1xx`); os demais podem ser alcançados do `Engine.dll` ou ser mortos. Eles aparecem na tabela como "nenhum chamador achado".
9. **`SendTimeCheckPacket`**: o que arma `[Engine+0x2D4]` é o manipulador do `0xD4`; eu não revi esse manipulador, só o consumidor no `UGameEngine::Tick`.
10. **Efeito de um `d` de lixo (`FinishRotating`) e da `S` de código (`0x80`, `0x7C`)** num servidor estrito: INFERIDO; o REF ignora o excesso. Se o Lucera-C1 valida o tamanho do corpo, esses três pacotes serão recusados.
11. **Estouro do buffer de recepção** (`> 0x44E8`) e **tamanho `< 2`**: a análise é do código (não vi o jogo cair); o efeito (falha de proteção) é INFERIDO.
12. (removido: tratava de um módulo privado (não publicado).)
13. **O ping da lista de servidores usa um `connect` bloqueante** numa thread: um servidor que aceita e demora, ou um firewall que descarta, prende a thread por até o timeout do Windows; o cliente não limita (INFERIDO; o resultado 9999 só sai em falha imediata).
14. **Contagem:** 154 opcodes distintos no jogo e 170 sítios de envio são a contagem do meu varredor (`call [reg+0x60]`); um envio por outro caminho (chamada direta a `0x10002B70` ou ao `CPingSocket::Send` `0x10002900`) só teria sido visto na busca textual (nenhum, além do ping).

## Revisão independente (Fable)

Feita em 04/10/2026, só leitura: capstone (scripts próprios no scratchpad, nenhum arquivo do repositório fora deste mudou) sobre `orig/Network.dll` (md5 conferido:
`f6d590082999cf527b1505841a070a13`, igual ao do cliente de teste), `orig/Engine.dll` (stubs do ILT resolvidos ao corpo: `UGameEngine::Tick` `0x104D51E0`, `ScreenFadeDone` `0x104CB2B0`,
`OnServerClose` `0x1041F3E0`, `ALineagePlayerController::Tick` `0x105017E0`, `UInput::Exec` `0x104E5160`, `APawn::SetBase` `0x1048B220`) e `orig/WinDrv.dll`; `include/ue2/gen/*.vt.inc`,
`src/nwindow/*`, as notas `F6_ReturnToLogin.md`, `F6_ManufactureCmd.md`; e o REF (`L2J_Mobius_C1_HarbingersOfWar`) lido **com os comentários removidos**. Nada rodou; nenhuma
cifra foi executada contra vetor de teste. **Veredito: GO_WITH_FIXES** (as correções já estão aplicadas no corpo, marcadas "[corrigido pela revisão]"): a metade `Network.dll` da nota
(enquadramento, cifras, fluxo de login, tabela de pacotes, defeitos, tabela de erros) confere no assembly; os erros estavam na coluna **REF** e em dois sítios internos.

### Verificado (com a prova)

- **Importação.** Só `WinDrv.dll` importa `Network.dll`, e só `?DispatchSocketWindowMsg@@YAHPAUHWND__@@IIJ@Z` (pefile sobre `orig/*.dll` e `L2.exe`). `UEngine::NetHandler` em `+0x5C`
  (`include/ue2/UE2Engine.h:802` + `NW_ASSERT_OFFSET`). Global do handler `0x10055308`; socket de jogo `[handler+0x3C]`, de login `[handler+0x40]`, `FL2NetNotify` `[handler+0x48]`, estado `[handler+0x44]`.
- **Empacotador `0x100026E0`.** Índice `letra - 'S'`, tabela de índices `0x100028CC` (`S`->0, `b`/`x`->1, `c`->2, `d`->3, `f`->4, `h`->5, `s`->6, resto 7) e de saltos `0x100028AC`
  (`0x100027C8` S, `0x10002736` b/x, `0x1000281A` c, `0x10002844` d, `0x10002857` f, `0x1000282F` h, `0x1000276D` s, `0x10002881` padrão: **não consome argumento**). `S` nula = 2 zeros
  (`0x10002810`), `s` nula = 1 zero; `b` = (tamanho, ponteiro); estouro do limite -> `xor eax,eax; ret` (`0x100028A0`).
- **`CClientSocket::Send` `0x10002B70`.** `alloca 0x2000`; `EnterCriticalSection(+0x4D2C)`; sai sem nada se formato nulo, socket nulo ou empacotador devolveu 0 (`0x10002BC7`);
  `tamanho = corpo+2` com byte baixo em `[esp+0xC]` e alto em `[esp+0xD]` (`sar eax,8`), imediatamente antes do corpo; cifra via `[vt+0x70](buf, +0x4E78, corpo)` só se `[+0x4E70] != 0`;
  `send(sock=[+0x2C], buf, tamanho, 0)` com retorno ignorado; `+0x496C` e `+0x4970 += tamanho`. `__cdecl`, socket em `[esp+4]`, formato em `[esp+8]`, opcode em `[esp+0xC]`.
- **Cifra do jogo `0x10002430`/`0x10002480`.** Exatamente como a seção 2.2 (`buf[i] ^= key[i&7] ^ buf[i-1]` com o anterior já cifrado; decifra com `in[i-1]`); `cdq; add; adc` = soma
  de 64 bits do tamanho do corpo. **`0x10007F40`**: lê `"cdd"` (`0x1002EF10`), `setne` de `d2` em `[sock+0x4E70]`, `cdq; or edx,0x87546CA1` duas vezes (`0x10007FA3..0x10007FCA`) para
  `+0x4E80/+0x4E84` e `+0x4E78/+0x4E7C`; `PushBack(movsx c)` e `FL2NetNotify[+0x308]` (slot 194 `OnReceiveVersionCheck`); depois `Send(sock, "cSdddd", 8, 0x100567CC, [0x100567C4],
  [0x100567C8], [0x100567AC], [0x100567B0])` em `0x1000806A`. O REF (`Encryption.java`) faz o mesmo com soma de 32 bits; `CRYPT_KEY = 94 35 00 00 A1 6C 54 87` (`GameClient.java:57`).
- **`ProtocolVersion`.** `OnConnect` `0x10002320`: `WSAAsyncSelect(sock, hwnd, 0x503, 0x25)`; `sscanf("$Revision: 419 $", "%cRevision: %d %c")` (strings em `0x1002E8D4`/`0x1002E8E8`);
  `Send(sock, "cd", 0, 419)` em `0x100023C1`; `[0x10041134] = 419`; erro `0x274D` -> só `OutputDebugStringW` (`0x100021C0` = `wvsprintfW`+`OutputDebugStringW`).
- **Blowfish do login.** `P[0] = 0x243F6A88` em `0x10040070`, `S0[0] = 0xD1310BA6` em `0x100400B8`; chave `"[;'.]94-31==-%&@!^+]\0"` em `0x10041138`, `push 0x15` no construtor
  `0x10014120` (vtable `0x10033DD0`, flag `+0x4E70 = 0`); escalonamento `0x10001880` (18 palavras, `idiv` pelo tamanho da chave, bytes combinados com `shl 8`: o padrão). Blocos de 8
  bytes **independentes** (`0x10001850`/`0x10001820`: laço `n>>3` sobre `0x10001000`/`0x10001410`), palavra lida direto da memória (`xor ecx,[0x10040070]` sobre `[edx]`, sem `bswap`): ECB little-endian.
  **Envio `0x100032E0`**: `L=(n+7)&~7`, XOR dos `L/4` dwords gravado em `[L]`, tamanho `L+8`, Blowfish sobre `L+8`; `CAuthSocket::Send` `0x10003230` zera `0x2000` bytes antes
  (`rep stosd` de `0x800`), cabeçalho `L+8+2`. **Recepção `0x10003330`**: `test al,7` sobre `len-2` (passado por `0x10003150`: `lea edx,[ebx-2]`), sem conferir o checksum; o
  bloco entregue é `len-2` bytes copiados de `buf+3` (preenchimento e checksum inclusos).
- **Login.** `RequestAuthLogin` `0x100033A0`: `wcsncpy` (`0x10022974`, cópia de words) de 14 e 16 para `0x100567CC`/`0x100567EA`, `[auth+0x4E70]=0`, `Connect(&[auth+0x484C], 0x83A)`
  em `0x10003411`, `[handler+0x44]=1`, `mov eax,1`. `RequestServerList` `0x10003460`: só com `[auth+0x2C]!=0`, `"cddc", 5, [AC], [B0], 3`. `RequestServerLogin` `0x100034C0`: 3 `Top()` ->
  `0x100567A8/BC/C0`, `"cddc", 2, [AC], [B0], P1`; `NCServerSelectWnd.cpp:361..368` empurra `ServerID, IP, Port` e marca `bRequested`. Manipulador do op 0 `0x100075F0`: lê `"dd"`
  (`0x1002F544`), `[auth+0x4E70]=1` em `0x10007636`, `WideCharToMultiByte(0,0,0x100567CC,14,...,15)` e `(...,0x100567EA,16,...,17)`, `package.dat` via `GFileManager` + `atoi`
  (`0x10022E86`), `"cxxdh", 0, 14, conta, 16, senha, 8, h` em `0x10007706` (e `h=0` em `0x10007744`). Tabela do login `0x10041150` = as 11 entradas da nota; formatos lidos: `Init dd`,
  `LoginFail c`, `AccountKicked d`, `LoginOk dddddddd` (-> `0x100567AC/B0/B4/B8` + 4 locais; `OnStartNotice` = slot 120 `+0x1E0`), `ServerList cc + cddcchhcd`, `ServerFail c`, `PlayFail c`,
  `PlayOk dd` (-> `0x100567C8`, `0x100567C4`; fecha o login, **deixa `+0x4E70 = 1`** em `0x10011A20`, `inet_ntoa([0x100567BC])` + `Connect` `0x10010020`, estado 2), `AccountKicked c`,
  `BlockedAccountWithMsg c + hS`, `Dummy`. REF: `RequestAuthLogin.java` lê a conta em `[1..14]` e a senha em `[15..28]`; `Init` grava `0x0000C621`; `LoginOk` grava `03 d d 0 0 0x3EA 0 0 2`.
- **Recepção e despacho.** Thread `0x1000FD60` (`WaitForSingleObject(+0x494C, 100)`, sai quando `[0x1005530C]==0`; `recv(sock, 0x10051300, 0x4000, 0)`; estados `+0x561C`, buffer
  `+0x844`, esperado `+0x5614` lido como `u16` (`mov cx,[ebx+0x844]`), acumulado `+0x5618`; com `< 2` loga "packet size is under 2 something wrong!" em `0x1000FFD2` **e entra no estado 1**,
  onde `esperado - acumulado` fica negativo e vira o `ecx` do `rep movsd` em `0x1000FE73`: a queda é certa, INFERIDA só quanto ao tipo de falha). `OnPacket` `0x10010240`: decifra
  `(buf+2, +0x4E80, len-2)`; `len-2 == 0` -> "Bad Packet Size" linha `0x1E55` = 7765 e descarta; aloca `len-2` ("NetCmd"), copia `len-3` de `buf+3`, zero no fim, `NetPacket` de 8 bytes,
  `handler[+0x74]` = `AddNetworkQueue` (slot 29). `Tick` `0x10010730`: `DispatchNetworkQueue` (`+0x70`); estado 1: `cmp byte,0xA; jb; mov byte,0xA` e `call [0x10041150+op*4]`, depois
  dois `GMalloc->Free`; estado 2: `0xED` e `imul edx,edx,0x104; call [edx+0x10041180]`; qualquer outro estado volta ao `Dispatch` sem liberar (`0x100109BD`). A tabela `0x10041180` no
  arquivo só tem a entrada 0 (`0x10007F40`); as outras 237 são zero: **é preenchida em tempo de execução** (o instalador `0x1002C100` que a nota não leu).
- **Mensagens.** `DispatchSocketWindowMsg` `0x10022160`: `1` -> `0x10022020` (`GConfig->GetString("URL","ServerAddr",NULL)`, `appToAnsi`, cópia para `+0x484C` dos dois sockets, `hwnd`
  em `+0x28`); `0x502` -> `[vt+0x64]`, `0x503` -> `[vt+0x68]`, socket de login se estado 0/1, senão o de jogo, só com `[sock+0x2C]!=0`; devolve sempre 0. `WinDrv` `0x111077F6`: testa
  `GIsEditor` **antes** da chamada em `0x1110781C`. `OnUserNetmessage` `0x10010370`: tabela `0x100104F8` -> `FD_READ` `ReleaseSemaphore`; `FD_WRITE` e `FD_OOB` `MessageBoxW`
  (`0x10010447`/`0x10010464`); `FD_CLOSE` com erro 0 -> `0x10010406` (só `LeaveCriticalSection`: **nada**); com erro e `wParam == [+0x2C]` -> `FL2NetNotify[+0x230](1)` = `OnServerClose`
  (slot 140); o do login idem com `(0)` em `0x100030D9`. `Connect` `0x10002C50`: `WSAStartup(0x101)`, as 4 `MessageBoxW` ("Can't find winsock.dll", "Socket version is low",
  "INVALID_SOCKET", "Can not resolve hostname"), `WSAAsyncSelect(0x502, 0x10)`, `connect` + `WSAGetLastError != 0x2733` só debug, `InitializeCriticalSection`.
  `AuthReconnect` `0x10003530` e os dois fechamentos `0x10002E30`/`0x100022D0` (`closesocket`, `DeleteCriticalSection`, `+0x4950`, `[0x1005530C]=0`) como na seção 3.3; as duas
  assinaturas de bytes do `ReturnToLogin` batem em `0x10002320` e `0x10010370`. Vtables: `CClientSocket` `0x10033D58` tem **30 slots** (`0x60` Send, `0x64` OnConnect, `0x68`
  OnUserNetmessage, `0x6C` OnPacket, `0x70`/`0x74` cifra; a vtable do `CAuthSocket` começa em `0x10033DD0 = 0x10033D58+0x78`), `CAuthSocket` 32 slots (`0x78` `0x100032E0`, `0x7C` `0x10003330`).
- **Ping.** `EndServerPingList` `0x10011E80` cria a thread `0x10011B30` (`_beginthread`) sobre `[handler+0x70]` se `[+0x840]==0`; a thread percorre 128 entradas de 16 bytes e chama
  `0x10010520` (socket próprio, `connect` **bloqueante**, tempo por `0x10001970`; falha -> `FL2NetNotify[+0x49C](id, 0x270F)` = `SetServerPing`, slot 295) e depois
  `CPingSocket::Send` `0x10002900` com `("cd", 0, -2)` em `0x1001065D..0x10010667`, sem cifra. A vtable `0x100310F0` do `CPingSocket` **não** foi conferida.
- **Tabela de pacotes.** Varredor próprio (toda `call [reg+0x60]` em todas as exportações + os 6 sítios internos): **170 sítios, 154 opcodes distintos no jogo**, e **todos** os
  pares (opcode, formato) da seção 1 batem, inclusive os constantes (`RequestAddTradeItem` d=1, `RequestShowboard` d=1, `RequestGMCommand` 3o d=0, `Say2` tipo 2 -> `cSdS`, `RequestBlock`
  `cd`/`cdS`, `RequestTeleport` só modos 2 e 3, `0x39` subopcodes 0/1/3/2). Lidos à mão: `FinishRotating` `0x10003B90` (só `push esi`=A1 antes de `"cdd"`: o 2o `d` é o `esi` salvo),
  `RequestPetitionCancel` `0x10006350` (sem prólogo: a `S` é o endereço de retorno), `RequestTutorialPassCmdToServer` `0x10004560` (`Top()` descartado; a `S` é o `esi` salvo),
  `NetPing` (`[handler+0x78]` zerado depois), `SendTimeCheckPacket` (2 `Top()`), `RequestCharacterCreate` (`ret 0x58`; offsets `+0x28, +0x30, +0x2C, +0x40, +0x34, +0x3C, +0x48, +0x38,
  +0x44, +0x4C, +0x50, +0x54` = race, sex, classId, INT, STR, CON, MEN, DEX, WIT, hair, color, face com `Stats` em `+0x34` conforme `UE2Engine.h:1114`), `Say2`, `RequestMagicSkillUse`
  (`movzx al` no `shift`), `ValidatePosition`, `MoveToLocation` (`sete al` sobre A9; A9!=0 só muda a procura de veículo em `[A1+0x50]`), `CanNotMoveAnymore` (`[esp+0x44]` = A6 -> `jne`
  = nada sai), `ReviveReply` (3 `Top()`, 1 no fio), `EnterWorld` (`L"narcasse"` `0x10035954`, `User` novo, `[URL] CharName`), `RequestPledgeInfo` (`GetRequestedPledgeID` `+0x94`),
  o verificador de brasões `0x10002500` (`cmp [0x10041134],0x16B`), `RequestRemainTime` (só `c`). Listas: formatos por item `ddd`, `dd`, `dd`, `dd`, `ddd`, `ddd`, `dhdd` (`movsx`),
  `ddhdd` (`movsx`), `dd`; buffer `0x2400`; `N` no fio = o `P` lido (`0x1000412F`).
- **Engine.dll.** Todos os sítios citados são `call` ao slot certo sobre `[x+0x5C]`: `0x104D559F` (`+0x154`, `[0x109F53DC]=0` antes e `[0x109F5A44]=1` depois; o contador em 0 exige
  `GIsL2NetLogin` e chama `[Engine+0x164]->+0x9C`, em 3 chama `[Engine+0x2C]->+0x1EC` = slot 123 com 1, `>= 4` manda), `0x104D52FB` (`+0x344`, `ParamStack` com `[Engine+0x2D8]` e
  `[+0x2DC]`, guardado por `NetHandler != 0` e console no estado 6, `[+0x2D4]=0` depois), `0x104CB354` (`+0xF0`, com arg1 != 0 e arg2 == 1), `0x104E570E` (`+0x27C`), `0x104E5B2C`
  e `0x104EA32C` (`+0xD4`), `0x104EA41C` (`+0xD0`), `0x104EA519` (`+0x134`), `0x104E6148` (`+0xE8`, 0), `0x104E603C` (`+0xEC`, 1), `0x104EB1A4` (`+0x108`, `[esi+0x2D8]`, -1),
  `0x104EB28E` (`+0x10C`), `0x104EACAF` (`+0xC8`), `0x1048B3A4`/`0x1048B2CC` (`+0xFC`/`+0x100` em `APawn::SetBase`). `[0x10797598] = 1.2f`; `ALineagePlayerController::Tick` usa
  `GIsL2NetLogin`, `[pawn+0x2E0]`, `fadd [0x10797598]` e dois `call [+0x104]` (`0x1050197A`, `0x105019B3`).
- **NWindow.** `ReceiveVersionCheck` (padrão 1, `SendLogOutPacket` + `MSG_ProtocolQuit`), `ReturnToLogin` `0x10045EF0`, `NCServerSelectWnd::OnCancel` -> `AuthReconnect`,
  `AddServerPingList(ServerID-1, IP, Port)`, `RunHtmlCommand` `0x10049750` (`TCHAR Bypass[1024]`, `-h`, `$nome`, o viewer de tutorial -> `0x7C`), `UseShortCut` `0x100561F0`,
  `MoveToLocation` `0x1004D250` (modos 2/3), `PartyItemDistribution`, `Item->BodyPart`, `RequestServerList` em `NCNoticeWnd` (3) e `NCWarningWnd` (2), `ManufactureCmd` 120/100 ms.
- **REF.** `ClientPackets.java` trata 127 opcodes (um deles, `0x69`, com manipulador `null`); todos têm remetente no cliente; os 16 opcodes `0x05..0x07, 0x13, 0x18, 0x19, 0x3A, 0x3B,
  0x3D, 0x40, 0x4C, 0x54, 0x56, 0x5A, 0x65, 0x8D` não saem do cliente nem entram no REF (a lista da seção 6.4 confere). `Say2` lê a 2a `S` só com tipo 2; `RequestBlock` lê o nome só
  nos tipos 0 e 1; `ProtocolVersion` com `-2` desconecta; `KeyPacket` = `c, 8 bytes, d serverId, d 1`; `MoveToLocation` 7 `d`; `Action`/`AttackRequest` `ddddc`; `RequestPetition` `Sd`;
  `MultiSellChoose` `ddd`; `RequestNetPing`, `EnterWorld` e `RequestPetitionCancel` não leem nada. Listas: `BATCH_LENGTH` 12/8/8/8/12/12/12/8 e a conferência **estrita**
  `N*BATCH_LENGTH != remaining()` (menos `SetPrivateStoreListBuy`, `>`, com `BATCH_LENGTH = 12` para itens de 14 B, inofensivo).

### Refutado (corrigido no corpo)

1. **`0x14`, `0x24`, `0x73`, `0x8A`: "o REF lê mais".** As leituras extras (`_ctrlPressed`, `_pledgeType`, `readInt/readLong`, `readLong/readInt`) estão **comentadas** no REF; a
   extração automática da nota contou linhas comentadas. Formatos iguais.
2. **`0x9D RequestSkillCoolTime` "="**: a entrada do REF está comentada; é "ausente" (28 ausentes, não 27).
3. **`0x6E RequestGMCommand` "="**: o REF lê `S d`; o 3o `d` (0) do cliente sobra.
4. **`0x96 SendPrivateStoreBuyBuyList` "= (lista)"**: o REF lê 20 B por item (`d d h h d d`) e exige `N*20 == restante`; o cliente manda 18 B (`d d h d d`): o REF **recusa**.
5. **`0x66`/`0x68`: os sítios `0x1001D9F8` e `0x1001DC42`** chamam `[handler+0x48]` (o `FL2NetNotify`: slots 98 `OnAutoAttackStart` e 100 `OnSocialAction`), não o `UL2NetHandler`.
   O varredor da nota confundiu as duas vtables (os offsets `0x188`/`0x190` coincidem). Ficam só `0x100126EF`, `0x10002575` e `0x10002628`.
6. **Seção 6.3 "um opcode desconhecido ... cai em `onUnknownPacket()`"**: no REF `GamePacketHandler.handlePacket` devolve `null` (ignorado, conexão mantida); `onUnknownPacket()` é
   o Lucera do dono.
7. Consequências nas contagens: seção 0 item 4 (27/11 -> 28/9) e seção 7 (as duas listas).

### Não verificado / observações

- As duas cifras não foram executadas contra vetor de teste (igual à nota); o Blowfish foi lido só até o nível das constantes, da independência dos blocos e da ordem dos bytes.
- A vtable `0x100310F0` do `CPingSocket`; os sítios "heur." do `Engine.dll` que a nota já marca; `ScreenFadeDone`: as duas condições (`arg1 != 0`, `arg2 == 1`) conferem, mas **não vi**
  a checagem "pawn do jogador existente" da seção 3.2 item 4 (o envio em `0x104CB354` é incondicional depois delas).
- Pela conferência estrita do REF (`N*tamanho != restante`), a peculiaridade "N = P lido" das listas (seção 4) faz o REF **recusar** uma lista que estourou `0x2400` bytes.
- Tudo o que depende do servidor Lucera do dono continua NÃO VERIFICADO (ele não está nesta máquina).

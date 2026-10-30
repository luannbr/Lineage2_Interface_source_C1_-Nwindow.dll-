# Cores de chat do Essence 542 — referência para o futuro

Levantado em 29/09/2026 a pedido do dono ("verifica no Essence quais cores temos novas"). **Decisão do
dono: o C1 fica como está** (o `ChatColors` atual: COMMANDCHANNEL_ALL, COMMANDCHANNEL_COMMANDER e HERO_VOICE
nas cores do **Interlude**, ver `F6_ChatColors.md`). As cores do Essence ficam só documentadas aqui; nada
disto está implementado no cliente nem no servidor.

## De onde vêm

- Cliente: `<essence-client>/LINEAGE2-ESSENCE-SAMURAI-CROW-541-EU-IL/system.il542`
  (`NWindow.dll` de 2025-11-27, 32 bits, 19 MB).
- Fonte da interface: `<essence-client>/L2Emu-IFC-542-main/L2Emu-IFC-542-main`
  (`InterfaceClassic/Classes/ChatWnd.uc`, `NWindow/Classes/UIScript.uc`, `UIEventManager.uc`).
- **As cores não são constantes no código.** `GetChatColorByType(int Type)` e `GetChatSubColorByType(int
  Type)` são nativas do `UIScript` (`UUIScript::execGetChatColorByType`, `0x109bb550`), que consultam um
  mapa (`NCChatChannelManager::GetChannelColor`, `AddChannelColor`) que o `XMLChatChannelDefinition`
  enche ao carregar os dados da interface (a tag `ChatChannelDefinition`, com `Color` e `SubColor` por
  canal). No arquivo, o bloco está perto do fim do `InterfaceClassic.xdat` (offset 7631670) e do
  `Interface.xdat` (6775967), **idêntico nos dois**.
- Formato do bloco: `int32` 30 (a contagem) e 30 registros de 12 bytes: `int32` tipo (o `SayPacketType`),
  cor e subcor, cada uma 4 bytes B,G,R,A (o dword lido é `0xAARRGGBB`, como as cores do C1).
- Reprodução: `python tools/chat/essence_chatcolors.py "<caminho>/InterfaceClassic.xdat"`.
- Uso no script (`ChatWnd.uc`, `HandleChatmessage`): `Color = GetChatColorByType(SayType)` e `subColor =
  GetChatSubColorByType(SayType)` vão para `AddStringToChatWindow(...)`: a cor é a do **nome / prefixo** da
  linha e a subcor a do **texto da mensagem** (mais clara). O chat do C1 tem uma cor por linha
  (`NCChatListBox::AddString(Text, Color, -1)`): só a cor principal caberia.

## A tabela completa (id = `SayPacketType` = o `ordinal` que o servidor manda no `Say2`)

Cores em RGB hexadecimal (`RRGGBB`). "C1" = o `NConsoleWnd::Say2` de fábrica do C1 (o que não está na
lista sai branco, `FFFFFF`, como chat normal); "IL" = Interlude (`NWindow.dll` do LuceraTestPatch, build
2007-06-19); "Lucera" = o nome no `ChatType.java` do servidor (só existe até o id 22).

| Id | Essence (`SayPacketType`) | Lucera `ChatType` | C1 | IL | **Essence cor** | Essence subcor |
|---|---|---|---|---|---|---|
| 0 | NORMAL | ALL | FFFFFF | DCDCDC | **DCDCDC** | E7E7E7 |
| 1 | SHOUT | SHOUT | FF7200 | FF7200 | **FF7200** | E4A166 |
| 2 | TELL | TELL | FF00FF | FF00FF | **DA64FC** | EB8FEB |
| 3 | PARTY | PARTY | 00FF00 | 00FF00 | **00FF00** | 66E466 |
| 4 | PLEDGE | CLAN | 7D77FF | 7D77FF | **9999FC** | D6C0F4 |
| 5 | SYSTEM | GM | (branco) | (DCDCDC) | **B09B79** | B09B79 |
| 6 | USER_PET | PETITION_PLAYER | 80FFFF | 80FFFF | **DCDCDC** | DCDCDC |
| 7 | GM_PET | PETITION_GM | 80FFFF | 80FFFF | **80FFFF** | 80FFFF |
| 8 | MARKET | TRADE | EAA5F5 | EAA5F5 | **FFAEE1** | F8D8EC |
| 9 | ALLIANCE | ALLIANCE | 77FF99 | 77FF99 | **77FF99** | AEF1BE |
| 10 | ANNOUNCE | ANNOUNCEMENT | 80FFFF | 80FFFF | **80FFFF** | 80FFFF |
| 11 | CUSTOM (msg de sistema) | SYSTEM_MESSAGE | (branco) | FF7200 | **FF7200** | FF7200 |
| 12 | L2_FRIEND | L2FRIEND | (branco) | (DCDCDC) | **DCDCDC** | DCDCDC |
| 13 | MSN_CHAT | MSNCHAT | (branco) | (DCDCDC) | **DCDCDC** | DCDCDC |
| 14 | PARTY_ROOM_CHAT | PARTY_ROOM | (branco) | DCDCDC | **DCDCDC** | DCDCDC |
| 15 | COMMANDER_CHAT | COMMANDCHANNEL_ALL | FF9695 (*) | FF9695 | **FF9695** | FF9695 |
| 16 | INTER_PARTYMASTER_CHAT | COMMANDCHANNEL_COMMANDER | FFF8B2 (*) | FFF8B2 | **FFF8B2** | F8F8D6 |
| 17 | HERO | HERO_VOICE | 408CFF (*) | 408CFF | **15BAFF** | 15BAFF |
| 18 | CRITICAL_ANNOUNCE | CRITICAL_ANNOUNCE | (branco) | 00FFFF (sem nome) | **00FFFF** | 00FFFF |
| 19 | SCREEN_ANNOUNCE | SCREEN_ANNOUNCE | (branco) | (DCDCDC) | **FF9695** | FF9695 |
| 20 | DOMINIONWAR | BATTLEFIELD | (branco) | (DCDCDC) | **FFCC00** | FFCC00 |
| 21 | MPCC_ROOM_CHAT | MPCC_ROOM | (branco) | (DCDCDC) | **DCDCDC** | DCDCDC |
| 22 | NPC_NORMAL | NPC_NORMAL | (branco) | (DCDCDC) | **9B9B9B** | 9B9B9B |
| 23 | NPC_SHOUT | — | (branco) | (DCDCDC) | **9B9B9B** | 9B9B9B |
| 24 | FRIEND_ANNOUNCE | — | (branco) | (DCDCDC) | **00AAA4** | 00AAA4 |
| 25 | **WORLD** | — | (branco) | (DCDCDC) | **F45454** | F45454 |
| 26 | PLEDGE_INRAIDSERVER | — | (branco) | (DCDCDC) | **DCDCDC** | DCDCDC |
| 27 | ALLIANCE_INRAIDSERVER | — | (branco) | (DCDCDC) | **DCDCDC** | DCDCDC |
| 28 | CASTLEWAR_PLEDGE_COMMAND_MSG | — | (branco) | (DCDCDC) | **C193FF** | C193FF |
| 29 | WORLD_INRAIDSERVER | — | (branco) | (DCDCDC) | **EEAA22** | DEBC79 |

(*) já implementado no C1 (`src/features/ChatColors.cpp`), com a cor do Interlude. Entre parênteses: o que o
tratador faz por omissão (o `else` final), não uma cor própria do tipo.

## O que o Essence tem de novo

- **Cores de canal que o Interlude não tem:** 20 DOMINIONWAR/BATTLEFIELD `FFCC00`, 22 e 23 NPC `9B9B9B`,
  24 FRIEND_ANNOUNCE `00AAA4`, **25 WORLD `F45454`** (vermelho: o chat mundial, o equivalente ao VIP chat
  do servidor), 28 CASTLEWAR_PLEDGE_COMMAND_MSG `C193FF`, 29 WORLD_INRAIDSERVER `EEAA22`.
- **Cores que o Essence reajustou:** TELL `FF00FF` → `DA64FC`, CLAN `7D77FF` → `9999FC`, TRADE `EAA5F5` →
  `FFAEE1`, HERO `408CFF` → `15BAFF`.
- **Subcor** por canal (cor do texto, mais clara que a do nome) e o "chat normal" `DCDCDC`.
- O Essence tem ainda filtros de aba por canal (`chatfilter.ini`: normal, party, shout, market, pledge,
  tell, ally, worldUnion, hero, union, nonpcmessage, worldChat).

## Se um dia for implementar (plano, nada feito)

**Servidor (Lucera)** — só o servidor manda o tipo, e o VIP chat usa `WorldChatMsgColor`:

1. `java/l2/gameserver/network/l2/components/ChatType.java`: acrescentar **no fim, depois de
   `NPC_NORMAL`** (o `ordinal()` é o id que vai no `Say2`; nunca reordenar): `NPC_SHOUT` (23),
   `FRIEND_ANNOUNCE` (24), `WORLD` (25), `PLEDGE_INRAIDSERVER` (26), `ALLIANCE_INRAIDSERVER` (27),
   `CASTLEWAR_PLEDGE_COMMAND_MSG` (28), `WORLD_INRAIDSERVER` (29). Conferir quem usa `ChatType.VALUES` e o
   `switch` do `Say2C.java` (um tipo de 23 em diante vindo do cliente não pode ser aceito como chat).
2. `server.properties`: `WorldChatMsgColor = WORLD` (o `Config.java` lê com `ChatType.valueOf`); o
   comentário da linha lista os nomes válidos, atualizar.
3. Um cliente sem a DLL nova mostra o texto branco (cai no `else` do `Say2`).

**Cliente (NWindow_Decomp)**:

1. `src/features/ChatColors.cpp`, `ChatColors_Find`: acrescentar 19, 20, 22, 23, 24, 25, 28, 29 (o 18 do
   Interlude tira o nome da linha; decidir antes). A linha continua entrando como chat normal (aba All).
2. Se quiser as cores reajustadas do Essence para tell, clan, trade ou hero: trocar as constantes em
   `NConsoleWnd::Say2` (`src/nwindow/NConsoleWndEvents.cpp`) e em `NpcSay`, ou em `ChatColors.cpp` para o
   hero.
3. Duas cores por linha (nome e texto) exigem que o `NCChatListBox` aceite cores por trecho: não tem hoje.
4. Teste: só no jogo (o ciclo `tools/cycle.py` não desenha o chat).

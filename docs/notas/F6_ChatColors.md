# Fase 6 — ChatColors (as cores dos tipos de chat que o C1 desenha em branco)

Origem: pedido do dono em 29/09/2026: o VIP chat global do servidor (Lucera) é mandado como um tipo de
chat e é o **tipo** que dá a cor (`WorldChatMsgColor` no `server.properties`: ALL, SHOUT, TELL, PARTY,
CLAN, TRADE, ALLIANCE, COMMANDCHANNEL_ALL, COMMANDCHANNEL_COMMANDER ou HERO_VOICE); no C1 quase todos
saem brancos. Não existe no NWindow_C1. Aqui: `include/features/ChatColors.h`,
`src/features/ChatColors.cpp`.

## Como a cor chega no chat

O protocolo não tem cor por mensagem: o `Say2` do servidor (`s2c/Say2.java`) manda o **tipo** (o
`ordinal` de `ChatType`) e o cliente decide a cor, em `NConsoleWnd::Say2`
(`src/nwindow/NConsoleWndEvents.cpp`, `0x10054a30`). O C1 conhece só estes tipos, com estas cores:

| Tipo | Id | Cor (ARGB) |
|---|---|---|
| ALL | 0 | `FFFFFFFF` (branco; no Interlude `FFDCDCDC`) |
| SHOUT | 1 | `FFFF7200` |
| TELL | 2 | `FFFF00FF` (também põe o nome na lista de tells) |
| PARTY | 3 | `FF00FF00` |
| CLAN | 4 | `FF7D77FF` |
| TRADE | 8 | `FFEAA5F5` |
| ALLIANCE | 9 | `FF77FF99` |
| PETITION 6 / 7, ANNOUNCEMENT 10 | | `FF80FFFF` |

Todo **outro** tipo cai no `else`: branco, e a linha entra como chat normal. É o que acontecia com o
VIP chat no padrão do servidor (`COMMANDCHANNEL_COMMANDER`, id 16).

## O que esta feature acrescenta

As cores do Interlude para os tipos que o servidor oferece e o C1 não tinha (lidas no
`NConsoleWnd::Say2` do `NWindow.dll` do cliente Interlude do Lucera, build de 2007-06-19, `0x10163d90`):

| Tipo | Id | Cor |
|---|---|---|
| COMMANDCHANNEL_ALL | 15 | `FFFF9695` (rosa-claro) |
| COMMANDCHANNEL_COMMANDER | 16 | `FFFFF8B2` (amarelo-claro) |
| HERO_VOICE | 17 | `FF408CFF` (azul) |

A linha continua entrando como chat normal (aba All, filtro Normal, sem efeito colateral): só a cor
muda. Em `NConsoleWnd::Say2`, o `else` final consulta `nwf::ChatColors_Find`; só no build do L2.

## O que os clientes do disco mostram

| Cliente | `NWindow.dll` | cores de 15 / 16 / 17 |
|---|---|---|
| C1 | 2004-06 | nenhuma |
| C3 (`1cc_3rise_of_darkness`) | 2005-04 | nenhuma |
| C4 (`C4_Client`) | 2006-01 | só a 17 (hero) |
| Interlude (`LuceraTestPatch`) | 2007-06 | as três |
| Essence 542 | 2025-11 (19 MB) | as cores não estão no código: vêm dos dados da interface (`ChatColors_Essence.md`); 15 `FF9695`, 16 `FFF8B2`, 17 `15BAFF` |

Outros tipos do Interlude que o C1 não trata e que ficaram de fora (o servidor os tem): PARTY_ROOM (14,
`FFDCDCDC`), SYSTEM_MESSAGE (11, o Interlude o trata como shout laranja) e CRITICAL_ANNOUNCE (18,
`FF00FFFF`, sem nome).

## O Essence 542 (só documentado)

O Essence guarda as cores nos dados da interface (não no código) e tem 7 cores de canal que o Interlude
não tem (entre elas a do WORLD, `F45454`, o chat mundial) e 4 reajustadas (tell, clan, trade, hero), além
de uma subcor por canal. A tabela completa, de onde ela vem e o plano para implementar um dia estão em
**`docs/notas/ChatColors_Essence.md`**. **Decisão do dono (29/09): o C1 fica como está**, com as três
cores do Interlude acima; as do Essence não foram implementadas.

## Usar no servidor

`server.properties`: `WorldChatMsgColor = HERO_VOICE` (azul), `COMMANDCHANNEL_COMMANDER` (amarelo-claro),
`COMMANDCHANNEL_ALL` (rosa-claro) ou um dos que o C1 já colore (TRADE rosa, ALLIANCE verde-água, SHOUT
laranja, CLAN azul-violeta, PARTY verde). Evite TELL (põe o nome na lista de tells do jogador) e ALL
(branco). Um cliente sem a DLL nova mostra o VIP chat branco, como hoje.

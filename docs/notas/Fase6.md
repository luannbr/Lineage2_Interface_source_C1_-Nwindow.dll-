# Fase 6 — os recursos do LineageWindow dentro do nosso NWindow.dll

Começada em 27/09/2026, depois do teste do standalone no jogo (o usuário: "parece que tá tudo ok").
O cliente em uso (`System - Copia`) roda o `NWindow.dll` original com uma importação a mais, o
`LineageWindow.dll` do projeto `NWindow_C1`, que põe os recursos por fora: vtables e thunks
remendados em endereços do original, detours, IAT. Aqui eles viram source.

## O que entra (NWindow_C1 HEAD `4e56326`, o que o `LineageWindow.dll` do usuário roda)

| Recurso | NWindow_C1 (`src/inject/`) | Doc do NWindow_C1 | Trabalhador |
|---|---|---|---|
| Combat log (painel de sistema/combate sobre o chat) | `combatlog/CombatLog.cpp`, `CombatLog.h` | `COMBAT_LOG.md`, `SESSION_2026-07-16_COMBATLOG_UI.md` | status |
| BuffTimer (contagem na barra de buffs) | `BuffTimer.cpp` | `SESSION_2026-07-19_BUFFTIMER.md`, `NCAbnormalStatusWnd_PORT.md` | status |
| ManufactureCmd (`/dwarvenmanufacture`, ação 37, ação 52 do servitor) | `ManufactureCmd.cpp` | `MANUFACTURE_37_*.md`, `SERVITOR_RETURN_ACTION_52.md`, `REQUEST_ACTION_USE_37_SERVER.md` | menus |
| MinimapHotkey (Alt+M / M) | `MinimapHotkey.cpp` | `MINIMAP_HOTKEY.md` | menus |
| AutoShot (soulshot/spiritshot automático, estilo C3) | `AutoShot.cpp` | `AUTO_SHOT.md` | menus |
| CacheCleaner (FLUSH + OBJ GARBAGE periódicos) | `CacheCleaner.cpp` | — | menus |
| Balão de manufatura (loja privada tipo 5) | `CraftBalloon.cpp` | `CRAFT_BALLOON_ACTIVATION.md`, `FEATURE_CRAFT_BALLOON.md`, `MANUFACTURE_BALLOON_SHOWS_BUY.md` | clan |
| Janelas nativas de craft estilo C2 (loja, livro de receitas, árvore) | `craft/*` (CraftUi, CraftWindows, CraftSession, CraftData, NwUi) | `CRAFT_UI_C2.md`, `RECIPE_UI_C2_SPEC.md`, `NWINDOW_NATIVE_WINDOWS.md` | clan |
| WindowFix (modo janela: moldura móvel, mouse da UI, filtro de clique...) | `WindowFix.cpp` | `WINDOW_FIX.md` | lobby |
| ReturnToLogin (volta ao login em vez de lista morta / jogo fechado) | `ReturnToLogin.cpp` | `C1_DISCONNECT_RE.md` | lobby |

Fica de fora: `WindowBridge`, `TestWindow`, os ports do `NConsoleWnd` (`NConsoleWnd_Tick/Destroy/Dtor`,
`NConsole_LocalVtable.gen.*`) e o `IltHook` usado dentro do NWindow — eram só o jeito de entrar no
código original, que aqui é nosso. As mudanças de craft **não commitadas** no NWindow_C1 (arrastar
entre listas, 20/09) não estão no DLL do usuário e ficam para depois.

### Além do NWindow_C1

| Recurso | Origem | Código |
|---|---|---|
| Discord Rich Presence ("Jogando L2GIRAN.COM", linha de detalhes, tempo de jogo, botões Site e Discord) | pedido do dono em 29/09 | `DiscordPresence.cpp` |
| ChatColors (as cores do Interlude para os tipos de chat que o C1 desenha em branco: command channel e hero voice; o VIP chat do servidor) | pedido do dono em 29/09 (`WorldChatMsgColor` do Lucera) | `ChatColors.cpp` (nota `F6_ChatColors.md`) |
| ItemHoverTip (a dica do item sob o mouse: caixa escura com o símbolo do grade, como nas crônicas recentes) | pedido do dono em 29/09 (capturas do `Interface_542`; visual do C3 em diante) | `ItemHoverTip.cpp` (nota `F6_ItemHoverTip.md`) |

A reconstrução do `Engine.dll` é um projeto separado, que não faz parte deste repositório.

O `L2DiscordPresence` (fork GPLv3 do `zikdoz`) é um DLL à parte que um `ogg.dll` falso carrega, com a
Discord Game SDK e um `presence.ini` lido na compilação. Aqui o recurso é código nosso no `NWindow.dll`
(o `Engine.dll` é o binário da NCsoft, que só remendamos; o nosso DLL é o que já leva os recursos), sem
código copiado de lá, sem SDK: fala direto com o pipe local do Discord (`\\.\pipe\discord-ipc-0..9`:
HANDSHAKE, `SET_ACTIVITY`, PING/PONG) numa thread própria, que reconecta quando o Discord abre ou
reinicia. O que o `presence.ini` trazia está compilado (id `1154460536088567829`, detalhes
`Play L2GIRAN.COM chronicle 1`, botões `https://l2giran.com` e `https://discord.gg/uzWj8M4gcm`).
Sem imagem nomeada: o único Art Asset da aplicação é o logo de outro servidor (l2skelth), e sem
`large_image` o Discord mostra o ícone da aplicação. Só no `L2.exe` (smoke e difftest não conectam);
`[Presence] Enable=0` no registro desliga. Teste fora do jogo: `tools/presence/presence_test.py` (passo
`l2: discord presence` do ciclo), contra um Discord falso num pipe de nome próprio.

As proteções do cliente (anti-bot e id do computador) são um módulo privado e **não fazem parte
desta source publicada**.

## Regras

- **Dois builds, um código.** O alvo `NWindowL2` (`build/l2/Release/NWindow.dll`) compila as mesmas
  fontes com `NW_FEATURES=1` mais `src/features/`. O `NWindowStandalone` (fiel) e o modo gancho não
  enxergam nada disto: continuam sendo onde o verificador, o `verify_standalone`, o difftest e o
  smoke comparam com o original. O ciclo prova que nada vazou.
- **Código do recurso** em `src/features/<Recurso>.cpp` (+ `include/features/<Recurso>.h`). No código
  decompilado, só o ponto de chamada, num bloco `#if NW_FEATURES ... #endif` com o comentário
  `// phase 6: <Recurso> (NWindow_C1 src/inject/<arquivo>)`. O código fiel continua lendo igual.
- **Direto em vez de remendo.** O que o LineageWindow fazia remendando o NWindow (slot de vtable,
  thunk, detour de função do NWindow, IAT do NWindow, campo lido por offset do original) vira chamada
  ou mudança direta no nosso código, com os campos declarados. Remendo em tempo de execução só sobra
  para outros módulos (`WinDrv.dll`, `Network.dll`, partes do `Engine.dll` que não passam pelo
  NWindow): conferir no install que o alvo é o esperado e, se não for, desligar o recurso com log.
- **Mesmo comportamento e mesmo protocolo.** Bypasses (`mfg_*`, `mfg_ui`, `ss_ui`...), as mensagens
  de dados `L2W mfg` v1, os ids de ação (37, 52), os textos. O servidor (Lucera C1) não muda.
- **Configuração sem arquivo** (pedido do dono, 28/09: "o combatlog.ini quero que fique salvo dentro do
  dll, o usuário só vai alterar essas config dentro do jogo; o mesmo com LineageWindow.ini; também
  desativar os logs"). As seções e chaves continuam as do NWindow_C1 (`nwf::IniInt`, `IniString`,
  `IniWrite*` em `include/features/NwFeatures.h`), mas nada fica na pasta: o que o `LineageWindow.ini`
  trazia está compilado (`GBuiltIn` em `src/features/NwFeatureConfig.cpp`, os valores do dono); o que o
  jogador muda no jogo (posição da janela, das janelas de craft, filtros do combat log, seção
  `CombatLog`) vai para o registro, `HKCU\Software\Lineage2C1\NWindow\<seção>` (um DLL carregado não
  se regrava). Leitura: registro, depois `GBuiltIn`, depois o padrão do recurso. O log
  (`LineageWindow.log`) só existe com a opção de CMake `NW_FEATURE_LOG=ON`
  (desligada); as tags continuam (`wfix`, `craftui`, `autoshot`, `chatlog`...). O `NWindowDecomp.ini` é
  só do modo gancho.
- **Testes** só fora do jogo (build, smoke). O teste no jogo é no `System - NWindow Teste`, depois que
  o coordenador instalar, com autorização do usuário.

## Estado

| Recurso | Estado |
|---|---|
| Base (`NWindowL2`, `NwFeatures.h`, ini/log, passos `l2:` do ciclo) | feita (coordenador, `887115d`) |
| ManufactureCmd, MinimapHotkey, AutoShot, CacheCleaner | no master (menus, `fc27053`; notas `F6_*.md`) — falta teste no jogo |
| Combat log, BuffTimer | no master (status, `029b887`; notas `F6_CombatLog.md`, `F6_BuffTimer.md`) — falta teste no jogo |
| Janelas de craft, balão de manufatura | no master (clan + integração do lobby, `e4d2ce2`; notas `F6_Craft.md`, `F6_Infra.md`) — 8 `UClass` só do L2 (`tests/standalone/l2_extras.csv`, exports 181..188) — falta teste no jogo |
| WindowFix, ReturnToLogin | no master (lobby, `d7577c5`; notas `F6_WindowFix.md`, `F6_ReturnToLogin.md`, `F6_Infra.md`) — falta teste no jogo |
| Discord Rich Presence | feito (29/09, não commitado); teste fora do jogo passa — falta ver no Discord de verdade |
| ChatColors | feito (29/09, não commitado); compila e o ciclo passa — falta ver no jogo com `WorldChatMsgColor` mudado |
| ItemHoverTip | feito (29/09, não commitado); compila e o ciclo passa — falta ver no jogo (medidas estimadas de uma captura, `F6_ItemHoverTip.md`) |

Início: `nwf::OnDllMain` (`src/features/NwFeatureStartup.cpp`), chamado pelo nosso `DllMain` no mesmo
`LoadLibrary` em que rodava o do `LineageWindow.dll`; ordem do NWindow_C1: CraftBalloon, AutoShot,
MinimapHotkey, ReturnToLogin, ManufactureCmd, BuffTimer, CacheCleaner, WindowFix, CraftUi;
depois do WindowFix, a thread do Discord Rich Presence (não bloqueia o carregamento).

Por quadro: o `NConsoleWnd::Tick` chama `nwf::OnConsoleTick` (`src/features/NwFeatureTick.cpp`) antes do
`UL2ConsoleWnd::Tick`, com a ordem do NWindow_C1 — combat log, balão, craft UI, AutoShot, ReturnToLogin,
fila de bypass, CacheCleaner — sob um tratador de exceção só, como o Tick do NWindow_C1 (uma falha num
recurso perde aquele quadro dos recursos e o Tick original segue). Uma fila de bypass só
(`nwf::SendBypassToServer`, 120 ms, `features/ManufactureCmd.h`) para todos os recursos.

Para o teste no jogo: só o `NWindow.dll` do L2 (desde 28/09 nenhum `.ini` nem `.log` ao lado dele).

Correção fora do NWindow_C1 (anotada): a fila de bypass aceita 1024 caracteres (`0a4c9a0`); com 200,
como no NWindow_C1, o `mfg_start` de uma loja com uns 15–20 ofertas não saía.

Vistos e deixados como no NWindow_C1 (o cliente do usuário se comporta igual):
- o par "label" do balão nunca instala no jogo: o `L2.exe` fica em `0x10900000`, dentro da faixa do
  Engine, então o Engine é sempre relocado e o endereço absoluto que o install confere não bate; os
  pares de tipo e de cor instalam e a cor já troca o texto (`F6_Infra.md`);
- craft e combat log leem cursor/foco pelo `GViewportHWnd`, não pela API do WindowFix: igual com
  `[Window] Enable=1` (o padrão e o do usuário), difere só com `Enable=0`.

Revisão da rodada 12 (conferente) corrigida em `c4bde06`: `LineageWindow.ini`/`.log` pelas chamadas
Unicode; mover atalho sem o som de pegar item; o anúncio do AutoShot/craft sobrevive à tela de
carregamento; a volta ao login só se dá por instalada com os dois slots; o bloco por quadro sob um
tratador só.

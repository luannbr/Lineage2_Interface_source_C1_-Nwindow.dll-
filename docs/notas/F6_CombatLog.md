# Fase 6 — combat log

O painel de sistema/combate em cima do chat, do `NWindow_C1` (HEAD `4e56326`,
`src/inject/combatlog/CombatLog.cpp`; docs `COMBAT_LOG.md` e `SESSION_2026-07-16_COMBATLOG_UI.md`),
agora na nossa source. Porte do trabalhador status, 27/09/2026.

O que ele faz, igual ao `NWindow_C1`:
- Uma segunda lista acima do chat, com fundo `L2UI.etcwnd.NPCHtml_BACK`, rolagem à esquerda.
- Recebe as linhas de sistema (tipo 5) e as que parecem de combate pelo texto. Essas linhas saem
  do chat: exp/SP, drop/adena/"Earned <item>", dano dado e recebido, kill/level, skills.
- Cada categoria tem a sua cor. Uma linha longa quebra em mais de uma.
- Engrenagem `L2UI.ChatWnd.OptionBtn` no canto, ou Alt+L: abre o filtro embaixo da lista.
  - Uma linha por categoria, mais "All on" e "All off".
  - Fica em `combatlog.ini` `[filter]` (`exp_sp`, `drop`, `damage`, `dmg_recv`, `kill`, `other`).
- Arrasta com o botão esquerdo; some com o Alt+H; esvazia ao sair do mundo ou entrar nele.

Além do `NWindow_C1` (pedidos do usuário depois do primeiro teste no jogo, 27/09/2026; seção
"Além do NWindow_C1" abaixo):
- É uma janela do desktop, como o chat: some com a interface (Alt+H, a tela de loading do
  Restart, o lobby) e volta com ela.
- As janelas que o jogador abre (inventário, Community Board...) ficam por cima da lista.

## Onde está

- `src/features/CombatLog.cpp` e `include/features/CombatLog.h` (`nwf::CombatLog_*`, no namespace
  dos outros recursos).
- Pontos de chamada no código decompilado, cada um num bloco `#if NW_FEATURES`:
  - `NConsoleWnd::Tick` (`src/nwindow/NConsoleWndInput.cpp`): `nwf::CombatLog_TrySpawn(this)`, a
    primeira coisa, antes do `UL2ConsoleWnd::Tick`. O port do Tick do `NWindow_C1` também a fazia
    primeiro (`WindowBridge_OnConsole`, que só chamava o `CombatLog_TrySpawn`).
  - `NCChatWnd::AddChatMessage` (`src/nwindow/NCChatWnd.cpp`): no começo,
    `if (nwf::CombatLog_DivertChatMessage(Text, Color, Type)) return;`. Era o `jmp` que o `NWindow_C1`
    punha em `0x10037b30`.
- `nwf::CombatLog_OnAfterCacheClean()`: quem chama é o CacheCleaner do menus (`nwf::CleanCaches`,
  já no master), depois de cada FLUSH / OBJ GARBAGE, como o `CacheCleaner.cpp:99` do `NWindow_C1`.
- No `NConsoleWnd::Tick` o bloco do combat log vem antes do bloco do menus (AutoShot, BypassFlush,
  CacheCleaner), na ordem do port do Tick do `NWindow_C1`.
- `include/ue2/UE2Core.h`: `UObject::AddToRoot` e `UObject::IsValid`, exports do `Core.dll` que o
  `NWindow_C1` pegava por `GetProcAddress`.
  - Só declarações: o modo gancho e o standalone fiel não as chamam, então não ganham import.
- As janelas: a lista é filha do desktop (`NConsoleWnd::Desktop`, o `NCConsole` dono do chat).
  - A engrenagem e o filtro são filhos da lista, como o botão de opções e o filtro do chat são
    filhos do chat.
  - O `NWindow_C1` registrava as três como janelas raiz (`WndMain->RegisterNWindow`).

## Do endereço ao nome

| `NWindow_C1` | aqui |
|---|---|
| lista "ChatBack", UClass `0x1012dcf8`, ctor `0x10057ad0(0x96, 0x14)` | `new NCDebugWnd(150, 20)` |
| AddLine `0x10018310`, refresh `0x10017bf0` | `NCListBox::AddString`, `UpdateScrollBar` |
| textura `0x10057740`, `+0x1a0` | `NCDebugWnd::SetBackTexture`, `BackTex` |
| lista `+0x1a8`..`+0x1e4` | `Items`, `ScrollUpButton`/`ScrollDownButton`/`ScrollBar`, `VisibleLines`, `MaxLines`, `TopIndex`, `NumLines`, `Selected`, `Unknown1D4` (altura da linha), `TextX`, `TextY`, `LineGap`, `bAutoScroll` |
| SetRect `0x10023d90`, `+0x30`..`+0x58` | `NCWnd::SetWindowRect`, `Local*`, `Screen*`, `Width`, `Height`, `Style`, `Entry` |
| vtable `+0x60`, `+0xa8`, `+0xac` | `WindowProc`, `ShowWindow`, `HideWindow` |
| Register `0x100257d0` na raiz (console `+0x36bc`) | `Desktop->CreateChild` / `CreateChild` na lista (seção "Além do NWindow_C1") |
| botão: ctor `0x10001a20`, UClass `0x10117260`, `+0x1b8`/`+0x1bc` | `new NCButton(...)`, `NormalTexH`/`NormalTexW` |
| console `+0x36e0` "chat", `+0x36d8` "bottom" | `DebugWnd`, `MessageWnd` |
| console `+0x36dc` "UI host", `+0x3718` | `Desktop`, `bSuspended` |
| raiz `+0x3c`/`+0x40` (lista de pintura), entrada `+4` | `WndMain->TopLevel`, `NWndEntry::Wnd` |
| `UL2ConsoleWnd::GetState` | `State` (o `GetState` do Engine só devolve o campo) |
| `WindowFix_CursorClientPos` / `GameHasFocus` / `CursorInClient` | os mesmos, do porte do WindowFix (`nwf::WindowFix_*`, `features/WindowFix.h`) |

Achado de passagem, mantido:
- O "ChatBack" do `NWindow_C1` é a lista de debug do console, criada do mesmo jeito
  (`NCDebugWnd(150, 20)` com a textura `NWindow.ChatBack`, daí o nome).
- O "chat" que ele mede (`+0x36e0`) é essa janela de debug, escondida durante o jogo. Então a
  conta usa o tamanho padrão (360×200) e o topo vem do `MessageWnd`, que fica na altura da tela
  menos 52.
- Em 1024×768 a conta dá (0, 400), com 310×110 (a posição do `MessageWnd` vem do
  `NConsoleWnd`, `SetScreenPos(0, GSEKScreenY - 52, ...)`).
- O porte lê os mesmos campos, então o lugar é o mesmo.
- **Testado e desfeito (30/09/2026):** deixar o painel do mesmo tamanho do chat (`Desktop->ChatWnd`, 345×174,
  em cima dele) foi feito e instalado, e o dono não gostou ("não ficou legal": o painel fica alto demais e
  cobre o mundo). Voltou ao tamanho do `NWindow_C1` (310×110 em 1024×768). Não tentar de novo sem outra
  proporção; se quiserem um combat log maior, melhor uma opção no `combatlog.ini`.

## O que difere do `NWindow_C1`, e por quê

1. **Desvio por chamada, não por gancho.**
   - O `NWindow_C1` instalava o `jmp` em `0x10037b30` ao criar a primeira lista e nunca tirava.
   - Aqui o `GDivertOn` faz esse papel: liga na primeira lista e não desliga mais.
   - Antes da primeira lista, o chat recebe tudo (as boas-vindas do primeiro login).
   - Depois, as linhas de sistema/combate não voltam ao chat. Isso vale também enquanto a lista
     está escondida (tela de personagens) ou sendo refeita, como no `NWindow_C1`.
2. **Limpar a lista.**
   - O `clear_list_soft` zerava a contagem de `Items` e deixava vazar as linhas (`NString`); o doc
     dele chamava isso de "aceitável".
   - Aqui é o `NCListBox::Clear` do original: apaga as linhas e zera a barra de rolagem.
   - Na tela dá o mesmo. Vale para a lista (ao sair/entrar no mundo) e para o filtro (a cada
     reconstrução).
3. **Cursor e foco** (resolvido em 27/09/2026, segunda rodada).
   - O `NWindow_C1` perguntava ao WindowFix, que segue a janela do viewport.
   - A primeira versão do porte fazia os mesmos três testes sobre o `GViewportHWnd`, porque o
     WindowFix ainda não estava portado. Com `[Window] Enable=0` ela divergia: o `NWindow_C1`
     usava coordenadas de tela e "sempre com foco".
   - Agora o porte do lobby expõe as mesmas funções (`nwf::WindowFix_CursorClientPos`,
     `WindowFix_GameHasFocus`, `WindowFix_CursorInClient`), e o combat log as usa: igual ao
     `NWindow_C1` nos dois modos, e coerente com o mouse do resto da interface
     (`nwf::WindowFix_GetCursorPos`).
4. **Log.**
   - No `4e56326` as linhas do combat log só existiam num build `NWINDOW_DIAG`. O DLL do
     `System - Copia`, anterior, escrevia sempre "create OK" e "stock UI HIDDEN/shown".
   - Aqui é `nwf::Log("chatlog", ...)` com `[Window] Log=1`, com os mesmos textos.
   - Ficaram de fora três grupos de linha:
     - as da instalação do gancho ("37b30@... bytes", "after patch", "hook OK/FAIL"), porque não
       há remendo;
     - a periódica "alive t=... hooks=... diverted=...", uma a cada 300 ticks, que encheria o log;
     - as de falhas que não podem mais acontecer (API não achada, `StaticAllocateObject` nulo).
   - O "create OK" sai uma vez (o build DIAG escrevia duas).
   - Os 40 primeiros "DIVERT t=... #n" de cada entrada no mundo continuam.
   - Com as janelas no desktop (seção abaixo), saíram também "Register null", "OptionBtn Register
     null" e "OptionBtn FAIL (Alt+L still works)": o `CreateChild` do original não falha assim.
     "no root" virou "no desktop".
5. **SEH.**
   - O `NWindow_C1` punha `__try` em quase todo acesso por offset.
   - Aqui os campos são declarados. O `__try/__except` ficou onde há risco real: os pontos de
     entrada, a linha nova, o layout e o teste de objeto vivo.
6. **Não portado, porque ninguém chamava:** `CombatLog_Get`, `CombatLog_GetConsole` e os aliases do
   `TestWindow.h` (`TestWindow_TrySpawn`/`TestWindow_Get`). Só o `WindowBridge` os usava, e o
   `WindowBridge` e o `TestWindow` ficam de fora (`Fase6.md`).
7. **`combatlog.ini`.**
   - Fica ao lado do nosso `NWindow.dll`. O do `NWindow_C1` ficava ao lado do
     `LineageWindow.dll`: é a mesma pasta `System`.
   - Mesma seção, mesmas chaves, mesma regra:
     - chave ausente ou negativa: a categoria não muda;
     - todas ausentes: tudo ligado.

8. **Só as linhas de sistema são desviadas** (04/10/2026, a pedido do dono).
   - O `NWindow_C1` e a primeira versão do porte desviavam também pelo texto (`is_system_combat`): uma fala de
     jogador com "use ", "drop", "damage", " hp"... ia para o combat log ("mouse bugs", "the cause of"), e sumia
     com a categoria desmarcada no filtro.
   - O original já separa por tipo: as linhas de sistema chegam com tipo 5 (`NCConsole::AddSystemMessage` /
     `AddSystemText`), a fala vem do `NCConsole::Say` com o tipo do Say2.
   - Agora o desvio é `Type == 5`, menos o subtipo 6 (chat de petição, que vai para as janelas de petição).
     `IsSystemCombat` saiu; `ClassifyMessage` segue dando categoria e cor.
   - O teste diferencial da seção "Conferido" comparava o `is_system_combat`: essa parte deixou de valer.

## Além do `NWindow_C1`: uma janela do desktop, como o chat

Pedidos do usuário depois de testar o build L2 no jogo (27/09/2026). São mudanças de propósito,
que o `NWindow_C1` não tinha:
- Depois do Restart (voltar à seleção de personagem), a lista ficava na tela de LOADING com as
  últimas linhas. Ela tem de sumir quando a interface do jogo some e voltar no mundo.
- A lista era desenhada por cima do Community Board e do inventário. Tem de ficar como o chat:
  embaixo das janelas que o jogador abre.

**A causa** (achada pelo coordenador, conferida no código):
- O `NWindow_C1` registrava a lista, a engrenagem e o filtro como janelas raiz
  (`WndMain->RegisterNWindow(..., NULL)`).
- O `NCVirtualWndMain::DispatchWndMsg` pinta as janelas raiz na ordem do registro, cada uma com a
  sua árvore. As do combat log vinham depois do desktop, então por cima de tudo dele.
- A árvore de uma raiz só é pintada se a própria raiz está visível. O desktop escondido não
  escondia as três.
- No Restart, o `NConsoleWnd::RestartResetWindows` esconde o desktop e mostra o loading. O
  `State` continua `L2CS_InGame` até o `RestartToLobby`, depois do mapa do lobby carregar. O mapa
  carrega de uma vez, com quadros de progresso pintados e sem tick. O tick do combat log não
  rodava, e a lista seguia pintada no loading.

**Como o original faz com o chat** (`NCConsole::OnCreate`, `NCChatWnd::OnCreate`):
- `SetWindowRect` no espaço do pai, depois `CreateChild(ChatWnd, NWS_VISIBLE | NWS_NOHITTEST |
  NWS_TOPMOST)`.
- A moldura do chat e as listas de texto são `NWS_NOHITTEST`: um clique no texto do chat passa
  para o que está atrás, até para o mundo.
- Tomam clique a caixa de texto, as abas, o botão de opções e o filtro. O botão e o filtro
  (`NCChatFilterWnd`, acima do chat) são filhos do chat.
- O `CreateChild` põe a janela no fim da lista do pai. A lista é pintada do fim para o começo, então
  a janela nova fica embaixo. O começo da lista é a frente.
- Clicar numa janela que toma clique chama `SetFocus`, que faz `BringChildToTop` subindo pelos
  pais. Clicar na caixa de texto do chat traz o chat inteiro para a frente.
- Abrir uma janela (inventário, personagem) só faz `ShowWindow`: ela não sobe. Ela cobre o chat se
  estiver na frente dele na lista.
- O `NWS_TOPMOST` não é camada: só põe a janela na lista das que as outras "grudam" ao serem
  arrastadas (`NCWnd::Snap`). O combat log não usa, porque o `UnRegisterNWindow` do original nunca
  tira ninguém dessa lista.

**O que o combat log faz agora:**
- A lista é criada assim: `SetWindowRect` no espaço do desktop, depois
  `Desktop->CreateChild(Lista, NWS_VISIBLE | NWS_NOHITTEST)`. Ela entra no fim da lista do desktop,
  embaixo das janelas dele.
- A engrenagem é `CreateChild` na lista, como o botão de opções do chat. O filtro também é filho da
  lista, embaixo dela.
- Aparecer e sumir é com o desktop:
  - O `NCWnd::HideWindow` e o `ShowWindow` do desktop percorrem os filhos.
  - Com o desktop escondido (Alt+H, loading do Restart, lobby), a árvore dele não é pintada.
  - O tick não força mais `ShowWindow`/`Style` na lista a cada quadro, como o `NWindow_C1` fazia com
    as raízes. Ele só fecha o filtro enquanto a interface está escondida, e registra "stock UI
    HIDDEN/shown".
  - O teste do Alt+H (as três "canárias" e a contagem das janelas raiz) virou
    `IsInterfaceHidden`: `bSuspended`, ou o desktop escondido. No mundo, as canárias davam o mesmo.
- O filtro fechado sai da lista de filhos da lista (`NCWnd::RemoveChildNode`) e continua
  registrado. Ao abrir, volta (`AddChildNode`).
  - Motivo: o `NCConsole::ShowWindow` (entrar no mundo, Alt+H) mostra todos os filhos, e o tick
    espera 60 quadros ao entrar no mundo. Sem isso, o filtro fechado aparecia por um segundo.
  - Fora da lista de filhos, nem o `ShowWindow` recursivo nem a pintura chegam nele.
- Vir para a frente:
  - Um clique nos controles de rolagem, na engrenagem ou no filtro traz a lista pelo caminho do
    original (`SetFocus`).
  - Abrir o filtro (engrenagem ou Alt+L) traz a lista para a frente, para o filtro ficar por cima do
    chat, que ele sobrepõe.
  - Começar a arrastar a lista traz para a frente, como o arrasto pela barra de título de qualquer
    janela do original.
  - Um clique no texto passa, como no chat.
- A lista coberta não reage:
  - A engrenagem e o arrasto só agem se o combat log é o que está na tela sob o cursor
    (`IsFrontAt`).
  - "Na tela" quer dizer: nenhuma janela visível na frente dele na lista do desktop, incluindo as
    filhas dela (o filtro do chat, por exemplo), e nenhuma janela raiz registrada depois do desktop
    (opções, petição) cobre o ponto.
  - Sem isso, um clique no inventário por cima da lista arrastaria a lista escondida.
- Posição:
  - `PlaceWnd` move a janela e a subárvore pelo deslocamento na tela (`OffsetScreen`).
  - O `Local*` é a posição no pai.
  - O `SetWindowRect` do original desloca os filhos pela posição nova no pai menos a posição velha
    na tela. Isso só funciona com o pai em (0, 0), e a engrenagem e o filtro são filhos da lista.
- **Achado de passagem, corrigido:** o filtro mostrava os seus controles de rolagem, no
  `NWindow_C1` e na primeira versão do porte.
  - O `apply_layout` escondia os controles e depois chamava o `ShowWindow` do filtro, que mostra os
    filhos.
  - Os controles ficavam 12 pixels à esquerda do filtro: fora da tela com a lista em x = 0, visíveis
    com a lista arrastada para a direita.
  - Agora o `ShowWindow` vem antes e os controles ficam escondidos.

**Conferido fora do jogo** (`wndtest`, no scratchpad, fora do repositório):
- Um programa carrega o `build/l2/Release/NWindow.dll` numa pasta com os DLLs do engine.
- Monta janelas sem construtor: os campos e a vtable da classe escritos direto.
- Chama o código do DLL pelos endereços do `NWindow.map`: os nossos `IsFrontAt`, `HideCfg`,
  `ToggleCfg`, `PlaceWnd`, `IsInterfaceHidden` e os métodos do `NCWnd` (`ShowWindow` e `HideWindow`
  recursivos, `BringChildToTop`, `AddChildNode`).
- O `GMalloc` é trocado por um heap, como no `tools/difftest`.
- 31 verificações de janela, todas ok:
  - o que cobre a lista: o inventário na frente ou atrás, o filtro do chat como filha, a janela de
    opções depois do desktop, a janela de debug antes dele;
  - desktop escondido e mostrado: a lista, a engrenagem e a rolagem vão e voltam, o filtro fechado
    fica escondido;
  - abrir o filtro: volta à lista de filhos, com as dez linhas, a rolagem dele escondida e a lista na
    frente;
  - Alt+H com o filtro aberto: ele fecha e não volta;
  - `PlaceWnd` com a subárvore e com um desktop fora de (0, 0).

## Conferido

- **Teste diferencial** (fora do repositório, no scratchpad).
  - Lado a lado, as funções do `NWindow_C1` copiadas do arquivo (`classify_msg`,
    `is_system_combat`, `should_show_combat` com 10 filtros, `color_for_msg`, `is_earned_item_line`,
    `max_chars_for_panel`, `take_wrap_chunk`) e as nossas.
  - Corpus: as 777 linhas do `SystemMsg-e.txt` do C1, cada uma também com os `$s` trocados de 6
    jeitos, mais 40 amostras (PT com acento, canais, linhas longas).
  - A quebra de linha rodou em 10 larguras, 4 recuos e 4 tamanhos de buffer.
  - Resultado: 9,2 milhões de comparações, 0 diferenças.
- **Ciclo** (`--no-install`; em 27/09/2026, segunda rodada): tudo igual
  ao master.
  - Modo gancho: 2744 ganchos, verificador 2549 ok + 195 explicadas, 0 sem explicação; regcheck
    limpo; difftest 11 iguais.
  - Standalone fiel: `verify_standalone` OK; difftest 10 iguais + 1 pulado; smoke igual ao original.
  - L2: build, `craft data` (38 verificações), `l2: verify`, `l2: smoke` (todos os estágios, a mesma
    saída do original, os remendos de outros módulos no lugar),
    todos OK e sem aviso de compilação.
- **Binário.**
  - O `NWindow.dll` fiel reconstruído tem `.text`, `.data` e `.rsrc` idênticos aos de antes; só
    muda o diretório de debug (data e GUID do PDB).
  - No `NWindowDecomp.dll` só mudam os inicializadores de registro dos ganchos (`nwHook_<linha>`):
    as linhas mudaram em três arquivos.

## Para testar no jogo

Com o `build/l2/Release/NWindow.dll` instalado pelo coordenador no `System - NWindow Teste`.
Os itens 9 a 13 são os das mudanças de 27/09/2026 (seção "Além do NWindow_C1").
Copiar antes o `combatlog.ini` e o `LineageWindow.ini` do `System - Copia` para ter as mesmas
configurações; o `combatlog.ini` do usuário tem `kill=0`.

1. Entrar no mundo. Em cerca de um segundo a lista aparece acima do chat, com a rolagem à esquerda
   e a engrenagem no canto de cima à direita.
2. Caçar e conferir as linhas na lista, com as cores:
   - exp/SP em amarelo;
   - "Earned ..." e adena em verde;
   - dano dado em vermelho claro, recebido em laranja;
   - skills/outros em cinza.
   Nenhuma delas pode aparecer nas abas do chat. O chat dos jogadores (normal, grito, party,
   comércio, clã) continua no chat.
3. Uma linha longa quebra na linha de baixo; a lista fica sempre na mais nova.
4. Engrenagem e Alt+L abrem e fecham o filtro.
   - Clicar numa categoria troca `[x]`/`[ ]`; "All on" e "All off" também funcionam.
   - Fechar o jogo e voltar: o filtro continua o mesmo (`combatlog.ini`).
5. Arrastar a lista pelo corpo: ela fica onde foi solta.
6. Alt+H esconde a lista junto com a interface, e mostra de novo.
7. Voltar à seleção de personagem e entrar de novo: a lista começa vazia.
8. Com `Log=1`, o `LineageWindow.log` ganha as linhas `chatlog:` (create OK, state, DIVERT...).
9. Restart (voltar à seleção de personagem):
   - a lista some junto com a interface, sem ficar na tela de LOADING;
   - na seleção de personagem não aparece;
   - volta, vazia, ao entrar no mundo.
10. Abrir o inventário e o Community Board por cima da lista: eles cobrem a lista, como cobrem o
    chat.
11. Trazer a lista para a frente:
    - clicar nas setas de rolagem ou na engrenagem traz a lista para a frente;
    - arrastar a lista também traz;
    - clicar só no texto não traz (como no chat), e o clique vai para o mundo, como antes.
12. Com o inventário por cima de uma parte da lista, clicar no inventário ali não arrasta a lista e
    não abre o filtro.
13. Filtro aberto:
    - aparece por cima do chat;
    - Alt+H fecha o filtro;
    - ao mostrar a interface de novo, o filtro continua fechado e a lista volta;
    - arrastar a lista com o filtro aberto leva o filtro junto, sem setas de rolagem soltas ao lado.

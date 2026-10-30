# Fase 6 — WindowFix (modo janela)

Porte do `src/inject/WindowFix.cpp` do NWindow_C1 (HEAD `4e56326`; RE no `docs/WINDOW_FIX.md` de
lá). Código: `src/features/WindowFix.cpp`, `include/features/WindowFix.h`. Trabalhador lobby,
27/09/2026.

## O que faz (igual ao NWindow_C1)

- Moldura móvel (`Mode` = `Auto` / `Framed` / `Borderless`), posição salva (`X`, `Y`).
- Mouse da UI no espaço do cliente (`FixUiMouse`): a UI do C1 acerta cliques com a janela fora de
  (0,0).
- Filtro de clique fora da área do jogo (`FilterClicks`), cursor preso durante o arrasto
  (`ConfineDrag`) e em tela cheia (`ConfineFullscreen`).
- Minimizar sem `SetRes(0,0)` (`SafeMinimize`), confirmação no X (`ConfirmClose`, texto "Deseja
  fechar o Lineage II?").
- Recupera o cursor escondido por uma captura perdida.
- `[Window]`: as mesmas chaves e padrões do `LineageWindow.ini`, **sem arquivo** (pedido do dono,
  28/09/2026): os valores estão compilados (`NwFeatureConfig.cpp`) e o que o jogador muda (posição
  `X`, `Y`; `Fullscreen`) fica em `HKCU\Software\Lineage2C1\NWindow\Window`. A DLL não grava nada na
  pasta do jogo; uma pasta sem permissão de escrita não a afeta (a escrita no `Option.ini`,
  `WindowsInfo.ini` etc. é do próprio jogo).
- O `ini_int` próprio do NWindow_C1 foi mantido: valor vazio ou não numérico dá o padrão (o
  `GetPrivateProfileInt` daria 0).
- Log `wfix:` em `LineageWindow.log` só no build com `NW_FEATURE_LOG=1` (diagnóstico); o build
  normal não escreve arquivo.

## O que virou código direto

O NWindow_C1 trocava na IAT do NWindow o `GetCursorPos` (tela → cliente do viewport) e o
`ClientToScreen` (identidade para a janela do viewport). O original chama os dois em 5 funções
(6 chamadas); a nossa source tem 7 chamadas, porque o `MasterConsoleEventProcess` separa botão
esquerdo e direito. Cada uma tem agora o seu bloco `#if NW_FEATURES` com `nwf::WindowFix_*`:

| Função (original) | Arquivo | Chamada |
|---|---|---|
| `NConsoleWnd::UpdateCursor` (0x10054e20) | `NConsoleWndInput.cpp` | `GetCursorPos` (encantamento: cursor fora do inventário) |
| `NConsoleWnd::MasterConsoleEventProcess` (0x10055f20) | `NConsoleWndInput.cpp` | 3× `GetCursorPos` (botões e roda do DirectInput) |
| `NCVirtualWndMain::ClientToScreenPacked` (0x100243c0) | `NCVirtualWndMain.cpp` | `ClientToScreen(GViewportHWnd)` |
| `NCVirtualWndMain::PaintToolTips` (0x10024b00) | `NCVirtualWndMain.cpp` | `GetCursorPos` |
| `NCWnd::Snap` (0x100232a0) | `NCWnd.cpp` | `GetCursorPos` |

A condição é a mesma do NWindow_C1: o recurso ligado (`Enable` e `FixUiMouse`) e a janela do
viewport conhecida. Enquanto o hook do WinDrv não viu a janela (ou se o WinDrv não é o esperado),
as chamadas ficam iguais às do original.

## O que continua remendo em tempo de execução (WinDrv.dll)

O WinDrv do C1 é `/INCREMENTAL`: toda chamada passa por um thunk `E9 rel32`. Trocar o `rel32` pega
todos os chamadores sem tocar no corpo.

| Thunk (RVA) | Corpo | Uso |
|---|---|---|
| `ResizeViewport` `0x100A` | `0x6570` | depois do original: moldura/posição (janela) ou tira o menu de sistema (tela cheia) |
| `OpenWindow` `0x1136` | `0x4310` | pega o HWND e faz a subclasse da janela |
| `CauseInputEvent` `0x11A9` | `0x51F0` | filtro de clique e confinamento durante o arrasto |
| `ToggleFullscreen` `0x101E` | `0x6440` | Alt+Enter, maximizar e o comando `TOGGLEFULLSCREEN`: tamanhos iguais aos da UI, recusa sem modo (seção "Janela ↔ tela cheia") |

- **Conferência no install:** para cada um, `E9`, o alvo esperado e o prólogo SEH
  `55 8B EC 6A FF 68`. Os três primeiros, todos ou nenhum; o `ToggleFullscreen` é conferido à
  parte (se não bater, o Alt+Enter fica o original e a página de vídeo não mostra a caixa).
- O `SetMouseCapture` (`0x52B0`) só é chamado, nunca desviado; o prólogo é conferido antes.
- Outro WinDrv: nada é instalado e sai a linha `wfix: WinDrv.dll at … is not the expected C1
  build - hooks skipped`.
- A subclasse da janela do viewport (`ViewportProc`, propriedade `LineageWindow.OrigWndProc`)
  continua igual. Ela engole o `WM_MOVE`, que o NWindow transformaria em "mover a janela de UI do
  topo" e que fazia a UI sumir; cuida também de minimizar, foco, Alt, X e roda.

## Quando entra

No nosso `DllMain` (`nwf::OnDllMain`, `src/features/NwFeatureStartup.cpp`).
- O `LineageWindow.dll` era import do `NWindow.dll`: o `DllMain` dele rodava no mesmo
  `LoadLibrary` ("Bound to NWindow").
- Nesse momento o WinDrv já está carregado, e a janela do viewport só abre depois do mapa do
  lobby. É o mesmo momento de antes.
- A thread adiada do NWindow_C1 só repetia o install logo depois; aqui não há repetição.
- `DisableProcessWindowsGhosting` como antes.

## Diferenças do NWindow_C1

- IAT → chamadas diretas (acima). Sem a IAT não existe o caso "slots não achados": o recurso
  depende só do ini.
- Caminhos ANSI (`nwf::IniPath`, o helper da fase 6) no lugar de `GetModuleFileNameW`.
- A linha de log da IAT virou `wfix: NWindow: GetCursorPos/ClientToScreen -> client space`.
- As linhas `windowfix: install @attach OK/FAIL…` (antes `diag`, só no build de diagnóstico) saem
  com `Log=1`.
- Para os outros recursos, as mesmas funções do `WindowFix.h` do NWindow_C1, agora em `nwf::`:
  `WindowFix_GameHwnd`, `WindowFix_CursorClientPos`, `WindowFix_GameHasFocus` e
  `WindowFix_CursorInClient`. O combat log e as janelas de craft as usam.

## Janela ↔ tela cheia (03/10/2026; não existe no NWindow_C1)

**O que um testador viu:** com o `Option.ini` dele o jogo não ficava em janela; com o `Option.ini` do
dono ficou, mas o Alt+Enter para tela cheia "bugou" (no PC do dono não). O que se sabe, lendo o
`WinDrv.dll`, o `D3DDrv.dll` e o `Engine.dll` do cliente de teste:

- O Alt+Enter (e o comando `TOGGLEFULLSCREEN`, e `SC_MAXIMIZE`) chama `UWindowsViewport::ToggleFullscreen`
  (`0x6440`; os três chamadores passam pelo thunk `0x101E`). Em janela ele chama
  `TryRenderDevice("ini:Engine.Engine.RenderDevice", -1, -1, 1)`; o `-1` vira
  `UClient.FullscreenViewportX/Y` (`+0x68/+0x6C`). Na volta, `EndFullscreen` usa
  `WindowedViewportX/Y` (`+0x60/+0x64`). Os quatro vêm do `l2.ini` (cifrado, o jogador não edita);
  o tamanho do `Option.ini` (`GamePlayViewportX/Y`, o `GSEKScreenX/Y` em que a UI é montada) **não
  entra**. Os offsets são os da `UClient::StaticConstructor` (`Engine.dll`, em ordem: Windowed X, Y,
  Fullscreen X, Y a `0x60, 0x64, 0x68, 0x6C`).
- O `SetRes` de tela cheia do `D3DDrv` não obedece o tamanho que recebe: faz um
  `FindBestMatchingMode` ("Best-match display mode: %ix%ix%i@%i" no log) e **sobrescreve** X e Y com
  o modo mais próximo que o monitor lista. Sem o modo do `Option.ini`, o viewport sai de outro
  tamanho e a UI continua montada no tamanho antigo. Sem nenhum modo ("No acceptable display modes
  found") o `SetRes` falha e o `TryRenderDevice` solta o `RenDev` do viewport (`[viewport+0x70] = 0`,
  linha "Failed3D"); o `ToggleFullscreen` não se recupera disso.
- O `NCVideoOptionWnd::Apply` decidia janela/tela cheia pelo `StartupFullscreen` do `l2.ini`, não
  pelo estado real do viewport: depois de Alt+Enter, trocar a resolução pedia o modo antigo.
- **O Alt+Enter nem chegava ao `ToggleFullscreen` dentro do jogo** (achado no primeiro teste, 03/10):
  o `ViewportWndProc` do WinDrv chama primeiro o console do NWindow (vtable `+0x3B8`,
  `NConsoleWnd::DispatchWndMsgX`) e só trata a tecla se ele devolver 0. O console passa as teclas ao
  `HookShortCutKey` só com o jogo em `L2CS_InGame`, e este devolve `1` (tomada) para **todo**
  `WM_SYSKEYDOWN` (Alt + letra abre janelas); no lobby o console devolve `1` para qualquer mensagem
  que não seja clique esquerdo. Só a tela de login deixava o Alt+Enter chegar ao WinDrv.

**O que o código faz agora** (`WindowFix.cpp`, seção "Window <-> fullscreen"):

- `HookToggleFullscreen` (só com a UI já montada, `NConsoleWnd::Initialize` pronto): copia
  `GSEKScreenX/Y` para os quatro tamanhos do `UClient`; indo para tela cheia, confere que o monitor
  lista um modo desse tamanho (`GetRes` do `RenDev`, qualquer profundidade) e, se não lista, **recusa**
  com uma caixa de aviso (pt-BR, manda escolher outra resolução em Options > Video) e não chama o
  original. Depois do original grava o modo em que ficou (`HKCU\...\Window`, valor `Fullscreen`).
  Sem lista de modos ou sem tamanho conhecido, o dispositivo decide, como no original.
- Início: `WindowFix_StartupFullscreen`, chamada pelo `NConsoleWnd::Initialize` no lugar de usar o
  `StartupFullscreen` direto: vale o último modo que o jogador usou; sem registro, o
  `StartupFullscreen` do cliente; se o resultado é tela cheia e o monitor não tem o modo do
  `Option.ini`, começa em janela (o início original acabaria noutro tamanho). Sem o gancho, o
  `StartupFullscreen` é o original.
- **Alt+Enter na subclasse da janela** (`ViewportProc`, `WM_SYSKEYDOWN` + `VK_RETURN`): trata a tecla
  antes do console do NWindow (a repetição da tecla é ignorada; janela minimizada também) e chama o
  mesmo gancho. Assim vale em qualquer tela, inclusive no jogo e no lobby. Só com o gancho do
  `ToggleFullscreen` instalado; sem ele, tudo como o original.
- Página de vídeo (`NCVideoOptionWnd`): linha **"Full Screen"** (caixa "Use", como as outras) **no topo**
  da página, as demais abaixo. Não há altura sobrando (a página tem 270 px, o clip vai a 280, os
  botões começam em 284), então toda linha original desce 15 px e as caixas (12 px de altura) que são
  seguidas por outra linha ficam 15 px uma da outra, não 18; a última caixa termina em 269. Em
  `RowY` (tabela do y original para o novo); `CreateOptionCombo`, `CreateOptionBox` e `DrawLabel`
  passam por ela, de modo que `OnCreate` e `OnPaint` continuam com o texto do original. A caixa
  mora em `UnusedD0[0]` (ponteiro livre; o tamanho `0xDC` não muda) e só existe se o gancho do
  `ToggleFullscreen` está instalado (sem ela a página é a original). OK/Apply mudam o modo pelo
  mesmo gancho, depois da resolução (assim o modo pedido existe na lista); a caixa mostra o modo
  real do viewport quando a janela de opções abre (`NCOptionWnd::ShowWindow`) e depois de Apply
  (senão um Alt+Enter feito antes e um OK depois desfariam a troca).
- **O build fiel não muda**: o código só entra em `#if NW_FEATURES`. Conferido compilando o
  `NWindowStandalone` do `HEAD` e o do `HEAD` + estas alterações na mesma árvore: `.text`, `.data`,
  `.rsrc` e `.reloc` idênticos byte a byte (só o GUID do PDB muda no `.rdata`). A primeira versão
  mexia nas linhas originais de `DrawLabel` e a do Korean literal sem `#if`; mesmo equivalentes, o
  `OnPaint` do build fiel mudou 35 bytes (a comparação pegou), por isso essas linhas ficam no `#else`
  como no original.
- `Apply` passa a usar o modo real do viewport no `SetRes` da resolução.
- Chave nova no registro: `[Window] Fullscreen` (0/1). Não está no `GBuiltIn`: o padrão é o
  `StartupFullscreen` do cliente.

**Não verificado (precisa do jogo):**

- Que a causa do testador seja essa. É o mecanismo achado no código, não a reprodução: falta o
  `Option.ini` original dele, o que apareceu na tela e o `l2.ini` dele (cifrado).
- Por que o `Option.ini` original dele não ficava em janela. Hipótese: o tamanho dele não cabe na
  área útil e o `Mode=Auto` cai em sem moldura (`LayoutWindowed`), que parece tela cheia.
- O visual da página de vídeo: não li a textura `OptionBack`. Se ela tiver divisórias a cada 18 px,
  as linhas novas ficam fora delas. O rótulo "Full Screen" está em inglês, como o resto da UI.
- O comportamento de `TryRenderDevice`/`SetRes` com o monitor do testador: só sei o código.
- Quando o modo atual não está na lista do monitor, o combo de resolução abre em branco
  (`Selected = -1`) e o `Apply` não mexe na resolução; é o comportamento original.
- **Uma queda, não reproduzida** (03/10, RTX 3070, Win10): no jogo, em janela, Alt+Enter para tela cheia
  (`AttemptFullscreen`, `Best-match display mode: 1366x768x32@60`, funcionou) e o Alt+Enter de volta
  terminou em `Error setting display mode: CreateDevice failed (80070057)`, na pilha
  `UD3DRenderDevice::SetRes <- UWindowsViewport::EndFullscreen <- ToggleFullscreen <- MessagePump`
  (o `CreateDevice` do D3D8 em janela, depois do `Release` do dispositivo de tela cheia; o `D3DDrv` nunca
  usa `Reset` no jogo, sempre recria). No teste seguinte, com a mesma DLL, a volta funcionou.
  Já descartado num programa D3D8 isolado nesta máquina (mesma GPU e driver), com os mesmos parâmetros de
  apresentação que o `D3DDrv` monta: tela cheia a 60 ou 144 Hz, pausa antes do `CreateDevice`, cursor preso
  como o `UpdateClip`, janela com moldura e estilo mexido como o `StripFrameForFullscreen`, a volta feita
  de dentro do procedimento da janela. Todos passam. Um recurso vazado (buffer da pool padrão, ou uma
  referência ao dispositivo) dá `D3DERR_DEVICELOST` e o desktop não é restaurado, que não é o que o log
  mostrou (`WM_DisplayChange` aconteceu, o erro foi `E_INVALIDARG`). Diferença entre as voltas que
  funcionaram (caixa das Opções, um evento da UI no `Tick`) e a que falhou (Alt+Enter, dentro do
  `WndProc`): vale como primeira hipótese se voltar a ocorrer (adiar o pedido para o `Tick`). O build com
  `NW_FEATURE_LOG=1` registra o estado da janela e do desktop antes e depois de cada troca (`toggle begins` /
  `toggle ended`) e cada Alt+Enter recebido.

## Conferido fora do jogo

- `smoke_standalone.py --l2` (passo `l2: smoke`) carrega o WinDrv.dll do cliente (com
  Window, Network e Engine) antes do nosso `NWindow.dll`, em `build/smoke_l2_patches`, e relê os
  quatro thunks: `hooked (NWindow.dll)`, com `wfix: WinDrv hooks OK … base=11100000` e
  `wfix: WinDrv ToggleFullscreen hook OK` no log (`PATCHES = 7`).
- No smoke normal (sem WinDrv) sai `wfix: WinDrv.dll not loaded` e nada é instalado.

## Testar no jogo

1. Janela: abre com moldura e centralizada; arrastar pela barra de título (os cliques ali não
   chegam ao jogo) e reabrir o jogo, que deve voltar na posição salva.
2. Clicar em campos e botões da UI com a janela longe de (0,0), inclusive no 2º monitor; conferir
   o tooltip e o encaixe de janelas arrastadas.
3. Girar a câmera com o botão direito: o cursor fica preso na janela e solta ao largar.
4. Minimizar e restaurar sem voltar 1×1; Alt+Tab no meio de um giro de câmera: o cursor volta.
5. X da janela: aparece a confirmação.
6. Alt+Enter ida e volta: tela cheia no tamanho da UI (a UI e o mouse alinhados) e volta para a
   janela no mesmo tamanho. Com um `Option.ini` de um tamanho que o monitor não lista (por exemplo
   o de outro PC): o Alt+Enter mostra o aviso e fica em janela, sem tela preta.
7. Options > Video > "Full Screen": marcar e dar OK vai para tela cheia, desmarcar volta. Fazer
   Alt+Enter e abrir as Opções: a caixa mostra o modo atual. Reabrir o jogo: volta no último modo.
   Trocar a resolução em tela cheia e em janela: o modo não muda sozinho.
8. A página de vídeo inteira: "Full Screen" no topo e as outras linhas abaixo, sem sobreposição,
   cada rótulo alinhado com o seu controle, e nada cortado em cima dos botões OK / Cancel / Apply
   (a última linha, Lower Detail, termina em 269 de 270).
   Alt+Enter: dentro do jogo, no lobby e na tela de login (antes só funcionava no login).
9. `Mode=Borderless` e `Enable=0`, que deve se comportar como o original (sem a caixa "Full Screen").

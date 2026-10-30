# Fase 6 — infraestrutura do lobby: início dos recursos, smoke do L2, ciclo

Trabalhador lobby, 27/09/2026, sobre o master `887115d`. Os recursos do lobby estão em
`F6_WindowFix.md` e `F6_ReturnToLogin.md`.

## Onde os recursos começam

**`nwf::OnDllMain(Reason)`** (`src/features/NwFeatureStartup.cpp`, declarado no `NwFeatures.h`).
- É chamado pelo nosso `DllMain` no `NWindowL2`. O `src/standalone/NWindowPackage.cpp` expande o
  `NW_IMPLEMENT_PACKAGE` sem mudança fora de `NW_FEATURES`; com `NW_FEATURES`, as mesmas três
  linhas mais a chamada.
- **Por que aqui.** O `LineageWindow.dll` do NWindow_C1 era import do `NWindow.dll` e o `DllMain`
  dele rodava no mesmo `LoadLibrary` ("Bound to NWindow" no L2.log):
  - depois do WinDrv.dll e do Network.dll;
  - antes do mapa do lobby e da janela do viewport.

  O nosso roda no mesmo `LoadLibrary`, depois dos inicializadores estáticos (as 178 `UClass`).
  Nada entre os dois momentos muda para os recursos.
- **Ordem do NWindow_C1** (`NWindow_inject.cpp`): CraftBalloon, AutoShot, MinimapHotkey,
  ReturnToLogin, ManufactureCmd, BuffTimer, CacheCleaner, WindowFix, CraftUi. Os
  outros trabalhadores põem os seus no lugar deles nessa lista.
- **A thread adiada do NWindow_C1** (`deferred_vtport`) só repetia os installs logo depois.
  - Os ports do NConsoleWnd que ela instalava não existem mais.
  - A repetição do ReturnToLogin é a do `Tick`, como lá.
- **`DLL_PROCESS_DETACH`:** devolve as funções do Engine.dll nossas às originais.
- Tudo ali roda sob o loader lock, como no NWindow_C1: handles de módulo, ini, proteção de página,
  `CreateThread`.
- `nwf::ModuleSibling(Name, …)` (antes `static` no `NwFeatureConfig.cpp`) ficou público: dá o
  caminho ao lado do nosso DLL.

## Declarações novas nos headers compartilhados

Só nomes; o layout não muda.

| Header | O quê |
|---|---|
| `UE2Core.h` | `CORE_API extern UBOOL GIsRequestingExit` (`?GIsRequestingExit@@3HA`, antes lido por `GetProcAddress`) |
| `UE2Engine.h` | `UL2ConsoleWnd::SavedState` (+0x30; era `_Unknown30`), `L2CS_Loading = 7` |

## Para os outros trabalhadores

**API do WindowFix** (`include/features/WindowFix.h`, namespace `nwf`): os nomes do NWindow_C1.
- `HWND WindowFix_GameHwnd()`.
- `bool WindowFix_CursorClientPos(POINT*)`.
- `bool WindowFix_GameHasFocus()`.
- `bool WindowFix_CursorInClient()`.

O combat log (status) e as janelas de craft (clan) chamavam essas funções no NWindow_C1.

**Ponto do Tick.** O `ReturnToLogin_OnTick` está no começo do `NConsoleWnd::Tick`, depois do
`guard`. No porte do Tick do NWindow_C1 a ordem era:
1. WindowBridge;
2. `CraftUi_OnTick`;
3. `AutoShot_OnTick`;
4. `ReturnToLogin_OnTick`;
5. `BypassFlush`;
6. `CacheCleaner_OnTick`;
7. o `UL2ConsoleWnd::Tick`.

**`__LINE__` no build fiel.** O `check()` do UE2 grava o `__LINE__` no código.
- Uma linha a mais acima de um `check()` muda dois bytes do `NWindowStandalone`, por isso o
  include do `NConsoleWndLogin.cpp` ficou abaixo dos `check()` dele.
- Só dois arquivos têm `check()`: `NConsoleWndLogin.cpp` e `NCPawnView_CharacterSelectWnd.cpp`.
- Com isso o `NWindowStandalone` desta branch é igual ao do master, byte a byte, fora o horário do
  link.
- No modo gancho, os nomes das inscrições de gancho também vêm do `__LINE__`. A arrumação delas no
  `.text`/`.rdata` muda com qualquer linha a mais, mas o verificador e o smoke mostram que o
  comportamento é o mesmo.

## O smoke do `NWindowL2`

Unificado com o do clan na rodada 13: uma lista, uma opção.

```bat
python tools\smoke_standalone.py --dll build\l2\Release\NWindow.dll --l2
```

- **System próprio:** `build/smoke_l2`, para o `LineageWindow.ini`/`.log` que o L2 escreve não
  cair no do fiel.
- **"Extras do L2".** São as `UClass` que só o `NWindowL2` registra (as 8 janelas de craft do
  clan).
  - A lista é uma só, `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`). O
    `verify_standalone.py --l2` do clan lê a mesma.
  - **Exports** (do clan, com `pefile`): a tabela do original igual, nome e ordinal, e depois os
    `autoclass` das extras, nada mais, nos ordinais logo após os do original (181..188).
  - **Contagens** (`nwd_smoke --standalone … --extras A,B`): as extras ficam fora das contas e
    conferências do original (exports, ordem dos ordinais, fila `GAutoRegister`, registradas).
    Aparecem em linhas próprias `  l2 extra …`, que a comparação com a saída do original pula.
    Não sobra conta a corrigir depois: a saída do resto é a do original.
  - **Registro:** o Python confere as linhas `l2 extra` com a lista (pacote, nome, pai). Tamanho,
    flags de classe e `InternalConstructor` ficam com o `verify_standalone --l2`, que os lê do
    DLL; a lista não tem tamanho.
  - **Falha:**
    - classe listada que o DLL não registra;
    - classe do original na lista;
    - classe do pacote que não é do original nem está na lista (sem export, a fila a acusa; com
      export, a tabela de exports e as registradas a acusam).

    O fiel nunca recebe `--l2` e continua estrito.
- **Remendos em tempo de execução** (`nwd_smoke --l2-patches`, `build/smoke_l2_patches`):
  - carrega o Core, as mesmas substituições do `appInit` do smoke e o WinDrv.dll do cliente (que
    traz Window, Network e Engine), depois o nosso `NWindow.dll`, como no jogo;
  - no load, relê os 6 lugares remendados: 3 thunks do WinDrv (WindowFix), 2 slots do
    `CClientSocket` do Network e o thunk do `OnServerClose` do Engine (ReturnToLogin). Cada um tem
    de levar ao `NWindow.dll`, e as linhas de install do `LineageWindow.log` aparecem;
  - **o balão de craft** (CraftBalloon, clan) remenda 6 pontos do `FDynamicActor::Render` do Engine
    só no primeiro tick do console, que fora do jogo não acontece:
    - o smoke não o vê instalado;
    - confere que os 6 pontos têm os bytes que o install dele confere antes de escrever os `jmp`
      (tipo A/B, rótulo A/B, cor A/B). Assim, no primeiro tick ele entra nesse Engine.dll;
    - se um dia ele passar a entrar no load, o smoke o vê como `hooked (NWindow.dll)`.
  - **Achado (não mudado, igual ao NWindow_C1):** o par do rótulo (`mov ecx, 1097692Ch`) tem um
    endereço absoluto.
    - Só bate com o Engine.dll na base preferida.
    - No jogo o Engine.dll é sempre relocado: o `L2.exe` fica em `0x10900000`, sem relocações,
      dentro dos 63 MB que o Engine quer a partir de `0x10300000`. No smoke também:
      `Engine.dll at 03220000`.
    - Então o install do balão deixa esse par como está (loga `buy-label sig mismatch`) e instala
      só os pares do tipo e da cor.
    - O balão sai certo mesmo assim: as cavernas da cor também trocam o texto (caminho A: a
      primeira linha do buffer; caminho B: o texto em `[esp]`).
    - O smoke aceita o par do rótulo nesse estado ("relocated immediate") e exige os outros dois.
  - Resultado: `at load: 6 of 6 in place`; `craft balloon …: 4 of 6 sites as its install checks
    them, the label pair left stock (Engine.dll relocated)`.

## O ciclo

- **4c, todos bloqueiam:**
  1. `l2: build`;
  2. `l2: craft data` (`nwd_craft_test`, clan);
  3. `l2: verify` (`verify_standalone.py --l2`);
  4. `l2: smoke` (`smoke_standalone.py --l2`).

  O `l2: verify` e o `l2: smoke` passaram a bloquear na rodada 13, com o último ramo da fase 6.

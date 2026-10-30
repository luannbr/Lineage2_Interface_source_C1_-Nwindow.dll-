# Fase 6 — janelas nativas de craft (estilo C2) e o balão de manufatura

Origem: NWindow_C1 HEAD `4e56326`, `src/inject/craft/` (`CraftUi`, `CraftWindows`, `CraftSession`,
`CraftData`, `NwUi`) e `src/inject/CraftBalloon.cpp`; docs `CRAFT_UI_C2.md`, `RECIPE_UI_C2_SPEC.md`,
`NWINDOW_NATIVE_WINDOWS.md`, `CRAFT_BALLOON_ACTIVATION.md`, `FEATURE_CRAFT_BALLOON.md`,
`MANUFACTURE_BALLOON_SHOWS_BUY.md`. Aqui:

| Arquivo | O que tem |
|---|---|
| `include/features/craft/CraftData.h`, `src/features/craft/CraftData.cpp` | o parser do protocolo v1 (`L2W mfg`) |
| `include/features/craft/CraftSession.h`, `src/features/craft/CraftSession.cpp` | a última mensagem de cada visão e os bypasses `mfg_*` |
| `include/features/craft/CraftWnd.h`, `src/features/craft/CraftWnd.cpp` | `NCRecipeWnd` e `NCRecipeListWnd` (o `NwUi` do NWindow_C1) e o tool tip |
| `include/features/craft/CraftWindows.h`, `src/features/craft/CraftWindows.cpp` | as seis janelas |
| `include/features/craft/CraftUi.h`, `src/features/craft/CraftUi.cpp` | configuração, transporte, o tick das janelas, Esc, teste de texturas |
| `include/features/CraftBalloon.h`, `src/features/CraftBalloon.cpp` | o balão (remendo no Engine.dll) |
| `include/features/NwL2Class.h` | as macros das classes que só o `NWindowL2` registra |
| `tests/standalone/l2_extras.csv` | a lista dessas classes (as ferramentas leem) |
| `tools/craft/craft_data_test.cpp` | o teste do parser fora do jogo (alvo `nwd_craft_test`) |

As mudanças **não commitadas** do NWindow_C1 (20/09: arrastar ícones entre as grades da loja —
`OnListDragIcon`, `OnListDrop`, `ListAt` no `NwUi`, usadas pelo `SetupWnd`) não estão no DLL do
usuário e não foram portadas.

## O que faz

- **Janelas** (`LineageWindow.ini [Craft] NativeUI=1`; padrão 0, o servidor continua mandando a
  loja em HTML):

  | C2 | aqui | conteúdo |
  |---|---|---|
  | `NCRecipeShopWnd` | `NCRecipeShopWnd` | montagem da loja do anão: "Recipe List" e "Production List", setas, Abort / Message / Start |
  | `NCRecipeBuyListWnd` | `NCRecipeBuyListWnd` | a loja vista pelo cliente: linhas com ícone, nome, custo, taxa; "Creator MP" e adena |
  | `NCRecipeBuyManufactureWnd` | `NCRecipeBuyManufactureWnd` | fabricar numa loja: produto, grau, ingredientes (escurecidos quando faltam), resultado, Create / <PREV / Close |
  | `NCRecipeBookWnd` | `NCRecipeBookWnd` | o livro de receitas (skill Create Item), "(n/limite)", lixeira com confirmação |
  | `NCRecipeManufactureWnd` | `NCRecipeManufactureWnd` | fabricar para si (Create = `mfg_craft`, <PREV = o livro) |
  | `NCRecipeTreeWnd` | `NCRecipeTreeWnd` | "Recipe Information": a árvore de ingredientes, nós com +/−, `(tem/precisa)` |

  240 × 365, título "Item Creation", abrem em (largura − 290, 205) e dividem a posição salva em
  `[CraftUi] RecipeX/RecipeY`; a árvore tem a sua (`RecipeTreeX/RecipeTreeY`, padrão
  largura − 260, 150). Texturas de `SysTextures/c1recipe.utx` (pelo nome, `c1recipe.*`).
- **Transporte**: ao entrar no mundo, `mfg_ui 1`; o servidor passa a mandar as visões de
  `C1Manufacture` a esse jogador como mensagens de dados (`NpcHtmlMessage` cujo texto começa com
  `L2W mfg`), consumidas aqui — nunca viram página; as ações das janelas voltam como os bypasses
  `mfg_*` de sempre, pela fila única do `ManufactureCmd` (menus: uma a cada 120 ms).
- **Balão**: dono de loja privada tipo 5 (manufatura) aparece com `<Private Workshop - Craft>` em
  ciano, mantendo a segunda linha (o título da loja); o original mostrava "<Private Store - Buy>".
- `[Craft] TextureTest=1` carrega as 14 texturas uma vez e registra no log (`craftui:`).

## Como as janelas são feitas

No NWindow_C1 o `NwUi` montava as janelas por fora: objeto `NCWnd`/`NCScrollWnd` do original com
uma cópia da vtable original carregando os overrides, o estado num objeto C++ à parte
(`find_window(self)`). Aqui são classes de verdade sobre as nossas:

- `NCRecipeWnd : NCWnd` (abstrata) — o `Window` + `RecipeWnd` do NWindow_C1: overrides de
  `WindowProc` (comandos e respostas dos diálogos), `OnCreate` (barra de título `NCFrameCtrl(1)`,
  id `0x7AF0`, depois os controles da janela), `OnDestroy`, `ShowWindow` (fechada continua
  escondida quando o desktop mostra tudo), `HideWindow` (o X com o desktop visível = fechar pelo
  usuário; Alt+H e sair do mundo escondem o desktop antes e mantêm a janela aberta), `OnPaint`,
  `OnMouseWheel` (vai para a lista sob o cursor); posição no `LineageWindow.ini`; os diálogos são
  os do desktop (`NDialogInfo` + `NConsoleWnd::ShowDialog`: número com teclado, texto, sim/não).
- `NCRecipeListWnd : NCScrollWnd` — o `ScrollList`: linhas ou grade de ícones que a janela dona
  pinta, clique / duplo clique (0x203), tool tip no passe `NWM_DRAWTOOLTIP`, roda = uma linha.
- As seis janelas derivam de `NCRecipeWnd`; `NCRecipeBuyManufactureWnd` deriva de
  `NCRecipeManufactureWnd` (o C2 pinta as duas igual; mudam os dados e os botões).
- São feitas na primeira vez que abrem (no tick, dentro do mundo), filhas do desktop com
  `NWS_VISIBLE`, e depois só mostradas e escondidas, como toda janela original. Não são destruídas
  na sessão (como no NWindow_C1).

São 8 `UClass` novas — precisam do tamanho próprio no `StaticAllocateObject` —, registradas como as
178 (`NW_IMPLEMENT_CLASS`) mas só no `NWindowL2`:

- `NW_DECLARE_L2_CLASS` / `NW_DECLARE_L2_ABSTRACT_CLASS` (sem endereço de `UClass` original) e
  `NW_IMPLEMENT_L2_CLASS` (o `IMPLEMENT_CLASS` + o export `autoclass<Classe>`, como o do UE2; os
  originais têm ordinais fixos no `.def`, estes ficam em 181..188) — `include/features/NwL2Class.h`;
- a lista explícita `tests/standalone/l2_extras.csv` (classe, pai, flags, recurso).

## Onde entra no código

| Ponto | Arquivo | Antes (NWindow_C1) |
|---|---|---|
| `NConsoleWnd::Tick` → `nwf::OnConsoleTick` → `CraftBalloon_OnTick` (instala o remendo uma vez) e `CraftUi_OnTick` (antes do `AutoShot_OnTick`: `mfg_ui 1` sai antes de `ss_ui 1`, como lá) | `NwFeatureTick.cpp` (chamado do `NConsoleWndInput.cpp`) | `InstallCraftBalloon` no attach da DLL; `CraftUi_OnTick` no port do Tick |
| `NConsoleWnd::NpcHtmlMessage` → `CraftUi_OnNpcHtml` (antes do som e da página) | `NConsoleWndEvents.cpp` | thunk ILT do `OnNpcHtmlMessage` no Engine.dll |

- **`OnNpcHtmlMessage`** do Engine só empilha o texto e chama o slot 66 do console
  (`NConsoleWnd::NpcHtmlMessage`): consumir ali é o mesmo que consumir no Engine.
- **`OnUserInfo`** não chama nenhum slot do console, mas é ele que cria o pawn do jogador
  (`eventSpawnPlayerPawn`): um `GetMyUser()` novo (estado 6) é o "primeiro UserInfo de um User
  novo" do NWindow_C1. O AutoShot (menus) usa a mesma regra.
- **`OnRestartResponse` / `OnAcceptLogOut`** só zeravam o User anunciado: aqui sair do mundo
  (estado ≠ 6 e ≠ 7, o carregamento) zera. As janelas fecham com qualquer estado ≠ 6, como no NwUi.
- **O balão é desenhado pelo Engine.dll** (`FDynamicActor::Render`, RVA `0x27D680`: lê o tipo em
  `User+0x1F8`; 1 e 2 → rótulo e cor de venda, o resto → de compra). Continua remendo em tempo de
  execução, os mesmos 6 pontos do NWindow_C1, cada grupo só se os bytes forem os esperados; senão
  fica o original e sai uma linha no log (`craftballoon: ...`).

## Diferenças (e por quê)

- **Classes em vez de vtables clonadas** (acima). O comportamento, os textos, as cores, as
  posições e os ids são os mesmos.
- **Sem STL**: as listas das mensagens são vetores fixos (`CRAFT_MaxBook` 256, `CRAFT_MaxShop`
  160, `CRAFT_MaxItems` 160, `CRAFT_MaxMats` 32, `CRAFT_MaxNodes` 256), bem acima do que o servidor
  manda (40 ofertas, livro de 50 + bônus, árvore de 4 níveis); o parser não depende do Core e roda
  fora do jogo.
- **Instalação no primeiro tick do console** (não no attach): nenhum balão nem mensagem de dados
  existe antes.
- **O anúncio lê o `GetMyUser()`** a cada tick no mundo, em vez do gancho do `OnUserInfo` (acima).
  Confere antes que a viewport tenha o controlador (o `GetMyUser` original não confere).
- **"L2W mfg" só é consumido com `NativeUI=1`**; o NWindow_C1 consumia também com só o AutoShot
  ligado (o servidor só manda dados a quem anunciou, então não muda nada). As outras mensagens
  `L2W ` são do AutoShot (menus).
- **Sem `ss_ui 1`** (é do AutoShot, menus) e **sem `ManufactureCmd_NoteUser`** no balão (a detecção
  de anão que ela alimentava não existe mais: quem decide é o servidor).
- Partes do `NwUi` que nenhuma janela usava ficaram de fora (`AddEdit`/limite de dígitos,
  `EditValue`, `SetEnabled`, `Focus`, texto livre no `AskNumber`).
- Os logs `craftui:` / `craftballoon:` / `mfgcmd:` saem com `[Window] Log=1` (no DLL do usuário
  eram compilados fora). Novo: `craftui: mfg_start not sent (...)` quando a lista não cabe (abaixo).
  Os dados de livro/craft/árvore saem no log como "closed", como no NWindow_C1.

**Achado, corrigido (`0a4c9a0`):** a fila de bypass do NWindow_C1 (`BYPASS_MaxLen` 200) recusava
comando com 200 caracteres ou mais, e o `mfg_start` leva a lista inteira (`id:taxa,...`; ids de
receita até 871): com uns 15 a 20 ofertas o Start não saía — a janela fechava e a loja não abria,
sem aviso (o DLL do usuário tem esse bug). Agora 1024, o `Bypass[1024]` do
`NConsoleWnd::RunHtmlCommand` original (o Network.dll já recebe bypass desse tamanho). E a loja
aceita 40 ofertas (`SHOP_MAX`/`CRAFT_MaxOffers`, conferente rodada 12), o teto do servidor
(`C1Manufacture.MAX_OFFERS`, que guarda as 40 primeiras e descarta o resto calado); o NWindow_C1
aceitava 160. Com 40 o `mfg_start` fica em no máximo 689 caracteres.

**Falhas nos callbacks das janelas** (conferente, rodada 12): como o NwUi, cada chamada da moldura
para a janela (criar os controles, fechar, pintar, comando, respostas de diálogo, célula da lista,
dica, clique) roda sob `try/catch(...)` (`CRAFT_GUARDED` no `CraftWnd.cpp`): uma falha perde a
chamada, não o jogo; a pintura fecha o `EndWndPaint` mesmo assim.

## Ferramentas

- `python tools/verify_standalone.py --dll build/l2/Release/NWindow.dll --l2`: as checagens do
  standalone mais exatamente as classes da lista (exports: os `autoclass` delas depois dos ordinais
  originais; classes: o registro contra a lista — nome, pai, flags, `InternalConstructor` próprio
  ou, na abstrata, o do pai). O check `source` confere sempre as declarações L2: só em
  `include/features`, registradas uma vez em `src/features`, como na lista. Sem `--l2`, classe ou
  export que o original não tem falha, como no build fiel.
- `python tools/smoke_standalone.py --dll build/l2/Release/NWindow.dll --l2` (unificado com o do
  lobby na rodada 13, `docs/notas/F6_Infra.md`):
  - num System próprio (`build/smoke_l2`), confere a tabela de exports: a original, mais os
    `autoclass` das extras logo depois, 181..188;
  - o `nwd_smoke --extras` tira as extras das contagens e conferências do original e as mostra em
    linhas `l2 extra`, conferidas contra a lista;
  - o resto da saída é comparado com a do original;
  - relê os remendos em tempo de execução (`build/smoke_l2_patches`). Os 6 pontos do balão só são
    remendados no primeiro tick do console; fora do jogo confere que os bytes são os que o
    install dele espera.
- `build/craft/Release/nwd_craft_test.exe`: o parser nas mensagens como o `C1Manufacture` as monta
  (as 7 visões, CRLF, chave desconhecida, 64 bits, capacidade, texto longo) — 38 checagens.
- `tools/cycle.py`: `l2: build`, `l2: craft data`, `l2: verify` e `l2: smoke` (com `--l2`): todos bloqueiam.

## Teste no jogo (System de teste, com o servidor local)

`LineageWindow.ini` ao lado do `NWindow.dll`: `[Craft] NativeUI=1`, `[Window] Log=1` para o log;
`SysTextures/c1recipe.utx` presente; servidor com `mfg_` em `AltSimpleBypassAllowed`.

1. Entrar no mundo: no log, `craftui: announced native craft UI` (e o `autoshot: announced` logo
   depois, se `[Shots] AutoShot=1`).
2. Anão, ação 37: a janela de montagem abre (livro e ofertas); duplo clique numa receita → preço
   no teclado → vai para "Production List"; setas; Message (texto); Start → a loja abre, o anão
   senta e o balão diz `<Private Workshop - Craft>` em ciano com o título embaixo (visto por outro
   personagem também). Abort e o X fecham sem abrir.
3. Outro personagem clica no anão: lista (linhas, custo, taxa, MP, adena); clique → fabricar;
   Create com e sem materiais/adena (a mensagem do servidor vai para o chat, o resultado aparece
   na janela); <PREV volta à lista; o botão da árvore abre/fecha "Recipe Information" (+/−).
4. Skill Create Item: o livro, contador, duplo clique → fabricar para si; lixeira com confirmação.
5. Posições: mover, fechar, reabrir (voltam ao lugar; `[CraftUi] RecipeX/Y`, `RecipeTreeX/Y`);
   Alt+H com a janela aberta (some e volta); Esc fecha a da frente; restart e entrar de novo
   (anúncio de novo).
6. Tool tips das grades e das linhas; roda do mouse nas listas.

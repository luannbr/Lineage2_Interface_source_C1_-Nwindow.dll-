# Como modificar a interface

**Português** · [English](MODIFY_INTERFACE.md) · [Русский](MODIFY_INTERFACE.ru.md)

Guia prático para mudar a interface do C1 nesta source: como as janelas são montadas, como os recursos da
fase 6 estão ligados e receitas passo a passo (barra de atalhos nova, botão, janela, cor de chat, linha de
tooltip). Toda mudança de interface vai no build **`NWindowL2`** (`NW_FEATURES=1`); o modo hook e o
`NWindowStandalone` continuam fiéis ao original e são onde os juízes comparam com ele.

## 1. Como as janelas são montadas

### A hierarquia

| Classe | Arquivo | Papel |
|---|---|---|
| `NCObject : UObject` | `include/nwindow/NCObject.h` | base de tudo; `FindTexture(TEXT("Pacote.Grupo.Nome"))` carrega uma textura |
| `NCWnd` (0x94 bytes) | `include/nwindow/NCWnd.h` | toda janela: posição (`LocalX/Y/W/H` relativos ao pai, `ScreenX/Y`, `Width`, `Height` absolutos), estilo, filhas, foco, barra de título (`FrameCtrl`), ícone minimizado |
| `NControl : NCWnd` (0x1A0) | `include/nwindow/NControl.h` | todo controle; tem `ID` e avisa o pai com `NWM_COMMAND` (wParam = `ID`) |
| `NCButton`, `NCPushButton`, `NCExButton`, `NCRadioButton` | `include/nwindow/NCButton.h` | botões com textura normal/apertada (`SetNormalTex`, `SetPushedTex`, `SetText`, `SetTextID`) |
| `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCProgressCtrl`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`... | `include/nwindow/` | os outros controles |
| `NC*Wnd` | `include/nwindow/` | as janelas do jogo (inventário, chat, barra de atalhos, status...) |
| `NCVirtualWndMain` | `include/nwindow/NCVirtualWndMain.h` | dono de todas as janelas: registro, ordem Z, teste de clique, foco, despacho das mensagens, cursores |
| `NConsoleWnd : UL2ConsoleWnd` | `include/nwindow/NConsoleWnd.h` | o console da engine (285 slots: os eventos de rede e de UI); `GConsole` |
| `NCConsole` | `include/nwindow/NCConsole.h` | o **desktop** do jogo (`GConsole->GetDesktop()`): pai de todas as janelas in-game, com um ponteiro para cada uma (`MainWnd`, `InventoryWnd`, `ShortCutWnd`, `ChatWnd`...) |

### Criar uma janela ou controle

O padrão do código original (por exemplo `NCConsole::OnCreate` em `src/nwindow/NCConsole.cpp`, ou
`NCSystemMenuWnd::OnCreate` em `src/nwindow/NCSystemMenuWnd.cpp`):

```cpp
Button = new NCButton(TEXT("L2UI.Control.DefaultButton"), TEXT("L2UI.Control.DefaultButton_click"), 0);
Button->ID = NID_ALGUMA_COISA;          // o que chega no NWM_COMMAND do pai
Button->SetWindowRect(43, 43, 67, 19);  // relativo ao pai
CreateChild(Button, NWS_VISIBLE);       // registra, faz a posição absoluta, manda WM_CREATE -> OnCreate
Button->SetTextID(0x91);                // texto do SysString do cliente
```

- `new` de uma classe do pacote é o `operator new` do UE2 (o objeto é um `UObject`, criado com o tamanho da
  `UClass` registrada).
- `CreateChild(filho, estilo)` (`NCWnd`, inline) = `AddChildWindow` + `Entry` + posição de tela + `WM_CREATE`. O
  desktop usa também a variante escondida `CreateHiddenChild` (`AddChildWindow`, `HideWindow`, `AttachEntry`).
- Estilos (`NWS_*` em `NCWnd.h`): `NWS_VISIBLE`, `NWS_TOPMOST` (lista à parte, por cima), `NWS_TITLEBAR` (o
  `OnCreate` de `NCWnd` cria uma barra de título `NCFrameCtrl`), `NWS_MINIMIZABLE` (ícone ao minimizar),
  `NWS_NOCLOSE`.
- Destruir: no `OnDestroy` do dono, `RemoveChildWindow(Filho->Entry); delete Filho;` (veja
  `NCSystemMenuWnd::OnDestroy`). O `NCWnd::OnDestroy` só destrói a barra de título e o ícone.

### Mensagens, pintura, tooltips

- Tudo chega por `WindowProc(Msg, wParam, lParam)` (vslot 24) com números de mensagem do Win32; o `NCWnd`
  distribui para os virtuais: `OnCreate` (35), `OnDestroy` (36), `OnPaint(NCGDevice*)` (50), `OnDrawToolTip` (51),
  `OnKeyDown`, `OnMouseMove`, `OnLButtonDown/Up`, `OnRButtonDown/Up`, `OnMove` (`WM_MOVE`), `OnMouseWheel`.
- Um controle avisa o pai com `NWM_COMMAND` (`0x501`), `wParam` = `ID` do controle. O pai trata no seu
  `WindowProc` (exemplo: `NCSystemMenuWnd::WindowProc`).
- Pintura (`OnPaint`) com o `NCGDevice` (`include/nwindow/NCGDevice.h`): `SetClip`, `BeginPaint(ScreenX,
  ScreenY)`, `DrawTexture(x, y, w, h, u, v, ul, vl, Tex)`, `DrawStretchedTex`, `DrawRect`, `DrawLine`,
  `DrawText(...)`, `EndPaint`, `ReleaseClip`. As texturas são carregadas uma vez no `OnCreate` com `FindTexture`
  (pacotes `L2UI.*` do cliente).
- Tooltip: `OnDrawToolTip(NCGDevice*)` da janela sob o mouse, chamado depois da pintura.

### Console e desktop

`NConsoleWnd` (o objeto que a engine cria a partir do `L2.ini`) recebe os eventos do servidor e do jogo (os 285
slots) e tem o desktop `NCConsole`. O `NCConsole::OnCreate` cria todas as janelas do jogo (posições a partir do
tamanho da tela) e o `NCConsole::OnDestroy` as destrói. O `NConsoleWnd::Tick` roda a cada quadro.

## 2. Como os recursos da fase 6 estão ligados

| Peça | Arquivo | O que faz |
|---|---|---|
| `NW_FEATURES` | `include/features/NwFeatures.h` | só o `NWindowL2` define 1; todo código de recurso fica em `#if NW_FEATURES` |
| `nwf::OnDllMain` | `src/features/NwFeatureStartup.cpp` | chamado pelo nosso `DllMain` (`src/standalone/NWindowPackage.cpp`) no attach/detach: no attach, instala o que mexe em outros módulos (WindowFix, ReturnToLogin, Discord) |
| `nwf::OnConsoleTick` | `src/features/NwFeatureTick.cpp` | primeira coisa do `NConsoleWnd::Tick`, todo quadro, sob um tratador de exceção só (combat log, craft, AutoShot, ReturnToLogin, fila de bypass, CacheCleaner) |
| `nwf::IniInt/IniString/IniWriteInt/IniWriteString` | `src/features/NwFeatureConfig.cpp` | configuração: registro `HKCU\Software\Lineage2C1\NWindow\<Seção>`, depois a tabela compilada `GBuiltIn`, depois o padrão |
| `nwf::Log(tag, ...)` | `src/features/NwFeatureConfig.cpp` | `LineageWindow.log`, só com `-DNW_FEATURE_LOG=ON` |
| `nwf::SendBypassToServer`, `SendShortCutReg`, `SendShortCutDel` | `include/features/ManufactureCmd.h` | pedidos ao servidor pelo protocolo de fábrica (bypass numa fila, um a cada 120 ms) |
| `NW_DECLARE_L2_CLASS` / `NW_IMPLEMENT_L2_CLASS` | `include/features/NwL2Class.h` | `UClass` só do L2, para janelas com campos próprios; cada uma é uma linha em `tests/standalone/l2_extras.csv` |

Pontos de chamada no código decompilado (sempre `#if NW_FEATURES` + `// phase 6: ...`):

| Recurso | Onde entra |
|---|---|
| AutoShot | `NCShortCutWnd.cpp` (`OnRButtonDown`, `OnLButtonUp`, `OnPaint`), `NConsoleWndEvents.cpp` (mensagens de dados no `NpcHtmlMessage`) |
| BuffTimer | `NCAbnormalStatusWnd.cpp` (contagem nos ícones, linha no tooltip) |
| ChatColors | `NConsoleWndEvents.cpp`, `NConsoleWnd::Say2` (o último `else`) |
| CombatLog | `NCChatWnd.cpp` (desvia as linhas de combate) + o tick |
| ItemHoverTip | `OnDrawToolTip` de `NCItemWnd.cpp`, `NCEquipItemWnd.cpp`, `NCShortCutWnd.cpp` |
| ManufactureCmd | `NCommandParser.cpp` (`/dwarvenmanufacture`), `NCActionItemWnd.cpp` e `NCShortCutWnd.cpp` (ações 32..52) |
| MinimapHotkey | `NConsoleWndInput.cpp` (Alt+M / M) |
| ReturnToLogin | `NCDialogBox.cpp`, `NConsoleWndLogin.cpp` + o tick + o `DllMain` |
| WindowFix | `NCVirtualWndMain.cpp`, `NCWnd.cpp`, `NConsoleWndInput.cpp` (posição do mouse), `NCVideoOptionWnd.cpp`, `NConsoleWndWindows.cpp` (tela cheia) + o `DllMain` |
| Janelas de craft | criadas sob demanda no desktop (`src/features/craft/CraftUi.cpp`: `new NCRecipe...Wnd`, `Create(GConsole->Desktop)`) + o tick |

As notas de cada recurso estão em `docs/notas/F6_*.md`.

## 3. Receita: uma barra de atalhos nova (como no C5 / Interlude)

Esta receita descreve a abordagem que já foi feita e **testada no jogo** num experimento separado (ramo
`experiment/extra-shortcut-bars` do repositório de desenvolvimento; não está nesta source). O estudo está em
`docs/notas/BarrasExtras_Atalhos.md`.

**A ideia (a do C5):** a barra extra **não tem slots próprios**. É mais uma instância de `NCShortCutWnd` que
mostra outra página da mesma tabela de 100 atalhos (10 páginas de 10). O slot de um atalho é
`página × 10 + posição`, um número plano que o servidor guarda e devolve; uma barra que só mostra outra página
não muda nada no protocolo, no servidor nem nas texturas. A classe não muda de tamanho e nenhuma `UClass` nova
é criada.

**Passos:**

1. **Arquivos do recurso:** `include/features/ShortCutBars.h` e `src/features/ShortCutBars.cpp`, em
   `namespace nwf` e `#if NW_FEATURES`, com funções como `ShortCutBars_Create(NCWnd* Desktop, NCShortCutWnd* Main)`,
   `ShortCutBars_Destroy`, `ShortCutBars_OnTick`, `ShortCutBars_OnCommand`, `ShortCutBars_OnMove`,
   `ShortCutBars_OnDropOnOtherBar`, `ShortCutBars_OnPaint`. O estado (as barras, o botão) fica em globais do
   `.cpp`, nunca em campos novos de `NCShortCutWnd`.
2. **Criar** em `NCConsole::OnCreate`, logo depois de a barra principal ser criada
   (`ShortCutWnd = new NCShortCutWnd(10, 10, 32)` ... `CreateHiddenChild(...)`): para cada barra extra,
   `new NCShortCutWnd(Main->Cols, Main->Pages, Main->IconSize)`, `SetWindowRect`, e o mesmo registro que o
   desktop faz (`AddChildWindow` com `NWS_VISIBLE | 0x20 | NWS_TOPMOST`, `HideWindow`, `AttachEntry`). O
   `OnCreate` de fábrica criou tabelas próprias: libere-as (`delete[] Bar->Items`, `delete[] Bar->Keys`) e aponte
   para as da principal (`Bar->Items = Main->Items`, `Bar->Keys = Main->Keys`). `Bar->Page` = a página inicial
   (no C5 a barra 2 começa na página 2 e a 3 na página 3). Esconda o `RotateButton` das extras (a principal gira
   todas).
3. **Destruir** em `NCConsole::OnDestroy`, **antes** de `DestroyChild(this, ShortCutWnd)`: nas extras, ponha
   `Items` e `Keys` em `NULL` (são da principal; senão o `OnDestroy` de fábrica as libera duas vezes),
   `RemoveChildWindow(Bar->Entry)` e `delete Bar`.
4. **Seguir a principal** no fim do `NCShortCutWnd::Tick` (só a principal tem tick): para cada extra, a mesma
   orientação (`bHorizontal`, e `UpdateLayout()` quando muda), a posição (em cima de uma barra horizontal, à
   esquerda de uma vertical, do outro lado quando falta tela; 44 × 417 cada) com `SetWindowRect`, e visível só
   quando a principal está visível.
5. **Botão de expandir** (opcional): um `NCPushButton` filho da principal (`CreateChild`), com um `ID` novo depois
   dos de fábrica (os da barra vão até `0xFF000008`, `NID_SHORTCUT_ROTATE`), tratado no ramo `NWM_COMMAND` do
   `NCShortCutWnd::WindowProc`. O C1 não tem arte própria para isso: o experimento usou as setas de página
   (`L2UI.ShortcutWnd.UpBtn1/2`, `LeftBtn1/2`...). Guarde quantas barras aparecem com `IniWriteInt`.
6. **Arrastar entre barras:** em `NCShortCutWnd::OnLButtonUp`, o ramo de fábrica que **apaga** o atalho solto
   fora da barra (`RequestShortCutDel`) passa a verificar antes se o ponto está sobre outra barra; se estiver,
   move o atalho para o slot de lá (`SlotAt`) com `SendShortCutReg`/`SendShortCutDel` (troca se o slot estiver
   ocupado), e solto sobre uma barra mas fora de um slot, deixa como está.
7. **Arrastar a alça de uma extra move o grupo:** no começo do `NCShortCutWnd::WindowProc`, um `WM_MOVE` de uma
   barra extra é repassado à principal. Antes, copie para o `FrameCtrl->GrabX/GrabY` da principal o ponto de
   pega da extra somado ao deslocamento dela em relação à principal (pelo layout calculado, não pelas posições
   atuais, que atrasam um quadro); sem isso o `NCWnd::Snap` puxa o grupo e o mouse "pula".
8. **Arte:** os fundos `L2UI.ShortcutWnd.Back` (vertical) e `.Back1` (horizontal) já trazem o ícone de girar
   desenhado na textura. Nas extras não há botão por cima: no `NCShortCutWnd::OnPaint` (fim), cubra essa célula
   com uma cópia 1:1 da célula vazia vizinha da mesma textura (`DrawTexture` entre `BeginPaint`/`EndPaint`).

**O que vale saber:** tooltip, clique para usar, AutoShot e arrastar do inventário/skills funcionam sem mudança,
porque é a mesma classe e tudo é calculado a partir de `Page`. As teclas (F1..F10, páginas, joypad) ficam só na
barra principal, como no C5. O filtro de pacotes do servidor pode descartar registros de atalho rápidos demais.
Só o jogo mostra problemas de posição e de arte: o experimento precisou de três ajustes depois do primeiro teste
(o ícone na arte, a alça de arrastar, o ponto de pega).

## 4. Receita: um botão novo numa janela existente

Exemplo: um botão a mais no menu do sistema (`NCSystemMenuWnd`).

1. `include/features/MeuBotao.h` / `src/features/MeuBotao.cpp` (`namespace nwf`, `#if NW_FEATURES`):
   `MeuBotao_Create(NCWnd* Dono)`, `MeuBotao_OnCommand(NCWnd* Dono, WPARAM ID)`, `MeuBotao_Destroy(NCWnd* Dono)`;
   o ponteiro do botão fica numa global do `.cpp`.
2. Criar no fim de `NCSystemMenuWnd::OnCreate`:
   `#if NW_FEATURES` / `// phase 6: MeuBotao ...` / `nwf::MeuBotao_Create(this);` / `#endif`. Dentro:
   `new NCButton(normal, apertada, 0)`, um `ID` que nenhum controle daquela janela usa (veja o enum `NID_*` dela),
   `SetWindowRect` dentro da área da janela, `Dono->CreateChild(Botao, NWS_VISIBLE)`, `SetText`.
3. Tratar o clique no ramo `NWM_COMMAND` de `NCSystemMenuWnd::WindowProc`:
   `if (nwf::MeuBotao_OnCommand(this, wParam)) return 1;` (em `#if NW_FEATURES`).
4. Destruir no `NCSystemMenuWnd::OnDestroy`, antes de `NCWnd::OnDestroy()`: `RemoveChildWindow(Botao->Entry);
   delete Botao;`.
5. Se a janela precisa ficar maior, mude o `SetWindowRect` dela no `NCConsole::OnCreate` e a pintura do fundo no
   `OnPaint` — sempre em `#if NW_FEATURES`, deixando o ramo de fábrica intacto.

## 5. Receita: uma janela nova

1. Uma janela com campos ou virtuais próprios é uma classe nova, registrada só no L2:
   `class NCMinhaWnd : public NCWnd { NW_DECLARE_L2_CLASS(NCMinhaWnd, NCWnd, 0) ... };` no header e
   `NW_IMPLEMENT_L2_CLASS(NCMinhaWnd, NCWnd, 0)` no `.cpp` (`include/features/NwL2Class.h`).
2. Acrescente a linha em `tests/standalone/l2_extras.csv` (`class,super,class_flags,feature`): o
   `verify_standalone.py --l2` e o `smoke_standalone.py --l2` aceitam **exatamente** essas classes a mais.
3. Sobrescreva os virtuais de que precisa: `OnCreate` (chame `NCWnd::OnCreate()` primeiro, carregue texturas com
   `FindTexture`, crie a barra de título e os controles), `OnDestroy` (destrua os filhos, depois
   `NCWnd::OnDestroy()`), `WindowProc` (`NWM_COMMAND`), `OnPaint`.
4. Crie-a no desktop quando precisar (as janelas de craft fazem isso sob demanda:
   `SetWindowRect(0, 0, L, A)`, `GConsole->Desktop` como pai em `CreateChild(this, NWS_VISIBLE)`, `HideWindow()`),
   e mostre com `ShowWindow()` + `Desktop->BringChildToTop(Entry)`.
5. Modelo completo: `include/features/craft/CraftWnd.h`, `src/features/craft/CraftWnd.cpp` (`NCRecipeWnd`: barra
   de título decorada `NCFrameCtrl(1)`, botões, listas com rolagem, tooltip, posição guardada no registro).

## 6. Receita: uma cor de chat nova

O servidor não manda cor: manda o **tipo** do chat (`Say2`), e o cliente escolhe a cor em `NConsoleWnd::Say2`
(`src/nwindow/NConsoleWndEvents.cpp`). Os tipos que o C1 não conhece caem no último `else` (branco), onde a fase
6 consulta `nwf::ChatColors_Find(Type, &Color)`.

1. Acrescente o tipo e a cor (ARGB, ex. `0xFF408CFF`) no `switch` de `src/features/ChatColors.cpp`.
2. A linha continua entrando como chat normal (aba All, filtro Normal). Não reaproveite tipos que já têm efeito
   colateral (o TELL põe o nome na lista de tells).
3. Mudar a cor de um tipo que o C1 **já** colore é mudar um ramo fiel: faça-o em `#if NW_FEATURES` dentro de
   `Say2`, mantendo a cor original no ramo `#else`.
4. Detalhes e as cores do Interlude/Essence: `docs/notas/F6_ChatColors.md`, `docs/notas/ChatColors_Essence.md`.

## 7. Receita: uma linha nova no tooltip de item

O tooltip completo do item é desenhado pelas rotinas de `include/nwindow/NItemToolTip.h`, que as grades de itens
incluem: `SetTexts`/`SetStoreTexts` preenchem `ToolTipName` e `ToolTipText` (o texto que **mede** a caixa) e
`DrawLines` desenha as linhas, 13 pixels uma da outra (`DrawTypeLines`, depois `DrawTailLines`, que termina com a
descrição).

1. Acrescente a linha nos dois lugares, em `#if NW_FEATURES`: no texto que mede (para a caixa crescer) e no
   desenho (por exemplo em `DrawTailLines`, antes da descrição, com `DrawLabel`/`DrawValue` e `Y += 13`).
2. As rotinas são modelos instanciados por cada grade com parâmetros diferentes (inventário, paperdoll, troca,
   armazém, lojas privadas): confira cada cópia que você muda.
3. Para tooltips de outras janelas, o modelo é o BuffTimer (`nwf::BuffTimer_ToolTipExtraHeight` e
   `BuffTimer_DrawToolTipTime`, chamados de `NCAbnormalStatusWnd.cpp`): um gancho dá a altura a mais, o outro
   desenha a linha. A dica curta sob o mouse é o `ItemHoverTip` (`F6_ItemHoverTip.md`).

## 8. O ciclo de compilar e testar

```bat
cmake --build build --config Release --target NWindowL2
python tools\cycle.py --system "C:\...\Lineage II\System - Teste" --no-install
```

1. Compile o `NWindowL2` até não haver erro nem aviso novo.
2. Rode o ciclo completo: ele prova que o modo hook e o standalone fiel não mudaram (os juízes comparam com o
   original) e que o `NWindowL2` passa no `verify_standalone --l2` e no `smoke_standalone --l2`.
3. Teste no jogo: feche o jogo, guarde o `NWindow.dll` de uma **cópia** da pasta `System` e copie por cima o
   `build\l2\Release\NWindow.dll`. O ciclo não desenha a interface: posição, arte e arrastar só o jogo mostra.
4. Para diagnosticar, compile com `-DNW_FEATURE_LOG=ON` e leia o `LineageWindow.log` ao lado do DLL.

## 9. Regras de segurança

- Nunca acrescente campos nem slots virtuais novos a uma classe original: o tamanho dela está registrado na
  engine e o código original indexa nela e na vtable. Sobrescrever um slot que a classe já herda é permitido (o
  AutoShot sobrescreve `OnRButtonDown` em `NCShortCutWnd`, em `#if NW_FEATURES`). Estado do recurso vai em globais
  do `.cpp` ou numa classe L2 nova.
- No código decompilado, só o ponto de chamada, em `#if NW_FEATURES` com `// phase 6: ...`; o ramo de fábrica
  fica como está.
- Toda chamada a outro módulo remendado em tempo de execução confere os bytes antes e desliga o recurso com log
  se não baterem.
- Configuração pelo `nwf::Ini*` (registro), nunca arquivos ao lado do DLL.
- Mesmo protocolo do cliente original: o servidor não muda; respeite os limites do filtro de pacotes dele.
- Teste numa cópia do cliente, nunca na pasta em que se joga; nenhum binário do jogo entra no repositório.
- Commits pequenos, um assunto cada, com o ciclo verde.

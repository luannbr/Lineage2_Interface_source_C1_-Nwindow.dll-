# Fase 6 — MinimapHotkey (Alt+M / M)

Origem: NWindow_C1 `src/inject/MinimapHotkey.cpp` (HEAD `4e56326`), doc `MINIMAP_HOTKEY.md`.
Aqui: `include/features/MinimapHotkey.h`, `src/features/MinimapHotkey.cpp`.

## O que faz

Alt+M, e M sozinho quando as letras são atalhos, abrem e fecham a janela do mapa
(`NCConsole::MinimapWnd`), como no C3. O C1 original só abre o mapa por um item de mapa na barra ou
pelo servidor. Na primeira vez (id do mapa 0) passa pelo `ShowMapItem(MINIMAP_Aden)` — o
`NCMinimapWnd::OnPaint` divide por um valor não inicializado com o id 0 —; depois,
`NCConsole::ToggleMinimap`, com o som original. A repetição da tecla segurada é engolida sem virar
o mapa de novo.

`LineageWindow.ini` `[Hotkeys]`: `Minimap` (1) liga tudo, `MinimapBareKey` (1) o M sozinho,
`MinimapGmPetition` (0) devolve ao GM (nível 1..6) o Alt+M original (janela de petição).

## Onde entra no código

As duas chamadas ficam nos `switch` do `NConsoleWnd::HookShortCutKey` (`NConsoleWndInput.cpp`):

- `WM_SYSKEYDOWN`, `case 'M'`: `MinimapHotkey_AltM` antes da petição do GM. O ramo de Alt do
  original já devolve 1 para qualquer tecla, como o desvio do NWindow_C1 fazia antes dele.
- `WM_KEYDOWN`, no `switch` das letras (chat pede Enter e nenhum controle quer o teclado):
  `case 'M'`: `MinimapHotkey_BareM`. É exatamente onde o original devolvia 0 para o M, e a regra
  que o NWindow_C1 conferia depois (`GIsEnterChatting` e o `vfn032` da janela com foco).

Antes: o `call` único de `0x10055abc` (no `DispatchWndMsgX`) redirecionado para um invólucro do
`FUN_1004f050`.

## Diferenças

Nenhuma de comportamento. As chaves do `.ini` são lidas uma vez, no primeiro uso (o NWindow_C1 lia
no attach).

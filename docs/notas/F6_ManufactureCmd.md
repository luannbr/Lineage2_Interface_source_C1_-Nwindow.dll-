# Fase 6 — ManufactureCmd (`/dwarvenmanufacture`, ações 37 e 52, pedidos ao servidor)

Origem: NWindow_C1 `src/inject/ManufactureCmd.cpp` (HEAD `4e56326`), docs
`MANUFACTURE_37_ACTIONID_BUG.md`, `MANUFACTURE_37_REMOVE_CLIENT_DWARF_GATE.md`,
`SERVITOR_RETURN_ACTION_52.md`, `REQUEST_ACTION_USE_37_SERVER.md`. Aqui:
`include/features/ManufactureCmd.h`, `src/features/ManufactureCmd.cpp`.

## O que faz

- **`/dwarvenmanufacture`** (a tabela de comandos do C1 não tem): manda `RequestActionUse(37, 0, 0)`
  e a linha não vira comando. Sem checagem de raça no cliente: o servidor responde "Only Dwarves
  can open a Manufacture store." (`MANUFACTURE_37_REMOVE_CLIENT_DWARF_GATE.md`).
- **Ações 32 .. 52** na barra de atalhos (clique e tecla) e na janela de ações: o despacho original
  para no id 31. 37 (Dwarven Manufacture) e 52 (Return do servitor) vão como
  `RequestActionUse(id, 0, 0)`; os outros ids desse intervalo, como o grupo `RequestActionUse` do
  original (com Ctrl e Shift) — o NWindow_C1 apontava as entradas novas da tabela para o caso do
  id 0. Acima de 52, nada (como acima de 31 no original).
- **Pedidos que os recursos mandam**: `SendRequestActionUse`, `SendBypassToServer` (fila de 8, uma
  a cada 120 ms — o servidor descarta bypass a menos de 100 ms do anterior; `BypassFlush` no tick),
  `SendShortCutReg` / `SendShortCutDel`. O clan (craft: `mfg_*`) usa o `SendBypassToServer` daqui.

## Onde entra no código

| Ponto | Arquivo | Antes (NWindow_C1) |
|---|---|---|
| `NCommandParser::GetCommand` → `ManufactureCmd_OnCommand` | `NCommandParser.cpp` | desvio no `FL2GameData::GetCommandType` do Engine.dll |
| `UseAction` da barra (clique e F1..F12) → `ManufactureCmd_UseAction(id, 0)` | `NCShortCutWnd.cpp` | tabelas de salto em `0x100aa050`/`0x100aa914`, `cmp 0x1F` → `0x34` |
| `NCActionItemWnd::UseAction` → `ManufactureCmd_UseAction(id, 1)` | `NCActionItemWnd.cpp` | tabela de remapeamento `0x10034cb4` e de casos `0x10034c7c` |
| `NConsoleWnd::Tick` → `BypassFlush` | `NConsoleWndInput.cpp` | idem, no port do Tick |

O `GetCommand` chama o `SetString` duas vezes quando o comando não é o nosso: ele recomeça o
parser (copia a linha para o buffer interno, sem mexer nela), então o original recebe o mesmo
primeiro token. Os dois chamadores (`NCChatWnd::OnEditCommand` e `NCGMPetitionWnd::OnEdit`) são os
mesmos que o desvio do NWindow_C1 pegava — o `GetCommandType` só é importado pelo NWindow.

## O que ficou de fora e por quê

- **Captura do `UNetworkHandler`** (desvios no Network.dll: `RequestActionUse`, compra/venda de
  loja privada, `Tick`): aqui o handler é `GConsole->Engine->NetHandler`, o mesmo que as chamadas
  originais usam. O log `RAU id=...` dos 24 primeiros `RequestActionUse` era só diagnóstico (fora
  do DLL do usuário, `LW_DIAG_ON` = 0).
- **Detecção de anão** (desvio no `User::GetClassNamePointer`, `IsLocalDwarf`,
  `ManufactureCmd_NoteUser`): o estado só era lido pelo `should_block_manufacture`, que ninguém
  chama desde que o bloqueio foi para o servidor. O `CraftBalloon` do NWindow_C1 chama
  `ManufactureCmd_NoteUser`; no port do clan essa chamada pode sair.

## Diferenças

- Sem "open Buy once to capture NH": o envio funciona desde a primeira ação.
- Os logs (`mfgcmd: ...`) saem com `[Window] Log=1`; no DLL do usuário eram compilados fora.

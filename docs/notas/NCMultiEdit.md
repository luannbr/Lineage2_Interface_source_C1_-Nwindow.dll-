# NCMultiEdit — notas de análise (adiado em 25/09/2026)

Editor de várias linhas com quebra automática (UClass "CMultiEdit", 0x264, pai `NControl`,
vtable `0x100dd060`, 72 slots). Usado por `NCHtmlMultiEdit` (caixas `<multiedit>` dos formulários
HTML: quadro de avisos, petição) e por `NCMatchOptionWnd`. ~50 funções, ~32 KB. Adiado porque quase
não é usado em jogo; retomar junto com `NCHtmlMultiEdit`.

## Estruturas auxiliares

**NStringArray** (nome nosso), 0xC bytes, em `NCMultiEdit+0x1AC`: `{INT Num; NString* Data; INT Max}`.
- `0x10022130` construtor: zera e chama `InsertAt(-1)` (começa com uma string vazia).
- `0x10021f70` destrutor: `jmp Empty`.
- `0x10021f00` `Empty()` (retorna 0): `Empty()` em cada `NString`, `delete[] Data`, zera tudo.
  O Ghidra arquivou esse corpo sob `Unwind@100cc568`; está em `db/extra_functions.csv`.
- `0x10021f80` `InsertAt(Index)` (-1 = no fim): primeira vez aloca `new NString[128]`; cresce de
  128 em 128 quando `Num >= Max - 1`. **Bug do original:** ao crescer copia só `Num` *bytes*
  (`appMemcpy(New, Old, Num)`) e depois destrói os antigos — passou de 127 elementos, os textos se
  perdem. `memmove` de `(Num - Index + 1)` elementos e `appMemset` do novo.

**NLineNode** (nome nosso), 0x14 bytes: `{FString Text; NLineNode* Prev (+0xC); NLineNode* Next (+0x10)}`.
- `0x1001b390` construtor (`FString()`, Prev = Next = NULL, `Text.Empty()`).
- `0x1001b3e0` destrutor (Prev = Next = NULL, `Empty`, `~FString`). Alocado com `new` (GMalloc 0x14).

**NLineList** (nome nosso), 0x18 bytes, em `NCMultiEdit+0x1B8`:
`{INT Count; INT Unknown4; NLineNode* Head (+8); NLineNode* Current (+0xC); NLineNode* Tail (+0x10); INT CurIndex (+0x14)}`.
- `0x1001bc20` construtor: Count = 1, um nó novo em Head/Current/Tail.
- `0x10020a20` destrutor: `Clear()` e apaga o nó da cabeça.
- `0x1001bd10` `GetLine(i)`: devolve o nó (o `FString` está no início), movendo o cursor em cache.
  Quatro caminhos: da cabeça (correto), da cauda (correto), para trás ou para frente a partir do
  cursor. **Bug do original:** nesses dois últimos o limite do laço é recalculado com o índice que
  anda (`j < CurIndex - i`), então só percorre metade da distância: ±1 linha funciona, ±2 ou mais
  cai na linha errada.
- `0x1001e000` `RemoveAt(i)`, `0x1001e0c0` `Clear()` (RemoveAt(0) até vazio), `0x1001de60`
  `Insert(Index = -1)` (parâmetro na pilha; o Ghidra o confunde com o estado de EH),
  `0x1001bca0` `TotalLength()` (soma dos `Len()`, anda da cabeça).
- `0x10033ac0` getter dobrado (`return *this` = Count) — **os chamadores do original contam que ele
  preserva ecx/edx** (LTCG); fazer os chamadores juntos (o `regcheck` aponta).
- `0x1001b380` cópia fora de linha de `FString::Len()`.

## Campos do NCMultiEdit (parcial)

| off | campo | | off | campo |
|---|---|---|---|---|
| 0x1A0 | NCIMEManager* | | 0x204 | CaretRow (linha de tela) |
| 0x1A4 | UTexture* "NWindow.chatback" | | 0x208 | CaretX (px na linha de tela) |
| 0x1A8 | UTexture* "NWindow.WhiteTexture" | | 0x20C | caret dentro da composição |
| 0x1AC | NStringArray | | 0x214/0x216 | WORDs |
| 0x1B8 | NLineList Lines | | 0x218 | largura de "가" |
| 0x1D0 | FString Comp (composição) | | 0x21C | altura de "가" |
| 0x1DC | TCHAR* texto de `GetText` (GMalloc) | | 0x220 | ? |
| 0x1E0 | altura de "가" | | 0x224/0x228/0x22C | botão cima, baixo, barra |
| 0x1E4 | 2 (espaço entre linhas) | | 0x230/0x234 | posição para o IME (NIM_CARET) |
| 0x1E8 | timer do cursor (0xFA0D05, 500 ms) | | 0x238 | bReading |
| 0x1EC | CaretLine (linha lógica) | | 0x23C | FString Reading |
| 0x1F0 | CaretCol | | 0x248 | lParam de NIM_READING |
| 0x1F4 | bCaretOn (OnTimer inverte, sem testar o id) | | 0x24C/0x250 | 1 |
| 0x1F8 | linhas visíveis = (Height-2)/(altura+2) | | 0x25C | 16 |
| 0x1FC | TopLine (rolagem) | | 0x260 | total de linhas de tela |
| 0x200 | total de linhas lógicas | | | |

Quebra de linha: largura útil `Width - 36`. Controles de rolagem **fora** da caixa, em
`x = LocalW + 2`. IDs: cima 0xF0F01409, baixo 0xF0F01410, barra 0xF0F01411.

## Virtuais

WindowProc `0x1001a440`: `NWM_COMMAND` → 64 (cima), 65 (baixo), 66 (barra: TopLine = pos);
`NIM_COMPOSITION` → 67, `NIM_RESULT` → 68, `NIM_GETCARET` → 69, `NIM_READING` → 70,
`NIM_ATTRIBUTE` → 71 (limpa a leitura). 28 = `NCEditBox::SetFocus` (corpo idêntico), 29 KillFocus
(`IMEManager->Deactivate()` + `NCWnd::KillFocus`), 34 OnTimer, 35 OnCreate `0x1001cd00`,
36 OnDestroy `0x1001e550`, 50 OnPaint `0x1001e700` (5,2 KB), 52 OnChar `0x1001fb60`
(0x20..0x7E; `appSprintf` sem uso; `InsertChar`), 53 OnKeyDown `0x1001fbc0` (3,4 KB), 61 roda,
63 atualiza a barra, 64/65 rolam uma linha.

## Outras funções mapeadas

`0x1001a5a0` rola até o cursor; `0x1001bb70` SetRepeatDelay; `0x1001bba0` init da barra (chama
SetRepeatDelay(200)); `0x1001be80` CountRows(linha) (inclui a composição no cursor);
`0x1001c120` UpdateRowCount(linha, antigas, bComp); `0x1001c9e0` InsertString(Str, bResult);
`0x1001d040` SetCaretX(linha, coluna); `0x1001d150` GetRowStart(row) (depende de edx preservado
por `0x10033ac0`); `0x1001dd60` GetText; `0x1001e0f0` InsertChar (Enter divide a linha);
`0x1001e500` SetText (InsertChar em cada caractere + slot 63); `0x1001e670` Clear.
Pintores (só da pintura, por língua como no NCEditBox): `0x1001a620`, `0x1001aad0`,
`0x1001b1a0`, `0x1001b430`, `0x1001d4c0`, `0x1001d8e0`. Não analisados: `0x1001c3d0` (1,5 KB,
do OnKeyDown). `0x1001a010`, `0x1001a220`, `0x1001a310` são do `NCListCtrl`, não daqui.

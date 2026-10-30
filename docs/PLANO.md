# Plano e método

## O binário

| Fato | Valor |
|---|---|
| Arquivo | `NWindow.dll` stock C1, 1.277.972 bytes, md5 `2dacb6eb…`, TimeDateStamp `0x40DBD452` (25/06/2004) |
| Compilador | **Visual C++ .NET 2002 (7.0, cl 13.00.9466)**, CRT estático (libcmt 7.0), linker 7.0 (Rich header) |
| Código | `.text` 0xD59D0 bytes; ImageBase `0x10000000`, sem ASLR |
| Exports | 180: `GPackage`, `_DllMain@12`, 178 `autoclass*` |
| Imports | 335: Core.dll 151, Engine.dll 82, IMM32, VERSION, KERNEL32, USER32 |
| RTTI | não (compilado com /GR-) |
| Funções | 5.357 no Ghidra: 3.019 do jogo, 542 inicializadores estáticos, 1.262 funclets de EH, 472 do CRT, 62 thunks |
| A decompilar | **3.561 funções, 755.782 bytes** (jogo + inicializadores) |

De onde vêm os nomes (nenhum PDB existe):
- **178 classes** com tamanho exato, classe-pai, construtor e vtable: dos inicializadores do
  `IMPLEMENT_CLASS` (`tools/extract_classes.py` → `analysis/classes.md`);
- **216 funções** com o nome original: as strings dos macros `guard()` do UE2, achadas pelas
  tabelas de exceção (`tools/eh_scan.py`);
- slots de vtable: o nome dado a um slot (ex.: 50 = `OnPaint`) vale para todas as classes filhas;
- as vtables da engine (`UCanvas`, `UObject`…) com assinatura completa, lidas do próprio
  `Engine.dll`/`Core.dll` (`tools/engine_vtables.py`).

## Método: modo hook, função por função

Mesmo caminho de projetos como re3 e OpenRCT2: o executável original continua rodando e cada função
reimplementada o substitui no lugar, até não sobrar nada do original.

1. **Escolher** funções `todo` em `db/functions.csv` (de preferência uma classe inteira, das
   folhas para a base ou das mais usadas).
2. **Ler** `analysis/ghidra/decomp/<va>.c` e **conferir no assembly** `analysis/ghidra/asm/<va>.asm`:
   o decompilador do Ghidra erra número de parâmetros (conferir o `ret N`), confunde
   parâmetros com endereço de retorno e não sabe os slots de vtable.
3. **Escrever** em C++ no estilo UE2 em `src/nwindow/<Classe>.cpp`, com o endereço em comentário e o
   `NW_HOOK_*` logo abaixo de cada função. Layout de classe com `NW_ASSERT_OFFSET/SIZE`.
4. **`python tools/cycle.py`**: compila, instala, carrega fora do jogo (todo hook tem de dar `ok`),
   e o `verify_hooks.py` compara com o original: bytes desempilhados no `ret`, slots de vtable
   chamados e imports chamados. Toda diferença tem de ser entendida; as que não são bug vão
   para `db/verify_notes.csv` com a explicação.
5. **Testar no jogo** com o System de teste; passando, registrar em `db/verified.csv`.
6. Um bug no jogo? Desligar hooks no `NWindowDecomp.ini` (`Disable=NCGDevice::*`, um endereço,
   um método) até isolar a função.

O que o verificador já pegou na primeira classe: o MSVC põe **sobrecargas virtuais em ordem
inversa** na vtable. `UCanvas::DrawStretchedTex(5 args)` está no slot 35 e a de 9 args no 34;
declaradas na ordem natural, o nosso código chamava o slot errado.

## Fases

| Fase | Conteúdo | Estado |
|---|---|---|
| 0 Fundação | pasta, análise, banco, build, hooks, instalador, smoke test, verificador | **feito (25/09/2026)** |
| 1 Base da UI | `NCObject`, `NCGDevice` (feitos); `NCCmdTarget`, `NCWnd` (0x94, 38 virtuais novas), `NControl` (0x1A0), `NCVirtualWndMain` (despacho de mensagens), `NL2Font`, `NHtmlAnalyzer` | em andamento |
| 2 Controles | `NCButton`, `NCEditBox`, `NCListBox`, `NCScrollWnd`, `NCComboBox`, `NCTabControl`, `NCSliderBar`, `NCProgressCtrl`, `NCListCtrl`, `NCMultiEdit`, `NCHtmlViewer`… | |
| 3 Janelas | ~110 classes `NC*Wnd` (inventário, skills, chat, party, loja, clã…) | |
| 4 Console e sistemas | `NConsoleWnd` (0x3740 bytes, 285 slots: os callbacks de rede que o Engine chama), IME (4 variantes), HTML | |
| 5 Standalone | `IMPLEMENT_CLASS` das 178 classes, globais e inicializadores próprios, CRT moderno; a source gera `NWindow.dll` sozinha e substitui o original | |
| 6 Evolução | portar as funções do `NWindow_C1` como mudanças de source; features novas | |

Na fase 5 não pode sobrar nenhum `NW_STOCK`, `nw::StockCall` ou `nw::Field` — são andaimes do
modo hook para falar com código ainda não decompilado.

## Pendências de ferramenta

- Conferir automaticamente as vtables que declaramos contra as do original (slot a slot, seguindo
  os thunks de import) — pegaria erros de ordem de virtual antes do jogo.
- Renomear no projeto Ghidra (`ghidra/`) a partir do `db/functions.csv` e reexportar: o
  decompilador melhora muito com nomes e tipos.
- Compilação *matching* (byte a byte) exige o VC 7.0 (13.00.9466), que não está instalado. Por isso
  o código decompilado evita C++ moderno: dá para tentar mais tarde.

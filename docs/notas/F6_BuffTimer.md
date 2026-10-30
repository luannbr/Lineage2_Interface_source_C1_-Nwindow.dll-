# Fase 6 — BuffTimer

A contagem do tempo restante sobre cada ícone de buff/debuff, do `NWindow_C1` (HEAD `4e56326`,
`src/inject/BuffTimer.cpp`; docs `SESSION_2026-07-19_BUFFTIMER.md` e `NCAbnormalStatusWnd_PORT.md`),
agora na nossa source. Porte do trabalhador status, 27/09/2026.

O que o `NWindow_C1` fazia (e o porte fazia igual até o pedido de 27/09/2026, seção "Além do
NWindow_C1"; hoje é o `[BuffTimer] IconMinutes=1`):
- Com 60 s ou mais, mostra os minutos e um M: "14M". Abaixo disso, só os segundos: "59". Em 0,
  nada.
- A textura de números (`L2UI.NWindow.Number`, o `DrawSpecialDigit` grande) só tem 0-9, H, M, P,
  /, % e `.`. Por isso não há S nem letra minúscula.
- Os números são brancos, com contorno preto de 1 pixel e sombra. Ficam no canto de baixo à
  esquerda do ícone: (x+1, y+15) do ícone.
- Três linhas de dez ícones. Um ícone que pisca nos últimos 10 s leva a contagem junto.
- Com mais de 30 ícones, nenhuma contagem aparece (o `NWindow_C1` desistia inteiro).

O que faz agora (padrão):
- O ícone mostra só o último minuto: "60", "59"... "1", com o mesmo contorno e a mesma piscada.
  Com mais de 60 s, nada no ícone.
- A tooltip do ícone ganha a linha "Remaining Time : N Minute(s)" ("N Second(s)" abaixo de um
  minuto), logo abaixo do título.

## Onde está

- `src/features/BuffTimer.cpp` e `include/features/BuffTimer.h`.
- Ponto de chamada do ícone: o fim do `NCAbnormalStatusWnd::OnPaint`
  (`src/nwindow/NCAbnormalStatusWnd.cpp`), depois do `EndPaint`/`ReleaseClip` do original.
  - É `nwf::BuffTimer_DrawCountdowns(this, Device)`, num bloco `#if NW_FEATURES`.
  - O `NWindow_C1` fazia um detour do `OnPaint` (`jmp` em `0x100aed40`) e desenhava depois que o
    original voltava, no mesmo ponto.
- Pontos de chamada da tooltip (27/09/2026), no mesmo arquivo:
  - `NCAbnormalStatusWnd::OnDrawToolTip`: depois do `GetTextExtent` do texto da tooltip,
    `Size.cy += nwf::BuffTimer_ToolTipExtraHeight(Status.RemainTime)`. Com tempo restante, soma 12:
    uma linha a mais na medida, logo uma faixa a mais de 13 pixels na caixa.
  - `NCAbnormalStatusWnd::DrawStatusToolTip`: entre o título (`Y += 13`) e a descrição,
    `Y += nwf::BuffTimer_DrawToolTipTime(Device, Status.RemainTime, X, Y)`. Ela desenha a linha e
    devolve 13, ou 0 sem tempo.
- `LineageWindow.ini` `[BuffTimer] IconMinutes`: padrão 0; 1 traz de volta o ícone do
  `NWindow_C1`. Lido uma vez. A linha da tooltip fica nos dois modos.
- Sem linha de log: a única do `NWindow_C1` era a da instalação do detour.

## Do endereço ao nome

| `NWindow_C1` | aqui |
|---|---|
| `0xd120` "Paint_DrawText", tipo 1 | `NCGDevice::DrawSpecialDigit(1, ...)` (os números grandes) |
| self `+0x44`/`+0x48` | `ScreenX`/`ScreenY` |
| self `+0x94`/`+0x98`, entradas de `0x1214` | `Statuses` (`FAbnormalStatus`) |
| entrada `+0x1200`/`+0x1204`/`+0x1210` | `Icon`/`RemainTime`/`bVisible` |
| paint `+0x2c`/`+0x30`/`+0x5c`/`+0x48`/`+0x4c` | `OriginX`/`OriginY`/`Unknown5C`/`DirtyTop`/`DirtyBottom` (postos como o `NCGDevice::BeginPaint` faz e devolvidos depois, igual ao `NWindow_C1`) |
| paint `+0x54`/`+0x58` | `SmallDigits`/`LargeDigits` (só salvos e devolvidos) |

## O que difere do `NWindow_C1`, e por quê

1. **O "BeginClip"/"EndClip" sem argumento saiu.**
   - O `NWindow_C1` chamava `0x1000d100` e `0x1000d110` como um par sem argumentos. Só que
     `0x1000d100` é o `NCGDevice::SetClip(X, Y, XL, YL)`, um `jmp` para o `UCanvas::SetClip`.
   - O `UCanvas::SetClip` lia como retângulo quatro posições da pilha de quem chamou: o cookie do
     frame e o `edi`/`esi`/`ebx` salvos (conferido no `LineageWindow.dll` do `System - Copia`,
     `0x10015d43`).
   - Depois voltava com `ret 0x10`: a pilha ficava 16 bytes fora. O frame SEH devolvia o `esp`, e o
     estrago ficava no valor de retorno do `OnPaint` (que ninguém lê) e num recorte do canvas com
     lixo (conferente, rodada 12; a primeira versão desta nota dizia que `ebx`/`esi`/`edi` voltavam
     errados — não voltam).
   - Os números não dependiam daquele retângulo. O `UCanvas::DrawSpecialDigit` do `Engine.dll`
     desenha pelo `FCanvasUtil::DrawTile` com coordenadas absolutas e não lê o recorte do canvas
     (`0x104a2da0`).
   - Então o porte deixa o canvas como o `OnPaint` original deixa: sem `SetClip` e sem
     `ReleaseClip` em volta dos números. Na tela, o mesmo resultado.
2. **Exceções.**
   - O `NWindow_C1` engolia também uma exceção do próprio `OnPaint` original, porque o trampolim
     estava dentro de um `__try`.
   - Aqui o `OnPaint` fiel mantém o `guard/unguard` do original. Só o desenho dos números tem o seu
     `__try/__except`.
3. **`IsBadReadPtr` saiu.** Os dados são tipados: `Statuses` e o device já são lidos pelo próprio
   `OnPaint` logo antes.

## Além do `NWindow_C1`: o tempo na tooltip, o ícone só no último minuto

Pedido do usuário depois de testar o build L2 no jogo (27/09/2026), como as crônicas mais novas
mostram: título "Apprentice Song of Meditation Lv. 1", depois "Remaining Time : 1 Minute(s)",
depois a descrição. É uma mudança de propósito, que o `NWindow_C1` não tinha.

- **A linha da tooltip**, no desenho da tooltip original (`DrawStatusToolTip`):
  - Mesma fonte (`L2FONT_Normal`, `DrawText` sem quebra).
  - Cores da linha do título: "Remaining Time :" no cinza do "Lv" (`0xFFA3A3A3`); o tempo no
    dourado do nível (`0xFFB09B79`), 4 pixels depois do rótulo (o `GetTextExtent` dele).
  - Minutos inteiros a partir de 60 s: 60–119 s dão "1 Minute(s)", como o "1M" do ícone antigo.
  - Abaixo de 60 s, os segundos: "59 Second(s)".
  - O cliente C1 é em inglês, então o texto é inglês fixo.
  - Sem tempo (0, ou −1 dos toggles), não há linha nem espaço a mais.
- **A caixa da tooltip:**
  - O original mede o texto guardado ("nome\ndescrição") com `GetTextExtent` na largura 168.
  - Desenha uma faixa de 13 pixels para cada 12 de altura, e a descrição 13 pixels abaixo do
    título.
  - Com a linha nova, a medida ganha 12: mais uma faixa, e a descrição desce 13.
  - A linha é desenhada a cada quadro com o `RemainTime` do momento: o texto guardado da tooltip
    não muda.
- **O ícone:** nada acima de 60 s. De "60" a "1" nos últimos 60 s, com o mesmo contorno, a mesma
  piscada dos últimos 10 s e a mesma posição.
- **`[BuffTimer] IconMinutes=1`** (em `LineageWindow.ini`): o ícone do `NWindow_C1` de volta,
  "NM" a partir de um minuto e os segundos abaixo. Padrão 0. O WindowFix escreve só o modelo de
  `[Window]`, então a chave `[BuffTimer]` é escrita à mão.

## Conferido

- **Teste diferencial** (fora do repositório, no scratchpad): `format_countdown` e `icon_xy` do
  `NWindow_C1` contra os nossos.
  - Todos os valores de −5 a 1.000.500 s, com 4 tamanhos de buffer.
  - Posições dos ícones de −3 a 39.
  - Resultado: 0 diferenças (59 → "59", 60 → "1M", 1199 → "19M").
  - Esse teste é do ícone do `NWindow_C1`, que hoje é o modo `IconMinutes=1`.
- **`wndtest`** (27/09/2026, scratchpad): o `build/l2/Release/NWindow.dll` carregado fora do jogo,
  com as funções chamadas pelos endereços do `NWindow.map`. 26 verificações, todas ok:
  - `FormatCountdown` nos dois modos: padrão 0/−1/1/10/59/60 dão "", "", "1", "10", "59", "60";
    61 e 1200 dão ""; com `IconMinutes`, 60 dá "1M", 119 "1M", 1199 "19M";
  - `FormatRemaining`: 1 e 59 dão "N Second(s)", 60/119/120/1200/3600 dão "N Minute(s)" com os
    minutos inteiros;
  - `BuffTimer_ToolTipExtraHeight`: 12 com tempo, 0 com 0 e −1;
  - `IconMinutes` lido do `LineageWindow.ini`: 0 sem a chave, 1 com `[BuffTimer] IconMinutes=1`
    (duas execuções).
  - O `BuffTimer_DrawToolTipTime` precisa de um canvas: fica para o teste no jogo.
- **Ciclo:** o mesmo resultado do `F6_CombatLog.md` (as mudanças estão no mesmo ramo).

## Para testar no jogo

1. Pegar buffs de durações diferentes:
   - acima de um minuto, o ícone não mostra número;
   - no último minuto aparecem os segundos, de 60 a 1;
   - nos últimos 10 s o ícone pisca e a contagem pisca junto.
2. Passar o mouse sobre um buff. A tooltip mostra:
   - o título ("Nome  Lv N");
   - "Remaining Time : N Minute(s)" (ou "N Second(s)" no último minuto), mudando com o tempo;
   - a descrição inteira, dentro da caixa, sem cortar a última linha.
3. Um toggle ou efeito sem tempo: a tooltip fica como a original, sem a linha.
4. Com mais de dez buffs, a segunda e a terceira linha também têm contagem e tooltip no lugar
   certo.
5. Com `[BuffTimer] IconMinutes=1` no `LineageWindow.ini` (e o jogo reaberto): o ícone volta a
   mostrar "NM" acima de um minuto; a tooltip continua com a linha.
6. As outras janelas continuam desenhando normalmente depois da barra de buffs (tooltips, janela
   de alvo, chat).

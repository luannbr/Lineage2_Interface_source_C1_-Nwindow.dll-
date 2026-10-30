# Fase 6 — ItemHoverTip (a dica do item sob o mouse, no estilo das crônicas recentes)

Origem: pedido do dono em 29/09/2026 (capturas do `Interface_542`: crônicas recentes mostram o nome do
item numa caixa preta com o símbolo do grade; o C1 mostra o nome numa faixa translúcida, sem grade, e o
grade só aparece no tooltip completo, ao clicar). Não existe no NWindow_C1. Aqui:
`include/features/ItemHoverTip.h`, `src/features/ItemHoverTip.cpp`.

## O que faz

Nas grades de itens (as derivadas de `NCItemWnd`: as páginas do inventário, o inventário do pet, a
troca, o armazém, as lojas privadas, a loja, a entrega; e o paperdoll, `NCEquipItemWnd`, que não
deriva de `NCItemWnd` mas tem o mesmo ramo), o item **sob o mouse que não é o selecionado** (o
selecionado continua com a caixa completa) passa a mostrar o nome numa caixa escura com borda fina de 1 px e, depois do nome, o
símbolo do grade do item (D, C, B, A, S); item sem grade não leva símbolo. A caixa fica em cima da
célula (embaixo se a janela está no topo da tela) e é empurrada para dentro da tela, como a faixa
de fábrica. Loja privada: a dica é "nome\npreço"; o símbolo vai depois da primeira linha.

**Atalhos** (`NCShortCutWnd`, barra vertical e horizontal): o mesmo tip, em cima do slot (embaixo se não
há espaço). O nome é o `DisplayName` do slot (item com o "+enchant" ou a contagem, skill com o MP, ação);
o slot não guarda o grade, então o de um atalho de item sai da tabela de itens pelo ID da classe
(`NItemTable::Find`: `CrystalGrade` de arma, `ArmorCrystalGrade` de armadura; item comum não tem), como o
inventário o obtém. Skills e ações não levam símbolo. De fábrica o rótulo do atalho era a faixa
translúcida desenhada no canto de cima-esquerda do próprio slot, por cima do ícone; agora a caixa fica
fora do ícone.

`[ItemTip] HoverStyle=0` (registro, `HKCU\Software\Lineage2C1\NWindow\ItemTip`) volta à faixa de fábrica.
O padrão é 1. Só no build do L2 (`NW_FEATURES`); o gancho e o standalone fiel não veem o código.

## Onde entra

`NCItemWnd::OnDrawToolTip` (`src/nwindow/NCItemWnd.cpp`, vslot 51, `0x10016ba0`) e
`NCEquipItemWnd::OnDrawToolTip` (`0x100646e0`) e `NCShortCutWnd::OnDrawToolTip` (`0x100a9660`): no ramo do item não selecionado,
`nwf::ItemHoverTip_Draw` vem antes do desenho de fábrica, que fica como estava e só roda com a
opção desligada. Nenhuma derivada de `NCItemWnd` sobrescreve o método, exceto a `NCSkillItemWnd`
(skills, não itens: fica de fábrica).

## O símbolo do grade

Não é textura: nenhum pacote do cliente C1 (`SysTextures/*.utx`) tem textura `grades`/`gradea`/...
(procurado em todos). É um glifo inline do motor: o `DrawText` recebe o texto `` `graded` `` ..
`` `grades` `` (`NConsoleWnd::GetGradeText`) com o 9º argumento 1, exatamente como o tooltip
completo já desenha (`ItemTip::DrawLines`). Nas capturas ele mede 11 × 11 px, com 1 px de sombra, e
traz a própria moldura (quadradinho preto de borda clara). Como o nome e o glifo saem pela mesma
chamada do tooltip completo, a aparência é a mesma da captura 221.

## Medidas (das capturas de referência)

Interior da caixa ≈ `(20,20,21)` (quase opaco, quase preto: `0xFF141414`), borda 1 px ≈
`(55,55,53)` (`0xFF3A3A38`), texto branco. Ar: 4 px dos lados, 3 em cima e embaixo, 3 px entre o
nome e o glifo. **Estimadas de uma captura de 104 × 76 px: conferir no jogo** e ajustar as
constantes de `ItemHoverTip.cpp` (`PadX`, `PadY`, `GradeGap`, `GradeW`, `ColorFill`, `ColorBorder`).

## Em que crônica isso mudou (o que os `NWindow.dll` dos clientes do disco mostram)

| Cliente | Build do `NWindow.dll` | referencia `L2UI_ch3.Tooltip.Tooltip%d` |
|---|---|---|
| C0 (Prelude) | 2004-03-08 | não |
| C1 | 2004-06-25 | não |
| C2 | 2004-10-07 | não |
| C3 (`1cc_3rise_of_darkness`) | 2005-04-29 | **sim** |
| C3 / C4 (pastas `C3_..._system`, `C4_Client`, BlackBird) | 2006-01-13 (o mesmo build) | **sim** |

`L2UI_CH3` é o pacote de UI da Chronicle 3: as nove texturas `Tooltip1..9` (moldura 3×3) chegam no C3.
Ou seja: C0, C1 e C2 têm o tooltip antigo (`mini-tool01` + `Tool01/02/03`); o novo começa no C3, e o C4
já o tem. Isto é evidência de **recursos e código presentes**; o desenho exato do hover do C3/C4 não foi
conferido no código (o `NWindow.dll` deles é de outra geração: coordenadas em ponto flutuante,
classes com outro layout), e este recurso não copia nada de lá: reproduz o visual das capturas com
`DrawRect` (que o C1 já tem) e o glifo do grade que o C1 já desenha. As texturas `L2UI_CH3.Tooltip*`
estão em `C4_Client/systextures/L2UI_CH3.utx`, criptografado (`Lineage2Ver121`); o C1 não as tem.

## Fora do escopo (ainda)

A dica das janelas de ações (`NCActionItemWnd`), das skills (`NCSkillItemWnd`), dos botões e dos
ícones continua a de fábrica (translúcida); nas crônicas recentes esses também mudam.

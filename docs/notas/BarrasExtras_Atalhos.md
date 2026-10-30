# Barras de atalhos extras (estilo C5 / Interlude) — estudo de viabilidade

Pedido do dono em 30/09/2026: "se é possível adicionar nova barra adicional de skills em cima da barra
atual, como é no C5/Interlude; só quero que analise a viabilidade". Depois: implementar com 2 barras
extras **como um projeto separado**, só para ver como ficaria; se os usuários quiserem, entra no principal.

**Conclusão: viável, esforço médio, sem mudança no servidor, no protocolo nem nas texturas.** O
experimento está na branch `experiment/extra-shortcut-bars`, no worktree
`../nwd_wt_shortcuts` (nada disso entrou no `master`); as notas da
implementação estão lá, em `docs/notas/F6_ExtraShortcutBars.md`.

## Como o C5 faz (verificado no cliente C5 do disco)

Fonte: `C5_Interlude_to_C5/C5_Client/system`. A interface do C5 é em script (UnrealScript) e XML; o
`Interface.u` e o `NWindow.u` "criptografados" (`Lineage2Ver111`) são só um XOR `0xAC` em tudo depois
dos 28 bytes do cabeçalho UTF-16, o que dá um UPackage comum (v123) com o texto de cada classe (o
`ShortcutWnd.uc` do C5 tem 21 KB). O `l2ui_ch3.utx` (`Lineage2Ver121`) é XOR `0x68`. O `interface.xdat`
tem as janelas.

- **Janelas** (`interface.xdat`): raiz `ShortcutWnd`; três barras por orientação
  (`ShortcutWndHorizontal`, `_1`, `_2` e `ShortcutWndVertical`, `_1`, `_2`), mais as do joypad. Uma
  orientação aparece por vez; o botão de girar alterna.
- **Cada barra**: 12 slots de 36 px (passo de 37 px e 5 px extras a cada 4 slots), fundo `shortcut_back`
  (504 × 46 na horizontal). As extras são "gavetas" (`Drawer`): a 2ª sai para **cima** da principal (na
  vertical, para a **esquerda**) e a 3ª para cima da 2ª, com animação de deslizar.
- **Botões**: a principal tem Expand/Reduce, anterior/próxima, joypad, girar e travar; as extras só
  anterior/próxima e o número da página.
- **Não têm slots próprios.** O ID de um atalho é `página × 12 + slot`, 0 a 119; a tabela é uma só (120
  entradas). Cada barra é **uma vista de uma das 10 páginas**: a barra 1 na página escolhida, a 2 começa na
  página 2 e a 3 na página 3, e cada uma pagina sozinha (`CurrentShortcutPage2/3`, `SetCurPage2`). Soltar
  um item numa barra extra registra pelo ID daquele slot.
- **Teclas**: F1..F12 (e Ctrl/Shift+F, e os dígitos) usam **só a barra principal**; Alt+1..0 e Alt+F1..F10
  trocam a página só dela. Nada endereça as extras (os rótulos F1..F12 delas são decorativos).
- **Expand** (botão da principal): alterna nenhuma → 1 extra → 2 extras → nenhuma. O estado vai para o
  `Option.ini` (`Is1ExpandShortcutWnd`, `Is2ExpandShortcutWnd`); as páginas voltam a 2 e 3 a cada entrada.
- **Texturas** (`L2UI_CH3.ShortcutWnd`, 54): as extras reusam as da principal. O C1 (`L2UI.utx`) tem só 17:
  fundos vertical e horizontal (`Back`, `Back1`, 64 × 512), setas 16 × 16 (esquerda, direita, cima, baixo,
  girar), a máscara e os botões do joypad. **Faltam no C1** os rótulos F1..F12, expandir/minimizar,
  travar/destravar e as variantes `_over`.

## O C1 hoje

- `NCShortCutWnd(10, 10, 32)`: 10 slots por página, 10 páginas, 32 px, com as próprias tabelas
  (`Items[100]`, `Keys[100]`) e a página atual (`Page`). O console só conhece uma (`Desktop->ShortCutWnd`).
- **O protocolo é de slot plano**: um inteiro, `slot = página × 10 + posição` (`SetShortCut` descarta
  slot ≥ `Pages × Cols`, 100). Uma barra que mostra outra página usa os mesmos slots.
- A classe já é uma "vista de página": desenha, acha o slot sob o mouse (`SlotAt`, base de arrastar e
  soltar do inventário, das ações e do paperdoll) e mostra o tooltip pela `Page` e pelas tabelas.
- O C1 desenha o número do slot em código: não precisa dos rótulos F1..F12 do C5. Nem existe o mecanismo
  de "gaveta" do C5 (nada de `Drawer` no `NWindow.dll` do C1): a barra extra é uma janela comum.

## O servidor (verificado, lido por um agente; nada foi alterado)

- O slot é um número opaco: guardado e devolvido de 0 a 143 (o `RequestShortCutReg` aceita página 0..11
  com 12 slots por página). Uma barra que só muda de página fica dentro disso. Passar de 143 pede uma linha
  (`RequestShortCutReg.java:29`); o banco (`character_shortcuts`, `TINYINT`) não muda.
- O servidor conta **12** por página e o C1 conta **10**: o número que o cliente manda volta idêntico, então
  não faz mal ao guardar; mas os atalhos padrão do personagem novo (`character_shortcuts.xml`: Sit/Stand
  no slot 10 e o livro do tutorial no 11, página 0) aparecem no C1 na **página 2**, F1 e F2.
- O filtro de pacotes (`packetfilter.xml`) deixa passar 2 registros de atalho por 300 ms (o excesso é
  descartado) e 1 remoção por 300 ms: trocar muitos atalhos de uma vez pode perder pacotes.
- AutoShot e Manufacture não dependem do layout (o estado é por item; as ações são atalhos comuns).
- Só o AutoFarm (`AllowAutoFarm = False`) assume 12 por página e a página 10; dormente.

## Opções

- **A. Barras extras como vistas de páginas (o desenho do C5) — escolhida.** Nenhuma mudança de servidor,
  de protocolo ou de textura. A capacidade continua 100 atalhos; passam a ficar visíveis 2 ou 3 páginas ao
  mesmo tempo. A barra extra é **outra instância** de `NCShortCutWnd` que usa as tabelas da principal
  emprestadas (sem mudar o layout da classe nem criar `UClass` nova).
- **B. Barras com slots próprios além de 100.** Mais capacidade que o C5, mas pede subir o limite do
  cliente (tabelas e teclas), talvez uma linha no servidor, e foge do comportamento que se quer copiar.
  Não feita.

## O que a opção A pede no cliente

Criar as duas barras; posicioná-las (em cima da principal, à esquerda se for vertical, do outro lado se
faltar tela) e fazê-las seguir a principal ao arrastar e ao girar; um botão de expandir; o estado no
registro; e o arrastar entre barras (soltar um atalho de uma barra em outra barra o move, em vez de
apagá-lo como o código de fábrica faz ao soltar fora da barra). O tooltip, o AutoShot e o arrastar do
inventário funcionam sem mudança, por serem a mesma classe.

## Resultado do experimento (30/09/2026)

Implementado a opção A com 2 barras extras na branch `experiment/extra-shortcut-bars` (worktree
`nwd_wt_shortcuts`) e **testado no jogo pelo dono: funciona**. Três ajustes vieram do teste: o ícone de girar
que a arte do fundo desenha (coberto nas extras), a alça de arrastar das extras (agora move o grupo) e o ponto
de pega que a barra principal precisa receber quando a extra arrasta (senão o `NCWnd::Snap` puxa o grupo e o
mouse "pula"). Fica como projeto separado; entra no principal se os usuários quiserem. Detalhes em
`docs/notas/F6_ExtraShortcutBars.md` da branch.

## Riscos (como eram antes do teste)

Só se testa no jogo (o ciclo não desenha a interface); posições e tamanhos são deduzidos; sem a animação
de gaveta; o botão de expandir reaproveita as setas de página (não há arte própria no C1). Detalhes e
limites da implementação: `F6_ExtraShortcutBars.md` na branch do experimento.

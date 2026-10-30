# Fase 6 — AutoShot (soulshot / spiritshot automático, estilo C3)

Origem: NWindow_C1 `src/inject/AutoShot.cpp` e, de `src/inject/craft/CraftUi.cpp`, o anúncio
`ss_ui 1` e o canal de dados (HEAD `4e56326`); doc `AUTO_SHOT.md`. Aqui:
`include/features/AutoShot.h`, `src/features/AutoShot.cpp`.

## O que faz

- **Clique direito num shot** da barra de atalhos (item que o `NConsoleWnd::IsShotItem` reconhece):
  manda `ss_set <item> <0|1>` pedindo o estado contrário, toca o som de clique e engole o clique
  (sem girar a câmera). Em qualquer outra coisa o clique segue como no original.
- **O estado é do servidor**: chega como mensagem de dados no `NpcHtmlMessage`
  (`L2W shot 1 state` / `on=<ids>` / `msg=<texto>`, com ou sem `<html><body>` na frente), nunca
  vira página. `on=` troca a lista inteira; `msg=` vai para o chat (`NCConsole::AddSystemText`,
  0xFFB09B79) no tick seguinte. Nada muda aqui antes de o servidor dizer.
- **Marca**: depois da pintura original da barra, o `ToggleTex` (a marca animada das skills de
  alternar) sobre cada shot ligado da página, como o original faz com as skills.
- **Mover atalho**: soltar um atalho arrastado em outro slot o move para lá (troca se o destino
  está ocupado): `RequestShortCutReg`/`RequestShortCutDel` como a barra original manda (o 4º
  parâmetro vai 1, como no NWindow_C1).
- **Anúncio**: `ss_ui 1` na entrada no mundo (a lista local zera antes: o servidor esquece os
  shots automáticos a cada relog).

`LineageWindow.ini` `[Shots] AutoShot` (1). Com 0 fica tudo como o original.

## Onde entra no código

| Ponto | Arquivo | Antes (NWindow_C1) |
|---|---|---|
| fim do `NCShortCutWnd::OnPaint` → `AutoShot_OnPaint` | `NCShortCutWnd.cpp` | vtable clonada, slot +0xC8 |
| `NCShortCutWnd::OnRButtonDown` (novo, só no L2) → `AutoShot_OnRButtonDown` | `NCShortCutWnd.h/.cpp` | slot +0xE8 (o no-op do `NCWnd`) |
| fim do `NCShortCutWnd::OnLButtonUp` → `AutoShot_OnLButtonUp` | `NCShortCutWnd.cpp` | slot +0xE4, invólucro do original |
| `NConsoleWnd::NpcHtmlMessage` → `AutoShot_OnNpcHtml` | `NConsoleWndEvents.cpp` | thunk ILT do `OnNpcHtmlMessage` no Engine.dll (CraftUi) |
| `NConsoleWnd::Tick` → `AutoShot_OnTick` | `NConsoleWndInput.cpp` | port do Tick + thunk ILT do `OnUserInfo` (CraftUi) |

O `OnLButtonUp` original não mexe no `PressedSlot` nem no slot de origem quando o ícone cai em
outro slot, então conferir origem e destino depois dele dá o mesmo que o NWindow_C1 (que lia
antes).

## Diferenças (e por quê)

- **Momento do `ss_ui 1`**: o NWindow_C1 anunciava no primeiro `OnUserInfo` de um `User*` novo
  (desvio no Engine.dll; o `UserInfo` não passa pelo NWindow) e esquecia o anúncio nas respostas de
  restart/logout. Aqui: no primeiro tick com `State == L2CS_InGame` e um `GetMyUser()` novo, e
  sair do mundo esquece. Um quadro depois, no máximo; a fila de bypass já separava os dois anúncios
  em 120 ms. Um restart **recusado** não reanuncia mais (o NWindow_C1 reanunciava no `UserInfo`
  seguinte).
- **Ordem com o `mfg_ui 1`** (clan): o NWindow_C1 mandava `mfg_ui 1` e depois `ss_ui 1` no mesmo
  `OnUserInfo`. Se o craft anunciar no tick, a chamada dele tem de vir antes do
  `AutoShot_OnTick` (como no Tick do NWindow_C1: CraftUi, AutoShot, ReturnToLogin, BypassFlush,
  CacheCleaner); se anunciar num desvio do `OnUserInfo`, já vem antes.
- **Som do clique e `msg=` no chat**: no NWindow_C1 os dois passavam pelo `nwui`, cujo console só
  existe com `[Craft] NativeUI=1` — com `NativeUI=0` não havia som nem mensagem. Aqui sempre (o
  usuário joga com `NativeUI=1`, então para ele nada muda).
- **Mensagens de dados com o AutoShot desligado**: o NWindow_C1 engolia as `L2W ` (menos
  `L2W mfg`) sempre que o gancho existia (`NativeUI=1` ou `AutoShot=1`); aqui só com
  `AutoShot=1`. O servidor só manda depois do `ss_ui 1`, então não aparece.
- **Log**: as linhas `autoshot: ...` saíam sempre no NWindow_C1; aqui só com `[Window] Log=1`
  (como todo `nwf::Log`). O `announced auto shots` era `craftui:` (e compilado fora); agora é
  `autoshot:`.

## O soulshot usado no meio do ataque cancela o ataque (04/10/2026; achado no `Engine.dll`, não aqui)

Relato do dono: o auto-shot "ativa cedo e rápido demais", o efeito do shot no acerto não aparece
sempre e a animação de ataque recomeça (pior com velocidade de ataque alta); usado à mão, no momento
certo, o efeito aparece certo. A parte do cliente (esta feature) só manda `ss_ui`/`ss_set` e mostra
o estado: **quem usa o shot e escolhe o momento é o servidor**, o cliente nunca usa o item.

- **Servidor (Lucera)**: no instante do acerto, `Creature` chama `Player.unChargeShots` →
  `autoShot` → `useItem`, e o handler (`SoulShots`/`SpiritShot`/`BlessedSpiritShot`) transmite para o
  jogador e para quem está em volta um `MagicSkillUse(jogador, jogador, <skill anexada ao item>, nível, 0)`
  (`ShotsItemHandler.broadcastShotSkill`). A força do shot não está nisso: é a carga da arma
  (`setChargedSoulshot(1)`, `setChargedSpiritshot(1)`, blessed `2`), o consumo (`getSoulShotCount`)
  e a checagem de grau do item contra a arma.
- **Cliente**: `UGameEngine::OnReceiveMagicSkillUse` (jogador `0x10423630`, NPC `0x104239d0`) chama
  primeiro `APawn::SpawnNTransientEffect(atacante, alvo, id)` (`0x1045a500`); se devolve `1` o caminho
  acaba (só efeito e som). Se devolve `0`, chama `APawn::NActionStop` (`0x10558670`: `AttackStop` se a
  ação é um ataque, tipos 1, 2 ou 7; `MagicStop` se é magia; depois zera a ação), `SetMagicInfo` e
  `InitSkillProcess`. Ou seja, **qualquer skill que não esteja na lista "transiente" aborta o ataque
  em curso** do atacante, na tela dele e na de quem o vê.
- **A lista transiente** (tirada da tabela de salto do `SpawnNTransientEffect`): 2011, 2012, 2031 a
  2035, 2037 a 2039, 2042 a 2045, 2047, 2048, 2060, 2061, 2063, os cubics 4049 a 4055 e 20000 a 20002.
  Os shots **sem grade** estão nela (soulshot **2039**, spiritshot e blessed **2061**, com efeito
  `LineageEffect.e_u028_a/b` conforme o `CurWeaponType`). Os shots de grau D a S do servidor
  (soulshot 2150–2154, spiritshot 2155–2159, blessed 2160–2164) **não**: cancelam o ataque a cada uso.
- **Correção aplicada no servidor para teste** (`ShotsItemHandler.clientShotSkillId`): esses ids saem
  como 2039 (soulshot) e 2061 (spiritshot e blessed). Muda só o aviso visual e o som enviado aos
  clientes (o som é buscado pela id da skill). Não mexe na força, no consumo nem na checagem de grau.
  Alternativa só no cliente: um gancho no `SpawnNTransientEffect` que faça a mesma troca (exigiria a
  DLL nova em todos os jogadores).
- **Não verificado**: nada disso foi testado no jogo; que o efeito do acerto some só porque o ataque
  é cancelado antes do momento do golpe (é a explicação provável, o trecho não foi lido); se o
  `skillgrp` do cliente tem as skills 2150 em diante.

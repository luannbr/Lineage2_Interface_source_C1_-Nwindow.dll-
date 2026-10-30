# Fase 6 — CacheCleaner (FLUSH + OBJ GARBAGE periódicos)

Origem: NWindow_C1 `src/inject/CacheCleaner.cpp` (HEAD `4e56326`). Aqui:
`include/features/CacheCleaner.h`, `src/features/CacheCleaner.cpp`.

## O que faz

A cada 300 s de jogo **no mundo** (`State == L2CS_InGame`; a tela de login e a seleção não contam),
os comandos `FLUSH` e `OBJ GARBAGE` — o par que o Interlude roda pelo
`UIDATA_PAWNVIEWER.ExecuteCommand`. Primeiro pelo `GExec` (a cadeia de comandos do launcher),
senão `UObject::StaticExec`; se não houver saída (`GNull`/`GLog`), `UObject::CollectGarbage(RF_Native)`.
Exceção lá dentro é engolida, como o `__try` do NWindow_C1. Tick com `DeltaTime` fora de
(0, 5] s não conta.

Sem `.ini`: intervalo e FLUSH são constantes, como no NWindow_C1 (`CacheCleanInterval`,
`bCacheCleanFlush`).

## Onde entra no código

`NConsoleWnd::Tick` (`NConsoleWndInput.cpp`) → `CacheCleaner_OnTick(this, InDeltaTime)`, antes do
tick original, como no port do Tick do NWindow_C1. `include/ue2/UE2Core.h` ganhou as declarações
de `GExec`, `UObject::StaticExec` e `UObject::CollectGarbage` (exports do Core.dll; o NWindow_C1
pegava por `GetProcAddress`).

## Combat log

Depois de cada limpeza o NWindow_C1 chamava `CombatLog_OnAfterCacheClean()` (o painel perde
textura e estado de pintura com o GC/FLUSH). O combat log é do trabalhador status
(`include/features/CombatLog.h`): a chamada está atrás de `__has_include("features/CombatLog.h")`
e liga sozinha quando o header chegar (a função pode estar no namespace `nwf` ou no global). Se o
nome ou a assinatura forem outros, o build do L2 acusa no merge.

## Interruptores (30/09/2026)

Registro, `HKCU\Software\Lineage2C1\NWindow\CacheCleaner`, valores de texto lidos a cada limpeza (a cada 5 min):

- `Enable` = `0`: desliga a limpeza inteira (o padrão é `1`, como no NWindow_C1);
- `Flush` = `0`: só o `OBJ GARBAGE`, sem o `FLUSH` (o padrão é `1`).

Com `-DNW_FEATURE_LOG=ON` o `LineageWindow.log` ganha, antes e depois de cada limpeza, a memória do processo
(`private`, `working set` e o **espaço de endereços que sobra**).

## Suspeita: lag depois de um item ficar sem ícone (relato de 30/09/2026)

Relato do dono: um item ficou sem ícone; depois veio lag, o mapa passou a trocar de textura e o FPS caiu;
fechar e reabrir o jogo resolveu; pode voltar a qualquer hora. Não foi reproduzido: o `L2.log` da sessão
ruim já tinha sido sobrescrito, então isto é análise de código, **não uma causa confirmada**.

O que se sabe:

- A limpeza periódica é o único comportamento nosso que o cliente original nunca teve, e roda sozinha a cada
  5 min no mundo. `FLUSH` é o `UEngine::Flush(1)` (`Engine.dll`: o tratador chama o virtual `+0x6c` com o
  argumento 1): descarta as texturas e os buffers do dispositivo de render e o deixa **pré-carregar** de novo.
  `OBJ GARBAGE` (`Core.dll`) coleta com `RF_Native` como raiz.
- Os ícones que o NWindow segura **não** são apagados pela coleta: o `FindTexture` marca toda textura com
  `RF_Native` (`NCObject.cpp`), e é essa flag que o `OBJ GARBAGE` preserva.
- O `L2.exe`, o `Engine.dll` e o `D3DDrv.dll` **não são Large Address Aware**: o processo tem 2 GB de
  endereços (o próprio log diz `Virt=2097024K`), mesmo com muita RAM e uma RTX 3070. Uma limpeza que
  descarta e recarrega as texturas de render é o tipo de operação que fragmenta esse espaço; quando falta
  um bloco contíguo, o upload de uma textura falha (um ícone que aparece vazio) e o driver passa a
  descartar e recarregar texturas o tempo todo (o mapa trocando de textura, o FPS caindo).
- O próprio `CombatLog` já teve de aprender a se curar depois da limpeza (`RerootWindows`, `AddToRoot`,
  "FLUSH often drops the render cache and the material links").

O que não se sabe: se o ícone que sumiu era de um item cuja textura simplesmente não existe no `Icon.utx`
(aí o `FindTexture` cai no `NWindow.BlackTexture` e escreve `Can not find Texture : <nome>` no `L2.log`),
e se o lag começa perto de um múltiplo de 5 min de jogo.

Como confirmar:

1. Jogar horas com `Enable=0` (ou só `Flush=0`); se o problema não voltar, é a limpeza.
2. Se voltar, logo que acontecer **copiar o `L2.log` antes de reabrir o jogo** (o cliente o sobrescreve) e
   procurar `Can not find Texture`; anotar o item que perdeu o ícone e olhar na hora, no Gerenciador de
   Tarefas, quanto o `L2.exe` usa de memória privada (perto de 2 GB é o sinal).
3. Uma DLL de diagnóstico (`-DNW_FEATURE_LOG=ON`) mostra a memória em cada limpeza.

Alternativa de fundo, não feita: marcar o `L2.exe` como Large Address Aware (4 GB de endereços no Windows de
64 bits); mexe no executável do cliente, então fica por decisão do dono.

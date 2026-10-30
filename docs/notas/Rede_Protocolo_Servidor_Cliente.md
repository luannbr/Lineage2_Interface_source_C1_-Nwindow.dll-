# O protocolo servidor -> cliente do Chronicle 1: as duas tabelas de despacho do `Network.dll`, o transporte e o handshake

Feito em 04/10/2026, **só análise de leitura** (o único arquivo novo é este): nada foi compilado, instalado ou executado, o registro e as pastas do jogo não foram tocados, o Ghidra não foi usado.
Fonte principal: `orig/Network.dll` (build único do cliente C1, `TimeDateStamp 0x40DBD380`, **não** incremental: os endereços são VAs diretos, base `0x10000000`) lido com o capstone
(`tools/nwd/image.py`) e com programas descartáveis que escrevi no *scratchpad* da sessão (não estão no repositório; o método está no fim, §11). Conferências cruzadas:
a tabela de opcodes que o dono já extraiu para o servidor (`Lucera-Interlude-FULL-main/off-converters/c1_client_packets/c1_client_opcodes.txt`: **238 de 238 entradas iguais** às que reparseei da inicialização estática),
o código-fonte do servidor Mobius C1 (`L2J_Mobius_C1_HarbingersOfWar`, que o dono usa como referência e que fala com este mesmo cliente) e a nota irmã `Rede_Protocolo_Cliente_Servidor.md` (o sentido cliente -> servidor, escrita no mesmo dia por outro analista, sem que eu a lesse antes: **coincide** comigo em porta 2106 fixa, chave Blowfish estática, quirk da chave do jogo, *thread* de leitura do jogo, `package.dat` ausente (`h = 0`), `Init` com 2 `d` lidos e `cxxdh`; onde ela é mais detalhada, o sentido cliente -> servidor, vale ela).

Convenção: **VERIFICADO** = lido no assembly/nos bytes do `Network.dll` ou no código-fonte citado; **INFERIDO** = raciocínio sobre o que foi lido; **NÃO VERIFICADO** = só rodar o jogo diria.
Endereços sem módulo são do `Network.dll`. `slot N` = índice no vtable do `FL2NetNotify` do `UGameEngine` (`UGameEngine::On*`; assinaturas em `include/ue2/gen/FL2NetNotify.vt.inc`).

## 1. Resumo: o que o dono aprende e pode fazer com isto

1. **Todo pacote do servidor passa por `UNetworkHandler::Tick` (`0x10010730`)** e é despachado por **duas tabelas de ponteiros de função**: a da fase de login (11 entradas em `0x10041150`, opcode limitado a `0xA`)
   e a da fase de jogo (238 entradas de `0x104` bytes em `0x10041180`, opcode limitado a `0xED`). A fase vem de `[handler+0x44]` (0 = nada, 1 = login, 2 = jogo). **As §4 e §5 têm uma linha por opcode das duas tabelas** (249 linhas): nome, handler, formato lido, eventos do Engine, comparação com o Mobius.
2. **Das 238 entradas do jogo, 201 têm handler de verdade** (198 handlers distintos entre essas 201 [corrigido pela revisão: 201 − 3 pares; são 199 contando o de `0x11`, que só loga]: `0x06/0x07`, `0x08/0x09` e `0xB5/0xD2` repartem o mesmo), **24 são ignoradas** (handler `0x10001BE0` = `xor al,al; ret`), **12 são `DummyPacket`** (`0x10002130`, só logam) e **1 só loga** (`0x11`). Oito handlers agem sem disparar evento do Engine (`0x70`, `0x9C`, `0xB5`, `0xB8`, `0xC0`, `0xD2`, `0xE5`, `0xEC`).
   Essas 36 entradas sem comportamento são onde registrar um handler novo (§8).
3. **Quadro (VERIFICADO):** `uint16` little-endian com o tamanho total **incluindo** os 2 bytes + corpo (`opcode` de 1 byte + campos). **Login:** corpo em Blowfish-ECB (chave estática de 21 bytes em `0x10041138`, **idêntica à do `LoginClient` do Mobius C1**), enchimento a múltiplo de 8 e checksum XOR que o cliente
   **não confere**; o 1º pacote do servidor (`Init`) vem **em claro**. **Jogo:** XOR encadeado com chave de 8 bytes: 4 bytes vêm do `KeyPacket` e 4 são fixos (`0x87546CA1`), a chave avança pelo tamanho de cada pacote; o `KeyPacket` liga a cifra (`d2 != 0`). O cliente manda, ao conectar, `c d` = `(0, 419)` (versão do protocolo, lida de `"$Revision: 419 $"`).
4. **O leitor de campos (`0x10002A00`) não confere limites** (§3.7, §7.1): pacote **curto** lê lixo do *heap* (o buffer do payload tem 1 byte nulo a mais e nada depois); pacote **longo** é tolerado (o que sobra é ignorado). Isto permite estender um pacote **acrescentando campos no fim** (compatível com o cliente de hoje), mas nunca no meio.
5. **Cinco handlers escolhem o formato pela versão do protocolo** (`[0x10041134]` > `0x150` = 336): `CharInfo 0x03`, `UserInfo 0x04`, `NpcInfo 0x22`, `MagicSkillUse 0x5A`, `ShortCutInit 0x57`. Como este cliente manda 419, **só vale a variante longa** (§3.7).
6. **Armadilhas que o dono vai querer conhecer** (§7): `SpawnItem` lê 8 `d` (o 8º é a guinada do item) e o Mobius manda 7; `CharacterSelected` lê a hora do jogo no campo 56 (o Mobius a põe no 58; o Lucera do dono já corrigiu, comentário em `CharSelected.java`);
   `PartyMemberPosition` só lê as coordenadas de membros que ele conhece (desalinha o resto); as listas de loja têm um bloco `dhhh` que só existe para `tipo1` em 0..3; strings maiores que o buffer desalinham o pacote.
7. **Onde estender o cliente** (§8): um opcode novo é uma entrada a mais na tabela `0x10041180` (ponteiro de função `cdecl (UNetworkHandler*, BYTE* payload)`); o `NWindow.dll` pode gravar nela em tempo de execução. NÃO VERIFICADO em jogo; o mecanismo está lido.
8. **O que não cobri** está em §10 (o corpo semântico dos eventos do Engine não é tratado aqui; nada foi visto rodando; a leitura de campos por handler é estática e linear).

## 2. Método e limites

- **Tabelas.** A de login é estática (11 ponteiros em `.data`). A de jogo é preenchida pela inicialização estática (`0x1002C0F0`..`0x1002DC09`): para cada entrada, `push L"NomePacket"; push &entrada+4; mov [entrada], handler; call` (cópia do nome); reparseei as 238 (inclusive as três últimas, que usam outro endereço: `0xEB SkillRemainSecPacket`, `0xEC NetPingPacket` e `0xED DummyPacket`).
- **Handlers.** Para cada um (211 distintos no total): descida recursiva do grafo (saltos diretos e tabelas de salto), chamadas ao leitor `0x10002A00` com a *string* de formato e os destinos, laços (arestas de volta), chamadas ao `FL2NetNotify` (`call [[[0x10055308]+0x48]+off]`, por rastreio de registradores) **e às funções que o handler chama** (profundidade 3), propagação do deslocamento da pilha (207 dos 211 handlers consistentes; nos 4 restantes — `0x100075F0`, `0x10013090`, `0x100137C0`, `0x1001B490`, que chamam o gerenciador de arquivos ou destinos fora da tabela que mapeei — a análise de uso de campos não vale; o `0x100075F0` li à mão, os outros três só pelo esqueleto), *strings* de log (`"(Receive)... Nome:%d"`, que dão nomes de campo).
- **Leitura à mão** do assembly: transporte inteiro (`0x10002F00`, `0x10003150`, `0x100032E0`, `0x10003330`, `0x10002B70`, `0x10002320`, `0x10002430`, `0x10002480`, `0x10010020`..`0x10010520`, `0x10022020`, `0x10022160`, `0x1000FD60`), os 11 handlers de login, `0x00`, `0x15`, `0x1A`, `0x57`, `0x7A`, `0x9C`, `0xC0`, `0xEC` e os esqueletos de ~40 handlers com listas.
- **Nomes de campo** vêm, nesta ordem, do log do próprio cliente e da expressão que o Mobius C1 escreve na mesma posição (o alinhamento é por letra: onde as letras não batem, não dou nome). São **dicas**: o que vale é a posição e a letra.
- **O que a análise de uso diz (campos ignorados)** só é confiável em handlers que não copiam estruturas em bloco; por isso quase não afirmo "campo ignorado" (§7).

## 3. O transporte: sockets, quadro, cifras e handshake

### 3.1 Os objetos (VERIFICADO)

`UNetworkHandler::Init(INT, FL2NetNotify*)` (`0x100141D0`, exportada) guarda o `FL2NetNotify` do `UGameEngine` em `[this+0x48]`, grava a instância em `0x10055308` e cria **três objetos de socket**:

| Campo | Objeto | vtable | Papel |
|---|---|---|---|
| `[+0x40]` | `CAuthSocket` (construtor `0x10014120`, grava a chave Blowfish) | `0x10033DD0` | conexão com o **servidor de login** |
| `[+0x3C]` | `CClientSocket` (construtor `0x10013FA0`) | `0x10033D58` | conexão com o **servidor de jogo** (tem uma *thread* de recepção) |
| `[+0x70]` | `CPingSocket` (construtor `0x100074C0`, 128 entradas de 16 B) | `0x100310F0` | mede o ping dos servidores da lista (§3.8) |

Os nomes vêm das linhas de log `"CAuthSocket::OnUserNetmessage"` (em `0x10002F00`) e `"CClientSocket::OnUserNetmessage"` (em `0x10010370`) e das classes `autoclassCAuthSocket/CClientSocket/CPingSocket` exportadas. Outros campos do handler: `[+0x44]` estado, `[+0x30]` fila de pacotes (um `TArray<NetPacket*>`) com a `CRITICAL_SECTION` em `[+0x58]`, `[+0x78]` um `float` que o `Tick` e o `NetPing` usam [corrigido pela revisão: não é tempo acumulado; o `Tick` faz `[+0x78] = 0.98*[+0x78] + 0.02*x` (constantes em `0x10033584`/`0x10033580`) com `x` = um contador do `GStats` (índice `[GEngineStats+4]`) × `GSecondsPerCycle`, isto é, uma média móvel exponencial de um tempo em segundos (INFERIDO: o tempo de quadro); o `NetPing` manda `1/[+0x78]` (≈ FPS) e zera].

**Eventos de janela.** O `WinDrv.dll` importa `?DispatchSocketWindowMsg@@YAHPAUHWND__@@IIJ@Z` do `Network.dll` [corrigido pela revisão: VERIFICADO na tabela de importação de `orig/WinDrv.dll`; o sítio da chamada dentro do `WinDrv` não foi lido] e, por essa função (`0x10022160`), repassa `WM_USER+1` (`0x501`), `0x502` e `0x503`:
`0x501` -> `0x10022020`, que lê **`[URL] ServerAddr` do ini padrão** (`GConfig->GetString(L"URL", L"ServerAddr", ...)`, VERIFICADO) e grava o *hostname* em `socket+0x484C` dos dois sockets e o `HWND` em `socket+0x28`;
`0x502` (conexão concluída) -> `[socket vtable +0x64]`; `0x503` (leitura/fechamento) -> `[socket vtable +0x68]`; o socket certo é o de login se o estado é 0 ou 1 e o de jogo se é 2.
`WSAAsyncSelect` pede `FD_READ|FD_OOB|FD_CLOSE` (`0x25`); um `FD_OOB` (dado urgente TCP) ou `FD_WRITE` abre um `MessageBoxW` modal com o texto do evento (login `0x10003072`..`0x10003098`, jogo `0x10010447`/`0x10010464`): o servidor nunca deve mandar dado urgente. [corrigido pela revisão: como `FD_WRITE` (`0x02`) não está na máscara `0x25`, o ramo do `MessageBoxW("FD_WRITE")` é código morto; só o `FD_OOB` pode abrir a caixa.]
FD_CLOSE com **código de erro** chama `OnServerClose(0)` (login, `0x100030D9`) ou `OnServerClose(1)` (jogo, `0x100104A7`) no `FL2NetNotify` (slot 140); um FD_CLOSE sem erro cai num caso vazio no socket de jogo (NÃO VERIFICADO o que o resto do cliente faz com um fechamento limpo).

### 3.2 O quadro (VERIFICADO)

```
 servidor -> cliente          [uint16 LE: tamanho total, INCLUI estes 2 bytes] [corpo: opcode (1 B) + campos]
 cliente  -> servidor         idem
```

- **Recepção do login** (`0x10002F00`, na *thread* principal, por mensagem `0x503`): faz `recv` de 2 bytes (cabeçalho), guarda o tamanho em `[sock+0x5614]` (se `< 2`: log "packet size is under 2 something wrong!"), depois `recv` do resto até completar; chama `[vtable+0x6C]` (`0x10003150`); depois espia 1 byte (`recv` com `MSG_PEEK`) e repete enquanto houver dados.
- **Recepção do jogo** (`0x10010020` cria uma *thread* em `0x1000FD60`): a mensagem `0x503` só libera um semáforo (`ReleaseSemaphore`, `0x10010400`); a *thread* espera o semáforo (100 ms de *timeout*), faz `recv(sock, 0x10051300, 0x4000, 0)` (até 16 KiB de uma vez), e
  separa os quadros com a mesma máquina de estados (`[+0x561C]` 0 = esperando cabeçalho, 1 = esperando corpo; `[+0x5614]` tamanho; `[+0x5618]` bytes já lidos), decifra e enfileira. Quem trata os pacotes é a *thread* principal (`Tick`).
- **Buffer de quadro:** `socket+0x844` até `socket+0x4844` = `0x4000` = **16.384 bytes** [corrigido pela revisão: o original dizia `+0x4D2C` = 17.640; mas `+0x4844` é o contador de pacotes recebidos (`inc [this+0x4844]` em `0x10010337`/`0x10003223`), `+0x4848` outro contador zerado pelos handlers de falha, `+0x484C` o *hostname* (gravado em `0x100220BD`), `+0x494C` o semáforo, `+0x4950` a *thread*, `+0x4954..` os acumuladores; o `recv` da *thread* também lê no máximo `0x4000` por vez]. **Não há comparação do tamanho com um máximo** (a única conferência é `>= 2`): um quadro maior que 16.384 bytes sobrescreve os campos seguintes do objeto (contadores, *hostname*, ..., e a `CRITICAL_SECTION` em `+0x4D2C`). O servidor deve manter cada pacote abaixo de 16.384 bytes (INFERIDO o efeito; o fato da falta de limite é VERIFICADO).
- **`OnReceivePacket`** (login `0x10003150`, jogo `0x10010240`): se a cifra está ligada, decifra `corpo[0..tamanho-2)` no lugar; `opcode = corpo[0]`; se `tamanho == 2` (corpo vazio) loga `"Critical Bug : Bad Packet Size(size=%d) [Src\ClientSocket.cpp:...]"` e descarta [corrigido pela revisão: o teste é `lea eax,[tamanho-2]; test eax,eax; ja` — **sem sinal**, só `tamanho-2 == 0` descarta. Um cabeçalho `0` ou `1` **não chega aqui**: no jogo, a *thread* (`0x1000FE4B..0x1000FE7A`) calcula `tamanho-2` negativo como contagem do `rep movsd` e copia ~4 GB sobre o objeto (INFERIDO: *crash*); no login, `0x10003028` loga "packet size is under 2" **sem** avançar o estado e o laço `recv(2-2=0)`/`MSG_PEEK` repete sem consumir nada (INFERIDO: *hang* da *thread* principal enquanto houver dados). O cliente não tem nenhuma proteção contra cabeçalho < 2];
  senão aloca `tamanho-2` bytes (`"NetCmd"`), copia os `tamanho-3` bytes depois do opcode e grava **um byte nulo no fim**, aloca um `NetPacket` de 8 bytes (`"NetPacket"`: `[0]` opcode, `[4]` ponteiro do payload) e chama `UNetworkHandler::AddNetworkQueue` (`0x10011CA0`, vtable `+0x74`, sob a `CRITICAL_SECTION`).
  **O handler recebe o ponteiro do payload SEM o byte de opcode.**
- **Envio** (`Send` = vtable `+0x60`, `__cdecl (socket, fmt, ...)`; jogo `0x10002B70`, login `0x10003230`): empacota com `0x100026E0` (letras `c h d f S s` e `b`/`x` = tamanho + ponteiro; limite de saída `0x1FFE`; **estoura -> não envia nada**), grava o cabeçalho `(tamanho+2)`, cifra se a cifra está ligada, `send`. O `Send` do jogo soma os bytes enviados em `[sock+0x496C/0x4970]`.

### 3.3 As cifras (VERIFICADO)

**Servidor de login (`CAuthSocket`) = Blowfish-ECB + checksum XOR.** Implementação padrão de Blowfish (P de 18 palavras e 4 S-boxes de 256, em `.data`: `0x10040070`, `0x100400B8`..), cada bloco de 8 bytes como duas palavras de 32 bits **little-endian nativas**.
A chave é **estática**: 21 bytes em `0x10041138` (texto ASCII com o `NUL` final; `0x10014120` chama `0x10001880(chave, 0x15)`). Não a reproduzo aqui; conferi que os 21 bytes são **idênticos** ao literal que o `LoginClient` do Mobius C1 passa ao seu `NewCrypt` (e que são diferentes da chave dos arquivos `Lineage2Ver211`, `Engine.dll 0x10904160`).
Para conferir sem copiar: `Image('orig/Network.dll').read(0x10041138, 21)` tem `SHA-1` começando em `f75237bf`.

- *Receber* (`0x10003330`): **só decifra se o tamanho do corpo é múltiplo de 8** (`test al,7`); **não confere o checksum** (não há laço de XOR nessa função). O corpo decifrado inteiro (opcode + campos + enchimento/checksum) vai para a fila; os bytes do enchimento viram "pacote longo" (inofensivo).
- *Enviar* (`0x100032E0`): `n1 = (n + 7) & ~7` (o enchimento é de zeros, porque o buffer é zerado antes), `checksum = XOR de todas as palavras de [0, n1)`, gravado na palavra seguinte (offset `n1`), `n2 = n1 + 8` (a palavra depois do checksum fica zero), cifra `n2` bytes em ECB; cabeçalho = `n2 + 2`.
  Esta forma é a que o `NewCrypt.verifyChecksum` do Mobius aceita (XOR de tudo menos a última palavra = 0).
- A cifra do login **liga** quando o handler do `Init` (login op 0) roda (`mov byte [sock+0x4E70], 1` em `0x10007636`) e **desliga** em `RequestAuthLogin` (`0x10003407`) e nos handlers de falha (ops 1, 2, 5, 6, 8, 9 gravam 0 depois de fechar: `0x100077CF`, `0x1000789F`, `0x10007C3F`, `0x10007D0F`, `0x10007DDF`, `0x10007EFA`) [corrigido pela revisão: o fechamento em si (`0x10002E30`) não toca na flag; e o `PlayOk` (`0x10011A20`) grava **1** na flag do socket de login já fechado — inofensivo, o `RequestAuthLogin` zera de novo]. O **`Init` do servidor chega em claro**.

**Servidor de jogo (`CClientSocket`) = XOR encadeado com chave de 8 bytes que avança.** Duas chaves de 8 bytes no objeto: **TX `+0x4E78`** (cliente -> servidor) e **RX `+0x4E80`** (servidor -> cliente), as duas iniciadas com o mesmo valor pelo `KeyPacket` (§3.4). Flag de ligado em `+0x4E70`.

```
RX (0x10002480, ret 0xC: dados, chave, tamanho; tamanho = bytes do corpo, sem o cabeçalho de 2 B):
    p[0] = c[0] ^ k[0]
    p[i] = c[i] ^ k[i & 7] ^ c[i-1]            (i = 1 .. n-1; c = byte cifrado de entrada)
    k (64 bits, little-endian) += n             (depois do pacote; adição com extensão de sinal de n)
TX (0x10002430): c[0] = p[0] ^ k[0];  c[i] = p[i] ^ k[i & 7] ^ c[i-1]   (c[i-1] já cifrado);  k += n
```

É a conhecida cifra "dual key" do L2 antigo (o servidor aplica a função TX do cliente ao que manda e a RX ao que recebe, com as chaves espelhadas e avançando o mesmo tanto).

### 3.4 O handshake do servidor de jogo (VERIFICADO)

| # | Dir. | Opcode e formato | O que acontece | Evidência |
|--:|---|---|---|---|
| 1 | C->S | `cd` = `(0, 419)`, **em claro** | enviado quando a conexão se completa (mensagem `0x502`, `0x10002320`); a versão sai de `sscanf("$Revision: 419 $", "%cRevision: %d %c")` e fica em `[0x10041134]` | `0x100023A5`..`0x100023C9` |
| 2 | S->C | game op `0x00` (`VersionCheck`, o `KeyPacket`): `cdd` | `c` = resultado (empilhado num `L2ParamStack` e entregue a `OnReceiveVersionCheck`, slot 194 do `FL2NetNotify`); `d1` = 4 bytes baixos da chave; **`d2 != 0` liga a cifra** (`setne dl; mov [+0x4E70], dl`); as duas chaves := `d1` (baixa) e `0x87546CA1 \| sinal(d1)` (alta) | `0x10007F40`..`0x10007FCA` |
| 3 | C->S | op `0x08`: `cSdddd` = `(8, conta UTF-16, d, d, d, d)`, **cifrado se `d2 != 0`** | `(playOk2, playOk1, loginOk1, loginOk2)`: `[0x100567C4]`, `[0x100567C8]` (guardados pelo `PlayOk` do login), `[0x100567AC]`, `[0x100567B0]` (guardados pelo `LoginOk`) | `0x10008037`..`0x1000806A` |

- Quirk da chave: a metade alta é calculada como `0x87546CA1 | (sinal de d1 estendido)`: com `d1 >= 0` a chave de 8 bytes é `d1(4 B LE) A1 6C 54 87`; com `d1 < 0` o `cdq` faz a metade alta virar `0xFFFFFFFF` (`FF FF FF FF`). **Um servidor tem de usar o mesmo cálculo** (o Mobius C1 manda a chave com `a1 6c 54 87` fixos nos bytes 4..7, e esses mesmos bytes viram o `d2` != 0 que liga a cifra).
- O `KeyPacket` do Mobius tem 17 bytes de corpo (`c`, 8 bytes de chave, `d` serverId, `d` 1); o cliente lê 9 (`cdd`) e ignora o resto.
- O que o servidor manda depois do `AuthLogin` (a lista de personagens, `0x1F`) e o que o cliente pede em seguida (`RequestGameStart` = `cd` op `0x0D`, `RequestEnterWorldPacket` = `c` op `0x03`) estão no Apêndice A; **quando** o cliente manda o op `0x03` NÃO VERIFICADO (depende do `UGameEngine`).

### 3.5 O fluxo do servidor de login (VERIFICADO, salvo onde marcado)

| # | Dir. | Opcode/formato | O que acontece | Evidência |
|--:|---|---|---|---|
| 1 | UI | `UNetworkHandler::RequestAuthLogin(id, senha)` | copia o id (até 14 `wchar`) para `0x100567CC` e a senha (até 16) para `0x100567EA`; desliga a cifra do login; conecta em `ServerAddr` porta **2106** (`0x83A`, fixa); estado := 1 | `0x100033A0`..`0x10003422` |
| 2 | S->C | login op `0x00` (`Init`), **em claro**: lê `dd` | os dois `d` são descartados (slots locais zerados e nunca relidos); estado := 1; **liga a cifra Blowfish**; resposta imediata abaixo. O resto do pacote (no Mobius: módulo RSA de 0x80 bytes + 16 bytes) é ignorado | `0x100075F0` |
| 3 | C->S | op `0x00`: `cxxdh` = `(0, conta[14 B ANSI], senha[16 B ANSI], 8, atoi(package.dat))` | conta/senha convertidas por `WideCharToMultiByte`; **`package.dat`** (pasta do jogo, via `GFileManager`) é lido inteiro e passado a `atoi` (o `h` é esse número; 0 se o arquivo não abre); `d = 8` constante | `0x10007604`..`0x10007744` |
| 4 | S->C | op `0x03` `LoginOk`: `d×8` | `w1..w4` -> globais `0x100567AC/B0/B4/B8`; `w5..w7` descartados; `w8` -> `OnStartNotice(L2ParamStack)` (slot 120). **Não** fecha o socket nem muda o estado | `0x100078F0` |
| 5 | C->S | op `0x05`: `cddc` = `(5, w1, w2, 3)` | `RequestServerList` (pede a lista) | `0x10003460` |
| 6 | S->C | op `0x04` `ServerList`: `cc` + n × `cddcchhcd` | `OnStartServerList(2º c)`, `OnAddServerList(L2ParamStack[9])` por servidor, `OnEndServerList(0)` (slots 28, 29, 31) | `0x100079F0` |
| 7 | C->S | op `0x02`: `cddc` = `(2, w1, w2, serverId)` | `RequestServerLogin`: grava `serverId` em `0x100567A8`, IP em `0x100567BC` e porta em `0x100567C0` (os três vêm do `L2ParamStack` da entrada escolhida) | `0x100034C0` |
| 8 | S->C | op `0x07` `PlayOk`: `dd` | `w1` -> `0x100567C8`, `w2` -> `0x100567C4`; estado := 0, fecha o socket de login; `inet_ntoa([0x100567BC])` e conecta o **servidor de jogo** em `IP:[0x100567C0]` (`CClientSocket::Connect`, `0x10010020`); estado := 2. O IP é lido como `in_addr` direto: **os 4 bytes do campo `d` são os octetos na ordem a.b.c.d** | `0x100119B0`..`0x10011A44` |

Falhas: op `0x01` (`c` reason sem sinal), `0x05` (`c`), `0x06` `PlayFail` (`c`), `0x08` (`c`) -> `OnAuthLoginFail(reason)` (slot 32); op `0x02` -> `OnBlockedAccount(d)` (34); op `0x09` -> `OnBlockedAccountWithMsg(S)` (33), uma vez por linha.
**Os ops 1, 2, 5, 6, 7, 8 e 9 fecham o socket de login e zeram o estado** (chamam `0x10002E30`: `closesocket` + `DeleteCriticalSection`); os ops 0, 3 e 4 não.

### 3.6 Despacho: `UNetworkHandler::Tick` (VERIFICADO)

```
Tick(dt):                                             // 0x10010730; roda na thread principal
  repete:  pkt = DispatchNetworkQueue()               // vtable +0x70 (0x10011B80), sob a CRITICAL_SECTION; devolve 0 se vazia
    se estado == 1:  op = min(pkt.opcode, 0x0A);  handler = tabela_login[op]       // 0x10041150 (4 B)    0x10010965..0x10010986
    se estado == 2:  op = min(pkt.opcode, 0xED);  handler = tabela_jogo[op].handler // 0x10041180 + op*0x104   0x100109BD..0x10010A0E
    senão (estado 0): o pacote é descartado
    handler(this, pkt.payload)                         // cdecl: push payload; push this; call; add esp, 8; o retorno (al) é ignorado
    libera payload e pkt
  quando a fila esvazia: chama o virtual [vtable+0x398] (INFERIDO: o `Tick` da classe base) e copia contadores de estatística
```

- **Todos os pacotes enfileirados são tratados no mesmo `Tick`** (não há limite por quadro).
- O opcode clampa para a última entrada: login `>= 0x0A` -> `0x0A` (Dummy); jogo `>= 0xED` -> `0xED` (`DummyPacket`). **Um opcode desconhecido não derruba nada**: só grava "(Receive)Dummy".
- Um pacote que chega com o estado errado (p.ex. dados do login depois do `PlayOk`) é tratado pela tabela da fase atual (INFERIDO: a fila não guarda de qual socket veio).
- `Tick` tem um bloco `SEH` que loga `"UNetworkHandler::Tick"` e `"Function Name=%s"` (o nome da entrada da tabela) se um handler lançar exceção (`0x10010AEE`..`0x10010B37`).
- **Perfilador embutido:** com `[[handler+0x3C]+0x4D28] != 0` o `Tick` mede ciclos (`rdtsc`) por opcode (`0x100109D5`..`0x10010A6A`); liga/desliga por `UNetworkHandler::ServerPacketCountStart/Stop` (`0x10003350/0x10003360`, comandos de console `PACKETCOUNTSTART/STOP`).
- Cada handler loga uma linha `"(Receive)Nome ..."` no `GNetworkLog` **e** na janela de console do jogo (`GL2Console` vslot `+0x1FC`, nível 2). Um servidor que manda muitos pacotes enche a janela de console.

### 3.7 O leitor `0x10002A00` e a versão do protocolo (VERIFICADO)

`leitor(ctx, buf, fmt, saídas...)` devolve o ponteiro depois do último byte consumido (`ctx` não é usado). Para cada letra do formato:

| Letra | Bytes do fio | Saída |
|---|---|---|
| `c` | 1 | `*(BYTE*)`; quando o handler zerou o `DWORD` antes (VERIFICADO nos ops 1/5/6/8 do login) o valor chega **sem sinal**; há handlers que leem com `movsx` (login op 9) |
| `h` | 2 | `*(WORD*)` |
| `d` | 4 | `*(DWORD*)` |
| `f` | 8 | 8 bytes copiados (um `double`) |
| `S` | UTF-16 terminada em zero | dois argumentos: **tamanho do buffer em bytes (por valor)** e o ponteiro; copia no máximo `tamanho/2 - 1` caracteres, grava o `NUL`, e **avança `min(len, máx)*2 + 2` bytes** |
| `s` | ANSI terminada em zero | tamanho + destino; copia `tamanho` bytes **do fio** (mesmo além do `NUL`), `dest[tamanho-1] = 0`, avança até depois do primeiro `NUL` dentro de `tamanho` (**nenhum handler usa `s`**) |
| outras | - | só grava a linha de log `"unknown type %c in %s"`; **não consome nada** |

**Não há verificação de fim de pacote.** O corpo do payload é seguido de **um** byte nulo e, depois, do que houver no *heap*.

**Versão do protocolo.** `[0x10041134]` recebe a versão no `Send Protocol Version` (`0x100023C9`): 419 neste cliente. Os handlers abaixo comparam com `0x150` (336) e **só a variante `> 336` é usada** por este cliente:

| Opcode | `<= 336` | `> 336` (a que vale) |
|---|---|---|
| `0x03` CharInfo | `d×5 S d×27 f×4 d×3 S d×3 c×7` | `d×5 S d×27 f×4 d×3 S d×5 c×7 h` + `h×N` + `c` |
| `0x04` UserInfo | `d×5 S d×69 f×4 d×4 S d×3 c×3 d d` | `d×5 S d×69 f×4 d×4 S d×5 c×3 d d h` + `h×N` + `c` |
| `0x22` NpcInfo | `d×18 f×4 d×3 c×5` | `d×18 f×4 d×3 c×5 S S d×3` |
| `0x5A` MagicSkillUse | `d×6` | `d×9 h` + `h×N` |
| `0x57` ShortCutInit | **ignorado** (o handler pula tudo) | `d` + N × (...) |

(Os formatos exatos de cada variante estão nos blocos de campos da §6; as colunas acima são as letras que o leitor recebe.)
Além disso `0x1000257B` compara com `0x16B` (363): o `RequestAllyCrest` só é feito com versão `>= 363`.

### 3.8 O `CPingSocket` e o `NetPing` (VERIFICADO / INFERIDO)

- **`CPingSocket`** (`0x10010520`): para cada servidor da lista (até 128) cria um socket TCP, conecta, mede o tempo do `connect` e **envia `c d` = `(0, -2)`** (`0x10010661`..`0x10010667`: versão de protocolo `-2`, a "sonda" de ping do L2; o servidor deve responder fechando). O objeto não tem `recv`. O resultado volta por `FL2NetNotify::SetServerPing(INT index, DWORD ms)` (slot **295**; falha = `0x270F` = 9999 ms).
  Não toca nas tabelas de despacho.
- **`NetPing`** (servidor -> cliente `0xEC`, `d`): o handler lê o `d` e chama `UNetworkHandler::NetPing(d)` (vtable `+0x37C`, `0x100071F0`), que responde ao servidor com `cddd` opcode **`0xA8`** = `(0xA8, d ecoado, ftol(1/[this+0x78]) ≈ FPS, ftol(GL2PawnCR × const × const))` e depois zera `[this+0x78]` [corrigido pela revisão: a ordem dos dois últimos `d` e o `1/x` lidos em `0x100071F5..0x1000724A`; `[+0x78]` é a média móvel do §3.1, não um acumulador]. Serve de *keep-alive* do jogo.

## 4. A tabela da fase de login (11 entradas, `0x10041150`)

Estado `[handler+0x44] == 1`; opcode limitado a `0x0A`; `handler(this, payload)`. O payload é o corpo **sem o opcode**, já decifrado (§3.2, §3.3); o leitor não confere limites (§7.1). A coluna "fecha" diz se o handler chama `0x10002E30` (fecha o socket de login, estado := 0). Os nomes são os das linhas de log dos handlers (a tabela de login não tem nomes).

| Op | Handler | Nome (log) | Formato lido | Eventos / efeito | Fecha | Notas |
|--:|---|---|---|---|---|---|
| 0x00 | 0x100075F0 | (Init; sem log, abre `package.dat`) | `d d` (e o resto do corpo é ignorado) | estado := 1; liga a cifra; **responde `cxxdh`** (§3.5 passo 3); nenhum evento | não | Os dois `d` não são usados. No Mobius o `Init` tem `d sessionId, d 0x0000C621, 128 B de RSA embaralhado, 4 d`; o cliente só lê os 8 primeiros bytes. VERIFICADO |
| 0x01 | 0x10007760 | LoginFailPacket2 | `c` (reason, sem sinal) | slot 32 `OnAuthLoginFail(reason)` | sim | Os códigos 101..199 chegam inteiros. Log "(Receive)LoginFailPacket2 errorcode:%d" |
| 0x02 | 0x10007830 | AccountKickedPacket | `d` | slot 34 `OnBlockedAccount(d)` | sim | máscara de 16 bits |
| 0x03 | 0x100078F0 | LoginOkPacket | `d×8` | slot 120 `OnStartNotice(L2ParamStack{w8})` | não | `w1,w2` -> `0x100567AC/B0` (**as chaves de login** reenviadas em ops 5, 2 e no `AuthLogin` do jogo); `w3,w4` -> `0x100567B4/B8` (sem uso achado: INFERIDO reservado); `w5..w7` descartados |
| 0x04 | 0x100079F0 | ServerListPacket | `c c` + n × `c d d c c h h c d` | slot 28 `OnStartServerList(2º c)`; por servidor slot 29 `OnAddServerList(L2ParamStack[9])`; no fim slot 31 `OnEndServerList(0)` | não | 1º `c` = n (nº de servidores, sem sinal); 2º `c` = último servidor usado. Por servidor: `serverId c`, `ip d` (octetos a.b.c.d, ver §3.5), `port d`, `ageLimit c`, `pvp c`, `users h`, `maxUsers h`, `status c`, `flags d` (comentário do `ServerList.java` do Mobius: `0x02` relógio, `0x04` não mostrar o nome, `0x08` servidor de teste; o uso no cliente não foi conferido). O Mobius ainda escreve um `c` final por servidor (colchetes), que o cliente não lê |
| 0x05 | 0x10007BD0 | ServerFailPacket | `c` | slot 32 `OnAuthLoginFail` | sim | |
| 0x06 | 0x10007CA0 | PlayFailPacket | `c` | slot 32 `OnAuthLoginFail` | sim | resposta negativa ao `RequestServerLogin` |
| 0x07 | 0x100119B0 | (PlayOk; sem log) | `d d` | **nenhum evento**: grava `0x100567C8`, `0x100567C4`, fecha o login, conecta o jogo, estado := 2 | sim | §3.5 passo 8. Aqui sai do login e entra na tabela de jogo |
| 0x08 | 0x10007D70 | AccountKickedPacket (2ª) | `c` | slot 32 `OnAuthLoginFail` | sim | mesmo nome de log do op 2, handler diferente |
| 0x09 | 0x10007E30 | BlockedAccountWithMsgPacket | `c` n + n × `h S` | slot 33 `OnBlockedAccountWithMsg(S)` por linha | sim | `c` com sinal (`n <= 0` não lê linha nenhuma); `S` até 511 chars; o `h` é lido e descartado |
| 0x0A | 0x10002130 | Dummy | - | só grava "(Receive)Dummy" | não | também é o destino de todo opcode >= 0x0A |

## 5. A tabela da fase de jogo (238 entradas, `0x10041180`)

Estado `[handler+0x44] == 2`; opcode limitado a `0xED`; entrada de `0x104` bytes: `+0x00` handler, `+0x04` nome em UTF-16 (até 0x100 bytes). Colunas:

- **Formato lido**: letras que o handler passa ao leitor (`c`=1 B, `h`=2 B, `d`=4 B, `f`=8 B, `S`=UTF-16 terminada em zero); `×n` repete; `N×( ... )` é um laço cujo contador é o último `h`/`d` lido (o contador é o campo indicado quando não é óbvio); `‖` separa o que vem antes do laço; `;` separa leituras consecutivas do mesmo handler.
  Para os cinco handlers dependentes de versão mostro só a variante `> 336` (a que este cliente usa, §3.7). `S[x]` = com limite de caracteres na coluna Notas (`S#i ≤n chars`, `i` = índice da letra **dentro da chamada ao leitor** que a lê; cada chamada conta desde 0).
- **Eventos**: `slot nome`; `(via)` = o evento sai de uma função que o handler chama (profundidade 3), não do handler.
- **Mobius C1**: `=` formato igual ao do servidor Mobius C1; `M+n` o Mobius manda n campos a mais (o cliente os ignora); `C+n` o cliente lê n campos **a mais** do que o Mobius manda (lê lixo); `=p` o prefixo fixo (antes do 1º laço/condição) é igual; `~` não comparável/estrutura variável; `-` sem classe correspondente.
  (Comparação por letras do `writeImpl` do Mobius C1 contra as leituras do cliente; só a **estrutura** é comparada, não o significado.)
- **Notas**: `campos:` nomes de campo (log do cliente > expressão do Mobius na mesma posição; **dicas**); limite de caracteres das strings; e o que li no handler.

**Pacote curto ou longo:** vale para **todas** as linhas o comportamento da §7.1 (o leitor não confere o fim: curto lê lixo do *heap*, longo ignora a sobra), salvo o que a coluna Notas diz de diferente (strings truncadas, `PartyMemberPosition`, listas com bloco condicional). **Objeto desconhecido:** os handlers que recebem um `objectId` procuram o objeto no mapa global `0x1005680C`; se o objeto não existe ou o tipo não é o esperado, o handler **pula o evento em silêncio** (VERIFICADO em `StatusUpdate`, `StartRotating`, `StopMoveWithLocation`, `UserInfo`, `Attack`; INFERIDO para os demais).

Nomes dos eventos e assinaturas: `include/ue2/gen/FL2NetNotify.vt.inc`.

| Op | Nome (tabela) | Handler | Formato lido (c=1B h=2B d=4B f=8B S=UTF-16z) | Eventos `UGameEngine::On*` (slot) | Mobius C1 | Notas |
|---|---|---|---|---|---|---|
| 0x00 | VersionCheckPacket | 0x10007F40 | `c d d` | 194 `OnReceiveVersionCheck` | ~ (KeyPacket) | KeyPacket (§3.4): c = resultado (vai só ao `OnReceiveVersionCheck`), d1 = metade baixa da chave XOR, d2 != 0 liga a cifra do jogo; o cliente responde na hora com `cSdddd` (op 8). |
| 0x01 | MoveToLocationPacket | 0x10014D00 | `d×7` | 1 `OnMoveToLocation` | = (MoveToLocation) | campos: d:objectId d:xDst d:yDst d:zDst d:x d:y d:z |
| 0x02 | NpcSayPacket | 0x100080C0 | `d×3 S` | 11 `OnNpcSay` | = (NpcSay) | campos: d:CreatureID d:ClientId d:classid S:Msg ; S#3 ≤1023 chars ; npcId do fio = id do `npcname` + 1.000.000. |
| 0x03 | CharInfoPacket | 0x10015730 | `d×5 S[x] d×27 f×4 d×3 S[t] d×5 c×7 h ‖ h×N ‖ c   (variante >336)` | 12 `OnCharInfo`; 42 `OnEquipItem`; 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via) | =p (CharInfo) | S#5 ≤255 chars, S#40 ≤31 chars ; Duas variantes por `[0x10041134]` (§3.7). Nome S[x] 255 chars, título S[t] 31 chars. O handler tem a mensagem '[Error] Can not find the Vehicle data [CharacterInfoPacket]' (INFERIDO: sai quando o campo do veículo não acha o objeto). Campo a campo: §6.2. |
| 0x04 | UserInfoPacket | 0x100165D0 | `d×5 S[x] d×69 f×4 d×4 S[t] d×5 c×3 d d h ‖ h×N ‖ c   (variante >336)` | 13 `OnUserInfo`; 40 `OnEquipItemPlaySound`; 42 `OnEquipItem`; 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via); 226 `EnablePartyChat`; 227 `EnablePledgeChat`; 228 `EnableAllyChat` | =p (UserInfo) | S#5 ≤31 chars, S#83 ≤31 chars ; Duas variantes (§3.7). Nome e título S 31 chars. Detalhe campo a campo em §6.1. |
| 0x05 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x06 | AttackPacket | 0x10017DD0 | `d d d c d d d h ‖ N×( d d c )   (N = último h)` | 14 `OnAttack`; 15 `OnAttack`; 275 `OnAssociatedAttack` | ~ (Attack) | Golpe 1 embutido (alvo, dano, flags); `h` = nº de golpes extras, cada um `d d c`. O xyz do alvo que o Mobius manda no fim NÃO é lido. Flags testados: 0x80, 0x40, 0x20, 0x10 (INFERIDO: errou, crítico, escudo, soulshot). |
| 0x07 | AttackPacket | 0x10017DD0 | `idem 0x06` | 14 `OnAttack`; 15 `OnAttack`; 275 `OnAssociatedAttack` | - | Mesmo handler do 0x06 (os dois opcodes são equivalentes). |
| 0x08 | AttackedPacket | 0x10018580 | `d d` | 17 `OnAttacked` | - | campos: d:AttackerID d:DefenderID ; Mesmo handler do 0x09. |
| 0x09 | AttackedPacket | 0x10018580 | `d d` | 17 `OnAttacked` | - | campos: d:AttackerID d:DefenderID ; Mesmo handler do 0x08. |
| 0x0A | AttackCanceledPacket | 0x10018760 | `d` | 18 `OnAttackCanceled` | - | campos: d:AttackerID |
| 0x0B | DiePacket | 0x10018880 | `d×7` | 19 `OnDie` | = (Die) | campos: d:ID d:canTeleport d:toHideout d:toCastle d:toSiegeHQ d:sweepable d:fixedRes ; O cliente lê 7 d. [corrigido pela revisão: o Mobius C1 também manda **7** `d` — `objectId, canTeleport`, depois **ou** os 3 flags do clã **ou** 3 zeros (`if/else` em `Die.java`), depois `sweepable, fixedRes`; o "10" do original somava os dois ramos do `if/else`.] |
| 0x0C | RevivePacket | 0x10018BA0 | `d` | 22 `OnRevive` | = (Revive) | campos: d:ID |
| 0x0D | AttackOutofRangePacket | 0x10018CC0 | `(sem leitura)` | 23 `OnAttackOutofRange` | - |  |
| 0x0E | AttackinCoolTimePacket | 0x10018DC0 | `(sem leitura)` | 24 `OnAttackinCoolTime` | - |  |
| 0x0F | AttackDeadTargetPacket | 0x10018EC0 | `(sem leitura)` | 25 `OnAttackDeadTarget` | - |  |
| 0x10 | LeaveWorldPacket | 0x10018FC0 | `d` | 26 `OnLeaveWorld` | - | campos: d:ID ; objectId; evento 26 (`OnLeaveWorld` é STUB no Engine; recebe o `User*` ou 0 se o id não é um User conhecido) e depois remove/libera o objeto do mapa e chama 5 limpadores de mapas do `CClientSocket` (0x100125E0, 0x10012710, 0x10012760, 0x10012800, 0x10012790) [corrigido pela revisão: VERIFICADO — a chamada é **incondicional** (todos os caminhos passam por `0x100192B2`) e cada limpador esvazia um `TMap` do socket: `+0x4D58` (libera os valores com `GMalloc->Free`: INFERIDO o cache `PledgeInfo`), `+0x4D6C`, `+0x4D80`, `+0x4DA8` (libera os valores: o mapa de posição dos membros do grupo do `0xC0`) e `+0x4D94`; depois `FArray::Realloc(0)` e reinicialização com 8 baldes (`0x10011660`)]. |
| 0x11 | AuthLoginSuccessPacket | 0x10001BF0 | `(sem leitura)` | - | - | Só grava a linha de log "(Receive)AuthLoginSuccess ". |
| 0x12 | AuthLoginFailPacket | 0x100081F0 | `d` | 32 `OnAuthLoginFail` | - | campos: d:errorcode ; reason `d` -> mesmo evento do login (slot 32). [corrigido pela revisão: antes do evento o handler põe o estado `[handler+0x44] := 0` (`0x10008212`) e chama `0x100022D0` no socket de jogo (`closesocket`, `DeleteCriticalSection`, zera `+0x2C`/`+0x4950` e a flag `[0x1005530C]` que mantém a *thread* de recepção viva): depois dele todo pacote é descartado pelo `Tick` até um novo `RequestAuthLogin`.] |
| 0x13 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x14 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x15 | SpawnItemPacket | 0x10019320 | `d×8` | 38 `OnSpawnItem` | C+1 (SpawnItem) | campos: d:ItemID d:DisplayId d:x d:y d:z d:isStackable d:Count d ; 8º d lido = guinada (yaw) do item (VERIFICADO: vira `FRotator.Yaw` do evento); o Mobius manda só 7 d (§7.2). |
| 0x16 | DropItemPacket | 0x100195C0 | `d×9` | 39 `OnDropItem` | = (DropItem) | campos: d:UserID d:ItemID d:DisplayId d:x d:y d:z d:isStackable d:Count d:1 |
| 0x17 | GetItemPacket | 0x10019930 | `d×5` | 41 `OnGetItem` | = (GetItem) | campos: d:UserID d:ItemID d:X d:Y d:Z |
| 0x18 | EquipItemPacket | 0x10019B50 | `d×3` | 42 `OnEquipItem` | - | campos: d:UserID d:ItemID d:ItemSlot |
| 0x19 | UnequipItemPacket | 0x10019D40 | `d×3` | 43 `OnUnequipItem` | - | campos: d:UserID d:ItemID d:ItemSlot |
| 0x1A | StatusUpdatePacket | 0x10019F30 | `d d ‖ N×( d d )   (N = 2º d)` | 28 eventos (slots 44 a 71) | =p (StatusUpdate) | Atributos 0..27; atributo > 27 é ignorado; objectId inexistente descarta o pacote inteiro. |
| 0x1B | NpcHtmlMessagePacket | 0x10008280 | `d S` | 72 `OnNpcHtmlMessage` | M+1 (NpcHtmlMessage) | campos: d:UserID S:Html ; S#1 ≤8191 chars ; objectId (log "UserID") + HTML S de até 8191 chars (buffer 16 KiB). O Mobius manda um `d` a mais no fim (ignorado). |
| 0x1C | SellListPacket | 0x10008B80 | `d d h ‖ N×( h  d d d h h  [ d h h h  só se 0<=1º h<=3 ]  d )` | 80 `OnStartSellList`; 81 `OnAddSellList` | =p (SellList) | Header: `d` (adena), `d`, `h` N; item com bloco `dhhh` só se o tipo1 (1º h) está em 0..3 (VERIFICADO em 0x10008C77..7C). |
| 0x1D | BuyListPacket | 0x10008D30 | `d d h ‖ N×( h  d d d h h  [ d h h h  só se 0<=1º h<=3 ]  d )` | 82 `OnStartBuyList`; 83 `OnAddBuyList` | =p (BuyList) | Idem 0x1C (BuyList). O Mobius escreve o bloco `dhhh` sempre: item com tipo1 > 3 desalinha (a correção já está no porte do dono, PORTE-C1 §1). |
| 0x1E | DeleteObjectPacket | 0x1001B490 | `d` | 88 `OnDeleteUser`; 89 `OnDeleteItem`; 109 `OnDeleteVehicle`; 231 `OnHidePetWnd`; 268 `OnDeleteStaticObject` | = (DeleteObject) | campos: d:ObjectID ; objectId: o tipo do objeto no mapa 0x1005680C (`{tipo, ptr}`; 1 = User e 2 = Item, VERIFICADO em `UserInfo`/`SpawnItem`) escolhe o evento: User -> 88 (e 231 se for pet), Item -> 89, Vehicle -> 109, StaticObject -> 268 (os dois últimos INFERIDOS pela ordem das chamadas). |
| 0x1F | CharacterSelectionInfoPacket | 0x10008EE0 | `d ‖ N×( S[x] d S[x] d×10 f f d×46 f f d )` | 121 `OnReceiveCharacterInfo`; 122 `AddCharacterInfo` | =p (CharSelectionInfo) | S#0 ≤31 chars, S#2 ≤29 chars ; Resposta ao AuthLogin do jogo (INFERIDO). Nome S 31 chars; 2ª S 29 chars. Campo a campo: §6.5. |
| 0x20 | LoginFailPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x21 | CharacterSelectedPacket | 0x10009230 | `S d S d×10 f f d×41 ; d   (hora do jogo)` | 135 `OnReceiveCharacterSelectedPacket`; 278 `OnReceiveServerTime` | ~ (CharSelected) | S#0 ≤31 chars, S#2 ≤29 chars ; Só `x y z` (campos 10..12) e a hora (campo 56, minutos do dia) chegam ao Engine; o resto é lido e descartado (INFERIDO). Campo a campo: §6.6. |
| 0x22 | NpcInfoPacket | 0x1001BD60 | `d×18 f×4 d×3 c×5 S[x] S[x] d×3   (variante >336)` | 92 `OnNpcInfo` | = (AbstractNpcInfo) | S#30 ≤31 chars, S#31 ≤31 chars ; Variante >336 (§3.7): nome S e título S (31 chars) + 3 d. Campo a campo: §6.3. |
| 0x23 | NewCharacterSuccessPacket | 0x1000A660 | `d ‖ N×( d×20 )` | 125 `OnAcceptNewCharacter`; 136 `AddRecommandedData`; 137 `EndRecommandedData` | =p (NewCharacterSuccess) | Lista de classes iniciais: N×20 d (`AddRecommandedData`). Pacote NewCharacterSuccess. |
| 0x24 | NewCharacterFailPacket | 0x10001CA0 | `(sem leitura)` | 125 `OnAcceptNewCharacter` | - | Sem corpo: `OnAcceptNewCharacter(0)`. |
| 0x25 | CharacterCreateSuccessPacket | 0x10001CC0 | `(sem leitura)` | 126 `OnAcceptCharacterCreate` | M+1 (CharCreateOk) | Sem corpo (o Mobius manda um `d`, ignorado). |
| 0x26 | CharacterCreateFailPacket | 0x1000A780 | `d` | 35 `OnCharacterCreateFail` | = (CharCreateFail) | campos: d:errorcode ; reason 0..3. |
| 0x27 | ItemListPacket | 0x1000A800 | `h h ‖ N×( h  d d d h h h d h h )` | 127 `OnReceiveItemList`; 138 `AddInventoryItem`; 189 `OnUpdateShortCutItem` | =p (ItemList) | `h` 1 = mostrar a janela (OnReceiveItemList), `h` N itens; por item um `h` solto + `dddhhhdhh`. |
| 0x28 | SunRisePacket | 0x10001CE0 | `(sem leitura)` | 278 `OnReceiveServerTime` | = (SunRise) | Sem corpo: hora := 360 (06:00). |
| 0x29 | SunSetPacket | 0x10001D00 | `(sem leitura)` | 278 `OnReceiveServerTime` | = (SunSet) | Sem corpo: hora := 0 (00:00). |
| 0x2A | EquipItemSuccessPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x2B | EquipItemFailPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x2C | UnEquipItemSuccessPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x2D | UnEquipItemFailPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x2E | TradeStartPacket | 0x1000A9B0 | `d h ‖ N×( h  d d d h h d h h h )` | 129 `OnReceiveTradeStart`; 130 `OnReceiveAddTradeInventoryItem` | =p (TradeStart) | Parceiro `d`, N itens (um `h` solto + registro). |
| 0x2F | TradeStartOkPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x30 | TradeOwnAddPacket | 0x1000AB20 | `h ‖ N×( h  d d d h h d h h h )` | 131 `OnReceiveAddTradeOwnItem` | ~ (TradeOwnAdd) |  |
| 0x31 | TradeOtherAddPacket | 0x1000AC60 | `h ‖ N×( h  d d d h h d h h h )` | 132 `OnReceiveAddTradeOtherItem` | ~ (TradeOtherAdd) |  |
| 0x32 | TradeDonePacket | 0x1000B260 | `d` | 134 `OnReceiveTradeDoneMsg` | = (TradeDone) | campos: d |
| 0x33 | CharacterDeleteSuccessPacket | 0x10001D50 | `(sem leitura)` | 124 `OnAcceptCharacterDelete` | = (CharDeleteSuccess) |  |
| 0x34 | CharacterDeleteFailPacket | 0x1000B2E0 | `d` | 124 `OnAcceptCharacterDelete` | = (CharDeleteFail) | campos: d:error |
| 0x35 | ActionFailPacket | 0x10001DA0 | `(sem leitura)` | 93 `OnActionFail` | = (ActionFailed) | Sem corpo: `OnActionFail` (zera `UEngine+0x60`). |
| 0x36 | SeverClosePacket | 0x10001DF0 | `(sem leitura)` | 140 `OnServerClose` | = (ServerClose) | Sem corpo: `OnServerClose` (o mesmo slot que o socket chama em FD_CLOSE, §3.1). |
| 0x37 | InventoryUpdatePacket | 0x1000B350 | `h ‖ N×( h h  d d d h h h d h h )` | 128 `OnReceiveUpdateItemList`; 189 `OnUpdateShortCutItem` | ~ (InventoryUpdate) | Por item: `h` changeType (2 e 3 disparam `OnUpdateShortCutItem`: INFERIDO 1 add, 2 modify, 3 remove) + `h` + registro de item. |
| 0x38 | TeleportToLocationPacket | 0x1001D030 | `d×4` | 94 `OnTeleportToLocation` | M+2 (TeleportToLocation) | campos: d:id d:x d:y d:z ; objectId, x, y, z (o Mobius manda 2 d a mais). |
| 0x39 | TargetSelectedPacket | 0x1001D1B0 | `d×5` | 95 `OnTargetSelected` | M+1 (TargetSelected) | campos: d:actorid d:targetid d:x d:y d:z |
| 0x3A | TargetUnselectedPacket | 0x1001D7C0 | `d×4` | 97 `OnTargetUnselected` | M+1 (TargetUnselected) | campos: d:id d:x d:y d:z |
| 0x3B | AutoAttackStartPacket | 0x1001D920 | `d` | 98 `OnAutoAttackStart` | = (AutoAttackStart) | campos: d:id |
| 0x3C | AutoAttackStopPacket | 0x1001DA40 | `d` | 99 `OnAutoAttackStop` | = (AutoAttackStop) | campos: d:id ; Nota: o Mobius usa o opcode 0x3C (60) para JoinParty (erro dele, PORTE-C1 §1); neste cliente 0x3C é AutoAttackStop. |
| 0x3D | SocialActionPacket | 0x1001DB60 | `d d` | 100 `OnSocialAction` | = (SocialAction) | campos: d:id d:ActionType |
| 0x3E | ChangeMoveTypePacket | 0x1001DCA0 | `d d` | 101 `OnChangeMoveType` | = (ChangeMoveType) | campos: d:id d:MoveType |
| 0x3F | ChangeWaitTypePacket | 0x1001DDE0 | `d×5` | 102 `OnChangeWaitType` | = (ChangeWaitType) | campos: d:id d:WaitType d:x d:y d:z |
| 0x40 | NetworkFailPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x41 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x42 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x43 | CreatePledgePacket | 0x1000B4F0 | `d` | 150 `OnReceiveCreatePledge` | - | campos: d ; O Mobius associa o opcode 0x43 à classe `ManagePledgePower` (`ddd`); no cliente C1 é `CreatePledgePacket` (`d`). |
| 0x44 | AskJoinPledgePacket | 0x1000B5C0 | `d S` | 152 `OnReceiveAskJoinPledge` | = (AskJoinPledge) | campos: d:requestorObjId S:pledgeName ; S#1 ≤255 chars |
| 0x45 | JoinPledgePacket | 0x1000B6B0 | `d` | 155 `OnReceiveJoinPledge` | = (JoinPledge) | campos: d:pledgeId |
| 0x46 | WithdrawalPledgePacket | 0x1000B780 | `d` | 156 `OnReceiveWidthdrawalPledge` | - | campos: d |
| 0x47 | OustPledgeMemberPacket | 0x1000B850 | `d` | 153 `OnReceiveOustPledgeMember` | - | campos: d |
| 0x48 | SetOustPledgeMemberPacket | 0x10001E40 | `(sem leitura)` | 157 `OnReceiveOustPledgeMessage` | - |  |
| 0x49 | DismissPledgePacket | 0x1000B920 | `d` | 154 `OnReceiveDismissPledge` | - | campos: d |
| 0x4A | SetDismissPledgePacket | 0x10001EE0 | `(sem leitura)` | 157 `OnReceiveOustPledgeMessage` | - | Sem corpo; mesmo evento 157 do 0x48 (não há "OnReceiveDismissPledgeMessage"). |
| 0x4B | AskJoinPartyPacket | 0x1000C000 | `d d` | 179 `OnReceiveAskJoinParty` | = (AskJoinParty) | campos: d:requestorObjectId d:Id |
| 0x4C | JoinPartyPacket | 0x1000C0E0 | `d` | 180 `OnReceiveJoinParty` | - | campos: d |
| 0x4D | WithdrawalPartyPacket | 0x1000C1B0 | `d` | 181 `OnReceiveWithdrawalParty` | - | campos: d |
| 0x4E | OustPartyMemberPacket | 0x1000C280 | `d` | 182 `OnReceiveOustPartyMember` | - | campos: d |
| 0x4F | SetOustPartyMemberPacket | 0x10001F80 | `(sem leitura)` | 183 `OnReceiveOustPartyMessage` | - |  |
| 0x50 | DismissPartyPacket | 0x1000C350 | `d` | 184 `OnReceiveDismissParty` | - | campos: d |
| 0x51 | SetDismissPartyPacket | 0x10002020 | `(sem leitura)` | 185 `OnRecevieDismissPartyMessage` | - |  |
| 0x52 | MagicAndSkillList | 0x1000C420 | `d ‖ N×( d d )` | 192 `OnReceiveMagicSkillListPacket` | - |  |
| 0x53 | WareHouseDepositListPacket | 0x1000C540 | `d h ‖ N×( h  d d d h h d h h h d )` | 84 `OnStartDepositList`; 85 `OnAddDepositList` | =p (WareHouseDepositList) |  |
| 0x54 | WareHouseWithdrawListPacket | 0x1000C690 | `d h ‖ N×( h  d d d h h d h h h d )` | 86 `OnStartWithdrawList`; 87 `OnAddWithdrawList` | =p (WareHouseWithdrawalList) |  |
| 0x55 | WareHouseDonePacket | 0x1000C7E0 | `d` | 186 `OnReceiveWareHouseDonePacket` | - | campos: d |
| 0x56 | ShortCutRegisterPacket | 0x1000C8B0 | `d d ; depois, conforme o 1º d (tipo): 1 -> d ; 2 -> d d ; 3 -> d ; outro -> nada ; e sempre d` | 188 `AddShortCutItem` | ~ (ShortcutRegister) | Registro de atalho; pode ser tipo 1 (item), 2 (skill: id, nível), 3 (ação); sempre termina em `d`. |
| 0x57 | ShortCutInitPacket | 0x1000CA60 | `d ‖ N×( d d ; conforme o tipo (1: d / 2: d d / 3: d / outro: nada) ; d )   (só com versão >336)` | 187 `OnReceiveShortCutList`; 188 `AddShortCutItem` | =p (ShortcutInit) | Com versão <= 336 o handler ignora o pacote. Lista de atalhos: ver layout. |
| 0x58 | ShortCutDeletePacket | 0x1000CC90 | `d` | 191 `OnReceiveShortCutDelete` | - | campos: d |
| 0x59 | StopMovePacket | 0x1001DF60 | `d×5` | 103 `OnStopMove` | = (StopMove) | campos: d:ID d:OriginX d:OriginY d:OriginZ d:OriginYaw |
| 0x5A | MagicSkillUse | 0x1001F1B0 | `d×9 h ‖ h×N   (variante >336)` | 270 `OnReceiveMagicSkillUse`; 271 `OnReceiveMagicSkillUse` | ~ (MagicSkillUse) | Variante >336 (§3.7): 9 d, `h` N e N×`h` (INFERIDO: lista de efeitos). Com versão <= 336 só 6 d. |
| 0x5B | MagicSkillCanceled | 0x1001F6B0 | `d` | 272 `OnReceiveMagicSkillCanceled` | = (MagicSkillCanceled) | campos: d:objectId |
| 0x5C | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x5D | SayPacket2 | 0x1000CD90 | `d d S S` | 193 `OnSay2` | ~ (CreatureSay) | campos: d:CreatureID d:Type S:Name S:Msg ; S#2 ≤255 chars, S#3 ≤511 chars ; Say2: objectId, tipo, nome S, texto S. |
| 0x5E | EquipUpdatePacket | 0x1000CEE0 | `d×3` | 200 `OnUpdateEquipItem` | ~ (EquipUpdate) | campos: d:change d:ObjectId d:x01 |
| 0x5F | StopMoveWithLocationPacket | 0x1001E100 | `d×4` | 105 `OnStopMoveWithLocation` | - | campos: d:ID d:X d:Y d:Z |
| 0x60 | DoorInfoPacket | 0x1001E280 | `d×5` | 106 `OnDoorInfo` | = (DoorInfo) | campos: d:ID d:X d:Y d:Z d:isWall |
| 0x61 | DoorStatusUpdatePacket | 0x1001E550 | `d×5` | 107 `OnDoorStatusUpdate` | = (DoorStatusUpdate) | campos: d:ID d:bClose d:DamageGrade d:bEnemy d:isWall ; O 5º `d` não aparece em nenhum uso (análise de uso: INFERIDO). |
| 0x62 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x63 | PartySmallWindowAllPacket | 0x1001F7D0 | `d ‖ N×( d S[x] d×8 )` | 204 `OnPartyAddMemberPacket` | =p (PartySmallWindowAll) | S#1 ≤255 chars |
| 0x64 | PartySmallWindowAddPacket | 0x1001FA30 | `d S d×8` | 204 `OnPartyAddMemberPacket` | = (PartySmallWindowAdd) | campos: d:ObjectId S:Name d:CurrentHp d:MaxHp d:CurrentMp d:MaxMp d:PlayerClass d:Level d:0 d:0 ; S#1 ≤255 chars |
| 0x65 | PartySmallWindowDeleteAllPacket | 0x100134D0 | `(sem leitura)` | 207 `OnPartyDeleteAllPacket` | = (PartySmallWindowDeleteAll) |  |
| 0x66 | PartySmallWindowDeletePacket | 0x10013570 | `d S` | 206 `OnPartyDeleteMemberPacket` | = (PartySmallWindowDelete) | campos: d:ObjectId S:Name ; S#1 ≤255 chars |
| 0x67 | PartySmallWindowUpdatePacket | 0x1000D350 | `d S d×6` | 205 `OnPartyUpdateMemberPacket` | = (PartySmallWindowUpdate) | campos: d:ObjectId S:Name d:CurrentHp d:MaxHp d:CurrentMp d:MaxMp d:PlayerClass d:Level ; S#1 ≤255 chars |
| 0x68 | PledgeShowMemberListAllPacket | 0x100131D0 | `d S S ; d×9 S d ; d ‖ N×( S d×5 )` | 143 `OnReceivePledgeInfo`; 145 `OnReceivePledgeMemberList`; 146 `OnAddPledgeMember`; 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via) | ~ (PledgeShowMemberListAll) | S#1 ≤31 chars, S#2 ≤31 chars, S#9 ≤31 chars, S#0 ≤255 chars ; Lista do clã: cabeçalho `dSS`, `d×9 S d`, N e N membros; o handler tem saídas antecipadas depois do cabeçalho (NÃO VERIFICADO a condição). Brasões via `0x100024E0` (eventos 151/166 `via`). |
| 0x69 | PledgeShowMemberListUpdatePacket | 0x1000CFD0 | `S d×5` | 147 `OnUpdatePledgeMember` | ~ (PledgeShowMemberListUpdate) | campos: S:name d:level d:classId d:female d:race d:objectId ; S#0 ≤255 chars |
| 0x6A | PledgeShowMemberListAddPacket | 0x1000D0D0 | `S d×5` | 146 `OnAddPledgeMember` | M+1 (PledgeShowMemberListAdd) | campos: S:name d:level d:classId d:0 d:1 d:isOnline ; S#0 ≤255 chars |
| 0x6B | PledgeShowMemberListDeletePacket | 0x1000D1D0 | `S` | 148 `OnDeletePledgeMember` | = (PledgeShowMemberListDelete) | campos: S:player ; S#0 ≤255 chars |
| 0x6C | MagicListPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x6D | SkillListPacket | 0x1000D4A0 | `d ‖ N×( d d d )` | 209 `OnShowSkillWnd`; 210 `OnSkillListPacket` | =p (SkillList) |  |
| 0x6E | VehicleInfoPacket | 0x1001E720 | `d×5` | 108 `OnVehicleInfo`; 111 `OnVehicleCheckLocation` | = (VehicleInfo) | campos: d:ID d:x d:y d:z d:yaw |
| 0x6F | VehicleDeparturePacket | 0x1001EA90 | `d×6` | 110 `OnVehicleDeparture` | = (VehicleDeparture) | campos: d:ID d:MoveSpeed d:RotationSpeed d:x d:y d:z |
| 0x70 | VehicleCheckLocationPacket | 0x1000CD00 | `d×4` | - | M+1 (VehicleCheckLocation) | campos: d:ID d:x d:y d:z ; O handler lê `dddd` e só loga (sem efeito de jogo). |
| 0x71 | GetOnVehiclePacket | 0x1001ECE0 | `d×5` | 112 `OnGetOnVehicle` | = (GetOnVehicle) | campos: d:UserID d:VehicleID d:X d:Y d:Z |
| 0x72 | GetOffVehiclePacket | 0x1001EF90 | `d×5` | 113 `OnGetOffVehicle` | = (GetOffVehicle) | campos: d:UserID d:VehicleID d:x d:y d:z |
| 0x73 | TradeRequestPacket | 0x1000D600 | `d` | 212 `OnReceiveTradeDemand` | = (SendTradeRequest) | campos: d:ID |
| 0x74 | RestartResponsePacket | 0x10013670 | `d S` | 213 `OnRestartResponse` | C+1 (RestartResponse) | campos: d:result S ; S#1 ≤1023 chars ; RestartResponse: `d` resultado + `S` mensagem; o Mobius manda só o `d` (a `S` lê lixo do heap). |
| 0x75 | MoveToPawnPacket | 0x10015050 | `d×6` | 4 `OnMoveToPawn`; 269 `OnMoveToActor` | M+3 (MoveToPawn) | campos: d:ID d:TargetID d:Dist d:OriginX d:OriginY d:OriginZ |
| 0x76 | ValidateLocationPacket | 0x10015380 | `d×5` | 5 `OnValidateLocation` | = (ValidateLocation) | campos: d:objectId d:X d:Y d:Z d:Heading |
| 0x77 | StartRotatingPacket | 0x100154B0 | `d×3` | 9 `OnStartRotating` | M+1 (StartRotation) | campos: d:ID d:CurYaw d:Dir ; objectId, guinada atual, sentido (Dir). O Mobius manda 1 d a mais. |
| 0x78 | FinishRotatingPacket | 0x10015600 | `d d` | 10 `OnFinishRotating` | M+2 (StopRotation) | campos: d:ID d:CurYaw ; objectId, guinada atual. O Mobius manda `dddc`. |
| 0x79 | MoveBackwardToLocationPacket | 0x10014E70 | `d×7` | 3 `OnMoveBackwardToLocation` | - | campos: d:ID d:X d:Y d:Z d:OriginX d:OriginY d:OriginZ |
| 0x7A | SystemMessagePacket | 0x1000D6D0 | `d d ‖ N×( d tipo ; tipo 0: S ; 1,2,3,5: d ; 4: d d ; >5: nada )` | 214 `OnAddSystemMessageParam`; 215 `OnEndSystemMessageParam` | ~ (SystemMessage) | S#0 ≤1023 chars ; SystemMessage: id da mensagem, nº de parâmetros, cada parâmetro tipado (tabela de saltos em 0x1000D8D4). |
| 0x7B | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x7C | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x7D | StartPledgeWarPacket | 0x1000B9F0 | `S S` | 159 `OnReceiveStartPledgeWar` | = (StartPledgeWar) | campos: S:playerName S:pledgeName ; S#0 ≤255 chars, S#1 ≤255 chars |
| 0x7E | ReplyStartPledgeWarPacket | 0x1000BAF0 | `d` | 160 `OnReceiveReplyStartPledgeWar` | - | campos: d |
| 0x7F | StopPledgeWarPacket | 0x1000BBC0 | `S S` | 161 `OnReceiveStopPledgeWar` | = (StopPledgeWar) | campos: S:pledgeName S:playerName ; S#0 ≤255 chars, S#1 ≤255 chars |
| 0x80 | ReplyStopPledgeWarPacket | 0x1000BCC0 | `d` | 162 `OnReceiveReplyStopPledgeWar` | - | campos: d |
| 0x81 | SurrenderPledgeWarPacket | 0x1000BD90 | `d` | 163 `OnReceiveSurrenderPledgeWar` | - | campos: d ; O Mobius associa o 0x81 a `SurrenderPledgeWar` com `SS` (2 strings); o cliente lê um `d` (NÃO VERIFICADO o efeito). |
| 0x82 | ReplySurrenderPledgeWarPacket | 0x1000BE60 | `d` | 164 `OnReceiveReplySurrenderPledgeWar` | - | campos: d |
| 0x83 | SetPledgeCrestPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x84 | PledgeCrestPacket | 0x10013090 | `d d` | 151 `OnRegisterPledgeCrest` | ~ (PledgeCrest) | campos: d:crestId d:length ; Lê `dd` (crestId, tamanho); os bytes do brasão (INFERIDO) são copiados fora do leitor e gravados em `..\PledgeCrest\%d_%d.bmp` (strings do handler). Evento 151 `OnRegisterPledgeCrest`. |
| 0x85 | SetupGaugePacket | 0x1000D8F0 | `d d` | 216 `OnSetupGaugePacket` | M+1 (SetupGauge) | campos: d:dat1 d:time |
| 0x86 | ShowBoardPacket | 0x1000D9D0 | `S×7` | 217 `OnShowboardPacket` | = (ShowBoard) | campos: S:bypass S:bypass S:bypass S:bypass S:bypass S:bypass S:content ; S#0 ≤127 chars, S#1 ≤127 chars, S#2 ≤127 chars, S#3 ≤127 chars, S#4 ≤127 chars, S#5 ≤127 chars, S#6 ≤8191 chars |
| 0x87 | ChooseInventoryItemPacket | 0x100020E0 | `(sem leitura)` | 218 `OnSelectItemToEnchant` | M+1 (ChooseInventoryItem) | Sem corpo: `OnSelectItemToEnchant`. |
| 0x88 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0x89 | MoveToLocationInVehiclePacket | 0x1001FC60 | `d×8` | 2 `OnMoveToLocationInVehicle` | = (MoveToLocationInVehicle) | campos: d:objectId d:boatId d:X d:Y d:Z d:X d:Y d:Z |
| 0x8A | StopMoveInVehiclePacket | 0x1001FEB0 | `d×6` | 104 `OnStopMoveInVehicle` | = (StopMoveInVehicle) | campos: d:objectId d:boatId d:X d:Y d:Z d:heading |
| 0x8B | ValidateLocationInVehiclePacket | 0x100200C0 | `d×6` | 6 `OnValidateLocationInVehicle` | = (ValidateLocationInVehicle) | campos: d:ID d:boatObjId d:OriginX d:OriginY d:OriginZ d:OriginYaw |
| 0x8C | TradeUpdatePacket | 0x1000ADA0 | `h ‖ N×( h h  d d d h h d h h h )` | 133 `OnReceiveTradeUpdate` | =p (TradeUpdate) |  |
| 0x8D | TradePressOwnOkPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x8E | MagicSkillLaunchedPacket | 0x10020310 | `d d d d ‖ N×( d )` | 273 `OnReceiveMagicSkillLaunched`; 274 `OnReceiveMagicSkillLaunched` | =p (MagicSkillLaunched) |  |
| 0x8F | FriendAddRequestResult | 0x1000EBA0 | `d` | 172 `OnReceiveFriendAddRequestResult` | - | campos: d |
| 0x90 | FriendAdd | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x91 | FriendRemove | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x92 | FriendList | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x93 | FriendStatus | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x94 | TradePressOtherOkPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x95 | FriendAddRequest | 0x1000EC70 | `S` | 173 `OnReceiveFriendAddRequest` | M+1 (FriendAddRequest) | campos: S:requestorName ; S#0 ≤127 chars |
| 0x96 | LogOutOkPacket | 0x10001D20 | `(sem leitura)` | 119 `OnAcceptLogOut` | = (LeaveWorld) |  |
| 0x97 | AbnormalStatusUpdatePacket | 0x1000AEF0 | `h ‖ N×( d h d )` | 195 `OnReceiveAbnormalStatus`; 196 `AddAbnormalStatus` | =p (AbnormalStatusUpdate) |  |
| 0x98 | QuestListPacket | 0x1000B050 | `h ‖ N×( d d ) ; h ‖ M×( d d d d )` | 197 `OnReceiveQuestList`; 198 `AddQuestID`; 199 `AddQuestItem` | =p (QuestList) |  |
| 0x99 | EnchantResultPacket | 0x1000B1E0 | `d` | 219 `OnReceiveEnchantResult` | = (EnchantResult) | campos: d |
| 0x9A | AuthServerListPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0x9B | PledgeShowMemberListDeleteAllPacket | 0x100020C0 | `(sem leitura)` | 149 `OnDeleteAllPledgeMember` | = (PledgeShowMemberListDeleteAll) |  |
| 0x9C | PledgeInfoPacket | 0x1000D220 | `d S S` | - | = (PledgeInfo) | campos: d:Id S:Name S:AllyName ; S#1 ≤127 chars, S#2 ≤127 chars ; clanId `d`, nome do clã `S`, nome da aliança `S`: grava no cache `PledgeInfo` (0x10002640 copia as strings em +4 e +0xBC); sem evento. |
| 0x9D | PledgeExtendedInfoPacket | 0x1000D290 | `S d d S d×8 S d` | 142 `OnReceivePledgeExtendedInfo` | - | S#0 ≤31 chars, S#3 ≤31 chars, S#12 ≤31 chars ; Informação estendida do clã (14 campos). |
| 0x9E | SurrenderPersonallyPacket | 0x1000BF30 | `d` | 165 `OnReceiveSurrenderPersonally` | - | campos: d |
| 0x9F | RidePacket | 0x10018A50 | `d×3` | 20 `OnRide` | M+4 (Ride) | campos: d:ID d:bRide d:RideType |
| 0xA0 | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |
| 0xA1 | PledgeShowInfoUpdatePacket | 0x10013420 | `d×9` | 144 `OnReceivePledgeInfoUpdate`; 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via) | - | campos: d d d d d d d d d ; O cliente lê 9 d; o Mobius manda outro layout (PledgeShowInfoUpdate de Interlude). |
| 0xA2 | ClientActionPacket | 0x1000DB60 | `d` | 220 `OnReceiveAction` | - | campos: d |
| 0xA3 | AcquireSkillListPacket | 0x1000DBD0 | `d ‖ N×( d×5 )` | 221 `OnShowTrainListWnd`; 223 `AddAcquireSkillInfo` | =p (AcquireSkillList) |  |
| 0xA4 | AcquireSkillInfoPacket | 0x1000DD70 | `d d d ; d ‖ N×( d×4 )` | 222 `OnShowTrainInfoWnd`; 224 `AddAcquireSkillExtendedInfo` | =p (AcquireSkillInfo) |  |
| 0xA5 | ServerObjectInfoPacket | 0x1001C990 | `d d S d×5 f×4 d×4` | 91 `OnServerObjectInfo` | = (ServerObjectInfo) | S#2 ≤31 chars ; Campo a campo: §6.7 (formato fixo). |
| 0xA6 | GMHidePacket | 0x10009510 | `d` | 90 `OnGMHide` | - | campos: d |
| 0xA7 | AcquireSkillDonePacket | 0x10001C50 | `(sem leitura)` | 225 `OnHideTrainWnd` | = (AcquireSkillDone) |  |
| 0xA8 | GMViewCharacterInfoPacket | 0x10009980 | `d×5 S d×69 f×4 d×4 S d×3 c×3 d d` | 237 `OnGMViewCharacterInfo` | ≠ (GMViewCharacterInfo) | S#5 ≤31 chars, S#83 ≤31 chars ; O cliente lê 92 letras (cauda `S d×3 c×3 d d`, sem `h` de cubics); os campos finais diferem do `GMViewCharacterInfo` do Mobius (NÃO VERIFICADO o efeito). |
| 0xA9 | GMViewPledgeInfoPacket | 0x10009D10 | `S d S S ; d×9 S d ; d ‖ N×( S d×5 )` | 238 `OnGMViewReceivePledgeInfo`; 239 `OnGMViewReceivePledgeMemberList`; 240 `OnGMViewAddPledgeMember` | ~ (GMViewPledgeInfo) | S#0 ≤31 chars, S#2 ≤31 chars, S#3 ≤31 chars, S#9 ≤31 chars, S#0 ≤255 chars |
| 0xAA | GMViewSkillInfoPacket | 0x10009F40 | `S d ‖ N×( d d d )` | 241 `OnGMViewShowSkillWnd`; 242 `OnGMViewSkillListPacket` | =p (GMViewSkillInfo) | S#0 ≤31 chars |
| 0xAB | GMViewMagicInfoPacket | 0x1000A090 | `S d ‖ N×( d d d )` | 243 `OnGMViewShowMagicWnd`; 244 `OnGMViewMagicListPacket` | - | S#0 ≤31 chars |
| 0xAC | GMViewQuestInfoPacket | 0x1000A1D0 | `S h ‖ N×( d d ) ; h ‖ M×( d d d d )` | 245 `OnGMViewReceiveQuestList`; 246 `GMViewAddQuestID`; 247 `GMViewAddQuestItem` | ~ (GmViewQuestInfo) | S#0 ≤31 chars |
| 0xAD | GMViewItemListPacket | 0x1000A360 | `S h h ‖ N×( h  d d d h h h d h h )` | 248 `OnGMViewReceiveItemList`; 249 `GMViewAddInventoryItem` | =p (GMViewItemList) | S#0 ≤31 chars |
| 0xAE | GMViewWarehouseWithdrawListPacket | 0x1000A4C0 | `S d h ‖ N×( h  d d d h h  [ d h h h  só se 0<=1º h<=3 ]  d )` | 250 `OnGMViewStartWithdrawList`; 251 `OnGMViewAddWithdrawList` | =p (GMViewWarehouseWithdrawList) | S#0 ≤31 chars |
| 0xAF | PartyMatchListPacket | 0x10009580 | `d ‖ N×( d S[x] d×8 )` | 201 `ReceivePartyMatchList`; 202 `AddPartyMatchMember` | =p (PartyMatchList) | S#1 ≤31 chars |
| 0xB0 | PartyMatchDetailPacket | 0x10009740 | `d×4 S` | 203 `OnReceivePartyMatchDetail` | = (PartyMatchDetail) | campos: d:ObjectId d:isPartyMatchingShowLevel d:isPartyMatchingShowClass d:isInParty S:PartyMatchingMemo ; S#4 ≤511 chars |
| 0xB1 | PlaySoundPacket | 0x10009860 | `d S d×5` | 265 `OnPlaySound` | M+1 (PlaySound) | campos: d:unknown1 S:soundFile d:unknown3 d:unknown4 d:unknown5 d:unknown6 d:unknown7 ; S#1 ≤255 chars |
| 0xB2 | StaticObjectPacket | 0x10020770 | `d d` | 267 `OnStaticObjectInfo` | = (StaticObjectInfo) | campos: d:staticObjectId d:objectId |
| 0xB3 | PrivateStoreManageList | 0x1000DFD0 | `d d d ‖ N×( d d d d h h h d d ) ; d ‖ M×( d d d d h h h d d d )` | 255 `OnStartPrivateStoreManageList`; 257 `OnPrivateStoreManageList` | =p (PrivateStoreManageListSell) |  |
| 0xB4 | PrivateStoreList | 0x1000E240 | `d d d ‖ N×( d d d d h h h d d d )` | 256 `OnStartPrivateStoreList`; 258 `OnPrivateStoreList` | ~ (PrivateStoreListSell) |  |
| 0xB5 | PrivateStoreMsg | 0x100209F0 | `d S` | - | = (PrivateStoreMsgSell) | campos: d:objectId S:MSG ; S#1 ≤511 chars ; Mesmo handler do 0xD2 (loja de venda e de compra gravam a mesma mensagem no User): objectId, S texto; sem evento. |
| 0xB6 | ShowMinimapPacket | 0x1000DF60 | `d` | 254 `OnShowMinimap` | = (ShowMiniMap) | campos: d:mapId ; ShowMinimap: o C1 manda só `d`. |
| 0xB7 | ReviveRequestPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xB8 | AbnormalVisualEffectPacket | 0x1001CEF0 | `d×3` | - | - | campos: d d d ; AbnormalVisualEffect: objectId, máscara, segundos; grava direto no pawn; sem evento. |
| 0xB9 | TutorialShowHtmlPacket | 0x10008310 | `S` | 73 `OnTutorialShowHtml` | = (TutorialShowHtml) | campos: S:Html ; S#0 ≤8191 chars |
| 0xBA | TutorialShowQuestionMarkPacket | 0x10008390 | `d` | 74 `OnTutorialShowQuestionMark` | = (TutorialShowQuestionMark) | campos: d:markId |
| 0xBB | TutorialEnableClientEventPacket | 0x10008400 | `d` | 75 `OnTutorialEnableClientEvent` | = (TutorialEnableClientEvent) | campos: d:eventId |
| 0xBC | TutorialCloseHtmlPacket | 0x10001C30 | `(sem leitura)` | 76 `OnTutorialCloseHtml` | = (TutorialCloseHtml) |  |
| 0xBD | ShowRadarPacket | 0x10008470 | `d×3` | 78 `OnShowRadar` | - | campos: d d d ; ShowRadar: `ddd` = x, y, z (INFERIDO pelos nomes de `OnShowRadar`). |
| 0xBE | DeleteRadarPacket | 0x100084F0 | `d×3` | 79 `OnDeleteRadar` | - | campos: d d d ; DeleteRadar: `ddd` = x, y, z (INFERIDO); o servidor Lucera só manda um `d` (§7.8). |
| 0xBF | MyTargetSelectedPacket | 0x1001D540 | `d h` | 96 `OnMyTargetSelected` | = (MyTargetSelected) | campos: d:objectId h:color |
| 0xC0 | PartyMemberPositionPacket | 0x10011A50 | `d ‖ N×( d [ d d d  só se o objectId está no mapa de membros do grupo ] )` | - | =p (PartyMemberPosition) | Atenção: lê `ddd` só se o objectId está no mapa de membros do grupo (0x10011B08); para um id desconhecido os 12 bytes ficam sem ler e o próximo objectId sai errado (VERIFICADO no assembly 0x10011AC0..0x10011B0B). |
| 0xC1 | AskJoinAlliancePacket | 0x1000E730 | `d S×3` | 167 `OnReceiveAskJoinAlly` | = (AskJoinAlly) | campos: d:requestorId S:requestorName S:? S:requestorAllyName ; S#1 ≤255 chars, S#2 ≤255 chars, S#3 ≤255 chars |
| 0xC2 | JoinAlliancePacket | 0x1000E860 | `d` | 170 `OnReceiveJoinAlly` | - | campos: d |
| 0xC3 | WithdrawAlliancePacket | 0x1000E930 | `d` | 171 `OnReceiveWithdrawAlly` | - | campos: d |
| 0xC4 | OustAllianceMemberPledgePacket | 0x1000EA00 | `d` | 168 `OnReceiveOustAlly` | - | campos: d |
| 0xC5 | DismissAlliancePacket | 0x1000EAD0 | `d` | 169 `OnReceiveDismissAlly` | - | campos: d |
| 0xC6 | SetAllianceCrestPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xC7 | AllianceCrestPacket | 0x100137C0 | `d d` | 166 `OnRegisterAllyCrest` | ~ (AllyCrest) | campos: d:crestId d:length |
| 0xC8 | ServerCloseSocketPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xC9 | PetStatusShowPacket | 0x10008570 | `d` | 229 `OnShowPetStatusWnd` | = (PetStatusShow) | campos: d:summonType |
| 0xCA | PetInfoPacket | 0x1001A420 | `d×19 f×4 d×3 c×5 S S d×26` | 92 `OnNpcInfo`; 230 `OnShowPetPartyWnd`; 232 `OnShowPetNameBtn` | ~ (PetInfo) | S#31 ≤31 chars, S#32 ≤31 chars ; PetInfo; campo a campo parecido com NpcInfo: §6.4. |
| 0xCB | PetItemListPacket | 0x100085B0 | `h ‖ N×( h ; d×3 h×3 d h h )` | 234 `OnReceivePetItemList`; 235 `AddPetInventoryItem` | =p (PetItemList) |  |
| 0xCC | PetInventoryUpdatePacket | 0x10008700 | `h ‖ N×( h h ; d×3 h×3 d h h )` | 190 `OnUpdatePetShortCutItem`; 236 `UpdatePetInventoryItem` | =p (PetInventoryUpdate) |  |
| 0xCD | AllianceInfoPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xCE | PetStatusUpdatePacket | 0x100088A0 | `d×5 S d×10` | 233 `OnReceivePetStatusInfo` | = (PetStatusUpdate) | S#5 ≤31 chars |
| 0xCF | PetDeletePacket | 0x10008B20 | `d d` | 231 `OnHidePetWnd` | = (PetDelete) | campos: d:petType d:petObjId |
| 0xD0 | PrivateStoreBuyManageList | 0x1000E380 | `d d d ‖ N×( d h d d h d h ) ; d ‖ M×( d h d d h d h d d )` | 260 `OnStartPrivateStoreBuyManageList`; 262 `OnPrivateStoreBuyManageList` | =p (PrivateStoreManageListBuy) |  |
| 0xD1 | PrivateStoreBuyList | 0x1000E5F0 | `d d d ‖ N×( d d h d d h d h d d )` | 261 `OnStartPrivateStoreBuyList`; 263 `OnPrivateStoreBuyList` | =p (PrivateStoreListBuy) |  |
| 0xD2 | PrivateStoreBuyMsg | 0x100209F0 | `d S` | - | = (PrivateStoreMsgBuy) | campos: d:objectId S:MSG ; S#1 ≤511 chars ; Mesmo handler do 0xB5. |
| 0xD3 | VehicleStartPacket | 0x1000ED50 | `d` | 276 `OnVehicleStartPacket` | M+1 (VehicleStarted) | campos: d:objectId |
| 0xD4 | RequestTimeCheckPacket | 0x1000EE00 | `d` | 277 `OnRequestTimeCheckPacket` | - | campos: d |
| 0xD5 | StartAllianceWarPacket | 0x1000EFD0 | `S S` | 280 `OnStartAllianceWar` | - | campos: S S ; S#0 ≤127 chars, S#1 ≤127 chars |
| 0xD6 | ReplyStartAllianceWarPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xD7 | StopAllianceWarPacket | 0x1000F0A0 | `S S` | 281 `OnStopAllianceWar` | - | campos: S S ; S#0 ≤127 chars, S#1 ≤127 chars |
| 0xD8 | ReplyStopAllianceWarPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xD9 | SurrenderAllianceWarPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xDA | SkillCoolTimePacket | 0x1000EEB0 | `d ‖ N×( d d d d )` | 279 `OnSkillCoolTimePacket` | =p (SkillCoolTime) |  |
| 0xDB | PackageToListPacket | 0x1000F170 | `d ‖ N×( d S )` | 252 `AddPackageToList` | =p (PackageToList) | S#1 ≤127 chars |
| 0xDC | PackageSendableListPacket | 0x1000F290 | `d d d ‖ N×( h  d d d h h d h h h d )` | 253 `AddSendableItemList` | =p (PackageSendableList) |  |
| 0xDD | EarthQuakePacket | 0x1000F3D0 | `d×6` | 8 `OnEarthQuake` | = (Earthquake) | campos: d:OriginX d:OriginY d:OriginZ d:Intensity d:Time d:0 |
| 0xDE | FlyToLoactionPacket | 0x10020BE0 | `d×7` | 7 `OnFlyToLocation` | - | campos: d:ID d:X d:Y d:Z d:OriginX d:OriginY d:OriginZ |
| 0xDF | BlockListPacket | 0x10001BE0 | - | - | - | **ignorado**: handler `xor al,al; ret`, nada é lido |
| 0xE0 | SpecialCameraPacket | 0x10021170 | `d×6` | 114 `OnSpecialCamera` | M+5 (SpecialCamera) | campos: d:ID d:Dist d:Yaw d:Pitch d:Time d:Duration |
| 0xE1 | NormalCameraPacket | 0x10002170 | `(sem leitura)` | 115 `OnNormalCamera` | = (NormalCamera) |  |
| 0xE2 | CastleSiegeInfoPacket | 0x1000F4C0 | `d d d S S d S d d ; d(N) ‖ N×( d )` | 282 `OnReceiveSiegeInfo`; 285 `OnAddSelectableTime` | ~ (SiegeInfo) | S#3 ≤127 chars, S#4 ≤127 chars, S#6 ≤127 chars |
| 0xE3 | CastleSiegeAttackerListPacket | 0x10013900 | `d×6 ‖ N×( d S S d d d S S d )` | 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via); 283 `OnReceiveSiegeAttackerList`; 286 `OnEndSiegeAttackerlist`; 288 `ClearAttackerList` | ~ (SiegeAttackerList) | S#1 ≤127 chars, S#2 ≤127 chars, S#6 ≤127 chars, S#7 ≤127 chars |
| 0xE4 | CastleSiegeDefenderListPacket | 0x10013B10 | `d×6 ‖ N×( d S S d d d d S S d )` | 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via); 284 `OnReceiveSiegeDefenderList`; 287 `OnEndSiegeDefenderlist`; 289 `ClearDefenderList` | ~ (SiegeDefenderList) | S#1 ≤127 chars, S#2 ≤127 chars, S#7 ≤127 chars, S#8 ≤127 chars |
| 0xE5 | NickNameChangedPacket | 0x10020DC0 | `d S` | - | = (NicknameChanged) | campos: d:objectId S:title ; S#1 ≤127 chars ; objectId, apelido S: copia para o User (appStrcpy); sem evento. |
| 0xE6 | PledgeStatusChangedPacket | 0x10020ED0 | `d×5` | 151 `OnRegisterPledgeCrest` (via); 166 `OnRegisterAllyCrest` (via) | = (PledgeStatusChanged) | campos: d:LeaderId d:Id d:0 d:Level d:0 ; Atualiza o cache `PledgeInfo` (0x10012640 grava +0, +0xA0, +0xB8, +0xFC) e baixa brasões; sem evento próprio. |
| 0xE7 | RelationChangedPacket | 0x10021000 | `d×5` | 226 `EnablePartyChat` | = (RelationChanged) | campos: d:objectId d:relation d:autoAttackable d:karma d:pvpFlag |
| 0xE8 | EventTriggerPacket | 0x1000F6B0 | `d c` | 116 `OnEventTrigger` | = (OnEventTrigger) | campos: d:ID c:enabled |
| 0xE9 | MultiSellListPacket | 0x1000F740 | `d d ‖ N×( d×6 ; M×( d d d ) )` | 290 `OnSetVIPShopID`; 291 `OnAddVIPItemList`; 292 `OnAddVIPNeedItemList`; 293 `OnVIPItemListEnd` | =p (MultiSellList) |  |
| 0xEA | SetSummonRemainTimePacket | 0x1000F8A0 | `d d` | 294 `OnSetSummonRemainTime` | = (SetSummonRemainTime) | campos: d:maxTime d:remainingTime |
| 0xEB | SkillRemainSecPacket | 0x10021340 | `d×7` | 117 `OnSkillRemainSec` | - | campos: d d d d d d d |
| 0xEC | NetPingPacket | 0x1000F920 | `d` | - | = (NetPing) | campos: d:time ; NetPing: o handler lê `d` e chama `UNetworkHandler::NetPing(d)` (slot 223 **do vtable do `UNetworkHandler`** em `0x10033798`, `+0x37C` = `0x100071F0`; não é um slot do `FL2NetNotify` [corrigido pela revisão]), que responde `cddd` com opcode 0xA8 (§3.8). |
| 0xED | DummyPacket | 0x10002130 | - | - | - | **Dummy**: só grava a linha "(Receive)Dummy" no log |

## 6. Campo a campo dos pacotes grandes

Posição = índice da letra no formato lido (a partir de 0), `*` marca letra dentro de laço. Nomes: log do cliente, senão a expressão do Mobius C1 (**dica**; `?` = o alinhamento por letra não achou par). Para os cinco handlers dependentes de versão só a variante `> 336`.

### 6.1 `UserInfo` (0x04), variante > 336

```
UserInfoPacket (opcode 0x04, handler 0x100165D0; nomes: log do cliente > expressão do servidor Mobius C1 `UserInfo`)
  0 d  X
  1 d  Y
  2 d  Z
  3 d  Vehicle
  4 d  ObjectId
  5 S  VisibleName
  6 d  Race
  7 d  isFemale
  8 d  BaseClass
  9 d  Level
 10 d  Exp
 11 d  STR
 12 d  DEX
 13 d  CON
 14 d  INT
 15 d  WIT
 16 d  MEN
 17 d  MaxHp
 18 d  CurrentHp
 19 d  MaxMp
 20 d  CurrentMp
 21 d  Sp
 22 d  CurrentLoad
 23 d  MaxLoad
 24 d  ActiveWeaponItem
 25 d  paperdoll_objId.under
 26 d  paperdoll_objId.rear
 27 d  paperdoll_objId.lear
 28 d  paperdoll_objId.neck
 29 d  paperdoll_objId.rfinger
 30 d  paperdoll_objId.lfinger
 31 d  paperdoll_objId.head
 32 d  paperdoll_objId.rhand
 33 d  paperdoll_objId.lhand
 34 d  paperdoll_objId.gloves
 35 d  paperdoll_objId.chest
 36 d  paperdoll_objId.legs
 37 d  paperdoll_objId.feet
 38 d  paperdoll_objId.cloak
 39 d  paperdoll_objId.rhand
 40 d  paperdoll_itemId.under
 41 d  paperdoll_itemId.rear
 42 d  paperdoll_itemId.lear
 43 d  paperdoll_itemId.neck
 44 d  paperdoll_itemId.rfinger
 45 d  paperdoll_itemId.lfinger
 46 d  paperdoll_itemId.head
 47 d  paperdoll_itemId.rhand
 48 d  paperdoll_itemId.lhand
 49 d  paperdoll_itemId.gloves
 50 d  paperdoll_itemId.chest
 51 d  paperdoll_itemId.legs
 52 d  paperdoll_itemId.feet
 53 d  paperdoll_itemId.cloak
 54 d  paperdoll_itemId.rhand
 55 d  PAtk
 56 d  PAtkSpd
 57 d  PDef
 58 d  EvasionRate
 59 d  Accuracy
 60 d  CriticalHit
 61 d  MAtk
 62 d  MAtkSpd
 63 d  PAtkSpd
 64 d  MDef
 65 d  PvpFlag
 66 d  Karma
 67 d  runSpd
 68 d  walkSpd
 69 d  swimRunSpd
 70 d  swimWalkSpd
 71 d  flyRunSpd
 72 d  flyWalkSpd
 73 d  flyRunSpd
 74 d  flyWalkSpd
 75 f  moveMultiplier
 76 f  AttackSpeedMultiplier
 77 f  CollisionRadius
 78 f  CollisionHeight
 79 d  HairStyle
 80 d  HairColor
 81 d  Face
 82 d  isGM
 83 S  title
 84 d  ClanId
 85 d  ClanCrestId
 86 d  AllyId
 87 d  AllyCrestId
 88 d  relation
 89 c  MountType
 90 c  PrivateStoreType
 91 c  hasDwarvenCraft
 92 d  PkKills
 93 d  PvpKills
 94 h  Cubics
 95 h* cubicId
 96 c  isPartyMatchingActive
```

### 6.2 `CharInfo` (0x03), variante > 336

```
CharInfoPacket (opcode 0x03, handler 0x10015730; nomes: log do cliente > expressão do servidor Mobius C1 `CharInfo`)
  0 d  x
  1 d  y
  2 d  z
  3 d  vehicleId
  4 d  objId
  5 S  VisibleName
  6 d  Race
  7 d  isFemale
  8 d  BaseClass
  9 d  0
 10 d  paperdoll_itemId.head
 11 d  paperdoll_itemId.rhand
 12 d  paperdoll_itemId.lhand
 13 d  paperdoll_itemId.gloves
 14 d  paperdoll_itemId.chest
 15 d  paperdoll_itemId.legs
 16 d  paperdoll_itemId.feet
 17 d  paperdoll_itemId.cloak
 18 d  paperdoll_itemId.rhand
 19 d  paperdoll_itemId.under
 20 d  0
 21 d  mAtkSpd
 22 d  pAtkSpd
 23 d  PvpFlag
 24 d  Karma
 25 d  runSpd
 26 d  walkSpd
 27 d  swimRunSpd
 28 d  swimWalkSpd
 29 d  flyRunSpd
 30 d  flyWalkSpd
 31 d  flyRunSpd
 32 d  flyWalkSpd
 33 f  moveMultiplier
 34 f  AttackSpeedMultiplier
 35 f  CollisionRadius
 36 f  CollisionHeight
 37 d  HairStyle
 38 d  HairColor
 39 d  Face
 40 S  VisibleTitle
 41 d  ClanId
 42 d  ClanCrestId
 43 d  AllyId
 44 d  AllyCrestId
 45 d  0
 46 c  isSitting
 47 c  isRunning
 48 c  isInCombat
 49 c  isInOlympiadMode
 50 c  isInvisible
 51 c  MountType
 52 c  PrivateStoreType
 53 h  Cubics
 54 h* cubicId
 55 c  isPartyMatchingActive
```

### 6.3 `NpcInfo` (0x22), variante > 336

```
NpcInfoPacket (opcode 0x22, handler 0x1001BD60; nomes: log do cliente > expressão do servidor Mobius C1 `AbstractNpcInfo`)
  0 d  ObjectId
  1 d  displayId
  2 d  isAttackable
  3 d  x
  4 d  y
  5 d  z
  6 d  heading
  7 d  0
  8 d  mAtkSpd
  9 d  pAtkSpd
 10 d  runSpd
 11 d  walkSpd
 12 d  swimRunSpd
 13 d  swimWalkSpd
 14 d  flyRunSpd
 15 d  flyWalkSpd
 16 d  flyRunSpd
 17 d  flyWalkSpd
 18 f  moveMultiplier
 19 f  AttackSpeedMultiplier
 20 f  collisionRadius
 21 f  collisionHeight
 22 d  rhand
 23 d  chest
 24 d  lhand
 25 c  1
 26 c  isRunning
 27 c  isInCombat
 28 c  isAlikeDead
 29 c  isSummoned
 30 S  name
 31 S  title
 32 d  0
 33 d  0
 34 d  0
```

### 6.4 `PetInfo` (0xCA)

```
PetInfoPacket (opcode 0xCA, handler 0x1001A420; nomes: log do cliente > expressão do servidor Mobius C1 `PetInfo`)
  0 d  SummonType
  1 d  ID
  2 d  ClassID
  3 d  CanBeAttacked
  4 d  X
  5 d  Y
  6 d  Z
  7 d  heading
  8 d  0
  9 d  mAtkSpd
 10 d  pAtkSpd
 11 d  runSpd
 12 d  walkSpd
 13 d  swimRunSpd
 14 d  swimWalkSpd
 15 d  flyRunSpd
 16 d  flyWalkSpd
 17 d  flyRunSpd
 18 d  flyWalkSpd
 19 f  moveMultiplier
 20 f  AttackSpeedMultiplier
 21 f  Template
 22 f  Template
 23 d  Weapon
 24 d  Armor
 25 d  0
 26 c  Owner
 27 c  isRunning
 28 c  isInCombat
 29 c  isAlikeDead
 30 c  isSummoned
 31 S  Name
 32 S  Template
 33 d  1
 34 d  PvpFlag
 35 d  Karma
 36 d  curFed
 37 d  maxFed
 38 d  CurrentHp
 39 d  maxHp
 40 d  CurrentMp
 41 d  maxMp
 42 d  Stat
 43 d  Level
 44 d  Stat
 45 d  Stat
 46 d  ExpForThisLevel
 47 d  ExpForNextLevel
 48 d  isPet
 49 d  MaxLoad
 50 d  PAtk
 51 d  PDef
 52 d  MAtk
 53 d  MDef
 54 d  Accuracy
 55 d  EvasionRate
 56 d  CriticalHit
 57 d  MoveSpeed
 58 d  PAtkSpd
```

### 6.5 `CharacterSelectionInfo` (0x1F): `d` n e n registros (a posição abaixo é a do 1º registro; os `*` repetem)

```
CharacterSelectionInfoPacket (opcode 0x1F, handler 0x10008EE0; nomes: log do cliente > expressão do servidor Mobius C1 `CharSelectionInfo`)
  0 d  size
  1 S* Name
  2 d* ObjectId
  3 S* loginName
  4 d* sessionId
  5 d* ClanId
  6 d* 0
  7 d* Sex
  8 d* Race
  9 d* BaseClassId
 10 d* 1
 11 d* X
 12 d* Y
 13 d* Z
 14 f* CurrentHp
 15 f* CurrentMp
 16 d* Sp
 17 d* Exp
 18 d* Level
 19 d* Karma
 20 d* 0
 21 d* 0
 22 d* 0
 23 d* 0
 24 d* 0
 25 d* 0
 26 d* 0
 27 d* 0
 28 d* 0
 29 d* paperdoll_objId.under
 30 d* paperdoll_objId.rear
 31 d* paperdoll_objId.lear
 32 d* paperdoll_objId.neck
 33 d* paperdoll_objId.rfinger
 34 d* paperdoll_objId.lfinger
 35 d* paperdoll_objId.head
 36 d* paperdoll_objId.rhand
 37 d* paperdoll_objId.lhand
 38 d* paperdoll_objId.gloves
 39 d* paperdoll_objId.chest
 40 d* paperdoll_objId.legs
 41 d* paperdoll_objId.feet
 42 d* paperdoll_objId.cloak
 43 d* paperdoll_objId.rhand
 44 d* paperdoll_itemId.under
 45 d* paperdoll_itemId.rear
 46 d* paperdoll_itemId.lear
 47 d* paperdoll_itemId.neck
 48 d* paperdoll_itemId.rfinger
 49 d* paperdoll_itemId.lfinger
 50 d* paperdoll_itemId.head
 51 d* paperdoll_itemId.rhand
 52 d* paperdoll_itemId.lhand
 53 d* paperdoll_itemId.gloves
 54 d* paperdoll_itemId.chest
 55 d* paperdoll_itemId.legs
 56 d* paperdoll_itemId.feet
 57 d* paperdoll_itemId.cloak
 58 d* paperdoll_itemId.rhand
 59 d* HairStyle
 60 d* HairColor
 61 d* Face
 62 f* MaxHp
 63 f* MaxMp
 64 d* DeleteTimer
```

### 6.6 `CharacterSelected` (0x21)

```
CharacterSelectedPacket (opcode 0x21, handler 0x10009230). Nomes: Mobius C1 `CharSelected` (as posições 0..25 conferem letra por letra com o cliente)
  0 S  nome (<= 31 chars)                         ignorado pelo handler (INFERIDO pela análise de uso)
  1 d  objectId                                   ignorado
  2 S  título (<= 29 chars)                       ignorado
  3 d  sessionId (o mesmo do login)               ignorado
  4 d  clanId                                     ignorado
  5 d  0                                          ignorado
  6 d  sexo (0 = homem)                           gravado no `User` temporário do handler (sem consumidor achado)
  7 d  raça                                       ignorado
  8 d  classe                                     ignorado
  9 d  1 ("ativo")                                ignorado
 10 d  x                                          USADO: OnReceiveCharacterSelectedPacket(x, y, z) -> LoadSWMap
 11 d  y                                          USADO
 12 d  z                                          USADO
 13 f  HP atual (double)                          convertido (`_ftol2`) e gravado no `User` temporário
 14 f  MP atual (double)                          idem
 15 d  sp
 16 d  exp
 17 d  nível
 18 d  karma
 19 d  pkKills
 20..25 d  INT, STR, CON, MEN, DEX, WIT          (Mobius; ignorados pelo cliente)
 26..55 d  30 dwords reservados (zeros no Mobius)  ignorados
 56 d  hora do jogo em minutos do dia              USADO: OnReceiveServerTime(t) (leitura separada, depois dos 56 campos)
```
(O alinhamento automático por letra errou depois do campo 25 porque o `for` de 30 zeros do Mobius vira uma só linha; esta tabela é a que vale. O handler cria um `User` local (`User::User`, `0x10009241`) e grava nele alguns campos, mas **só entrega ao Engine `x y z` e a hora**; "ignorado"/"gravado" vem da análise de uso de leituras diretas, sem cópia em bloco: INFERIDO.)

### 6.7 `ServerObjectInfo` (0xA5)

```
ServerObjectInfoPacket (opcode 0xA5, handler 0x1001C990; nomes: log do cliente > expressão do servidor Mobius C1 `ServerObjectInfo`)
  0 d  ID
  1 d  ClassID
  2 S  name
  3 d  CanBeAttacked
  4 d  X
  5 d  Y
  6 d  Z
  7 d  heading
  8 f  ?
  9 f  ?
 10 f  collisionRadius
 11 f  collisionHeight
 12 d  CurrentHp
 13 d  MaxHp
 14 d  1
 15 d  0
```

### 6.8 `PetStatusUpdate` (0xCE)

```
PetStatusUpdatePacket (opcode 0xCE, handler 0x100088A0; nomes: log do cliente > expressão do servidor Mobius C1 `PetStatusUpdate`)
  0 d  SummonType
  1 d  ObjectId
  2 d  X
  3 d  Y
  4 d  Z
  5 S  Title
  6 d  curFed
  7 d  maxFed
  8 d  CurrentHp
  9 d  MaxHp
 10 d  CurrentMp
 11 d  MaxMp
 12 d  Level
 13 d  Stat
 14 d  ExpForThisLevel
 15 d  ExpForNextLevel
```

### 6.9 Limites de string por pacote (VERIFICADO: tamanho do buffer passado ao leitor)

| Pacote | `S` (índice da letra: máx. de caracteres) |
|---|---|
| 0x02 NpcSayPacket | #3: 1023 |
| 0x03 CharInfoPacket | #5: 255, #40: 31 |
| 0x04 UserInfoPacket | #5: 31, #83: 31 |
| 0x1B NpcHtmlMessagePacket | #1: 8191 |
| 0x1F CharacterSelectionInfoPacket | #0: 31, #2: 29 |
| 0x21 CharacterSelectedPacket | #0: 31, #2: 29 |
| 0x22 NpcInfoPacket | #30: 31, #31: 31 |
| 0x44 AskJoinPledgePacket | #1: 255 |
| 0x5D SayPacket2 | #2: 255, #3: 511 |
| 0x63 PartySmallWindowAllPacket | #1: 255 |
| 0x64 PartySmallWindowAddPacket | #1: 255 |
| 0x66 PartySmallWindowDeletePacket | #1: 255 |
| 0x67 PartySmallWindowUpdatePacket | #1: 255 |
| 0x68 PledgeShowMemberListAllPacket | #1: 31, #2: 31, #9: 31, #0: 255 |
| 0x69 PledgeShowMemberListUpdatePacket | #0: 255 |
| 0x6A PledgeShowMemberListAddPacket | #0: 255 |
| 0x6B PledgeShowMemberListDeletePacket | #0: 255 |
| 0x74 RestartResponsePacket | #1: 1023 |
| 0x7A SystemMessagePacket | #0: 1023 |
| 0x7D StartPledgeWarPacket | #0: 255, #1: 255 |
| 0x7F StopPledgeWarPacket | #0: 255, #1: 255 |
| 0x86 ShowBoardPacket | #0: 127, #1: 127, #2: 127, #3: 127, #4: 127, #5: 127, #6: 8191 |
| 0x95 FriendAddRequest | #0: 127 |
| 0x9C PledgeInfoPacket | #1: 127, #2: 127 |
| 0x9D PledgeExtendedInfoPacket | #0: 31, #3: 31, #12: 31 |
| 0xA5 ServerObjectInfoPacket | #2: 31 |
| 0xA8 GMViewCharacterInfoPacket | #5: 31, #83: 31 |
| 0xA9 GMViewPledgeInfoPacket | #0: 31, #2: 31, #3: 31, #9: 31, #0: 255 |
| 0xAA GMViewSkillInfoPacket | #0: 31 |
| 0xAB GMViewMagicInfoPacket | #0: 31 |
| 0xAC GMViewQuestInfoPacket | #0: 31 |
| 0xAD GMViewItemListPacket | #0: 31 |
| 0xAE GMViewWarehouseWithdrawListPacket | #0: 31 |
| 0xAF PartyMatchListPacket | #1: 31 |
| 0xB0 PartyMatchDetailPacket | #4: 511 |
| 0xB1 PlaySoundPacket | #1: 255 |
| 0xB5 PrivateStoreMsg | #1: 511 |
| 0xB9 TutorialShowHtmlPacket | #0: 8191 |
| 0xC1 AskJoinAlliancePacket | #1: 255, #2: 255, #3: 255 |
| 0xCA PetInfoPacket | #31: 31, #32: 31 |
| 0xCE PetStatusUpdatePacket | #5: 31 |
| 0xD2 PrivateStoreBuyMsg | #1: 511 |
| 0xD5 StartAllianceWarPacket | #0: 127, #1: 127 |
| 0xD7 StopAllianceWarPacket | #0: 127, #1: 127 |
| 0xDB PackageToListPacket | #1: 127 |
| 0xE2 CastleSiegeInfoPacket | #3: 127, #4: 127, #6: 127 |
| 0xE3 CastleSiegeAttackerListPacket | #1: 127, #2: 127, #6: 127, #7: 127 |
| 0xE4 CastleSiegeDefenderListPacket | #1: 127, #2: 127, #7: 127, #8: 127 |
| 0xE5 NickNameChangedPacket | #1: 127 |
| login op 0x09 | #1: 511 |

## 7. Armadilhas e divergências (cliente x servidores de referência)

### 7.1 O leitor não confere o fim do pacote (VERIFICADO)

- **Curto**: cada letra lê direto da memória (`§3.7`). O payload tem `tamanho-3` bytes e **um** byte nulo depois; o resto é o que estiver no *heap*. Campos `d` faltantes viram lixo (muitas vezes zero, às vezes não); um contador `h`/`d` lixo num laço pode ler muito além do buffer (INFERIDO que possa travar; não testei).
- **Longo**: o que sobra é ignorado. **Acrescentar campos no FIM de um pacote é seguro** para este cliente (o servidor Mobius já faz isso: manda campos que o cliente não lê, ver §7.7).
- **Strings `S`**: o destino tem tamanho fixo (coluna Notas, `S#i ≤n chars`). Se a string do fio tem mais caracteres que o limite, o leitor copia só o limite e avança `limite*2+2` bytes: **o resto da string é lido como se fosse o campo seguinte** (desalinha o pacote). Limites: nome/título de personagem 31 chars (`0x04`, `0x22`, `0x21`, `0x1F`; o 2º `S` da lista de personagens 29), `CharInfo` nome 255 chars, `NpcSay` 1023, `NpcHtmlMessage` 8191, parâmetro de texto do `SystemMessage` 1023, linha de `BlockedAccountWithMsg` 511.
- **Quadro**: sem limite superior de tamanho (§3.2).

### 7.2 `SpawnItem` (0x15): o cliente lê 8 `d`, o Mobius manda 7 (VERIFICADO nos dois lados)

O 8º `d` vira `FRotator.Yaw` do evento `OnSpawnItem(Item*, FVector, FRotator)` (o handler monta `FRotator(0, w8, 0)` em `0x10019509..0x10019553`). O Mobius escreve `objectId, itemId, x, y, z, stackable, count` (7). O 8º é, então, 1 byte nulo do próprio buffer + 3 bytes de *heap*: a guinada do item caído é lixo. Consequência visual NÃO VERIFICADA. Correção do lado do servidor: acrescentar um `d` (guinada) no fim.

### 7.3 `CharacterSelected` (0x21): a hora do jogo está no campo 56 (VERIFICADO no cliente; o Lucera do dono já está correto)

O cliente lê `S d S d×10 f f d×41` (56 letras) e depois **mais um `d`** (campo 56) que entrega a `OnReceiveServerTime` (minutos do dia). Depois dos 11 dwords de atributos vêm 30 zeros (campos 26..55); a hora vem logo em seguida.
O Mobius escreve **2 zeros a mais** entre os 30 zeros e a hora (campo 58): o cliente leria 0 -> todo login começaria à meia-noite. O `CharSelected.java` do Lucera do dono tem exatamente esta correção, com o comentário "made every login start at midnight". O que o cliente entrega ao Engine deste pacote: `x y z` (campos 10, 11, 12) em `OnReceiveCharacterSelectedPacket` e a hora em `OnReceiveServerTime`; sexo, HP e MP são convertidos e gravados num `User` local que não sai do handler (INFERIDO); o resto não é lido por ninguém.

### 7.4 `PartyMemberPosition` (0xC0): desalinha para membro desconhecido (VERIFICADO)

`d n` e, por membro, `d objectId` e **só se o `objectId` está no mapa de membros do grupo do cliente** (`socket+0x4DA8` e vizinhos; `0x10011AA4..0x10011B08`) lê `ddd` (x, y, z). Para um id que o cliente não conhece, os 12 bytes **não** são lidos e o próximo `objectId` sai das coordenadas. O servidor só deve mandar este pacote com membros que o cliente já recebeu por `PartySmallWindow*`.

### 7.5 As listas de loja e o bloco `dhhh` (VERIFICADO)

`SellList` (0x1C), `BuyList` (0x1D) e `GMViewWarehouseWithdrawList` (0xAE) só leem o bloco `dhhh` do item quando o `tipo1` (o `h` que abre o registro) está em **0..3** (`cmp ecx,0; jl; cmp ecx,3; jg`, `0x10008C77..0x10008C7C`). Um servidor que escreve o bloco sempre (o Mobius escreve) desalinha cada item de `tipo1` 4 (etc, quest). A correção já consta do `PORTE-C1.md` do dono.

### 7.6 Pares de opcodes que dividem handler

`0x06/0x07` (`Attack`), `0x08/0x09` (`Attacked`) e `0xB5/0xD2` (`PrivateStoreMsg` de venda e de compra: **gravam o mesmo campo** do `User`, então a mensagem de venda e a de compra não são independentes no cliente).

### 7.7 O que o Mobius manda a mais (benigno) e a menos (lixo)

Estrutura reta comparável (as duas pontas sem laço/condição; comparação mecânica):
- **Mobius manda mais, o cliente ignora:** `0x1B NpcHtmlMessage` (+1 `d`), `0x38` (+2), `0x39` (+1), `0x3A` (+1), `0x6A` (+1), `0x70` (+1), `0x75 MoveToPawn` (+3), `0x77` (+1), `0x78` (+2), `0x85` (+1), `0x95` (+1), `0x9F Ride` (+4), `0xB1` (+1), `0xD3` (+1), `0xE0 SpecialCamera` (+5), `0x20`/`0x25`/`0x87` (corpo que o cliente nem lê). [corrigido pela revisão: `0x0B Die` saiu desta lista — o Mobius manda 7 `d`, iguais aos 7 que o cliente lê (o "+3" somava os dois ramos de um `if/else`); os outros 15 itens foram conferidos um a um no `writeImpl` e estão certos.]
- **O cliente lê mais do que o Mobius manda (lixo):** `0x15 SpawnItem` (+1 `d`, §7.2) e `0x74 RestartResponse` (o cliente lê `d S`; o Mobius só manda o `d`, a `S` sai do lixo do *heap*).
- **Formato diferente:** `0xA1 PledgeShowInfoUpdate` (cliente: 9 `d`; o Mobius usa o layout de Interlude com nomes) — e o `0x81` do Mobius é de outro pacote.

### 7.8 Triagem do servidor Lucera do dono (extrator por regex do `writeImpl`; NÃO VERIFICADO: só leitura de fonte)

O mesmo cruzamento contra `Lucera-Interlude-FULL-main/java/l2/gameserver/network/l2/s2c` (classes de mesmo nome e opcode) mostrou, nos pacotes de estrutura reta onde o Lucera manda **menos** do que o cliente lê: `0xBE DeleteRadar` (o cliente lê `ddd`; o Lucera escreve só `d 1`). Os demais itens "vazios" ou diferentes que o extrator listou (p.ex. `0x02 NpcSay`, `0x7A SystemMessage`, `0x9D`, `0xA2`, `0xA5`, `0xBD`) usam funções auxiliares (`writeElements()` etc.) ou `writeImpl` vazio por supressão do porte, e o extrator não os segue: **não concluí nada sobre eles**. Se o dono quiser a conferência completa, o próximo passo é um extrator que siga as chamadas.

## 8. Como acrescentar ou mudar um pacote

**Mudar um pacote existente (lado do servidor).** (1) Descubra o opcode e o formato na §5; (2) respeite a ordem e as letras; (3) strings `S` dentro do limite; (4) **campos novos só no fim**; (5) cuidado com os pares/estruturas condicionais (§7.4, §7.5); (6) se o pacote tem 2 variantes por versão (§3.7), a que vale é a `> 336`.
O servidor deve usar o mesmo cálculo de chave do `KeyPacket` (§3.4) e, no login, o enchimento + checksum da §3.3.

**Pacote novo, opção A (sem mexer no cliente):** os 36 opcodes ignorados/Dummy **não fazem nada**: o servidor não ganha nada em usá-los sem código novo no cliente. **O que cabe sem código novo** é só o que já tem handler (por exemplo, texto na tela por `SystemMessage 0x7A`, HTML por `NpcHtmlMessage 0x1B` ou `TutorialShowHtml 0xB9`, som por `PlaySound 0xB1`, `ShowBoard 0x86`).

**Pacote novo, opção B (código novo no `NWindow.dll`) — mecanismo lido, NÃO VERIFICADO em jogo:**
1. Escolha um opcode livre (um dos Dummy: `0x05, 0x13, 0x14, 0x41, 0x42, 0x5C, 0x62, 0x7B, 0x7C, 0x88, 0xA0`, ou um ignorado como `0x20, 0x2A..0x2D, 0x2F, 0x40, 0x6C, 0x83, 0x8D, 0x90..0x94, 0x9A, 0xB7, 0xC6, 0xC8, 0xCD, 0xD6, 0xD8, 0xD9, 0xDF`; `0xED` e acima caem na entrada `0xED`, **não usar**).
2. Escreva um handler `__cdecl BYTE handler(UNetworkHandler* self, BYTE* payload)` (o `Tick` empilha `payload` e depois `this` e limpa a pilha depois: `0x10010A0C..0x10010A1A`; o retorno é ignorado). O `payload` **não inclui o opcode**; leia com o próprio leitor `0x10002A00(ctx, payload, "fmt", &saídas...)` (o `Network.dll` não exporta o leitor: chame pelo endereço, ou reimplemente as 6 letras da §3.7) e **confira os limites você mesmo**.
3. Em tempo de execução (por exemplo no primeiro *tick* do console, como as nossas features), grave o ponteiro em `base(Network.dll) + 0x41180 + opcode*0x104` (`DWORD` na posição `+0`; os endereços desta nota valem para a base preferida `0x10000000`: calcule a partir de `GetModuleHandle`) e, se quiser o nome no log de exceção, o nome UTF-16 em `+4`. A região é `.data` (gravável: VERIFICADO nas características da seção). Restaure o ponteiro original no `DLL_PROCESS_DETACH`.
4. Para falar com o resto do cliente, chame o `FL2NetNotify` como os handlers (`call [[[0x10055308]+0x48]+off]`, slots de `FL2NetNotify.vt.inc`) ou o console (`GL2Console`).
5. O handler roda dentro do `Tick` (a *thread* do motor, onde o `Tick` é chamado: INFERIDO), então pode mexer em objetos do jogo como os handlers de fábrica fazem.
O servidor Lucera precisa de uma classe `s2c` com `writeC(opcode)` e os campos; para o cliente ignorar um pacote que um servidor mande por engano, o opcode já é ignorado (§5).

**Pacote novo do cliente para o servidor:** `UNetworkHandler` tem um `Send` por vtable (`+0x60`, formato `__cdecl (socket, fmt, ...)` com o opcode como 1º argumento `c`); o Apêndice A lista o que o cliente já manda.

## 9. Apêndice A: cliente -> servidor (o que o `Network.dll` manda)

**Esta é só uma lista mecânica.** A referência completa do sentido cliente -> servidor (nomes dos campos, quem chama, o fluxo da UI) é `Rede_Protocolo_Cliente_Servidor.md`. Extração mecânica: cada chamada ao `Send` (`call [reg+0x60]`) com o formato ASCII empilhado e o opcode (o `push` imediatamente anterior ao do formato). O 1º `c` do formato é o opcode. A coluna "função" é a exportada que contém a chamada (os nomes `Exec` são o export mais próximo antes de um handler não exportado).
**Primeiros envios de cada fase:** login: depois do `Init` do servidor, `cxxdh` op `0x00` (§3.5); jogo: `cd` `(0, 419)` ao conectar, depois `cSdddd` op `0x08` (§3.4); ping: `cd` `(0, -2)`.
Os rótulos entre parênteses são **meus** (pontos de envio dentro de handlers que não são exportados). NÃO VERIFICADO quando cada função é chamada (o `UGameEngine`/console decide).


| Fase | Opcode | Função exportada (ou ponto de envio) | Formato (`c` = opcode) | Site da chamada |
|---|---|---|---|---|
| login | 0x00 | (handler de login 0x00, `Init`) | `cxxdh` | 0x10007706 |
| login | 0x00 | (handler de login 0x00, sem `package.dat`) | `cxxdh` | 0x10007744 |
| login | 0x02 | `RequestServerLogin` | `cddc` | 0x10003517 |
| login | 0x05 | `RequestServerList` | `cddc` | 0x10003484 |
| jogo | 0x00 | (conexão do `CClientSocket`, 0x10002320) | `cd` | 0x100023C1 |
| jogo | 0x01 | `MoveBackwardToLocation` | `cdddddd` | 0x1000360A |
| jogo | 0x01 | `MoveToLocation` | `cddddddd` | 0x10010BCC |
| jogo | 0x02 | `Say` | `cS` | 0x10003813 |
| jogo | 0x03 | `RequestEnterWorldPacket` | `c` | 0x10003F6D |
| jogo | 0x03 | `EnterWorld` | `cS` | 0x10021BC6 |
| jogo | 0x04 | `Action` | `cddddc` | 0x100036C2 |
| jogo | 0x04 | `RequestAttack` | `cdddd` | 0x10005D5C |
| jogo | 0x08 | (handler de jogo 0x00, `KeyPacket`) | `cSdddd` | 0x1000806A |
| jogo | 0x09 | `SendLogOutPacket` | `c` | 0x1000591D |
| jogo | 0x0A | `Attack` | `cddddc` | 0x10003752 |
| jogo | 0x0B | `RequestCharacterCreate` | `cSdddddddddddd` | 0x10003D6E |
| jogo | 0x0C | `RequestCharacterDelete` | `cd` | 0x10003C42 |
| jogo | 0x0D | `RequestGameStart` | `cd` | 0x10003BF2 |
| jogo | 0x0E | `RequestNewCharacter` | `c` | 0x10003CDD |
| jogo | 0x0F | `RequestItemList` | `c` | 0x10003E1D |
| jogo | 0x10 | `RequestEquipItem` | `cdd` | 0x10005AFB |
| jogo | 0x11 | `RequestUnEquipItem` | `cd` | 0x10005B5E |
| jogo | 0x12 | `RequestDropItem` | `cddddd` | 0x10004037 |
| jogo | 0x12 | `RequestDropItemFromPet` | `cddddd` | 0x10006557 |
| jogo | 0x14 | `RequestUseItem` | `cd` | 0x10003FC2 |
| jogo | 0x15 | `RequestTrade` | `cd` | 0x10003E73 |
| jogo | 0x16 | `RequestAddTradeItem` | `cddd` | 0x10003EC9 |
| jogo | 0x17 | `RequestTradeDone` | `cd` | 0x10003F22 |
| jogo | 0x1A | `RequestTeleport` | `cdddd` | 0x10004287 |
| jogo | 0x1A | `RequestTeleport` | `cddddd` | 0x100042B7 |
| jogo | 0x1B | `SocialAction` | `cd` | 0x10003863 |
| jogo | 0x1C | `ChangeMoveType` | `cd` | 0x100038B3 |
| jogo | 0x1D | `ChangeWaitType` | `cd` | 0x10003903 |
| jogo | 0x1E | `RequestSellItem` | `cddb` | 0x10004151 |
| jogo | 0x1F | `RequestBuyItem` | `cddb` | 0x1000421D |
| jogo | 0x20 | `RequestLinkHtml` | `cS` | 0x1000431B |
| jogo | 0x21 | `RequestBypassToServer` | `cS` | 0x1000451B |
| jogo | 0x22 | `RequestBBSWrite` | `cSSSSSS` | 0x100044C0 |
| jogo | 0x23 | `RequestCreatePledge` | `cS` | 0x1000473B |
| jogo | 0x24 | `RequestJoinPledge` | `cd` | 0x100048AB |
| jogo | 0x25 | `RequestAnswerJoinPledge` | `cd` | 0x1000479B |
| jogo | 0x26 | `RequestWithDrawalPledge` | `c` | 0x100048FD |
| jogo | 0x27 | `RequestOustPledgeMember` | `cS` | 0x100047FB |
| jogo | 0x28 | `RequestDismissPledge` | `c` | 0x1000484D |
| jogo | 0x29 | `RequestJoinParty` | `cdd` | 0x10005189 |
| jogo | 0x2A | `RequestAnswerJoinParty` | `cd` | 0x100051EB |
| jogo | 0x2B | `RequestWithDrawalParty` | `c` | 0x1000523D |
| jogo | 0x2C | `RequestOustPartyMember` | `cS` | 0x1000529B |
| jogo | 0x2D | `RequestDismissParty` | `c` | 0x100052ED |
| jogo | 0x2E | `RequestMagicSkillList` | `c` | 0x100057AD |
| jogo | 0x2F | `RequestMagicSkillUse` | `cddc` | 0x10005829 |
| jogo | 0x30 | `SendAppearingPacket` | `c` | 0x1000394D |
| jogo | 0x31 | `SendWareHouseDepositList` | `cdb` | 0x100054E9 |
| jogo | 0x32 | `SendWareHouseWithdrawList` | `cdb` | 0x100055B9 |
| jogo | 0x33 | `RequestShortCutReg` | `cdddd` | 0x10005644 |
| jogo | 0x34 | `RequestShortCutUse` | `cddc` | 0x100056B9 |
| jogo | 0x35 | `RequestShortCutDel` | `cd` | 0x1000571B |
| jogo | 0x36 | `CanNotMoveAnymore` | `cdddd` | 0x10010F5B |
| jogo | 0x37 | `RequestTargetCancel` | `c` | 0x1000575D |
| jogo | 0x38 | `Say2` | `cSd` | 0x100058AF |
| jogo | 0x38 | `Say2` | `cSdS` | 0x100058C8 |
| jogo | 0x39 | `SendCharacterInfo` | `ccS` | 0x1000596D |
| jogo | 0x39 | `SendSummonCmd` | `ccS` | 0x100059CD |
| jogo | 0x39 | `SendL2ParamSetting` | `ccdd` | 0x10005A3D |
| jogo | 0x39 | `SendServerStatus` | `cc` | 0x10005A8F |
| jogo | 0x3C | `RequestPledgeMemberList` | `c` | 0x100046DD |
| jogo | 0x3E | `RequestMagicItem` | `c` | 0x10005BFD |
| jogo | 0x3F | `RequestSkillItem` | `c` | 0x10005BAD |
| jogo | 0x41 | `MoveWithDelta` | `cddd` | 0x100039B7 |
| jogo | 0x42 | `GetOnVehicle` | `cdddd` | 0x10003A33 |
| jogo | 0x43 | `GetOffVehicle` | `cdddd` | 0x10003AA3 |
| jogo | 0x44 | `AnswerTradeRequest` | `cd` | 0x10005C5E |
| jogo | 0x45 | `RequestActionUse` | `cddc` | 0x10005CD9 |
| jogo | 0x46 | `RequestRestart` | `c` | 0x10005DAD |
| jogo | 0x47 | `RequestSiegeInfo` | `c` | 0x10005E4D |
| jogo | 0x48 | `ValidatePosition` | `cddddd` | 0x10003B17 |
| jogo | 0x49 | `RequestSEKCustom` | `cdd` | 0x10005EBB |
| jogo | 0x4A | `StartRotating` | `cdd` | 0x10003B49 |
| jogo | 0x4B | `FinishRotating` | `cdd` | 0x10003BA3 |
| jogo | 0x4D | `RequestStartPledgeWar` | `cS` | 0x1000495B |
| jogo | 0x4E | `RequestReplyStartPledgeWar` | `cSd` | 0x100049C9 |
| jogo | 0x4F | `RequestStopPledgeWar` | `cS` | 0x10004A2B |
| jogo | 0x50 | `RequestReplyStopPledgeWar` | `cSd` | 0x10004A99 |
| jogo | 0x51 | `RequestSurrenderPledgeWar` | `cS` | 0x10004AFB |
| jogo | 0x52 | `RequestReplySurrenderPledgeWar` | `cSd` | 0x10004B69 |
| jogo | 0x53 | `RequestSetPledgeCrest` | `cdb` | 0x100045D8 |
| jogo | 0x55 | `RequestGiveNickName` | `cSS` | 0x10004C39 |
| jogo | 0x57 | `RequestShowboard` | `cd` | 0x10005F0F |
| jogo | 0x58 | `RequestEnchantItem` | `cd` | 0x10006152 |
| jogo | 0x59 | `SendDestroyItem` | `cdd` | 0x10005FB9 |
| jogo | 0x59 | `RequestDestroyItem` | `cdd` | 0x10006017 |
| jogo | 0x5B | `SendBypassBuildCmd` | `cS` | 0x10005F6B |
| jogo | 0x5C | `MoveToLocation` | `cddddddd` | 0x10010D63 |
| jogo | 0x5D | `CanNotMoveAnymore` | `cddddd` | 0x10010ED5 |
| jogo | 0x5E | `RequestFriendInvite` | `cS` | 0x10004F5B |
| jogo | 0x5F | `RequestFriendAddReply` | `cd` | 0x1000506B |
| jogo | 0x60 | `RequestFriendList` | `c` | 0x1000500D |
| jogo | 0x61 | `RequestFriendDel` | `cS` | 0x10004FBB |
| jogo | 0x62 | `RequestCharacterRestore` | `cd` | 0x10003C92 |
| jogo | 0x63 | `RequestQuestList` | `c` | 0x100060AD |
| jogo | 0x64 | `RequestDestroyQuest` | `cd` | 0x10006102 |
| jogo | 0x66 | `RequestPledgeInfo` | `cd` | 0x10012AC3 |
| jogo | 0x67 | `RequestPledgeExtendedInfo` | `cS` | 0x1000468B |
| jogo | 0x68 | `RequestPledgeCrest` | `cd` | 0x10004632 |
| jogo | 0x69 | `RequestSurrenderPersonally` | `cS` | 0x10004BCB |
| jogo | 0x6A | `Ride` | `cdd` | 0x100037B9 |
| jogo | 0x6B | `RequestAcquireSkillInfo` | `cdd` | 0x100061B9 |
| jogo | 0x6C | `RequestAcquireSkill` | `cdd` | 0x10006229 |
| jogo | 0x6D | `RequestRestartPoint` | `cd` | 0x10005E0B |
| jogo | 0x6E | `RequestGMCommand` | `cSdd` | 0x100062B9 |
| jogo | 0x6F | `RequestPartyMatchConfig` | `cdddS` | 0x10005374 |
| jogo | 0x70 | `RequestPartyMatchList` | `cd` | 0x100053D2 |
| jogo | 0x71 | `RequestPartyMatchDetail` | `cd` | 0x10005422 |
| jogo | 0x72 | `RequestCrystallizeItem` | `cdd` | 0x10006067 |
| jogo | 0x73 | `RequestPrivateStoreManage` | `c` | 0x100065ED |
| jogo | 0x74 | `SetPrivateStoreList` | `cdb` | 0x100066BD |
| jogo | 0x75 | `RequestPrivateStoreManageCancel` | `c` | 0x100066FD |
| jogo | 0x76 | `RequestPrivateStoreQuit` | `c` | 0x1000671D |
| jogo | 0x77 | `SetPrivateStoreMsg` | `cS` | 0x10006746 |
| jogo | 0x78 | `RequestPrivateStoreList` | `cd` | 0x10006762 |
| jogo | 0x79 | `SendPrivateStoreBuyList` | `cddb` | 0x10006841 |
| jogo | 0x7A | `ReviveReply` | `cd` | 0x10003DE6 |
| jogo | 0x7B | `RequestTutorialLinkHtml` | `cS` | 0x1000437B |
| jogo | 0x7C | `RequestTutorialPassCmdToServer` | `cS` | 0x1000457A |
| jogo | 0x7D | `RequestTutorialQuestionMarkPressed` | `cd` | 0x100043D2 |
| jogo | 0x7E | `RequestTutorialClientEvent` | `cd` | 0x10004422 |
| jogo | 0x7F | `RequestPetition` | `cS` | 0x10006312 |
| jogo | 0x80 | `RequestPetitionCancel` | `cS` | 0x10006360 |
| jogo | 0x81 | `RequestGMList` | `c` | 0x100063B0 |
| jogo | 0x82 | `RequestJoinAlly` | `cd` | 0x10004C9E |
| jogo | 0x83 | `RequestAnswerJoinAlly` | `cd` | 0x10004CFE |
| jogo | 0x84 | `RequestWithdrawAlly` | `c` | 0x10004E00 |
| jogo | 0x85 | `RequestOustAlly` | `cS` | 0x10004D5E |
| jogo | 0x86 | `RequestDismissAlly` | `c` | 0x10004E50 |
| jogo | 0x87 | `RequestSetAllyCrest` | `cdb` | 0x10004EAB |
| jogo | 0x88 | `RequestAllyCrest` | `cd` | 0x10004F05 |
| jogo | 0x89 | `RequestChangePetName` | `cS` | 0x100065A5 |
| jogo | 0x8A | `RequestPetUseItem` | `cd` | 0x10006455 |
| jogo | 0x8B | `RequestGiveItemToPet` | `cdd` | 0x100064BC |
| jogo | 0x8C | `RequestGetItemFromPet` | `cddd` | 0x10006509 |
| jogo | 0x8E | `RequestAllyInfo` | `c` | 0x10004DB0 |
| jogo | 0x8F | `RequestPetGetItem` | `cd` | 0x10006405 |
| jogo | 0x90 | `RequestPrivateStoreBuyManage` | `c` | 0x10006880 |
| jogo | 0x91 | `SetPrivateStoreBuyList` | `cdb` | 0x1000697F |
| jogo | 0x92 | `RequestPrivateStoreBuyManageCancel` | `c` | 0x100069C0 |
| jogo | 0x93 | `RequestPrivateStoreBuyQuit` | `c` | 0x100069E0 |
| jogo | 0x94 | `SetPrivateStoreBuyMsg` | `cS` | 0x10006A09 |
| jogo | 0x95 | `RequestPrivateStoreBuyList` | `cd` | 0x10006A35 |
| jogo | 0x96 | `SendPrivateStoreBuyBuyList` | `cddb` | 0x10006B08 |
| jogo | 0x97 | `SendTimeCheckPacket` | `cdd` | 0x10006B5C |
| jogo | 0x98 | `RequestStartAllianceWar` | `cS` | 0x10006BCE |
| jogo | 0x99 | `ReplyStartAllianceWar` | `cSd` | 0x10006C3C |
| jogo | 0x9A | `RequestStopAllianceWar` | `cS` | 0x10006C9E |
| jogo | 0x9B | `ReplyStopAllianceWar` | `cSd` | 0x10006D0C |
| jogo | 0x9C | `RequestSurrenderAllianceWar` | `cS` | 0x10006D6E |
| jogo | 0x9D | `RequestSkillCoolTime` | `c` | 0x10006B90 |
| jogo | 0x9E | `RequestPackageSendableItemList` | `cd` | 0x10006DCE |
| jogo | 0x9F | `RequestPackageSend` | `cddb` | 0x10007140 |
| jogo | 0xA0 | `RequestBlock` | `cd` | 0x100050E0 |
| jogo | 0xA0 | `RequestBlock` | `cdS` | 0x100050FF |
| jogo | 0xA0 | `RequestBlock` | `cdS` | 0x1000511B |
| jogo | 0xA1 | `RequestCastleSiegeInfo` | `cd` | 0x10006E2E |
| jogo | 0xA2 | `RequestCastleSiegeAttackerList` | `cd` | 0x10006E8E |
| jogo | 0xA3 | `RequestCastleSiegeDefenderList` | `cd` | 0x10006EEE |
| jogo | 0xA4 | `RequestJoinCastleSiege` | `cddd` | 0x10006F69 |
| jogo | 0xA5 | `RequestConfirmCastleSiegeWaitingList` | `cddd` | 0x10006FE9 |
| jogo | 0xA6 | `RequestSetCastleSiegeTime` | `cdd` | 0x1000705C |
| jogo | 0xA7 | `RequestMultiSellChoose` | `cdd` | 0x100071AC |
| jogo | 0xA8 | `NetPing` | `cddd` | 0x1000724A |
| jogo | 0xA9 | `RequestRemainTime` | `c` | 0x100072A0 |

## 10. Não verificado / perguntas em aberto

1. **Nada foi executado.** O fluxo de §3.4/§3.5 é a leitura estática dos handlers e dos `Send`; a ordem dos eventos no tempo (p.ex. quando o cliente manda o op `0x03` `RequestEnterWorldPacket`, o que o servidor manda entre o `AuthLogin` e o `CharacterSelectionInfo`) vem do código dos handlers e do servidor Mobius, **não** de uma captura de tráfego.
2. **Campos "ignorados".** A análise de uso de campos é confiável só em handlers sem cópia de estrutura em bloco; nos grandes (`0x03`, `0x04`, `0x22`, `0xA8`, `0xCA`...) as letras são VERIFICADAS (vêm do leitor) mas **não sei quais campos o evento realmente usa**.
3. **Nomes de campo** são dicas (log do cliente ou Mobius); onde o Mobius e o cliente discordam (§7.7) não dei nome.
4. **`CharInfo`/`UserInfo` etc., variante `<= 336`:** só descrevi as letras; nunca roda neste cliente (manda 419). Se alguém trocar o texto `"$Revision: 419 $"`, os cinco handlers da §3.7 mudam de formato.
5. **`ServerAddr` e `package.dat`.** O `l2.ini` do cliente de teste é `Ver212` (não decodifiquei), então não li o valor de `[URL] ServerAddr`; INFERIDO que o "ini padrão" seja o `l2.ini`. Não há `package.dat` na pasta `System - Teste Acessorio`: nesse cliente o `h` do `AuthLogin` é **0** (VERIFICADO a ausência do arquivo; não sei se o servidor do dono usa o campo).
6. **Fechamento limpo do socket de jogo** (FD_CLOSE sem erro) não chama `OnServerClose` por esse caminho; o que o cliente faz quando o servidor fecha sem erro NÃO VERIFICADO.
7. **`LeaveWorld 0x10`**: [corrigido pela revisão: resolvido — os 5 limpadores esvaziam, incondicionalmente, os `TMap` do socket em `+0x4D58`, `+0x4D6C`, `+0x4D80`, `+0x4DA8` e `+0x4D94` (ver a linha `0x10` da §5).] **`DeleteRadar 0xBE`**: o cliente lê `ddd`; o que acontece com `d 1` + lixo (Lucera) NÃO VERIFICADO.
8. **O valor de `c` do `LoginOk`/`PlayOk`** e os campos `w3`, `w4` do `LoginOk` e o `c = 3` do `RequestServerList`: sem uso achado; INFERIDOS como reservados.
9. **Duas tabelas, uma fila:** a fila de pacotes não guarda de qual socket veio o pacote; um pacote atrasado do login depois do `PlayOk` seria tratado pela tabela de jogo (INFERIDO; não vi acontecer).
10. **`DispatchSocketWindowMsg`** (`0x10022160`) é exportado; [corrigido pela revisão: quem o importa é o **`WinDrv.dll`** (único módulo de `orig/` com essa entrada na tabela de importação; `L2.exe`, `Engine.dll`, `Window.dll` não a têm)] o sítio da chamada dentro do `WinDrv` não foi lido: o desenho "mensagens `0x501/0x502/0x503`" vem do corpo da função, não do chamador.
11. **O servidor Lucera:** a conferência da §7.8 é uma triagem por regex; a conferência completa pede um extrator que siga as chamadas (`writeElements()`, `super`, etc.).
12. O Mobius C1 não é a especificação oficial do C1: onde ele e o cliente discordam, **o cliente é a verdade**; mas pode haver pacotes que o Mobius escreve certo e este cliente lê de outra forma só em ramos que não percorri (listas com condição).

## 11. Como repetir

Os programas descartáveis (no *scratchpad* da sessão, `rede_net/`, **fora do repositório**): `tables.py` (reparse das tabelas e da inicialização estática), `ana.py` (descida recursiva, leitor, eventos), `flow.py` (propagação da pilha, uso de campos), `dossier.py` (junta tudo), `sends.py` (envios), `jx.py`/`lux.py` (extração do `writeImpl` do Mobius e do Lucera), `gen_tables.py` (este documento). Receita mínima para conferir uma linha:

1. `Image('orig/Network.dll')` (`tools/nwd/image.py`); `u32(0x10041150 + 4*op)` (login) ou `u32(0x10041180 + op*0x104)` (jogo) dá o handler; o nome é `wstr(0x10041184 + op*0x104)` **depois da inicialização estática** (no arquivo em disco só a entrada 0 está preenchida: o resto vem de `mov [entrada], handler` em `0x1002C0F0..0x1002DC09`).
2. Desassemblar o handler e achar `call 0x10002A00`: o 3º argumento empilhado é o formato (`push offset`, *string* ASCII em `.rdata`); os `push` anteriores são os destinos.
3. Eventos: procurar `mov r,[0x10055308]; mov r,[r+0x48]; mov r,[r]; call [r+off]`; `off/4` é o slot.
4. Cifra do jogo: funções `0x10002430` (TX) e `0x10002480` (RX) e a atribuição das chaves em `0x10007FA4..0x10007FCA`. Cifra do login: `0x100032E0` e `0x10003330`; chave em `0x10041138`.

## Revisão independente (Fable)

Feita em 04/10/2026, **só leitura**: capstone (`tools/nwd/image.py`) sobre `orig/Network.dll` com programas próprios no *scratchpad* da sessão (fora do repositório), a tabela de importação de `orig/WinDrv.dll` (pefile), o código-fonte do Mobius C1 e do Lucera do dono, e a pasta `System - Teste Acessorio` (só leitura). Nada foi compilado, instalado ou executado; registro e pastas do jogo intactos; Ghidra não usado. As correções estão **no corpo**, marcadas `[corrigido pela revisão]` (15 marcas em 14 trechos); esta seção lista o que foi conferido, o que foi refutado e o que continua em aberto.

### R1. Método da revisão

- **Tabelas e handlers (mecânico, independente do analista).** Reparseei a tabela de login (11 ponteiros em `0x10041150`) e a de jogo (238 entradas de `0x104` em `0x10041180`, a partir dos `mov [entrada], handler` / `push L"nome"; push entrada+4` da inicialização estática `0x1002C0F0..0x1002DC09`). Para **cada um dos 238 + 11 handlers** varri o corpo linearmente e extraí: as *strings* de formato passadas a `0x10002A00` (em ordem de endereço), os imediatos empilhados antes de cada chamada com `S` (tamanho do buffer), e os slots do `FL2NetNotify` por rastreio de registradores (`[0x10055308]` → `+0x48` → `[.]` → `call [reg+off]`). Comparei com as colunas "Formato lido", "Eventos" e com a §6.9.
- **Leitura à mão** (assembly inteiro): `Tick`, leitor `0x10002A00` e seus auxiliares (`0x100029D0`, `0x100029B0`), empacotador `0x100026E0`, `OnReceivePacket` (login e jogo), `recv` do login `0x10002F00`, *thread* `0x1000FD60`, `Connect` (login `0x10002C50` início, jogo `0x10010020`), `OnConnect`/versão `0x10002320`, as 4 funções de cifra (`0x10002430`, `0x10002480`, `0x100032E0`, `0x10003330`) e o escalonador Blowfish `0x10001880`/`0x10001820`/`0x10001850`, `KeyPacket` `0x10007F40`, `RequestAuthLogin`/`RequestServerList`/`RequestServerLogin`, os 11 handlers de login, `DispatchSocketWindowMsg`, `0x10022020`, `CPingSocket::Connect` `0x10010520` e `0x10002900`, `NetPing` `0x100071F0`, `AddNetworkQueue`/`DispatchNetworkQueue`, os construtores dos 3 sockets, `UNetworkHandler::Init`, `0x100022D0`, os 5 limpadores do `LeaveWorld`, e os handlers de jogo `0x01, 0x0B, 0x10, 0x12, 0x15, 0x1A, 0x1C, 0x1F, 0x21, 0x56, 0x57, 0x70, 0x7A, 0x89, 0x9C, 0xB5/0xD2, 0xB8, 0xC0, 0xE5, 0xE6, 0xEC` (27 handlers de jogo + 11 de login, acima da amostra de 25 pedida).
- **Fontes de servidor:** `L2J_Mobius_C1_HarbingersOfWar` (`KeyPacket`, `GameClient.CRYPT_KEY`, `Encryption`, `LoginClient`/`NewCrypt`, `Init`, `LoginOk`, `PlayOk`, `ServerList`, `AuthLogin`, `SpawnItem`, `Die`, `NpcHtmlMessage`, `RestartResponse`, `MoveToPawn`, `TeleportToLocation`, `TargetSelected`, `TargetUnselected`, `Ride`, `SpecialCamera`, `StopRotation`, `StartRotation`, `PlaySound`, `SetupGauge`, `FriendAddRequest`, `VehicleStarted`, `PledgeShowMemberListAdd`, `VehicleCheckLocation`, `Attack`, `StatusUpdate`, `LeaveWorld`, enum `ServerPackets`) e Lucera (`CharSelected.java`, `DeleteRadar.java`, `c1_client_opcodes.txt`, `PORTE-C1.md`).

### R2. Verificado (com evidência)

| Afirmação do documento | Evidência da revisão |
|---|---|
| Tabela de login: 11 entradas, handlers da §4; `0x0A` = Dummy `0x10002130` | `u32(0x10041150+4i)` bate entrada a entrada; a 12ª palavra é 0 |
| Tabela de jogo: 238 entradas, **todos os 238 handlers e nomes** da §5 | reparse da inicialização estática: 238/238 iguais; `0x10001BE0` = `xor al,al; ret` (24 entradas), `0x10002130` = Dummy (12, inclusive `0xED`), `0x10001BF0` só loga `(Receive)AuthLoginSuccess` |
| Contagens 24 / 12 / 1 / 201 e os 8 handlers sem evento (`0x70 0x9C 0xB5 0xB8 0xC0 0xD2 0xE5 0xEC`) | iguais no meu varredor (o `0xE6` só tem eventos "via", como o documento diz) |
| **Coluna "Formato lido" das 238 linhas** (letras, laços, variantes) | o varredor reproduz todas as *strings* de formato na mesma ordem; conferi à mão os laços/condições de `0x1A, 0x1C, 0x56, 0x57, 0x7A, 0xC0` (tabela de saltos do `0x7A` em `0x1000D8D4`: tipo 0 → `S`, 1/2/3/5 → `d`, 4 → `dd`, >5 → nada; condição `0 <= tipo1 <= 3` do `0x1C` em `0x10008C77..7C`; `0xC0` só lê `ddd` se o id está no mapa `+0x4DA8`) |
| Variantes por versão (`cmp ..., 0x150` em `0x1000CA84`, `0x100157A1`, `0x10016642`; `0x1000257B` compara com `0x16B`) | são as únicas 4 comparações com `0x150`/`0x16B` no `.text`; `[0x10041134]` só é escrito em `0x100023C9` |
| Formatos longos da §3.7/§6: `CharInfo` 54+`h`+`c`, `UserInfo` 95+`h`+`c`, `GMViewCharacterInfo` 92, `CharacterSelectionInfo` 64, `CharacterSelected` 56+`d`, `NpcInfo` 35, `PetInfo` 59 | letras contadas nas *strings* reais (`0x10015AC9`, `0x10016BA6`, `0x10009C7D`, `0x10009107`, `0x1000941D`, `0x1001BFEE`, `0x1001A66A`) |
| **Coluna "Eventos" (slots)** das 238 linhas | iguais no varredor, inclusive os slots 1..10 das famílias de movimento (`0x01`→1, `0x89`→2, `0x79`→3, `0x75`→4, `0x76`→5, `0x8B`→6, `0xDE`→7, `0xDD`→8, `0x77`→9, `0x78`→10) e os 28 slots 44..71 do `StatusUpdate` |
| **§6.9, limites das strings** (todos) | imediatos antes de cada leitura com `S`: `0x40`→31, `0x3C`→29, `0x100`→127, `0x200`→255, `0x400`→511, `0x800`→1023, `0x4000`→8191, nas posições que o documento diz; o auxiliar `0x100029D0` é um `wcslen` **limitado** (para em `max`), logo a cópia nunca passa do buffer |
| Leitor `0x10002A00`: letras, avanços, `S` = `min(len,máx)*2+2`, letra desconhecida não consome, sem conferência de fim | assembly completo; tabela de índices `0x10002B4C` (`S..s`) e de saltos `0x10002B30` |
| `Tick`: estado em `[0x10055308]+0x44`, clamps `0x0A`/`0xED`, `push payload; push this; call; add esp,8`, laço até a fila esvaziar, `rdtsc` com `[[+0x3C]+0x4D28]`, SEH com `"UNetworkHandler::Tick"`/`"Function Name=%s"`, virtual `+0x398` no fim | `0x10010730..0x10010B4C`; strings `0x10033554`/`0x10033530` |
| Quadro `uint16` LE **incluindo** os 2 bytes; payload = `tamanho-3` bytes + 1 NUL; `NetPacket` de 8 B; `AddNetworkQueue` sob a `CRITICAL_SECTION +0x58` | `0x10010240`/`0x10003150`; `0x10011CA0` (`FArray::Add` em `this+0x30`) |
| Login: `recv` 2 + resto, `[+0x5614/5618/561C]`, `MSG_PEEK`; jogo: `recv(sock, 0x10051300, 0x4000, 0)` numa *thread* criada por `_beginthread`, semáforo `+0x494C` com `WaitForSingleObject(…, 100 ms)`, `0x503` só faz `ReleaseSemaphore` | `0x10002F64..0x1000305F`; `0x1000FD60..0x10010011`; `0x100103F2..0x10010400` |
| `Send`: empacotador com letras `c h d f S s b x`, limite `0x1FFE`, estouro → devolve 0 e nada é enviado; cabeçalho `n+2`; cifra se a flag está ligada; contadores `+0x496C/+0x4970` | `0x100026E0` (tabela `0x100028AC`/`0x100028CC`), `0x10002B70`, `0x10003230` |
| Blowfish: chave de 21 bytes em `0x10041138` (20 chars + NUL), `0x10014120 → 0x10001880(chave, 0x15)`, P/S-boxes em `0x10040070`/`0x100400B8`, blocos de duas palavras LE; **só decifra se múltiplo de 8** e **não confere checksum**; envio: `(n+7)&~7`, XOR das palavras, `n2 = n1+8` | `0x10001880` (P[0] = `0x243F6A88`), `0x10003330` (`test al,7`), `0x100032E0`; SHA-1 dos 21 bytes = `f75237bf…` |
| Chave do login **idêntica** à do `LoginClient` do Mobius C1 | SHA-1 do literal Java (`"…\000"`, 21 bytes) = SHA-1 dos 21 bytes do `Network.dll` (`f75237bf78eb63f2d1d009124d0ae56940aadd10`); conferido sem copiar a chave |
| Cifra do jogo: `p[i] = c[i] ^ k[i&7] ^ c[i-1]`, `k += n` com extensão de sinal; TX `+0x4E78`, RX `+0x4E80`, flag `+0x4E70`; `KeyPacket` `cdd`, `setne` → flag, `cdq; or edx,0x87546CA1` nas duas chaves; resposta imediata `cSdddd` op 8 com `(8, conta, [C4], [C8], [AC], [B0])` | `0x10002430`, `0x10002480`, `0x10007F40..0x1000806A`; `Encryption.java` do Mobius faz a mesma operação (`raw ^ key[i&7] ^ prev`, `old += size`); `GameClient.CRYPT_KEY` termina em `a1 6c 54 87` |
| Versão: `sscanf("$Revision: 419 $", "%cRevision: %d %c")` em `0x10002320`, `Send("cd", 0, 419)` ao `0x502`, `WSAAsyncSelect(0x503, 0x25)` | `0x1002E8D4`, `0x100023A5..0x100023C9` |
| Fluxo de login completo da §3.5 (globais `0x100567AC/B0/B4/B8`, `C4/C8`, `A8/BC/C0`, `CC`/`EA`; porta `0x83A`; `cxxdh` com `x(14,id) x(16,senha) d=8 h=atoi(package.dat)`; `cddc` dos ops 5 e 2; `PlayOk` → `inet_ntoa` + `Connect` do jogo + estado 2; quais handlers fecham o socket) | assembly dos 11 handlers e das 3 exportadas; `0x10022974` é um `wcsncpy` (copia até n e preenche com zeros); strings `0x10031150 "cxxdh"`, `0x10031158 L"package.dat"` |
| `ServerList`: `cc` + n × `cddcchhcd`, `OnStartServerList(2º c)`, `OnAddServerList(L2ParamStack[9])`, `OnEndServerList(0)` | `0x100079F0`; os `h` entram com `movsx` (com sinal) |
| `LoginOk`: `w1..w4` → `AC/B0/B4/B8`, `w8` → `OnStartNotice`; Mobius escreve `loginOkID1, loginOkID2, 0, 0, 0x3EA, 0, 0, 2` | `0x100078F0`; `LoginOk.java` |
| `CPingSocket`: 128 × 16 B a partir de `+0x28`, `connect` bloqueante, `SetServerPing(…, 0x270F)` na falha, `Send("cd", 0, -2)` no sucesso | `0x100074C0`, `0x10010520..0x10010667`, `0x100106E0` |
| `.data` gravável; tabela de jogo é `.data` | características `0xC0000040` |
| `UNetworkHandler` vtable `+0x70` `DispatchNetworkQueue`, `+0x74` `AddNetworkQueue`, `+0x37C` `NetPing`, `+0x1D8` `RequestAllyCrest` | vtable em **`0x10033798`** (`+0x60` = `Tick`, `+0x190` = `RequestPledgeCrest` `0x10004620`) |
| Achados da §7: `SpawnItem` 8º `d` → `FRotator.Yaw` (Mobius manda 7); `CharacterSelected` hora no campo 56 (Lucera tem o comentário "made every login start at midnight"); `PartyMemberPosition` desalinha; bloco `dhhh` condicional; `RestartResponse` `d S` vs Mobius só `d`; `NpcHtmlMessage` Mobius +1 `d`; `DeleteRadar` do Lucera = `d 1`; `0x3C` JoinParty **e** AutoAttackStop no enum do Mobius; `0x43` = `ManagePledgePower`, `0x81` = `SurrenderPledgeWar` no Mobius | `0x10019505..0x10019553`, `0x100094C7..0x100094EE`, `0x10011AC0..0x10011B0B`, `0x10008C77`; `SpawnItem.java` (7 `writeInt`), `CharSelected.java` do Lucera linha 92, `DeleteRadar.java`, `ServerPackets.java` |
| §7.7, "Mobius manda mais": `0x38`(+2) `0x39`(+1) `0x3A`(+1) `0x6A`(+1) `0x70`(+1) `0x75`(+3) `0x77`(+1) `0x78`(+2, `dddc`) `0x85`(+1) `0x95`(+1) `0x9F`(+4) `0xB1`(+1) `0xD3`(+1) `0xE0`(+5) | `writeImpl` de cada classe, contado à mão (nenhum tem `if/else` no corpo) |
| `c1_client_opcodes.txt` tem 238 linhas; `package.dat` não existe na pasta de teste; `l2.ini` começa por `Lineage2Ver212` | `wc -l`; `ls`; os 28 primeiros bytes |

### R3. Refutado e corrigido no corpo (`[corrigido pela revisão]`)

| # | Onde | Estava | Está (evidência) |
|---|---|---|---|
| 1 | §3.2 buffer de quadro | `+0x844..+0x4D2C` = 17.640 B | `+0x844..+0x4844` = **16.384 B**: `+0x4844` é o contador de pacotes (`inc [this+0x4844]`), `+0x4848` contador, `+0x484C` *hostname*, `+0x494C` semáforo; o `recv` da *thread* também é de `0x4000`. A ausência de limite continua VERIFICADA |
| 2 | §3.2 `OnReceivePacket` | "`tamanho-2 <= 0` loga e descarta" | o teste é **sem sinal** (`ja`): só `tamanho == 2` é descartado. Cabeçalho `0`/`1`: na *thread* do jogo vira contagem negativa do `rep movsd` (INFERIDO: *crash*); no login, o estado nunca avança e o laço `recv(0)`/`MSG_PEEK` gira (INFERIDO: *hang*) |
| 3 | §5 `0x0B`, §7.7 | "Mobius manda 10 `d`" / "Die (+3)" | o Mobius manda **7** (`if/else` com 3 `d` em cada ramo; o extrator somou os dois ramos). Coluna Mobius `~` → `=` |
| 4 | §3.1, §3.8 `[+0x78]` | "tempo acumulado"; `NetPing` manda "dois `d` derivados de `GL2PawnCR` e do tempo acumulado" | média móvel exponencial `0.98/0.02` de um tempo em segundos (contador do `GStats` × `GSecondsPerCycle`; INFERIDO tempo de quadro); o `NetPing` manda `(0xA8, d, 1/[+0x78] ≈ FPS, GL2PawnCR×k)` e zera |
| 5 | §3.1, §10.10 | "`WinDrv` (INFERIDO)" | **`WinDrv.dll` importa** `?DispatchSocketWindowMsg@@YAHPAUHWND__@@IIJ@Z` (único módulo de `orig/`); o sítio da chamada continua não lido |
| 6 | §1 item 2 | "199 handlers distintos" para 201 entradas | 198 (201 − 3 pares); 199 só contando o `0x11` |
| 7 | §5 `0x10`, §10.7 | limpadores "NÃO VERIFICADO" | incondicionais; esvaziam os `TMap` `+0x4D58` (libera valores), `+0x4D6C`, `+0x4D80`, `+0x4DA8` (libera valores; é o mapa do `0xC0`) e `+0x4D94` |
| 8 | §3.1 | "`FD_OOB` ou `FD_WRITE` abre `MessageBoxW`" | `FD_WRITE` não está na máscara `0x25`: ramo morto; só `FD_OOB` |
| 9 | §5 `0x12` | só o evento 32 | antes dele, estado := 0 e `0x100022D0` no socket de jogo (`closesocket`, zera `[0x1005530C]` → a *thread* termina no próximo *timeout* de 100 ms) |
| 10 | §5 `0xEC` | "(slot 223)" na coluna de eventos | slot 223 **do vtable do `UNetworkHandler`** (`0x10033798+0x37C`), não do `FL2NetNotify` (cujo 223 é `AddAcquireSkillInfo`) |
| 11 | §3.3 | cifra do login desliga "quando o socket fecha" | quem zera são os handlers de falha; `0x10002E30` não toca na flag; o `PlayOk` grava **1** na flag do login já fechado (inofensivo) |

Nenhuma das refutações muda uma letra de formato, um opcode, um handler ou um slot: as tabelas §4/§5/§6 saíram intactas da conferência.

### R4. Continua NÃO VERIFICADO (e o que o analista não examinou)

1. **Semântica dos campos** (nomes, "ignorado") e o mapa campo → `User`/`Pawn`: fora do escopo desta revisão, como o documento já avisa; a correspondência `x y z` + hora do `0x21` para os slots 135/278 foi conferida só na estrutura (3 `ftol(fild(int))` + `d` separado).
2. **Comportamento em jogo**: nada rodou. O *crash*/*hang* com cabeçalho < 2 (R3.2) e o efeito de um quadro > 16.384 B são leitura de código.
3. **A *thread* de recepção chama `GMalloc->Malloc` (duas vezes por pacote: `"NetCmd"` e `"NetPacket"`) e `AddNetworkQueue` fora da *thread* principal** (`0x1000FEF1 → [vt+0x6C] = 0x10010240 → 0x100102E0/0x1001031C`). Só a fila está sob `CRITICAL_SECTION`; se o `GMalloc` do `Core.dll` não é *thread-safe* (NÃO VERIFICADO), há uma corrida entre a *thread* de rede e o motor em todo pacote recebido. O analista não mencionou; é uma pista barata para quem investigar o lag (contar/serializar essas alocações num gancho de diagnóstico).
4. Leitor `s` sem NUL em `tamanho` bytes: `0x100029B0` devolve `buf+n` (não NULL) e o leitor avança `n+1`; nenhum handler usa `s`, continua irrelevante.
5. O `0x1000257B` (versão ≥ 363) protege **todo** o bloco do brasão de aliança (`..\AllyCrest\%d_%d.bmp` → `OnRegisterAllyCrest` ou `RequestAllyCrest`), não só o pedido; o do clã (`..\PledgeCrest\%d_%d.bmp` → `OnRegisterPledgeCrest` / `RequestPledgeCrest` `+0x190`) não tem gate de versão.
6. A nota irmã `Rede_Protocolo_Cliente_Servidor.md` (§2.3 dela) **reproduz em texto a chave Blowfish do login**; este documento não a reproduz e esta revisão também não. Fica o registro para quem mantém a outra nota.
7. Tudo o que a §10 do analista já listava (campos reservados, duas tabelas/uma fila, triagem do Lucera por regex, `ServerAddr` do `l2.ini` cifrado) continua em aberto.

### R5. Veredito

**GO_WITH_FIXES (as correções já estão aplicadas no corpo).** O que o dono vai usar para mexer no servidor — opcode, nome, handler, letras do formato, laços e condições, limites das strings, slots disparados, quadro, cifras, chaves e o handshake — foi reconstruído de forma independente e bate em 249/249 linhas e em todas as afirmações de transporte. Os erros encontrados estão nas vizinhanças (tamanho do buffer de quadro, o teste de tamanho, um campo do objeto, um "quem chama") e num item da comparação com o Mobius (`Die`), todos corrigidos no lugar. A coluna "Mobius C1" deve continuar a ser lida como dica mecânica: o único artefato de `if/else` achado foi o `Die`, mas listas com condição (`~`/`=p`) não foram recontadas por esta revisão.

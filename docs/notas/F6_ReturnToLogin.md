# Fase 6 — ReturnToLogin (volta ao login)

Porte do `src/inject/ReturnToLogin.cpp` do NWindow_C1 (HEAD `4e56326`; RE no
`docs/C1_DISCONNECT_RE.md` de lá, seções 2 a 5). Código: `src/features/ReturnToLogin.cpp`,
`include/features/ReturnToLogin.h`. Trabalhador lobby, 27/09/2026.

## O que faz (igual ao NWindow_C1)

1. **Servidor de jogo inalcançável depois da lista.**
   - O original fica parado numa lista cuja conexão de login já fechou.
   - Agora a mesma coisa que o Cancel da lista (AuthReconnect, tela de login, `SetState(2)`) e
     "Unable to connect to server." (systemmsg 90) na linha de mensagem.
   - Dispara num erro de conexão, num fechamento do socket de jogo ou depois de `TimeoutSeconds`.
2. **Caixa "desconectado" (id 0x1D, systemmsg 127).**
   - OK, ou Esc, que no original só fechava a caixa sobre um mundo morto, volta ao login em vez de
     fechar o jogo.
   - No jogo são duas partes. Primeiro a desmontagem do Restart (janelas do jogo escondidas,
     `ContinueServerClose`, os 5 caches do socket, `Browse("Lobby?L2Restart")`). Depois, com a
     seleção de personagem de pé, o "Voltar" da seleção.
   - Um servidor que só fecha a conexão (FIN sem erro) passa a mostrar essa caixa depois de 1 s.
     O original nem percebia.
- `LineageWindow.ini [Connect]`: `ReturnToLogin` (1), `DisconnectToLogin` (1), `TimeoutSeconds`
  (25, limitado a 5..120; lido sem sinal como no NWindow_C1: um valor negativo vira 120).
- Log `returntologin:` em `LineageWindow.log` com `[Window] Log=1`.

## O que virou código direto (NWindow)

| NWindow_C1 | Aqui |
|---|---|
| slot +0x444 do console trocado (`Hook_DiscOk`) | `NConsoleWnd::QuitGame` (slot 273, `NConsoleWndLogin.cpp`), bloco `#if NW_FEATURES`: com a parte 2 ligada, só pede a transição (`nwf::ReturnToLogin_QuitGame`) e não chama `SaveWindowPositions`/`appRequestExit` |
| slot +0x100 da vtable da caixa (`Hook_DlgCancel`, vtable 0x100D9510) | `NCDialogBox::OnCancel` (`NCDialogBox.cpp`): guarda o `DialogID` antes do `Reset` e avisa no fim (`nwf::ReturnToLogin_DialogCancelled`) |
| chamada no começo do porte do Tick | `NConsoleWnd::Tick` (`NConsoleWndInput.cpp`), logo depois do `guard`, antes do `UL2ConsoleWnd::Tick` (`nwf::ReturnToLogin_OnTick`) |
| rotinas do original por endereço | métodos nossos: `SaveWindowPositions`, `RestartResetWindows` (slot 142) e `RestartToLobby` (143), pela vtable como antes; `ReturnToCharacterSelect`, `ReturnToLogin`, `ShowMessageWnd` e `LoginWnd->ServerSelectWnd->OnCancel()` |
| campos do console por offset (+0x2C, +0x30, +0x44, +0x36C8, +0xA0) | campos declarados (`State`, `SavedState` novo em `UL2ConsoleWnd`, `Engine`, `LoginWnd`, `ServerSelectWnd`) |

Por que os dois pontos no código são equivalentes às trocas de slot:
- O `QuitGame` só é chamado pelo OK da caixa 0x1D (`NCDialogBox::OnOK`), como o slot +0x444 no
  original.
- Os quatro chamadores internos do `OnCancel` usam a vtable (`call [reg+0x100]`) e nenhuma classe
  deriva de `NCDialogBox`. Mexer no corpo equivale a trocar o slot da vtable 0x100D9510.

As conferências do NWindow_C1 sobre o lado NWindow (slots, primeiros bytes das rotinas, vtable da
lista de servidores) somem: é o nosso código.

## O que continua remendo em tempo de execução

| Módulo | Onde | Uso | Conferido no install |
|---|---|---|---|
| Network.dll | vtable `CClientSocket` 0x33D58, +0x64 (`OnConnect` 0x2320) e +0x68 (`OnUserNetmessage` 0x10370) | anota erro de conexão, FD_CLOSE e FIN sem erro do socket atual | os dois slots apontam para os corpos esperados; assinaturas `8B 44 24 08 83 EC 08 C1 E8 10` e `55 8B EC 6A FF 68` |
| Engine.dll | thunk ILT 0x78F6 de `UGameEngine::OnServerClose` (0x11F3E0) | anota que a caixa do original já saiu | `E9` para o corpo com `8B 49 2C 8B 41 2C 83 F8 06` |

- Só conferidos, nunca trocados:
  - Engine: slot 56 (+0xE0) da vtable `UObject` do `UGameEngine` (0x487198) → thunk 0x10B9F →
    `ContinueServerClose` 0x11F430 (`56 8B F1 8B 46 4C 8B 48 2C`);
  - Network: slot +0xC0 `ClearObjectMap` (0x218C0) e os caches 0x125E0 / 0x12800.
- Na hora da chamada, como antes:
  - o objeto engine tem de ter a vtable 0x487198, e o socket a 0x33D58;
  - o `ContinueServerClose` só roda com a cadeia `Client->Viewports(0)->Actor->Pawn` inteira;
    senão, `ClearObjectMap(0)`.
- Outro Network/Engine: a parte fica desligada, com a linha `returntologin: … off (config or other
  build)`.

## Quando entra

- No nosso `DllMain` (`nwf::OnDllMain`), como o `DllMain` do LineageWindow: o Network.dll já está
  mapeado com o WinDrv.
- Se faltar um módulo, o `Tick` tenta de novo a cada quadro, como antes. O `GIsRequestingExit`
  agora vem por import (`UE2Core.h`).

## Conferido fora do jogo

- `build/smoke_l2_patches` (passo `l2: smoke`): Network.dll e Engine.dll do cliente carregados
  antes do nosso DLL.
- Os dois slots do `CClientSocket` e o thunk do `OnServerClose` apontam para o `NWindow.dll`.
- No log: `connect watchdog on` e `disconnect -> login on`.

## Testar no jogo

1. **Servidor fora do ar.** Na lista, escolher um servidor cujo game server está parado (ou uma
   porta recusada): em cerca de 1 s volta ao login com "Unable to connect to server.".
2. **Servidor que aceita e não responde.** Deve voltar depois de 25 s (`TimeoutSeconds`).
3. **Desconexão no jogo.** Com `//server_restart` ou derrubando o servidor, OK na caixa: tela de
   carregamento, seleção, login. Entrar de novo e jogar: o mundo anterior não pode ter sobrado.
4. **Esc na mesma caixa:** o mesmo caminho.
5. **Servidor morto sem ServerClose** (processo encerrado, proxy caído): a caixa aparece sozinha
   depois de ~1 s.
6. **Na seleção de personagem:** a queda leva ao login.
7. **Com `DisconnectToLogin=0`:** OK fecha o jogo, como o original.

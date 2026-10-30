# NWindow.dll do Lineage II C1 — source reconstruída em C++

**Português** · [English](README.en.md) · [Русский](README.ru.md)

Reconstrução, função por função, do `NWindow.dll` do cliente Lineage II **Chronicle 1 (Harbingers of War)**:
a DLL nativa da interface (janelas, controles, HTML das NPCs, IME, chat, console). O alvo é o build
original de 25/06/2004 (TimeDateStamp `0x40DBD452`, Visual C++ .NET 2002, md5 `2dacb6eb…`). A mesma
source compila de três jeitos:

| Alvo CMake | Arquivo gerado | O que é |
|---|---|---|
| `NWindowDecomp` | `build/bin/Release/NWindowDecomp.dll` | **modo hook**: roda ao lado do `NWindow.dll` original e troca cada função já reimplementada por um `jmp` para a nossa |
| `NWindowStandalone` | `build/standalone/Release/NWindow.dll` | **standalone fiel**: o `NWindow.dll` inteiro, só com o nosso código; é a referência que todo teste compara com o original |
| `NWindowL2` | `build/l2/Release/NWindow.dll` | **standalone + recursos da fase 6** (o DLL que os jogadores recebem): o mesmo código com `NW_FEATURES=1` e `src/features/` |

## Como funciona

**Modo hook.** O instalador (`tools/install_test.py`) acrescenta ao `NWindow.dll` original um import da
`NWindowDecomp.dll`. Ela carrega antes de qualquer código do original rodar e põe um `jmp` na entrada de cada
função que já reimplementamos (um `NW_HOOK_*` ao lado de cada definição, `src/re/NwHook.h`). O resto continua
sendo o código original. Assim cada função nova é testada no jogo de verdade, e qualquer uma pode ser
desligada no `NWindowDecomp.ini` (`[Hooks] Disable=`) para isolar um bug.

**Standalone (fase 5).** Com `NW_HOOK_MODE=0` os macros de hook ficam vazios e a source gera o
`NWindow.dll` sozinha: as 178 `UClass` (`IMPLEMENT_CLASS`), as globais, os exports com os ordinais do
original (`src/standalone/NWindow.def`) e os 7 cursores coloridos. A engine (`Core.dll`/`Engine.dll`)
carrega o nosso DLL como carregava o original.

**Fase 6 (recursos).** Os recursos que antes eram remendos de fora (o projeto anterior `NWindow_C1`, uma
DLL injetada) viram source em `src/features/`. No código decompilado só aparece o ponto de chamada, num
bloco `#if NW_FEATURES` com o comentário `// phase 6: ...`; o modo hook e o standalone fiel não veem nada
disso. Recursos:

- **Combat log** (painel de sistema/combate sobre o chat), **BuffTimer** (tempo restante nos ícones de buff);
- **AutoShot** (soulshot/spiritshot automático, estilo C3), **ManufactureCmd** (`/dwarvenmanufacture`, ações
  37 e 52), **MinimapHotkey** (Alt+M / M), **CacheCleaner** (`FLUSH` + `OBJ GARBAGE` periódicos);
- **janelas de craft** estilo C2 (loja, livro de receitas, árvore) e o **balão de manufatura**;
- **WindowFix** (modo janela: moldura móvel, mouse da UI, filtro de clique), **ReturnToLogin** (volta à tela de
  login em vez de uma lista morta);
- **Discord Rich Presence** (fala direto com o pipe local do Discord, sem SDK);
- **ChatColors** (as cores do Interlude para command channel e hero voice), **ItemHoverTip** (dica do item
  sob o mouse em caixa escura com o símbolo do grade).

As configurações não usam arquivo: o que era `LineageWindow.ini` está compilado
(`src/features/NwFeatureConfig.cpp`) e o que o jogador muda no jogo vai para o registro,
`HKCU\Software\Lineage2C1\NWindow\<Seção>`. As proteções do cliente usadas pelo servidor do autor são um
módulo privado e **não fazem parte desta source**.

## Estado

- Modo hook: **2.744 de 3.653** funções a reimplementar estão feitas (**95,42 %** dos bytes de código), nenhuma
  pendente sem motivo; as que faltam o compilador gera no standalone (*scalar deleting destructors*,
  `InternalConstructor`, inicialização estática). **1.360** já foram testadas no jogo.
  Contagem viva: [docs/PROGRESSO.md](docs/PROGRESSO.md) (gerado por `tools/build_db.py`).
- Standalone: linka, passa nas verificações estáticas e no smoke (a saída é a do original, linha a linha) e
  foi testado no jogo.
- Fase 6: todos os recursos acima estão na source e passam no ciclo; o estado de cada um (testado no jogo
  ou não) está em [docs/notas/Fase6.md](docs/notas/Fase6.md).
- As notas técnicas (`docs/`) estão em português; os comentários de código, em inglês.

## Requisitos

- Windows, **Visual Studio 2022** com C++ para x86 (MSVC v143), **CMake ≥ 3.20**;
- **Python 3** com `pefile` e `capstone` (`pip install pefile capstone`): usado pelo build standalone (extração
  dos cursores) e por todas as ferramentas;
- **o seu próprio cliente Lineage II C1**. Nenhum arquivo do jogo vem no repositório: copie para `orig/` os
  arquivos listados em [orig/README.md](orig/README.md) (o `NWindow.dll` tem de ter o md5 de lá);
- para os juízes do ciclo completo (`verify_hooks.py`, `regcheck.py`, `verify_standalone.py`,
  `build_db.py`): o export do Ghidra em `analysis/ghidra/`, recriado por `tools/ghidra/run_export.sh`
  (Ghidra 11.3.2 e JDK 21; caminhos nas variáveis `GHIDRA` e `GHIDRA_JAVA_HOME`).

## Compilar

Num *Developer Command Prompt* x86 do VS 2022 (ou depois de `vcvars32.bat`), na raiz do repositório:

```bat
cmake -S . -B build -A Win32
cmake --build build --config Release
cmake --build build --config Release --target NWindowStandalone
cmake --build build --config Release --target NWindowL2
```

O primeiro `--build` gera o DLL do modo hook e as ferramentas de teste (`nwd_smoke`, `nwd_difftest`,
`nwd_craft_test`) e não precisa de `orig/`. `NWindowStandalone` e `NWindowL2` precisam de
`orig/NWindow.dll`. Opção: `-DNW_FEATURE_LOG=ON` faz o `NWindowL2` escrever `LineageWindow.log` (só para
diagnóstico; o build normal não escreve arquivo nenhum).

## Instalar e testar

Use uma **cópia** da pasta `System` do cliente, nunca a que você usa para jogar.

**Modo hook:**

```bat
python tools\install_test.py --system "C:\...\Lineage II\System - Teste"
python tools\install_test.py --system "C:\...\Lineage II\System - Teste" --restore
```

O primeiro grava um `NWindow.dll` refeito a partir de `orig/NWindow.dll` com o import a mais, a
`NWindowDecomp.dll` e um `NWindowDecomp.ini` (o `NWindow.dll` que estava lá fica como
`NWindow.dll.pre-decomp-bak`); o segundo devolve o que estava lá. O `NWindowDecomp.log` dessa pasta lista cada
hook instalado. No `.ini`: `[Hooks] Enabled=0` deixa todo o código original; `Disable=` aceita um endereço
(`1000cc40`), um método (`NCGDevice::DrawText`) ou uma classe (`NCGDevice::*`); `[Debug] Marker=1` desenha
marcas que só o nosso código desenha (linha amarela no topo, `#` na senha), para conferir que os hooks estão
ativos.

**Standalone / fase 6:** feche o jogo, guarde o `NWindow.dll` da pasta de teste e copie por cima dele o
`build\standalone\Release\NWindow.dll` (fiel) ou o `build\l2\Release\NWindow.dll` (com os recursos). Só esse
arquivo: nenhum `.ini` nem `.log` ao lado. Para voltar, ponha o original de volta.

**Ciclo completo** (compila, smoke, juízes, standalone, L2, instala, atualiza o banco; para no primeiro passo
que falhar):

```bat
python tools\cycle.py --system "C:\...\Lineage II\System - Teste"
python tools\cycle.py --system "C:\...\Lineage II\System - Teste" --no-install
```

Passe `--system`, ou defina `NWD_CLIENT_SYSTEM` (abaixo). O ciclo usa
`build/` e copia dessa pasta as DLLs que o carregador precisa (`Core.dll`, `Engine.dll` e as suas
dependências). O jogo dessa pasta tem de estar fechado para o passo de instalação.

**Caminhos da máquina.** Nenhuma ferramenta tem um caminho da máquina do autor: uma pasta fora do repositório vem
da opção de linha de comando ou de uma destas variáveis (a opção vence; a ferramenta que precisa de uma e não tem
nenhuma para e diz qual):

| Variável | Aponta para | Usada por |
|---|---|---|
| `NWD_CLIENT_SYSTEM` | a pasta `System` do cliente de teste (uma cópia: a instalação escreve nela) | `--system` padrão de `install_test.py`, `cycle.py`, `smoke_standalone.py`, `difftest.py`; `verify_hooks.py --log` |
| `NWD_C1_SERVER` | a pasta de deploy do servidor (páginas em `gameserver/data/html-en`) | `difftest.py`, conjunto de páginas `server` (pulado sem ela) |
| `NWD_GHIDRA_HOME` | a instalação do Ghidra (com `support/analyzeHeadless.bat`; `GHIDRA` ainda tem precedência) | `tools/ghidra/*.sh`, `tools/c2/ghidra_census.sh` |

## Os juízes

O ciclo (`tools/cycle.py`) roda todos, nesta ordem; "verde" é o ciclo terminar com código 0.

| Juiz | Comando (o ciclo acrescenta o seu `--system` onde a ferramenta o aceita) | O que prova |
|---|---|---|
| smoke | `build\smoke\Release\nwd_smoke.exe build\smoke_system` | o `NWindow.dll` remendado (instalado em `build/smoke_system`) carrega fora do jogo e todo hook dá `ok` (`NWindowDecomp.log`) |
| verify | `python tools/verify_hooks.py --log build/smoke_system/NWindowDecomp.log` | cada função nossa contra a original no assembly: bytes desempilhados no `ret`, slots de vtable e imports chamados; diferença aceita só com explicação em `db/verify_notes.csv` |
| regcheck | `python tools/regcheck.py` | nenhum chamador original depende de um registrador que a nossa função pode mudar (o original foi compilado com `/GL`) |
| difftest | `python tools/difftest.py --golden` | pedaços do DLL rodam fora do jogo e escrevem o mesmo que o original (`tests/golden/`) |
| standalone: declarações | `python tools/verify_standalone.py --checks source` | os nossos `NW_DECLARE_CLASS` / `NW_CHECK_VTABLE` contra a tabela de classes original |
| standalone: verify | `python tools/verify_standalone.py --dll build/standalone/Release/NWindow.dll` | PE, recursos, exports, classes e vtables iguais aos do original, sem rodar |
| standalone: difftest | `python tools/difftest.py --standalone build/standalone/Release/NWindow.dll` | os mesmos casos do difftest no DLL standalone |
| standalone: smoke | `python tools/smoke_standalone.py --dll build/standalone/Release/NWindow.dll` | carregado como a engine carrega: 178 classes registradas, `ConsoleWnd`/`L2Font` construídos, saída igual à do original |
| l2: craft data | `build\craft\Release\nwd_craft_test.exe` | o parser do protocolo das janelas de craft |
| l2: discord presence | `python tools/presence/presence_test.py` | o Rich Presence contra um Discord falso |
| l2: verify / smoke | `python tools/verify_standalone.py --dll build/l2/Release/NWindow.dll --l2` e `python tools/smoke_standalone.py --dll build/l2/Release/NWindow.dll --l2` | o DLL da fase 6 passa nos mesmos testes, com exatamente as classes extras de `tests/standalone/l2_extras.csv` |
| db | `python tools/build_db.py` | regenera `db/functions.csv` e `docs/PROGRESSO.md` |

## Pastas

```
orig/          binários do seu cliente (fora do git; md5 em orig/README.md)
analysis/      fatos extraídos do original (classes, EH, vtables; as vtables exportadas do Core/Engine); analysis/ghidra/ = export do Ghidra (gerado, fora do git)
db/            banco de funções (functions.csv gerado) + nomes, notas e verificações dados à mão
include/ue2/   declarações do Core.dll/Engine.dll do C1, conferidas contra os exports
include/nwindow/, src/nwindow/    a source decompilada, uma classe por arquivo
src/re/        runtime do modo hook (instalação dos hooks, log, ini, compatibilidade do CRT)
src/standalone/                   IMPLEMENT_CLASS / IMPLEMENT_PACKAGE, globais, NWindow.def
include/features/, src/features/  fase 6: os recursos
tests/         saídas de referência do original (golden) e listas do standalone
third_party/ue_import/            .def dos exports do Core/Engine (viram .lib no build)
tools/         análise, geração, instalação, juízes, ciclo
docs/          plano, convenções, progresso e notas técnicas (português)
```

## Guias

- [docs/PLANO.md](docs/PLANO.md) — plano, método e fases; [docs/CONVENCOES.md](docs/CONVENCOES.md) — regras de código e armadilhas
- [docs/MODIFICAR_INTERFACE.md](docs/MODIFICAR_INTERFACE.md) — como modificar a interface (barra de atalhos, botão, janela, cor de chat, linha de tooltip)
  · [English](docs/MODIFY_INTERFACE.md) · [Русский](docs/MODIFY_INTERFACE.ru.md)
- [docs/LLM.md](docs/LLM.md) — guia para agentes de IA que forem trabalhar neste código (em inglês)
- [docs/notas/Standalone.md](docs/notas/Standalone.md) — a fase 5; [docs/notas/Fase6.md](docs/notas/Fase6.md) — a fase 6 e cada recurso (`docs/notas/F6_*.md`)

## Nota legal

Projeto independente de engenharia reversa, para compatibilidade e preservação. Não é afiliado, autorizado nem
endossado pela NCSoft. *Lineage II* é marca registrada da NCSoft Corporation. **Nenhum arquivo original do
jogo está incluído** (nem DLLs, nem executáveis, nem pacotes de dados): cada usuário fornece o próprio cliente.

## Licença

[GNU General Public License v3.0](LICENSE).

# Binários de referência (fora do git) / Reference binaries (not in git)

**PT.** Esta pasta não vem com nenhum arquivo do jogo: nenhum binário da NCSoft é publicado. Copie os
arquivos abaixo do **seu próprio** cliente Lineage II C1 (pasta `System`) para cá. O build do modo
hook (`NWindowDecomp`) não precisa deles; o `NWindowStandalone` e o `NWindowL2` precisam de
`orig/NWindow.dll` (os 7 cursores coloridos são extraídos dele na compilação); as ferramentas de
verificação precisam do `NWindow.dll`, do `Core.dll` e do `Engine.dll`. Todo `NWindow.dll` tem de ter o
md5 da tabela.

**EN.** This folder ships without any game file: no NCSoft binary is published. Copy the files below
from **your own** Lineage II C1 client (its `System` folder) into it. The hook-mode build
(`NWindowDecomp`) does not need them; `NWindowStandalone` and `NWindowL2` need `orig/NWindow.dll` (the 7
colour cursors are extracted from it at build time); the verification tools need `NWindow.dll`,
`Core.dll` and `Engine.dll`. The `NWindow.dll` must have the md5 in the table.

| Arquivo | md5 | Origem |
|---|---|---|
| `NWindow.dll` | `2dacb6ebcbb3bbe22cec2b296231dc08` | stock C1 (TimeDateStamp `0x40DBD452`, 25/06/2004), cópia de `Lineage II - Chronicle 1\CUSTOMIZACOES\NWindow_ori.dll` |
| `Core.dll` | `c4397d53b3bb256219612405e7daf534` | a que roda no cliente (`System - Copia`), modificada em 13/07/2026 |
| `Core_stock2017.dll` | `d491f67e6261eab9d7b20f777f506575` | a original (`System_C1_Win10`) |
| `Engine.dll` | `e705066c9c65238a7587ef10510a5a5a` | stock |
| `Window.dll`, `WinDrv.dll`, `L2.exe` | ver `md5sum` | do cliente em uso, só para consulta |
| `Network.dll` | `f6d590082999cf527b1505841a070a13` | do cliente em uso: o `UNetworkHandler` exportado dá nome e assinatura às virtuais puras do `UL2NetHandler` (`tools/gen_engine_decls.py`) |
| `ALAudio.dll` | `876d12b78501845705bcf4eb8dd3336a` | idem: o `UALAudioSubsystem`, para as puras do `UAudioSubsystem` |
| `D3DDrv.dll` | `7a1f5e8195f33a4be1b37be1ccc33954` | idem: o `UD3DRenderDevice`, para as puras do `URenderDevice` (as do `UViewport` vêm do `UWindowsViewport` do `WinDrv.dll`, `fc54af74…`) |

`NWindow.dll` é **o** binário que o projeto reconstrói: toda ferramenta confere o md5 dele
(`tools/nwd/image.py: STOCK_MD5`). Outra build (ex.: a de 19/01/2004 em
`Client_C1_teste\system_stock_2004_backup`, md5 `1bf0eb04...`) não serve.

Para recriar a pasta:

```
copy "..\Lineage II - Chronicle 1\CUSTOMIZACOES\NWindow_ori.dll" orig\NWindow.dll
copy "..\Lineage II - Chronicle 1\System - Copia\Core.dll"   orig\
copy "..\Lineage II - Chronicle 1\System - Copia\Engine.dll" orig\
copy "..\Lineage II - Chronicle 1\System - Copia\WinDrv.dll" orig\
copy "..\Lineage II - Chronicle 1\System - Copia\Network.dll" orig\
copy "..\Lineage II - Chronicle 1\System - Copia\ALAudio.dll" orig\
copy "..\Lineage II - Chronicle 1\System - Copia\D3DDrv.dll" orig\
```

Os quatro últimos só servem para regenerar `include/ue2/gen/*.vt.inc`: sem eles o
`tools/gen_engine_decls.py` não roda, e o `tools/engine_vtables.py` lê só o Core e o Engine.

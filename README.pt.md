# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · **Português** · [Français](README.fr.md) · [Deutsch](README.de.md) · [Русский](README.ru.md)

Código-fonte, obtido por engenharia reversa, do motor de **Call of Duty 2**: o cliente multijogador e o servidor dedicado, para Linux e Windows.

Este repositório é um fork de [opencod2/opencod2](https://github.com/opencod2/opencod2), onde foi feita a reconstrução do motor. Também traz alterações de [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). Aqui só são acrescentadas as alterações descritas em [Alterações deste fork](#alterações-deste-fork). O seu objetivo é tornar-se um único repositório com o cliente e o servidor dedicado, trazendo o servidor do [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) módulo a módulo.

> [!WARNING]
> **Trabalho em curso — isto ainda não funciona por completo.** O cliente arranca até ao menu principal e o servidor dedicado, agora o do CoD2rev-Plus, carrega mapas e mantém jogadores na 1.0 e na 1.3, mas espere falhas, funcionalidades em falta e outras que não funcionam. É partilhado para os curiosos e para colaboração, não como um port acabado ou jogável.

> [!IMPORTANT]
> **Nenhum conteúdo do jogo está incluído — traga os seus próprios dados.** Este repositório contém apenas código-fonte: nenhum executável, arquivo, mapa, modelo, textura, som ou script. Para executar qualquer coisa, é preciso fornecer ficheiros de dados de uma cópia do jogo que **possua legalmente**.

## Transparência

- **Quem mantém isto:** sou o JotaJazz. Gosto de jogos antigos e este fork existe por isso: quero continuar a jogar CoD2 com os meus amigos e manter o jogo vivo para a comunidade que ainda o joga, com um projeto aberto.
- **Como se trabalha aqui:** trabalho com o [Claude Code](https://claude.com/claude-code), o assistente de programação da Anthropic. As alterações deste fork, incluindo este README e as suas traduções, são escritas com a sua ajuda. Os commits feitos assim têm a linha `Co-Authored-By: Claude` na mensagem, pelo que podem ser identificados no histórico com `git log`.
- **Relação com o original:** a reconstrução é obra dos autores do opencod2 (riicchhaarrd) e do Sipex. Este fork já não traz alterações deles; em vez disso, traz código do CoD2rev-Plus, copiado sem alterações para que os seus commits também se apliquem aqui.
- **Licença:** o repositório original não tem licença, e este também ainda não tem. O CoD2rev-Plus, cuja VM de scripts, cujo servidor, cujo game e cujo bgame já estão aqui (`src/script`, `src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`), é AGPL-3.0. A licença do conjunto está a ser resolvida com os autores originais antes de publicar aqui mais desse código.
- **O que não é:** não é um projeto oficial nem está associado à Activision, à Infinity Ward ou aos autores do repositório original. Não inclui nenhum ficheiro do jogo.

## Alterações deste fork

- **Compilações que voltam a funcionar:** as compilações para Linux e MinGW, que deixaram de compilar desde que o original passou a ser compilado só com MSVC, voltam a compilar; `scripts/build_check.py` compila Linux 1.3, Linux 1.0 e MinGW de uma só vez.
- **Compilado como C++:** no Linux e no MinGW todo o motor compila como C++ (gnu++17), com os cabeçalhos partilhados tornados C++ válido, sem mudar o que faz; verificações protegem os nomes C de que dependem os blobs de dados e a disposição das estruturas do binário original.
- **A VM de scripts do CoD2rev-Plus:** a máquina virtual GSC é a do CoD2rev-Plus, copiada sem alterações para `src/script`, com uma ponte C/C++ gerada. O servidor mantém agora dois jogadores, desligações, novas ligações e `clientkick`, onde a VM reconstruída falhava assim que o segundo jogador entrava.
- **O servidor do CoD2rev-Plus:** o servidor dedicado é compilado com o servidor do CoD2rev-Plus, copiado sem alterações para `src/server`, com uma ponte gerada em que cada chamada entre os dois é verificada contra a ABI do motor. O cliente mantém o servidor reconstruído (`src/PC/server_mp`) para os seus jogos locais. Com ele, o servidor recusa-se a enviar os ficheiros `.iwd` originais e os que são só do servidor. Com ele vêm o seu game e o seu bgame (`src/game`, `src/bgame`), e com eles o `addip` e a rotação do `games_mp.log`; o cliente mantém também `src/PC/game_mp` e `src/PC/bgame`. Desde então o servidor dedicado corre também sobre o seu núcleo: `universal`, `qcommon` e o código de plataforma (`src/universal`, `src/qcommon`, `src/unix`, `src/win32`), que trazem a rotação e as marcas de tempo do registo da consola, o `net_strictPort` e o seu relatório de falhas.
- **Um servidor dedicado sem as bibliotecas do cliente:** no Linux o `cod2_lnxded` precisa apenas da libc, da libm, da libz e da libstdc++, não do GL, do X11, do SDL2 nem do curl.
- **Correções:** o servidor respondia na porta errada, não via nenhum `.iwd` em unidades do Windows no WSL e falhava em qualquer kick; anunciava o protocolo 118 na compilação 1.0; a compilação C++ para Linux continua na glibc 2.34; `getAnimLength` e outras três funções de animação já não derrubam o servidor. O quarto ou quinto carregamento de mapa derrubava o servidor: o script de animações crescia a cada mapa, e o erro que lançava a partir do rcon corrompia a pilha. O servidor 1.0 escrevia por cima das suas próprias configstrings e nenhum cliente 1.0 conseguia entrar. As mensagens fragmentadas usavam os offsets errados nas duas versões, e a porta de um endereço de rede estava onde o Quake III a guarda, não onde o CoD2 a guarda. Os ficheiros `.iwd` da própria pasta do servidor não eram procurados, e as funções que recebem um jogador não o viam com a VM do CoD2rev-Plus. As entidades eram ligadas aos setores do mundo com o número errado, por isso desligar algumas derrubava o servidor e os traces testavam a entidade anterior.
- **Testes:** o smoke test do CoD2rev-Plus, sem alterações, contra este servidor, com a lista das verificações que ainda não passa; testes com dois jogadores, de carregamentos de mapa, de um erro drop a partir do rcon e das funções de animação; e verificações da disposição das estruturas, dos nomes C, da ABI da ponte e dos ficheiros copiados do CoD2rev-Plus.

Cada alteração em detalhe está em [docs/CHANGES.md](docs/CHANGES.md) (em inglês).

## Versões do jogo

- **A versão é escolhida ao compilar.** Por predefinição o motor é compilado como 1.3 (protocolo 118); com `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` é compilado como 1.0 (protocolo 115).
- **Os dois servidores dedicados funcionam para testes:** na 1.3 e na 1.0 o servidor carrega, muda e reinicia mapas, responde a `getinfo`, `getstatus` e rcon, e mantém dois jogadores que entram, saem, voltam e são expulsos.
- **O que o CoD2rev-Plus tem no libcod ainda não está aqui:** as suas funções de script, os bots e `sv_master1`…`sv_master5`, por isso o servidor envia o seu heartbeat só para o servidor master da Activision.

## Compilar

Todos os alvos são geridos pelo **CMake (≥ 3.16)**, cada um numa compilação fora da árvore de código, na sua própria pasta.

### Linux

O motor é um binário x86 de 32 bits. Num sistema Debian ou Ubuntu de 64 bits são necessárias a arquitetura e as bibliotecas de 32 bits:

```sh
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib cmake \
     libsdl2-dev:i386 libgl1-mesa-dev:i386 libx11-dev:i386 \
     libcurl4-openssl-dev:i386 zlib1g-dev:i386 libstdc++6:i386
```

```sh
cmake -S . -B build-native
cmake --build build-native -j
# -> build-native/cod2_linux    (client)
# -> build-native/cod2_lnxded   (dedicated server)
```

As bibliotecas do cliente são precisas para compilar os dois, mas o `cod2_lnxded` executa apenas com a libc, a libm, a libz e a libstdc++ de 32 bits.

### Windows (cruzado a partir do Linux com MinGW)

Precisa dos compiladores MinGW-w64 de 32 bits (`g++-mingw-w64-i686`) e da zlib estática para MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Acrescente `-DCOD2_WIN32_CLIENT=ON` para o cliente SDL2/GL (forneça as bibliotecas estáticas do SDL2 em `src/win32/sdl2/lib/`; o servidor dedicado não precisa de nenhuma).

#### DLL de renderizador substituível (opcional, experimental)

O renderizador pode ser compilado como uma DLL separada e substituível: exporta `GetRefAPI` e comunica com o motor apenas através das tabelas `ri`/`re`. Com o cliente configurado, execute o alvo `gfxdll`:

```sh
cmake --build build-win32 --target gfxdll
# -> build-win32/gfx_d3d_mp_x86_s.dll   (renderer DLL)
# -> build-win32/cod2_win32_gfxdll.exe  (engine that loads it at runtime)
```

A ponte entre o renderizador e o motor está pré-gerada e incluída no repositório em `build/gfxdll/`; a compilação é apenas compilar e ligar. Experimental e não testada exaustivamente.

### Windows (MSVC, nativo)

Uma compilação nativa do cliente SDL2/GL completo com o Visual Studio 2022 ou mais recente e o seu conjunto de ferramentas **x86**. Configure e compile a partir de uma linha de comandos "x86 Native Tools for VS":

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

Esta compilação ainda compila C e usa a VM de scripts reconstruída (`src/PC/script`). O SDL2 é fornecido por si: coloque o pacote de desenvolvimento SDL2 para MSVC de 32 bits em `third_party/SDL2-<version>/` e copie `SDL2.dll` para junto do executável. Com `-DCOD2_SDL2_STATIC=ON` liga um SDL2 estático compilado por si (`SDL2-static.lib`, com a mesma configuração que o motor) e o runtime C estático, num único executável que não precisa de nenhuma DLL; compile o SDL2 com `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. A Release aplica `/O2` a código fiel ao descompilador, menos testado que a compilação Debug: se algo se comportar mal apenas na Release, suspeite do otimizador.

### Opções do CMake

| Opção | Predefinição | Descrição |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compila o motor como C++ no Linux e no MinGW. `OFF` compila C. |
| `COD2_CXX_DIRS` | `""` | Com `COD2_CXX_ALL=OFF`, os módulos ou ficheiros de `src/` que continuam a ser compilados como C++. |
| `COD2_PLUS_SCRIPT` | `ON` | A VM de scripts do CoD2rev-Plus (`src/script`) no Linux e no MinGW. `OFF` usa a reconstruída (`src/PC/script`), como fazem o MSVC, a compilação web e o x64. |
| `COD2_PLUS_SERVER` | `ON` | O servidor, o game, o bgame e o núcleo do CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/unix` ou `src/win32`) no servidor dedicado, no Linux e no MinGW; precisa de `COD2_PLUS_SCRIPT`. `OFF` usa os reconstruídos (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`), que o cliente usa sempre. |
| `COD2_FEATURE_CFLAGS` | `""` | Opções de funcionalidades; `-DCOD2_PATCH_10` compila a versão 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: o cliente SDL2/GL em vez do servidor dedicado. |
| `COD2_WIN_MSVC` | `OFF` | Compilação nativa com MSVC (definida pelos presets `msvc-client`). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: um único executável autónomo com SDL2 e runtime C estáticos. |
| `COD2_DEBUG` | `OFF` | `-O0` e sem hardening, para depuração. |
| `COD2_X64` | `OFF` | Alvo x86_64; um port em curso que compila, mas ainda não executa. |

Existe também uma compilação WebAssembly (`emcmake cmake -S . -B build/web`, com o Emscripten SDK), descrita no início de `CMakeLists.txt`.

## Executar

O motor precisa dos ficheiros do jogo original (a pasta `main` com os seus `.iwd`), que não estão neste repositório. É preciso ter uma cópia própria de Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

No Windows:

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` mantém o servidor na rede local; é um servidor de testes, ainda não um para colocar na lista pública.

## Testes

As verificações precisam da compilação e, as que iniciam o servidor, de uma pasta do jogo com os `.iwd` originais. Iniciam o servidor em modo LAN com uma pasta de utilizador temporária, pelo que a pasta do jogo só é lida.

```bash
python3 scripts/build_check.py all                      # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/map_reload_test.py /path/to/CoD2 --protocol 118
python3 scripts/rcon_drop_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` executa o `scripts/smoke_test.py` do CoD2rev-Plus, sem alterações, e compara as suas falhas com `test/smoke_expected_failures_<protocol>.txt`: as verificações que este servidor ainda não passa. Falha perante uma falha nova e perante uma da lista que agora passa, pelo que as listas só diminuem. `--protocol 115` testa a compilação 1.0.
- `two_players_test.py` liga dois clientes em tdm, faz com que enviem comandos, desliga um e volta a ligá-lo, e expulsa o outro; o servidor, com `developer 1` para que um erro de script pare o mapa, deve responder após cada passo.
- `map_reload_test.py` carrega mapas por rcon, alternando `map_restart` e `map` entre dois mapas, e verifica cada carregamento; `rcon_drop_test.py` lança um erro drop a partir do rcon (`error drop`) e verifica que o servidor continua de pé.
- `anim_builtins_test.py` executa `getAnimLength`, `animHasNotetrack`, `getMoveDelta` e `getAngleDelta` a partir do script de nível de um mod de teste.
- `scripts/cxx_errors.py` conta os erros de C++ de um módulo (`--return-type` encontra funções não void que podem terminar sem return, o que é indefinido em C++); `scripts/gen_script_bridge.py --module script|server` regenera a ponte para a VM de scripts ou para o servidor, e `scripts/engine_abi.py --module server` regista, com o gdb, a ABI do motor contra a qual a ponte verifica (`test/server_abi.txt`).

## Estrutura do repositório

| Caminho | Conteúdo |
| --- | --- |
| `src/PC` | O motor reconstruído: cliente (`client_mp`, `cgame_mp`, `ui_mp`), servidor (`server_mp`), jogo (`game_mp`, `bgame`), renderizador (`gfx_d3d`), código comum (`qcommon`, `universal`), animações (`xanim`) e a VM de scripts reconstruída (`script`). |
| `src/Mac` | A camada de plataforma do port para Mac de onde veio a reconstrução: OpenGL por trás de uma interface Direct3D, entrada e som. |
| `src/script` | A VM de scripts do CoD2rev-Plus, copiada sem alterações, com os cabeçalhos de que precisa em `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` e `src/libcod`. |
| `src/server` | O servidor do CoD2rev-Plus, copiado sem alterações; é usado pelo servidor dedicado. |
| `src/game`, `src/bgame` | O game e o bgame do CoD2rev-Plus, copiados sem alterações; são usados pelo servidor dedicado. |
| `src/universal`, `src/qcommon`, `src/unix`, `src/win32` | O núcleo e o código de plataforma do CoD2rev-Plus, copiados sem alterações; o servidor dedicado corre sobre eles. `test/plus_replaces.txt` lista as fontes do motor que eles substituem lá. |
| `src/compat` | As pontes entre os nomes C do motor e a VM C++ e o servidor do CoD2rev-Plus; na maior parte geradas. |
| `src/headers`, `src/imports` | Tipos partilhados, offsets verificados contra o binário original e declarações de funções importadas. |
| `src/blobs`, `build/native_gen`, `build/win_gen`, `build/gfxdll` | Dados tirados do binário original, ainda usados pelo motor. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Código de plataforma e stubs. |
| `scripts` | Scripts de compilação e de testes, e o cliente de testes do CoD2rev-Plus. |
| `test` | Testes unitários e as listas que as verificações leem. |
| `docs` | [CHANGES.md](docs/CHANGES.md) e o banner. |

## Aviso

Esta é uma reconstrução independente, ao nível do código-fonte, do motor de Call of Duty 2. Não está associada, autorizada, patrocinada nem endossada pela Activision Publishing, Inc., pela Infinity Ward ou por qualquer uma das suas afiliadas.

"Call of Duty" e "Call of Duty 2" são marcas registadas da Activision Publishing, Inc. São usadas neste repositório apenas para identificação e interoperabilidade, para descrever o que o código reconstrói. Não é reivindicado nenhum direito sobre essas marcas.

Este projeto não fornece dados de jogo protegidos por direitos de autor nem ajuda a obtê-los. Para usá-lo com conteúdo real do jogo, é preciso fornecer ficheiros de dados de uma cópia de Call of Duty 2 que possua legalmente.

O código-fonte do motor reconstruído é uma obra derivada criada para fins de preservação, interoperabilidade, investigação e educação. É fornecido no estado em que se encontra, sem garantia de qualquer tipo, expressa ou implícita.

Se é titular de direitos e acredita que algo aqui não deveria ser distribuído, abra uma issue ou contacte o responsável pela manutenção, e o assunto será tratado prontamente.

## Créditos

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), a reconstrução do motor
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), robustez e correções de rede
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) e [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), o servidor dedicado e a VM de scripts
- id Software
- Activision / Infinity Ward

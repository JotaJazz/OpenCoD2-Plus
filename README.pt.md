# OpenCoD2-Plus

<p align="center"><img src="docs/banner.svg" alt="OpenCoD2-Plus" width="100%"></p>

![Status: in development](https://img.shields.io/badge/status-in%20development-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · **Português** · [Français](README.fr.md) · [Deutsch](README.de.md) · [Русский](README.ru.md)

**O Call of Duty 2 continua vivo.** O OpenCoD2-Plus é o motor multijogador de Call of Duty 2 reconstruído a partir do seu código-fonte: o cliente e o servidor dedicado, para Windows e Linux, feito por pessoas que ainda jogam o jogo e o continuam a melhorar.

<table>
  <tr>
    <td><img src="docs/images/carentan.jpg" alt="Carentan: o piano em frente ao café Jeanne d'Arc"></td>
    <td><img src="docs/images/toujane.jpg" alt="Toujane: o canhão antiaéreo e a torre da mesquita"></td>
  </tr>
  <tr>
    <td colspan="2"><img src="docs/images/scoreboard.jpg" alt="O placar de um jogo local com bots"></td>
  </tr>
</table>
<p align="center"><sub>Carentan, Toujane e um jogo local com bots, desenhados pelo cliente Linux com os shaders do próprio jogo.</sub></p>

## O que já funciona

- **Jogar online em servidores públicos 1.3** com o cliente Windows, com a iluminação original: Direct3D 9 e os shaders do próprio jogo.
- **Criar um jogo a partir do cliente**, para os amigos entrarem, com bots se quiser (libcod).
- **Um servidor dedicado para 1.3 e 1.0**, assente no [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus): no Linux passa todo o teste de fumo do CoD2rev-Plus.
- **Um cliente Linux**, e outro sobre Vulkan (DXVK) que também desenha com os shaders do próprio jogo.
- **Verificado contra os binários originais da 1.3**: previsão de movimento, efeitos, sons, recuo, menus e muitas correções ([o que mudou](docs/changes.md)).

> [!NOTE]
> **Ainda em desenvolvimento.** Espere falhas e partes em falta, e por favor reporte-as nas [issues](https://github.com/JotaJazz/opencod2-plus/issues).

> [!IMPORTANT]
> **Nenhum conteúdo do jogo está incluído — traga os seus próprios dados.** Este repositório contém apenas código-fonte: nenhum executável, arquivo, mapa, modelo, textura, som ou script. Para executar qualquer coisa, é preciso fornecer ficheiros de dados de uma cópia do jogo que **possua legalmente**.

## Como jogar

As compilações prontas a usar serão publicadas na página de **Releases**. Até lá, compile o cliente (ver [Compilar](#compilar)) e aponte-o para a sua pasta do Call of Duty 2, a que tem `main/`:

```bat
cod2_win32.exe +set fs_basepath "C:\path\to\CoD2" +connect IP:port
```

```sh
./cod2_linux +set fs_basepath /path/to/CoD2 +connect IP:port
./cod2_linux_vk +set fs_basepath /path/to/CoD2 +connect IP:port   # with the game's own shaders (Vulkan)
```

Sem `+connect` o cliente abre o menu principal, onde também pode iniciar o seu próprio jogo. Um servidor dedicado para a sua rede local:

```sh
./cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

## Para colaboradores

Toda a ajuda conta: jogar e reportar falhas, testar noutros sistemas, ou código. A documentação técnica, em inglês, começa em [docs/README.md](docs/README.md); o [AGENTS.md](AGENTS.md) é o ponto de entrada para agentes de programação com IA.

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

O `cod2_linux` desenha com shaders provisórios (sem sol, céu branco). Para os shaders do próprio jogo, compile o `cod2_linux_vk`: `scripts/build_linux_vk_deps.sh` compila o DXVK native e o vkd3d-shader (32 bits, de versões fixadas; também precisa de `meson`, `ninja` e `libvulkan1:i386`), e depois configure com `-DCOD2_LINUX_DXVK=ON` num diretório de compilação próprio. Precisa de um dispositivo Vulkan.

### Windows (cruzado a partir do Linux com MinGW)

Precisa dos compiladores MinGW-w64 de 32 bits (`g++-mingw-w64-i686`) e da zlib estática para MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Acrescente `-DCOD2_WIN32_CLIENT=ON` para o cliente, que desenha com Direct3D 9 e os shaders do jogo (`-DCOD2_WIN32_REAL_D3D9=OFF`: o dispositivo GL) (forneça as bibliotecas estáticas do SDL2 em `src/win32/sdl2/lib/`; o servidor dedicado não precisa de nenhuma). Como o cliente Linux, corre os seus jogos locais sobre o servidor, o game e o bgame do CoD2rev-Plus, e traça com a sua colisão. Sem Windows, `scripts/portable_wine.sh DIR` instala um Wine de 32 bits sem root (Debian ou Ubuntu com i386 ativado) e escreve `DIR/wine.sh` para o executar; `COD2_CONSOLE_STDOUT=1` copia a consola para stdout, e `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` faz com que crie um jogo onde entra o cliente Linux.

<details>
<summary><b>Windows (MSVC, nativo)</b></summary>

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

</details>

<details>
<summary><b>Opções do CMake</b></summary>

| Opção | Predefinição | Descrição |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compila o motor como C++ no Linux e no MinGW. `OFF` compila C. |
| `COD2_CXX_DIRS` | `""` | Com `COD2_CXX_ALL=OFF`, os módulos ou ficheiros de `src/` que continuam a ser compilados como C++. |
| `COD2_PLUS_SCRIPT` | `ON` | A VM de scripts do CoD2rev-Plus (`src/script`) no Linux e no MinGW. `OFF` usa a reconstruída (`src/PC/script`), como fazem o MSVC, a compilação web e o x64. |
| `COD2_PLUS_SERVER` | `ON` | O servidor, o game, o bgame e o núcleo do CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` ou `src/win32`) no servidor dedicado, no Linux e no MinGW; precisa de `COD2_PLUS_SCRIPT`. `OFF` usa os reconstruídos (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | O servidor, o game, o bgame e a colisão do CoD2rev-Plus para os jogos locais do cliente (Linux, e MinGW com `COD2_WIN32_CLIENT`), sobre o núcleo do motor; precisa de `COD2_PLUS_SERVER`. `OFF` mantém `src/PC/server_mp`, como ainda fazem o MSVC, a compilação web e o x64. |
| `COD2_PLUS_LIBCOD` | `ON` | A libcod do CoD2rev-Plus (`src/libcod`) no servidor dedicado e, com `COD2_PLUS_CLIENT_SERVER`, nos jogos locais do cliente, com os seus módulos exceto o MySQL; precisa de `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Opções de funcionalidades; `-DCOD2_PATCH_10` compila a versão 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: o cliente em vez do servidor dedicado. |
| `COD2_WIN32_REAL_D3D9` | `ON` | Cliente MinGW: Direct3D 9 real e os shaders do jogo (d3dx9 do sistema); `OFF` mantém o dispositivo GL. |
| `COD2_WIN_MSVC` | `OFF` | Compilação nativa com MSVC (definida pelos presets `msvc-client`). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: um único executável autónomo com SDL2 e runtime C estáticos. |
| `COD2_DEBUG` | `OFF` | `-O0` e sem hardening, para depuração. |
| `COD2_X64` | `OFF` | Alvo x86_64; um port em curso que compila, mas ainda não executa. |

Existe também uma compilação WebAssembly (`emcmake cmake -S . -B build/web`, com o Emscripten SDK), descrita no início de `CMakeLists.txt`.

</details>

## Testes

<details>
<summary><b>Os scripts de testes</b></summary>

As verificações precisam da compilação e, as que iniciam o servidor, de uma pasta do jogo com os `.iwd` originais. Iniciam o servidor em modo LAN com uma pasta de utilizador temporária, pelo que a pasta do jogo só é lida.

```bash
python3 scripts/build_check.py all                      # every variant (linux, linux-10, mingw, mingw-client...) into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/check_c_decls.py build/check-linux     # cada chamada do motor corresponde aos tipos da sua definição
python3 scripts/check_var_sizes.py build/check-linux    # nenhuma global é usada maior do que é definida
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/listen_test.py /path/to/CoD2 --protocol 118
python3 scripts/map_reload_test.py /path/to/CoD2 --protocol 118
python3 scripts/rcon_drop_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/prediction_test.py /path/to/CoD2 --protocol 118
python3 scripts/fx_test.py /path/to/CoD2 --protocol 118
python3 scripts/bots_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/http_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` executa o `scripts/smoke_test.py` do CoD2rev-Plus, sem alterações, e compara as suas falhas com `test/smoke_expected_failures_<protocol>.txt`: as verificações que este servidor ainda não passa. Falha perante uma falha nova e perante uma da lista que agora passa, pelo que as listas só diminuem. `--protocol 115` testa a compilação 1.0. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` executa-o contra o servidor dedicado de Windows sob Wine (`scripts/portable_wine.sh`), com a sua própria lista, `test/smoke_expected_failures_wine.txt`: sob Wine o servidor escreve só na sua janela e um sinal não lhe chega como tal (um SIGSEGV enviado com `kill` é ignorado, o SIGTERM termina-o de imediato), por isso aí não se fazem o relatório de crash, as verificações de SIGTERM nem as que leem a sua saída (o script verifica `net_strictPort` no seu log de consola). As listas de Linux estão vazias: o servidor dedicado passa todo o teste de fumo.
- `two_players_test.py` liga dois clientes em tdm, faz com que enviem comandos, desliga um e volta a ligá-lo, e expulsa o outro; o servidor, com `developer 1` para que um erro de script pare o mapa, deve responder após cada passo.
- `listen_test.py` arranca um cliente que cria um jogo e um segundo que entra nele, e verifica-os depois de um `map_restart` e de uma mudança de mapa; precisa de uma CD key em `~/.cod2/preferences`.
- `map_reload_test.py` carrega mapas por rcon, alternando `map_restart` e `map` entre dois mapas, e verifica cada carregamento; `rcon_drop_test.py` lança um erro drop a partir do rcon (`error drop`) e verifica que o servidor continua de pé.
- `anim_builtins_test.py` executa `getAnimLength`, `animHasNotetrack`, `getMoveDelta` e `getAngleDelta` a partir do script de nível de um mod de teste.
- `prediction_test.py` conta os erros de previsão de um cliente real (com chave de CD) que anda, se agacha, se deita, salta e dispara; `--max-misses` define quantos admite.
- `fx_test.py` faz um cliente real (com chave de CD) reproduzir um sprite, um impacto de bala e duas explosões à sua frente, e verifica que cada um muda a captura.
- `bots_test.py` e `http_test.py` são os do CoD2rev-Plus, sem alterações, para a libcod: os bots de `mods/bots` têm de encontrar os seus caminhos, guardar e reler o seu grafo e lutar num mapa real; `httpFetch` e `webSocket*` têm de funcionar contra servidores locais HTTP, HTTPS, WS e WSS (precisam de `openssl`). Passe-lhes `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` conta os erros de C++ de um módulo (`--return-type` encontra funções não void que podem terminar sem return, o que é indefinido em C++); `scripts/gen_script_bridge.py --module script|server|client_server` regenera a ponte para a VM de scripts ou para o servidor, e `scripts/engine_abi.py --module server` regista, com o gdb, a ABI do motor contra a qual a ponte verifica (`test/server_abi.txt`).

</details>

## Estrutura do repositório

<details>
<summary><b>O que está onde</b></summary>

| Caminho | Conteúdo |
| --- | --- |
| `src/PC` | O motor reconstruído: cliente (`client_mp`, `cgame_mp`, `ui_mp`), servidor (`server_mp`), jogo (`game_mp`, `bgame`), renderizador (`gfx_d3d`), código comum (`qcommon`, `universal`), animações (`xanim`) e a VM de scripts reconstruída (`script`). No Linux e no servidor dedicado MinGW ficam de fora `server_mp`, `game_mp`, `game` e o núcleo que o CoD2rev-Plus substitui (`test/plus_replaces.txt`); o cliente MinGW deixa de fora `server_mp`, `game_mp`, `game` e a colisão. |
| `src/Mac` | A camada de plataforma do port para Mac de onde veio a reconstrução: OpenGL por trás de uma interface Direct3D, entrada e som. |
| `src/script` | A VM de scripts do CoD2rev-Plus, copiada sem alterações, com os cabeçalhos de que precisa em `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` e `src/libcod`. |
| `src/server` | O servidor do CoD2rev-Plus, copiado sem alterações; é usado pelo servidor dedicado. |
| `src/game`, `src/bgame` | O game e o bgame do CoD2rev-Plus, copiados sem alterações; são usados pelo servidor dedicado. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | O núcleo e o código de plataforma do CoD2rev-Plus, copiados sem alterações; o servidor dedicado corre sobre eles. `test/plus_replaces.txt` lista as fontes do motor que eles substituem lá. |
| `src/libcod`, `mods/bots` | A libcod do CoD2rev-Plus e as suas bibliotecas (yyjson, SQLite, Mbed TLS; o seu Speex não é compilado, usa-se o do motor), e o seu mod de bots, copiados sem alterações; o servidor dedicado e os jogos locais do cliente usam-nos. |
| `src/compat` | As pontes entre os nomes C do motor e a VM C++, o servidor e o núcleo do CoD2rev-Plus; na maior parte geradas. |
| `src/headers`, `src/imports` | Tipos partilhados, offsets verificados contra o binário original e declarações de funções importadas. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Dados tirados do binário original, ainda usados pelo motor. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Código de plataforma e stubs. |
| `scripts` | Scripts de compilação e de testes, e o cliente de testes do CoD2rev-Plus. |
| `test` | Testes unitários e as listas que as verificações leem. |
| `docs` | A [documentação](docs/README.md) e o banner. |

</details>

## Transparência

- **Quem mantém isto:** sou o JotaJazz. Gosto de jogos antigos e este fork existe por isso: quero continuar a jogar CoD2 com os meus amigos e manter o jogo vivo para a comunidade que ainda o joga, com um projeto aberto.
- **Como se trabalha aqui:** trabalho com o [Claude Code](https://claude.com/claude-code), o assistente de programação da Anthropic. As alterações deste fork, incluindo este README e as suas traduções, são escritas com a sua ajuda. Os commits feitos assim têm a linha `Co-Authored-By: Claude` na mensagem, pelo que podem ser identificados no histórico com `git log`.
- **Relação com o original:** a reconstrução é obra dos autores do opencod2 (riicchhaarrd) e do Sipex. Este fork já não traz alterações deles; em vez disso, traz código do CoD2rev-Plus, copiado sem alterações para que os seus commits também se apliquem aqui.
- **Licença:** ainda nenhuma, como no original. O código do CoD2rev-Plus que está aqui (VM de scripts, servidor, game, núcleo, libcod e o seu mod de bots) é AGPL-3.0, e a licença do conjunto está a ser resolvida com os autores originais. As bibliotecas de terceiros mantêm as suas: SQLite (domínio público), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT) e Speex (BSD, não compilado).
- **O que não é:** não é um projeto oficial nem está associado à Activision, à Infinity Ward ou aos autores do repositório original. Não inclui nenhum ficheiro do jogo.

## Aviso

Esta é uma reconstrução independente, ao nível do código-fonte, do motor de Call of Duty 2. Não está associada, autorizada, patrocinada nem endossada pela Activision Publishing, Inc., pela Infinity Ward ou por qualquer uma das suas afiliadas.

"Call of Duty" e "Call of Duty 2" são marcas registadas da Activision Publishing, Inc. São usadas neste repositório apenas para identificação e interoperabilidade, para descrever o que o código reconstrói. Não é reivindicado nenhum direito sobre essas marcas.

Este projeto não fornece dados de jogo protegidos por direitos de autor nem ajuda a obtê-los. Para usá-lo com conteúdo real do jogo, é preciso fornecer ficheiros de dados de uma cópia de Call of Duty 2 que possua legalmente.

O código-fonte do motor reconstruído é uma obra derivada criada para fins de preservação, interoperabilidade, investigação e educação. É fornecido no estado em que se encontra, sem garantia de qualquer tipo, expressa ou implícita.

Se é titular de direitos e acredita que algo aqui não deveria ser distribuído, abra uma issue ou contacte o responsável pela manutenção, e o assunto será tratado prontamente.

As capturas de ecrã mostram conteúdo de Call of Duty 2, propriedade da Activision Publishing, Inc., apenas para identificação.

## Créditos

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), a reconstrução do motor
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), robustez e correções de rede
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) e [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), o servidor dedicado e a VM de scripts
- id Software
- Activision / Infinity Ward

# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

**Español** · [English](README.md) · [Português](README.pt.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Русский](README.ru.md)

Código fuente, obtenido por ingeniería inversa, del motor de **Call of Duty 2**: el cliente multijugador y el servidor dedicado, para Linux y Windows.

Este repositorio es un fork de [opencod2/opencod2](https://github.com/opencod2/opencod2), que es donde se hizo la reconstrucción del motor. También trae cambios de [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). Aquí solo se añaden los cambios descritos en [Cambios de este fork](#cambios-de-este-fork). El objetivo es llegar a un solo repositorio con el cliente y el servidor dedicado, trayendo módulo por módulo el servidor de [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus). En Linux eso ya está hecho: el servidor dedicado corre entero sobre el código de CoD2rev-Plus, y las partidas locales del cliente corren sobre su servidor.

> [!WARNING]
> **Trabajo en curso: todavía no funciona del todo.** El cliente arranca hasta el menú principal, crea partidas locales a las que se conectan otros clientes, y el servidor dedicado, ahora el de CoD2rev-Plus, carga mapas y aguanta jugadores en 1.0 y 1.3, pero hay que esperar cierres inesperados, funciones que faltan y cosas que fallan. Se comparte para curiosos y para colaborar, no como un port terminado o jugable.

> [!IMPORTANT]
> **No incluye contenido del juego: hay que poner los datos propios.** Este repositorio es solo código fuente: no hay ejecutables, archivos, mapas, modelos, texturas, sonidos ni scripts. Para ejecutar algo hacen falta los archivos de datos de una copia del juego **que tengas legalmente**.

## Transparencia

- **Quién mantiene esto:** soy JotaJazz. Me gustan los juegos antiguos y este fork existe por eso: quiero seguir jugando CoD2 con mis amigos y mantener vivo el juego para la comunidad que todavía lo juega, con un proyecto abierto.
- **Cómo se trabaja aquí:** trabajo con [Claude Code](https://claude.com/claude-code), el asistente de programación de Anthropic. Los cambios de este fork, incluidos este README y sus traducciones, están escritos con su ayuda. Los commits hechos así llevan la línea `Co-Authored-By: Claude` en el mensaje, de modo que se pueden identificar en el historial con `git log`.
- **Relación con el original:** la reconstrucción es obra de los autores de opencod2 (riicchhaarrd) y de Sipex. Este fork ya no trae cambios de ellos; en su lugar trae código de CoD2rev-Plus, copiado sin cambios para que sus commits también se apliquen aquí.
- **Licencia:** el repositorio original no tiene licencia, y este todavía tampoco. CoD2rev-Plus, cuya máquina virtual de scripts, cuyo servidor, cuyo game y cuyo bgame ya están aquí (`src/script`, `src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`), es AGPL-3.0. La licencia del conjunto se está resolviendo con los autores originales antes de publicar aquí más código de ese proyecto. libcod (`src/libcod`) y su mod de bots (`mods/bots`) también vienen de CoD2rev-Plus, con bibliotecas de terceros bajo sus propias licencias: SQLite (dominio público), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT, en `yyjson.h`) y Speex (BSD, `src/libcod/speex/COPYING`, no se compila).
- **Qué no es:** no es un proyecto oficial ni está afiliado a Activision, Infinity Ward ni a los autores del repositorio original. No incluye ningún archivo del juego.

## Cambios de este fork

- **Compilaciones que vuelven a funcionar:** las compilaciones de Linux y MinGW, rotas desde que el original solo se compilaba con MSVC, vuelven a compilar; `scripts/build_check.py` compila Linux y MinGW (el servidor dedicado y, con SDL2, el cliente), como 1.3 y como 1.0, de una vez.
- **Compilado como C++:** en Linux y MinGW todo el motor se compila como C++ (gnu++17), con los headers compartidos convertidos en C++ válido, sin cambiar lo que hace; hay comprobaciones de los nombres de C de los que dependen los blobs de datos y de la disposición de las estructuras del binario original.
- **La máquina virtual de scripts de CoD2rev-Plus:** la máquina virtual de GSC es la de CoD2rev-Plus, copiada sin cambios en `src/script`, con un puente C/C++ generado. El servidor ahora aguanta dos jugadores, desconexiones, reconexiones y `clientkick`, donde la máquina virtual reconstruida se caía en cuanto entraba el segundo jugador.
- **El servidor de CoD2rev-Plus:** el servidor dedicado se compila con el servidor de CoD2rev-Plus, copiado sin cambios en `src/server`, con un puente generado en el que cada llamada entre los dos se comprueba contra la ABI del motor. Las partidas locales de los clientes de Linux y Windows (MinGW) también corren sobre él, y todo el cliente traza con su colisión: un cliente crea una partida y otros se conectan. Con él, el servidor se niega a enviar los `.iwd` originales y los propios del servidor. Con él vienen su game y su bgame (`src/game`, `src/bgame`), y con ellos `addip` y la rotación de `games_mp.log`; en el cliente, cgame se queda con el bgame del motor (`src/PC/bgame`), cuyos globales llevan el prefijo `eng_`, y que ahora predice los movimientos, el mantle y el arma del jugador con el código de CoD2rev-Plus: con los mismos movimientos ahora falla la predicción (tirones) unas ocho veces menos. Desde entonces el servidor dedicado corre también sobre su núcleo: `universal`, `qcommon` y el código de plataforma (`src/universal`, `src/qcommon`, `src/unix`, `src/win32`), que traen la rotación y las marcas de tiempo del log de consola, `net_strictPort` y su informe de crash. Con su xanim (`src/xanim`), todo lo que corre el servidor dedicado es código de CoD2rev-Plus.
- **La libcod de CoD2rev-Plus:** el servidor dedicado se compila con libcod, copiada sin cambios en `src/libcod` con los módulos que activa CoD2rev-Plus (todos menos MySQL): sus funciones de script, los bots, `sv_master1` a `sv_master5`, HTTP y WebSocket (Mbed TLS), SQLite y JSON. Pasa las pruebas de humo, de bots y de HTTP de CoD2rev-Plus en 1.3 y 1.0; las partidas locales del cliente también la tienen, en su servidor local (Linux y Windows): la prueba de bots pasa ahí también, y la de HTTP en Linux.
- **Un servidor dedicado sin las bibliotecas del cliente:** en Linux, `cod2_lnxded` solo necesita libc, libm, libz y libstdc++, no GL, X11, SDL2 ni curl.
- **Arreglos:** el servidor respondía al puerto equivocado, no veía ningún `.iwd` en unidades de Windows bajo WSL y se caía con cualquier expulsión; anunciaba el protocolo 118 en la compilación 1.0; la compilación C++ de Linux sigue en glibc 2.34; `getAnimLength` y otras tres funciones de animación ya no tumban el servidor. La cuarta o quinta carga de mapa tumbaba el servidor: el script de animaciones crecía con cada mapa, y el error que lanzaba desde rcon corrompía la pila. El servidor 1.0 escribía sobre sus propios configstrings y ningún cliente 1.0 podía entrar. Los mensajes fragmentados usaban desplazamientos equivocados en las dos versiones, y el puerto de una dirección de red estaba donde lo guarda Quake III, no donde lo guarda CoD2. No se buscaban los `.iwd` de la carpeta propia del servidor, y las funciones que reciben un jugador no lo veían con la máquina virtual de CoD2rev-Plus. Las entidades se enlazaban en los sectores del mundo con el número equivocado, así que desenlazar algunas tumbaba el servidor y las trazas probaban la entidad anterior. En el cliente, toda entidad recortada como caja quedaba fuera de los sectores del mundo y la prueba de caja fallaba los segmentos que la cruzaban, así que las balas atravesaban a los jugadores de una partida local; y desde un cambio de upstream para x64, en 32 bits no se reproducía ninguna animación de más de un cuadro, lo que dejaba a los jugadores y al arma en primera persona en una pose rota. El cliente se caía al ver morir a un jugador, y su predicción de movimiento podía desbordar la pila contra las paredes; enviaba un salto, un cambio de postura o de arma un comando tarde y truncaba la velocidad que predice; las animaciones de la MG42, el relleno y los anchos de texto de la UI y una docena más de llamadas del motor pasaban argumentos o leían resultados de otro tipo que el de la función, y los efectos con dirección recibían ejes rotos. No se dibujaba ningún efecto (impactos, explosiones, el humo de los mapas): el código de efectos leía sus flags, escalas de curva, colores y velocidades de otra forma que el binario original, programaba las partes con retardo para un tiempo que nunca llegaba, dejaba cada partícula en el origen del mundo sin segundo radio, y nada las dibujaba.
- **Pruebas:** el smoke test de CoD2rev-Plus, sin cambios, contra este servidor, con la lista de comprobaciones que todavía no pasa; pruebas de dos jugadores, de cargas de mapa, de un error de drop desde rcon y de las funciones de animación; y comprobaciones de la disposición de las estructuras, de los nombres de C, de la ABI del puente y de los archivos copiados de CoD2rev-Plus. Unas pruebas disparan a un jugador a nivel y hacia abajo y comprueban que cada tiro dé en la parte superior del cuerpo o en las piernas, y que un cuchillo y una explosión alcancen a un jugador, en el servidor dedicado y en una partida local.

Cada cambio en detalle está en [docs/CHANGES.md](docs/CHANGES.md) (en inglés).

## Versiones del juego

- **La versión se elige al compilar.** Por defecto el motor se compila como 1.3 (protocolo 118); con `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` se compila como 1.0 (protocolo 115).
- **Los dos servidores dedicados sirven para pruebas:** en 1.3 y en 1.0 el servidor carga, cambia y reinicia mapas, responde a `getinfo`, `getstatus` y rcon, y aguanta dos jugadores que entran, salen, vuelven y son expulsados.
- **Las partidas locales sirven para pruebas:** en 1.3 y en 1.0 un cliente crea una partida sobre el servidor de CoD2rev-Plus, otro cliente se conecta y los dos siguen después de un `map_restart` y de un cambio de mapa (`scripts/listen_test.py`, con tu CD key). Un disparo da en la parte del cuerpo a la que apunta (`scripts/hit_location_test.py`).
- **Lo que CoD2rev-Plus tiene en libcod todavía no está aquí:** sus funciones de script, los bots y `sv_master1`…`sv_master5`, así que el servidor manda su latido solo al maestro de Activision.

## Compilar

Todos los objetivos se manejan con **CMake (≥ 3.16)**, cada uno compilado fuera del código fuente en su propia carpeta.

### Linux

El motor es un binario x86 de 32 bits. En un Debian o Ubuntu de 64 bits hacen falta la arquitectura y las bibliotecas de 32 bits:

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

Las bibliotecas del cliente hacen falta para compilar los dos, pero `cod2_lnxded` se ejecuta solo con libc, libm, libz y libstdc++ de 32 bits.

### Windows (cruzado desde Linux con MinGW)

Necesita los compiladores MinGW-w64 de 32 bits (`g++-mingw-w64-i686`) y la zlib estática para MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Añade `-DCOD2_WIN32_CLIENT=ON` para el cliente SDL2/GL (pon las bibliotecas estáticas de SDL2 en `src/win32/sdl2/lib/`; el servidor dedicado no necesita ninguna). Como el cliente de Linux, corre sus partidas locales sobre el servidor, el game y el bgame de CoD2rev-Plus, y traza con su colisión. Sin Windows, `scripts/portable_wine.sh DIR` instala un Wine de 32 bits sin root (Debian o Ubuntu con i386 habilitado) y escribe `DIR/wine.sh` para correrlo; `COD2_CONSOLE_STDOUT=1` copia la consola a stdout, y `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` hace que cree una partida a la que se une el cliente de Linux.

### Windows (MSVC, nativo)

Una compilación nativa del cliente SDL2/GL completo con Visual Studio 2022 o más nuevo y su toolset **x86**. Configura y compila desde un "x86 Native Tools for VS" command prompt:

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

Esta compilación todavía compila C y usa la máquina virtual de scripts reconstruida (`src/PC/script`). SDL2 lo pones tú: deja el paquete de desarrollo de SDL2 para MSVC de 32 bits en `third_party/SDL2-<versión>/` y copia `SDL2.dll` junto al ejecutable. Con `-DCOD2_SDL2_STATIC=ON` enlaza un SDL2 estático compilado por ti (`SDL2-static.lib`, en la misma configuración que el motor) y el runtime de C estático, en un solo ejecutable que no necesita ninguna DLL; compila SDL2 con `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. Release aplica `/O2` a código fiel al descompilador que está menos probado que la compilación Debug: si algo falla solo en Release, sospecha del optimizador.

### Opciones de CMake

| Opción | Por defecto | Descripción |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compila el motor como C++ en Linux y MinGW. `OFF` compila C. |
| `COD2_CXX_DIRS` | `""` | Con `COD2_CXX_ALL=OFF`, los módulos o archivos de `src/` que se siguen compilando como C++. |
| `COD2_PLUS_SCRIPT` | `ON` | La máquina virtual de scripts de CoD2rev-Plus (`src/script`) en Linux y MinGW. `OFF` usa la reconstruida (`src/PC/script`), como MSVC, la compilación web y x64. |
| `COD2_PLUS_SERVER` | `ON` | El servidor, el game, el bgame y el núcleo de CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` o `src/win32`) en el servidor dedicado, en Linux y MinGW; necesita `COD2_PLUS_SCRIPT`. `OFF` usa los reconstruidos (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | El servidor, el game, el bgame y la colisión de CoD2rev-Plus para las partidas locales del cliente (Linux, y MinGW con `COD2_WIN32_CLIENT`), sobre el núcleo del motor; necesita `COD2_PLUS_SERVER`. `OFF` deja `src/PC/server_mp`, como todavía hacen MSVC, la compilación web y x64. |
| `COD2_PLUS_LIBCOD` | `ON` | La libcod de CoD2rev-Plus (`src/libcod`) en el servidor dedicado y, con `COD2_PLUS_CLIENT_SERVER`, en las partidas locales del cliente, con sus módulos salvo MySQL; necesita `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Interruptores de funciones; `-DCOD2_PATCH_10` compila la versión 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: el cliente SDL2/GL en vez del servidor dedicado. |
| `COD2_WIN_MSVC` | `OFF` | Compilación nativa con MSVC (la activan los presets `msvc-client`). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: un solo ejecutable independiente con SDL2 y runtime de C estáticos. |
| `COD2_DEBUG` | `OFF` | `-O0` y sin endurecimiento, para depurar. |
| `COD2_X64` | `OFF` | Objetivo x86_64; un port en curso que compila pero todavía no se ejecuta. |

También hay una compilación WebAssembly (`emcmake cmake -S . -B build/web`, con el SDK de Emscripten), descrita al principio de `CMakeLists.txt`.

## Ejecutar

El motor necesita los archivos del juego original (la carpeta `main` con sus `.iwd`), que no están en este repositorio. Hay que tener una copia propia de Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

En Windows:

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` deja el servidor en la red local; es un servidor de pruebas, todavía no uno para poner en la lista pública.

## Pruebas

Las comprobaciones necesitan la compilación y, las que arrancan el servidor, una carpeta del juego con las `.iwd` originales. Arrancan el servidor en modo LAN con una carpeta de usuario temporal, así que la carpeta del juego solo se lee.

```bash
python3 scripts/build_check.py all                      # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/check_c_decls.py build/check-linux     # cada llamada del motor coincide con los tipos de su definición
python3 scripts/check_var_sizes.py build/check-linux    # ninguna global se usa más grande de lo que se define
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

- `opencod2_smoke.py` ejecuta el `scripts/smoke_test.py` de CoD2rev-Plus, sin cambios, y compara sus fallos con `test/smoke_expected_failures_<protocolo>.txt`: las comprobaciones que este servidor todavía no pasa. Falla ante un fallo nuevo y ante uno de la lista que ahora pasa, así que las listas solo se achican. `--protocol 115` prueba la compilación 1.0. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` lo ejecuta contra el servidor dedicado de Windows bajo Wine (`scripts/portable_wine.sh`), con su propia lista, `test/smoke_expected_failures_wine.txt`: bajo Wine el servidor imprime solo en su ventana y una señal no le llega como tal (un SIGSEGV enviado con `kill` se ignora, SIGTERM lo termina de inmediato), así que ahí no se comprueban el informe de crash, las comprobaciones de SIGTERM ni las que leen su salida (el envoltorio comprueba `net_strictPort` en su log de consola). Las listas de Linux están vacías: el servidor dedicado pasa la prueba de humo completa.
- `two_players_test.py` conecta dos clientes en tdm, les hace enviar comandos, desconecta a uno y lo vuelve a conectar, y expulsa al otro; el servidor, con `developer 1` para que un error de script detenga el mapa, tiene que responder después de cada paso.
- `listen_test.py` arranca un cliente que crea una partida y otro que se conecta, y los comprueba después de un `map_restart` y de un cambio de mapa; necesita una CD key en `~/.cod2/preferences`.
- `map_reload_test.py` carga mapas por rcon, alternando `map_restart` y `map` entre dos mapas, y comprueba cada carga; `rcon_drop_test.py` lanza un error de drop desde rcon (`error drop`) y comprueba que el servidor siga en pie.
- `anim_builtins_test.py` ejecuta `getAnimLength`, `animHasNotetrack`, `getMoveDelta` y `getAngleDelta` desde el script de mapa de un mod de prueba.
- `prediction_test.py` cuenta los errores de predicción de un cliente real (con clave de CD) que camina, se agacha, se tira al suelo, salta y dispara; `--max-misses` fija cuántos admite.
- `fx_test.py` hace que un cliente real (con clave de CD) reproduzca un sprite, un impacto de bala y dos explosiones delante de él, y comprueba que cada uno cambie la captura.
- `bots_test.py` y `http_test.py` son los de CoD2rev-Plus, sin cambios, para libcod: los bots de `mods/bots` deben encontrar sus rutas, guardar y releer su grafo y pelear en un mapa real; `httpFetch` y `webSocket*` deben funcionar contra servidores locales HTTP, HTTPS, WS y WSS (necesitan `openssl`). Pásales `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` cuenta los errores de C++ de un módulo (`--return-type` encuentra funciones no `void` que pueden terminar sin `return`, algo indefinido en C++); `scripts/gen_script_bridge.py --module script|server` regenera el puente con la máquina virtual de scripts o con el servidor, y `scripts/engine_abi.py --module server` registra, con gdb, la ABI del motor contra la que se comprueba el puente (`test/server_abi.txt`).

## Estructura del repositorio

| Ruta | Contenido |
| --- | --- |
| `src/PC` | El motor reconstruido: cliente (`client_mp`, `cgame_mp`, `ui_mp`), servidor (`server_mp`), juego (`game_mp`, `bgame`), render (`gfx_d3d`), código compartido (`qcommon`, `universal`), animaciones (`xanim`) y la máquina virtual de scripts reconstruida (`script`). En Linux y en el servidor dedicado de MinGW se dejan afuera `server_mp`, `game_mp`, `game` y el núcleo que reemplaza CoD2rev-Plus (`test/plus_replaces.txt`); el cliente MinGW deja afuera `server_mp`, `game_mp`, `game` y la colisión. |
| `src/Mac` | La capa de plataforma del port de Mac del que sale la reconstrucción: OpenGL detrás de una interfaz de Direct3D, entrada y sonido. |
| `src/script` | La máquina virtual de scripts de CoD2rev-Plus, copiada sin cambios, con los headers que necesita en `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` y `src/libcod`. |
| `src/server` | El servidor de CoD2rev-Plus, copiado sin cambios; lo usa el servidor dedicado. |
| `src/game`, `src/bgame` | El game y el bgame de CoD2rev-Plus, copiados sin cambios; los usa el servidor dedicado. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | El núcleo y el código de plataforma de CoD2rev-Plus, copiados sin cambios; sobre ellos corre el servidor dedicado. `test/plus_replaces.txt` lista las fuentes del motor que reemplazan ahí. |
| `src/libcod`, `mods/bots` | La libcod de CoD2rev-Plus y sus bibliotecas (yyjson, SQLite, Mbed TLS; su Speex no se compila, se usa el del motor), y su mod de bots, copiados sin cambios; los usan el servidor dedicado y las partidas locales del cliente. |
| `src/compat` | Los puentes entre los nombres de C del motor y la máquina virtual, el servidor y el núcleo C++ de CoD2rev-Plus; casi todo generado. |
| `src/headers`, `src/imports` | Tipos compartidos, desplazamientos comprobados contra el binario original y declaraciones de las funciones importadas. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Datos sacados del binario original, que el motor todavía usa. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Código de cada plataforma y stubs. |
| `scripts` | Scripts de compilación y de prueba, y el cliente de prueba de CoD2rev-Plus. |
| `test` | Pruebas unitarias y las listas que leen las comprobaciones. |
| `docs` | [CHANGES.md](docs/CHANGES.md) y el banner. |

## Aviso

Esta es una reconstrucción independiente, a nivel de código fuente, del motor de Call of Duty 2. No está afiliada a Activision Publishing, Inc., Infinity Ward ni a ninguna de sus filiales, ni autorizada, patrocinada o respaldada por ellas.

"Call of Duty" y "Call of Duty 2" son marcas registradas de Activision Publishing, Inc. Se usan en este repositorio solo para identificar y para la interoperabilidad, para describir lo que el código reconstruye. No se reivindica ningún derecho sobre esas marcas.

Este proyecto no proporciona datos del juego protegidos por derechos de autor ni ayuda a obtenerlos. Para usarlo con contenido real del juego, tienes que poner los archivos de datos de una copia de Call of Duty 2 que tengas legalmente.

El código fuente reconstruido del motor es una obra derivada creada con fines de preservación, interoperabilidad, investigación y educación. Se proporciona tal cual, sin garantía de ningún tipo, expresa ni implícita.

Si eres titular de derechos y crees que algo de aquí no debería distribuirse, abre un issue o contacta con el mantenedor y se atenderá cuanto antes.

## Créditos

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), la reconstrucción del motor
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), endurecimiento del código y arreglos de red
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) y [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), el servidor dedicado y la máquina virtual de scripts
- id Software
- Activision / Infinity Ward

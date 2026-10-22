# OpenCoD2-Plus

<p align="center"><img src="docs/banner.svg" alt="OpenCoD2-Plus" width="100%"></p>

![Status: in development](https://img.shields.io/badge/status-in%20development-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · [Português](README.pt.md) · **Français** · [Deutsch](README.de.md) · [Русский](README.ru.md)

**Call of Duty 2 est toujours vivant.** OpenCoD2-Plus est le moteur multijoueur de Call of Duty 2 reconstruit à partir de son code source : le client et le serveur dédié, pour Windows et Linux, réalisé par des gens qui jouent encore au jeu et continuent de l'améliorer.

<table>
  <tr>
    <td><img src="docs/images/carentan.jpg" alt="Carentan : le piano devant le café Jeanne d'Arc"></td>
    <td><img src="docs/images/toujane.jpg" alt="Toujane : le canon antiaérien et la tour de la mosquée"></td>
  </tr>
  <tr>
    <td colspan="2"><img src="docs/images/scoreboard.jpg" alt="Le tableau des scores d'une partie locale avec des bots"></td>
  </tr>
</table>
<p align="center"><sub>Carentan, Toujane et une partie locale avec des bots, dessinés par le client Linux avec les shaders du jeu lui-même.</sub></p>

## Ce qui fonctionne aujourd'hui

- **Le jeu en ligne sur les serveurs publics 1.3** avec le client Windows, avec l'éclairage d'origine : Direct3D 9 et les shaders du jeu lui-même.
- **Héberger une partie depuis le client**, que vos amis peuvent rejoindre, avec des bots si vous le souhaitez (libcod).
- **Un serveur dédié pour la 1.3 et la 1.0**, basé sur [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) : sous Linux, il passe tout le test de fumée de CoD2rev-Plus.
- **Un client Linux**, et un autre sur Vulkan (DXVK) qui dessine lui aussi avec les shaders du jeu lui-même.
- **Vérifié par rapport aux binaires 1.3 d'origine** : prédiction des mouvements, effets, sons, recul, menus et de nombreuses corrections ([ce qui a changé](docs/changes.md)).

> [!NOTE]
> **Encore en développement.** Attendez-vous à des bugs et à des éléments manquants, et signalez-les dans les [issues](https://github.com/JotaJazz/opencod2-plus/issues).

> [!IMPORTANT]
> **Aucun contenu du jeu n'est inclus — apportez vos propres données.** Ce dépôt ne contient que du code source : ni exécutables, ni archives, ni cartes, modèles, textures, sons ou scripts. Pour lancer quoi que ce soit, vous devez fournir les fichiers de données d'une copie du jeu que **vous possédez légalement**.

## Comment jouer

Des versions prêtes à l'emploi seront publiées sur la page **Releases**. En attendant, compilez le client (voir [Compilation](#compilation)) et indiquez-lui votre dossier de Call of Duty 2, celui qui contient `main/` :

```bat
cod2_win32.exe +set fs_basepath "C:\path\to\CoD2" +connect IP:port
```

```sh
./cod2_linux +set fs_basepath /path/to/CoD2 +connect IP:port
./cod2_linux_vk +set fs_basepath /path/to/CoD2 +connect IP:port   # with the game's own shaders (Vulkan)
```

Sans `+connect`, le client ouvre le menu principal, d'où vous pouvez aussi lancer votre propre partie. Un serveur dédié pour votre réseau local :

```sh
./cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

## Pour les contributeurs

Toute aide compte : jouer et signaler des bugs, tester sur d'autres systèmes, ou du code. La documentation technique, en anglais, commence dans [docs/README.md](docs/README.md) ; [AGENTS.md](AGENTS.md) est le point d'entrée pour les agents de programmation IA.

## Compilation

Toutes les cibles sont pilotées par **CMake (≥ 3.16)**, chacune compilée hors des sources dans son propre dossier.

### Linux

Le moteur est un binaire x86 32 bits. Sur un système Debian ou Ubuntu 64 bits, il faut l'architecture et les bibliothèques 32 bits :

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

Les bibliothèques du client sont nécessaires pour compiler les deux, mais `cod2_lnxded` s'exécute avec seulement libc, libm, libz et libstdc++ en 32 bits.

`cod2_linux` dessine avec des shaders de remplacement (pas de soleil, ciel blanc). Pour les shaders du jeu lui-même, compilez `cod2_linux_vk` : `scripts/build_linux_vk_deps.sh` compile DXVK native et vkd3d-shader (32 bits, depuis des versions fixées ; il faut aussi `meson`, `ninja` et `libvulkan1:i386`), puis configurez avec `-DCOD2_LINUX_DXVK=ON` dans son propre répertoire de compilation. Il faut un périphérique Vulkan.

### Windows (compilation croisée depuis Linux avec MinGW)

Nécessite les compilateurs MinGW-w64 32 bits (`g++-mingw-w64-i686`) et la zlib statique pour MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Ajoutez `-DCOD2_WIN32_CLIENT=ON` pour le client, qui dessine avec Direct3D 9 et les shaders du jeu (`-DCOD2_WIN32_REAL_D3D9=OFF` : le périphérique GL) (fournissez les bibliothèques statiques de SDL2 dans `src/win32/sdl2/lib/` ; le serveur dédié n'en a pas besoin). Comme le client Linux, il fait tourner ses parties locales sur le serveur, le game et le bgame de CoD2rev-Plus, et trace avec sa collision. Sans Windows, `scripts/portable_wine.sh DIR` installe un Wine 32 bits sans root (Debian ou Ubuntu avec i386 activé) et écrit `DIR/wine.sh` pour le lancer ; `COD2_CONSOLE_STDOUT=1` copie la console sur stdout, et `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` lui fait héberger une partie que rejoint le client Linux.

<details>
<summary><b>Windows (MSVC, natif)</b></summary>

Une compilation native du client SDL2/GL complet avec Visual Studio 2022 ou plus récent et son jeu d'outils **x86**. Configurez et compilez depuis une invite de commandes « x86 Native Tools for VS » :

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

Cette compilation compile encore en C et utilise la machine virtuelle de scripts reconstruite (`src/PC/script`). SDL2 est à fournir par vous : placez le paquet de développement SDL2 MSVC 32 bits dans `third_party/SDL2-<version>/` et copiez `SDL2.dll` à côté de l'exécutable. Avec `-DCOD2_SDL2_STATIC=ON`, elle lie une SDL2 statique que vous compilez vous-même (`SDL2-static.lib`, même configuration que le moteur) et le runtime C statique, en un seul exécutable qui n'a besoin d'aucune DLL ; compilez SDL2 avec `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. Release applique `/O2` à du code fidèle au décompilateur et moins testé que la compilation Debug : si quelque chose ne se comporte mal qu'en Release, soupçonnez l'optimiseur.

</details>

<details>
<summary><b>Options CMake</b></summary>

| Option | Valeur par défaut | Description |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compile le moteur en C++ sous Linux et MinGW. `OFF` compile en C. |
| `COD2_CXX_DIRS` | `""` | Avec `COD2_CXX_ALL=OFF`, les modules ou fichiers de `src/` encore compilés en C++. |
| `COD2_PLUS_SCRIPT` | `ON` | La machine virtuelle de scripts de CoD2rev-Plus (`src/script`) sous Linux et MinGW. `OFF` utilise celle reconstruite (`src/PC/script`), comme le font MSVC, la compilation web et x64. |
| `COD2_PLUS_SERVER` | `ON` | Le serveur, le game, le bgame et le noyau de CoD2rev-Plus (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` ou `src/win32`) dans le serveur dédié, sous Linux et MinGW ; nécessite `COD2_PLUS_SCRIPT`. `OFF` utilise ceux reconstruits (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | Le serveur, le game, le bgame et la collision de CoD2rev-Plus pour les parties locales du client (Linux, et MinGW avec `COD2_WIN32_CLIENT`), sur le noyau du moteur ; nécessite `COD2_PLUS_SERVER`. `OFF` garde `src/PC/server_mp`, comme le font encore MSVC, la version web et x64. |
| `COD2_PLUS_LIBCOD` | `ON` | La libcod de CoD2rev-Plus (`src/libcod`) dans le serveur dédié et, avec `COD2_PLUS_CLIENT_SERVER`, dans les parties locales du client, avec ses modules sauf MySQL ; nécessite `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Options de fonctionnalités ; `-DCOD2_PATCH_10` compile la version 1.0. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW : le client au lieu du serveur dédié. |
| `COD2_WIN32_REAL_D3D9` | `ON` | Client MinGW : vrai Direct3D 9 et les shaders du jeu (d3dx9 du système) ; `OFF` garde le périphérique GL. |
| `COD2_WIN_MSVC` | `OFF` | Compilation MSVC native (activée par les presets `msvc-client`). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC : un seul exécutable autonome avec SDL2 et le runtime C statiques. |
| `COD2_DEBUG` | `OFF` | `-O0` et sans durcissement, pour le débogage. |
| `COD2_X64` | `OFF` | Cible x86_64 ; un portage en cours qui compile mais ne fonctionne pas encore. |

Il existe aussi une compilation WebAssembly (`emcmake cmake -S . -B build/web`, avec l'Emscripten SDK), décrite en tête de `CMakeLists.txt`.

</details>

## Tests

<details>
<summary><b>Les scripts de test</b></summary>

Les vérifications ont besoin de la compilation et, pour celles qui démarrent le serveur, d'un dossier du jeu avec les `.iwd` d'origine. Elles démarrent le serveur en mode LAN avec un dossier utilisateur temporaire, le dossier du jeu n'est donc que lu.

```bash
python3 scripts/build_check.py all                      # every variant (linux, linux-10, mingw, mingw-client...) into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/check_c_decls.py build/check-linux     # chaque appel du moteur correspond aux types de sa définition
python3 scripts/check_var_sizes.py build/check-linux    # aucune globale n'est utilisée plus grande qu'elle n'est définie
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

- `opencod2_smoke.py` lance le `scripts/smoke_test.py` de CoD2rev-Plus, sans modification, et compare ses échecs avec `test/smoke_expected_failures_<protocol>.txt` : les vérifications que ce serveur ne passe pas encore. Il échoue sur un nouvel échec et sur une vérification listée qui passe désormais, de sorte que les listes ne font que raccourcir. `--protocol 115` teste la compilation 1.0. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` le lance contre le serveur dédié Windows sous Wine (`scripts/portable_wine.sh`), avec sa propre liste, `test/smoke_expected_failures_wine.txt` : sous Wine, le serveur n'affiche que dans sa fenêtre et un signal ne l'atteint pas comme tel (un SIGSEGV envoyé avec `kill` est ignoré, SIGTERM le termine aussitôt), donc le rapport de plantage, les vérifications de SIGTERM et celles qui lisent sa sortie n'y sont pas faites (le script vérifie `net_strictPort` dans son journal de console). Les listes Linux sont vides : le serveur dédié passe tout le test de fumée.
- `two_players_test.py` fait rejoindre deux clients en tdm, leur fait envoyer des commandes, déconnecte l'un puis le fait revenir, et kicke l'autre ; le serveur, avec `developer 1` pour qu'une erreur de script arrête la carte, doit répondre après chaque étape.
- `listen_test.py` lance un client qui crée une partie et un second qui la rejoint, et les vérifie après un `map_restart` et un changement de carte ; il faut une clé CD dans `~/.cod2/preferences`.
- `map_reload_test.py` charge des cartes par rcon, en alternant `map_restart` et `map` entre deux cartes, et vérifie chaque chargement ; `rcon_drop_test.py` lève une erreur drop depuis rcon (`error drop`) et vérifie que le serveur reste en marche.
- `anim_builtins_test.py` exécute `getAnimLength`, `animHasNotetrack`, `getMoveDelta` et `getAngleDelta` depuis le script de niveau d'un mod de test.
- `prediction_test.py` compte les erreurs de prédiction d'un vrai client (avec une clé CD) qui marche, s'accroupit, se couche, saute et tire ; `--max-misses` fixe combien il en admet.
- `fx_test.py` fait jouer à un vrai client (avec une clé CD) un sprite, un impact de balle et deux explosions devant lui, et vérifie que chacun change la capture d'écran.
- `bots_test.py` et `http_test.py` sont ceux de CoD2rev-Plus, sans modification, pour libcod : les bots de `mods/bots` doivent trouver leurs chemins, enregistrer et relire leur graphe et se battre sur une vraie carte ; `httpFetch` et `webSocket*` doivent fonctionner avec des serveurs locaux HTTP, HTTPS, WS et WSS (ils nécessitent `openssl`). Passez-leur `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` compte les erreurs C++ d'un module (`--return-type` trouve les fonctions non void qui peuvent se terminer sans return, ce qui est indéfini en C++) ; `scripts/gen_script_bridge.py --module script|server|client_server` régénère le pont vers la machine virtuelle de scripts ou vers le serveur, et `scripts/engine_abi.py --module server` enregistre, avec gdb, l'ABI du moteur par rapport à laquelle le pont vérifie (`test/server_abi.txt`).

</details>

## Structure du dépôt

<details>
<summary><b>Ce qui se trouve où</b></summary>

| Chemin | Contenu |
| --- | --- |
| `src/PC` | Le moteur reconstruit : client (`client_mp`, `cgame_mp`, `ui_mp`), serveur (`server_mp`), jeu (`game_mp`, `bgame`), moteur de rendu (`gfx_d3d`), code commun (`qcommon`, `universal`), animations (`xanim`) et la machine virtuelle de scripts reconstruite (`script`). Sous Linux et dans le serveur dédié MinGW, `server_mp`, `game_mp`, `game` et le noyau que remplace CoD2rev-Plus sont laissés de côté (`test/plus_replaces.txt`) ; le client MinGW laisse de côté `server_mp`, `game_mp`, `game` et la collision. |
| `src/Mac` | La couche de plateforme du portage Mac dont provient la reconstruction : OpenGL derrière une interface Direct3D, entrées et son. |
| `src/script` | La machine virtuelle de scripts de CoD2rev-Plus, copiée sans modification, avec les en-têtes dont elle a besoin dans `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` et `src/libcod`. |
| `src/server` | Le serveur de CoD2rev-Plus, copié sans modification ; le serveur dédié l'utilise. |
| `src/game`, `src/bgame` | Le game et le bgame de CoD2rev-Plus, copiés sans modification ; le serveur dédié les utilise. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | Le noyau et le code de plateforme de CoD2rev-Plus, copiés sans modification ; le serveur dédié tourne dessus. `test/plus_replaces.txt` liste les sources du moteur qu'ils y remplacent. |
| `src/libcod`, `mods/bots` | La libcod de CoD2rev-Plus et ses bibliothèques (yyjson, SQLite, Mbed TLS ; son Speex n'est pas compilé, celui du moteur est utilisé), et son mod de bots, copiés sans modification ; le serveur dédié et les parties locales du client les utilisent. |
| `src/compat` | Les ponts entre les noms C du moteur et la machine virtuelle C++, le serveur et le noyau de CoD2rev-Plus ; en grande partie générés. |
| `src/headers`, `src/imports` | Types communs, offsets vérifiés par rapport au binaire d'origine, et déclarations des fonctions importées. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Données extraites du binaire d'origine, encore utilisées par le moteur. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Code de plateforme et stubs. |
| `scripts` | Scripts de compilation et de test, et le client de test de CoD2rev-Plus. |
| `test` | Tests unitaires et les listes que lisent les vérifications. |
| `docs` | La [documentation](docs/README.md) et la bannière. |

</details>

## Transparence

- **Qui maintient ce dépôt :** je suis JotaJazz. J'aime les vieux jeux, et c'est la raison d'être de ce fork : je veux continuer à jouer à CoD2 avec mes amis, et garder le jeu vivant pour la communauté qui y joue encore, avec un projet ouvert.
- **Comment le travail est fait :** je travaille avec [Claude Code](https://claude.com/claude-code), l'assistant de programmation d'Anthropic. Les modifications de ce fork, y compris ce README et ses traductions, sont écrites avec son aide. Les commits réalisés ainsi portent la ligne `Co-Authored-By: Claude` dans leur message, ce qui permet de les identifier dans l'historique avec `git log`.
- **Relation avec l'original :** la reconstruction est l'œuvre des auteurs d'opencod2 (riicchhaarrd) et de Sipex. Ce fork ne récupère plus leurs modifications ; il intègre à la place du code de CoD2rev-Plus, copié sans modification pour que ses commits s'appliquent aussi ici.
- **Licence :** pas encore, comme pour l'original. Le code de CoD2rev-Plus présent ici (machine virtuelle de scripts, serveur, game, noyau, libcod et son mod de bots) est sous AGPL-3.0, et la licence de l'ensemble est en cours de discussion avec les auteurs d'origine. Les bibliothèques tierces gardent la leur : SQLite (domaine public), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT) et Speex (BSD, non compilé).
- **Ce que ce n'est pas :** ce n'est pas un projet officiel et il n'est affilié ni à Activision, ni à Infinity Ward, ni aux auteurs du dépôt d'origine. Il ne contient aucun fichier du jeu.

## Avis

Ceci est une reconstruction indépendante, au niveau du code source, du moteur de Call of Duty 2. Elle n'est ni affiliée à, ni autorisée, ni parrainée, ni approuvée par Activision Publishing, Inc., Infinity Ward ou l'une quelconque de leurs sociétés affiliées.

« Call of Duty » et « Call of Duty 2 » sont des marques d'Activision Publishing, Inc. Elles ne sont utilisées dans ce dépôt qu'à des fins d'identification et d'interopérabilité, pour décrire ce que le code reconstruit. Aucun droit n'est revendiqué sur ces marques.

Ce projet ne fournit pas de données de jeu protégées par le droit d'auteur et n'aide pas à les obtenir. Pour l'utiliser avec le véritable contenu du jeu, vous devez fournir les fichiers de données d'une copie de Call of Duty 2 que vous possédez légalement.

Le code source du moteur reconstruit est une œuvre dérivée créée à des fins de préservation, d'interopérabilité, de recherche et d'éducation. Il est fourni tel quel, sans garantie d'aucune sorte, expresse ou implicite.

Si vous êtes titulaire de droits et estimez que quelque chose ici ne devrait pas être distribué, veuillez ouvrir une issue ou contacter le mainteneur, et la question sera traitée rapidement.

Les captures d'écran montrent du contenu de Call of Duty 2, propriété d'Activision Publishing, Inc., à des fins d'identification uniquement.

## Crédits

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), la reconstruction du moteur
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), durcissement et corrections réseau
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) et [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), le serveur dédié et la machine virtuelle de scripts
- id Software
- Activision / Infinity Ward

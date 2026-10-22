# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · [English](README.md) · [Português](README.pt.md) · **Français** · [Deutsch](README.de.md) · [Русский](README.ru.md)

Code source, obtenu par rétro-ingénierie, du moteur de **Call of Duty 2** : le client multijoueur et le serveur dédié, pour Linux et Windows.

Ce dépôt est un fork de [opencod2/opencod2](https://github.com/opencod2/opencod2), où la reconstruction du moteur a été réalisée. Il reprend aussi des modifications de [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). Ce fork n'ajoute que les modifications décrites dans [Modifications de ce fork](#modifications-de-ce-fork). Son objectif est de devenir un seul dépôt avec le client et le serveur dédié, en y intégrant module par module le serveur de [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus). Sous Linux, c'est fait : le serveur dédié tourne entièrement sur le code de CoD2rev-Plus, et les parties locales du client tournent sur son serveur.

> [!WARNING]
> **Travail en cours — cela ne fonctionne pas encore entièrement.** Le client démarre jusqu'au menu principal, crée des parties locales que d'autres clients rejoignent, et le serveur dédié, désormais celui de CoD2rev-Plus, charge des cartes et accueille des joueurs en 1.0 et en 1.3, mais attendez-vous à des plantages, des fonctionnalités manquantes et d'autres qui ne marchent pas. Il est partagé pour les curieux et pour la collaboration, pas comme un portage terminé ou jouable.

> [!IMPORTANT]
> **Aucun contenu du jeu n'est inclus — apportez vos propres données.** Ce dépôt ne contient que du code source : ni exécutables, ni archives, ni cartes, modèles, textures, sons ou scripts. Pour lancer quoi que ce soit, vous devez fournir les fichiers de données d'une copie du jeu que **vous possédez légalement**.

## Transparence

- **Qui maintient ce dépôt :** je suis JotaJazz. J'aime les vieux jeux, et c'est la raison d'être de ce fork : je veux continuer à jouer à CoD2 avec mes amis, et garder le jeu vivant pour la communauté qui y joue encore, avec un projet ouvert.
- **Comment le travail est fait :** je travaille avec [Claude Code](https://claude.com/claude-code), l'assistant de programmation d'Anthropic. Les modifications de ce fork, y compris ce README et ses traductions, sont écrites avec son aide. Les commits réalisés ainsi portent la ligne `Co-Authored-By: Claude` dans leur message, ce qui permet de les identifier dans l'historique avec `git log`.
- **Relation avec l'original :** la reconstruction est l'œuvre des auteurs d'opencod2 (riicchhaarrd) et de Sipex. Ce fork ne récupère plus leurs modifications ; il intègre à la place du code de CoD2rev-Plus, copié sans modification pour que ses commits s'appliquent aussi ici.
- **Licence :** le dépôt d'origine n'a pas de licence, et celui-ci n'en a pas encore non plus. CoD2rev-Plus, dont la machine virtuelle de scripts, le serveur, le game et le bgame sont déjà ici (`src/script`, `src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`), est sous AGPL-3.0. La licence de l'ensemble est en cours de discussion avec les auteurs d'origine avant de publier ici davantage de ce code. libcod (`src/libcod`) et son mod de bots (`mods/bots`) viennent aussi de CoD2rev-Plus, avec des bibliothèques tierces sous leurs propres licences : SQLite (domaine public), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT, dans `yyjson.h`) et Speex (BSD, `src/libcod/speex/COPYING`, non compilé).
- **Ce que ce n'est pas :** ce n'est pas un projet officiel et il n'est affilié ni à Activision, ni à Infinity Ward, ni aux auteurs du dépôt d'origine. Il ne contient aucun fichier du jeu.

## Modifications de ce fork

- **Des compilations qui fonctionnent à nouveau :** les compilations Linux et MinGW, cassées depuis que l'original n'était plus compilé qu'avec MSVC, fonctionnent de nouveau ; `scripts/build_check.py` compile Linux et MinGW (le serveur dédié et, avec SDL2, le client), en 1.3 et en 1.0, en une seule fois.
- **Compilé en C++ :** sous Linux et MinGW, tout le moteur se compile en C++ (gnu++17), avec les en-têtes communs rendus valides en C++, sans changer ce qu'il fait ; des vérifications protègent les noms C dont dépendent les blobs de données et la disposition des structures du binaire d'origine.
- **La machine virtuelle de scripts de CoD2rev-Plus :** la machine virtuelle GSC est celle de CoD2rev-Plus, copiée sans modification dans `src/script`, avec un pont C/C++ généré. Le serveur accueille maintenant deux joueurs, les déconnexions, les reconnexions et `clientkick`, là où la machine virtuelle reconstruite plantait dès que le deuxième joueur arrivait.
- **Le serveur de CoD2rev-Plus :** le serveur dédié est compilé avec le serveur de CoD2rev-Plus, copié sans modification dans `src/server`, avec un pont généré dans lequel chaque appel entre les deux est vérifié par rapport à l'ABI du moteur. Les parties locales des clients Linux et Windows (MinGW) tournent aussi dessus, et tout le client trace avec sa collision : un client crée une partie et d'autres la rejoignent. Avec lui, le serveur refuse d'envoyer les fichiers `.iwd` d'origine et ceux propres au serveur. Son game et son bgame (`src/game`, `src/bgame`) viennent avec lui, et avec eux `addip` et la rotation de `games_mp.log` ; dans le client, cgame garde le bgame du moteur (`src/PC/bgame`), dont les globales prennent le préfixe `eng_`, et qui prédit désormais les mouvements, l'escalade et l'arme du joueur avec le code de CoD2rev-Plus : pour les mêmes mouvements, sa prédiction se trompe (retours en arrière) environ huit fois moins souvent. Depuis, le serveur dédié tourne aussi sur son noyau : `universal`, `qcommon` et le code de plateforme (`src/universal`, `src/qcommon`, `src/unix`, `src/win32`), qui apportent la rotation et l'horodatage du journal de la console, `net_strictPort` et son rapport de plantage. Avec son xanim (`src/xanim`), tout ce que le serveur dédié exécute est du code de CoD2rev-Plus.
- **La libcod de CoD2rev-Plus :** le serveur dédié est compilé avec libcod, copiée sans modification dans `src/libcod` avec les modules qu'active CoD2rev-Plus (tous sauf MySQL) : ses fonctions de script, les bots, `sv_master1` à `sv_master5`, HTTP et WebSocket (Mbed TLS), SQLite et JSON. Il passe les tests de fumée, de bots et HTTP de CoD2rev-Plus en 1.3 et 1.0 ; les parties locales du client l'ont aussi, sur leur serveur local (Linux et Windows) : le test des bots y passe également, et le test HTTP sous Linux.
- **Un serveur dédié sans les bibliothèques du client :** sous Linux, `cod2_lnxded` n'a besoin que de libc, libm, libz et libstdc++, et non de GL, X11, SDL2 ou curl.
- **Le vrai éclairage du client Windows :** le client Windows dessine avec Direct3D 9 et compile les shaders du jeu lui-même avec le d3dx9 du système, comme le moteur de rendu d'origine, au lieu des shaders de remplacement de la couche GL du portage Mac (un monde et un ciel blancs). Le soleil, les lightmaps, l'éclairage des modèles et le ciel apparaissent comme dans l'original : sous Wine, à côté du `CoD2MP_s.exe` d'origine aux mêmes endroits, les deux dessinent presque la même image. En chemin, plusieurs tables du moteur de rendu que la reconstruction avait laissées à zéro ont été remplies d'après l'original (l'arme en vue subjective et les joueurs étaient noirs, les modèles statiques aussi), les textures se chargent de nouveau en taille réelle, et `r_aspectRatio` et le plein écran fonctionnent.
- **Corrections :** le serveur répondait sur le mauvais port, ne voyait aucun `.iwd` sur les lecteurs Windows sous WSL et plantait à chaque kick ; il annonçait le protocole 118 dans la compilation 1.0 ; la compilation Linux en C++ reste sur glibc 2.34 ; `getAnimLength` et trois autres fonctions intégrées d'animation ne font plus planter le serveur. Le quatrième ou cinquième chargement de carte faisait planter le serveur : le script d'animation grossissait à chaque carte, et l'erreur qu'il levait depuis rcon corrompait la pile. Le serveur 1.0 écrasait ses propres configstrings et aucun client 1.0 ne pouvait se connecter. Les messages fragmentés utilisaient les mauvais offsets dans les deux versions, et le port d'une adresse réseau était là où Quake III le place, et non là où CoD2 le place. Les fichiers `.iwd` du propre dossier du serveur n'étaient pas cherchés, et les fonctions intégrées qui prennent un joueur n'en voyaient aucun avec la machine virtuelle de CoD2rev-Plus. Les entités étaient liées aux secteurs du monde sous le mauvais numéro : en délier certaines faisait planter le serveur, et les traces testaient l'entité précédente. Dans le client, toute entité découpée comme une boîte restait hors des secteurs du monde et le test de boîte ratait les segments qui la traversaient : les balles traversaient les joueurs d'une partie locale ; et depuis une modification amont pour x64, aucune animation de plus d'une image ne jouait en 32 bits, ce qui laissait les joueurs et l'arme à la première personne dans une pose cassée. Le client plantait en voyant mourir un joueur, et sa prédiction de mouvement pouvait déborder la pile contre les murs ; il envoyait un saut, un changement de posture ou d'arme une commande trop tard et tronquait la vitesse qu'il prédit ; les animations de la MG42, le remplissage et les largeurs de texte de l'UI et une douzaine d'autres appels du moteur passaient des arguments ou lisaient des résultats d'un autre type que celui de la fonction, et les effets joués selon une direction recevaient des axes faux. Aucun effet n'était dessiné (impacts, explosions, la fumée des cartes) : le code des effets lisait ses flags, ses échelles de courbe, ses couleurs et ses vitesses autrement que le binaire d'origine, programmait les parties différées pour un instant qui n'arrivait jamais, laissait chaque particule à l'origine du monde sans second rayon, et rien ne les dessinait. Un configstring modifié déconnectait le client et le tableau des scores restait vide (les lettres des commandes du serveur avaient d'autres sens), une grenade lancée restait suspendue en l'air (aucune entité ne bougeait entre ses événements), et la neige ne tombait qu'à l'abri (la carte des extérieurs était inversée).
- **Tests :** le smoke test de CoD2rev-Plus, sans modification, contre ce serveur, avec la liste des vérifications qu'il ne passe pas encore ; des tests à deux joueurs, de chargement de cartes, d'une erreur drop depuis rcon et des fonctions intégrées d'animation ; et des vérifications de la disposition des structures, des noms C, de l'ABI du pont et des fichiers copiés de CoD2rev-Plus. Des tests tirent sur un joueur à l'horizontale puis vers le bas et vérifient que chaque tir touche le haut du corps ou les jambes, et qu'un couteau et une explosion atteignent un joueur, sur le serveur dédié et dans une partie locale.

La documentation technique se trouve dans [docs/README.md](docs/README.md) (en anglais).

## Versions du jeu

- **La version se choisit à la compilation.** Par défaut, le moteur est compilé en 1.3 (protocole 118) ; avec `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10`, il est compilé en 1.0 (protocole 115).
- **Les deux serveurs dédiés fonctionnent pour les tests :** en 1.3 comme en 1.0, le serveur charge, change et redémarre des cartes, répond à `getinfo`, `getstatus` et rcon, et accueille deux joueurs qui rejoignent, partent, reviennent et se font kicker.
- **Les parties locales fonctionnent pour les tests :** en 1.3 comme en 1.0, un client crée une partie sur le serveur de CoD2rev-Plus, un autre client la rejoint, et les deux restent après un `map_restart` et un changement de carte (`scripts/listen_test.py`, avec votre clé CD). Un tir touche la partie du corps qu'il vise (`scripts/hit_location_test.py`).

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

### Windows (MSVC, natif)

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

### Options CMake

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

## Exécution

Le moteur a besoin des fichiers du jeu d'origine (le dossier `main` avec ses fichiers `.iwd`), qui ne sont pas dans ce dépôt. Il faut posséder sa propre copie de Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

Sous Windows :

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` garde le serveur sur le réseau local ; c'est un serveur de test, pas encore un serveur à lancer sur la liste publique.

## Tests

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

## Structure du dépôt

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

## Avis

Ceci est une reconstruction indépendante, au niveau du code source, du moteur de Call of Duty 2. Elle n'est ni affiliée à, ni autorisée, ni parrainée, ni approuvée par Activision Publishing, Inc., Infinity Ward ou l'une quelconque de leurs sociétés affiliées.

« Call of Duty » et « Call of Duty 2 » sont des marques d'Activision Publishing, Inc. Elles ne sont utilisées dans ce dépôt qu'à des fins d'identification et d'interopérabilité, pour décrire ce que le code reconstruit. Aucun droit n'est revendiqué sur ces marques.

Ce projet ne fournit pas de données de jeu protégées par le droit d'auteur et n'aide pas à les obtenir. Pour l'utiliser avec le véritable contenu du jeu, vous devez fournir les fichiers de données d'une copie de Call of Duty 2 que vous possédez légalement.

Le code source du moteur reconstruit est une œuvre dérivée créée à des fins de préservation, d'interopérabilité, de recherche et d'éducation. Il est fourni tel quel, sans garantie d'aucune sorte, expresse ou implicite.

Si vous êtes titulaire de droits et estimez que quelque chose ici ne devrait pas être distribué, veuillez ouvrir une issue ou contacter le mainteneur, et la question sera traitée rapidement.

## Crédits

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), la reconstruction du moteur
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), durcissement et corrections réseau
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) et [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), le serveur dédié et la machine virtuelle de scripts
- id Software
- Activision / Infinity Ward

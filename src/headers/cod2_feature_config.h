#ifndef COD2_FEATURE_CONFIG_H
#define COD2_FEATURE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(COD2_PATCH_10)
  #define COD2_IS_PATCH_13 0
#else
  #define COD2_IS_PATCH_13 1
#endif

/* network protocol of the game version: 115 for 1.0, 118 for 1.3 */
#if COD2_IS_PATCH_13
  #define COD2_PROTOCOL_VERSION 118
#else
  #define COD2_PROTOCOL_VERSION 115
#endif

#ifndef COD2_FEATURE_WWW_DOWNLOAD
  #define COD2_FEATURE_WWW_DOWNLOAD COD2_IS_PATCH_13
#endif

#ifndef COD2_FEATURE_PUNKBUSTER
  #define COD2_FEATURE_PUNKBUSTER COD2_IS_PATCH_13
#endif

#ifndef COD2_FEATURE_SCRIPT_DEBUGGER
  #define COD2_FEATURE_SCRIPT_DEBUGGER 0
#endif

#ifndef COD2_FEATURE_SCRIPT_DEBUGGER_REMOTE
  #define COD2_FEATURE_SCRIPT_DEBUGGER_REMOTE COD2_FEATURE_SCRIPT_DEBUGGER
#endif
#ifndef COD2_FEATURE_SCRIPT_DEBUGGER_UI
  #define COD2_FEATURE_SCRIPT_DEBUGGER_UI COD2_FEATURE_SCRIPT_DEBUGGER
#endif

#ifndef COD2_FEATURE_MODERN_LIBS
  #define COD2_FEATURE_MODERN_LIBS 0
#endif

#ifndef COD2_FEATURE_GAMEPAD
  #define COD2_FEATURE_GAMEPAD 0
#endif
#ifndef COD2_FEATURE_SPLITSCREEN
  #define COD2_FEATURE_SPLITSCREEN 0
#endif
#ifndef COD2_FEATURE_RUMBLE
  #define COD2_FEATURE_RUMBLE COD2_FEATURE_GAMEPAD
#endif

#ifndef COD2_FEATURE_NET_HARDENING
  #define COD2_FEATURE_NET_HARDENING 1
#endif

#if COD2_IS_PATCH_13
  #define COD2_VERSION_SHORT "1.3"
  #define COD2_VERSION_DATE  "May 01 2006"
#else
  #define COD2_VERSION_SHORT "1.0"
  #define COD2_VERSION_DATE  "Apr 18 2006"
#endif

/* what the console and the version dvar show; shortversion stays the bare
 * number, which servers compare */
#ifdef COD2_PLUS
  #define COD2_VERSION_NAME COD2_VERSION_SHORT " Plus"
#else
  #define COD2_VERSION_NAME COD2_VERSION_SHORT
#endif
#if defined(__EMSCRIPTEN__)
  #define COD2_BUILD_ARCH "web"
#elif defined(_WIN32) && (defined(_WIN64) || defined(__x86_64__))
  #define COD2_BUILD_ARCH "win-x64"
#elif defined(_WIN32)
  #define COD2_BUILD_ARCH "win-x86"
#elif defined(__x86_64__)
  #define COD2_BUILD_ARCH "linux-x64"
#else
  #define COD2_BUILD_ARCH "linux-x86"
#endif

#if COD2_IS_PATCH_13
  #define MAX_GAMESTATE_CHARS      0x20000
  #define MAX_MSGLEN               0x20000
#else
  #define MAX_GAMESTATE_CHARS      0x3e80
  #define MAX_MSGLEN               0x4000
#endif

#define MAX_MSG_DECOMPRESS_BYTES   MAX_MSGLEN

#if defined(_XENON) || defined(COD2_PLATFORM_XENON)
  #define COD2_IS_XENON 1
#else
  #define COD2_IS_XENON 0
#endif

#ifdef __cplusplus
}
#endif
#endif

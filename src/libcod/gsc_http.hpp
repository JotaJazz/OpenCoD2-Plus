#ifndef _GSC_HTTP_HPP_
#define _GSC_HTTP_HPP_

/* gsc functions */
#include "gsc.hpp"

void gsc_http_register_dvars();
void gsc_http_frame();
void gsc_http_cleanup_on_level_load();
// a detached worker thread with the stack the network code needs
bool Http_StartThread(void *(*func)(void *), void *arg);

void gsc_http_fetch();
void gsc_websocket_connect();
void gsc_websocket_sendtext();
void gsc_websocket_close();

#endif

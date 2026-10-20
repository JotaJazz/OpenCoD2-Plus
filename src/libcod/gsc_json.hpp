/*
 * Ported from zk_libcod (https://github.com/ibuddieat/zk_libcod), code/gsc_json.* (JSON support backed by yyjson, src/libcod/yyjson),
 * under its MIT license:
 *
 * MIT License
 *
 * Copyright (c) 2022 iBuddie
 * Copyright (c) 2022 Maxim Kuzmichev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _GSC_JSON_HPP_
#define _GSC_JSON_HPP_

#include "gsc.hpp"

void gsc_json_register_dvars(void);
void gsc_json_cleanup_on_spawn_server(void);

void gsc_json_parse();
void gsc_json_stringify();
void gsc_json_load();
void gsc_json_save();
void gsc_json_load_async();
void gsc_json_save_async();
void gsc_json_async_done();
void gsc_json_async_result();

#endif

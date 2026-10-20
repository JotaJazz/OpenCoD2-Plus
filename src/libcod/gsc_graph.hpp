/*
 * Ported from zk_libcod (https://github.com/ibuddieat/zk_libcod), code/gsc_graph.*,
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

#ifndef _GSC_GRAPH_HPP_
#define _GSC_GRAPH_HPP_

#include "gsc.hpp"

#if LIBCOD_COMPILE_GRAPH == 1

/*
 * Waypoint-graph subsystem with A* pathfinding for server-side AI.
 * The A* shape (binary-heap open list with lazy deletion) follows the approach
 * popularized by Justin Heyes-Jones' astar-algorithm-cpp:
 * https://github.com/justinhj/astar-algorithm-cpp
 *
 * All graph state is owned by the main server thread - no locking, by design.
 */

void gsc_graph_create();
void gsc_graph_find_path();
void gsc_graph_find_closest_node();
void gsc_graph_remove();
void gsc_graph_removeall();
void gsc_graph_add_node();
void gsc_graph_add_edge();
void gsc_graph_get_node_origin();
void gsc_graph_get_node_count();
void gsc_graph_get_edge_count();
void gsc_graph_get_stats();
void gsc_graph_find_closest_edge();
void gsc_graph_get_all_nodes();
void gsc_graph_get_all_edges();
void gsc_graph_get_edge_properties();
void gsc_graph_set_edge_cost();
void gsc_graph_set_edge_type();
void gsc_graph_remove_edge();
void gsc_graph_get_node_properties();
void gsc_graph_set_node_type();
void gsc_graph_set_node_origin();
void gsc_graph_get_node_ids_accessible_from();
void gsc_graph_get_node_ids_accessible_to();
void gsc_graph_remove_node();
void gsc_graph_precompute_paths_to_node();
void gsc_graph_save();
void gsc_graph_load();
void gsc_graph_autodiscover();
void gsc_graph_autodiscover_ex();

void gsc_graph_cleanup_on_spawn_server(void);

#endif

#endif

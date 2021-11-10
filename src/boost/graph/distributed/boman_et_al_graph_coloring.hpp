// Copyright (C) 2005-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_BOMAN_ET_AL_GRAPH_COLORING_HPP
#define BOOST_GRAPH_DISTRIBUTED_BOMAN_ET_AL_GRAPH_COLORING_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <functional>
#include <vector>
#include <utility>
#include <boost/graph/iteration_macros.hpp>
#include <boost/optional.hpp>
#include <boost/assert.hpp>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/graph/properties.hpp>

#ifdef PBGL_ACCOUNTING
#  include <boost/graph/accounting.hpp>
#endif // PBGL_ACCOUNTING

namespace boost { namespace graph { namespace distributed {

/**************************************************************************
 * This source file implements the distributed graph coloring algorithm   *
 * by Boman et al in:                                                     *
 *                                                                        *
 *   Erik G. Boman, Doruk Bozdag, Umit Catalyurek, Assefaw H. Gebremedhin,*
 *   and Fredrik Manne. A Scalable Parallel Graph Coloring Algorithm for  *
 *   Distributed Memory Computers. [unpublished preprint?]                *
 *                                                                        *
 **************************************************************************/

#ifdef PBGL_ACCOUNTING
struct boman_et_al_graph_coloring_stats_t
{
  /* The size of the blocks to step through (i.e., the parameter s). */
  std::size_t block_size;
  
  /* Total wall-clock time used by the algorithm.*/
  accounting::time_type execution_time;

  /* The number of conflicts that occurred during execution. */
  std::size_t conflicts;

  /* The number of supersteps. */
  std::size_t supersteps;

  /* The number of colors used. */
  std::size_t num_colors;

  template<typename OutputStream>
  void print(OutputStream& out)
  {
    out << "Problem = \"Coloring\"\n"
        << "Algorithm = \"Boman et al\"\n"
        << "Function = boman_et_al_graph_coloring\n"
        << "(P) Block size = " << block_size << "\n"
        << "Wall clock time = " << accounting::print_time(execution_time) 
        << "\nConflicts = " << conflicts << "\n"
        << "Supersteps = " << supersteps << "\n"
        << "(R) Colors = " << num_colors << "\n";
  }
};

static boman_et_al_graph_coloring_stats_t boman_et_al_graph_coloring_stats;
#endif

namespace detail {
  template<typename T>
  struct graph_coloring_reduce
  {
    BOOST_STATIC_CONSTANT(bool, non_default_resolver = true);

    template<typename Key>
    T operator()(const Key&) const { return (std::numeric_limits<T>::max)(); }

    template<typename Key> T operator()(const Key&, T, T y) const { return y; }
  };
}

template<typename Color>
struct first_fit_color
{
  template<typename T>
  Color operator()(const std::vector<T>& marked, T marked_true)
  {
    Color k = 0;
    while (k < (Color)marked.size() && marked[k] == marked_true)
      ++k;
    return k;
  }
};

template<typename DistributedGraph, typename ColorMap, typename ChooseColor,
         typename VertexOrdering, typename VertexIndexMap>
typename property_traits<ColorMap>::value_type
boman_et_al_graph_coloring
  (const DistributedGraph& g,
   ColorMap color,
   typename graph_traits<DistributedGraph>::vertices_size_type s,
   ChooseColor choose_color,
   VertexOrdering ordering, VertexIndexMap vertex_index)
{
  using namespace boost::graph::parallel;
  using boost::parallel::all_reduce;

  typename property_map<DistributedGraph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  typedef typename process_group_type<DistributedGraph>::type 
    process_group_type;
  typedef typename process_group_type::process_id_type process_id_type;
  typedef typename graph_traits<DistributedGraph>::vertex_descriptor Vertex;
  typedef typename graph_traits<DistributedGraph>::vertices_size_type 
    vertices_size_type;
  typedef typename property_traits<ColorMap>::value_type color_type;
  typedef unsigned long long iterations_type;
  typedef typename std::vector<Vertex>::iterator vertex_set_iterator;
  typedef std::pair<Vertex, color_type> message_type;

#ifdef PBGL_ACCOUNTING
  boman_et_al_graph_coloring_stats.block_size = s;
  boman_et_al_graph_coloring_stats.execution_time = accounting::get_time();
  boman_et_al_graph_coloring_stats.conflicts = 0;
  boman_et_al_graph_coloring_stats.supersteps = 0;
#endif

  // Initialize color map
  color_type no_color = (std::numeric_limits<color_type>::max)();
  BGL_FORALL_VERTICES_T(v, g, DistributedGraph)
    put(color, v, no_color);
  color.set_reduce(detail::graph_coloring_reduce<color_type>());
  
  // Determine if we'll be using synchronous or asynchronous communication.
  typedef typename process_group_type::communication_category
    communication_category;
  static const bool asynchronous = 
    is_convertible<communication_category, boost::parallel::immediate_process_group_tag>::value;
  process_group_type pg = process_group(g);

  // U_i <- V_i
  std::vector<Vertex> vertices_to_color(vertices(g).first, vertices(g).second);

  iterations_type iter_num = 1, outer_iter_num = 1;
  std::vector<iterations_type> marked;
  std::vector<iterations_type> marked_conflicting(num_vertices(g), 0);
  std::vector<bool> sent_to_processors;

  std::size_t rounds = vertices_to_color.size() / s 
    + (vertices_to_color.size() % s == 0? 0 : 1);
  rounds = all_reduce(pg, rounds, boost::parallel::maximum<std::size_t>());

#ifdef PBGL_GRAPH_COLORING_DEBUG
  std::cerr << "Number of rounds = " << rounds << std::endl;
#endif

  while (rounds > 0) {
    if (!vertices_to_color.empty()) {
      // Set of conflicting vertices
      std::vector<Vertex> conflicting_vertices;

      vertex_set_iterator first = vertices_to_color.begin();
      while (first != vertices_to_color.end()) {
        // For each subset of size s (or smaller for the last subset)
        vertex_set_iterator start = first;
        for (vertices_size_type counter = s; 
             first != vertices_to_color.end() && counter > 0;
             ++first, --counter) {
          // This vertex hasn't been sent to anyone yet
          sent_to_processors.assign(num_processes(pg), false);
          sent_to_processors[process_id(pg)] = true;

          // Mark all of the colors that we see
          BGL_FORALL_OUTEDGES_T(*first, e, g, DistributedGraph) {
            color_type k = get(color, target(e, g));
            if (k != no_color) {
              if (k >= (color_type)marked.size()) marked.resize(k + 1, 0);
              marked[k] = iter_num;
            }
          }

          // Find a color for this vertex
          put(color, *first, choose_color(marked, iter_num));

#ifdef PBGL_GRAPH_COLORING_DEBUG
          std::cerr << "Chose color " << get(color, *first) << " for vertex "
                    << *first << std::endl;
#endif

          // Send this vertex's color to the owner of the edge target.
          BGL_FORALL_OUTEDGES_T(*first, e, g, DistributedGraph) {
            if (!sent_to_processors[get(owner, target(e, g))]) {
              send(pg, get(owner, target(e, g)), 17, 
                   message_type(source(e, g), get(color, source(e, g))));
              sent_to_processors[get(owner, target(e, g))] = true;
            }
          }

          ++iter_num;
        }

        // Synchronize for non-immediate process groups.
        if (!asynchronous) { 
          --rounds;
          synchronize(pg);
        }

        // Receive boundary colors from other processors
        while (optional<std::pair<process_id_type, int> > stp = probe(pg)) {
          BOOST_ASSERT(stp->second == 17);
          message_type msg;
          receive(pg, stp->first, stp->second, msg);
          cache(color, msg.first, msg.second);
#ifdef PBGL_GRAPH_COLORING_DEBUG
          std::cerr << "Cached color " << msg.second << " for vertex " 
                    << msg.first << std::endl;
#endif
        }

        // Compute the set of conflicting vertices
        // [start, first) contains all vertices in this subset
        for (vertex_set_iterator vi = start; vi != first; ++vi) {
          Vertex v = *vi;
          BGL_FORALL_OUTEDGES_T(v, e, g, DistributedGraph) {
            Vertex w = target(e, g);
            if (get(owner, w) != process_id(pg) // boundary vertex
                && marked_conflicting[get(vertex_index, v)] != outer_iter_num
                && get(color, v) == get(color, w)
                && ordering(v, w)) {
              conflicting_vertices.push_back(v);
              marked_conflicting[get(vertex_index, v)] = outer_iter_num;
              put(color, v, no_color);
#ifdef PBGL_GRAPH_COLORING_DEBUG
              std::cerr << "Vertex " << v << " has a conflict with vertex "
                        << w << std::endl;
#endif
              break;
            }
          }
        }

#ifdef PBGL_ACCOUNTING
        boman_et_al_graph_coloring_stats.conflicts += 
          conflicting_vertices.size();
#endif
      }

      if (asynchronous) synchronize(pg);
      else {
        while (rounds > 0) {
          synchronize(pg);
          --rounds;
        }
      }
      conflicting_vertices.swap(vertices_to_color);
      ++outer_iter_num;
    } else {
      if (asynchronous) synchronize(pg);
      else {
        while (rounds > 0) {
          synchronize(pg);
          --rounds;
        }
      }
    }

    // Receive boundary colors from other processors
    while (optional<std::pair<process_id_type, int> > stp = probe(pg)) {
      BOOST_ASSERT(stp->second == 17);
      message_type msg;
      receive(pg, stp->first, stp->second, msg);
      cache(color, msg.first, msg.second);
    }

    rounds = vertices_to_color.size() / s 
      + (vertices_to_color.size() % s == 0? 0 : 1);
    rounds = all_reduce(pg, rounds, boost::parallel::maximum<std::size_t>());

#ifdef PBGL_ACCOUNTING
    ++boman_et_al_graph_coloring_stats.supersteps;
#endif
  }

  // Determine the number of colors used.
  color_type num_colors = 0;
  BGL_FORALL_VERTICES_T(v, g, DistributedGraph) {
    color_type k = get(color, v);
    BOOST_ASSERT(k != no_color);
    if (k != no_color) {
      if (k >= (color_type)marked.size()) marked.resize(k + 1, 0); // TBD: perf?
      if (marked[k] != iter_num) {
        marked[k] = iter_num;
        ++num_colors;
      }
    }
  }

  num_colors = 
    all_reduce(pg, num_colors, boost::parallel::maximum<color_type>());


#ifdef PBGL_ACCOUNTING
  boman_et_al_graph_coloring_stats.execution_time = 
    accounting::get_time() - boman_et_al_graph_coloring_stats.execution_time;
  
  boman_et_al_graph_coloring_stats.conflicts = 
    all_reduce(pg, boman_et_al_graph_coloring_stats.conflicts,
               std::plus<color_type>());
  boman_et_al_graph_coloring_stats.num_colors = num_colors;
#endif

  return num_colors;
}


template<typename DistributedGraph, typename ColorMap, typename ChooseColor, 
         typename VertexOrdering>
inline typename property_traits<ColorMap>::value_type
boman_et_al_graph_coloring
  (const DistributedGraph& g, ColorMap color,
   typename graph_traits<DistributedGraph>::vertices_size_type s,
   ChooseColor choose_color, VertexOrdering ordering)
{
  return boman_et_al_graph_coloring(g, color, s, choose_color, ordering, 
                                    get(vertex_index, g));
}

template<typename DistributedGraph, typename ColorMap, typename ChooseColor>
inline typename property_traits<ColorMap>::value_type
boman_et_al_graph_coloring
  (const DistributedGraph& g,
   ColorMap color,
   typename graph_traits<DistributedGraph>::vertices_size_type s,
   ChooseColor choose_color)
{
  typedef typename graph_traits<DistributedGraph>::vertex_descriptor
    vertex_descriptor;
  return boman_et_al_graph_coloring(g, color, s, choose_color,
                                    std::less<vertex_descriptor>());
}

template<typename DistributedGraph, typename ColorMap>
inline typename property_traits<ColorMap>::value_type
boman_et_al_graph_coloring
  (const DistributedGraph& g,
   ColorMap color,
   typename graph_traits<DistributedGraph>::vertices_size_type s = 100)
{
  typedef typename property_traits<ColorMap>::value_type Color;
  return boman_et_al_graph_coloring(g, color, s, first_fit_color<Color>());
}

} } } // end namespace boost::graph::distributed

namespace boost { namespace graph {
using distributed::boman_et_al_graph_coloring;
} } // end namespace boost::graph

#endif // BOOST_GRAPH_DISTRIBUTED_BOMAN_ET_AL_GRAPH_COLORING_HPP

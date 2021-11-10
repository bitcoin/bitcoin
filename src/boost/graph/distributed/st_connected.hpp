// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP
#define BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/distributed/queue.hpp>
#include <boost/pending/queue.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <utility>
#include <boost/optional.hpp>

namespace boost { namespace graph { namespace distributed {

namespace detail {
  struct pair_and_or 
  {
    std::pair<bool, bool> 
    operator()(std::pair<bool, bool> x, std::pair<bool, bool> y) const
    {
      return std::pair<bool, bool>(x.first && y.first,
                                   x.second || y.second);
    }
  };

} // end namespace detail

template<typename DistributedGraph, typename ColorMap, typename OwnerMap>
bool 
st_connected(const DistributedGraph& g, 
             typename graph_traits<DistributedGraph>::vertex_descriptor s,
             typename graph_traits<DistributedGraph>::vertex_descriptor t,
             ColorMap color, OwnerMap owner)
{
  using boost::graph::parallel::process_group;
  using boost::graph::parallel::process_group_type;
  using boost::parallel::all_reduce;

  typedef typename property_traits<ColorMap>::value_type Color;
  typedef color_traits<Color> ColorTraits;
  typedef typename process_group_type<DistributedGraph>::type ProcessGroup;
  typedef typename ProcessGroup::process_id_type ProcessID;
  typedef typename graph_traits<DistributedGraph>::vertex_descriptor Vertex;

  // Set all vertices to white (unvisited)
  BGL_FORALL_VERTICES_T(v, g, DistributedGraph)
    put(color, v, ColorTraits::white());

  // "color" plays the role of a color map, with no synchronization.
  set_property_map_role(vertex_color, color);
  color.set_consistency_model(0);

  // Vertices found from the source are grey
  put(color, s, ColorTraits::gray());

  // Vertices found from the target are green
  put(color, t, ColorTraits::green());

  ProcessGroup pg = process_group(g);
  ProcessID rank = process_id(pg);

  // Build a local queue
  queue<Vertex> Q;
  if (get(owner, s) == rank) Q.push(s);
  if (get(owner, t) == rank) Q.push(t);

  queue<Vertex> other_Q;

  while (true) {
    bool found = false;

    // Process all vertices in the local queue
    while (!found && !Q.empty()) {
      Vertex u = Q.top(); Q.pop();
      Color u_color = get(color, u);

      BGL_FORALL_OUTEDGES_T(u, e, g, DistributedGraph) {
        Vertex v = target(e, g);
        Color v_color = get(color, v);
        if (v_color == ColorTraits::white()) {
          // We have not seen "v" before; mark it with the same color as u
          Color u_color = get(color, u);
          put(color, v, u_color);

          // Either push v into the local queue or send it off to its
          // owner.
          ProcessID v_owner = get(owner, v);
          if (v_owner == rank) 
            other_Q.push(v);
          else
            send(pg, v_owner, 0, 
                 std::make_pair(v, u_color == ColorTraits::gray()));
        } else if (v_color != ColorTraits::black() && u_color != v_color) {
          // Colors have collided. We're done!
          found = true;
          break;
        }
      }

      // u is done, so mark it black
      put(color, u, ColorTraits::black());
    }

    // Ensure that all transmitted messages have been received.
    synchronize(pg);

    // Move all of the send-to-self values into the local Q.
    other_Q.swap(Q);

    if (!found) {
      // Receive all messages
      while (optional<std::pair<ProcessID, int> > msg = probe(pg)) {
        std::pair<Vertex, bool> data;
        receive(pg, msg->first, msg->second, data);
        
        // Determine the colors of u and v, the source and target
        // vertices (v is local).
        Vertex v = data.first;
        Color v_color = get(color, v);
        Color u_color = data.second? ColorTraits::gray() : ColorTraits::green();
        if (v_color == ColorTraits::white()) {
          // v had no color before, so give it u's color and push it
          // into the queue.
          Q.push(v);
          put(color, v, u_color);
        } else if (v_color != ColorTraits::black() && u_color != v_color) {
          // Colors have collided. We're done!
          found = true;
          break;
        }
      }
    }

    // Check if either all queues are empty or 
    std::pair<bool, bool> results = all_reduce(pg, 
            boost::parallel::detail::make_untracked_pair(Q.empty(), found),
            detail::pair_and_or());

    // If someone found the answer, we're done!
    if (results.second)
      return true;

    // If all queues are empty, we're done.
    if (results.first)
      return false;
  }
}

template<typename DistributedGraph, typename ColorMap>
inline bool 
st_connected(const DistributedGraph& g, 
             typename graph_traits<DistributedGraph>::vertex_descriptor s,
             typename graph_traits<DistributedGraph>::vertex_descriptor t,
             ColorMap color)
{
  return st_connected(g, s, t, color, get(vertex_owner, g));
}

template<typename DistributedGraph>
inline bool 
st_connected(const DistributedGraph& g, 
             typename graph_traits<DistributedGraph>::vertex_descriptor s,
             typename graph_traits<DistributedGraph>::vertex_descriptor t)
{
  return st_connected(g, s, t, 
                      make_two_bit_color_map(num_vertices(g),
                                             get(vertex_index, g)));
}

} } } // end namespace boost::graph::distributed

#endif // BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP

// Copyright (C) 2007 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

/**************************************************************************
 * This source file implements the Delta-stepping algorithm:              *
 *                                                                        *
 *   Ulrich Meyer and Peter Sanders. Parallel Shortest Path for Arbitrary *
 *   Graphs. In Proceedings from the 6th International Euro-Par           *
 *   Conference on Parallel Processing, pages 461--470, 2000.             *
 *                                                                        * 
 *   Ulrich Meyer, Peter Sanders: [Delta]-stepping: A Parallelizable      *
 *   Shortest Path Algorithm. J. Algorithms 49(1): 114-152, 2003.         *
 *                                                                        *
 * There are several potential optimizations we could still perform for   *
 * this algorithm implementation:                                         *
 *                                                                        *
 *   - Implement "shortcuts", which bound the number of reinsertions      *
 *     in a single bucket (to one). The computation of shortcuts looks    *
 *     expensive in a distributed-memory setting, but it could be         *
 *     ammortized over many queries.                                      *
 *                                                                        *
 *   - The size of the "buckets" data structure can be bounded to         *
 *     max_edge_weight/delta buckets, if we treat the buckets as a        *
 *     circular array.                                                    *
 *                                                                        *
 *   - If we partition light/heavy edges ahead of time, we could improve  *
 *     relaxation performance by only iterating over the right portion    *
 *     of the out-edge list each time.                                    *
 *                                                                        *
 *   - Implement a more sophisticated algorithm for guessing "delta",     *
 *     based on the shortcut-finding algorithm.                           *
 **************************************************************************/
#ifndef BOOST_GRAPH_DELTA_STEPPING_SHORTEST_PATHS_HPP
#define BOOST_GRAPH_DELTA_STEPPING_SHORTEST_PATHS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <limits>
#include <list>
#include <vector>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/distributed/detail/dijkstra_shortest_paths.hpp>
#include <utility> // for std::pair
#include <functional> // for std::logical_or
#include <boost/graph/parallel/algorithm.hpp> // for all_reduce
#include <cassert>
#include <algorithm> // for std::min, std::max
#include <boost/graph/parallel/simple_trigger.hpp>

#ifdef PBGL_DELTA_STEPPING_DEBUG
#  include <iostream> // for std::cerr
#endif

namespace boost { namespace graph { namespace distributed {

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
class delta_stepping_impl {
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::degree_size_type Degree;
  typedef typename property_traits<EdgeWeightMap>::value_type Dist;
  typedef typename boost::graph::parallel::process_group_type<Graph>::type 
    ProcessGroup;

  typedef std::list<Vertex> Bucket;
  typedef typename Bucket::iterator BucketIterator;
  typedef typename std::vector<Bucket*>::size_type BucketIndex;

  typedef detail::dijkstra_msg_value<DistanceMap, PredecessorMap> MessageValue;

  enum { 
    // Relax a remote vertex. The message contains a pair<Vertex,
    // MessageValue>, the first part of which is the vertex whose
    // tentative distance is being relaxed and the second part
    // contains either the new distance (if there is no predecessor
    // map) or a pair with the distance and predecessor.
    msg_relax 
  };

public:
  delta_stepping_impl(const Graph& g,
                      PredecessorMap predecessor, 
                      DistanceMap distance, 
                      EdgeWeightMap weight,
                      Dist delta);

  delta_stepping_impl(const Graph& g,
                      PredecessorMap predecessor, 
                      DistanceMap distance, 
                      EdgeWeightMap weight);

  void run(Vertex s);

private:
  // Relax the edge (u, v), creating a new best path of distance x.
  void relax(Vertex u, Vertex v, Dist x);

  // Synchronize all of the processes, by receiving all messages that
  // have not yet been received.
  void synchronize();

  // Handle a relax message that contains only the target and
  // distance. This kind of message will be received when the
  // predecessor map is a dummy_property_map.
  void handle_relax_message(Vertex v, Dist x) { relax(v, v, x); }

  // Handle a relax message that contains the source (predecessor),
  // target, and distance. This kind of message will be received when
  // the predecessor map is not a dummy_property_map.
  void handle_relax_message(Vertex v, const std::pair<Dist, Vertex>& p)
  { relax(p.second, v, p.first); }
  
  // Setup triggers for msg_relax messages
  void setup_triggers();

  void handle_msg_relax(int /*source*/, int /*tag*/,
                        const std::pair<Vertex, typename MessageValue::type>& data,
                        trigger_receive_context)
  { handle_relax_message(data.first, data.second); }

  const Graph& g;
  PredecessorMap predecessor;
  DistanceMap distance;
  EdgeWeightMap weight;
  Dist delta;
  ProcessGroup pg;
  typename property_map<Graph, vertex_owner_t>::const_type owner;
  typename property_map<Graph, vertex_local_t>::const_type local;

  // A "property map" that contains the position of each vertex in
  // whatever bucket it resides in.
  std::vector<BucketIterator> position_in_bucket;

  // Bucket data structure. The ith bucket contains all local vertices
  // with (tentative) distance in the range [i*delta,
  // (i+1)*delta). 
  std::vector<Bucket*> buckets;

  // This "dummy" list is used only so that we can initialize the
  // position_in_bucket property map with non-singular iterators. This
  // won't matter for most implementations of the C++ Standard
  // Library, but it avoids undefined behavior and allows us to run
  // with library "debug modes".
  std::list<Vertex> dummy_list;

  // A "property map" that states which vertices have been deleted
  // from the bucket in this iteration.
  std::vector<bool> vertex_was_deleted;
};

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
delta_stepping_impl(const Graph& g,
                    PredecessorMap predecessor, 
                    DistanceMap distance, 
                    EdgeWeightMap weight,
                    Dist delta)
    : g(g),
      predecessor(predecessor),
      distance(distance),
      weight(weight),
      delta(delta),
      pg(boost::graph::parallel::process_group_adl(g), attach_distributed_object()),
      owner(get(vertex_owner, g)),
      local(get(vertex_local, g))
{
  setup_triggers();
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
delta_stepping_impl(const Graph& g,
                    PredecessorMap predecessor, 
                    DistanceMap distance, 
                    EdgeWeightMap weight)
    : g(g),
      predecessor(predecessor),
      distance(distance),
      weight(weight),
      pg(boost::graph::parallel::process_group_adl(g), attach_distributed_object()),
      owner(get(vertex_owner, g)),
      local(get(vertex_local, g))
{
  using boost::parallel::all_reduce;
  using boost::parallel::maximum;
  using std::max;

  // Compute the maximum edge weight and degree
  Dist max_edge_weight = 0;
  Degree max_degree = 0;
  BGL_FORALL_VERTICES_T(u, g, Graph) {
    max_degree = max BOOST_PREVENT_MACRO_SUBSTITUTION (max_degree, out_degree(u, g));
    BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
      max_edge_weight = max BOOST_PREVENT_MACRO_SUBSTITUTION (max_edge_weight, get(weight, e));
  }

  max_edge_weight = all_reduce(pg, max_edge_weight, maximum<Dist>());
  max_degree = all_reduce(pg, max_degree, maximum<Degree>());

  // Take a guess at delta, based on what works well for random
  // graphs.
  delta = max_edge_weight / max_degree;
  if (delta == 0)
    delta = 1;

  setup_triggers();
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
run(Vertex s)
{
  Dist inf = (std::numeric_limits<Dist>::max)();

  // None of the vertices are stored in the bucket.
  position_in_bucket.clear();
  position_in_bucket.resize(num_vertices(g), dummy_list.end());

  // None of the vertices have been deleted
  vertex_was_deleted.clear();
  vertex_was_deleted.resize(num_vertices(g), false);

  // No path from s to any other vertex, yet
  BGL_FORALL_VERTICES_T(v, g, Graph)
    put(distance, v, inf);

  // The distance to the starting node is zero
  if (get(owner, s) == process_id(pg))
    // Put "s" into its bucket (bucket 0)
    relax(s, s, 0);
  else
    // Note that we know the distance to s is zero
    cache(distance, s, 0);

  BucketIndex max_bucket = (std::numeric_limits<BucketIndex>::max)();
  BucketIndex current_bucket = 0;
  do {
    // Synchronize with all of the other processes.
    synchronize();

    // Find the next bucket that has something in it.
    while (current_bucket < buckets.size() 
           && (!buckets[current_bucket] || buckets[current_bucket]->empty()))
      ++current_bucket;
    if (current_bucket >= buckets.size())
      current_bucket = max_bucket;

#ifdef PBGL_DELTA_STEPPING_DEBUG
    std::cerr << "#" << process_id(pg) << ": lowest bucket is #" 
              << current_bucket << std::endl;
#endif
    // Find the smallest bucket (over all processes) that has vertices
    // that need to be processed.
    using boost::parallel::all_reduce;
    using boost::parallel::minimum;
    current_bucket = all_reduce(pg, current_bucket, minimum<BucketIndex>());

    if (current_bucket == max_bucket)
      // There are no non-empty buckets in any process; exit. 
      break;

#ifdef PBGL_DELTA_STEPPING_DEBUG
    if (process_id(pg) == 0)
      std::cerr << "Processing bucket #" << current_bucket << std::endl;
#endif

    // Contains the set of vertices that have been deleted in the
    // relaxation of "light" edges. Note that we keep track of which
    // vertices were deleted with the property map
    // "vertex_was_deleted".
    std::vector<Vertex> deleted_vertices;

    // Repeatedly relax light edges
    bool nonempty_bucket;
    do {
      // Someone has work to do in this bucket.

      if (current_bucket < buckets.size() && buckets[current_bucket]) {
        Bucket& bucket = *buckets[current_bucket];
        // For each element in the bucket
        while (!bucket.empty()) {
          Vertex u = bucket.front();

#ifdef PBGL_DELTA_STEPPING_DEBUG
          std::cerr << "#" << process_id(pg) << ": processing vertex " 
                    << get(vertex_global, g, u).second << "@" 
                    << get(vertex_global, g, u).first
                    << std::endl;
#endif

          // Remove u from the front of the bucket
          bucket.pop_front();
          
          // Insert u into the set of deleted vertices, if it hasn't
          // been done already.
          if (!vertex_was_deleted[get(local, u)]) {
            vertex_was_deleted[get(local, u)] = true;
            deleted_vertices.push_back(u);
          }

          // Relax each light edge. 
          Dist u_dist = get(distance, u);
          BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
            if (get(weight, e) <= delta) // light edge
              relax(u, target(e, g), u_dist + get(weight, e));
        }
      }

      // Synchronize with all of the other processes.
      synchronize();

      // Is the bucket empty now?
      nonempty_bucket = (current_bucket < buckets.size() 
                         && buckets[current_bucket]
                         && !buckets[current_bucket]->empty());
     } while (all_reduce(pg, nonempty_bucket, std::logical_or<bool>()));

    // Relax heavy edges for each of the vertices that we previously
    // deleted.
    for (typename std::vector<Vertex>::iterator iter = deleted_vertices.begin();
         iter != deleted_vertices.end(); ++iter) {
      // Relax each heavy edge. 
      Vertex u = *iter;
      Dist u_dist = get(distance, u);
      BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
        if (get(weight, e) > delta) // heavy edge
          relax(u, target(e, g), u_dist + get(weight, e)); 
    }

    // Go to the next bucket: the current bucket must already be empty.
    ++current_bucket;
  } while (true);

  // Delete all of the buckets.
  for (typename std::vector<Bucket*>::iterator iter = buckets.begin();
       iter != buckets.end(); ++iter) {
    if (*iter) {
      delete *iter;
      *iter = 0;
    }
  }
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
relax(Vertex u, Vertex v, Dist x) 
{
#ifdef PBGL_DELTA_STEPPING_DEBUG
  std::cerr << "#" << process_id(pg) << ": relax(" 
            << get(vertex_global, g, u).second << "@" 
            << get(vertex_global, g, u).first << ", " 
            << get(vertex_global, g, v).second << "@" 
            << get(vertex_global, g, v).first << ", "
            << x << ")" << std::endl;
#endif

  if (x < get(distance, v)) {
    // We're relaxing the edge to vertex v.
    if (get(owner, v) == process_id(pg)) {
      // Compute the new bucket index for v
      BucketIndex new_index = static_cast<BucketIndex>(x / delta);
      
      // Make sure there is enough room in the buckets data structure.
      if (new_index >= buckets.size()) buckets.resize(new_index + 1, 0);

      // Make sure that we have allocated the bucket itself.
      if (!buckets[new_index]) buckets[new_index] = new Bucket;

      if (get(distance, v) != (std::numeric_limits<Dist>::max)()
          && !vertex_was_deleted[get(local, v)]) {
        // We're moving v from an old bucket into a new one. Compute
        // the old index, then splice it in.
        BucketIndex old_index 
          = static_cast<BucketIndex>(get(distance, v) / delta);
        buckets[new_index]->splice(buckets[new_index]->end(),
                                   *buckets[old_index],
                                   position_in_bucket[get(local, v)]);
      } else {
        // We're inserting v into a bucket for the first time. Put it
        // at the end.
        buckets[new_index]->push_back(v);
      }

      // v is now at the last position in the new bucket
      position_in_bucket[get(local, v)] = buckets[new_index]->end();
      --position_in_bucket[get(local, v)];

      // Update predecessor and tentative distance information
      put(predecessor, v, u);
      put(distance, v, x);
    } else {
#ifdef PBGL_DELTA_STEPPING_DEBUG
      std::cerr << "#" << process_id(pg) << ": sending relax(" 
                << get(vertex_global, g, u).second << "@" 
                << get(vertex_global, g, u).first << ", " 
                << get(vertex_global, g, v).second << "@" 
                << get(vertex_global, g, v).first << ", "
            << x << ") to " << get(owner, v) << std::endl;
      
#endif
      // The vertex is remote: send a request to the vertex's owner
      send(pg, get(owner, v), msg_relax, 
           std::make_pair(v, MessageValue::create(x, u)));

      // Cache tentative distance information
      cache(distance, v, x);
    }
  }
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
synchronize()
{
  using boost::parallel::synchronize;

  // Synchronize at the process group level.
  synchronize(pg);

  // Receive any relaxation request messages.
//   typedef typename ProcessGroup::process_id_type process_id_type;
//   while (optional<std::pair<process_id_type, int> > stp = probe(pg)) {
//     // Receive the relaxation message
//     assert(stp->second == msg_relax);
//     std::pair<Vertex, typename MessageValue::type> data;
//     receive(pg, stp->first, stp->second, data);

//     // Turn it into a "relax" call
//     handle_relax_message(data.first, data.second);
//   }
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void 
delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>::
setup_triggers() 
{
  using boost::graph::parallel::simple_trigger;
        
  simple_trigger(pg, msg_relax, this, 
                 &delta_stepping_impl::handle_msg_relax);
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void 
delta_stepping_shortest_paths
  (const Graph& g,
   typename graph_traits<Graph>::vertex_descriptor s,
   PredecessorMap predecessor, DistanceMap distance, EdgeWeightMap weight,
   typename property_traits<EdgeWeightMap>::value_type delta)
{
  // The "distance" map needs to act like one, retrieving the default
  // value of infinity.
  set_property_map_role(vertex_distance, distance);

  // Construct the implementation object, which will perform all of
  // the actual work.
  delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>
    impl(g, predecessor, distance, weight, delta);

  // Run the delta-stepping algorithm. The results will show up in
  // "predecessor" and "weight".
  impl.run(s);
}

template<typename Graph, typename PredecessorMap, typename DistanceMap, 
         typename EdgeWeightMap>
void 
delta_stepping_shortest_paths
  (const Graph& g,
   typename graph_traits<Graph>::vertex_descriptor s,
   PredecessorMap predecessor, DistanceMap distance, EdgeWeightMap weight)
{
  // The "distance" map needs to act like one, retrieving the default
  // value of infinity.
  set_property_map_role(vertex_distance, distance);

  // Construct the implementation object, which will perform all of
  // the actual work.
  delta_stepping_impl<Graph, PredecessorMap, DistanceMap, EdgeWeightMap>
    impl(g, predecessor, distance, weight);

  // Run the delta-stepping algorithm. The results will show up in
  // "predecessor" and "weight".
  impl.run(s);
}
   
} } } // end namespace boost::graph::distributed

#endif // BOOST_GRAPH_DELTA_STEPPING_SHORTEST_PATHS_HPP

// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

/**************************************************************************
 * This source file implements the variation on Dijkstra's algorithm      *
 * presented by Crauser et al. in:                                        *
 *                                                                        *
 *   Andreas Crauser, Kurt Mehlhorn, Ulrich Meyer, and Peter              *
 *   Sanders. A Parallelization of Dijkstra's Shortest Path               *
 *   Algorithm. In Lubos Brim, Jozef Gruska, and Jiri Zlatuska,          *
 *   editors, Mathematical Foundations of Computer Science (MFCS),        *
 *   volume 1450 of Lecture Notes in Computer Science, pages              *
 *   722--731, 1998. Springer.                                            *
 *                                                                        *
 * This implementation is, however, restricted to the distributed-memory  *
 * case, where the work is distributed by virtue of the vertices being    *
 * distributed. In a shared-memory (single address space) context, we     *
 * would want to add an explicit balancing step.                          *
 **************************************************************************/
#ifndef BOOST_GRAPH_CRAUSER_ET_AL_SHORTEST_PATHS_HPP
#define BOOST_GRAPH_CRAUSER_ET_AL_SHORTEST_PATHS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/graph/distributed/detail/dijkstra_shortest_paths.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <functional>
#include <boost/graph/iteration_macros.hpp>
#include <boost/property_map/property_map_iterator.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/algorithm/minmax_element.hpp>
#include <boost/property_map/parallel/caching_property_map.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/distributed/detail/remote_update_set.hpp>
#include <vector>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/pending/relaxed_heap.hpp>

#ifdef PBGL_ACCOUNTING
#  include <boost/graph/accounting.hpp>
#  include <numeric>
#endif // PBGL_ACCOUNTING

#ifdef MUTABLE_QUEUE
#    include <boost/pending/mutable_queue.hpp>
#endif

namespace boost { namespace graph { namespace distributed {

#ifdef PBGL_ACCOUNTING
struct crauser_et_al_shortest_paths_stats_t
{
  /* Total wall-clock time used by the algorithm.*/
  accounting::time_type execution_time;

  /* The number of vertices deleted in each superstep. */
  std::vector<std::size_t> deleted_vertices;

  template<typename OutputStream>
  void print(OutputStream& out)
  {
    double avg_deletions = std::accumulate(deleted_vertices.begin(),
                                           deleted_vertices.end(),
                                           0.0);
    avg_deletions /= deleted_vertices.size();

    out << "Problem = \"Single-Source Shortest Paths\"\n"
        << "Algorithm = \"Crauser et al\"\n"
        << "Function = crauser_et_al_shortest_paths\n"
        << "Wall clock time = " << accounting::print_time(execution_time)
        << "\nSupersteps = " << deleted_vertices.size() << "\n"
        << "Avg. deletions per superstep = " << avg_deletions << "\n";
  }
};

static crauser_et_al_shortest_paths_stats_t crauser_et_al_shortest_paths_stats;
#endif

namespace detail {

  /************************************************************************
   * Function objects that perform distance comparisons modified by the   *
   * minimum or maximum edge weights.                                     *
   ************************************************************************/
  template<typename Vertex, typename DistanceMap, typename MinInWeightMap,
           typename Combine, typename Compare>
  struct min_in_distance_compare
  {
    typedef Vertex first_argument_type;
    typedef Vertex second_argument_type;
    typedef bool result_type;
    min_in_distance_compare(DistanceMap d, MinInWeightMap m,
                            Combine combine, Compare compare)
      : distance_map(d), min_in_weight(m), combine(combine),
        compare(compare)
    {
    }

    bool operator()(const Vertex& x, const Vertex& y) const
    {
      return compare(combine(get(distance_map, x), -get(min_in_weight, x)),
                     combine(get(distance_map, y), -get(min_in_weight, y)));
    }

  private:
    DistanceMap distance_map;
    MinInWeightMap min_in_weight;
    Combine combine;
    Compare compare;
  };

  template<typename Vertex, typename DistanceMap, typename MinOutWeightMap,
           typename Combine, typename Compare>
  struct min_out_distance_compare
  {
    typedef Vertex first_argument_type;
    typedef Vertex second_argument_type;
    typedef bool result_type;
    min_out_distance_compare(DistanceMap d, MinOutWeightMap m,
                             Combine combine, Compare compare)
      : distance_map(d), min_out_weight(m), combine(combine),
        compare(compare)
    {
    }

    bool operator()(const Vertex& x, const Vertex& y) const
    {
      return compare(combine(get(distance_map, x), get(min_out_weight, x)),
                     combine(get(distance_map, y), get(min_out_weight, y)));
    }

  private:
    DistanceMap distance_map;
    MinOutWeightMap min_out_weight;
    Combine combine;
    Compare compare;
  };
  /************************************************************************/

  /************************************************************************
   * Dijkstra queue that implements Crauser et al.'s criteria. This queue *
   * actually stores three separate priority queues, to help expose all   *
   * vertices that can be processed in a single phase.                    *
   ************************************************************************/
  template<typename Graph, typename Combine,
           typename Compare, typename VertexIndexMap, typename DistanceMap,
           typename PredecessorMap, typename MinOutWeightMap,
           typename MinInWeightMap>
  class crauser_et_al_dijkstra_queue
    : public graph::detail::remote_update_set<
               crauser_et_al_dijkstra_queue<
                 Graph, Combine, Compare, VertexIndexMap, DistanceMap, 
                 PredecessorMap, MinOutWeightMap, MinInWeightMap>,
               typename boost::graph::parallel::process_group_type<Graph>::type,
               typename dijkstra_msg_value<DistanceMap, PredecessorMap>::type,
               typename property_map<Graph, vertex_owner_t>::const_type>
  {
    typedef typename graph_traits<Graph>::vertex_descriptor
      vertex_descriptor;
    typedef crauser_et_al_dijkstra_queue self_type;
    typedef dijkstra_msg_value<DistanceMap, PredecessorMap> msg_value_creator;
    typedef typename msg_value_creator::type msg_value_type;
    typedef typename graph_traits<Graph>::vertices_size_type
      vertices_size_type;
    typedef typename property_map<Graph, vertex_owner_t>::const_type
      OwnerPropertyMap;
    typedef typename boost::graph::parallel::process_group_type<Graph>::type
      process_group_type;
    typedef graph::detail::remote_update_set<self_type, process_group_type,
                                             msg_value_type, OwnerPropertyMap>
      inherited;

    // Priority queue for tentative distances
    typedef indirect_cmp<DistanceMap, Compare> dist_queue_compare_type;

    typedef typename property_traits<DistanceMap>::value_type distance_type;

#ifdef MUTABLE_QUEUE
    typedef mutable_queue<vertex_descriptor, std::vector<vertex_descriptor>, 
                          dist_queue_compare_type, VertexIndexMap> dist_queue_type;

#else
    typedef relaxed_heap<vertex_descriptor, dist_queue_compare_type,
                         VertexIndexMap> dist_queue_type;
#endif // MUTABLE_QUEUE

    // Priority queue for OUT criteria
    typedef min_out_distance_compare<vertex_descriptor, DistanceMap,
                                     MinOutWeightMap, Combine, Compare>
      out_queue_compare_type;

#ifdef MUTABLE_QUEUE
    typedef mutable_queue<vertex_descriptor, std::vector<vertex_descriptor>, 
                          out_queue_compare_type, VertexIndexMap> out_queue_type;

#else
    typedef relaxed_heap<vertex_descriptor, out_queue_compare_type,
                         VertexIndexMap> out_queue_type;
#endif // MUTABLE_QUEUE

    // Priority queue for IN criteria
    typedef min_in_distance_compare<vertex_descriptor, DistanceMap,
                                    MinInWeightMap, Combine, Compare>
      in_queue_compare_type;

#ifdef MUTABLE_QUEUE
    typedef mutable_queue<vertex_descriptor, std::vector<vertex_descriptor>, 
                          in_queue_compare_type, VertexIndexMap> in_queue_type;

#else
    typedef relaxed_heap<vertex_descriptor, in_queue_compare_type,
                         VertexIndexMap> in_queue_type;
#endif // MUTABLE_QUEUE

    typedef typename process_group_type::process_id_type process_id_type;

  public:
    typedef typename dist_queue_type::size_type  size_type;
    typedef typename dist_queue_type::value_type value_type;

    crauser_et_al_dijkstra_queue(const Graph& g,
                                 const Combine& combine,
                                 const Compare& compare,
                                 const VertexIndexMap& id,
                                 const DistanceMap& distance_map,
                                 const PredecessorMap& predecessor_map,
                                 const MinOutWeightMap& min_out_weight,
                                 const MinInWeightMap& min_in_weight)
      : inherited(boost::graph::parallel::process_group(g), get(vertex_owner, g)),
        dist_queue(num_vertices(g),
                   dist_queue_compare_type(distance_map, compare),
                   id),
        out_queue(num_vertices(g),
                  out_queue_compare_type(distance_map, min_out_weight,
                                         combine, compare),
                  id),
        in_queue(num_vertices(g),
                 in_queue_compare_type(distance_map, min_in_weight,
                                       combine, compare),
                 id),
        g(g),
        distance_map(distance_map),
        predecessor_map(predecessor_map),
        min_out_weight(min_out_weight),
        min_in_weight(min_in_weight),
        min_distance(0),
        min_out_distance(0)
#ifdef PBGL_ACCOUNTING
        , local_deletions(0)
#endif
    { }

    void push(const value_type& x)
    {
      msg_value_type msg_value =
        msg_value_creator::create(get(distance_map, x),
                                  predecessor_value(get(predecessor_map, x)));
      inherited::update(x, msg_value);
    }

    void update(const value_type& x) { push(x); }

    void pop()
    {
      // Remove from distance queue
      dist_queue.remove(top_vertex);

      // Remove from OUT queue
      out_queue.remove(top_vertex);

      // Remove from IN queue
      in_queue.remove(top_vertex);

#ifdef PBGL_ACCOUNTING
      ++local_deletions;
#endif
    }

    vertex_descriptor& top() { return top_vertex; }
    const vertex_descriptor& top() const { return top_vertex; }

    bool empty()
    {
      inherited::collect();

      // If there are no suitable messages, wait until we get something
      while (!has_suitable_vertex()) {
        if (do_synchronize()) return true;
      }
      // Return true only if nobody has any messages; false if we
      // have suitable messages
      return false;
    }

    bool do_synchronize()
    {
      using boost::parallel::all_reduce;
      using boost::parallel::minimum;

      inherited::synchronize();

      // TBD: could use combine here, but then we need to stop using
      // minimum<distance_type>() as the function object.
      distance_type local_distances[2];
      local_distances[0] =
        dist_queue.empty()? (std::numeric_limits<distance_type>::max)()
        : get(distance_map, dist_queue.top());

      local_distances[1] =
        out_queue.empty()? (std::numeric_limits<distance_type>::max)()
        : (get(distance_map, out_queue.top())
           + get(min_out_weight, out_queue.top()));

      distance_type distances[2];
      all_reduce(this->process_group, local_distances, local_distances + 2,
                 distances, minimum<distance_type>());
      min_distance = distances[0];
      min_out_distance = distances[1];

#ifdef PBGL_ACCOUNTING
      std::size_t deletions = 0;
      all_reduce(this->process_group, &local_deletions, &local_deletions + 1,
                 &deletions, std::plus<std::size_t>());
      if (process_id(this->process_group) == 0) {
        crauser_et_al_shortest_paths_stats.deleted_vertices.push_back(deletions);
      }
      local_deletions = 0;
      BOOST_ASSERT(deletions > 0);
#endif

      return min_distance == (std::numeric_limits<distance_type>::max)();
    }

  private:
    vertex_descriptor predecessor_value(vertex_descriptor v) const
    { return v; }

    vertex_descriptor
    predecessor_value(property_traits<dummy_property_map>::reference) const
    { return graph_traits<Graph>::null_vertex(); }

    bool has_suitable_vertex() const
    {
      if (!dist_queue.empty()) {
        top_vertex = dist_queue.top();
        if (get(distance_map, dist_queue.top()) <= min_out_distance)
          return true;
      }

      if (!in_queue.empty()) {
        top_vertex = in_queue.top();
        return (get(distance_map, top_vertex)
                - get(min_in_weight, top_vertex)) <= min_distance;
      }
      return false;
    }

  public:
    void
    receive_update(process_id_type source, vertex_descriptor vertex,
                   distance_type distance)
    {
      // Update the queue if the received distance is better than
      // the distance we know locally
      if (distance < get(distance_map, vertex)
          || (distance == get(distance_map, vertex)
              && source == process_id(this->process_group))) {
        // Update the local distance map
        put(distance_map, vertex, distance);

        bool is_in_queue = dist_queue.contains(vertex);

        if (!is_in_queue) {
          dist_queue.push(vertex);
          out_queue.push(vertex);
          in_queue.push(vertex);
        }
        else {
          dist_queue.update(vertex);
          out_queue.update(vertex);
          in_queue.update(vertex);
        }
      }
    }

    void
    receive_update(process_id_type source, vertex_descriptor vertex,
                   std::pair<distance_type, vertex_descriptor> p)
    {
      if (p.first <= get(distance_map, vertex)) {
        put(predecessor_map, vertex, p.second);
        receive_update(source, vertex, p.first);
      }
    }

  private:
    dist_queue_type           dist_queue;
    out_queue_type            out_queue;
    in_queue_type             in_queue;
    mutable value_type        top_vertex;
    const Graph&              g;
    DistanceMap               distance_map;
    PredecessorMap            predecessor_map;
    MinOutWeightMap           min_out_weight;
    MinInWeightMap            min_in_weight;
    distance_type             min_distance;
    distance_type             min_out_distance;
#ifdef PBGL_ACCOUNTING
    std::size_t               local_deletions;
#endif
  };
  /************************************************************************/

  /************************************************************************
   * Initialize the property map that contains the minimum incoming edge  *
   * weight for each vertex. There are separate implementations for       *
   * directed, bidirectional, and undirected graph.                       *
   ************************************************************************/
  template<typename Graph, typename MinInWeightMap, typename WeightMap,
           typename Inf, typename Compare>
  void
  initialize_min_in_weights(const Graph& g, MinInWeightMap min_in_weight,
                            WeightMap weight, Inf inf, Compare compare,
                            directed_tag, incidence_graph_tag)
  {
    // Send minimum weights off to the owners
    set_property_map_role(vertex_distance, min_in_weight);
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      BGL_FORALL_OUTEDGES_T(v, e, g, Graph) {
        if (get(weight, e) < get(min_in_weight, target(e, g)))
          put(min_in_weight, target(e, g), get(weight, e));
      }
    }

    using boost::graph::parallel::process_group;
    synchronize(process_group(g));

    // Replace any infinities with zeros
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (get(min_in_weight, v) == inf) put(min_in_weight, v, 0);
    }
  }

  template<typename Graph, typename MinInWeightMap, typename WeightMap,
           typename Inf, typename Compare>
  void
  initialize_min_in_weights(const Graph& g, MinInWeightMap min_in_weight,
                            WeightMap weight, Inf inf, Compare compare,
                            directed_tag, bidirectional_graph_tag)
  {
#if 0
    typename property_map<Graph, vertex_local_t>::const_type
      local = get(vertex_local, g);

    // This code assumes that the properties of the in-edges are
    // available locally. This is not necessarily the case, so don't
    // do this yet.
    set_property_map_role(vertex_distance, min_in_weight);
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (in_edges(v, g).first != in_edges(v, g).second) {
        std::cerr << "weights(" << g.distribution().global(get(local, v))
                  << ") = ";
        BGL_FORALL_INEDGES_T(v, e, g, Graph) {
          std::cerr << get(weight, e) << ' ';
        }
        std::cerr << std::endl;
        put(min_in_weight, v,
            *boost::first_min_element
            (make_property_map_iterator(weight, in_edges(v, g).first),
             make_property_map_iterator(weight, in_edges(v, g).second),
             compare));
      } else {
        put(min_in_weight, v, 0);
      }
      std::cerr << "miw(" << g.distribution().global(get(local, v)) << ") = "
                << get(min_in_weight, v) << std::endl;
    }
#else
    initialize_min_in_weights(g, min_in_weight, weight, inf, compare,
                              directed_tag(), incidence_graph_tag());
#endif
  }

  template<typename Graph, typename MinInWeightMap, typename WeightMap,
           typename Inf, typename Compare>
  inline void
  initialize_min_in_weights(const Graph&, MinInWeightMap, WeightMap, Inf,
                            Compare, undirected_tag, bidirectional_graph_tag)
  {
    // In weights are the same as out weights, so do nothing
  }
  /************************************************************************/


  /************************************************************************
   * Initialize the property map that contains the minimum outgoing edge  *
   * weight for each vertex.                                              *
   ************************************************************************/
  template<typename Graph, typename MinOutWeightMap, typename WeightMap,
           typename Compare>
  void
  initialize_min_out_weights(const Graph& g, MinOutWeightMap min_out_weight,
                             WeightMap weight, Compare compare)
  {
    typedef typename property_traits<WeightMap>::value_type weight_type;

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (out_edges(v, g).first != out_edges(v, g).second) {
        put(min_out_weight, v,
            *boost::first_min_element
            (make_property_map_iterator(weight, out_edges(v, g).first),
             make_property_map_iterator(weight, out_edges(v, g).second),
             compare));
        if (get(min_out_weight, v) < weight_type(0))
            boost::throw_exception(negative_edge());
      }
    }
  }

  /************************************************************************/

} // end namespace detail

template<typename DistributedGraph, typename DijkstraVisitor,
         typename PredecessorMap, typename DistanceMap, typename WeightMap,
         typename IndexMap, typename ColorMap, typename Compare,
         typename Combine, typename DistInf, typename DistZero>
void
crauser_et_al_shortest_paths
  (const DistributedGraph& g,
   typename graph_traits<DistributedGraph>::vertex_descriptor s,
   PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
   IndexMap index_map, ColorMap color_map,
   Compare compare, Combine combine, DistInf inf, DistZero zero,
   DijkstraVisitor vis)
{

#ifdef PBGL_ACCOUNTING
  crauser_et_al_shortest_paths_stats.deleted_vertices.clear();
  crauser_et_al_shortest_paths_stats.execution_time = accounting::get_time();
#endif

  // Property map that stores the lowest edge weight outgoing from
  // each vertex. If a vertex has no out-edges, the stored weight
  // is zero.
  typedef typename property_traits<WeightMap>::value_type weight_type;
  typedef iterator_property_map<weight_type*, IndexMap> MinOutWeightMap;
  std::vector<weight_type> min_out_weights_vec(num_vertices(g), inf);
  MinOutWeightMap min_out_weight(&min_out_weights_vec.front(), index_map);
  detail::initialize_min_out_weights(g, min_out_weight, weight, compare);

  // Property map that stores the lowest edge weight incoming to
  // each vertex. For undirected graphs, this will just be a
  // shallow copy of the version for outgoing edges.
  typedef typename graph_traits<DistributedGraph>::directed_category
    directed_category;
  const bool is_undirected =
    is_same<directed_category, undirected_tag>::value;
  typedef MinOutWeightMap MinInWeightMap;
  std::vector<weight_type>
    min_in_weights_vec(is_undirected? 1 : num_vertices(g), inf);
  MinInWeightMap min_in_weight(&min_in_weights_vec.front(), index_map);
  typedef typename graph_traits<DistributedGraph>::traversal_category
    category;
  detail::initialize_min_in_weights(g, min_in_weight, weight, inf, compare,
                                    directed_category(), category());

  // Initialize local portion of property maps
  typename graph_traits<DistributedGraph>::vertex_iterator ui, ui_end;
  for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui) {
    put(distance, *ui, inf);
    put(predecessor, *ui, *ui);
  }
  put(distance, s, zero);

  // Dijkstra Queue
  typedef detail::crauser_et_al_dijkstra_queue
            <DistributedGraph, Combine, Compare, IndexMap, DistanceMap, 
             PredecessorMap, MinOutWeightMap, MinInWeightMap>
    Queue;

  Queue Q(g, combine, compare, index_map, distance, predecessor,
          min_out_weight, is_undirected? min_out_weight : min_in_weight);

  // Parallel Dijkstra visitor
  ::boost::detail::dijkstra_bfs_visitor<
      DijkstraVisitor, Queue, WeightMap,
      boost::parallel::caching_property_map<PredecessorMap>,
      boost::parallel::caching_property_map<DistanceMap>, Combine, Compare
    > bfs_vis(vis, Q, weight,
              boost::parallel::make_caching_property_map(predecessor),
              boost::parallel::make_caching_property_map(distance),
              combine, compare, zero);

  set_property_map_role(vertex_color, color_map);
  set_property_map_role(vertex_distance, distance);

  breadth_first_search(g, s, Q, bfs_vis, color_map);

#ifdef PBGL_ACCOUNTING
  crauser_et_al_shortest_paths_stats.execution_time =
    accounting::get_time() - crauser_et_al_shortest_paths_stats.execution_time;
#endif
}

template<typename DistributedGraph, typename PredecessorMap,
         typename DistanceMap, typename WeightMap>
void
crauser_et_al_shortest_paths
  (const DistributedGraph& g,
   typename graph_traits<DistributedGraph>::vertex_descriptor s,
   PredecessorMap predecessor, DistanceMap distance, WeightMap weight)
{
  typedef typename property_traits<DistanceMap>::value_type distance_type;

  std::vector<default_color_type> colors(num_vertices(g), white_color);

  crauser_et_al_shortest_paths(g, s, predecessor, distance, weight,
                               get(vertex_index, g),
                               make_iterator_property_map(&colors[0],
                                                          get(vertex_index, g)),
                               std::less<distance_type>(),
                               closed_plus<distance_type>(),
                               (std::numeric_limits<distance_type>::max)(),
                               distance_type(),
                               dijkstra_visitor<>());
}

template<typename DistributedGraph, typename PredecessorMap,
         typename DistanceMap>
void
crauser_et_al_shortest_paths
  (const DistributedGraph& g,
   typename graph_traits<DistributedGraph>::vertex_descriptor s,
   PredecessorMap predecessor, DistanceMap distance)
{
  crauser_et_al_shortest_paths(g, s, predecessor, distance,
                               get(edge_weight, g));
}

} // end namespace distributed

#ifdef PBGL_ACCOUNTING
using distributed::crauser_et_al_shortest_paths_stats;
#endif

using distributed::crauser_et_al_shortest_paths;


} } // end namespace boost::graph

#endif // BOOST_GRAPH_CRAUSER_ET_AL_SHORTEST_PATHS_HPP

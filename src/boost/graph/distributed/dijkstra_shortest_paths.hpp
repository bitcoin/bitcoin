// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_PARALLEL_DIJKSTRA_HPP
#define BOOST_GRAPH_PARALLEL_DIJKSTRA_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/distributed/crauser_et_al_shortest_paths.hpp>
#include <boost/graph/distributed/eager_dijkstra_shortest_paths.hpp>

namespace boost {

  namespace graph { namespace detail {

    
    template<typename Lookahead>
    struct parallel_dijkstra_impl2
    {
      template<typename DistributedGraph, typename DijkstraVisitor,
               typename PredecessorMap, typename DistanceMap, 
               typename WeightMap, typename IndexMap, typename ColorMap, 
               typename Compare, typename Combine, typename DistInf, 
               typename DistZero>
      static void 
      run(const DistributedGraph& g,
          typename graph_traits<DistributedGraph>::vertex_descriptor s,
          PredecessorMap predecessor, DistanceMap distance, 
          typename property_traits<DistanceMap>::value_type lookahead,
          WeightMap weight, IndexMap index_map, ColorMap color_map,
          Compare compare, Combine combine, DistInf inf, DistZero zero,
          DijkstraVisitor vis)
      {
        eager_dijkstra_shortest_paths(g, s, predecessor, distance, lookahead,
                                      weight, index_map, color_map, compare,
                                      combine, inf, zero, vis);
      }
    };

    template<>
    struct parallel_dijkstra_impl2< ::boost::param_not_found >
    {
      template<typename DistributedGraph, typename DijkstraVisitor,
               typename PredecessorMap, typename DistanceMap, 
               typename WeightMap, typename IndexMap, typename ColorMap, 
               typename Compare, typename Combine, typename DistInf, 
               typename DistZero>
      static void 
      run(const DistributedGraph& g,
          typename graph_traits<DistributedGraph>::vertex_descriptor s,
          PredecessorMap predecessor, DistanceMap distance, 
          ::boost::param_not_found,
          WeightMap weight, IndexMap index_map, ColorMap color_map,
          Compare compare, Combine combine, DistInf inf, DistZero zero,
          DijkstraVisitor vis)
      {
        crauser_et_al_shortest_paths(g, s, predecessor, distance, weight,
                                     index_map, color_map, compare, combine,
                                     inf, zero, vis);
      }
    };

    template<typename ColorMap>
    struct parallel_dijkstra_impl
    {
      template<typename DistributedGraph, typename DijkstraVisitor,
               typename PredecessorMap, typename DistanceMap, 
               typename Lookahead, typename WeightMap, typename IndexMap,
               typename Compare, typename Combine, 
               typename DistInf, typename DistZero>
      static void 
      run(const DistributedGraph& g,
          typename graph_traits<DistributedGraph>::vertex_descriptor s,
          PredecessorMap predecessor, DistanceMap distance, 
          Lookahead lookahead,
          WeightMap weight, IndexMap index_map, ColorMap color_map,
          Compare compare, Combine combine, DistInf inf, DistZero zero,
          DijkstraVisitor vis)
      {
        graph::detail::parallel_dijkstra_impl2<Lookahead>
          ::run(g, s, predecessor, distance, lookahead, weight, index_map,
                color_map, compare, combine, inf, zero, vis);
      }
    };
    
    template<>
    struct parallel_dijkstra_impl< ::boost::param_not_found >
    {
    private:
      template<typename DistributedGraph, typename DijkstraVisitor,
               typename PredecessorMap, typename DistanceMap, 
               typename Lookahead, typename WeightMap, typename IndexMap,
               typename ColorMap, typename Compare, typename Combine, 
               typename DistInf, typename DistZero>
      static void 
      run_impl(const DistributedGraph& g,
               typename graph_traits<DistributedGraph>::vertex_descriptor s,
               PredecessorMap predecessor, DistanceMap distance, 
               Lookahead lookahead, WeightMap weight, IndexMap index_map, 
               ColorMap color_map, Compare compare, Combine combine, 
               DistInf inf, DistZero zero, DijkstraVisitor vis)
      {
        BGL_FORALL_VERTICES_T(u, g, DistributedGraph)
          BGL_FORALL_OUTEDGES_T(u, e, g, DistributedGraph)
            local_put(color_map, target(e, g), white_color);

        graph::detail::parallel_dijkstra_impl2<Lookahead>
          ::run(g, s, predecessor, distance, lookahead, weight, index_map,
                color_map, compare, combine, inf, zero, vis);
      }

    public:
      template<typename DistributedGraph, typename DijkstraVisitor,
               typename PredecessorMap, typename DistanceMap, 
               typename Lookahead, typename WeightMap, typename IndexMap,
               typename Compare, typename Combine, 
               typename DistInf, typename DistZero>
      static void 
      run(const DistributedGraph& g,
          typename graph_traits<DistributedGraph>::vertex_descriptor s,
          PredecessorMap predecessor, DistanceMap distance, 
          Lookahead lookahead, WeightMap weight, IndexMap index_map, 
          ::boost::param_not_found,
          Compare compare, Combine combine, DistInf inf, DistZero zero,
          DijkstraVisitor vis)
      {
        typedef typename graph_traits<DistributedGraph>::vertices_size_type
          vertices_size_type;

        vertices_size_type n = num_vertices(g);
        std::vector<default_color_type> colors(n, white_color);

        run_impl(g, s, predecessor, distance, lookahead, weight, index_map,
                 make_iterator_property_map(colors.begin(), index_map),
                 compare, combine, inf, zero, vis);
      }
    };
  } } // end namespace graph::detail


  /** Dijkstra's single-source shortest paths algorithm for distributed
   * graphs.
   *
   * Also implements the heuristics of:
   *
   *   Andreas Crauser, Kurt Mehlhorn, Ulrich Meyer, and Peter
   *   Sanders. A Parallelization of Dijkstra's Shortest Path
   *   Algorithm. In Lubos Brim, Jozef Gruska, and Jiri Zlatuska,
   *   editors, Mathematical Foundations of Computer Science (MFCS),
   *   volume 1450 of Lecture Notes in Computer Science, pages
   *   722--731, 1998. Springer.
   */
  template<typename DistributedGraph, typename DijkstraVisitor,
           typename PredecessorMap, typename DistanceMap,
           typename WeightMap, typename IndexMap, typename Compare,
           typename Combine, typename DistInf, typename DistZero,
           typename T, typename Tag, typename Base>
  inline
  void
  dijkstra_shortest_paths
    (const DistributedGraph& g,
     typename graph_traits<DistributedGraph>::vertex_descriptor s,
     PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
     IndexMap index_map,
     Compare compare, Combine combine, DistInf inf, DistZero zero,
     DijkstraVisitor vis,
     const bgl_named_params<T, Tag, Base>& params
     BOOST_GRAPH_ENABLE_IF_MODELS_PARM(DistributedGraph,distributed_graph_tag))
  {
    typedef typename graph_traits<DistributedGraph>::vertices_size_type
      vertices_size_type;

    // Build a distributed property map for vertex colors, if we need it
    bool use_default_color_map 
      = is_default_param(get_param(params, vertex_color));
    vertices_size_type n = use_default_color_map? num_vertices(g) : 1;
    std::vector<default_color_type> color(n, white_color);
    typedef iterator_property_map<std::vector<default_color_type>::iterator,
                                  IndexMap> DefColorMap;
    DefColorMap color_map(color.begin(), index_map);

    typedef typename get_param_type< vertex_color_t, bgl_named_params<T, Tag, Base> >::type color_map_type;

    graph::detail::parallel_dijkstra_impl<color_map_type>
      ::run(g, s, predecessor, distance, 
            get_param(params, lookahead_t()),
            weight, index_map,
            get_param(params, vertex_color),
            compare, combine, inf, zero, vis);
  }
} // end namespace boost

#endif // BOOST_GRAPH_PARALLEL_DIJKSTRA_HPP

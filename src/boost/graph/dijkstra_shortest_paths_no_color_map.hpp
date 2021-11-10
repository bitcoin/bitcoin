//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2009 Trustees of Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Michael Hansen
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_DIJKSTRA_NO_COLOR_MAP_HPP
#define BOOST_GRAPH_DIJKSTRA_NO_COLOR_MAP_HPP

#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/relax.hpp>
#include <boost/graph/detail/d_ary_heap.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/iteration_macros.hpp>

namespace boost
{

// No init version
template < typename Graph, typename DijkstraVisitor, typename PredecessorMap,
    typename DistanceMap, typename WeightMap, typename VertexIndexMap,
    typename DistanceCompare, typename DistanceWeightCombine,
    typename DistanceInfinity, typename DistanceZero >
void dijkstra_shortest_paths_no_color_map_no_init(const Graph& graph,
    typename graph_traits< Graph >::vertex_descriptor start_vertex,
    PredecessorMap predecessor_map, DistanceMap distance_map,
    WeightMap weight_map, VertexIndexMap index_map,
    DistanceCompare distance_compare,
    DistanceWeightCombine distance_weight_combine,
    DistanceInfinity distance_infinity, DistanceZero distance_zero,
    DijkstraVisitor visitor)
{
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename property_traits< DistanceMap >::value_type Distance;

    typedef indirect_cmp< DistanceMap, DistanceCompare >
        DistanceIndirectCompare;
    DistanceIndirectCompare distance_indirect_compare(
        distance_map, distance_compare);

    // Default - use d-ary heap (d = 4)
    typedef detail::vertex_property_map_generator< Graph, VertexIndexMap,
        std::size_t >
        IndexInHeapMapHelper;
    typedef typename IndexInHeapMapHelper::type IndexInHeapMap;
    typedef d_ary_heap_indirect< Vertex, 4, IndexInHeapMap, DistanceMap,
        DistanceCompare >
        VertexQueue;

    boost::scoped_array< std::size_t > index_in_heap_map_holder;
    IndexInHeapMap index_in_heap = IndexInHeapMapHelper::build(
        graph, index_map, index_in_heap_map_holder);
    VertexQueue vertex_queue(distance_map, index_in_heap, distance_compare);

    // Add vertex to the queue
    vertex_queue.push(start_vertex);

    // Starting vertex will always be the first discovered vertex
    visitor.discover_vertex(start_vertex, graph);

    while (!vertex_queue.empty())
    {
        Vertex min_vertex = vertex_queue.top();
        vertex_queue.pop();

        visitor.examine_vertex(min_vertex, graph);

        // Check if any other vertices can be reached
        Distance min_vertex_distance = get(distance_map, min_vertex);

        if (!distance_compare(min_vertex_distance, distance_infinity))
        {
            // This is the minimum vertex, so all other vertices are unreachable
            return;
        }

        // Examine neighbors of min_vertex
        BGL_FORALL_OUTEDGES_T(min_vertex, current_edge, graph, Graph)
        {
            visitor.examine_edge(current_edge, graph);

            // Check if the edge has a negative weight
            if (distance_compare(get(weight_map, current_edge), distance_zero))
            {
                boost::throw_exception(negative_edge());
            }

            // Extract the neighboring vertex and get its distance
            Vertex neighbor_vertex = target(current_edge, graph);
            Distance neighbor_vertex_distance
                = get(distance_map, neighbor_vertex);
            bool is_neighbor_undiscovered = !distance_compare(
                neighbor_vertex_distance, distance_infinity);

            // Attempt to relax the edge
            bool was_edge_relaxed
                = relax_target(current_edge, graph, weight_map, predecessor_map,
                    distance_map, distance_weight_combine, distance_compare);

            if (was_edge_relaxed)
            {
                visitor.edge_relaxed(current_edge, graph);
                if (is_neighbor_undiscovered)
                {
                    visitor.discover_vertex(neighbor_vertex, graph);
                    vertex_queue.push(neighbor_vertex);
                }
                else
                {
                    vertex_queue.update(neighbor_vertex);
                }
            }
            else
            {
                visitor.edge_not_relaxed(current_edge, graph);
            }

        } // end out edge iteration

        visitor.finish_vertex(min_vertex, graph);
    } // end while queue not empty
}

// Full init version
template < typename Graph, typename DijkstraVisitor, typename PredecessorMap,
    typename DistanceMap, typename WeightMap, typename VertexIndexMap,
    typename DistanceCompare, typename DistanceWeightCombine,
    typename DistanceInfinity, typename DistanceZero >
void dijkstra_shortest_paths_no_color_map(const Graph& graph,
    typename graph_traits< Graph >::vertex_descriptor start_vertex,
    PredecessorMap predecessor_map, DistanceMap distance_map,
    WeightMap weight_map, VertexIndexMap index_map,
    DistanceCompare distance_compare,
    DistanceWeightCombine distance_weight_combine,
    DistanceInfinity distance_infinity, DistanceZero distance_zero,
    DijkstraVisitor visitor)
{
    // Initialize vertices
    BGL_FORALL_VERTICES_T(current_vertex, graph, Graph)
    {
        visitor.initialize_vertex(current_vertex, graph);

        // Default all distances to infinity
        put(distance_map, current_vertex, distance_infinity);

        // Default all vertex predecessors to the vertex itself
        put(predecessor_map, current_vertex, current_vertex);
    }

    // Set distance for start_vertex to zero
    put(distance_map, start_vertex, distance_zero);

    // Pass everything on to the no_init version
    dijkstra_shortest_paths_no_color_map_no_init(graph, start_vertex,
        predecessor_map, distance_map, weight_map, index_map, distance_compare,
        distance_weight_combine, distance_infinity, distance_zero, visitor);
}

namespace detail
{

    // Handle defaults for PredecessorMap, DistanceCompare,
    // DistanceWeightCombine, DistanceInfinity and DistanceZero
    template < typename Graph, typename DistanceMap, typename WeightMap,
        typename VertexIndexMap, typename Params >
    inline void dijkstra_no_color_map_dispatch2(const Graph& graph,
        typename graph_traits< Graph >::vertex_descriptor start_vertex,
        DistanceMap distance_map, WeightMap weight_map,
        VertexIndexMap index_map, const Params& params)
    {
        // Default for predecessor map
        dummy_property_map predecessor_map;

        typedef
            typename property_traits< DistanceMap >::value_type DistanceType;
        DistanceType inf = choose_param(get_param(params, distance_inf_t()),
            (std::numeric_limits< DistanceType >::max)());
        dijkstra_shortest_paths_no_color_map(graph, start_vertex,
            choose_param(
                get_param(params, vertex_predecessor), predecessor_map),
            distance_map, weight_map, index_map,
            choose_param(get_param(params, distance_compare_t()),
                std::less< DistanceType >()),
            choose_param(get_param(params, distance_combine_t()),
                std::plus< DistanceType >()),
            inf,
            choose_param(get_param(params, distance_zero_t()), DistanceType()),
            choose_param(get_param(params, graph_visitor),
                make_dijkstra_visitor(null_visitor())));
    }

    template < typename Graph, typename DistanceMap, typename WeightMap,
        typename IndexMap, typename Params >
    inline void dijkstra_no_color_map_dispatch1(const Graph& graph,
        typename graph_traits< Graph >::vertex_descriptor start_vertex,
        DistanceMap distance_map, WeightMap weight_map, IndexMap index_map,
        const Params& params)
    {
        // Default for distance map
        typedef typename property_traits< WeightMap >::value_type DistanceType;
        typename std::vector< DistanceType >::size_type vertex_count
            = is_default_param(distance_map) ? num_vertices(graph) : 1;

        std::vector< DistanceType > default_distance_map(vertex_count);

        detail::dijkstra_no_color_map_dispatch2(graph, start_vertex,
            choose_param(distance_map,
                make_iterator_property_map(default_distance_map.begin(),
                    index_map, default_distance_map[0])),
            weight_map, index_map, params);
    }
} // namespace detail

// Named parameter version
template < typename Graph, typename Param, typename Tag, typename Rest >
inline void dijkstra_shortest_paths_no_color_map(const Graph& graph,
    typename graph_traits< Graph >::vertex_descriptor start_vertex,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    // Default for edge weight and vertex index map is to ask for them
    // from the graph. Default for the visitor is null_visitor.
    detail::dijkstra_no_color_map_dispatch1(graph, start_vertex,
        get_param(params, vertex_distance),
        choose_const_pmap(get_param(params, edge_weight), graph, edge_weight),
        choose_const_pmap(get_param(params, vertex_index), graph, vertex_index),
        params);
}

} // namespace boost

#endif // BOOST_GRAPH_DIJKSTRA_NO_COLOR_MAP_HPP

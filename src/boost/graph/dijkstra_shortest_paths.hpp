//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
//
// Revision History:
//   04 April 2001: Added named parameter variant. (Jeremy Siek)
//   01 April 2001: Modified to use new <boost/limits.hpp> header. (JMaddock)
//
#ifndef BOOST_GRAPH_DIJKSTRA_HPP
#define BOOST_GRAPH_DIJKSTRA_HPP

#include <functional>
#include <boost/limits.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/relax.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/graph/detail/d_ary_heap.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/detail/mpi_include.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/type_traits.hpp>
#include <boost/concept/assert.hpp>

#ifdef BOOST_GRAPH_DIJKSTRA_TESTING
#include <boost/pending/mutable_queue.hpp>
#endif // BOOST_GRAPH_DIJKSTRA_TESTING

namespace boost
{

/**
 * @brief Updates a particular value in a queue used by Dijkstra's
 * algorithm.
 *
 * This routine is called by Dijkstra's algorithm after it has
 * decreased the distance from the source vertex to the given @p
 * vertex. By default, this routine will just call @c
 * Q.update(vertex). However, other queues may provide more
 * specialized versions of this routine.
 *
 * @param Q             the queue that will be updated.
 * @param vertex        the vertex whose distance has been updated
 * @param old_distance  the previous distance to @p vertex
 */
template < typename Buffer, typename Vertex, typename DistanceType >
inline void dijkstra_queue_update(
    Buffer& Q, Vertex vertex, DistanceType old_distance)
{
    (void)old_distance;
    Q.update(vertex);
}

template < class Visitor, class Graph > struct DijkstraVisitorConcept
{
    void constraints()
    {
        BOOST_CONCEPT_ASSERT((CopyConstructibleConcept< Visitor >));
        vis.initialize_vertex(u, g);
        vis.discover_vertex(u, g);
        vis.examine_vertex(u, g);
        vis.examine_edge(e, g);
        vis.edge_relaxed(e, g);
        vis.edge_not_relaxed(e, g);
        vis.finish_vertex(u, g);
    }
    Visitor vis;
    Graph g;
    typename graph_traits< Graph >::vertex_descriptor u;
    typename graph_traits< Graph >::edge_descriptor e;
};

template < class Visitors = null_visitor >
class dijkstra_visitor : public bfs_visitor< Visitors >
{
public:
    dijkstra_visitor() {}
    dijkstra_visitor(Visitors vis) : bfs_visitor< Visitors >(vis) {}

    template < class Edge, class Graph > void edge_relaxed(Edge e, Graph& g)
    {
        invoke_visitors(this->m_vis, e, g, on_edge_relaxed());
    }
    template < class Edge, class Graph > void edge_not_relaxed(Edge e, Graph& g)
    {
        invoke_visitors(this->m_vis, e, g, on_edge_not_relaxed());
    }

private:
    template < class Edge, class Graph > void tree_edge(Edge u, Graph& g) {}
};
template < class Visitors >
dijkstra_visitor< Visitors > make_dijkstra_visitor(Visitors vis)
{
    return dijkstra_visitor< Visitors >(vis);
}
typedef dijkstra_visitor<> default_dijkstra_visitor;

namespace detail
{

    template < class UniformCostVisitor, class UpdatableQueue, class WeightMap,
        class PredecessorMap, class DistanceMap, class BinaryFunction,
        class BinaryPredicate >
    struct dijkstra_bfs_visitor
    {
        typedef typename property_traits< DistanceMap >::value_type D;
        typedef typename property_traits< WeightMap >::value_type W;

        dijkstra_bfs_visitor(UniformCostVisitor vis, UpdatableQueue& Q,
            WeightMap w, PredecessorMap p, DistanceMap d,
            BinaryFunction combine, BinaryPredicate compare, D zero)
        : m_vis(vis)
        , m_Q(Q)
        , m_weight(w)
        , m_predecessor(p)
        , m_distance(d)
        , m_combine(combine)
        , m_compare(compare)
        , m_zero(zero)
        {
        }

        template < class Edge, class Graph > void tree_edge(Edge e, Graph& g)
        {
            bool decreased = relax_target(e, g, m_weight, m_predecessor,
                m_distance, m_combine, m_compare);
            if (decreased)
                m_vis.edge_relaxed(e, g);
            else
                m_vis.edge_not_relaxed(e, g);
        }
        template < class Edge, class Graph > void gray_target(Edge e, Graph& g)
        {
            D old_distance = get(m_distance, target(e, g));

            bool decreased = relax_target(e, g, m_weight, m_predecessor,
                m_distance, m_combine, m_compare);
            if (decreased)
            {
                dijkstra_queue_update(m_Q, target(e, g), old_distance);
                m_vis.edge_relaxed(e, g);
            }
            else
                m_vis.edge_not_relaxed(e, g);
        }

        template < class Vertex, class Graph >
        void initialize_vertex(Vertex u, Graph& g)
        {
            m_vis.initialize_vertex(u, g);
        }
        template < class Edge, class Graph > void non_tree_edge(Edge, Graph&) {}
        template < class Vertex, class Graph >
        void discover_vertex(Vertex u, Graph& g)
        {
            m_vis.discover_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void examine_vertex(Vertex u, Graph& g)
        {
            m_vis.examine_vertex(u, g);
        }
        template < class Edge, class Graph > void examine_edge(Edge e, Graph& g)
        {
            // Test for negative-weight edges:
            //
            // Reasons that other comparisons do not work:
            //
            // m_compare(e_weight, D(0)):
            //    m_compare only needs to work on distances, not weights, and
            //    those types do not need to be the same (bug 8398,
            //    https://svn.boost.org/trac/boost/ticket/8398).
            // m_compare(m_combine(source_dist, e_weight), source_dist):
            //    if m_combine is project2nd (as in prim_minimum_spanning_tree),
            //    this test will claim that the edge weight is negative whenever
            //    the edge weight is less than source_dist, even if both of
            //    those are positive (bug 9012,
            //    https://svn.boost.org/trac/boost/ticket/9012).
            // m_compare(m_combine(e_weight, source_dist), source_dist):
            //    would fix project2nd issue, but documentation only requires
            //    that m_combine be able to take a distance and a weight (in
            //    that order) and return a distance.

            // W e_weight = get(m_weight, e);
            // sd_plus_ew = source_dist + e_weight.
            // D sd_plus_ew = m_combine(source_dist, e_weight);
            // sd_plus_2ew = source_dist + 2 * e_weight.
            // D sd_plus_2ew = m_combine(sd_plus_ew, e_weight);
            // The test here is equivalent to e_weight < 0 if m_combine has a
            // cancellation law, but always returns false when m_combine is a
            // projection operator.
            if (m_compare(m_combine(m_zero, get(m_weight, e)), m_zero))
                boost::throw_exception(negative_edge());
            // End of test for negative-weight edges.

            m_vis.examine_edge(e, g);
        }
        template < class Edge, class Graph > void black_target(Edge, Graph&) {}
        template < class Vertex, class Graph >
        void finish_vertex(Vertex u, Graph& g)
        {
            m_vis.finish_vertex(u, g);
        }

        UniformCostVisitor m_vis;
        UpdatableQueue& m_Q;
        WeightMap m_weight;
        PredecessorMap m_predecessor;
        DistanceMap m_distance;
        BinaryFunction m_combine;
        BinaryPredicate m_compare;
        D m_zero;
    };

} // namespace detail

namespace detail
{
    template < class Graph, class IndexMap, class Value, bool KnownNumVertices >
    struct vertex_property_map_generator_helper
    {
    };

    template < class Graph, class IndexMap, class Value >
    struct vertex_property_map_generator_helper< Graph, IndexMap, Value, true >
    {
        typedef boost::iterator_property_map< Value*, IndexMap > type;
        static type build(const Graph& g, const IndexMap& index,
            boost::scoped_array< Value >& array_holder)
        {
            array_holder.reset(new Value[num_vertices(g)]);
            std::fill(array_holder.get(), array_holder.get() + num_vertices(g),
                Value());
            return make_iterator_property_map(array_holder.get(), index);
        }
    };

    template < class Graph, class IndexMap, class Value >
    struct vertex_property_map_generator_helper< Graph, IndexMap, Value, false >
    {
        typedef boost::vector_property_map< Value, IndexMap > type;
        static type build(const Graph& g, const IndexMap& index,
            boost::scoped_array< Value >& array_holder)
        {
            return boost::make_vector_property_map< Value >(index);
        }
    };

    template < class Graph, class IndexMap, class Value >
    struct vertex_property_map_generator
    {
        typedef boost::is_base_and_derived< boost::vertex_list_graph_tag,
            typename boost::graph_traits< Graph >::traversal_category >
            known_num_vertices;
        typedef vertex_property_map_generator_helper< Graph, IndexMap, Value,
            known_num_vertices::value >
            helper;
        typedef typename helper::type type;
        static type build(const Graph& g, const IndexMap& index,
            boost::scoped_array< Value >& array_holder)
        {
            return helper::build(g, index, array_holder);
        }
    };
}

namespace detail
{
    template < class Graph, class IndexMap, bool KnownNumVertices >
    struct default_color_map_generator_helper
    {
    };

    template < class Graph, class IndexMap >
    struct default_color_map_generator_helper< Graph, IndexMap, true >
    {
        typedef boost::two_bit_color_map< IndexMap > type;
        static type build(const Graph& g, const IndexMap& index)
        {
            size_t nv = num_vertices(g);
            return boost::two_bit_color_map< IndexMap >(nv, index);
        }
    };

    template < class Graph, class IndexMap >
    struct default_color_map_generator_helper< Graph, IndexMap, false >
    {
        typedef boost::vector_property_map< boost::two_bit_color_type,
            IndexMap >
            type;
        static type build(const Graph& g, const IndexMap& index)
        {
            return boost::make_vector_property_map< boost::two_bit_color_type >(
                index);
        }
    };

    template < class Graph, class IndexMap > struct default_color_map_generator
    {
        typedef boost::is_base_and_derived< boost::vertex_list_graph_tag,
            typename boost::graph_traits< Graph >::traversal_category >
            known_num_vertices;
        typedef default_color_map_generator_helper< Graph, IndexMap,
            known_num_vertices::value >
            helper;
        typedef typename helper::type type;
        static type build(const Graph& g, const IndexMap& index)
        {
            return helper::build(g, index);
        }
    };
}

// Call breadth first search with default color map.
template < class Graph, class SourceInputIter, class DijkstraVisitor,
    class PredecessorMap, class DistanceMap, class WeightMap, class IndexMap,
    class Compare, class Combine, class DistZero >
inline void dijkstra_shortest_paths_no_init(const Graph& g,
    SourceInputIter s_begin, SourceInputIter s_end, PredecessorMap predecessor,
    DistanceMap distance, WeightMap weight, IndexMap index_map, Compare compare,
    Combine combine, DistZero zero, DijkstraVisitor vis)
{
    typedef detail::default_color_map_generator< Graph, IndexMap >
        ColorMapHelper;
    typedef typename ColorMapHelper::type ColorMap;
    ColorMap color = ColorMapHelper::build(g, index_map);
    dijkstra_shortest_paths_no_init(g, s_begin, s_end, predecessor, distance,
        weight, index_map, compare, combine, zero, vis, color);
}

// Call breadth first search with default color map.
template < class Graph, class DijkstraVisitor, class PredecessorMap,
    class DistanceMap, class WeightMap, class IndexMap, class Compare,
    class Combine, class DistZero >
inline void dijkstra_shortest_paths_no_init(const Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
    IndexMap index_map, Compare compare, Combine combine, DistZero zero,
    DijkstraVisitor vis)
{
    dijkstra_shortest_paths_no_init(g, &s, &s + 1, predecessor, distance,
        weight, index_map, compare, combine, zero, vis);
}

// Call breadth first search
template < class Graph, class SourceInputIter, class DijkstraVisitor,
    class PredecessorMap, class DistanceMap, class WeightMap, class IndexMap,
    class Compare, class Combine, class DistZero, class ColorMap >
inline void dijkstra_shortest_paths_no_init(const Graph& g,
    SourceInputIter s_begin, SourceInputIter s_end, PredecessorMap predecessor,
    DistanceMap distance, WeightMap weight, IndexMap index_map, Compare compare,
    Combine combine, DistZero zero, DijkstraVisitor vis, ColorMap color)
{
    typedef indirect_cmp< DistanceMap, Compare > IndirectCmp;
    IndirectCmp icmp(distance, compare);

    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

    // Now the default: use a d-ary heap
    boost::scoped_array< std::size_t > index_in_heap_map_holder;
    typedef detail::vertex_property_map_generator< Graph, IndexMap,
        std::size_t >
        IndexInHeapMapHelper;
    typedef typename IndexInHeapMapHelper::type IndexInHeapMap;
    IndexInHeapMap index_in_heap
        = IndexInHeapMapHelper::build(g, index_map, index_in_heap_map_holder);
    typedef d_ary_heap_indirect< Vertex, 4, IndexInHeapMap, DistanceMap,
        Compare >
        MutableQueue;
    MutableQueue Q(distance, index_in_heap, compare);

    detail::dijkstra_bfs_visitor< DijkstraVisitor, MutableQueue, WeightMap,
        PredecessorMap, DistanceMap, Combine, Compare >
        bfs_vis(vis, Q, weight, predecessor, distance, combine, compare, zero);

    breadth_first_visit(g, s_begin, s_end, Q, bfs_vis, color);
}

// Call breadth first search
template < class Graph, class DijkstraVisitor, class PredecessorMap,
    class DistanceMap, class WeightMap, class IndexMap, class Compare,
    class Combine, class DistZero, class ColorMap >
inline void dijkstra_shortest_paths_no_init(const Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
    IndexMap index_map, Compare compare, Combine combine, DistZero zero,
    DijkstraVisitor vis, ColorMap color)
{
    dijkstra_shortest_paths_no_init(g, &s, &s + 1, predecessor, distance,
        weight, index_map, compare, combine, zero, vis, color);
}

// Initialize distances and call breadth first search with default color map
template < class VertexListGraph, class SourceInputIter, class DijkstraVisitor,
    class PredecessorMap, class DistanceMap, class WeightMap, class IndexMap,
    class Compare, class Combine, class DistInf, class DistZero, typename T,
    typename Tag, typename Base >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    SourceInputIter s_begin, SourceInputIter s_end, PredecessorMap predecessor,
    DistanceMap distance, WeightMap weight, IndexMap index_map, Compare compare,
    Combine combine, DistInf inf, DistZero zero, DijkstraVisitor vis,
    const bgl_named_params< T, Tag, Base >& BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        VertexListGraph, vertex_list_graph_tag))
{
    boost::two_bit_color_map< IndexMap > color(num_vertices(g), index_map);
    dijkstra_shortest_paths(g, s_begin, s_end, predecessor, distance, weight,
        index_map, compare, combine, inf, zero, vis, color);
}

// Initialize distances and call breadth first search with default color map
template < class VertexListGraph, class DijkstraVisitor, class PredecessorMap,
    class DistanceMap, class WeightMap, class IndexMap, class Compare,
    class Combine, class DistInf, class DistZero, typename T, typename Tag,
    typename Base >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
    IndexMap index_map, Compare compare, Combine combine, DistInf inf,
    DistZero zero, DijkstraVisitor vis,
    const bgl_named_params< T, Tag, Base >& BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        VertexListGraph, vertex_list_graph_tag))
{
    dijkstra_shortest_paths(g, &s, &s + 1, predecessor, distance, weight,
        index_map, compare, combine, inf, zero, vis);
}

// Initialize distances and call breadth first search
template < class VertexListGraph, class SourceInputIter, class DijkstraVisitor,
    class PredecessorMap, class DistanceMap, class WeightMap, class IndexMap,
    class Compare, class Combine, class DistInf, class DistZero,
    class ColorMap >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    SourceInputIter s_begin, SourceInputIter s_end, PredecessorMap predecessor,
    DistanceMap distance, WeightMap weight, IndexMap index_map, Compare compare,
    Combine combine, DistInf inf, DistZero zero, DijkstraVisitor vis,
    ColorMap color)
{
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;
    typename graph_traits< VertexListGraph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        vis.initialize_vertex(*ui, g);
        put(distance, *ui, inf);
        put(predecessor, *ui, *ui);
        put(color, *ui, Color::white());
    }
    for (SourceInputIter it = s_begin; it != s_end; ++it)
    {
        put(distance, *it, zero);
    }

    dijkstra_shortest_paths_no_init(g, s_begin, s_end, predecessor, distance,
        weight, index_map, compare, combine, zero, vis, color);
}

// Initialize distances and call breadth first search
template < class VertexListGraph, class DijkstraVisitor, class PredecessorMap,
    class DistanceMap, class WeightMap, class IndexMap, class Compare,
    class Combine, class DistInf, class DistZero, class ColorMap >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
    IndexMap index_map, Compare compare, Combine combine, DistInf inf,
    DistZero zero, DijkstraVisitor vis, ColorMap color)
{
    dijkstra_shortest_paths(g, &s, &s + 1, predecessor, distance, weight,
        index_map, compare, combine, inf, zero, vis, color);
}

// Initialize distances and call breadth first search
template < class VertexListGraph, class SourceInputIter, class DijkstraVisitor,
    class PredecessorMap, class DistanceMap, class WeightMap, class IndexMap,
    class Compare, class Combine, class DistInf, class DistZero >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    SourceInputIter s_begin, SourceInputIter s_end, PredecessorMap predecessor,
    DistanceMap distance, WeightMap weight, IndexMap index_map, Compare compare,
    Combine combine, DistInf inf, DistZero zero, DijkstraVisitor vis)
{
    dijkstra_shortest_paths(g, s_begin, s_end, predecessor, distance, weight,
        index_map, compare, combine, inf, zero, vis, no_named_parameters());
}

// Initialize distances and call breadth first search
template < class VertexListGraph, class DijkstraVisitor, class PredecessorMap,
    class DistanceMap, class WeightMap, class IndexMap, class Compare,
    class Combine, class DistInf, class DistZero >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    PredecessorMap predecessor, DistanceMap distance, WeightMap weight,
    IndexMap index_map, Compare compare, Combine combine, DistInf inf,
    DistZero zero, DijkstraVisitor vis)
{
    dijkstra_shortest_paths(g, &s, &s + 1, predecessor, distance, weight,
        index_map, compare, combine, inf, zero, vis);
}

namespace detail
{

    // Handle defaults for PredecessorMap and
    // Distance Compare, Combine, Inf and Zero
    template < class VertexListGraph, class DistanceMap, class WeightMap,
        class IndexMap, class Params >
    inline void dijkstra_dispatch2(const VertexListGraph& g,
        typename graph_traits< VertexListGraph >::vertex_descriptor s,
        DistanceMap distance, WeightMap weight, IndexMap index_map,
        const Params& params)
    {
        // Default for predecessor map
        dummy_property_map p_map;

        typedef typename property_traits< DistanceMap >::value_type D;
        D inf = choose_param(get_param(params, distance_inf_t()),
            (std::numeric_limits< D >::max)());

        dijkstra_shortest_paths(g, s,
            choose_param(get_param(params, vertex_predecessor), p_map),
            distance, weight, index_map,
            choose_param(
                get_param(params, distance_compare_t()), std::less< D >()),
            choose_param(
                get_param(params, distance_combine_t()), std::plus< D >()),
            inf, choose_param(get_param(params, distance_zero_t()), D()),
            choose_param(get_param(params, graph_visitor),
                make_dijkstra_visitor(null_visitor())),
            params);
    }

    template < class VertexListGraph, class DistanceMap, class WeightMap,
        class IndexMap, class Params >
    inline void dijkstra_dispatch1(const VertexListGraph& g,
        typename graph_traits< VertexListGraph >::vertex_descriptor s,
        DistanceMap distance, WeightMap weight, IndexMap index_map,
        const Params& params)
    {
        // Default for distance map
        typedef typename property_traits< WeightMap >::value_type D;
        typename std::vector< D >::size_type n
            = is_default_param(distance) ? num_vertices(g) : 1;
        std::vector< D > distance_map(n);

        detail::dijkstra_dispatch2(g, s,
            choose_param(distance,
                make_iterator_property_map(
                    distance_map.begin(), index_map, distance_map[0])),
            weight, index_map, params);
    }
} // namespace detail

// Named Parameter Variant
template < class VertexListGraph, class Param, class Tag, class Rest >
inline void dijkstra_shortest_paths(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    // Default for edge weight and vertex index map is to ask for them
    // from the graph.  Default for the visitor is null_visitor.
    detail::dijkstra_dispatch1(g, s, get_param(params, vertex_distance),
        choose_const_pmap(get_param(params, edge_weight), g, edge_weight),
        choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
        params);
}

} // namespace boost

#include BOOST_GRAPH_MPI_INCLUDE(<boost/graph/distributed/dijkstra_shortest_paths.hpp>)

#endif // BOOST_GRAPH_DIJKSTRA_HPP

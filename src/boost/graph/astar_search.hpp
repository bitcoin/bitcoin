

//
//=======================================================================
// Copyright (c) 2004 Kristopher Beevers
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//

#ifndef BOOST_GRAPH_ASTAR_SEARCH_HPP
#define BOOST_GRAPH_ASTAR_SEARCH_HPP

#include <functional>
#include <vector>
#include <boost/limits.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/relax.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/detail/d_ary_heap.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/property_map/function_property_map.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{

template < class Heuristic, class Graph > struct AStarHeuristicConcept
{
    void constraints()
    {
        BOOST_CONCEPT_ASSERT((CopyConstructibleConcept< Heuristic >));
        h(u);
    }
    Heuristic h;
    typename graph_traits< Graph >::vertex_descriptor u;
};

template < class Graph, class CostType > class astar_heuristic
{
public:
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef Vertex argument_type;
    typedef CostType result_type;
    astar_heuristic() {}
    CostType operator()(Vertex u) { return static_cast< CostType >(0); }
};

template < class Visitor, class Graph > struct AStarVisitorConcept
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
        vis.black_target(e, g);
        vis.finish_vertex(u, g);
    }
    Visitor vis;
    Graph g;
    typename graph_traits< Graph >::vertex_descriptor u;
    typename graph_traits< Graph >::edge_descriptor e;
};

template < class Visitors = null_visitor >
class astar_visitor : public bfs_visitor< Visitors >
{
public:
    astar_visitor() {}
    astar_visitor(Visitors vis) : bfs_visitor< Visitors >(vis) {}

    template < class Edge, class Graph >
    void edge_relaxed(Edge e, const Graph& g)
    {
        invoke_visitors(this->m_vis, e, g, on_edge_relaxed());
    }
    template < class Edge, class Graph >
    void edge_not_relaxed(Edge e, const Graph& g)
    {
        invoke_visitors(this->m_vis, e, g, on_edge_not_relaxed());
    }

private:
    template < class Edge, class Graph > void tree_edge(Edge e, const Graph& g)
    {
    }
    template < class Edge, class Graph >
    void non_tree_edge(Edge e, const Graph& g)
    {
    }
};
template < class Visitors >
astar_visitor< Visitors > make_astar_visitor(Visitors vis)
{
    return astar_visitor< Visitors >(vis);
}
typedef astar_visitor<> default_astar_visitor;

namespace detail
{

    template < class AStarHeuristic, class UniformCostVisitor,
        class UpdatableQueue, class PredecessorMap, class CostMap,
        class DistanceMap, class WeightMap, class ColorMap,
        class BinaryFunction, class BinaryPredicate >
    struct astar_bfs_visitor
    {

        typedef typename property_traits< CostMap >::value_type C;
        typedef typename property_traits< ColorMap >::value_type ColorValue;
        typedef color_traits< ColorValue > Color;
        typedef
            typename property_traits< DistanceMap >::value_type distance_type;

        astar_bfs_visitor(AStarHeuristic h, UniformCostVisitor vis,
            UpdatableQueue& Q, PredecessorMap p, CostMap c, DistanceMap d,
            WeightMap w, ColorMap col, BinaryFunction combine,
            BinaryPredicate compare, C zero)
        : m_h(h)
        , m_vis(vis)
        , m_Q(Q)
        , m_predecessor(p)
        , m_cost(c)
        , m_distance(d)
        , m_weight(w)
        , m_color(col)
        , m_combine(combine)
        , m_compare(compare)
        , m_zero(zero)
        {
        }

        template < class Vertex, class Graph >
        void initialize_vertex(Vertex u, const Graph& g)
        {
            m_vis.initialize_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void discover_vertex(Vertex u, const Graph& g)
        {
            m_vis.discover_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void examine_vertex(Vertex u, const Graph& g)
        {
            m_vis.examine_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void finish_vertex(Vertex u, const Graph& g)
        {
            m_vis.finish_vertex(u, g);
        }
        template < class Edge, class Graph >
        void examine_edge(Edge e, const Graph& g)
        {
            if (m_compare(get(m_weight, e), m_zero))
                BOOST_THROW_EXCEPTION(negative_edge());
            m_vis.examine_edge(e, g);
        }
        template < class Edge, class Graph >
        void non_tree_edge(Edge, const Graph&)
        {
        }

        template < class Edge, class Graph >
        void tree_edge(Edge e, const Graph& g)
        {
            using boost::get;
            bool m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                m_combine, m_compare);

            if (m_decreased)
            {
                m_vis.edge_relaxed(e, g);
                put(m_cost, target(e, g),
                    m_combine(
                        get(m_distance, target(e, g)), m_h(target(e, g))));
            }
            else
                m_vis.edge_not_relaxed(e, g);
        }

        template < class Edge, class Graph >
        void gray_target(Edge e, const Graph& g)
        {
            using boost::get;
            bool m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                m_combine, m_compare);

            if (m_decreased)
            {
                put(m_cost, target(e, g),
                    m_combine(
                        get(m_distance, target(e, g)), m_h(target(e, g))));
                m_Q.update(target(e, g));
                m_vis.edge_relaxed(e, g);
            }
            else
                m_vis.edge_not_relaxed(e, g);
        }

        template < class Edge, class Graph >
        void black_target(Edge e, const Graph& g)
        {
            using boost::get;
            bool m_decreased = relax(e, g, m_weight, m_predecessor, m_distance,
                m_combine, m_compare);

            if (m_decreased)
            {
                m_vis.edge_relaxed(e, g);
                put(m_cost, target(e, g),
                    m_combine(
                        get(m_distance, target(e, g)), m_h(target(e, g))));
                m_Q.push(target(e, g));
                put(m_color, target(e, g), Color::gray());
                m_vis.black_target(e, g);
            }
            else
                m_vis.edge_not_relaxed(e, g);
        }

        AStarHeuristic m_h;
        UniformCostVisitor m_vis;
        UpdatableQueue& m_Q;
        PredecessorMap m_predecessor;
        CostMap m_cost;
        DistanceMap m_distance;
        WeightMap m_weight;
        ColorMap m_color;
        BinaryFunction m_combine;
        BinaryPredicate m_compare;
        C m_zero;
    };

} // namespace detail

template < typename VertexListGraph, typename AStarHeuristic,
    typename AStarVisitor, typename PredecessorMap, typename CostMap,
    typename DistanceMap, typename WeightMap, typename ColorMap,
    typename VertexIndexMap, typename CompareFunction, typename CombineFunction,
    typename CostInf, typename CostZero >
inline void astar_search_no_init(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, AStarVisitor vis, PredecessorMap predecessor,
    CostMap cost, DistanceMap distance, WeightMap weight, ColorMap color,
    VertexIndexMap index_map, CompareFunction compare, CombineFunction combine,
    CostInf /*inf*/, CostZero zero)
{
    typedef typename graph_traits< VertexListGraph >::vertex_descriptor Vertex;
    typedef boost::vector_property_map< std::size_t, VertexIndexMap >
        IndexInHeapMap;
    IndexInHeapMap index_in_heap(index_map);
    typedef d_ary_heap_indirect< Vertex, 4, IndexInHeapMap, CostMap,
        CompareFunction >
        MutableQueue;
    MutableQueue Q(cost, index_in_heap, compare);

    detail::astar_bfs_visitor< AStarHeuristic, AStarVisitor, MutableQueue,
        PredecessorMap, CostMap, DistanceMap, WeightMap, ColorMap,
        CombineFunction, CompareFunction >
        bfs_vis(h, vis, Q, predecessor, cost, distance, weight, color, combine,
            compare, zero);

    breadth_first_visit(g, s, Q, bfs_vis, color);
}

namespace graph_detail
{
    template < typename A, typename B > struct select1st
    {
        typedef std::pair< A, B > argument_type;
        typedef A result_type;
        A operator()(const std::pair< A, B >& p) const { return p.first; }
    };
}

template < typename VertexListGraph, typename AStarHeuristic,
    typename AStarVisitor, typename PredecessorMap, typename CostMap,
    typename DistanceMap, typename WeightMap, typename CompareFunction,
    typename CombineFunction, typename CostInf, typename CostZero >
inline void astar_search_no_init_tree(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, AStarVisitor vis, PredecessorMap predecessor,
    CostMap cost, DistanceMap distance, WeightMap weight,
    CompareFunction compare, CombineFunction combine, CostInf /*inf*/,
    CostZero zero)
{
    typedef typename graph_traits< VertexListGraph >::vertex_descriptor Vertex;
    typedef typename property_traits< DistanceMap >::value_type Distance;
    typedef d_ary_heap_indirect< std::pair< Distance, Vertex >, 4,
        null_property_map< std::pair< Distance, Vertex >, std::size_t >,
        function_property_map< graph_detail::select1st< Distance, Vertex >,
            std::pair< Distance, Vertex > >,
        CompareFunction >
        MutableQueue;
    MutableQueue Q(make_function_property_map< std::pair< Distance, Vertex > >(
                       graph_detail::select1st< Distance, Vertex >()),
        null_property_map< std::pair< Distance, Vertex >, std::size_t >(),
        compare);

    vis.discover_vertex(s, g);
    Q.push(std::make_pair(get(cost, s), s));
    while (!Q.empty())
    {
        Vertex v;
        Distance v_rank;
        boost::tie(v_rank, v) = Q.top();
        Q.pop();
        vis.examine_vertex(v, g);
        BGL_FORALL_OUTEDGES_T(v, e, g, VertexListGraph)
        {
            Vertex w = target(e, g);
            vis.examine_edge(e, g);
            Distance e_weight = get(weight, e);
            if (compare(e_weight, zero))
                BOOST_THROW_EXCEPTION(negative_edge());
            bool decreased
                = relax(e, g, weight, predecessor, distance, combine, compare);
            combine(get(distance, v), e_weight);
            if (decreased)
            {
                vis.edge_relaxed(e, g);
                Distance w_rank = combine(get(distance, w), h(w));
                put(cost, w, w_rank);
                vis.discover_vertex(w, g);
                Q.push(std::make_pair(w_rank, w));
            }
            else
            {
                vis.edge_not_relaxed(e, g);
            }
        }
        vis.finish_vertex(v, g);
    }
}

// Non-named parameter interface
template < typename VertexListGraph, typename AStarHeuristic,
    typename AStarVisitor, typename PredecessorMap, typename CostMap,
    typename DistanceMap, typename WeightMap, typename VertexIndexMap,
    typename ColorMap, typename CompareFunction, typename CombineFunction,
    typename CostInf, typename CostZero >
inline void astar_search(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, AStarVisitor vis, PredecessorMap predecessor,
    CostMap cost, DistanceMap distance, WeightMap weight,
    VertexIndexMap index_map, ColorMap color, CompareFunction compare,
    CombineFunction combine, CostInf inf, CostZero zero)
{

    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;
    typename graph_traits< VertexListGraph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(color, *ui, Color::white());
        put(distance, *ui, inf);
        put(cost, *ui, inf);
        put(predecessor, *ui, *ui);
        vis.initialize_vertex(*ui, g);
    }
    put(distance, s, zero);
    put(cost, s, h(s));

    astar_search_no_init(g, s, h, vis, predecessor, cost, distance, weight,
        color, index_map, compare, combine, inf, zero);
}

// Non-named parameter interface
template < typename VertexListGraph, typename AStarHeuristic,
    typename AStarVisitor, typename PredecessorMap, typename CostMap,
    typename DistanceMap, typename WeightMap, typename CompareFunction,
    typename CombineFunction, typename CostInf, typename CostZero >
inline void astar_search_tree(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, AStarVisitor vis, PredecessorMap predecessor,
    CostMap cost, DistanceMap distance, WeightMap weight,
    CompareFunction compare, CombineFunction combine, CostInf inf,
    CostZero zero)
{

    typename graph_traits< VertexListGraph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(distance, *ui, inf);
        put(cost, *ui, inf);
        put(predecessor, *ui, *ui);
        vis.initialize_vertex(*ui, g);
    }
    put(distance, s, zero);
    put(cost, s, h(s));

    astar_search_no_init_tree(g, s, h, vis, predecessor, cost, distance, weight,
        compare, combine, inf, zero);
}

// Named parameter interfaces
template < typename VertexListGraph, typename AStarHeuristic, typename P,
    typename T, typename R >
void astar_search(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, const bgl_named_params< P, T, R >& params)
{
    using namespace boost::graph::keywords;
    typedef bgl_named_params< P, T, R > params_type;
    BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)

    // Distance type is the value type of the distance map if there is one,
    // otherwise the value type of the weight map.
    typedef
        typename boost::detail::override_const_property_result< arg_pack_type,
            boost::graph::keywords::tag::weight_map, edge_weight_t,
            VertexListGraph >::type weight_map_type;
    typedef typename boost::property_traits< weight_map_type >::value_type D;
    const D inf = arg_pack[_distance_inf || detail::get_max< D >()];
    const D zero_actual = D();
    const D zero_d = arg_pack[_distance_zero | zero_actual];
    null_visitor null_vis;
    astar_visitor< null_visitor > default_visitor(null_vis);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::visitor, dummy_property_map& >::type vis
        = arg_pack[_visitor | default_visitor];
    dummy_property_map dummy_prop;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::predecessor_map,
        dummy_property_map& >::type pred_map
        = arg_pack[_predecessor_map | dummy_prop];
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::rank_map, D >
        rank_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::rank_map, D >::map_type r_map
        = rank_map_gen(g, arg_pack);
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::distance_map, D >
        dist_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::distance_map, D >::map_type dist_map
        = dist_map_gen(g, arg_pack);
    weight_map_type w_map = detail::override_const_property(
        arg_pack, _weight_map, g, edge_weight);
    typename boost::detail::override_const_property_result< arg_pack_type,
        boost::graph::keywords::tag::vertex_index_map, vertex_index_t,
        VertexListGraph >::type v_i_map
        = detail::override_const_property(
            arg_pack, _vertex_index_map, g, vertex_index);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::color_map,
        boost::default_color_type >::map_type c_map
        = boost::detail::make_color_map_from_arg_pack(g, arg_pack);
    std::less< D > default_compare;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_compare, std::less< D >& >::type
        dist_comp
        = arg_pack[_distance_compare | default_compare];
    closed_plus< D > default_combine(inf);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_combine, closed_plus< D >& >::type
        dist_comb
        = arg_pack[_distance_combine | default_combine];
    astar_search(g, s, h, vis, pred_map, r_map, dist_map, w_map, v_i_map, c_map,
        dist_comp, dist_comb, inf, zero_d);
}

template < typename VertexListGraph, typename AStarHeuristic, typename P,
    typename T, typename R >
void astar_search_tree(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, const bgl_named_params< P, T, R >& params)
{
    using namespace boost::graph::keywords;
    typedef bgl_named_params< P, T, R > params_type;
    BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)

    // Distance type is the value type of the distance map if there is one,
    // otherwise the value type of the weight map.
    typedef
        typename boost::detail::override_const_property_result< arg_pack_type,
            boost::graph::keywords::tag::weight_map, edge_weight_t,
            VertexListGraph >::type weight_map_type;
    typedef typename boost::property_traits< weight_map_type >::value_type D;
    const D inf = arg_pack[_distance_inf || detail::get_max< D >()];
    const D zero_actual = D();
    const D zero_d = arg_pack[_distance_zero | zero_actual];
    null_visitor null_vis;
    astar_visitor< null_visitor > default_visitor(null_vis);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::visitor, dummy_property_map& >::type vis
        = arg_pack[_visitor | default_visitor];
    dummy_property_map dummy_prop;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::predecessor_map,
        dummy_property_map& >::type pred_map
        = arg_pack[_predecessor_map | dummy_prop];
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::rank_map, D >
        rank_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::rank_map, D >::map_type r_map
        = rank_map_gen(g, arg_pack);
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::distance_map, D >
        dist_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::distance_map, D >::map_type dist_map
        = dist_map_gen(g, arg_pack);
    weight_map_type w_map = detail::override_const_property(
        arg_pack, _weight_map, g, edge_weight);
    std::less< D > default_compare;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_compare, std::less< D >& >::type
        dist_comp
        = arg_pack[_distance_compare | default_compare];
    closed_plus< D > default_combine(inf);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_combine, closed_plus< D >& >::type
        dist_comb
        = arg_pack[_distance_combine | default_combine];
    astar_search_tree(g, s, h, vis, pred_map, r_map, dist_map, w_map, dist_comp,
        dist_comb, inf, zero_d);
}

template < typename VertexListGraph, typename AStarHeuristic, typename P,
    typename T, typename R >
void astar_search_no_init(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, const bgl_named_params< P, T, R >& params)
{
    using namespace boost::graph::keywords;
    typedef bgl_named_params< P, T, R > params_type;
    BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)
    typedef
        typename boost::detail::override_const_property_result< arg_pack_type,
            boost::graph::keywords::tag::weight_map, edge_weight_t,
            VertexListGraph >::type weight_map_type;
    typedef typename boost::property_traits< weight_map_type >::value_type D;
    const D inf = arg_pack[_distance_inf || detail::get_max< D >()];
    const D zero_actual = D();
    const D zero_d = arg_pack[_distance_zero | zero_actual];
    null_visitor null_vis;
    astar_visitor< null_visitor > default_visitor(null_vis);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::visitor, dummy_property_map& >::type vis
        = arg_pack[_visitor | default_visitor];
    dummy_property_map dummy_prop;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::predecessor_map,
        dummy_property_map& >::type pred_map
        = arg_pack[_predecessor_map | dummy_prop];
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::rank_map, D >
        rank_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::rank_map, D >::map_type r_map
        = rank_map_gen(g, arg_pack);
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::distance_map, D >
        dist_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::distance_map, D >::map_type dist_map
        = dist_map_gen(g, arg_pack);
    weight_map_type w_map = detail::override_const_property(
        arg_pack, _weight_map, g, edge_weight);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::color_map,
        boost::default_color_type >::map_type c_map
        = boost::detail::make_color_map_from_arg_pack(g, arg_pack);
    typename boost::detail::override_const_property_result< arg_pack_type,
        boost::graph::keywords::tag::vertex_index_map, vertex_index_t,
        VertexListGraph >::type v_i_map
        = detail::override_const_property(
            arg_pack, _vertex_index_map, g, vertex_index);
    std::less< D > default_compare;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_compare, std::less< D >& >::type
        dist_comp
        = arg_pack[_distance_compare | default_compare];
    closed_plus< D > default_combine(inf);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_combine, closed_plus< D >& >::type
        dist_comb
        = arg_pack[_distance_combine | default_combine];
    astar_search_no_init(g, s, h, vis, pred_map, r_map, dist_map, w_map, c_map,
        v_i_map, dist_comp, dist_comb, inf, zero_d);
}

template < typename VertexListGraph, typename AStarHeuristic, typename P,
    typename T, typename R >
void astar_search_no_init_tree(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    AStarHeuristic h, const bgl_named_params< P, T, R >& params)
{
    using namespace boost::graph::keywords;
    typedef bgl_named_params< P, T, R > params_type;
    BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)
    typedef
        typename boost::detail::override_const_property_result< arg_pack_type,
            boost::graph::keywords::tag::weight_map, edge_weight_t,
            VertexListGraph >::type weight_map_type;
    typedef typename boost::property_traits< weight_map_type >::value_type D;
    const D inf = arg_pack[_distance_inf || detail::get_max< D >()];
    const D zero_actual = D();
    const D zero_d = arg_pack[_distance_zero | zero_actual];
    null_visitor null_vis;
    astar_visitor< null_visitor > default_visitor(null_vis);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::visitor, dummy_property_map& >::type vis
        = arg_pack[_visitor | default_visitor];
    dummy_property_map dummy_prop;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::predecessor_map,
        dummy_property_map& >::type pred_map
        = arg_pack[_predecessor_map | dummy_prop];
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::rank_map, D >
        rank_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::rank_map, D >::map_type r_map
        = rank_map_gen(g, arg_pack);
    boost::detail::make_property_map_from_arg_pack_gen<
        boost::graph::keywords::tag::distance_map, D >
        dist_map_gen(zero_actual);
    typename boost::detail::map_maker< VertexListGraph, arg_pack_type,
        boost::graph::keywords::tag::distance_map, D >::map_type dist_map
        = dist_map_gen(g, arg_pack);
    weight_map_type w_map = detail::override_const_property(
        arg_pack, _weight_map, g, edge_weight);
    std::less< D > default_compare;
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_compare, std::less< D >& >::type
        dist_comp
        = arg_pack[_distance_compare | default_compare];
    closed_plus< D > default_combine(inf);
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::distance_combine, closed_plus< D >& >::type
        dist_comb
        = arg_pack[_distance_combine | default_combine];
    astar_search_no_init_tree(g, s, h, vis, pred_map, r_map, dist_map, w_map,
        dist_comp, dist_comb, inf, zero_d);
}

} // namespace boost

#endif // BOOST_GRAPH_ASTAR_SEARCH_HPP

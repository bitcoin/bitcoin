//=======================================================================
// Copyright 2013 University of Warsaw.
// Authors: Piotr Wygocki
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
//
// This algorithm is described in "Network Flows: Theory, Algorithms, and
// Applications"
// by Ahuja, Magnanti, Orlin.

#ifndef BOOST_GRAPH_CYCLE_CANCELING_HPP
#define BOOST_GRAPH_CYCLE_CANCELING_HPP

#include <numeric>

#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/detail/augment.hpp>
#include <boost/graph/find_flow_cost.hpp>

namespace boost
{

namespace detail
{

    template < typename PredEdgeMap, typename Vertex >
    class RecordEdgeMapAndCycleVertex
    : public bellman_visitor<
          edge_predecessor_recorder< PredEdgeMap, on_edge_relaxed > >
    {
        typedef edge_predecessor_recorder< PredEdgeMap, on_edge_relaxed >
            PredRec;

    public:
        RecordEdgeMapAndCycleVertex(PredEdgeMap pred, Vertex& v)
        : bellman_visitor< PredRec >(PredRec(pred)), m_v(v), m_pred(pred)
        {
        }

        template < typename Graph, typename Edge >
        void edge_not_minimized(Edge e, const Graph& g) const
        {
            typename graph_traits< Graph >::vertices_size_type n
                = num_vertices(g) + 1;

            // edge e is not minimized but does not have to be on the negative
            // weight cycle to find vertex on negative wieight cycle we move n+1
            // times backword in the PredEdgeMap graph.
            while (n > 0)
            {
                e = get(m_pred, source(e, g));
                --n;
            }
            m_v = source(e, g);
        }

    private:
        Vertex& m_v;
        PredEdgeMap m_pred;
    };

} // detail

template < class Graph, class Pred, class Distance, class Reversed,
    class ResidualCapacity, class Weight >
void cycle_canceling(const Graph& g, Weight weight, Reversed rev,
    ResidualCapacity residual_capacity, Pred pred, Distance distance)
{
    typedef filtered_graph< const Graph, is_residual_edge< ResidualCapacity > >
        ResGraph;
    ResGraph gres = detail::residual_graph(g, residual_capacity);

    typedef graph_traits< ResGraph > ResGTraits;
    typedef graph_traits< Graph > GTraits;
    typedef typename ResGTraits::edge_descriptor edge_descriptor;
    typedef typename ResGTraits::vertex_descriptor vertex_descriptor;

    typename GTraits::vertices_size_type N = num_vertices(g);

    BGL_FORALL_VERTICES_T(v, g, Graph)
    {
        put(pred, v, edge_descriptor());
        put(distance, v, 0);
    }

    vertex_descriptor cycleStart;
    while (!bellman_ford_shortest_paths(gres, N,
        weight_map(weight).distance_map(distance).visitor(
            detail::RecordEdgeMapAndCycleVertex< Pred, vertex_descriptor >(
                pred, cycleStart))))
    {

        detail::augment(
            g, cycleStart, cycleStart, pred, residual_capacity, rev);

        BGL_FORALL_VERTICES_T(v, g, Graph)
        {
            put(pred, v, edge_descriptor());
            put(distance, v, 0);
        }
    }
}

// in this namespace argument dispatching takes place
namespace detail
{

    template < class Graph, class P, class T, class R, class ResidualCapacity,
        class Weight, class Reversed, class Pred, class Distance >
    void cycle_canceling_dispatch2(const Graph& g, Weight weight, Reversed rev,
        ResidualCapacity residual_capacity, Pred pred, Distance dist,
        const bgl_named_params< P, T, R >& params)
    {
        cycle_canceling(g, weight, rev, residual_capacity, pred, dist);
    }

    // setting default distance map
    template < class Graph, class P, class T, class R, class Pred,
        class ResidualCapacity, class Weight, class Reversed >
    void cycle_canceling_dispatch2(Graph& g, Weight weight, Reversed rev,
        ResidualCapacity residual_capacity, Pred pred, param_not_found,
        const bgl_named_params< P, T, R >& params)
    {
        typedef typename property_traits< Weight >::value_type D;

        std::vector< D > d_map(num_vertices(g));

        cycle_canceling(g, weight, rev, residual_capacity, pred,
            make_iterator_property_map(d_map.begin(),
                choose_const_pmap(
                    get_param(params, vertex_index), g, vertex_index)));
    }

    template < class Graph, class P, class T, class R, class ResidualCapacity,
        class Weight, class Reversed, class Pred >
    void cycle_canceling_dispatch1(Graph& g, Weight weight, Reversed rev,
        ResidualCapacity residual_capacity, Pred pred,
        const bgl_named_params< P, T, R >& params)
    {
        cycle_canceling_dispatch2(g, weight, rev, residual_capacity, pred,
            get_param(params, vertex_distance), params);
    }

    // setting default predecessors map
    template < class Graph, class P, class T, class R, class ResidualCapacity,
        class Weight, class Reversed >
    void cycle_canceling_dispatch1(Graph& g, Weight weight, Reversed rev,
        ResidualCapacity residual_capacity, param_not_found,
        const bgl_named_params< P, T, R >& params)
    {
        typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor;
        std::vector< edge_descriptor > p_map(num_vertices(g));

        cycle_canceling_dispatch2(g, weight, rev, residual_capacity,
            make_iterator_property_map(p_map.begin(),
                choose_const_pmap(
                    get_param(params, vertex_index), g, vertex_index)),
            get_param(params, vertex_distance), params);
    }

} // detail

template < class Graph, class P, class T, class R >
void cycle_canceling(Graph& g, const bgl_named_params< P, T, R >& params)
{
    cycle_canceling_dispatch1(g,
        choose_const_pmap(get_param(params, edge_weight), g, edge_weight),
        choose_const_pmap(get_param(params, edge_reverse), g, edge_reverse),
        choose_pmap(get_param(params, edge_residual_capacity), g,
            edge_residual_capacity),
        get_param(params, vertex_predecessor), params);
}

template < class Graph > void cycle_canceling(Graph& g)
{
    bgl_named_params< int, buffer_param_t > params(0);
    cycle_canceling(g, params);
}

}

#endif /* BOOST_GRAPH_CYCLE_CANCELING_HPP */

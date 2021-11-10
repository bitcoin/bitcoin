//=======================================================================
// Copyright 2000 University of Notre Dame.
// Authors: Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_EDGE_CONNECTIVITY
#define BOOST_EDGE_CONNECTIVITY

// WARNING: not-yet fully tested!

#include <boost/config.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <boost/graph/edmonds_karp_max_flow.hpp>

namespace boost
{

namespace detail
{

    template < class Graph >
    inline std::pair< typename graph_traits< Graph >::vertex_descriptor,
        typename graph_traits< Graph >::degree_size_type >
    min_degree_vertex(Graph& g)
    {
        typedef graph_traits< Graph > Traits;
        typename Traits::vertex_descriptor p;
        typedef typename Traits::degree_size_type size_type;
        size_type delta = (std::numeric_limits< size_type >::max)();

        typename Traits::vertex_iterator i, iend;
        for (boost::tie(i, iend) = vertices(g); i != iend; ++i)
            if (degree(*i, g) < delta)
            {
                delta = degree(*i, g);
                p = *i;
            }
        return std::make_pair(p, delta);
    }

    template < class Graph, class OutputIterator >
    void neighbors(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor u,
        OutputIterator result)
    {
        typename graph_traits< Graph >::adjacency_iterator ai, aend;
        for (boost::tie(ai, aend) = adjacent_vertices(u, g); ai != aend; ++ai)
            *result++ = *ai;
    }

    template < class Graph, class VertexIterator, class OutputIterator >
    void neighbors(const Graph& g, VertexIterator first, VertexIterator last,
        OutputIterator result)
    {
        for (; first != last; ++first)
            neighbors(g, *first, result);
    }

} // namespace detail

// O(m n)
template < class VertexListGraph, class OutputIterator >
typename graph_traits< VertexListGraph >::degree_size_type edge_connectivity(
    VertexListGraph& g, OutputIterator disconnecting_set)
{
    //-------------------------------------------------------------------------
    // Type Definitions
    typedef graph_traits< VertexListGraph > Traits;
    typedef typename Traits::vertex_iterator vertex_iterator;
    typedef typename Traits::edge_iterator edge_iterator;
    typedef typename Traits::out_edge_iterator out_edge_iterator;
    typedef typename Traits::vertex_descriptor vertex_descriptor;
    typedef typename Traits::degree_size_type degree_size_type;
    typedef color_traits< default_color_type > Color;

    typedef adjacency_list_traits< vecS, vecS, directedS > Tr;
    typedef typename Tr::edge_descriptor Tr_edge_desc;
    typedef adjacency_list< vecS, vecS, directedS, no_property,
        property< edge_capacity_t, degree_size_type,
            property< edge_residual_capacity_t, degree_size_type,
                property< edge_reverse_t, Tr_edge_desc > > > >
        FlowGraph;
    typedef typename graph_traits< FlowGraph >::edge_descriptor edge_descriptor;

    //-------------------------------------------------------------------------
    // Variable Declarations
    vertex_descriptor u, v, p, k;
    edge_descriptor e1, e2;
    bool inserted;
    vertex_iterator vi, vi_end;
    edge_iterator ei, ei_end;
    degree_size_type delta, alpha_star, alpha_S_k;
    std::set< vertex_descriptor > S, neighbor_S;
    std::vector< vertex_descriptor > S_star, non_neighbor_S;
    std::vector< default_color_type > color(num_vertices(g));
    std::vector< edge_descriptor > pred(num_vertices(g));

    //-------------------------------------------------------------------------
    // Create a network flow graph out of the undirected graph
    FlowGraph flow_g(num_vertices(g));

    typename property_map< FlowGraph, edge_capacity_t >::type cap
        = get(edge_capacity, flow_g);
    typename property_map< FlowGraph, edge_residual_capacity_t >::type res_cap
        = get(edge_residual_capacity, flow_g);
    typename property_map< FlowGraph, edge_reverse_t >::type rev_edge
        = get(edge_reverse, flow_g);

    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    {
        u = source(*ei, g), v = target(*ei, g);
        boost::tie(e1, inserted) = add_edge(u, v, flow_g);
        cap[e1] = 1;
        boost::tie(e2, inserted) = add_edge(v, u, flow_g);
        cap[e2] = 1; // not sure about this
        rev_edge[e1] = e2;
        rev_edge[e2] = e1;
    }

    //-------------------------------------------------------------------------
    // The Algorithm

    boost::tie(p, delta) = detail::min_degree_vertex(g);
    S_star.push_back(p);
    alpha_star = delta;
    S.insert(p);
    neighbor_S.insert(p);
    detail::neighbors(
        g, S.begin(), S.end(), std::inserter(neighbor_S, neighbor_S.begin()));

    boost::tie(vi, vi_end) = vertices(g);
    std::set_difference(vi, vi_end, neighbor_S.begin(), neighbor_S.end(),
        std::back_inserter(non_neighbor_S));

    while (!non_neighbor_S.empty())
    { // at most n - 1 times
        k = non_neighbor_S.front();

        alpha_S_k = edmonds_karp_max_flow(
            flow_g, p, k, cap, res_cap, rev_edge, &color[0], &pred[0]);

        if (alpha_S_k < alpha_star)
        {
            alpha_star = alpha_S_k;
            S_star.clear();
            for (boost::tie(vi, vi_end) = vertices(flow_g); vi != vi_end; ++vi)
                if (color[*vi] != Color::white())
                    S_star.push_back(*vi);
        }
        S.insert(k);
        neighbor_S.insert(k);
        detail::neighbors(g, k, std::inserter(neighbor_S, neighbor_S.begin()));
        non_neighbor_S.clear();
        boost::tie(vi, vi_end) = vertices(g);
        std::set_difference(vi, vi_end, neighbor_S.begin(), neighbor_S.end(),
            std::back_inserter(non_neighbor_S));
    }
    //-------------------------------------------------------------------------
    // Compute edges of the cut [S*, ~S*]
    std::vector< bool > in_S_star(num_vertices(g), false);
    typename std::vector< vertex_descriptor >::iterator si;
    for (si = S_star.begin(); si != S_star.end(); ++si)
        in_S_star[*si] = true;

    degree_size_type c = 0;
    for (si = S_star.begin(); si != S_star.end(); ++si)
    {
        out_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = out_edges(*si, g); ei != ei_end; ++ei)
            if (!in_S_star[target(*ei, g)])
            {
                *disconnecting_set++ = *ei;
                ++c;
            }
    }
    return c;
}

} // namespace boost

#endif // BOOST_EDGE_CONNECTIVITY

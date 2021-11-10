//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_GRAPH_NEIGHBOR_BREADTH_FIRST_SEARCH_HPP
#define BOOST_GRAPH_NEIGHBOR_BREADTH_FIRST_SEARCH_HPP

/*
  Neighbor Breadth First Search
  Like BFS, but traverses in-edges as well as out-edges.
  (for directed graphs only. use normal BFS for undirected graphs)
*/
#include <boost/config.hpp>
#include <boost/ref.hpp>
#include <vector>
#include <boost/pending/queue.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{

template < class Visitor, class Graph > struct NeighborBFSVisitorConcept
{
    void constraints()
    {
        BOOST_CONCEPT_ASSERT((CopyConstructibleConcept< Visitor >));
        vis.initialize_vertex(u, g);
        vis.discover_vertex(u, g);
        vis.examine_vertex(u, g);
        vis.examine_out_edge(e, g);
        vis.examine_in_edge(e, g);
        vis.tree_out_edge(e, g);
        vis.tree_in_edge(e, g);
        vis.non_tree_out_edge(e, g);
        vis.non_tree_in_edge(e, g);
        vis.gray_target(e, g);
        vis.black_target(e, g);
        vis.gray_source(e, g);
        vis.black_source(e, g);
        vis.finish_vertex(u, g);
    }
    Visitor vis;
    Graph g;
    typename graph_traits< Graph >::vertex_descriptor u;
    typename graph_traits< Graph >::edge_descriptor e;
};

template < class Visitors = null_visitor > class neighbor_bfs_visitor
{
public:
    neighbor_bfs_visitor(Visitors vis = Visitors()) : m_vis(vis) {}

    template < class Vertex, class Graph >
    void initialize_vertex(Vertex u, Graph& g)
    {
        invoke_visitors(m_vis, u, g, on_initialize_vertex());
    }
    template < class Vertex, class Graph >
    void discover_vertex(Vertex u, Graph& g)
    {
        invoke_visitors(m_vis, u, g, on_discover_vertex());
    }
    template < class Vertex, class Graph >
    void examine_vertex(Vertex u, Graph& g)
    {
        invoke_visitors(m_vis, u, g, on_examine_vertex());
    }
    template < class Edge, class Graph > void examine_out_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_examine_edge());
    }
    template < class Edge, class Graph > void tree_out_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_tree_edge());
    }
    template < class Edge, class Graph >
    void non_tree_out_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_non_tree_edge());
    }
    template < class Edge, class Graph > void gray_target(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_gray_target());
    }
    template < class Edge, class Graph > void black_target(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_black_target());
    }
    template < class Edge, class Graph > void examine_in_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_examine_edge());
    }
    template < class Edge, class Graph > void tree_in_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_tree_edge());
    }
    template < class Edge, class Graph > void non_tree_in_edge(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_non_tree_edge());
    }
    template < class Edge, class Graph > void gray_source(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_gray_target());
    }
    template < class Edge, class Graph > void black_source(Edge e, Graph& g)
    {
        invoke_visitors(m_vis, e, g, on_black_target());
    }
    template < class Vertex, class Graph >
    void finish_vertex(Vertex u, Graph& g)
    {
        invoke_visitors(m_vis, u, g, on_finish_vertex());
    }

protected:
    Visitors m_vis;
};

template < class Visitors >
neighbor_bfs_visitor< Visitors > make_neighbor_bfs_visitor(Visitors vis)
{
    return neighbor_bfs_visitor< Visitors >(vis);
}

namespace detail
{

    template < class BidirectionalGraph, class Buffer, class BFSVisitor,
        class ColorMap >
    void neighbor_bfs_impl(const BidirectionalGraph& g,
        typename graph_traits< BidirectionalGraph >::vertex_descriptor s,
        Buffer& Q, BFSVisitor vis, ColorMap color)

    {
        BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< BidirectionalGraph >));
        typedef graph_traits< BidirectionalGraph > GTraits;
        typedef typename GTraits::vertex_descriptor Vertex;
        typedef typename GTraits::edge_descriptor Edge;
        BOOST_CONCEPT_ASSERT(
            (NeighborBFSVisitorConcept< BFSVisitor, BidirectionalGraph >));
        BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< ColorMap, Vertex >));
        typedef typename property_traits< ColorMap >::value_type ColorValue;
        typedef color_traits< ColorValue > Color;

        put(color, s, Color::gray());
        vis.discover_vertex(s, g);
        Q.push(s);
        while (!Q.empty())
        {
            Vertex u = Q.top();
            Q.pop(); // pop before push to avoid problem if Q is priority_queue.
            vis.examine_vertex(u, g);

            typename GTraits::out_edge_iterator ei, ei_end;
            for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
            {
                Edge e = *ei;
                vis.examine_out_edge(e, g);
                Vertex v = target(e, g);
                ColorValue v_color = get(color, v);
                if (v_color == Color::white())
                {
                    vis.tree_out_edge(e, g);
                    put(color, v, Color::gray());
                    vis.discover_vertex(v, g);
                    Q.push(v);
                }
                else
                {
                    vis.non_tree_out_edge(e, g);
                    if (v_color == Color::gray())
                        vis.gray_target(e, g);
                    else
                        vis.black_target(e, g);
                }
            } // for out-edges

            typename GTraits::in_edge_iterator in_ei, in_ei_end;
            for (boost::tie(in_ei, in_ei_end) = in_edges(u, g);
                 in_ei != in_ei_end; ++in_ei)
            {
                Edge e = *in_ei;
                vis.examine_in_edge(e, g);
                Vertex v = source(e, g);
                ColorValue v_color = get(color, v);
                if (v_color == Color::white())
                {
                    vis.tree_in_edge(e, g);
                    put(color, v, Color::gray());
                    vis.discover_vertex(v, g);
                    Q.push(v);
                }
                else
                {
                    vis.non_tree_in_edge(e, g);
                    if (v_color == Color::gray())
                        vis.gray_source(e, g);
                    else
                        vis.black_source(e, g);
                }
            } // for in-edges

            put(color, u, Color::black());
            vis.finish_vertex(u, g);
        } // while
    }

    template < class VertexListGraph, class ColorMap, class BFSVisitor, class P,
        class T, class R >
    void neighbor_bfs_helper(VertexListGraph& g,
        typename graph_traits< VertexListGraph >::vertex_descriptor s,
        ColorMap color, BFSVisitor vis,
        const bgl_named_params< P, T, R >& params)
    {
        typedef graph_traits< VertexListGraph > Traits;
        // Buffer default
        typedef typename Traits::vertex_descriptor Vertex;
        typedef boost::queue< Vertex > queue_t;
        queue_t Q;
        // Initialization
        typedef typename property_traits< ColorMap >::value_type ColorValue;
        typedef color_traits< ColorValue > Color;
        typename boost::graph_traits< VertexListGraph >::vertex_iterator i,
            i_end;
        for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
        {
            put(color, *i, Color::white());
            vis.initialize_vertex(*i, g);
        }
        neighbor_bfs_impl(g, s,
            choose_param(get_param(params, buffer_param_t()), boost::ref(Q))
                .get(),
            vis, color);
    }

    //-------------------------------------------------------------------------
    // Choose between default color and color parameters. Using
    // function dispatching so that we don't require vertex index if
    // the color default is not being used.

    template < class ColorMap > struct neighbor_bfs_dispatch
    {
        template < class VertexListGraph, class P, class T, class R >
        static void apply(VertexListGraph& g,
            typename graph_traits< VertexListGraph >::vertex_descriptor s,
            const bgl_named_params< P, T, R >& params, ColorMap color)
        {
            neighbor_bfs_helper(g, s, color,
                choose_param(get_param(params, graph_visitor),
                    make_neighbor_bfs_visitor(null_visitor())),
                params);
        }
    };

    template <> struct neighbor_bfs_dispatch< param_not_found >
    {
        template < class VertexListGraph, class P, class T, class R >
        static void apply(VertexListGraph& g,
            typename graph_traits< VertexListGraph >::vertex_descriptor s,
            const bgl_named_params< P, T, R >& params, param_not_found)
        {
            std::vector< default_color_type > color_vec(num_vertices(g));
            null_visitor null_vis;

            neighbor_bfs_helper(g, s,
                make_iterator_property_map(color_vec.begin(),
                    choose_const_pmap(
                        get_param(params, vertex_index), g, vertex_index),
                    color_vec[0]),
                choose_param(get_param(params, graph_visitor),
                    make_neighbor_bfs_visitor(null_vis)),
                params);
        }
    };

} // namespace detail

// Named Parameter Variant
template < class VertexListGraph, class P, class T, class R >
void neighbor_breadth_first_search(const VertexListGraph& g,
    typename graph_traits< VertexListGraph >::vertex_descriptor s,
    const bgl_named_params< P, T, R >& params)
{
    // The graph is passed by *const* reference so that graph adaptors
    // (temporaries) can be passed into this function. However, the
    // graph is not really const since we may write to property maps
    // of the graph.
    VertexListGraph& ng = const_cast< VertexListGraph& >(g);
    typedef typename get_param_type< vertex_color_t,
        bgl_named_params< P, T, R > >::type C;
    detail::neighbor_bfs_dispatch< C >::apply(
        ng, s, params, get_param(params, vertex_color));
}

// This version does not initialize colors, user has to.

template < class IncidenceGraph, class P, class T, class R >
void neighbor_breadth_first_visit(IncidenceGraph& g,
    typename graph_traits< IncidenceGraph >::vertex_descriptor s,
    const bgl_named_params< P, T, R >& params)
{
    typedef graph_traits< IncidenceGraph > Traits;
    // Buffer default
    typedef boost::queue< typename Traits::vertex_descriptor > queue_t;
    queue_t Q;

    detail::neighbor_bfs_impl(g, s,
        choose_param(get_param(params, buffer_param_t()), boost::ref(Q)).get(),
        choose_param(get_param(params, graph_visitor),
            make_neighbor_bfs_visitor(null_visitor())),
        choose_pmap(get_param(params, vertex_color), g, vertex_color));
}

} // namespace boost

#endif // BOOST_GRAPH_NEIGHBOR_BREADTH_FIRST_SEARCH_HPP

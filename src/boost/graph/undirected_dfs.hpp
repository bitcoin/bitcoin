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
#ifndef BOOST_GRAPH_UNDIRECTED_DFS_HPP
#define BOOST_GRAPH_UNDIRECTED_DFS_HPP

#include <boost/graph/depth_first_search.hpp>
#include <vector>
#include <boost/concept/assert.hpp>

namespace boost
{

namespace detail
{

// Define BOOST_RECURSIVE_DFS to use older, recursive version.
// It is retained for a while in order to perform performance
// comparison.
#ifndef BOOST_RECURSIVE_DFS

    template < typename IncidenceGraph, typename DFSVisitor,
        typename VertexColorMap, typename EdgeColorMap >
    void undir_dfv_impl(const IncidenceGraph& g,
        typename graph_traits< IncidenceGraph >::vertex_descriptor u,
        DFSVisitor& vis, VertexColorMap vertex_color, EdgeColorMap edge_color)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< IncidenceGraph >));
        BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, IncidenceGraph >));
        typedef
            typename graph_traits< IncidenceGraph >::vertex_descriptor Vertex;
        typedef typename graph_traits< IncidenceGraph >::edge_descriptor Edge;
        BOOST_CONCEPT_ASSERT(
            (ReadWritePropertyMapConcept< VertexColorMap, Vertex >));
        BOOST_CONCEPT_ASSERT(
            (ReadWritePropertyMapConcept< EdgeColorMap, Edge >));
        typedef
            typename property_traits< VertexColorMap >::value_type ColorValue;
        typedef
            typename property_traits< EdgeColorMap >::value_type EColorValue;
        BOOST_CONCEPT_ASSERT((ColorValueConcept< ColorValue >));
        BOOST_CONCEPT_ASSERT((ColorValueConcept< EColorValue >));
        typedef color_traits< ColorValue > Color;
        typedef color_traits< EColorValue > EColor;
        typedef typename graph_traits< IncidenceGraph >::out_edge_iterator Iter;
        typedef std::pair< Vertex,
            std::pair< boost::optional< Edge >, std::pair< Iter, Iter > > >
            VertexInfo;

        std::vector< VertexInfo > stack;

        put(vertex_color, u, Color::gray());
        vis.discover_vertex(u, g);
        stack.push_back(std::make_pair(
            u, std::make_pair(boost::optional< Edge >(), out_edges(u, g))));
        while (!stack.empty())
        {
            VertexInfo& back = stack.back();
            u = back.first;
            boost::optional< Edge > src_e = back.second.first;
            Iter ei = back.second.second.first,
                 ei_end = back.second.second.second;
            stack.pop_back();
            while (ei != ei_end)
            {
                Vertex v = target(*ei, g);
                vis.examine_edge(*ei, g);
                ColorValue v_color = get(vertex_color, v);
                EColorValue uv_color = get(edge_color, *ei);
                put(edge_color, *ei, EColor::black());
                if (v_color == Color::white())
                {
                    vis.tree_edge(*ei, g);
                    src_e = *ei;
                    stack.push_back(std::make_pair(u,
                        std::make_pair(src_e, std::make_pair(++ei, ei_end))));
                    u = v;
                    put(vertex_color, u, Color::gray());
                    vis.discover_vertex(u, g);
                    boost::tie(ei, ei_end) = out_edges(u, g);
                }
                else if (v_color == Color::gray())
                {
                    if (uv_color == EColor::white())
                        vis.back_edge(*ei, g);
                    call_finish_edge(vis, *ei, g);
                    ++ei;
                }
                else
                { // if (v_color == Color::black())
                    call_finish_edge(vis, *ei, g);
                    ++ei;
                }
            }
            put(vertex_color, u, Color::black());
            vis.finish_vertex(u, g);
            if (src_e)
                call_finish_edge(vis, src_e.get(), g);
        }
    }

#else // BOOST_RECURSIVE_DFS

    template < typename IncidenceGraph, typename DFSVisitor,
        typename VertexColorMap, typename EdgeColorMap >
    void undir_dfv_impl(const IncidenceGraph& g,
        typename graph_traits< IncidenceGraph >::vertex_descriptor u,
        DFSVisitor& vis, // pass-by-reference here, important!
        VertexColorMap vertex_color, EdgeColorMap edge_color)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< IncidenceGraph >));
        BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, IncidenceGraph >));
        typedef
            typename graph_traits< IncidenceGraph >::vertex_descriptor Vertex;
        typedef typename graph_traits< IncidenceGraph >::edge_descriptor Edge;
        BOOST_CONCEPT_ASSERT(
            (ReadWritePropertyMapConcept< VertexColorMap, Vertex >));
        BOOST_CONCEPT_ASSERT(
            (ReadWritePropertyMapConcept< EdgeColorMap, Edge >));
        typedef
            typename property_traits< VertexColorMap >::value_type ColorValue;
        typedef
            typename property_traits< EdgeColorMap >::value_type EColorValue;
        BOOST_CONCEPT_ASSERT((ColorValueConcept< ColorValue >));
        BOOST_CONCEPT_ASSERT((ColorValueConcept< EColorValue >));
        typedef color_traits< ColorValue > Color;
        typedef color_traits< EColorValue > EColor;
        typename graph_traits< IncidenceGraph >::out_edge_iterator ei, ei_end;

        put(vertex_color, u, Color::gray());
        vis.discover_vertex(u, g);
        for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
        {
            Vertex v = target(*ei, g);
            vis.examine_edge(*ei, g);
            ColorValue v_color = get(vertex_color, v);
            EColorValue uv_color = get(edge_color, *ei);
            put(edge_color, *ei, EColor::black());
            if (v_color == Color::white())
            {
                vis.tree_edge(*ei, g);
                undir_dfv_impl(g, v, vis, vertex_color, edge_color);
            }
            else if (v_color == Color::gray() && uv_color == EColor::white())
                vis.back_edge(*ei, g);
            call_finish_edge(vis, *ei, g);
        }
        put(vertex_color, u, Color::black());
        vis.finish_vertex(u, g);
    }

#endif // ! BOOST_RECURSIVE_DFS

} // namespace detail

template < typename Graph, typename DFSVisitor, typename VertexColorMap,
    typename EdgeColorMap, typename Vertex >
void undirected_dfs(const Graph& g, DFSVisitor vis, VertexColorMap vertex_color,
    EdgeColorMap edge_color, Vertex start_vertex)
{
    BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, Graph >));
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph >));

    typedef typename property_traits< VertexColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;

    typename graph_traits< Graph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(vertex_color, *ui, Color::white());
        vis.initialize_vertex(*ui, g);
    }
    typename graph_traits< Graph >::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
        put(edge_color, *ei, Color::white());

    if (start_vertex != *vertices(g).first)
    {
        vis.start_vertex(start_vertex, g);
        detail::undir_dfv_impl(g, start_vertex, vis, vertex_color, edge_color);
    }

    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        ColorValue u_color = get(vertex_color, *ui);
        if (u_color == Color::white())
        {
            vis.start_vertex(*ui, g);
            detail::undir_dfv_impl(g, *ui, vis, vertex_color, edge_color);
        }
    }
}

template < typename Graph, typename DFSVisitor, typename VertexColorMap,
    typename EdgeColorMap >
void undirected_dfs(const Graph& g, DFSVisitor vis, VertexColorMap vertex_color,
    EdgeColorMap edge_color)
{
    undirected_dfs(g, vis, vertex_color, edge_color, *vertices(g).first);
}

namespace detail
{
    template < typename VertexColorMap > struct udfs_dispatch
    {

        template < typename Graph, typename Vertex, typename DFSVisitor,
            typename EdgeColorMap, typename P, typename T, typename R >
        static void apply(const Graph& g, DFSVisitor vis, Vertex start_vertex,
            const bgl_named_params< P, T, R >&, EdgeColorMap edge_color,
            VertexColorMap vertex_color)
        {
            undirected_dfs(g, vis, vertex_color, edge_color, start_vertex);
        }
    };

    template <> struct udfs_dispatch< param_not_found >
    {
        template < typename Graph, typename Vertex, typename DFSVisitor,
            typename EdgeColorMap, typename P, typename T, typename R >
        static void apply(const Graph& g, DFSVisitor vis, Vertex start_vertex,
            const bgl_named_params< P, T, R >& params, EdgeColorMap edge_color,
            param_not_found)
        {
            std::vector< default_color_type > color_vec(num_vertices(g));
            default_color_type c = white_color; // avoid warning about un-init
            undirected_dfs(g, vis,
                make_iterator_property_map(color_vec.begin(),
                    choose_const_pmap(
                        get_param(params, vertex_index), g, vertex_index),
                    c),
                edge_color, start_vertex);
        }
    };

} // namespace detail

// Named Parameter Variant
template < typename Graph, typename P, typename T, typename R >
void undirected_dfs(const Graph& g, const bgl_named_params< P, T, R >& params)
{
    typedef typename get_param_type< vertex_color_t,
        bgl_named_params< P, T, R > >::type C;
    detail::udfs_dispatch< C >::apply(g,
        choose_param(
            get_param(params, graph_visitor), make_dfs_visitor(null_visitor())),
        choose_param(get_param(params, root_vertex_t()), *vertices(g).first),
        params, get_param(params, edge_color), get_param(params, vertex_color));
}

template < typename IncidenceGraph, typename DFSVisitor,
    typename VertexColorMap, typename EdgeColorMap >
void undirected_depth_first_visit(const IncidenceGraph& g,
    typename graph_traits< IncidenceGraph >::vertex_descriptor u,
    DFSVisitor vis, VertexColorMap vertex_color, EdgeColorMap edge_color)
{
    detail::undir_dfv_impl(g, u, vis, vertex_color, edge_color);
}

} // namespace boost

#endif

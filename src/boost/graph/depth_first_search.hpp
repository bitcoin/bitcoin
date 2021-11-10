//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2003 Bruce Barr
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// Nonrecursive implementation of depth_first_visit_impl submitted by
// Bruce Barr, schmoost <at> yahoo.com, May/June 2003.
#ifndef BOOST_GRAPH_RECURSIVE_DFS_HPP
#define BOOST_GRAPH_RECURSIVE_DFS_HPP

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/detail/mpi_include.hpp>
#include <boost/ref.hpp>
#include <boost/implicit_cast.hpp>
#include <boost/optional.hpp>
#include <boost/parameter.hpp>
#include <boost/concept/assert.hpp>
#include <boost/tti/has_member_function.hpp>

#include <vector>
#include <utility>

namespace boost
{

template < class Visitor, class Graph > class DFSVisitorConcept
{
public:
    void constraints()
    {
        BOOST_CONCEPT_ASSERT((CopyConstructibleConcept< Visitor >));
        vis.initialize_vertex(u, g);
        vis.start_vertex(u, g);
        vis.discover_vertex(u, g);
        vis.examine_edge(e, g);
        vis.tree_edge(e, g);
        vis.back_edge(e, g);
        vis.forward_or_cross_edge(e, g);
        // vis.finish_edge(e, g); // Optional for user
        vis.finish_vertex(u, g);
    }

private:
    Visitor vis;
    Graph g;
    typename graph_traits< Graph >::vertex_descriptor u;
    typename graph_traits< Graph >::edge_descriptor e;
};

namespace detail
{

    struct nontruth2
    {
        template < class T, class T2 >
        bool operator()(const T&, const T2&) const
        {
            return false;
        }
    };

    BOOST_TTI_HAS_MEMBER_FUNCTION(finish_edge)

    template < bool IsCallable > struct do_call_finish_edge
    {
        template < typename E, typename G, typename Vis >
        static void call_finish_edge(Vis& vis, E e, const G& g)
        {
            vis.finish_edge(e, g);
        }
    };

    template <> struct do_call_finish_edge< false >
    {
        template < typename E, typename G, typename Vis >
        static void call_finish_edge(Vis&, E, const G&)
        {
        }
    };

    template < typename E, typename G, typename Vis >
    void call_finish_edge(Vis& vis, E e, const G& g)
    { // Only call if method exists
#if ((defined(__GNUC__) && (__GNUC__ > 4)               \
         || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9))) \
    || defined(__clang__)                               \
    || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1200)))
        do_call_finish_edge< has_member_function_finish_edge< Vis, void,
            boost::mpl::vector< E, const G& > >::value >::call_finish_edge(vis,
            e, g);
#else
        do_call_finish_edge< has_member_function_finish_edge< Vis,
            void >::value >::call_finish_edge(vis, e, g);
#endif
    }

// Define BOOST_RECURSIVE_DFS to use older, recursive version.
// It is retained for a while in order to perform performance
// comparison.
#ifndef BOOST_RECURSIVE_DFS

    // If the vertex u and the iterators ei and ei_end are thought of as the
    // context of the algorithm, each push and pop from the stack could
    // be thought of as a context shift.
    // Each pass through "while (ei != ei_end)" may refer to the out-edges of
    // an entirely different vertex, because the context of the algorithm
    // shifts every time a white adjacent vertex is discovered.
    // The corresponding context shift back from the adjacent vertex occurs
    // after all of its out-edges have been examined.
    //
    // See https://lists.boost.org/Archives/boost/2003/06/49265.php for FAQ.

    template < class IncidenceGraph, class DFSVisitor, class ColorMap,
        class TerminatorFunc >
    void depth_first_visit_impl(const IncidenceGraph& g,
        typename graph_traits< IncidenceGraph >::vertex_descriptor u,
        DFSVisitor& vis, ColorMap color, TerminatorFunc func = TerminatorFunc())
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< IncidenceGraph >));
        BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, IncidenceGraph >));
        typedef
            typename graph_traits< IncidenceGraph >::vertex_descriptor Vertex;
        typedef typename graph_traits< IncidenceGraph >::edge_descriptor Edge;
        BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< ColorMap, Vertex >));
        typedef typename property_traits< ColorMap >::value_type ColorValue;
        BOOST_CONCEPT_ASSERT((ColorValueConcept< ColorValue >));
        typedef color_traits< ColorValue > Color;
        typedef typename graph_traits< IncidenceGraph >::out_edge_iterator Iter;
        typedef std::pair< Vertex,
            std::pair< boost::optional< Edge >, std::pair< Iter, Iter > > >
            VertexInfo;

        boost::optional< Edge > src_e;
        Iter ei, ei_end;
        std::vector< VertexInfo > stack;

        // Possible optimization for vector
        // stack.reserve(num_vertices(g));

        put(color, u, Color::gray());
        vis.discover_vertex(u, g);
        boost::tie(ei, ei_end) = out_edges(u, g);
        if (func(u, g))
        {
            // If this vertex terminates the search, we push empty range
            stack.push_back(std::make_pair(u,
                std::make_pair(boost::optional< Edge >(),
                    std::make_pair(ei_end, ei_end))));
        }
        else
        {
            stack.push_back(std::make_pair(u,
                std::make_pair(
                    boost::optional< Edge >(), std::make_pair(ei, ei_end))));
        }
        while (!stack.empty())
        {
            VertexInfo& back = stack.back();
            u = back.first;
            src_e = back.second.first;
            boost::tie(ei, ei_end) = back.second.second;
            stack.pop_back();
            // finish_edge has to be called here, not after the
            // loop. Think of the pop as the return from a recursive call.
            if (src_e)
            {
                call_finish_edge(vis, src_e.get(), g);
            }
            while (ei != ei_end)
            {
                Vertex v = target(*ei, g);
                vis.examine_edge(*ei, g);
                ColorValue v_color = get(color, v);
                if (v_color == Color::white())
                {
                    vis.tree_edge(*ei, g);
                    src_e = *ei;
                    stack.push_back(std::make_pair(u,
                        std::make_pair(src_e, std::make_pair(++ei, ei_end))));
                    u = v;
                    put(color, u, Color::gray());
                    vis.discover_vertex(u, g);
                    boost::tie(ei, ei_end) = out_edges(u, g);
                    if (func(u, g))
                    {
                        ei = ei_end;
                    }
                }
                else
                {
                    if (v_color == Color::gray())
                    {
                        vis.back_edge(*ei, g);
                    }
                    else
                    {
                        vis.forward_or_cross_edge(*ei, g);
                    }
                    call_finish_edge(vis, *ei, g);
                    ++ei;
                }
            }
            put(color, u, Color::black());
            vis.finish_vertex(u, g);
        }
    }

#else // BOOST_RECURSIVE_DFS is defined

    template < class IncidenceGraph, class DFSVisitor, class ColorMap,
        class TerminatorFunc >
    void depth_first_visit_impl(const IncidenceGraph& g,
        typename graph_traits< IncidenceGraph >::vertex_descriptor u,
        DFSVisitor& vis, // pass-by-reference here, important!
        ColorMap color, TerminatorFunc func)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< IncidenceGraph >));
        BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, IncidenceGraph >));
        typedef
            typename graph_traits< IncidenceGraph >::vertex_descriptor Vertex;
        BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< ColorMap, Vertex >));
        typedef typename property_traits< ColorMap >::value_type ColorValue;
        BOOST_CONCEPT_ASSERT((ColorValueConcept< ColorValue >));
        typedef color_traits< ColorValue > Color;
        typename graph_traits< IncidenceGraph >::out_edge_iterator ei, ei_end;

        put(color, u, Color::gray());
        vis.discover_vertex(u, g);

        if (!func(u, g))
            for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
            {
                Vertex v = target(*ei, g);
                vis.examine_edge(*ei, g);
                ColorValue v_color = get(color, v);
                if (v_color == Color::white())
                {
                    vis.tree_edge(*ei, g);
                    depth_first_visit_impl(g, v, vis, color, func);
                }
                else if (v_color == Color::gray())
                    vis.back_edge(*ei, g);
                else
                    vis.forward_or_cross_edge(*ei, g);
                call_finish_edge(vis, *ei, g);
            }
        put(color, u, Color::black());
        vis.finish_vertex(u, g);
    }

#endif

} // namespace detail

template < class VertexListGraph, class DFSVisitor, class ColorMap >
void depth_first_search(const VertexListGraph& g, DFSVisitor vis,
    ColorMap color,
    typename graph_traits< VertexListGraph >::vertex_descriptor start_vertex)
{
    typedef typename graph_traits< VertexListGraph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT((DFSVisitorConcept< DFSVisitor, VertexListGraph >));
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;

    typename graph_traits< VertexListGraph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        Vertex u = implicit_cast< Vertex >(*ui);
        put(color, u, Color::white());
        vis.initialize_vertex(u, g);
    }

    if (start_vertex != detail::get_default_starting_vertex(g))
    {
        vis.start_vertex(start_vertex, g);
        detail::depth_first_visit_impl(
            g, start_vertex, vis, color, detail::nontruth2());
    }

    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        Vertex u = implicit_cast< Vertex >(*ui);
        ColorValue u_color = get(color, u);
        if (u_color == Color::white())
        {
            vis.start_vertex(u, g);
            detail::depth_first_visit_impl(
                g, u, vis, color, detail::nontruth2());
        }
    }
}

template < class VertexListGraph, class DFSVisitor, class ColorMap >
void depth_first_search(
    const VertexListGraph& g, DFSVisitor vis, ColorMap color)
{
    typedef typename boost::graph_traits< VertexListGraph >::vertex_iterator vi;
    std::pair< vi, vi > verts = vertices(g);
    if (verts.first == verts.second)
        return;

    depth_first_search(g, vis, color, detail::get_default_starting_vertex(g));
}

template < class Visitors = null_visitor > class dfs_visitor
{
public:
    dfs_visitor() {}
    dfs_visitor(Visitors vis) : m_vis(vis) {}

    template < class Vertex, class Graph >
    void initialize_vertex(Vertex u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_initialize_vertex());
    }
    template < class Vertex, class Graph >
    void start_vertex(Vertex u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_start_vertex());
    }
    template < class Vertex, class Graph >
    void discover_vertex(Vertex u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_discover_vertex());
    }
    template < class Edge, class Graph >
    void examine_edge(Edge u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_examine_edge());
    }
    template < class Edge, class Graph > void tree_edge(Edge u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_tree_edge());
    }
    template < class Edge, class Graph > void back_edge(Edge u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_back_edge());
    }
    template < class Edge, class Graph >
    void forward_or_cross_edge(Edge u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_forward_or_cross_edge());
    }
    template < class Edge, class Graph >
    void finish_edge(Edge u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_finish_edge());
    }
    template < class Vertex, class Graph >
    void finish_vertex(Vertex u, const Graph& g)
    {
        invoke_visitors(m_vis, u, g, ::boost::on_finish_vertex());
    }

    BOOST_GRAPH_EVENT_STUB(on_initialize_vertex, dfs)
    BOOST_GRAPH_EVENT_STUB(on_start_vertex, dfs)
    BOOST_GRAPH_EVENT_STUB(on_discover_vertex, dfs)
    BOOST_GRAPH_EVENT_STUB(on_examine_edge, dfs)
    BOOST_GRAPH_EVENT_STUB(on_tree_edge, dfs)
    BOOST_GRAPH_EVENT_STUB(on_back_edge, dfs)
    BOOST_GRAPH_EVENT_STUB(on_forward_or_cross_edge, dfs)
    BOOST_GRAPH_EVENT_STUB(on_finish_edge, dfs)
    BOOST_GRAPH_EVENT_STUB(on_finish_vertex, dfs)

protected:
    Visitors m_vis;
};
template < class Visitors >
dfs_visitor< Visitors > make_dfs_visitor(Visitors vis)
{
    return dfs_visitor< Visitors >(vis);
}
typedef dfs_visitor<> default_dfs_visitor;

// Boost.Parameter named parameter variant
namespace graph
{
    namespace detail
    {
        template < typename Graph > struct depth_first_search_impl
        {
            typedef void result_type;
            template < typename ArgPack >
            void operator()(const Graph& g, const ArgPack& arg_pack) const
            {
                using namespace boost::graph::keywords;
                boost::depth_first_search(g,
                    arg_pack[_visitor | make_dfs_visitor(null_visitor())],
                    boost::detail::make_color_map_from_arg_pack(g, arg_pack),
                    arg_pack[_root_vertex
                        || boost::detail::get_default_starting_vertex_t<
                            Graph >(g)]);
            }
        };
    }
    BOOST_GRAPH_MAKE_FORWARDING_FUNCTION(depth_first_search, 1, 4)
}

BOOST_GRAPH_MAKE_OLD_STYLE_PARAMETER_FUNCTION(depth_first_search, 1)

template < class IncidenceGraph, class DFSVisitor, class ColorMap >
void depth_first_visit(const IncidenceGraph& g,
    typename graph_traits< IncidenceGraph >::vertex_descriptor u,
    DFSVisitor vis, ColorMap color)
{
    vis.start_vertex(u, g);
    detail::depth_first_visit_impl(g, u, vis, color, detail::nontruth2());
}

template < class IncidenceGraph, class DFSVisitor, class ColorMap,
    class TerminatorFunc >
void depth_first_visit(const IncidenceGraph& g,
    typename graph_traits< IncidenceGraph >::vertex_descriptor u,
    DFSVisitor vis, ColorMap color, TerminatorFunc func = TerminatorFunc())
{
    vis.start_vertex(u, g);
    detail::depth_first_visit_impl(g, u, vis, color, func);
}
} // namespace boost

#include BOOST_GRAPH_MPI_INCLUDE(<boost/graph/distributed/depth_first_search.hpp>)

#endif

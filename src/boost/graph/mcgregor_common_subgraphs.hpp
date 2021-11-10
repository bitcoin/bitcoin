//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_MCGREGOR_COMMON_SUBGRAPHS_HPP
#define BOOST_GRAPH_MCGREGOR_COMMON_SUBGRAPHS_HPP

#include <algorithm>
#include <vector>
#include <stack>

#include <boost/make_shared.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/shared_array_property_map.hpp>

namespace boost
{

namespace detail
{

    // Traits associated with common subgraphs, used mainly to keep a
    // consistent type for the correspondence maps.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond >
    struct mcgregor_common_subgraph_traits
    {
        typedef typename graph_traits< GraphFirst >::vertex_descriptor
            vertex_first_type;
        typedef typename graph_traits< GraphSecond >::vertex_descriptor
            vertex_second_type;

        typedef shared_array_property_map< vertex_second_type,
            VertexIndexMapFirst >
            correspondence_map_first_to_second_type;

        typedef shared_array_property_map< vertex_first_type,
            VertexIndexMapSecond >
            correspondence_map_second_to_first_type;
    };

} // namespace detail

// ==========================================================================

// Binary function object that returns true if the values for item1
// in property_map1 and item2 in property_map2 are equivalent.
template < typename PropertyMapFirst, typename PropertyMapSecond >
struct property_map_equivalent
{

    property_map_equivalent(const PropertyMapFirst property_map1,
        const PropertyMapSecond property_map2)
    : m_property_map1(property_map1), m_property_map2(property_map2)
    {
    }

    template < typename ItemFirst, typename ItemSecond >
    bool operator()(const ItemFirst item1, const ItemSecond item2)
    {
        return (get(m_property_map1, item1) == get(m_property_map2, item2));
    }

private:
    const PropertyMapFirst m_property_map1;
    const PropertyMapSecond m_property_map2;
};

// Returns a property_map_equivalent object that compares the values
// of property_map1 and property_map2.
template < typename PropertyMapFirst, typename PropertyMapSecond >
property_map_equivalent< PropertyMapFirst, PropertyMapSecond >
make_property_map_equivalent(
    const PropertyMapFirst property_map1, const PropertyMapSecond property_map2)
{

    return (property_map_equivalent< PropertyMapFirst, PropertyMapSecond >(
        property_map1, property_map2));
}

// Binary function object that always returns true.  Used when
// vertices or edges are always equivalent (i.e. have no labels).
struct always_equivalent
{

    template < typename ItemFirst, typename ItemSecond >
    bool operator()(const ItemFirst&, const ItemSecond&)
    {
        return (true);
    }
};

// ==========================================================================

namespace detail
{

    // Return true if new_vertex1 and new_vertex2 can extend the
    // subgraph represented by correspondence_map_1_to_2 and
    // correspondence_map_2_to_1.  The vertices_equivalent and
    // edges_equivalent predicates are used to test vertex and edge
    // equivalency between the two graphs.
    template < typename GraphFirst, typename GraphSecond,
        typename CorrespondenceMapFirstToSecond,
        typename CorrespondenceMapSecondToFirst,
        typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate >
    bool can_extend_graph(const GraphFirst& graph1, const GraphSecond& graph2,
        CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
        CorrespondenceMapSecondToFirst /*correspondence_map_2_to_1*/,
        typename graph_traits< GraphFirst >::vertices_size_type subgraph_size,
        typename graph_traits< GraphFirst >::vertex_descriptor new_vertex1,
        typename graph_traits< GraphSecond >::vertex_descriptor new_vertex2,
        EdgeEquivalencePredicate edges_equivalent,
        VertexEquivalencePredicate vertices_equivalent,
        bool only_connected_subgraphs)
    {
        typedef typename graph_traits< GraphSecond >::vertex_descriptor
            VertexSecond;

        typedef typename graph_traits< GraphFirst >::edge_descriptor EdgeFirst;
        typedef
            typename graph_traits< GraphSecond >::edge_descriptor EdgeSecond;

        // Check vertex equality
        if (!vertices_equivalent(new_vertex1, new_vertex2))
        {
            return (false);
        }

        // Vertices match and graph is empty, so we can extend the subgraph
        if (subgraph_size == 0)
        {
            return (true);
        }

        bool has_one_edge = false;

        // Verify edges with existing sub-graph
        BGL_FORALL_VERTICES_T(existing_vertex1, graph1, GraphFirst)
        {

            VertexSecond existing_vertex2
                = get(correspondence_map_1_to_2, existing_vertex1);

            // Skip unassociated vertices
            if (existing_vertex2 == graph_traits< GraphSecond >::null_vertex())
            {
                continue;
            }

            // NOTE: This will not work with parallel edges, since the
            // first matching edge is always chosen.
            EdgeFirst edge_to_new1, edge_from_new1;
            bool edge_to_new_exists1 = false, edge_from_new_exists1 = false;

            EdgeSecond edge_to_new2, edge_from_new2;
            bool edge_to_new_exists2 = false, edge_from_new_exists2 = false;

            // Search for edge from existing to new vertex (graph1)
            BGL_FORALL_OUTEDGES_T(existing_vertex1, edge1, graph1, GraphFirst)
            {
                if (target(edge1, graph1) == new_vertex1)
                {
                    edge_to_new1 = edge1;
                    edge_to_new_exists1 = true;
                    break;
                }
            }

            // Search for edge from existing to new vertex (graph2)
            BGL_FORALL_OUTEDGES_T(existing_vertex2, edge2, graph2, GraphSecond)
            {
                if (target(edge2, graph2) == new_vertex2)
                {
                    edge_to_new2 = edge2;
                    edge_to_new_exists2 = true;
                    break;
                }
            }

            // Make sure edges from existing to new vertices are equivalent
            if ((edge_to_new_exists1 != edge_to_new_exists2)
                || ((edge_to_new_exists1 && edge_to_new_exists2)
                    && !edges_equivalent(edge_to_new1, edge_to_new2)))
            {

                return (false);
            }

            bool is_undirected1 = is_undirected(graph1),
                 is_undirected2 = is_undirected(graph2);

            if (is_undirected1 && is_undirected2)
            {

                // Edge in both graphs exists and both graphs are undirected
                if (edge_to_new_exists1 && edge_to_new_exists2)
                {
                    has_one_edge = true;
                }

                continue;
            }
            else
            {

                if (!is_undirected1)
                {

                    // Search for edge from new to existing vertex (graph1)
                    BGL_FORALL_OUTEDGES_T(
                        new_vertex1, edge1, graph1, GraphFirst)
                    {
                        if (target(edge1, graph1) == existing_vertex1)
                        {
                            edge_from_new1 = edge1;
                            edge_from_new_exists1 = true;
                            break;
                        }
                    }
                }

                if (!is_undirected2)
                {

                    // Search for edge from new to existing vertex (graph2)
                    BGL_FORALL_OUTEDGES_T(
                        new_vertex2, edge2, graph2, GraphSecond)
                    {
                        if (target(edge2, graph2) == existing_vertex2)
                        {
                            edge_from_new2 = edge2;
                            edge_from_new_exists2 = true;
                            break;
                        }
                    }
                }

                // Make sure edges from new to existing vertices are equivalent
                if ((edge_from_new_exists1 != edge_from_new_exists2)
                    || ((edge_from_new_exists1 && edge_from_new_exists2)
                        && !edges_equivalent(edge_from_new1, edge_from_new2)))
                {

                    return (false);
                }

                if ((edge_from_new_exists1 && edge_from_new_exists2)
                    || (edge_to_new_exists1 && edge_to_new_exists2))
                {
                    has_one_edge = true;
                }

            } // else

        } // BGL_FORALL_VERTICES_T

        // Make sure new vertices are connected to the existing subgraph
        if (only_connected_subgraphs && !has_one_edge)
        {
            return (false);
        }

        return (true);
    }

    // Recursive method that does a depth-first search in the space of
    // potential subgraphs.  At each level, every new vertex pair from
    // both graphs is tested to see if it can extend the current
    // subgraph.  If so, the subgraph is output to subgraph_callback
    // in the form of two correspondence maps (one for each graph).
    // Returning false from subgraph_callback will terminate the
    // search.  Function returns true if the entire search space was
    // explored.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond,
        typename CorrespondenceMapFirstToSecond,
        typename CorrespondenceMapSecondToFirst, typename VertexStackFirst,
        typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
        typename SubGraphInternalCallback >
    bool mcgregor_common_subgraphs_internal(const GraphFirst& graph1,
        const GraphSecond& graph2, const VertexIndexMapFirst& vindex_map1,
        const VertexIndexMapSecond& vindex_map2,
        CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
        CorrespondenceMapSecondToFirst correspondence_map_2_to_1,
        VertexStackFirst& vertex_stack1,
        EdgeEquivalencePredicate edges_equivalent,
        VertexEquivalencePredicate vertices_equivalent,
        bool only_connected_subgraphs,
        SubGraphInternalCallback subgraph_callback)
    {
        typedef
            typename graph_traits< GraphFirst >::vertex_descriptor VertexFirst;
        typedef typename graph_traits< GraphSecond >::vertex_descriptor
            VertexSecond;
        typedef typename graph_traits< GraphFirst >::vertices_size_type
            VertexSizeFirst;

        // Get iterators for vertices from both graphs
        typename graph_traits< GraphFirst >::vertex_iterator vertex1_iter,
            vertex1_end;

        typename graph_traits< GraphSecond >::vertex_iterator vertex2_begin,
            vertex2_end, vertex2_iter;

        boost::tie(vertex1_iter, vertex1_end) = vertices(graph1);
        boost::tie(vertex2_begin, vertex2_end) = vertices(graph2);
        vertex2_iter = vertex2_begin;

        // Iterate until all vertices have been visited
        BGL_FORALL_VERTICES_T(new_vertex1, graph1, GraphFirst)
        {

            VertexSecond existing_vertex2
                = get(correspondence_map_1_to_2, new_vertex1);

            // Skip already matched vertices in first graph
            if (existing_vertex2 != graph_traits< GraphSecond >::null_vertex())
            {
                continue;
            }

            BGL_FORALL_VERTICES_T(new_vertex2, graph2, GraphSecond)
            {

                VertexFirst existing_vertex1
                    = get(correspondence_map_2_to_1, new_vertex2);

                // Skip already matched vertices in second graph
                if (existing_vertex1
                    != graph_traits< GraphFirst >::null_vertex())
                {
                    continue;
                }

                // Check if current sub-graph can be extended with the matched
                // vertex pair
                if (can_extend_graph(graph1, graph2, correspondence_map_1_to_2,
                        correspondence_map_2_to_1,
                        (VertexSizeFirst)vertex_stack1.size(), new_vertex1,
                        new_vertex2, edges_equivalent, vertices_equivalent,
                        only_connected_subgraphs))
                {

                    // Keep track of old graph size for restoring later
                    VertexSizeFirst old_graph_size
                        = (VertexSizeFirst)vertex_stack1.size(),
                        new_graph_size = old_graph_size + 1;

                    // Extend subgraph
                    put(correspondence_map_1_to_2, new_vertex1, new_vertex2);
                    put(correspondence_map_2_to_1, new_vertex2, new_vertex1);
                    vertex_stack1.push(new_vertex1);

                    // Returning false from the callback will cancel iteration
                    if (!subgraph_callback(correspondence_map_1_to_2,
                            correspondence_map_2_to_1, new_graph_size))
                    {
                        return (false);
                    }

                    // Depth-first search into the state space of possible
                    // sub-graphs
                    bool continue_iteration
                        = mcgregor_common_subgraphs_internal(graph1, graph2,
                            vindex_map1, vindex_map2, correspondence_map_1_to_2,
                            correspondence_map_2_to_1, vertex_stack1,
                            edges_equivalent, vertices_equivalent,
                            only_connected_subgraphs, subgraph_callback);

                    if (!continue_iteration)
                    {
                        return (false);
                    }

                    // Restore previous state
                    if (vertex_stack1.size() > old_graph_size)
                    {

                        VertexFirst stack_vertex1 = vertex_stack1.top();
                        VertexSecond stack_vertex2
                            = get(correspondence_map_1_to_2, stack_vertex1);

                        // Contract subgraph
                        put(correspondence_map_1_to_2, stack_vertex1,
                            graph_traits< GraphSecond >::null_vertex());

                        put(correspondence_map_2_to_1, stack_vertex2,
                            graph_traits< GraphFirst >::null_vertex());

                        vertex_stack1.pop();
                    }

                } // if can_extend_graph

            } // BGL_FORALL_VERTICES_T (graph2)

        } // BGL_FORALL_VERTICES_T (graph1)

        return (true);
    }

    // Internal method that initializes blank correspondence maps and
    // a vertex stack for use in mcgregor_common_subgraphs_internal.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond,
        typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
        typename SubGraphInternalCallback >
    inline void mcgregor_common_subgraphs_internal_init(
        const GraphFirst& graph1, const GraphSecond& graph2,
        const VertexIndexMapFirst vindex_map1,
        const VertexIndexMapSecond vindex_map2,
        EdgeEquivalencePredicate edges_equivalent,
        VertexEquivalencePredicate vertices_equivalent,
        bool only_connected_subgraphs,
        SubGraphInternalCallback subgraph_callback)
    {
        typedef mcgregor_common_subgraph_traits< GraphFirst, GraphSecond,
            VertexIndexMapFirst, VertexIndexMapSecond >
            SubGraphTraits;

        typename SubGraphTraits::correspondence_map_first_to_second_type
            correspondence_map_1_to_2(num_vertices(graph1), vindex_map1);

        BGL_FORALL_VERTICES_T(vertex1, graph1, GraphFirst)
        {
            put(correspondence_map_1_to_2, vertex1,
                graph_traits< GraphSecond >::null_vertex());
        }

        typename SubGraphTraits::correspondence_map_second_to_first_type
            correspondence_map_2_to_1(num_vertices(graph2), vindex_map2);

        BGL_FORALL_VERTICES_T(vertex2, graph2, GraphSecond)
        {
            put(correspondence_map_2_to_1, vertex2,
                graph_traits< GraphFirst >::null_vertex());
        }

        typedef
            typename graph_traits< GraphFirst >::vertex_descriptor VertexFirst;

        std::stack< VertexFirst > vertex_stack1;

        mcgregor_common_subgraphs_internal(graph1, graph2, vindex_map1,
            vindex_map2, correspondence_map_1_to_2, correspondence_map_2_to_1,
            vertex_stack1, edges_equivalent, vertices_equivalent,
            only_connected_subgraphs, subgraph_callback);
    }

} // namespace detail

// ==========================================================================

// Enumerates all common subgraphs present in graph1 and graph2.
// Continues until the search space has been fully explored or false
// is returned from user_callback.
template < typename GraphFirst, typename GraphSecond,
    typename VertexIndexMapFirst, typename VertexIndexMapSecond,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphCallback >
void mcgregor_common_subgraphs(const GraphFirst& graph1,
    const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
    const VertexIndexMapSecond vindex_map2,
    EdgeEquivalencePredicate edges_equivalent,
    VertexEquivalencePredicate vertices_equivalent,
    bool only_connected_subgraphs, SubGraphCallback user_callback)
{

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2, vindex_map1,
        vindex_map2, edges_equivalent, vertices_equivalent,
        only_connected_subgraphs, user_callback);
}

// Variant of mcgregor_common_subgraphs with all default parameters
template < typename GraphFirst, typename GraphSecond,
    typename SubGraphCallback >
void mcgregor_common_subgraphs(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback)
{

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2,
        get(vertex_index, graph1), get(vertex_index, graph2),
        always_equivalent(), always_equivalent(), only_connected_subgraphs,
        user_callback);
}

// Named parameter variant of mcgregor_common_subgraphs
template < typename GraphFirst, typename GraphSecond, typename SubGraphCallback,
    typename Param, typename Tag, typename Rest >
void mcgregor_common_subgraphs(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback,
    const bgl_named_params< Param, Tag, Rest >& params)
{

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2,
        choose_const_pmap(
            get_param(params, vertex_index1), graph1, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph2, vertex_index),
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()),
        only_connected_subgraphs, user_callback);
}

// ==========================================================================

namespace detail
{

    // Binary function object that intercepts subgraphs from
    // mcgregor_common_subgraphs_internal and maintains a cache of
    // unique subgraphs.  The user callback is invoked for each unique
    // subgraph.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond,
        typename SubGraphCallback >
    struct unique_subgraph_interceptor
    {

        typedef typename graph_traits< GraphFirst >::vertices_size_type
            VertexSizeFirst;

        typedef mcgregor_common_subgraph_traits< GraphFirst, GraphSecond,
            VertexIndexMapFirst, VertexIndexMapSecond >
            SubGraphTraits;

        typedef typename SubGraphTraits::correspondence_map_first_to_second_type
            CachedCorrespondenceMapFirstToSecond;

        typedef typename SubGraphTraits::correspondence_map_second_to_first_type
            CachedCorrespondenceMapSecondToFirst;

        typedef std::pair< VertexSizeFirst,
            std::pair< CachedCorrespondenceMapFirstToSecond,
                CachedCorrespondenceMapSecondToFirst > >
            SubGraph;

        typedef std::vector< SubGraph > SubGraphList;

        unique_subgraph_interceptor(const GraphFirst& graph1,
            const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
            const VertexIndexMapSecond vindex_map2,
            SubGraphCallback user_callback)
        : m_graph1(graph1)
        , m_graph2(graph2)
        , m_vindex_map1(vindex_map1)
        , m_vindex_map2(vindex_map2)
        , m_subgraphs(make_shared< SubGraphList >())
        , m_user_callback(user_callback)
        {
        }

        template < typename CorrespondenceMapFirstToSecond,
            typename CorrespondenceMapSecondToFirst >
        bool operator()(
            CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
            CorrespondenceMapSecondToFirst correspondence_map_2_to_1,
            VertexSizeFirst subgraph_size)
        {

            for (typename SubGraphList::const_iterator subgraph_iter
                 = m_subgraphs->begin();
                 subgraph_iter != m_subgraphs->end(); ++subgraph_iter)
            {

                SubGraph subgraph_cached = *subgraph_iter;

                // Compare subgraph sizes
                if (subgraph_size != subgraph_cached.first)
                {
                    continue;
                }

                if (!are_property_maps_different(correspondence_map_1_to_2,
                        subgraph_cached.second.first, m_graph1))
                {

                    // New subgraph is a duplicate
                    return (true);
                }
            }

            // Subgraph is unique, so make a cached copy
            CachedCorrespondenceMapFirstToSecond new_subgraph_1_to_2
                = CachedCorrespondenceMapFirstToSecond(
                    num_vertices(m_graph1), m_vindex_map1);

            CachedCorrespondenceMapSecondToFirst new_subgraph_2_to_1
                = CorrespondenceMapSecondToFirst(
                    num_vertices(m_graph2), m_vindex_map2);

            BGL_FORALL_VERTICES_T(vertex1, m_graph1, GraphFirst)
            {
                put(new_subgraph_1_to_2, vertex1,
                    get(correspondence_map_1_to_2, vertex1));
            }

            BGL_FORALL_VERTICES_T(vertex2, m_graph2, GraphFirst)
            {
                put(new_subgraph_2_to_1, vertex2,
                    get(correspondence_map_2_to_1, vertex2));
            }

            m_subgraphs->push_back(std::make_pair(subgraph_size,
                std::make_pair(new_subgraph_1_to_2, new_subgraph_2_to_1)));

            return (m_user_callback(correspondence_map_1_to_2,
                correspondence_map_2_to_1, subgraph_size));
        }

    private:
        const GraphFirst& m_graph1;
        const GraphFirst& m_graph2;
        const VertexIndexMapFirst m_vindex_map1;
        const VertexIndexMapSecond m_vindex_map2;
        shared_ptr< SubGraphList > m_subgraphs;
        SubGraphCallback m_user_callback;
    };

} // namespace detail

// Enumerates all unique common subgraphs between graph1 and graph2.
// The user callback is invoked for each unique subgraph as they are
// discovered.
template < typename GraphFirst, typename GraphSecond,
    typename VertexIndexMapFirst, typename VertexIndexMapSecond,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
    const VertexIndexMapSecond vindex_map2,
    EdgeEquivalencePredicate edges_equivalent,
    VertexEquivalencePredicate vertices_equivalent,
    bool only_connected_subgraphs, SubGraphCallback user_callback)
{
    detail::unique_subgraph_interceptor< GraphFirst, GraphSecond,
        VertexIndexMapFirst, VertexIndexMapSecond, SubGraphCallback >
        unique_callback(
            graph1, graph2, vindex_map1, vindex_map2, user_callback);

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2, vindex_map1,
        vindex_map2, edges_equivalent, vertices_equivalent,
        only_connected_subgraphs, unique_callback);
}

// Variant of mcgregor_common_subgraphs_unique with all default
// parameters.
template < typename GraphFirst, typename GraphSecond,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback)
{
    mcgregor_common_subgraphs_unique(graph1, graph2, get(vertex_index, graph1),
        get(vertex_index, graph2), always_equivalent(), always_equivalent(),
        only_connected_subgraphs, user_callback);
}

// Named parameter variant of mcgregor_common_subgraphs_unique
template < typename GraphFirst, typename GraphSecond, typename SubGraphCallback,
    typename Param, typename Tag, typename Rest >
void mcgregor_common_subgraphs_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    mcgregor_common_subgraphs_unique(graph1, graph2,
        choose_const_pmap(
            get_param(params, vertex_index1), graph1, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph2, vertex_index),
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()),
        only_connected_subgraphs, user_callback);
}

// ==========================================================================

namespace detail
{

    // Binary function object that intercepts subgraphs from
    // mcgregor_common_subgraphs_internal and maintains a cache of the
    // largest subgraphs.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond,
        typename SubGraphCallback >
    struct maximum_subgraph_interceptor
    {

        typedef typename graph_traits< GraphFirst >::vertices_size_type
            VertexSizeFirst;

        typedef mcgregor_common_subgraph_traits< GraphFirst, GraphSecond,
            VertexIndexMapFirst, VertexIndexMapSecond >
            SubGraphTraits;

        typedef typename SubGraphTraits::correspondence_map_first_to_second_type
            CachedCorrespondenceMapFirstToSecond;

        typedef typename SubGraphTraits::correspondence_map_second_to_first_type
            CachedCorrespondenceMapSecondToFirst;

        typedef std::pair< VertexSizeFirst,
            std::pair< CachedCorrespondenceMapFirstToSecond,
                CachedCorrespondenceMapSecondToFirst > >
            SubGraph;

        typedef std::vector< SubGraph > SubGraphList;

        maximum_subgraph_interceptor(const GraphFirst& graph1,
            const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
            const VertexIndexMapSecond vindex_map2,
            SubGraphCallback user_callback)
        : m_graph1(graph1)
        , m_graph2(graph2)
        , m_vindex_map1(vindex_map1)
        , m_vindex_map2(vindex_map2)
        , m_subgraphs(make_shared< SubGraphList >())
        , m_largest_size_so_far(make_shared< VertexSizeFirst >(0))
        , m_user_callback(user_callback)
        {
        }

        template < typename CorrespondenceMapFirstToSecond,
            typename CorrespondenceMapSecondToFirst >
        bool operator()(
            CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
            CorrespondenceMapSecondToFirst correspondence_map_2_to_1,
            VertexSizeFirst subgraph_size)
        {

            if (subgraph_size > *m_largest_size_so_far)
            {
                m_subgraphs->clear();
                *m_largest_size_so_far = subgraph_size;
            }

            if (subgraph_size == *m_largest_size_so_far)
            {

                // Make a cached copy
                CachedCorrespondenceMapFirstToSecond new_subgraph_1_to_2
                    = CachedCorrespondenceMapFirstToSecond(
                        num_vertices(m_graph1), m_vindex_map1);

                CachedCorrespondenceMapSecondToFirst new_subgraph_2_to_1
                    = CachedCorrespondenceMapSecondToFirst(
                        num_vertices(m_graph2), m_vindex_map2);

                BGL_FORALL_VERTICES_T(vertex1, m_graph1, GraphFirst)
                {
                    put(new_subgraph_1_to_2, vertex1,
                        get(correspondence_map_1_to_2, vertex1));
                }

                BGL_FORALL_VERTICES_T(vertex2, m_graph2, GraphFirst)
                {
                    put(new_subgraph_2_to_1, vertex2,
                        get(correspondence_map_2_to_1, vertex2));
                }

                m_subgraphs->push_back(std::make_pair(subgraph_size,
                    std::make_pair(new_subgraph_1_to_2, new_subgraph_2_to_1)));
            }

            return (true);
        }

        void output_subgraphs()
        {
            for (typename SubGraphList::const_iterator subgraph_iter
                 = m_subgraphs->begin();
                 subgraph_iter != m_subgraphs->end(); ++subgraph_iter)
            {

                SubGraph subgraph_cached = *subgraph_iter;
                m_user_callback(subgraph_cached.second.first,
                    subgraph_cached.second.second, subgraph_cached.first);
            }
        }

    private:
        const GraphFirst& m_graph1;
        const GraphFirst& m_graph2;
        const VertexIndexMapFirst m_vindex_map1;
        const VertexIndexMapSecond m_vindex_map2;
        shared_ptr< SubGraphList > m_subgraphs;
        shared_ptr< VertexSizeFirst > m_largest_size_so_far;
        SubGraphCallback m_user_callback;
    };

} // namespace detail

// Enumerates the largest common subgraphs found between graph1
// and graph2.  Note that the ENTIRE search space is explored before
// user_callback is actually invoked.
template < typename GraphFirst, typename GraphSecond,
    typename VertexIndexMapFirst, typename VertexIndexMapSecond,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_maximum(const GraphFirst& graph1,
    const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
    const VertexIndexMapSecond vindex_map2,
    EdgeEquivalencePredicate edges_equivalent,
    VertexEquivalencePredicate vertices_equivalent,
    bool only_connected_subgraphs, SubGraphCallback user_callback)
{
    detail::maximum_subgraph_interceptor< GraphFirst, GraphSecond,
        VertexIndexMapFirst, VertexIndexMapSecond, SubGraphCallback >
        max_interceptor(
            graph1, graph2, vindex_map1, vindex_map2, user_callback);

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2, vindex_map1,
        vindex_map2, edges_equivalent, vertices_equivalent,
        only_connected_subgraphs, max_interceptor);

    // Only output the largest subgraphs
    max_interceptor.output_subgraphs();
}

// Variant of mcgregor_common_subgraphs_maximum with all default
// parameters.
template < typename GraphFirst, typename GraphSecond,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_maximum(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback)
{
    mcgregor_common_subgraphs_maximum(graph1, graph2, get(vertex_index, graph1),
        get(vertex_index, graph2), always_equivalent(), always_equivalent(),
        only_connected_subgraphs, user_callback);
}

// Named parameter variant of mcgregor_common_subgraphs_maximum
template < typename GraphFirst, typename GraphSecond, typename SubGraphCallback,
    typename Param, typename Tag, typename Rest >
void mcgregor_common_subgraphs_maximum(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    mcgregor_common_subgraphs_maximum(graph1, graph2,
        choose_const_pmap(
            get_param(params, vertex_index1), graph1, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph2, vertex_index),
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()),
        only_connected_subgraphs, user_callback);
}

// ==========================================================================

namespace detail
{

    // Binary function object that intercepts subgraphs from
    // mcgregor_common_subgraphs_internal and maintains a cache of the
    // largest, unique subgraphs.
    template < typename GraphFirst, typename GraphSecond,
        typename VertexIndexMapFirst, typename VertexIndexMapSecond,
        typename SubGraphCallback >
    struct unique_maximum_subgraph_interceptor
    {

        typedef typename graph_traits< GraphFirst >::vertices_size_type
            VertexSizeFirst;

        typedef mcgregor_common_subgraph_traits< GraphFirst, GraphSecond,
            VertexIndexMapFirst, VertexIndexMapSecond >
            SubGraphTraits;

        typedef typename SubGraphTraits::correspondence_map_first_to_second_type
            CachedCorrespondenceMapFirstToSecond;

        typedef typename SubGraphTraits::correspondence_map_second_to_first_type
            CachedCorrespondenceMapSecondToFirst;

        typedef std::pair< VertexSizeFirst,
            std::pair< CachedCorrespondenceMapFirstToSecond,
                CachedCorrespondenceMapSecondToFirst > >
            SubGraph;

        typedef std::vector< SubGraph > SubGraphList;

        unique_maximum_subgraph_interceptor(const GraphFirst& graph1,
            const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
            const VertexIndexMapSecond vindex_map2,
            SubGraphCallback user_callback)
        : m_graph1(graph1)
        , m_graph2(graph2)
        , m_vindex_map1(vindex_map1)
        , m_vindex_map2(vindex_map2)
        , m_subgraphs(make_shared< SubGraphList >())
        , m_largest_size_so_far(make_shared< VertexSizeFirst >(0))
        , m_user_callback(user_callback)
        {
        }

        template < typename CorrespondenceMapFirstToSecond,
            typename CorrespondenceMapSecondToFirst >
        bool operator()(
            CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
            CorrespondenceMapSecondToFirst correspondence_map_2_to_1,
            VertexSizeFirst subgraph_size)
        {

            if (subgraph_size > *m_largest_size_so_far)
            {
                m_subgraphs->clear();
                *m_largest_size_so_far = subgraph_size;
            }

            if (subgraph_size == *m_largest_size_so_far)
            {

                // Check if subgraph is unique
                for (typename SubGraphList::const_iterator subgraph_iter
                     = m_subgraphs->begin();
                     subgraph_iter != m_subgraphs->end(); ++subgraph_iter)
                {

                    SubGraph subgraph_cached = *subgraph_iter;

                    if (!are_property_maps_different(correspondence_map_1_to_2,
                            subgraph_cached.second.first, m_graph1))
                    {

                        // New subgraph is a duplicate
                        return (true);
                    }
                }

                // Subgraph is unique, so make a cached copy
                CachedCorrespondenceMapFirstToSecond new_subgraph_1_to_2
                    = CachedCorrespondenceMapFirstToSecond(
                        num_vertices(m_graph1), m_vindex_map1);

                CachedCorrespondenceMapSecondToFirst new_subgraph_2_to_1
                    = CachedCorrespondenceMapSecondToFirst(
                        num_vertices(m_graph2), m_vindex_map2);

                BGL_FORALL_VERTICES_T(vertex1, m_graph1, GraphFirst)
                {
                    put(new_subgraph_1_to_2, vertex1,
                        get(correspondence_map_1_to_2, vertex1));
                }

                BGL_FORALL_VERTICES_T(vertex2, m_graph2, GraphFirst)
                {
                    put(new_subgraph_2_to_1, vertex2,
                        get(correspondence_map_2_to_1, vertex2));
                }

                m_subgraphs->push_back(std::make_pair(subgraph_size,
                    std::make_pair(new_subgraph_1_to_2, new_subgraph_2_to_1)));
            }

            return (true);
        }

        void output_subgraphs()
        {
            for (typename SubGraphList::const_iterator subgraph_iter
                 = m_subgraphs->begin();
                 subgraph_iter != m_subgraphs->end(); ++subgraph_iter)
            {

                SubGraph subgraph_cached = *subgraph_iter;
                m_user_callback(subgraph_cached.second.first,
                    subgraph_cached.second.second, subgraph_cached.first);
            }
        }

    private:
        const GraphFirst& m_graph1;
        const GraphFirst& m_graph2;
        const VertexIndexMapFirst m_vindex_map1;
        const VertexIndexMapSecond m_vindex_map2;
        shared_ptr< SubGraphList > m_subgraphs;
        shared_ptr< VertexSizeFirst > m_largest_size_so_far;
        SubGraphCallback m_user_callback;
    };

} // namespace detail

// Enumerates the largest, unique common subgraphs found between
// graph1 and graph2.  Note that the ENTIRE search space is explored
// before user_callback is actually invoked.
template < typename GraphFirst, typename GraphSecond,
    typename VertexIndexMapFirst, typename VertexIndexMapSecond,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_maximum_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, const VertexIndexMapFirst vindex_map1,
    const VertexIndexMapSecond vindex_map2,
    EdgeEquivalencePredicate edges_equivalent,
    VertexEquivalencePredicate vertices_equivalent,
    bool only_connected_subgraphs, SubGraphCallback user_callback)
{
    detail::unique_maximum_subgraph_interceptor< GraphFirst, GraphSecond,
        VertexIndexMapFirst, VertexIndexMapSecond, SubGraphCallback >
        unique_max_interceptor(
            graph1, graph2, vindex_map1, vindex_map2, user_callback);

    detail::mcgregor_common_subgraphs_internal_init(graph1, graph2, vindex_map1,
        vindex_map2, edges_equivalent, vertices_equivalent,
        only_connected_subgraphs, unique_max_interceptor);

    // Only output the largest, unique subgraphs
    unique_max_interceptor.output_subgraphs();
}

// Variant of mcgregor_common_subgraphs_maximum_unique with all default
// parameters
template < typename GraphFirst, typename GraphSecond,
    typename SubGraphCallback >
void mcgregor_common_subgraphs_maximum_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback)
{

    mcgregor_common_subgraphs_maximum_unique(graph1, graph2,
        get(vertex_index, graph1), get(vertex_index, graph2),
        always_equivalent(), always_equivalent(), only_connected_subgraphs,
        user_callback);
}

// Named parameter variant of
// mcgregor_common_subgraphs_maximum_unique
template < typename GraphFirst, typename GraphSecond, typename SubGraphCallback,
    typename Param, typename Tag, typename Rest >
void mcgregor_common_subgraphs_maximum_unique(const GraphFirst& graph1,
    const GraphSecond& graph2, bool only_connected_subgraphs,
    SubGraphCallback user_callback,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    mcgregor_common_subgraphs_maximum_unique(graph1, graph2,
        choose_const_pmap(
            get_param(params, vertex_index1), graph1, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph2, vertex_index),
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()),
        only_connected_subgraphs, user_callback);
}

// ==========================================================================

// Fills a membership map (vertex -> bool) using the information
// present in correspondence_map_1_to_2. Every vertex in a
// membership map will have a true value only if it is not
// associated with a null vertex in the correspondence map.
template < typename GraphSecond, typename GraphFirst,
    typename CorrespondenceMapFirstToSecond, typename MembershipMapFirst >
void fill_membership_map(const GraphFirst& graph1,
    const CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
    MembershipMapFirst membership_map1)
{

    BGL_FORALL_VERTICES_T(vertex1, graph1, GraphFirst)
    {
        put(membership_map1, vertex1,
            get(correspondence_map_1_to_2, vertex1)
                != graph_traits< GraphSecond >::null_vertex());
    }
}

// Traits associated with a membership map filtered graph.  Provided
// for convenience to access graph and vertex filter types.
template < typename Graph, typename MembershipMap >
struct membership_filtered_graph_traits
{
    typedef property_map_filter< MembershipMap > vertex_filter_type;
    typedef filtered_graph< Graph, keep_all, vertex_filter_type > graph_type;
};

// Returns a filtered sub-graph of graph whose edge and vertex
// inclusion is dictated by membership_map.
template < typename Graph, typename MembershipMap >
typename membership_filtered_graph_traits< Graph, MembershipMap >::graph_type
make_membership_filtered_graph(
    const Graph& graph, MembershipMap& membership_map)
{

    typedef membership_filtered_graph_traits< Graph, MembershipMap > MFGTraits;
    typedef typename MFGTraits::graph_type MembershipFilteredGraph;

    typename MFGTraits::vertex_filter_type v_filter(membership_map);

    return (MembershipFilteredGraph(graph, keep_all(), v_filter));
}

} // namespace boost

#endif // BOOST_GRAPH_MCGREGOR_COMMON_SUBGRAPHS_HPP

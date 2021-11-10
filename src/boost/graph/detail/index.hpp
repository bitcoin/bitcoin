// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_DETAIL_INDEX_HPP
#define BOOST_GRAPH_DETAIL_INDEX_HPP

#include <boost/graph/graph_traits.hpp>

// The structures in this module are responsible for selecting and defining
// types for accessing a builting index map. Note that the selection of these
// types requires the Graph parameter to model either VertexIndexGraph or
// EdgeIndexGraph.

namespace boost
{
namespace detail
{
    template < typename Graph > struct vertex_indexer
    {
        typedef vertex_index_t index_type;
        typedef typename property_map< Graph, vertex_index_t >::type map_type;
        typedef typename property_map< Graph, vertex_index_t >::const_type
            const_map_type;
        typedef typename property_traits< map_type >::value_type value_type;
        typedef typename graph_traits< Graph >::vertex_descriptor key_type;

        static const_map_type index_map(const Graph& g)
        {
            return get(vertex_index, g);
        }

        static map_type index_map(Graph& g) { return get(vertex_index, g); }

        static value_type index(key_type k, const Graph& g)
        {
            return get(vertex_index, g, k);
        }
    };

    template < typename Graph > struct edge_indexer
    {
        typedef edge_index_t index_type;
        typedef typename property_map< Graph, edge_index_t >::type map_type;
        typedef typename property_map< Graph, edge_index_t >::const_type
            const_map_type;
        typedef typename property_traits< map_type >::value_type value_type;
        typedef typename graph_traits< Graph >::edge_descriptor key_type;

        static const_map_type index_map(const Graph& g)
        {
            return get(edge_index, g);
        }

        static map_type index_map(Graph& g) { return get(edge_index, g); }

        static value_type index(key_type k, const Graph& g)
        {
            return get(edge_index, g, k);
        }
    };

    // NOTE: The Graph parameter MUST be a model of VertexIndexGraph or
    // VertexEdgeGraph - whichever type Key is selecting.
    template < typename Graph, typename Key > struct choose_indexer
    {
        typedef typename mpl::if_<
            is_same< Key, typename graph_traits< Graph >::vertex_descriptor >,
            vertex_indexer< Graph >, edge_indexer< Graph > >::type indexer_type;
        typedef typename indexer_type::index_type index_type;
    };
}
}

#endif

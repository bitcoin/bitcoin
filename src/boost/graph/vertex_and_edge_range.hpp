// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#ifndef BOOST_GRAPH_VERTEX_AND_EDGE_RANGE_HPP
#define BOOST_GRAPH_VERTEX_AND_EDGE_RANGE_HPP

#include <boost/graph/graph_traits.hpp>
#include <iterator>

namespace boost
{

namespace graph
{
    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    class vertex_and_edge_range
    {
        typedef graph_traits< Graph > traits_type;

    public:
        typedef typename traits_type::directed_category directed_category;
        typedef
            typename traits_type::edge_parallel_category edge_parallel_category;
        struct traversal_category : public virtual vertex_list_graph_tag,
                                    public virtual edge_list_graph_tag
        {
        };

        typedef std::size_t vertices_size_type;
        typedef VertexIterator vertex_iterator;
        typedef typename std::iterator_traits< VertexIterator >::value_type
            vertex_descriptor;

        typedef EdgeIterator edge_iterator;
        typedef typename std::iterator_traits< EdgeIterator >::value_type
            edge_descriptor;

        typedef std::size_t edges_size_type;

        typedef void adjacency_iterator;
        typedef void out_edge_iterator;
        typedef void in_edge_iterator;
        typedef void degree_size_type;

        static vertex_descriptor null_vertex()
        {
            return traits_type::null_vertex();
        }

        vertex_and_edge_range(const Graph& g, VertexIterator first_v,
            VertexIterator last_v, vertices_size_type n, EdgeIterator first_e,
            EdgeIterator last_e, edges_size_type m)
        : g(&g)
        , first_vertex(first_v)
        , last_vertex(last_v)
        , m_num_vertices(n)
        , first_edge(first_e)
        , last_edge(last_e)
        , m_num_edges(m)
        {
        }

        vertex_and_edge_range(const Graph& g, VertexIterator first_v,
            VertexIterator last_v, EdgeIterator first_e, EdgeIterator last_e)
        : g(&g)
        , first_vertex(first_v)
        , last_vertex(last_v)
        , first_edge(first_e)
        , last_edge(last_e)
        {
            m_num_vertices = std::distance(first_v, last_v);
            m_num_edges = std::distance(first_e, last_e);
        }

        const Graph* g;
        vertex_iterator first_vertex;
        vertex_iterator last_vertex;
        vertices_size_type m_num_vertices;
        edge_iterator first_edge;
        edge_iterator last_edge;
        edges_size_type m_num_edges;
    };

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline std::pair< VertexIterator, VertexIterator > vertices(
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return std::make_pair(g.first_vertex, g.last_vertex);
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline typename vertex_and_edge_range< Graph, VertexIterator,
        EdgeIterator >::vertices_size_type
    num_vertices(
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return g.m_num_vertices;
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline std::pair< EdgeIterator, EdgeIterator > edges(
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return std::make_pair(g.first_edge, g.last_edge);
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline typename vertex_and_edge_range< Graph, VertexIterator,
        EdgeIterator >::edges_size_type
    num_edges(
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return g.m_num_edges;
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline typename vertex_and_edge_range< Graph, VertexIterator,
        EdgeIterator >::vertex_descriptor
    source(typename vertex_and_edge_range< Graph, VertexIterator,
               EdgeIterator >::edge_descriptor e,
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return source(e, *g.g);
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline typename vertex_and_edge_range< Graph, VertexIterator,
        EdgeIterator >::vertex_descriptor
    target(typename vertex_and_edge_range< Graph, VertexIterator,
               EdgeIterator >::edge_descriptor e,
        const vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >& g)
    {
        return target(e, *g.g);
    }

    template < typename Graph, typename VertexIterator, typename EdgeIterator >
    inline vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >
    make_vertex_and_edge_range(const Graph& g, VertexIterator first_v,
        VertexIterator last_v, EdgeIterator first_e, EdgeIterator last_e)
    {
        typedef vertex_and_edge_range< Graph, VertexIterator, EdgeIterator >
            result_type;
        return result_type(g, first_v, last_v, first_e, last_e);
    }

} // end namespace graph

using graph::make_vertex_and_edge_range;
using graph::vertex_and_edge_range;

} // end namespace boost
#endif // BOOST_GRAPH_VERTEX_AND_EDGE_RANGE_HPP

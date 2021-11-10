// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_CLUSTERING_COEFFICIENT_HPP
#define BOOST_GRAPH_CLUSTERING_COEFFICIENT_HPP

#include <boost/next_prior.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/lookup_edge.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{
namespace detail
{
    template < class Graph >
    inline typename graph_traits< Graph >::degree_size_type possible_edges(
        const Graph& g, std::size_t k, directed_tag)
    {
        BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
        typedef typename graph_traits< Graph >::degree_size_type T;
        return T(k) * (T(k) - 1);
    }

    template < class Graph >
    inline typename graph_traits< Graph >::degree_size_type possible_edges(
        const Graph& g, size_t k, undirected_tag)
    {
        // dirty little trick...
        return possible_edges(g, k, directed_tag()) / 2;
    }

    // This template matches directedS and bidirectionalS.
    template < class Graph >
    inline typename graph_traits< Graph >::degree_size_type count_edges(
        const Graph& g, typename graph_traits< Graph >::vertex_descriptor u,
        typename graph_traits< Graph >::vertex_descriptor v, directed_tag)

    {
        BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< Graph >));
        return (lookup_edge(u, v, g).second ? 1 : 0)
            + (lookup_edge(v, u, g).second ? 1 : 0);
    }

    // This template matches undirectedS
    template < class Graph >
    inline typename graph_traits< Graph >::degree_size_type count_edges(
        const Graph& g, typename graph_traits< Graph >::vertex_descriptor u,
        typename graph_traits< Graph >::vertex_descriptor v, undirected_tag)
    {
        BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< Graph >));
        return lookup_edge(u, v, g).second ? 1 : 0;
    }
}

template < typename Graph, typename Vertex >
inline typename graph_traits< Graph >::degree_size_type
num_paths_through_vertex(const Graph& g, Vertex v)
{
    BOOST_CONCEPT_ASSERT((AdjacencyGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::directed_category Directed;
    typedef
        typename graph_traits< Graph >::adjacency_iterator AdjacencyIterator;

    // TODO: There should actually be a set of neighborhood functions
    // for things like this (num_neighbors() would be great).

    AdjacencyIterator i, end;
    boost::tie(i, end) = adjacent_vertices(v, g);
    std::size_t k = std::distance(i, end);
    return detail::possible_edges(g, k, Directed());
}

template < typename Graph, typename Vertex >
inline typename graph_traits< Graph >::degree_size_type num_triangles_on_vertex(
    const Graph& g, Vertex v)
{
    BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< Graph >));
    BOOST_CONCEPT_ASSERT((AdjacencyGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::degree_size_type Degree;
    typedef typename graph_traits< Graph >::directed_category Directed;
    typedef
        typename graph_traits< Graph >::adjacency_iterator AdjacencyIterator;

    // TODO: I might be able to reduce the requirement from adjacency graph
    // to incidence graph by using out edges.

    Degree count(0);
    AdjacencyIterator i, j, end;
    for (boost::tie(i, end) = adjacent_vertices(v, g); i != end; ++i)
    {
        for (j = boost::next(i); j != end; ++j)
        {
            count += detail::count_edges(g, *i, *j, Directed());
        }
    }
    return count;
} /* namespace detail */

template < typename T, typename Graph, typename Vertex >
inline T clustering_coefficient(const Graph& g, Vertex v)
{
    T zero(0);
    T routes = T(num_paths_through_vertex(g, v));
    return (routes > zero) ? T(num_triangles_on_vertex(g, v)) / routes : zero;
}

template < typename Graph, typename Vertex >
inline double clustering_coefficient(const Graph& g, Vertex v)
{
    return clustering_coefficient< double >(g, v);
}

template < typename Graph, typename ClusteringMap >
inline typename property_traits< ClusteringMap >::value_type
all_clustering_coefficients(const Graph& g, ClusteringMap cm)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    BOOST_CONCEPT_ASSERT((WritablePropertyMapConcept< ClusteringMap, Vertex >));
    typedef typename property_traits< ClusteringMap >::value_type Coefficient;

    Coefficient sum(0);
    VertexIterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        Coefficient cc = clustering_coefficient< Coefficient >(g, *i);
        put(cm, *i, cc);
        sum += cc;
    }
    return sum / Coefficient(num_vertices(g));
}

template < typename Graph, typename ClusteringMap >
inline typename property_traits< ClusteringMap >::value_type
mean_clustering_coefficient(const Graph& g, ClusteringMap cm)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< ClusteringMap, Vertex >));
    typedef typename property_traits< ClusteringMap >::value_type Coefficient;

    Coefficient cc(0);
    VertexIterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        cc += get(cm, *i);
    }
    return cc / Coefficient(num_vertices(g));
}

} /* namespace boost */

#endif

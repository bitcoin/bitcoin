//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2004 The Trustees of Indiana University
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_SEQUENTIAL_VERTEX_COLORING_HPP
#define BOOST_GRAPH_SEQUENTIAL_VERTEX_COLORING_HPP

#include <vector>
#include <boost/graph/graph_traits.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/limits.hpp>

#ifdef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
#include <iterator>
#endif

/* This algorithm is to find coloring of a graph

   Algorithm:
   Let G = (V,E) be a graph with vertices (somehow) ordered v_1, v_2, ...,
   v_n. For k = 1, 2, ..., n the sequential algorithm assigns v_k to the
   smallest possible color.

   Reference:

   Thomas F. Coleman and Jorge J. More, Estimation of sparse Jacobian
   matrices and graph coloring problems. J. Numer. Anal. V20, P187-209, 1983

   v_k is stored as o[k] here.

   The color of the vertex v will be stored in color[v].
   i.e., vertex v belongs to coloring color[v] */

namespace boost
{
template < class VertexListGraph, class OrderPA, class ColorMap >
typename property_traits< ColorMap >::value_type sequential_vertex_coloring(
    const VertexListGraph& G, OrderPA order, ColorMap color)
{
    typedef graph_traits< VertexListGraph > GraphTraits;
    typedef typename GraphTraits::vertex_descriptor Vertex;
    typedef typename property_traits< ColorMap >::value_type size_type;

    size_type max_color = 0;
    const size_type V = num_vertices(G);

    // We need to keep track of which colors are used by
    // adjacent vertices. We do this by marking the colors
    // that are used. The mark array contains the mark
    // for each color. The length of mark is the
    // number of vertices since the maximum possible number of colors
    // is the number of vertices.
    std::vector< size_type > mark(V,
        std::numeric_limits< size_type >::max
            BOOST_PREVENT_MACRO_SUBSTITUTION());

    // Initialize colors
    typename GraphTraits::vertex_iterator v, vend;
    for (boost::tie(v, vend) = vertices(G); v != vend; ++v)
        put(color, *v, V - 1);

    // Determine the color for every vertex one by one
    for (size_type i = 0; i < V; i++)
    {
        Vertex current = get(order, i);
        typename GraphTraits::adjacency_iterator v, vend;

        // Mark the colors of vertices adjacent to current.
        // i can be the value for marking since i increases successively
        for (boost::tie(v, vend) = adjacent_vertices(current, G); v != vend;
             ++v)
            mark[get(color, *v)] = i;

        // Next step is to assign the smallest un-marked color
        // to the current vertex.
        size_type j = 0;

        // Scan through all useable colors, find the smallest possible
        // color that is not used by neighbors.  Note that if mark[j]
        // is equal to i, color j is used by one of the current vertex's
        // neighbors.
        while (j < max_color && mark[j] == i)
            ++j;

        if (j == max_color) // All colors are used up. Add one more color
            ++max_color;

        // At this point, j is the smallest possible color
        put(color, current, j); // Save the color of vertex current
    }

    return max_color;
}

template < class VertexListGraph, class ColorMap >
typename property_traits< ColorMap >::value_type sequential_vertex_coloring(
    const VertexListGraph& G, ColorMap color)
{
    typedef typename graph_traits< VertexListGraph >::vertex_descriptor
        vertex_descriptor;
    typedef typename graph_traits< VertexListGraph >::vertex_iterator
        vertex_iterator;

    std::pair< vertex_iterator, vertex_iterator > v = vertices(G);
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
    std::vector< vertex_descriptor > order(v.first, v.second);
#else
    std::vector< vertex_descriptor > order;
    order.reserve(std::distance(v.first, v.second));
    while (v.first != v.second)
        order.push_back(*v.first++);
#endif
    return sequential_vertex_coloring(G,
        make_iterator_property_map(order.begin(), identity_property_map(),
            graph_traits< VertexListGraph >::null_vertex()),
        color);
}
}

#endif

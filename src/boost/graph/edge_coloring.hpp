//=======================================================================
// Copyright 2013 Maciej Piechotka
// Authors: Maciej Piechotka
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_EDGE_COLORING_HPP
#define BOOST_GRAPH_EDGE_COLORING_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/properties.hpp>
#include <algorithm>
#include <limits>
#include <vector>

/* This algorithm is to find coloring of an edges

   Reference:

   Misra, J., & Gries, D. (1992). A constructive proof of Vizing's
   theorem. In Information Processing Letters.
*/

namespace boost
{
namespace detail
{
    template < typename Graph, typename ColorMap >
    bool is_free(const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor u,
        typename boost::property_traits< ColorMap >::value_type free_color)
    {
        typedef typename boost::property_traits< ColorMap >::value_type color_t;
        if (free_color == (std::numeric_limits< color_t >::max)())
            return false;
        BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
        {
            if (get(color, e) == free_color)
            {
                return false;
            }
        }
        return true;
    }

    template < typename Graph, typename ColorMap >
    std::vector< typename boost::graph_traits< Graph >::vertex_descriptor >
    maximal_fan(const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor x,
        typename boost::graph_traits< Graph >::vertex_descriptor y)
    {
        typedef
            typename boost::graph_traits< Graph >::vertex_descriptor vertex_t;
        std::vector< vertex_t > fan;
        fan.push_back(y);
        bool extended;
        do
        {
            extended = false;
            BGL_FORALL_OUTEDGES_T(x, e, g, Graph)
            {
                vertex_t v = target(e, g);
                if (is_free(g, color, fan.back(), get(color, e))
                    && std::find(fan.begin(), fan.end(), v) == fan.end())
                {
                    fan.push_back(v);
                    extended = true;
                }
            }
        } while (extended);
        return fan;
    }
    template < typename Graph, typename ColorMap >
    typename boost::property_traits< ColorMap >::value_type find_free_color(
        const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor u)
    {
        typename boost::property_traits< ColorMap >::value_type c = 0;
        while (!is_free(g, color, u, c))
            c++;
        return c;
    }

    template < typename Graph, typename ColorMap >
    void invert_cd_path(const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor x,
        typename boost::graph_traits< Graph >::edge_descriptor eold,
        typename boost::property_traits< ColorMap >::value_type c,
        typename boost::property_traits< ColorMap >::value_type d)
    {
        put(color, eold, d);
        BGL_FORALL_OUTEDGES_T(x, e, g, Graph)
        {
            if (get(color, e) == d && e != eold)
            {
                invert_cd_path(g, color, target(e, g), e, d, c);
                return;
            }
        }
    }

    template < typename Graph, typename ColorMap >
    void invert_cd_path(const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor x,
        typename boost::property_traits< ColorMap >::value_type c,
        typename boost::property_traits< ColorMap >::value_type d)
    {
        BGL_FORALL_OUTEDGES_T(x, e, g, Graph)
        {
            if (get(color, e) == d)
            {
                invert_cd_path(g, color, target(e, g), e, d, c);
                return;
            }
        }
    }

    template < typename Graph, typename ColorMap, typename ForwardIterator >
    void rotate_fan(const Graph& g, ColorMap color,
        typename boost::graph_traits< Graph >::vertex_descriptor x,
        ForwardIterator begin, ForwardIterator end)
    {
        typedef typename boost::graph_traits< Graph >::edge_descriptor edge_t;
        if (begin == end)
        {
            return;
        }
        edge_t previous = edge(x, *begin, g).first;
        for (begin++; begin != end; begin++)
        {
            edge_t current = edge(x, *begin, g).first;
            put(color, previous, get(color, current));
            previous = current;
        }
    }

    template < typename Graph, typename ColorMap > class find_free_in_fan
    {
    public:
        find_free_in_fan(const Graph& graph, const ColorMap color,
            typename boost::property_traits< ColorMap >::value_type d)
        : graph(graph), color(color), d(d)
        {
        }
        bool operator()(
            const typename boost::graph_traits< Graph >::vertex_descriptor u)
            const
        {
            return is_free(graph, color, u, d);
        }

    private:
        const Graph& graph;
        const ColorMap color;
        const typename boost::property_traits< ColorMap >::value_type d;
    };
}

template < typename Graph, typename ColorMap >
typename boost::property_traits< ColorMap >::value_type color_edge(
    const Graph& g, ColorMap color,
    typename boost::graph_traits< Graph >::edge_descriptor e)
{
    typedef typename boost::graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename boost::property_traits< ColorMap >::value_type color_t;
    typedef typename std::vector< vertex_t >::iterator fan_iterator;
    using namespace detail;
    vertex_t x = source(e, g), y = target(e, g);
    std::vector< vertex_t > fan = maximal_fan(g, color, x, y);
    color_t c = find_free_color(g, color, x);
    color_t d = find_free_color(g, color, fan.back());
    invert_cd_path(g, color, x, c, d);
    fan_iterator w = std::find_if(fan.begin(), fan.end(),
        find_free_in_fan< Graph, ColorMap >(g, color, d));
    rotate_fan(g, color, x, fan.begin(), w + 1);
    put(color, edge(x, *w, g).first, d);
    return (std::max)(c, d);
}

template < typename Graph, typename ColorMap >
typename boost::property_traits< ColorMap >::value_type edge_coloring(
    const Graph& g, ColorMap color)
{
    typedef typename boost::property_traits< ColorMap >::value_type color_t;
    BGL_FORALL_EDGES_T(e, g, Graph)
    {
        put(color, e, (std::numeric_limits< color_t >::max)());
    }
    color_t colors = 0;
    BGL_FORALL_EDGES_T(e, g, Graph)
    {
        colors = (std::max)(colors, color_edge(g, color, e) + 1);
    }
    return colors;
}
}

#endif

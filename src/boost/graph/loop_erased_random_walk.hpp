// Copyright 2010 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Andrew Lumsdaine

#ifndef BOOST_GRAPH_LOOP_ERASED_RANDOM_WALK_HPP
#define BOOST_GRAPH_LOOP_ERASED_RANDOM_WALK_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/random.hpp>
#include <boost/next_prior.hpp>
#include <vector>
#include <boost/assert.hpp>

namespace boost
{

struct BOOST_SYMBOL_VISIBLE loop_erased_random_walk_stuck
: public std::exception
{
    virtual ~loop_erased_random_walk_stuck() throw() {}
    inline virtual const char* what() const throw()
    {
        return "Loop-erased random walk found a vertex with no out-edges";
    }
};

// Do a loop-erased random walk from vertex s to any vertex colored black (or
// actually any color other than white or gray) in the color map.  The color
// white is for vertices that are not part of the path, while gray is for
// those that are on the path (for cycle detection).  The vector path is used
// for temporary storage and as the result of the algorithm; while all
// elements of the path except the last have their colors set to gray upon
// return.  Vertex s must start off colored white.
//
// Useful references:
// http://everything2.com/title/loop-erased+random+walk
// Wikipedia page on "Loop-Erased Random Walk"

template < typename Graph, typename ColorMap, typename NextEdge >
void loop_erased_random_walk(const Graph& g,
    typename boost::graph_traits< Graph >::vertex_descriptor s,
    NextEdge next_edge, ColorMap color,
    std::vector< typename boost::graph_traits< Graph >::vertex_descriptor >&
        path)
{
    typedef typename boost::graph_traits< Graph >::vertex_descriptor
        vertex_descriptor;
    typedef
        typename boost::graph_traits< Graph >::edge_descriptor edge_descriptor;
    typedef typename boost::property_traits< ColorMap >::value_type color_t;
    typedef boost::color_traits< color_t > color_gen;

    BOOST_ASSERT(get(color, s) == color_gen::white());
    path.clear();
    path.push_back(s);
    put(color, s, color_gen::gray());
    while (true)
    {
        edge_descriptor e = next_edge(s, g);
        vertex_descriptor t = target(e, g);
        color_t t_color = get(color, t);
        if (t_color == color_gen::white())
        {
            path.push_back(t);
            put(color, t, color_gen::gray());
            s = t;
        }
        else if (t_color == color_gen::gray())
        {
            // Found a loop; delete from path from the first occurrence of t to
            // the end, coloring vertices white.
            typename std::vector< vertex_descriptor >::iterator it
                = std::find(path.begin(), path.end(), t);
            BOOST_ASSERT(it != path.end());
            ++it;
            for (typename std::vector< vertex_descriptor >::iterator j = it;
                 j != path.end(); ++j)
            {
                put(color, *j, color_gen::white());
            }
            path.erase(it, path.end());
            s = t;
        }
        else
        {
            // Done
            path.push_back(t);
            break;
        }
    }
}

template < typename Graph, typename Gen > class unweighted_random_out_edge_gen
{
    Gen& gen;

    typedef boost::graph_traits< Graph > gt;

public:
    unweighted_random_out_edge_gen(Gen& gen) : gen(gen) {}

    typename gt::edge_descriptor operator()(
        typename gt::vertex_descriptor src, const Graph& g) const
    {
        if (out_degree(src, g) == 0)
            throw loop_erased_random_walk_stuck();
        return boost::random_out_edge(g, src, gen);
    }
};

template < typename Graph, typename WeightMap, typename Gen >
class weighted_random_out_edge_gen
{
    WeightMap weight;
    Gen& gen;

    typedef boost::graph_traits< Graph > gt;

public:
    weighted_random_out_edge_gen(const WeightMap& weight, Gen& gen)
    : weight(weight), gen(gen)
    {
    }

    typename gt::edge_descriptor operator()(
        typename gt::vertex_descriptor src, const Graph& g) const
    {
        if (out_degree(src, g) == 0)
            throw loop_erased_random_walk_stuck();
        return boost::weighted_random_out_edge(g, src, weight, gen);
    }
};
}

#endif // BOOST_GRAPH_LOOP_ERASED_RANDOM_WALK_HPP

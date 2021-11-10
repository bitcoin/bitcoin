// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_CYCLE_HPP
#define BOOST_GRAPH_CYCLE_HPP

#include <vector>

#include <boost/config.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/concept/assert.hpp>

#include <boost/concept/detail/concept_def.hpp>
namespace boost
{
namespace concepts
{
    BOOST_concept(CycleVisitor, (Visitor)(Path)(Graph))
    {
        BOOST_CONCEPT_USAGE(CycleVisitor) { vis.cycle(p, g); }

    private:
        Visitor vis;
        Graph g;
        Path p;
    };
} /* namespace concepts */
using concepts::CycleVisitorConcept;
} /* namespace boost */
#include <boost/concept/detail/concept_undef.hpp>

namespace boost
{

// The implementation of this algorithm is a reproduction of the Teirnan
// approach for directed graphs: bibtex follows
//
//     @article{362819,
//         author = {James C. Tiernan},
//         title = {An efficient search algorithm to find the elementary
//         circuits of a graph}, journal = {Commun. ACM}, volume = {13}, number
//         = {12}, year = {1970}, issn = {0001-0782}, pages = {722--726}, doi =
//         {http://doi.acm.org/10.1145/362814.362819},
//             publisher = {ACM Press},
//             address = {New York, NY, USA},
//         }
//
// It should be pointed out that the author does not provide a complete analysis
// for either time or space. This is in part, due to the fact that it's a fairly
// input sensitive problem related to the density and construction of the graph,
// not just its size.
//
// I've also taken some liberties with the interpretation of the algorithm -
// I've basically modernized it to use real data structures (no more arrays and
// matrices). Oh... and there's explicit control structures - not just gotos.
//
// The problem is definitely NP-complete, an unbounded implementation of this
// will probably run for quite a while on a large graph. The conclusions
// of this paper also reference a Paton algorithm for undirected graphs as being
// much more efficient (apparently based on spanning trees). Although not
// implemented, it can be found here:
//
//     @article{363232,
//         author = {Keith Paton},
//         title = {An algorithm for finding a fundamental set of cycles of a
//         graph}, journal = {Commun. ACM}, volume = {12}, number = {9}, year =
//         {1969}, issn = {0001-0782}, pages = {514--518}, doi =
//         {http://doi.acm.org/10.1145/363219.363232},
//             publisher = {ACM Press},
//             address = {New York, NY, USA},
//         }

/**
 * The default cycle visitor provides an empty visit function for cycle
 * visitors.
 */
struct cycle_visitor
{
    template < typename Path, typename Graph >
    inline void cycle(const Path& p, const Graph& g)
    {
    }
};

/**
 * The min_max_cycle_visitor simultaneously records the minimum and maximum
 * cycles in a graph.
 */
struct min_max_cycle_visitor
{
    min_max_cycle_visitor(std::size_t& min_, std::size_t& max_)
    : minimum(min_), maximum(max_)
    {
    }

    template < typename Path, typename Graph >
    inline void cycle(const Path& p, const Graph& g)
    {
        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();
        std::size_t len = p.size();
        minimum = min BOOST_PREVENT_MACRO_SUBSTITUTION(minimum, len);
        maximum = max BOOST_PREVENT_MACRO_SUBSTITUTION(maximum, len);
    }
    std::size_t& minimum;
    std::size_t& maximum;
};

inline min_max_cycle_visitor find_min_max_cycle(
    std::size_t& min_, std::size_t& max_)
{
    return min_max_cycle_visitor(min_, max_);
}

namespace detail
{
    template < typename Graph, typename Path >
    inline bool is_vertex_in_path(const Graph&,
        typename graph_traits< Graph >::vertex_descriptor v, const Path& p)
    {
        return (std::find(p.begin(), p.end(), v) != p.end());
    }

    template < typename Graph, typename ClosedMatrix >
    inline bool is_path_closed(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor u,
        typename graph_traits< Graph >::vertex_descriptor v,
        const ClosedMatrix& closed)
    {
        // the path from u to v is closed if v can be found in the list
        // of closed vertices associated with u.
        typedef typename ClosedMatrix::const_reference Row;
        Row r = closed[get(vertex_index, g, u)];
        if (find(r.begin(), r.end(), v) != r.end())
        {
            return true;
        }
        return false;
    }

    template < typename Graph, typename Path, typename ClosedMatrix >
    inline bool can_extend_path(const Graph& g,
        typename graph_traits< Graph >::edge_descriptor e, const Path& p,
        const ClosedMatrix& m)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< Graph >));
        BOOST_CONCEPT_ASSERT((VertexIndexGraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

        // get the vertices in question
        Vertex u = source(e, g), v = target(e, g);

        // conditions for allowing a traversal along this edge are:
        // 1. the index of v must be greater than that at which the
        //    path is rooted (p.front()).
        // 2. the vertex v cannot already be in the path
        // 3. the vertex v cannot be closed to the vertex u

        bool indices
            = get(vertex_index, g, p.front()) < get(vertex_index, g, v);
        bool path = !is_vertex_in_path(g, v, p);
        bool closed = !is_path_closed(g, u, v, m);
        return indices && path && closed;
    }

    template < typename Graph, typename Path >
    inline bool can_wrap_path(const Graph& g, const Path& p)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef typename graph_traits< Graph >::out_edge_iterator OutIterator;

        // iterate over the out-edges of the back, looking for the
        // front of the path. also, we can't travel along the same
        // edge that we did on the way here, but we don't quite have the
        // stringent requirements that we do in can_extend_path().
        Vertex u = p.back(), v = p.front();
        OutIterator i, end;
        for (boost::tie(i, end) = out_edges(u, g); i != end; ++i)
        {
            if ((target(*i, g) == v))
            {
                return true;
            }
        }
        return false;
    }

    template < typename Graph, typename Path, typename ClosedMatrix >
    inline typename graph_traits< Graph >::vertex_descriptor extend_path(
        const Graph& g, Path& p, ClosedMatrix& closed)
    {
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef typename graph_traits< Graph >::out_edge_iterator OutIterator;

        // get the current vertex
        Vertex u = p.back();
        Vertex ret = graph_traits< Graph >::null_vertex();

        // AdjacencyIterator i, end;
        OutIterator i, end;
        for (boost::tie(i, end) = out_edges(u, g); i != end; ++i)
        {
            Vertex v = target(*i, g);

            // if we can actually extend along this edge,
            // then that's what we want to do
            if (can_extend_path(g, *i, p, closed))
            {
                p.push_back(v); // add the vertex to the path
                ret = v;
                break;
            }
        }
        return ret;
    }

    template < typename Graph, typename Path, typename ClosedMatrix >
    inline bool exhaust_paths(const Graph& g, Path& p, ClosedMatrix& closed)
    {
        BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

        // if there's more than one vertex in the path, this closes
        // of some possible routes and returns true. otherwise, if there's
        // only one vertex left, the vertex has been used up
        if (p.size() > 1)
        {
            // get the last and second to last vertices, popping the last
            // vertex off the path
            Vertex last, prev;
            last = p.back();
            p.pop_back();
            prev = p.back();

            // reset the closure for the last vertex of the path and
            // indicate that the last vertex in p is now closed to
            // the next-to-last vertex in p
            closed[get(vertex_index, g, last)].clear();
            closed[get(vertex_index, g, prev)].push_back(last);
            return true;
        }
        else
        {
            return false;
        }
    }

    template < typename Graph, typename Visitor >
    inline void all_cycles_from_vertex(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor v, Visitor vis,
        std::size_t minlen, std::size_t maxlen)
    {
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef std::vector< Vertex > Path;
        BOOST_CONCEPT_ASSERT((CycleVisitorConcept< Visitor, Path, Graph >));
        typedef std::vector< Vertex > VertexList;
        typedef std::vector< VertexList > ClosedMatrix;

        Path p;
        ClosedMatrix closed(num_vertices(g), VertexList());
        Vertex null = graph_traits< Graph >::null_vertex();

        // each path investigation starts at the ith vertex
        p.push_back(v);

        while (1)
        {
            // extend the path until we've reached the end or the
            // maxlen-sized cycle
            Vertex j = null;
            while (((j = detail::extend_path(g, p, closed)) != null)
                && (p.size() < maxlen))
                ; // empty loop

            // if we're done extending the path and there's an edge
            // connecting the back to the front, then we should have
            // a cycle.
            if (detail::can_wrap_path(g, p) && p.size() >= minlen)
            {
                vis.cycle(p, g);
            }

            if (!detail::exhaust_paths(g, p, closed))
            {
                break;
            }
        }
    }

    // Select the minimum allowable length of a cycle based on the directedness
    // of the graph - 2 for directed, 3 for undirected.
    template < typename D > struct min_cycles
    {
        enum
        {
            value = 2
        };
    };
    template <> struct min_cycles< undirected_tag >
    {
        enum
        {
            value = 3
        };
    };
} /* namespace detail */

template < typename Graph, typename Visitor >
inline void tiernan_all_cycles(
    const Graph& g, Visitor vis, std::size_t minlen, std::size_t maxlen)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;

    VertexIterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        detail::all_cycles_from_vertex(g, *i, vis, minlen, maxlen);
    }
}

template < typename Graph, typename Visitor >
inline void tiernan_all_cycles(const Graph& g, Visitor vis, std::size_t maxlen)
{
    typedef typename graph_traits< Graph >::directed_category Dir;
    tiernan_all_cycles(g, vis, detail::min_cycles< Dir >::value, maxlen);
}

template < typename Graph, typename Visitor >
inline void tiernan_all_cycles(const Graph& g, Visitor vis)
{
    typedef typename graph_traits< Graph >::directed_category Dir;
    tiernan_all_cycles(g, vis, detail::min_cycles< Dir >::value,
        (std::numeric_limits< std::size_t >::max)());
}

template < typename Graph >
inline std::pair< std::size_t, std::size_t > tiernan_girth_and_circumference(
    const Graph& g)
{
    std::size_t min_ = (std::numeric_limits< std::size_t >::max)(), max_ = 0;
    tiernan_all_cycles(g, find_min_max_cycle(min_, max_));

    // if this is the case, the graph is acyclic...
    if (max_ == 0)
        max_ = min_;

    return std::make_pair(min_, max_);
}

template < typename Graph > inline std::size_t tiernan_girth(const Graph& g)
{
    return tiernan_girth_and_circumference(g).first;
}

template < typename Graph >
inline std::size_t tiernan_circumference(const Graph& g)
{
    return tiernan_girth_and_circumference(g).second;
}

} /* namespace boost */

#endif

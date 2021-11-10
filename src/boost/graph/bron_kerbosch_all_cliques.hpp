// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_CLIQUE_HPP
#define BOOST_GRAPH_CLIQUE_HPP

#include <vector>
#include <deque>
#include <boost/config.hpp>

#include <boost/concept/assert.hpp>

#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/lookup_edge.hpp>

#include <boost/concept/detail/concept_def.hpp>
namespace boost
{
namespace concepts
{
    BOOST_concept(CliqueVisitor, (Visitor)(Clique)(Graph))
    {
        BOOST_CONCEPT_USAGE(CliqueVisitor) { vis.clique(k, g); }

    private:
        Visitor vis;
        Graph g;
        Clique k;
    };
} /* namespace concepts */
using concepts::CliqueVisitorConcept;
} /* namespace boost */
#include <boost/concept/detail/concept_undef.hpp>

namespace boost
{
// The algorithm implemented in this paper is based on the so-called
// Algorithm 457, published as:
//
//     @article{362367,
//         author = {Coen Bron and Joep Kerbosch},
//         title = {Algorithm 457: finding all cliques of an undirected graph},
//         journal = {Communications of the ACM},
//         volume = {16},
//         number = {9},
//         year = {1973},
//         issn = {0001-0782},
//         pages = {575--577},
//         doi = {http://doi.acm.org/10.1145/362342.362367},
//             publisher = {ACM Press},
//             address = {New York, NY, USA},
//         }
//
// Sort of. This implementation is adapted from the 1st version of the
// algorithm and does not implement the candidate selection optimization
// described as published - it could, it just doesn't yet.
//
// The algorithm is given as proportional to (3.14)^(n/3) power. This is
// not the same as O(...), but based on time measures and approximation.
//
// Unfortunately, this implementation may be less efficient on non-
// AdjacencyMatrix modeled graphs due to the non-constant implementation
// of the edge(u,v,g) functions.
//
// TODO: It might be worthwhile to provide functionality for passing
// a connectivity matrix to improve the efficiency of those lookups
// when needed. This could simply be passed as a BooleanMatrix
// s.t. edge(u,v,B) returns true or false. This could easily be
// abstracted for adjacency matricies.
//
// The following paper is interesting for a number of reasons. First,
// it lists a number of other such algorithms and second, it describes
// a new algorithm (that does not appear to require the edge(u,v,g)
// function and appears fairly efficient. It is probably worth investigating.
//
//      @article{DBLP:journals/tcs/TomitaTT06,
//          author = {Etsuji Tomita and Akira Tanaka and Haruhisa Takahashi},
//          title = {The worst-case time complexity for generating all maximal
//          cliques and computational experiments}, journal = {Theor. Comput.
//          Sci.}, volume = {363}, number = {1}, year = {2006}, pages = {28-42}
//          ee = {https://doi.org/10.1016/j.tcs.2006.06.015}
//      }

/**
 * The default clique_visitor supplies an empty visitation function.
 */
struct clique_visitor
{
    template < typename VertexSet, typename Graph >
    void clique(const VertexSet&, Graph&)
    {
    }
};

/**
 * The max_clique_visitor records the size of the maximum clique (but not the
 * clique itself).
 */
struct max_clique_visitor
{
    max_clique_visitor(std::size_t& max) : maximum(max) {}

    template < typename Clique, typename Graph >
    inline void clique(const Clique& p, const Graph& g)
    {
        BOOST_USING_STD_MAX();
        maximum = max BOOST_PREVENT_MACRO_SUBSTITUTION(maximum, p.size());
    }
    std::size_t& maximum;
};

inline max_clique_visitor find_max_clique(std::size_t& max)
{
    return max_clique_visitor(max);
}

namespace detail
{
    template < typename Graph >
    inline bool is_connected_to_clique(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor u,
        typename graph_traits< Graph >::vertex_descriptor v,
        typename graph_traits< Graph >::undirected_category)
    {
        return lookup_edge(u, v, g).second;
    }

    template < typename Graph >
    inline bool is_connected_to_clique(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor u,
        typename graph_traits< Graph >::vertex_descriptor v,
        typename graph_traits< Graph >::directed_category)
    {
        // Note that this could alternate between using an || to determine
        // full connectivity. I believe that this should produce strongly
        // connected components. Note that using && instead of || will
        // change the results to a fully connected subgraph (i.e., symmetric
        // edges between all vertices s.t., if a->b, then b->a.
        return lookup_edge(u, v, g).second && lookup_edge(v, u, g).second;
    }

    template < typename Graph, typename Container >
    inline void filter_unconnected_vertices(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor v,
        const Container& in, Container& out)
    {
        BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));

        typename graph_traits< Graph >::directed_category cat;
        typename Container::const_iterator i, end = in.end();
        for (i = in.begin(); i != end; ++i)
        {
            if (is_connected_to_clique(g, v, *i, cat))
            {
                out.push_back(*i);
            }
        }
    }

    template < typename Graph,
        typename Clique, // compsub type
        typename Container, // candidates/not type
        typename Visitor >
    void extend_clique(const Graph& g, Clique& clique, Container& cands,
        Container& nots, Visitor vis, std::size_t min)
    {
        BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
        BOOST_CONCEPT_ASSERT((CliqueVisitorConcept< Visitor, Clique, Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

        // Is there vertex in nots that is connected to all vertices
        // in the candidate set? If so, no clique can ever be found.
        // This could be broken out into a separate function.
        {
            typename Container::iterator ni, nend = nots.end();
            typename Container::iterator ci, cend = cands.end();
            for (ni = nots.begin(); ni != nend; ++ni)
            {
                for (ci = cands.begin(); ci != cend; ++ci)
                {
                    // if we don't find an edge, then we're okay.
                    if (!lookup_edge(*ni, *ci, g).second)
                        break;
                }
                // if we iterated all the way to the end, then *ni
                // is connected to all *ci
                if (ci == cend)
                    break;
            }
            // if we broke early, we found *ni connected to all *ci
            if (ni != nend)
                return;
        }

        // TODO: the original algorithm 457 describes an alternative
        // (albeit really complicated) mechanism for selecting candidates.
        // The given optimizaiton seeks to bring about the above
        // condition sooner (i.e., there is a vertex in the not set
        // that is connected to all candidates). unfortunately, the
        // method they give for doing this is fairly unclear.

        // basically, for every vertex in not, we should know how many
        // vertices it is disconnected from in the candidate set. if
        // we fix some vertex in the not set, then we want to keep
        // choosing vertices that are not connected to that fixed vertex.
        // apparently, by selecting fix point with the minimum number
        // of disconnections (i.e., the maximum number of connections
        // within the candidate set), then the previous condition wil
        // be reached sooner.

        // there's some other stuff about using the number of disconnects
        // as a counter, but i'm jot really sure i followed it.

        // TODO: If we min-sized cliques to visit, then theoretically, we
        // should be able to stop recursing if the clique falls below that
        // size - maybe?

        // otherwise, iterate over candidates and and test
        // for maxmimal cliquiness.
        typename Container::iterator i, j;
        for (i = cands.begin(); i != cands.end();)
        {
            Vertex candidate = *i;

            // add the candidate to the clique (keeping the iterator!)
            // typename Clique::iterator ci = clique.insert(clique.end(),
            // candidate);
            clique.push_back(candidate);

            // remove it from the candidate set
            i = cands.erase(i);

            // build new candidate and not sets by removing all vertices
            // that are not connected to the current candidate vertex.
            // these actually invert the operation, adding them to the new
            // sets if the vertices are connected. its semantically the same.
            Container new_cands, new_nots;
            filter_unconnected_vertices(g, candidate, cands, new_cands);
            filter_unconnected_vertices(g, candidate, nots, new_nots);

            if (new_cands.empty() && new_nots.empty())
            {
                // our current clique is maximal since there's nothing
                // that's connected that we haven't already visited. If
                // the clique is below our radar, then we won't visit it.
                if (clique.size() >= min)
                {
                    vis.clique(clique, g);
                }
            }
            else
            {
                // recurse to explore the new candidates
                extend_clique(g, clique, new_cands, new_nots, vis, min);
            }

            // we're done with this vertex, so we need to move it
            // to the nots, and remove the candidate from the clique.
            nots.push_back(candidate);
            clique.pop_back();
        }
    }
} /* namespace detail */

template < typename Graph, typename Visitor >
inline void bron_kerbosch_all_cliques(
    const Graph& g, Visitor vis, std::size_t min)
{
    BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< Graph >));
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    BOOST_CONCEPT_ASSERT(
        (AdjacencyMatrixConcept< Graph >)); // Structural requirement only
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    typedef std::vector< Vertex > VertexSet;
    typedef std::deque< Vertex > Clique;
    BOOST_CONCEPT_ASSERT((CliqueVisitorConcept< Visitor, Clique, Graph >));

    // NOTE: We're using a deque to implement the clique, because it provides
    // constant inserts and removals at the end and also a constant size.

    VertexIterator i, end;
    boost::tie(i, end) = vertices(g);
    VertexSet cands(i, end); // start with all vertices as candidates
    VertexSet nots; // start with no vertices visited

    Clique clique; // the first clique is an empty vertex set
    detail::extend_clique(g, clique, cands, nots, vis, min);
}

// NOTE: By default the minimum number of vertices per clique is set at 2
// because singleton cliques aren't really very interesting.
template < typename Graph, typename Visitor >
inline void bron_kerbosch_all_cliques(const Graph& g, Visitor vis)
{
    bron_kerbosch_all_cliques(g, vis, 2);
}

template < typename Graph >
inline std::size_t bron_kerbosch_clique_number(const Graph& g)
{
    std::size_t ret = 0;
    bron_kerbosch_all_cliques(g, find_max_clique(ret));
    return ret;
}

} /* namespace boost */

#endif

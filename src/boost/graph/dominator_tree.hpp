//=======================================================================
// Copyright (C) 2005-2009 Jongsoo Park <jongsoo.park -at- gmail.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_DOMINATOR_HPP
#define BOOST_GRAPH_DOMINATOR_HPP

#include <boost/config.hpp>
#include <deque>
#include <set>
#include <boost/graph/depth_first_search.hpp>
#include <boost/concept/assert.hpp>

// Dominator tree computation

namespace boost
{
namespace detail
{
    /**
     * An extended time_stamper which also records vertices for each dfs number
     */
    template < class TimeMap, class VertexVector, class TimeT, class Tag >
    class time_stamper_with_vertex_vector
    : public base_visitor<
          time_stamper_with_vertex_vector< TimeMap, VertexVector, TimeT, Tag > >
    {
    public:
        typedef Tag event_filter;
        time_stamper_with_vertex_vector(
            TimeMap timeMap, VertexVector& v, TimeT& t)
        : timeStamper_(timeMap, t), v_(v)
        {
        }

        template < class Graph >
        void operator()(const typename property_traits< TimeMap >::key_type& v,
            const Graph& g)
        {
            timeStamper_(v, g);
            v_[timeStamper_.m_time] = v;
        }

    private:
        time_stamper< TimeMap, TimeT, Tag > timeStamper_;
        VertexVector& v_;
    };

    /**
     * A convenient way to create a time_stamper_with_vertex_vector
     */
    template < class TimeMap, class VertexVector, class TimeT, class Tag >
    time_stamper_with_vertex_vector< TimeMap, VertexVector, TimeT, Tag >
    stamp_times_with_vertex_vector(
        TimeMap timeMap, VertexVector& v, TimeT& t, Tag)
    {
        return time_stamper_with_vertex_vector< TimeMap, VertexVector, TimeT,
            Tag >(timeMap, v, t);
    }

    template < class Graph, class IndexMap, class TimeMap, class PredMap,
        class DomTreePredMap >
    class dominator_visitor
    {
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef
            typename graph_traits< Graph >::vertices_size_type VerticesSizeType;

    public:
        /**
         * @param g [in] the target graph of the dominator tree
         * @param entry [in] the entry node of g
         * @param indexMap [in] the vertex index map for g
         * @param domTreePredMap [out] the immediate dominator map
         *                             (parent map in dominator tree)
         */
        dominator_visitor(const Graph& g, const Vertex& entry,
            const IndexMap& indexMap, DomTreePredMap domTreePredMap)
        : semi_(num_vertices(g))
        , ancestor_(num_vertices(g), graph_traits< Graph >::null_vertex())
        , samedom_(ancestor_)
        , best_(semi_)
        , semiMap_(make_iterator_property_map(semi_.begin(), indexMap))
        , ancestorMap_(make_iterator_property_map(ancestor_.begin(), indexMap))
        , bestMap_(make_iterator_property_map(best_.begin(), indexMap))
        , buckets_(num_vertices(g))
        , bucketMap_(make_iterator_property_map(buckets_.begin(), indexMap))
        , entry_(entry)
        , domTreePredMap_(domTreePredMap)
        , numOfVertices_(num_vertices(g))
        , samedomMap(make_iterator_property_map(samedom_.begin(), indexMap))
        {
        }

        void operator()(const Vertex& n, const TimeMap& dfnumMap,
            const PredMap& parentMap, const Graph& g)
        {
            if (n == entry_)
                return;

            const Vertex p(get(parentMap, n));
            Vertex s(p);

            // 1. Calculate the semidominator of n,
            // based on the semidominator thm.
            // * Semidominator thm. : To find the semidominator of a node n,
            //   consider all predecessors v of n in the CFG (Control Flow
            //   Graph).
            //  - If v is a proper ancestor of n in the spanning tree
            //    (so dfnum(v) < dfnum(n)), then v is a candidate for semi(n)
            //  - If v is a non-ancestor of n (so dfnum(v) > dfnum(n))
            //    then for each u that is an ancestor of v (or u = v),
            //    Let semi(u) be a candidate for semi(n)
            //   of all these candidates, the one with lowest dfnum is
            //   the semidominator of n.

            // For each predecessor of n
            typename graph_traits< Graph >::in_edge_iterator inItr, inEnd;
            for (boost::tie(inItr, inEnd) = in_edges(n, g); inItr != inEnd;
                 ++inItr)
            {
                const Vertex v = source(*inItr, g);
                // To deal with unreachable nodes
                if (get(dfnumMap, v) < 0 || get(dfnumMap, v) >= numOfVertices_)
                    continue;

                Vertex s2;
                if (get(dfnumMap, v) <= get(dfnumMap, n))
                    s2 = v;
                else
                    s2 = get(semiMap_, ancestor_with_lowest_semi_(v, dfnumMap));

                if (get(dfnumMap, s2) < get(dfnumMap, s))
                    s = s2;
            }
            put(semiMap_, n, s);

            // 2. Calculation of n's dominator is deferred until
            // the path from s to n has been linked into the forest
            get(bucketMap_, s).push_back(n);
            get(ancestorMap_, n) = p;
            get(bestMap_, n) = n;

            // 3. Now that the path from p to v has been linked into
            // the spanning forest, these lines calculate the dominator of v,
            // based on the dominator thm., or else defer the calculation
            // until y's dominator is known
            // * Dominator thm. : On the spanning-tree path below semi(n) and
            //   above or including n, let y be the node
            //   with the smallest-numbered semidominator. Then,
            //
            //  idom(n) = semi(n) if semi(y)=semi(n) or
            //            idom(y) if semi(y) != semi(n)
            typename std::deque< Vertex >::iterator buckItr;
            for (buckItr = get(bucketMap_, p).begin();
                 buckItr != get(bucketMap_, p).end(); ++buckItr)
            {
                const Vertex v(*buckItr);
                const Vertex y(ancestor_with_lowest_semi_(v, dfnumMap));
                if (get(semiMap_, y) == get(semiMap_, v))
                    put(domTreePredMap_, v, p);
                else
                    put(samedomMap, v, y);
            }

            get(bucketMap_, p).clear();
        }

    protected:
        /**
         * Evaluate function in Tarjan's path compression
         */
        const Vertex ancestor_with_lowest_semi_(
            const Vertex& v, const TimeMap& dfnumMap)
        {
            const Vertex a(get(ancestorMap_, v));

            if (get(ancestorMap_, a) != graph_traits< Graph >::null_vertex())
            {
                const Vertex b(ancestor_with_lowest_semi_(a, dfnumMap));

                put(ancestorMap_, v, get(ancestorMap_, a));

                if (get(dfnumMap, get(semiMap_, b))
                    < get(dfnumMap, get(semiMap_, get(bestMap_, v))))
                    put(bestMap_, v, b);
            }

            return get(bestMap_, v);
        }

        std::vector< Vertex > semi_, ancestor_, samedom_, best_;
        PredMap semiMap_, ancestorMap_, bestMap_;
        std::vector< std::deque< Vertex > > buckets_;

        iterator_property_map<
            typename std::vector< std::deque< Vertex > >::iterator, IndexMap >
            bucketMap_;

        const Vertex& entry_;
        DomTreePredMap domTreePredMap_;
        const VerticesSizeType numOfVertices_;

    public:
        PredMap samedomMap;
    };

} // namespace detail

/**
 * @brief Build dominator tree using Lengauer-Tarjan algorithm.
 *                It takes O((V+E)log(V+E)) time.
 *
 * @pre dfnumMap, parentMap and verticesByDFNum have dfs results corresponding
 *      indexMap.
 *      If dfs has already run before,
 *      this function would be good for saving computations.
 * @pre Unreachable nodes must be masked as
 *      graph_traits<Graph>::null_vertex in parentMap.
 * @pre Unreachable nodes must be masked as
 *      (std::numeric_limits<VerticesSizeType>::max)() in dfnumMap.
 *
 * @param domTreePredMap [out] : immediate dominator map (parent map
 * in dom. tree)
 *
 * @note reference Appel. p. 452~453. algorithm 19.9, 19.10.
 *
 * @todo : Optimization in Finding Dominators in Practice, Loukas Georgiadis
 */
template < class Graph, class IndexMap, class TimeMap, class PredMap,
    class VertexVector, class DomTreePredMap >
void lengauer_tarjan_dominator_tree_without_dfs(const Graph& g,
    const typename graph_traits< Graph >::vertex_descriptor& entry,
    const IndexMap& indexMap, TimeMap dfnumMap, PredMap parentMap,
    VertexVector& verticesByDFNum, DomTreePredMap domTreePredMap)
{
    // Typedefs and concept check
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertices_size_type VerticesSizeType;

    BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< Graph >));

    const VerticesSizeType numOfVertices = num_vertices(g);
    if (numOfVertices == 0)
        return;

    // 1. Visit each vertex in reverse post order and calculate sdom.
    detail::dominator_visitor< Graph, IndexMap, TimeMap, PredMap,
        DomTreePredMap >
        visitor(g, entry, indexMap, domTreePredMap);

    VerticesSizeType i;
    for (i = 0; i < numOfVertices; ++i)
    {
        const Vertex u(verticesByDFNum[numOfVertices - 1 - i]);
        if (u != graph_traits< Graph >::null_vertex())
            visitor(u, dfnumMap, parentMap, g);
    }

    // 2. Now all the deferred dominator calculations,
    // based on the second clause of the dominator thm., are performed
    for (i = 0; i < numOfVertices; ++i)
    {
        const Vertex n(verticesByDFNum[i]);

        if (n == entry || n == graph_traits< Graph >::null_vertex())
            continue;

        Vertex u = get(visitor.samedomMap, n);
        if (u != graph_traits< Graph >::null_vertex())
        {
            put(domTreePredMap, n, get(domTreePredMap, u));
        }
    }
}

/**
 * Unlike lengauer_tarjan_dominator_tree_without_dfs,
 * dfs is run in this function and
 * the result is written to dfnumMap, parentMap, vertices.
 *
 * If the result of dfs required after this algorithm,
 * this function can eliminate the need of rerunning dfs.
 */
template < class Graph, class IndexMap, class TimeMap, class PredMap,
    class VertexVector, class DomTreePredMap >
void lengauer_tarjan_dominator_tree(const Graph& g,
    const typename graph_traits< Graph >::vertex_descriptor& entry,
    const IndexMap& indexMap, TimeMap dfnumMap, PredMap parentMap,
    VertexVector& verticesByDFNum, DomTreePredMap domTreePredMap)
{
    // Typedefs and concept check
    typedef typename graph_traits< Graph >::vertices_size_type VerticesSizeType;

    BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< Graph >));

    // 1. Depth first visit
    const VerticesSizeType numOfVertices = num_vertices(g);
    if (numOfVertices == 0)
        return;

    VerticesSizeType time = (std::numeric_limits< VerticesSizeType >::max)();
    std::vector< default_color_type > colors(
        numOfVertices, color_traits< default_color_type >::white());
    depth_first_visit(g, entry,
        make_dfs_visitor(
            make_pair(record_predecessors(parentMap, on_tree_edge()),
                detail::stamp_times_with_vertex_vector(
                    dfnumMap, verticesByDFNum, time, on_discover_vertex()))),
        make_iterator_property_map(colors.begin(), indexMap));

    // 2. Run main algorithm.
    lengauer_tarjan_dominator_tree_without_dfs(g, entry, indexMap, dfnumMap,
        parentMap, verticesByDFNum, domTreePredMap);
}

/**
 * Use vertex_index as IndexMap and make dfnumMap, parentMap, verticesByDFNum
 * internally.
 * If we don't need the result of dfs (dfnumMap, parentMap, verticesByDFNum),
 * this function would be more convenient one.
 */
template < class Graph, class DomTreePredMap >
void lengauer_tarjan_dominator_tree(const Graph& g,
    const typename graph_traits< Graph >::vertex_descriptor& entry,
    DomTreePredMap domTreePredMap)
{
    // typedefs
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertices_size_type VerticesSizeType;
    typedef typename property_map< Graph, vertex_index_t >::const_type IndexMap;
    typedef iterator_property_map<
        typename std::vector< VerticesSizeType >::iterator, IndexMap >
        TimeMap;
    typedef iterator_property_map< typename std::vector< Vertex >::iterator,
        IndexMap >
        PredMap;

    // Make property maps
    const VerticesSizeType numOfVertices = num_vertices(g);
    if (numOfVertices == 0)
        return;

    const IndexMap indexMap = get(vertex_index, g);

    std::vector< VerticesSizeType > dfnum(numOfVertices, 0);
    TimeMap dfnumMap(make_iterator_property_map(dfnum.begin(), indexMap));

    std::vector< Vertex > parent(
        numOfVertices, graph_traits< Graph >::null_vertex());
    PredMap parentMap(make_iterator_property_map(parent.begin(), indexMap));

    std::vector< Vertex > verticesByDFNum(parent);

    // Run main algorithm
    lengauer_tarjan_dominator_tree(g, entry, indexMap, dfnumMap, parentMap,
        verticesByDFNum, domTreePredMap);
}

/**
 * Muchnick. p. 182, 184
 *
 * using iterative bit vector analysis
 */
template < class Graph, class IndexMap, class DomTreePredMap >
void iterative_bit_vector_dominator_tree(const Graph& g,
    const typename graph_traits< Graph >::vertex_descriptor& entry,
    const IndexMap& indexMap, DomTreePredMap domTreePredMap)
{
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator vertexItr;
    typedef typename graph_traits< Graph >::vertices_size_type VerticesSizeType;
    typedef iterator_property_map<
        typename std::vector< std::set< Vertex > >::iterator, IndexMap >
        vertexSetMap;

    BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< Graph >));

    // 1. Finding dominator
    // 1.1. Initialize
    const VerticesSizeType numOfVertices = num_vertices(g);
    if (numOfVertices == 0)
        return;

    vertexItr vi, viend;
    boost::tie(vi, viend) = vertices(g);
    const std::set< Vertex > N(vi, viend);

    bool change = true;

    std::vector< std::set< Vertex > > dom(numOfVertices, N);
    vertexSetMap domMap(make_iterator_property_map(dom.begin(), indexMap));
    get(domMap, entry).clear();
    get(domMap, entry).insert(entry);

    while (change)
    {
        change = false;
        for (boost::tie(vi, viend) = vertices(g); vi != viend; ++vi)
        {
            if (*vi == entry)
                continue;

            std::set< Vertex > T(N);

            typename graph_traits< Graph >::in_edge_iterator inItr, inEnd;
            for (boost::tie(inItr, inEnd) = in_edges(*vi, g); inItr != inEnd;
                 ++inItr)
            {
                const Vertex p = source(*inItr, g);

                std::set< Vertex > tempSet;
                std::set_intersection(T.begin(), T.end(),
                    get(domMap, p).begin(), get(domMap, p).end(),
                    std::inserter(tempSet, tempSet.begin()));
                T.swap(tempSet);
            }

            T.insert(*vi);
            if (T != get(domMap, *vi))
            {
                change = true;
                get(domMap, *vi).swap(T);
            }
        } // end of for (boost::tie(vi, viend) = vertices(g)
    } // end of while(change)

    // 2. Build dominator tree
    for (boost::tie(vi, viend) = vertices(g); vi != viend; ++vi)
        get(domMap, *vi).erase(*vi);

    Graph domTree(numOfVertices);

    for (boost::tie(vi, viend) = vertices(g); vi != viend; ++vi)
    {
        if (*vi == entry)
            continue;

        // We have to iterate through copied dominator set
        const std::set< Vertex > tempSet(get(domMap, *vi));
        typename std::set< Vertex >::const_iterator s;
        for (s = tempSet.begin(); s != tempSet.end(); ++s)
        {
            typename std::set< Vertex >::iterator t;
            for (t = get(domMap, *vi).begin(); t != get(domMap, *vi).end();)
            {
                typename std::set< Vertex >::iterator old_t = t;
                ++t; // Done early because t may become invalid
                if (*old_t == *s)
                    continue;
                if (get(domMap, *s).find(*old_t) != get(domMap, *s).end())
                    get(domMap, *vi).erase(old_t);
            }
        }
    }

    for (boost::tie(vi, viend) = vertices(g); vi != viend; ++vi)
    {
        if (*vi != entry && get(domMap, *vi).size() == 1)
        {
            Vertex temp = *get(domMap, *vi).begin();
            put(domTreePredMap, *vi, temp);
        }
    }
}

template < class Graph, class DomTreePredMap >
void iterative_bit_vector_dominator_tree(const Graph& g,
    const typename graph_traits< Graph >::vertex_descriptor& entry,
    DomTreePredMap domTreePredMap)
{
    typename property_map< Graph, vertex_index_t >::const_type indexMap
        = get(vertex_index, g);

    iterative_bit_vector_dominator_tree(g, entry, indexMap, domTreePredMap);
}
} // namespace boost

#endif // BOOST_GRAPH_DOMINATOR_HPP

// (C) Copyright 2007 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_DETAIL_GEODESIC_HPP
#define BOOST_GRAPH_DETAIL_GEODESIC_HPP

#include <functional>
#include <boost/config.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/numeric_values.hpp>
#include <boost/concept/assert.hpp>

// TODO: Should this really be in detail?

namespace boost
{
// This is a very good discussion on centrality measures. While I can't
// say that this has been the motivating factor for the design and
// implementation of ths centrality framework, it does provide a single
// point of reference for defining things like degree and closeness
// centrality. Plus, the bibliography seems fairly complete.
//
//     @article{citeulike:1144245,
//         author = {Borgatti, Stephen  P. and Everett, Martin  G.},
//         citeulike-article-id = {1144245},
//         doi = {10.1016/j.socnet.2005.11.005},
//         journal = {Social Networks},
//         month = {October},
//         number = {4},
//         pages = {466--484},
//         priority = {0},
//         title = {A Graph-theoretic perspective on centrality},
//         url = {https://doi.org/10.1016/j.socnet.2005.11.005},
//             volume = {28},
//             year = {2006}
//         }
//     }

namespace detail
{
    // Note that this assumes T == property_traits<DistanceMap>::value_type
    // and that the args and return of combine are also T.
    template < typename Graph, typename DistanceMap, typename Combinator,
        typename Distance >
    inline Distance combine_distances(
        const Graph& g, DistanceMap dist, Combinator combine, Distance init)
    {
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
        BOOST_CONCEPT_ASSERT(
            (ReadablePropertyMapConcept< DistanceMap, Vertex >));
        BOOST_CONCEPT_ASSERT((NumericValueConcept< Distance >));
        typedef numeric_values< Distance > DistanceNumbers;
        BOOST_CONCEPT_ASSERT((AdaptableBinaryFunction< Combinator, Distance,
            Distance, Distance >));

        // If there's ever an infinite distance, then we simply return
        // infinity. Note that this /will/ include the a non-zero
        // distance-to-self in the combined values. However, this is usually
        // zero, so it shouldn't be too problematic.
        Distance ret = init;
        VertexIterator i, end;
        for (boost::tie(i, end) = vertices(g); i != end; ++i)
        {
            Vertex v = *i;
            if (get(dist, v) != DistanceNumbers::infinity())
            {
                ret = combine(ret, get(dist, v));
            }
            else
            {
                ret = DistanceNumbers::infinity();
                break;
            }
        }
        return ret;
    }

    // Similar to std::plus<T>, but maximizes parameters
    // rather than adding them.
    template < typename T > struct maximize
    {
        typedef T result_type;
        typedef T first_argument_type;
        typedef T second_argument_type;
        T operator()(T x, T y) const
        {
            BOOST_USING_STD_MAX();
            return max BOOST_PREVENT_MACRO_SUBSTITUTION(x, y);
        }
    };

    // Another helper, like maximize() to help abstract functional
    // concepts. This is trivially instantiated for builtin numeric
    // types, but should be specialized for those types that have
    // discrete notions of reciprocals.
    template < typename T > struct reciprocal
    {
        typedef T result_type;
        typedef T argument_type;
        T operator()(T t) { return T(1) / t; }
    };
} /* namespace detail */

// This type defines the basic facilities used for computing values
// based on the geodesic distances between vertices. Examples include
// closeness centrality and mean geodesic distance.
template < typename Graph, typename DistanceType, typename ResultType >
struct geodesic_measure
{
    typedef DistanceType distance_type;
    typedef ResultType result_type;
    typedef typename graph_traits< Graph >::vertices_size_type size_type;

    typedef numeric_values< distance_type > distance_values;
    typedef numeric_values< result_type > result_values;

    static inline distance_type infinite_distance()
    {
        return distance_values::infinity();
    }

    static inline result_type infinite_result()
    {
        return result_values::infinity();
    }

    static inline result_type zero_result() { return result_values::zero(); }
};

} /* namespace boost */

#endif

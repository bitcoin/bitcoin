// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_ECCENTRICITY_HPP
#define BOOST_GRAPH_ECCENTRICITY_HPP

#include <boost/next_prior.hpp>
#include <boost/config.hpp>
#include <boost/graph/detail/geodesic.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{
template < typename Graph, typename DistanceMap, typename Combinator >
inline typename property_traits< DistanceMap >::value_type eccentricity(
    const Graph& g, DistanceMap dist, Combinator combine)
{
    BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    typedef typename property_traits< DistanceMap >::value_type Distance;

    return detail::combine_distances(g, dist, combine, Distance(0));
}

template < typename Graph, typename DistanceMap >
inline typename property_traits< DistanceMap >::value_type eccentricity(
    const Graph& g, DistanceMap dist)
{
    BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    typedef typename property_traits< DistanceMap >::value_type Distance;

    return eccentricity(g, dist, detail::maximize< Distance >());
}

template < typename Graph, typename DistanceMatrix, typename EccentricityMap >
inline std::pair< typename property_traits< EccentricityMap >::value_type,
    typename property_traits< EccentricityMap >::value_type >
all_eccentricities(
    const Graph& g, const DistanceMatrix& dist, EccentricityMap ecc)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< DistanceMatrix, Vertex >));
    typedef typename property_traits< DistanceMatrix >::value_type DistanceMap;
    BOOST_CONCEPT_ASSERT(
        (WritablePropertyMapConcept< EccentricityMap, Vertex >));
    typedef
        typename property_traits< EccentricityMap >::value_type Eccentricity;
    BOOST_USING_STD_MIN();
    BOOST_USING_STD_MAX();

    Eccentricity r = numeric_values< Eccentricity >::infinity(),
                 d = numeric_values< Eccentricity >::zero();
    VertexIterator i, end;
    boost::tie(i, end) = vertices(g);
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        DistanceMap dm = get(dist, *i);
        Eccentricity e = eccentricity(g, dm);
        put(ecc, *i, e);

        // track the radius and diameter at the same time
        r = min BOOST_PREVENT_MACRO_SUBSTITUTION(r, e);
        d = max BOOST_PREVENT_MACRO_SUBSTITUTION(d, e);
    }
    return std::make_pair(r, d);
}

template < typename Graph, typename EccentricityMap >
inline std::pair< typename property_traits< EccentricityMap >::value_type,
    typename property_traits< EccentricityMap >::value_type >
radius_and_diameter(const Graph& g, EccentricityMap ecc)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< EccentricityMap, Vertex >));
    typedef
        typename property_traits< EccentricityMap >::value_type Eccentricity;
    BOOST_USING_STD_MIN();
    BOOST_USING_STD_MAX();

    VertexIterator i, end;
    boost::tie(i, end) = vertices(g);
    Eccentricity radius = get(ecc, *i);
    Eccentricity diameter = get(ecc, *i);
    for (i = boost::next(i); i != end; ++i)
    {
        Eccentricity cur = get(ecc, *i);
        radius = min BOOST_PREVENT_MACRO_SUBSTITUTION(radius, cur);
        diameter = max BOOST_PREVENT_MACRO_SUBSTITUTION(diameter, cur);
    }
    return std::make_pair(radius, diameter);
}

template < typename Graph, typename EccentricityMap >
inline typename property_traits< EccentricityMap >::value_type radius(
    const Graph& g, EccentricityMap ecc)
{
    return radius_and_diameter(g, ecc).first;
}

template < typename Graph, typename EccentricityMap >
inline typename property_traits< EccentricityMap >::value_type diameter(
    const Graph& g, EccentricityMap ecc)
{
    return radius_and_diameter(g, ecc).second;
}

} /* namespace boost */

#endif

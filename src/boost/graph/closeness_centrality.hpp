// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_CLOSENESS_CENTRALITY_HPP
#define BOOST_GRAPH_CLOSENESS_CENTRALITY_HPP

#include <boost/graph/detail/geodesic.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{
template < typename Graph, typename DistanceType, typename ResultType,
    typename Reciprocal = detail::reciprocal< ResultType > >
struct closeness_measure
: public geodesic_measure< Graph, DistanceType, ResultType >
{
    typedef geodesic_measure< Graph, DistanceType, ResultType > base_type;
    typedef typename base_type::distance_type distance_type;
    typedef typename base_type::result_type result_type;

    result_type operator()(distance_type d, const Graph&)
    {
        BOOST_CONCEPT_ASSERT((NumericValueConcept< DistanceType >));
        BOOST_CONCEPT_ASSERT((NumericValueConcept< ResultType >));
        BOOST_CONCEPT_ASSERT((AdaptableUnaryFunctionConcept< Reciprocal,
            ResultType, ResultType >));
        return (d == base_type::infinite_distance()) ? base_type::zero_result()
                                                     : rec(result_type(d));
    }
    Reciprocal rec;
};

template < typename Graph, typename DistanceMap >
inline closeness_measure< Graph,
    typename property_traits< DistanceMap >::value_type, double,
    detail::reciprocal< double > >
measure_closeness(const Graph&, DistanceMap)
{
    typedef typename property_traits< DistanceMap >::value_type Distance;
    return closeness_measure< Graph, Distance, double,
        detail::reciprocal< double > >();
}

template < typename T, typename Graph, typename DistanceMap >
inline closeness_measure< Graph,
    typename property_traits< DistanceMap >::value_type, T,
    detail::reciprocal< T > >
measure_closeness(const Graph&, DistanceMap)
{
    typedef typename property_traits< DistanceMap >::value_type Distance;
    return closeness_measure< Graph, Distance, T, detail::reciprocal< T > >();
}

template < typename T, typename Graph, typename DistanceMap,
    typename Reciprocal >
inline closeness_measure< Graph,
    typename property_traits< DistanceMap >::value_type, T, Reciprocal >
measure_closeness(const Graph&, DistanceMap)
{
    typedef typename property_traits< DistanceMap >::value_type Distance;
    return closeness_measure< Graph, Distance, T, Reciprocal >();
}

template < typename Graph, typename DistanceMap, typename Measure,
    typename Combinator >
inline typename Measure::result_type closeness_centrality(
    const Graph& g, DistanceMap dist, Measure measure, Combinator combine)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    typedef typename property_traits< DistanceMap >::value_type Distance;
    BOOST_CONCEPT_ASSERT((NumericValueConcept< Distance >));
    BOOST_CONCEPT_ASSERT((DistanceMeasureConcept< Measure, Graph >));

    Distance n = detail::combine_distances(g, dist, combine, Distance(0));
    return measure(n, g);
}

template < typename Graph, typename DistanceMap, typename Measure >
inline typename Measure::result_type closeness_centrality(
    const Graph& g, DistanceMap dist, Measure measure)
{
    BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    typedef typename property_traits< DistanceMap >::value_type Distance;

    return closeness_centrality(g, dist, measure, std::plus< Distance >());
}

template < typename Graph, typename DistanceMap >
inline double closeness_centrality(const Graph& g, DistanceMap dist)
{
    return closeness_centrality(g, dist, measure_closeness(g, dist));
}

template < typename T, typename Graph, typename DistanceMap >
inline T closeness_centrality(const Graph& g, DistanceMap dist)
{
    return closeness_centrality(g, dist, measure_closeness< T >(g, dist));
}

template < typename Graph, typename DistanceMatrixMap, typename CentralityMap,
    typename Measure >
inline void all_closeness_centralities(
    const Graph& g, DistanceMatrixMap dist, CentralityMap cent, Measure measure)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< DistanceMatrixMap, Vertex >));
    typedef
        typename property_traits< DistanceMatrixMap >::value_type DistanceMap;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    BOOST_CONCEPT_ASSERT((WritablePropertyMapConcept< CentralityMap, Vertex >));
    typedef typename property_traits< CentralityMap >::value_type Centrality;

    typename graph_traits< Graph >::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        DistanceMap dm = get(dist, *i);
        Centrality c = closeness_centrality(g, dm, measure);
        put(cent, *i, c);
    }
}

template < typename Graph, typename DistanceMatrixMap, typename CentralityMap >
inline void all_closeness_centralities(
    const Graph& g, DistanceMatrixMap dist, CentralityMap cent)
{
    BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< DistanceMatrixMap, Vertex >));
    typedef
        typename property_traits< DistanceMatrixMap >::value_type DistanceMap;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< DistanceMap, Vertex >));
    typedef typename property_traits< CentralityMap >::value_type Result;

    all_closeness_centralities(
        g, dist, cent, measure_closeness< Result >(g, DistanceMap()));
}

} /* namespace boost */

#endif

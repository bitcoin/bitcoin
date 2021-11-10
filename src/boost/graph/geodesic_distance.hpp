// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_GEODESIC_DISTANCE_HPP
#define BOOST_GRAPH_GEODESIC_DISTANCE_HPP

#include <boost/graph/detail/geodesic.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{
template < typename Graph, typename DistanceType, typename ResultType,
    typename Divides = std::divides< ResultType > >
struct mean_geodesic_measure
: public geodesic_measure< Graph, DistanceType, ResultType >
{
    typedef geodesic_measure< Graph, DistanceType, ResultType > base_type;
    typedef typename base_type::distance_type distance_type;
    typedef typename base_type::result_type result_type;

    result_type operator()(distance_type d, const Graph& g)
    {
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
        BOOST_CONCEPT_ASSERT((NumericValueConcept< DistanceType >));
        BOOST_CONCEPT_ASSERT((NumericValueConcept< ResultType >));
        BOOST_CONCEPT_ASSERT((AdaptableBinaryFunctionConcept< Divides,
            ResultType, ResultType, ResultType >));

        return (d == base_type::infinite_distance())
            ? base_type::infinite_result()
            : div(result_type(d), result_type(num_vertices(g) - 1));
    }
    Divides div;
};

template < typename Graph, typename DistanceMap >
inline mean_geodesic_measure< Graph,
    typename property_traits< DistanceMap >::value_type, double >
measure_mean_geodesic(const Graph&, DistanceMap)
{
    return mean_geodesic_measure< Graph,
        typename property_traits< DistanceMap >::value_type, double >();
}

template < typename T, typename Graph, typename DistanceMap >
inline mean_geodesic_measure< Graph,
    typename property_traits< DistanceMap >::value_type, T >
measure_mean_geodesic(const Graph&, DistanceMap)
{
    return mean_geodesic_measure< Graph,
        typename property_traits< DistanceMap >::value_type, T >();
}

// This is a little different because it's expected that the result type
// should (must?) be the same as the distance type. There's a type of
// transitivity in this thinking... If the average of distances has type
// X then the average of x's should also be type X. Is there a case where this
// is not true?
//
// This type is a little under-genericized... It needs generic parameters
// for addition and division.
template < typename Graph, typename DistanceType >
struct mean_graph_distance_measure
: public geodesic_measure< Graph, DistanceType, DistanceType >
{
    typedef geodesic_measure< Graph, DistanceType, DistanceType > base_type;
    typedef typename base_type::distance_type distance_type;
    typedef typename base_type::result_type result_type;

    inline result_type operator()(distance_type d, const Graph& g)
    {
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
        BOOST_CONCEPT_ASSERT((NumericValueConcept< DistanceType >));

        if (d == base_type::infinite_distance())
        {
            return base_type::infinite_result();
        }
        else
        {
            return d / result_type(num_vertices(g));
        }
    }
};

template < typename Graph, typename DistanceMap >
inline mean_graph_distance_measure< Graph,
    typename property_traits< DistanceMap >::value_type >
measure_graph_mean_geodesic(const Graph&, DistanceMap)
{
    typedef typename property_traits< DistanceMap >::value_type T;
    return mean_graph_distance_measure< Graph, T >();
}

template < typename Graph, typename DistanceMap, typename Measure,
    typename Combinator >
inline typename Measure::result_type mean_geodesic(
    const Graph& g, DistanceMap dist, Measure measure, Combinator combine)
{
    BOOST_CONCEPT_ASSERT((DistanceMeasureConcept< Measure, Graph >));
    typedef typename Measure::distance_type Distance;

    Distance n = detail::combine_distances(g, dist, combine, Distance(0));
    return measure(n, g);
}

template < typename Graph, typename DistanceMap, typename Measure >
inline typename Measure::result_type mean_geodesic(
    const Graph& g, DistanceMap dist, Measure measure)
{
    BOOST_CONCEPT_ASSERT((DistanceMeasureConcept< Measure, Graph >));
    typedef typename Measure::distance_type Distance;

    return mean_geodesic(g, dist, measure, std::plus< Distance >());
}

template < typename Graph, typename DistanceMap >
inline double mean_geodesic(const Graph& g, DistanceMap dist)
{
    return mean_geodesic(g, dist, measure_mean_geodesic(g, dist));
}

template < typename T, typename Graph, typename DistanceMap >
inline T mean_geodesic(const Graph& g, DistanceMap dist)
{
    return mean_geodesic(g, dist, measure_mean_geodesic< T >(g, dist));
}

template < typename Graph, typename DistanceMatrixMap, typename GeodesicMap,
    typename Measure >
inline typename property_traits< GeodesicMap >::value_type all_mean_geodesics(
    const Graph& g, DistanceMatrixMap dist, GeodesicMap geo, Measure measure)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< DistanceMatrixMap, Vertex >));
    typedef
        typename property_traits< DistanceMatrixMap >::value_type DistanceMap;
    BOOST_CONCEPT_ASSERT((DistanceMeasureConcept< Measure, Graph >));
    typedef typename Measure::result_type Result;
    BOOST_CONCEPT_ASSERT((WritablePropertyMapConcept< GeodesicMap, Vertex >));
    BOOST_CONCEPT_ASSERT((NumericValueConcept< Result >));

    // NOTE: We could compute the mean geodesic here by performing additional
    // computations (i.e., adding and dividing). However, I don't really feel
    // like fully genericizing the entire operation yet so I'm not going to.

    Result inf = numeric_values< Result >::infinity();
    Result sum = numeric_values< Result >::zero();
    VertexIterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        DistanceMap dm = get(dist, *i);
        Result r = mean_geodesic(g, dm, measure);
        put(geo, *i, r);

        // compute the sum along with geodesics
        if (r == inf)
        {
            sum = inf;
        }
        else if (sum != inf)
        {
            sum += r;
        }
    }

    // return the average of averages.
    return sum / Result(num_vertices(g));
}

template < typename Graph, typename DistanceMatrixMap, typename GeodesicMap >
inline typename property_traits< GeodesicMap >::value_type all_mean_geodesics(
    const Graph& g, DistanceMatrixMap dist, GeodesicMap geo)
{
    BOOST_CONCEPT_ASSERT((GraphConcept< Graph >));
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< DistanceMatrixMap, Vertex >));
    typedef
        typename property_traits< DistanceMatrixMap >::value_type DistanceMap;
    BOOST_CONCEPT_ASSERT((WritablePropertyMapConcept< GeodesicMap, Vertex >));
    typedef typename property_traits< GeodesicMap >::value_type Result;

    return all_mean_geodesics(
        g, dist, geo, measure_mean_geodesic< Result >(g, DistanceMap()));
}

template < typename Graph, typename GeodesicMap, typename Measure >
inline typename Measure::result_type small_world_distance(
    const Graph& g, GeodesicMap geo, Measure measure)
{
    BOOST_CONCEPT_ASSERT((DistanceMeasureConcept< Measure, Graph >));
    typedef typename Measure::result_type Result;

    Result sum
        = detail::combine_distances(g, geo, std::plus< Result >(), Result(0));
    return measure(sum, g);
}

template < typename Graph, typename GeodesicMap >
inline typename property_traits< GeodesicMap >::value_type small_world_distance(
    const Graph& g, GeodesicMap geo)
{
    return small_world_distance(g, geo, measure_graph_mean_geodesic(g, geo));
}

}

#endif

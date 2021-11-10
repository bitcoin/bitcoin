// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2016 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014, 2017.
// Modifications copyright (c) 2014-2017 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_DETAIL_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_DETAIL_HPP


#include <boost/geometry/strategies/geographic/distance.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace distance
{


/*!
\brief Point-point distance approximation taking flattening into account
\ingroup distance
\tparam Spheroid The reference spheroid model
\tparam CalculationType \tparam_calculation
\author After Andoyer, 19xx, republished 1950, republished by Meeus, 1999
\note Although not so well-known, the approximation is very good: in all cases the results
are about the same as Vincenty. In my (Barend's) testcases the results didn't differ more than 6 m
\see http://nacc.upc.es/tierra/node16.html
\see http://sci.tech-archive.net/Archive/sci.geo.satellite-nav/2004-12/2724.html
\see http://home.att.net/~srschmitt/great_circle_route.html (implementation)
\see http://www.codeguru.com/Cpp/Cpp/algorithms/article.php/c5115 (implementation)
\see http://futureboy.homeip.net/frinksamp/navigation.frink (implementation)
\see http://www.voidware.com/earthdist.htm (implementation)
\see http://www.dtic.mil/docs/citations/AD0627893
\see http://www.dtic.mil/docs/citations/AD703541
*/
template
<
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class andoyer
    : public strategy::distance::geographic
        <
            strategy::andoyer, Spheroid, CalculationType
        >
{
    typedef strategy::distance::geographic
        <
            strategy::andoyer, Spheroid, CalculationType
        > base_type;

public :
    inline andoyer()
        : base_type()
    {}

    explicit inline andoyer(Spheroid const& spheroid)
        : base_type(spheroid)
    {}
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename Spheroid, typename CalculationType>
struct tag<andoyer<Spheroid, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Spheroid, typename CalculationType, typename P1, typename P2>
struct return_type<andoyer<Spheroid, CalculationType>, P1, P2>
    : andoyer<Spheroid, CalculationType>::template calculation_type<P1, P2>
{};


template <typename Spheroid, typename CalculationType>
struct comparable_type<andoyer<Spheroid, CalculationType> >
{
    typedef andoyer<Spheroid, CalculationType> type;
};


template <typename Spheroid, typename CalculationType>
struct get_comparable<andoyer<Spheroid, CalculationType> >
{
    static inline andoyer<Spheroid, CalculationType> apply(andoyer<Spheroid, CalculationType> const& input)
    {
        return input;
    }
};

template <typename Spheroid, typename CalculationType, typename P1, typename P2>
struct result_from_distance<andoyer<Spheroid, CalculationType>, P1, P2>
{
    template <typename T>
    static inline typename return_type<andoyer<Spheroid, CalculationType>, P1, P2>::type
        apply(andoyer<Spheroid, CalculationType> const& , T const& value)
    {
        return value;
    }
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_DETAIL_HPP

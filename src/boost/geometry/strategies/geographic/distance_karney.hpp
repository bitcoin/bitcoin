// Boost.Geometry

// Copyright (c) 2018 Adeel Ahmad, Islamabad, Pakistan.

// Contributed and/or modified by Adeel Ahmad, as part of Google Summer of Code 2018 program.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_KARNEY_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_KARNEY_HPP


#include <boost/geometry/strategies/geographic/distance.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

/*!
\brief The solution of the inverse problem of geodesics on latlong coordinates,
       after Karney (2011).
\ingroup distance
\tparam Spheroid The reference spheroid model
\tparam CalculationType \tparam_calculation
\author See
- Charles F.F Karney, Algorithms for geodesics, 2011
https://arxiv.org/pdf/1109.4448.pdf
*/
template
<
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class karney
    : public strategy::distance::geographic
        <
            strategy::karney, Spheroid, CalculationType
        >
{
    typedef strategy::distance::geographic
        <
            strategy::karney, Spheroid, CalculationType
        > base_type;

public:
    inline karney()
        : base_type()
    {}

    explicit inline karney(Spheroid const& spheroid)
        : base_type(spheroid)
    {}
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename Spheroid, typename CalculationType>
struct tag<karney<Spheroid, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Spheroid, typename CalculationType, typename P1, typename P2>
struct return_type<karney<Spheroid, CalculationType>, P1, P2>
    : karney<Spheroid, CalculationType>::template calculation_type<P1, P2>
{};


template <typename Spheroid, typename CalculationType>
struct comparable_type<karney<Spheroid, CalculationType> >
{
    typedef karney<Spheroid, CalculationType> type;
};


template <typename Spheroid, typename CalculationType>
struct get_comparable<karney<Spheroid, CalculationType> >
{
    static inline karney<Spheroid, CalculationType> apply(karney<Spheroid, CalculationType> const& input)
    {
        return input;
    }
};

template <typename Spheroid, typename CalculationType, typename P1, typename P2>
struct result_from_distance<karney<Spheroid, CalculationType>, P1, P2 >
{
    template <typename T>
    static inline typename return_type<karney<Spheroid, CalculationType>, P1, P2>::type
        apply(karney<Spheroid, CalculationType> const& , T const& value)
    {
        return value;
    }
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_KARNEY_HPP

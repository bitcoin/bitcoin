// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2016 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2018.
// Modifications copyright (c) 2014-2018 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_HPP


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/formulas/andoyer_inverse.hpp>
#include <boost/geometry/formulas/meridian_inverse.hpp>
#include <boost/geometry/formulas/flattening.hpp>

#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/geometries/point_xy.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

/*!
\brief Distance calculation for geographic coordinates on a spheroid
\ingroup strategies
\tparam FormulaPolicy Formula used to calculate azimuths
\tparam Spheroid The spheroid model
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
\* [link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
\* [link geometry.reference.srs.srs_spheroid srs::spheroid]
}
*/
template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
{
public :
    template <typename Point1, typename Point2>
    struct calculation_type
        : promote_floating_point
          <
              typename select_calculation_type
                  <
                      Point1,
                      Point2,
                      CalculationType
                  >::type
          >
    {};

    typedef Spheroid model_type;

    inline geographic()
        : m_spheroid()
    {}

    explicit inline geographic(Spheroid const& spheroid)
        : m_spheroid(spheroid)
    {}

    template <typename CT>
    static inline CT apply(CT lon1, CT lat1, CT lon2, CT lat2,
                           Spheroid const& spheroid)
    {
        typedef typename formula::meridian_inverse
                <
                CT, strategy::default_order<FormulaPolicy>::value
                > meridian_inverse;

        typename meridian_inverse::result res =
                 meridian_inverse::apply(lon1, lat1, lon2, lat2, spheroid);

        if (res.meridian)
        {
            return res.distance;
        }

        return FormulaPolicy::template inverse
                <
                    CT, true, false, false, false, false
                >::apply(lon1, lat1, lon2, lat2, spheroid).distance;
    }

    template <typename Point1, typename Point2>
    inline typename calculation_type<Point1, Point2>::type
    apply(Point1 const& point1, Point2 const& point2) const
    {
        typedef typename calculation_type<Point1, Point2>::type CT;

        CT lon1 = get_as_radian<0>(point1);
        CT lat1 = get_as_radian<1>(point1);
        CT lon2 = get_as_radian<0>(point2);
        CT lat2 = get_as_radian<1>(point2);

        return apply(lon1, lat1, lon2, lat2, m_spheroid);
    }

    inline Spheroid const& model() const
    {
        return m_spheroid;
    }

private :
    Spheroid m_spheroid;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template
<
    typename FormulaPolicy,
    typename Spheroid,
    typename CalculationType
>
struct tag<geographic<FormulaPolicy, Spheroid, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template
<
    typename FormulaPolicy,
    typename Spheroid,
    typename CalculationType,
    typename P1,
    typename P2
>
struct return_type<geographic<FormulaPolicy, Spheroid, CalculationType>, P1, P2>
    : geographic<FormulaPolicy, Spheroid, CalculationType>::template calculation_type<P1, P2>
{};


template
<
    typename FormulaPolicy,
    typename Spheroid,
    typename CalculationType
>
struct comparable_type<geographic<FormulaPolicy, Spheroid, CalculationType> >
{
    typedef geographic<FormulaPolicy, Spheroid, CalculationType> type;
};


template
<
    typename FormulaPolicy,
    typename Spheroid,
    typename CalculationType
>
struct get_comparable<geographic<FormulaPolicy, Spheroid, CalculationType> >
{
    static inline geographic<FormulaPolicy, Spheroid, CalculationType>
        apply(geographic<FormulaPolicy, Spheroid, CalculationType> const& input)
    {
        return input;
    }
};

template
<
    typename FormulaPolicy,
    typename Spheroid,
    typename CalculationType,
    typename P1,
    typename P2
>
struct result_from_distance<geographic<FormulaPolicy, Spheroid, CalculationType>, P1, P2>
{
    template <typename T>
    static inline typename return_type<geographic<FormulaPolicy, Spheroid, CalculationType>, P1, P2>::type
        apply(geographic<FormulaPolicy, Spheroid, CalculationType> const& , T const& value)
    {
        return value;
    }
};


template <typename Point1, typename Point2>
struct default_strategy<point_tag, point_tag, Point1, Point2, geographic_tag, geographic_tag>
{
    typedef strategy::distance::geographic
                <
                    strategy::andoyer,
                    srs::spheroid
                        <
                            typename select_coordinate_type<Point1, Point2>::type
                        >
                > type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_DISTANCE_HPP

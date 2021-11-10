// Boost.Geometry

// Copyright (c) 2018-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_LINE_INTERPOLATE_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_LINE_INTERPOLATE_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/formulas/interpolate_point_spherical.hpp>
#include <boost/geometry/srs/spheroid.hpp>
#include <boost/geometry/strategies/line_interpolate.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace line_interpolate
{


/*!
\brief Interpolate point on a spherical segment.
\ingroup strategies
\tparam CalculationType \tparam_calculation
\tparam DistanceStrategy The underlying point-point distance strategy

\qbk{
[heading See also]
\* [link geometry.reference.algorithms.line_interpolate.line_interpolate_4_with_strategy line_interpolate (with strategy)]
}

 */
template
<
    typename CalculationType = void,
    typename DistanceStrategy = distance::haversine<double, CalculationType>
>
class spherical
{
public:

    typedef typename DistanceStrategy::radius_type radius_type;

    spherical() = default;

    explicit inline spherical(typename DistanceStrategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline spherical(DistanceStrategy const& s)
        : m_strategy(s)
    {}

    template <typename Point, typename Fraction, typename Distance>
    inline void apply(Point const& p0,
                      Point const& p1,
                      Fraction const& fraction,
                      Point & p,
                      Distance const&) const
    {
        typedef typename select_calculation_type_alt
        <
            CalculationType,
            Point
        >::type calc_t;

        formula::interpolate_point_spherical<calc_t> formula;

        calc_t angle01;
        formula.compute_angle(p0, p1, angle01);
        formula.compute_axis(p0, angle01);

        calc_t a = angle01 * fraction;
        formula.compute_point(a, p);
    }

    inline radius_type radius() const
    {
        return m_strategy.radius();
    }

private :
    DistanceStrategy m_strategy;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <>
struct default_strategy<spherical_equatorial_tag>
{
    typedef strategy::line_interpolate::spherical<> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::line_interpolate


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_LINE_INTERPOLATE_HPP

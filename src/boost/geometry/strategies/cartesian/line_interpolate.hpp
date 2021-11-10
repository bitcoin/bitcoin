// Boost.Geometry

// Copyright (c) 2018-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_LINE_INTERPOLATE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_LINE_INTERPOLATE_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/strategies/line_interpolate.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>
#include <boost/geometry/util/algorithm.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace line_interpolate
{


/*!
\brief Interpolate point on a cartesian segment.
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
    typename DistanceStrategy = distance::pythagoras<CalculationType>
>
class cartesian
{
public:
    template <typename Point, typename Fraction, typename Distance>
    inline void apply(Point const& p0,
                      Point const& p1,
                      Fraction const& fraction,
                      Point & p,
                      Distance const&) const
    {
        typedef typename select_calculation_type_alt
            <
                CalculationType, Point
            >::type calc_t;
        typedef typename coordinate_type<Point>::type coord_t;

        //segment convex combination: p0*fraction + p1*(1-fraction)
        Fraction const one_minus_fraction = 1-fraction;
        geometry::detail::for_each_dimension<Point>([&](auto dimension)
        {
            // NOTE: numeric_cast is a leftover from convert, it could probably be ommited.
            // NOTE: the order of points is different than in the formula above
            //       this is also a leftover from the previous implementation
            calc_t coord0 = boost::numeric_cast<calc_t>(get<dimension>(p0));
            calc_t coord1 = boost::numeric_cast<calc_t>(get<dimension>(p1));
            calc_t result = calc_t(coord1 * fraction) + calc_t(coord0 * one_minus_fraction);
            set<dimension>(p, boost::numeric_cast<coord_t>(result));
        });
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <>
struct default_strategy<cartesian_tag>
{
    typedef strategy::line_interpolate::cartesian<> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::line_interpolate


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_LINE_INTERPOLATE_HPP

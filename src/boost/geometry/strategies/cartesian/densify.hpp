// Boost.Geometry

// Copyright (c) 2017-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DENSIFY_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DENSIFY_HPP


#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/densify.hpp>
#include <boost/geometry/util/algorithm.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace densify
{


/*!
\brief Densification of cartesian segment.
\ingroup strategies
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.densify.densify_4_with_strategy densify (with strategy)]
}
 */
template
<
    typename CalculationType = void
>
class cartesian
{
public:
    template <typename Point, typename AssignPolicy, typename T>
    static inline void apply(Point const& p0, Point const& p1, AssignPolicy & policy, T const& length_threshold)
    {
        typedef typename AssignPolicy::point_type out_point_t;
        typedef typename coordinate_type<out_point_t>::type out_coord_t;
        typedef typename select_most_precise
            <
                typename coordinate_type<Point>::type, out_coord_t,
                CalculationType
            >::type calc_t;

        typedef model::point<calc_t, geometry::dimension<Point>::value, cs::cartesian> calc_point_t;
        
        assert_dimension_equal<calc_point_t, out_point_t>();

        calc_point_t cp0, dir01;
        // dir01 = p1 - p0
        geometry::detail::for_each_dimension<calc_point_t>([&](auto index)
        {
            calc_t const coord0 = boost::numeric_cast<calc_t>(get<index>(p0));
            calc_t const coord1 = boost::numeric_cast<calc_t>(get<index>(p1));
            set<index>(cp0, coord0);
            set<index>(dir01, coord1 - coord0);
        });

        calc_t const dot01 = geometry::dot_product(dir01, dir01);
        calc_t const len = math::sqrt(dot01);

        BOOST_GEOMETRY_ASSERT(length_threshold > T(0));

        signed_size_type const n = signed_size_type(len / length_threshold);
        if (n <= 0)
        {
            return;
        }

        calc_t const den = calc_t(n + 1);
        for (signed_size_type i = 0 ; i < n ; ++i)
        {
            out_point_t out;
            
            calc_t const num = calc_t(i + 1);
            geometry::detail::for_each_dimension<out_point_t>([&](auto index)
            {
                // out = p0 + d * dir01
                calc_t const coord = get<index>(cp0) + get<index>(dir01) * num / den;

                set<index>(out, boost::numeric_cast<out_coord_t>(coord));
            });

            policy.apply(out);
        }
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <>
struct default_strategy<cartesian_tag>
{
    typedef strategy::densify::cartesian<> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::densify


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DENSIFY_HPP

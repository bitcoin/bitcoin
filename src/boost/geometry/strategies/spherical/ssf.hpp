// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2016-2021.
// Modifications copyright (c) 2016-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategy/spherical/envelope.hpp>

#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/spherical/disjoint_segment_box.hpp>
//#include <boost/geometry/strategies/concepts/side_concept.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace side
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename T>
int spherical_side_formula(T const& lambda1, T const& delta1,
                           T const& lambda2, T const& delta2,
                           T const& lambda, T const& delta)
{
    // Create temporary points (vectors) on unit a sphere
    T const cos_delta1 = cos(delta1);
    T const c1x = cos_delta1 * cos(lambda1);
    T const c1y = cos_delta1 * sin(lambda1);
    T const c1z = sin(delta1);

    T const cos_delta2 = cos(delta2);
    T const c2x = cos_delta2 * cos(lambda2);
    T const c2y = cos_delta2 * sin(lambda2);
    T const c2z = sin(delta2);

    // (Third point is converted directly)
    T const cos_delta = cos(delta);

    // Apply the "Spherical Side Formula" as presented on my blog
    T const dist
        = (c1y * c2z - c1z * c2y) * cos_delta * cos(lambda)
        + (c1z * c2x - c1x * c2z) * cos_delta * sin(lambda)
        + (c1x * c2y - c1y * c2x) * sin(delta);

    T zero = T();
    return math::equals(dist, zero) ? 0
        : dist > zero ? 1
        : -1; // dist < zero
}

}
#endif // DOXYGEN_NO_DETAIL

/*!
\brief Check at which side of a Great Circle segment a point lies
         left of segment (> 0), right of segment (< 0), on segment (0)
\ingroup strategies
\tparam CalculationType \tparam_calculation
 */
template <typename CalculationType = void>
class spherical_side_formula
{

public :
    typedef spherical_tag cs_tag;

    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef typename promote_floating_point
            <
                typename select_calculation_type_alt
                    <
                        CalculationType,
                        P1, P2, P
                    >::type
            >::type calculation_type;

        calculation_type const lambda1 = get_as_radian<0>(p1);
        calculation_type const delta1 = get_as_radian<1>(p1);
        calculation_type const lambda2 = get_as_radian<0>(p2);
        calculation_type const delta2 = get_as_radian<1>(p2);
        calculation_type const lambda = get_as_radian<0>(p);
        calculation_type const delta = get_as_radian<1>(p);

        return detail::spherical_side_formula(lambda1, delta1,
                                              lambda2, delta2,
                                              lambda, delta);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

/*template <typename CalculationType>
struct default_strategy<spherical_polar_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};*/

template <typename CalculationType>
struct default_strategy<spherical_equatorial_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};

template <typename CalculationType>
struct default_strategy<geographic_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};

}
#endif

}} // namespace strategy::side

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SIDE_BY_CROSS_TRACK_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SIDE_BY_CROSS_TRACK_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/formulas/spherical.hpp>

//#include <boost/geometry/strategies/concepts/side_concept.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

namespace boost { namespace geometry
{


namespace strategy { namespace side
{

/*!
\brief Check at which side of a Great Circle segment a point lies
         left of segment (> 0), right of segment (< 0), on segment (0)
\ingroup strategies
\tparam CalculationType \tparam_calculation
 */
template <typename CalculationType = void>
class side_by_cross_track
{

public :
    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef strategy::within::spherical_point_point
            equals_point_point_strategy_type;
        if (equals_point_point_strategy_type::apply(p, p1)
            || equals_point_point_strategy_type::apply(p, p2)
            || equals_point_point_strategy_type::apply(p1, p2))
        {
            return 0;
        }

        typedef typename promote_floating_point
            <
                typename select_calculation_type_alt
                    <
                        CalculationType,
                        P1, P2, P
                    >::type
            >::type calc_t;

        calc_t d1 = 0.001; // m_strategy.apply(sp1, p);

        calc_t lon1 = geometry::get_as_radian<0>(p1);
        calc_t lat1 = geometry::get_as_radian<1>(p1);
        calc_t lon2 = geometry::get_as_radian<0>(p2);
        calc_t lat2 = geometry::get_as_radian<1>(p2);
        calc_t lon = geometry::get_as_radian<0>(p);
        calc_t lat = geometry::get_as_radian<1>(p);

        calc_t crs_AD = geometry::formula::spherical_azimuth<calc_t, false>
                             (lon1, lat1, lon, lat).azimuth;

        calc_t crs_AB = geometry::formula::spherical_azimuth<calc_t, false>
                             (lon1, lat1, lon2, lat2).azimuth;

        calc_t XTD = asin(sin(d1) * sin(crs_AD - crs_AB));

        return math::equals(XTD, 0) ? 0 : XTD < 0 ? 1 : -1;
    }
};

}} // namespace strategy::side


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SIDE_BY_CROSS_TRACK_HPP

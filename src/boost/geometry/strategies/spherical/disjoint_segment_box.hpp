// Boost.Geometry

// Copyright (c) 2017-2019 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_SEGMENT_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_SEGMENT_BOX_HPP


#include <cstddef>
#include <utility>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/calculation_type.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/disjoint/segment_box.hpp>

// TODO: spherical_point_box currently defined in the same file as cartesian
#include <boost/geometry/strategies/cartesian/point_in_box.hpp>
#include <boost/geometry/strategies/disjoint.hpp>
#include <boost/geometry/strategies/normalize.hpp>
#include <boost/geometry/strategies/spherical/azimuth.hpp>
#include <boost/geometry/strategies/spherical/disjoint_box_box.hpp>


namespace boost { namespace geometry { namespace strategy { namespace disjoint
{

// NOTE: This may be temporary place for this or corresponding strategy
// It seems to be more appropriate to implement the opposite of it
// e.g. intersection::segment_box because in disjoint() algorithm
// other strategies that are used are intersection and covered_by strategies.
struct segment_box_spherical
{
    typedef covered_by::spherical_point_box disjoint_point_box_strategy_type;

    static inline disjoint_point_box_strategy_type get_disjoint_point_box_strategy()
    {
        return disjoint_point_box_strategy_type();
    }

    template <typename Segment, typename Box>
    static inline bool apply(Segment const& segment, Box const& box)
    {
        typedef typename point_type<Segment>::type segment_point_type;
        typedef typename coordinate_type<segment_point_type>::type CT;
        geometry::strategy::azimuth::spherical<CT> azimuth_strategy;

        return geometry::detail::disjoint::disjoint_segment_box_sphere_or_spheroid
                <
                    spherical_equatorial_tag
                >::apply(segment, box,
                         azimuth_strategy,
                         strategy::normalize::spherical_point(),
                         strategy::covered_by::spherical_point_box(),
                         strategy::disjoint::spherical_box_box());
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace services
{

template <typename Linear, typename Box, typename LinearTag>
struct default_strategy<Linear, Box, LinearTag, box_tag, 1, 2,
                        spherical_equatorial_tag, spherical_equatorial_tag>
{
    typedef segment_box_spherical type;
};

template <typename Box, typename Linear, typename LinearTag>
struct default_strategy<Box, Linear, box_tag, LinearTag, 2, 1,
                        spherical_equatorial_tag, spherical_equatorial_tag>
{
    typedef segment_box_spherical type;
};

} // namespace services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}}} // namespace boost::geometry::strategy::disjoint


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_SEGMENT_BOX_HPP


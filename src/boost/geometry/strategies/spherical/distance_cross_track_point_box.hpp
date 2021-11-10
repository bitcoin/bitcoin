// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP


#include <type_traits>

#include <boost/config.hpp>
#include <boost/concept_check.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>
#include <boost/geometry/strategies/spherical/distance_cross_track.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

namespace details
{

template <typename ReturnType>
class cross_track_point_box_generic
{
public :

    template
    <
            typename Point,
            typename Box,
            typename Strategy
    >
    ReturnType static inline apply (Point const& point,
                                    Box const& box,
                                    Strategy ps_strategy)
    {
        // this method assumes that the coordinates of the point and
        // the box are normalized

        typedef typename point_type<Box>::type box_point_type;

        box_point_type bottom_left, bottom_right, top_left, top_right;
        geometry::detail::assign_box_corners(box,
                                             bottom_left, bottom_right,
                                             top_left, top_right);

        ReturnType const plon = geometry::get_as_radian<0>(point);
        ReturnType const plat = geometry::get_as_radian<1>(point);

        ReturnType const lon_min = geometry::get_as_radian<0>(bottom_left);
        ReturnType const lat_min = geometry::get_as_radian<1>(bottom_left);
        ReturnType const lon_max = geometry::get_as_radian<0>(top_right);
        ReturnType const lat_max = geometry::get_as_radian<1>(top_right);

        ReturnType const pi = math::pi<ReturnType>();
        ReturnType const two_pi = math::two_pi<ReturnType>();

        typedef typename point_type<Box>::type box_point_type;

        // First check if the point is within the band defined by the
        // minimum and maximum longitude of the box; if yes, determine
        // if the point is above, below or inside the box and compute
        // the distance (easy in this case)
        //
        // Notice that the point may not be inside the longitude range
        // of the box, but the shifted point may be inside the
        // longitude range of the box; in this case the point is still
        // considered as inside the longitude range band of the box
        if ((plon >= lon_min && plon <= lon_max) || plon + two_pi <= lon_max)
        {
            if (plat > lat_max)
            {
                return geometry::strategy::distance::services::result_from_distance
                        <
                            Strategy, Point, box_point_type
                        >::apply(ps_strategy, ps_strategy
                                 .vertical_or_meridian(plat, lat_max));
            }
            else if (plat < lat_min)
            {
                return geometry::strategy::distance::services::result_from_distance
                        <
                            Strategy, Point, box_point_type
                        >::apply(ps_strategy, ps_strategy
                                 .vertical_or_meridian(lat_min, plat));
            }
            else
            {
                BOOST_GEOMETRY_ASSERT(plat >= lat_min && plat <= lat_max);
                return ReturnType(0);
            }
        }

        // Otherwise determine which among the two medirian segments of the
        // box the point is closest to, and compute the distance of
        // the point to this closest segment

        // Below lon_midway is the longitude of the meridian that:
        // (1) is midway between the meridians of the left and right
        //     meridians of the box, and
        // (2) does not intersect the box
        ReturnType const two = 2.0;
        bool use_left_segment;
        if (lon_max > pi)
        {
            // the box crosses the antimeridian

            // midway longitude = lon_min - (lon_min + (lon_max - 2 * pi)) / 2;
            ReturnType const lon_midway = (lon_min - lon_max) / two + pi;
            BOOST_GEOMETRY_ASSERT(lon_midway >= -pi && lon_midway <= pi);

            use_left_segment = plon > lon_midway;
        }
        else
        {
            // the box does not cross the antimeridian

            ReturnType const lon_sum = lon_min + lon_max;
            if (math::equals(lon_sum, ReturnType(0)))
            {
                // special case: the box is symmetric with respect to
                // the prime meridian; the midway meridian is the antimeridian

                use_left_segment = plon < lon_min;
            }
            else
            {
                // midway long. = lon_min - (2 * pi - (lon_max - lon_min)) / 2;
                ReturnType lon_midway = (lon_min + lon_max) / two - pi;

                // normalize the midway longitude
                if (lon_midway > pi)
                {
                    lon_midway -= two_pi;
                }
                else if (lon_midway < -pi)
                {
                    lon_midway += two_pi;
                }
                BOOST_GEOMETRY_ASSERT(lon_midway >= -pi && lon_midway <= pi);

                // if lon_sum is positive the midway meridian is left
                // of the box, or right of the box otherwise
                use_left_segment = lon_sum > 0
                        ? (plon < lon_min && plon >= lon_midway)
                        : (plon <= lon_max || plon > lon_midway);
            }
        }

        return use_left_segment
                ? ps_strategy.apply(point, bottom_left, top_left)
                : ps_strategy.apply(point, bottom_right, top_right);
    }
};

}  //namespace details

/*!
\brief Strategy functor for distance point to box calculation
\ingroup strategies
\details Class which calculates the distance of a point to a box, for
points and boxes on a sphere or globe
\tparam CalculationType \tparam_calculation
\tparam Strategy underlying point-point distance strategy
\qbk{
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}
*/
template
<
    typename CalculationType = void,
    typename Strategy = haversine<double, CalculationType>
>
class cross_track_point_box
{
public:
    template <typename Point, typename Box>
    struct return_type
        : services::return_type<Strategy, Point, typename point_type<Box>::type>
    {};

    typedef typename Strategy::radius_type radius_type;

    // strategy getters

    // point-segment strategy getters
    struct distance_ps_strategy
    {
        typedef cross_track<CalculationType, Strategy> type;
    };

    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type pp_comparable_strategy;

    typedef std::conditional_t
        <
            std::is_same
                <
                    pp_comparable_strategy,
                    Strategy
                >::value,
            typename strategy::distance::services::comparable_type
                <
                    typename distance_ps_strategy::type
                >::type,
            typename distance_ps_strategy::type
        > ps_strategy_type;

    // constructors

    inline cross_track_point_box()
    {}

    explicit inline cross_track_point_box(typename Strategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline cross_track_point_box(Strategy const& s)
        : m_strategy(s)
    {}


    // methods

    // It might be useful in the future
    // to overload constructor with strategy info.
    // crosstrack(...) {}

    template <typename Point, typename Box>
    inline typename return_type<Point, Box>::type
    apply(Point const& point, Box const& box) const
    {
#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concepts::PointDistanceStrategy
                    <
                        Strategy, Point, typename point_type<Box>::type
                    >)
            );
#endif
        typedef typename return_type<Point, Box>::type return_type;
        return details::cross_track_point_box_generic
                    <return_type>::apply(point, box,
                                         ps_strategy_type(m_strategy));
    }

    inline typename Strategy::radius_type radius() const
    {
        return m_strategy.radius();
    }

private:
    Strategy m_strategy;
};



#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType, typename Strategy>
struct tag<cross_track_point_box<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_box type;
};


template <typename CalculationType, typename Strategy, typename P, typename Box>
struct return_type<cross_track_point_box<CalculationType, Strategy>, P, Box>
    : cross_track_point_box
        <
            CalculationType, Strategy
        >::template return_type<P, Box>
{};


template <typename CalculationType, typename Strategy>
struct comparable_type<cross_track_point_box<CalculationType, Strategy> >
{
    typedef cross_track_point_box
        <
            CalculationType, typename comparable_type<Strategy>::type
        > type;
};


template <typename CalculationType, typename Strategy>
struct get_comparable<cross_track_point_box<CalculationType, Strategy> >
{
    typedef cross_track_point_box<CalculationType, Strategy> this_strategy;
    typedef typename comparable_type<this_strategy>::type comparable_type;

public:
    static inline comparable_type apply(this_strategy const& strategy)
    {
        return comparable_type(strategy.radius());
    }
};


template <typename CalculationType, typename Strategy, typename P, typename Box>
struct result_from_distance
    <
        cross_track_point_box<CalculationType, Strategy>, P, Box
    >
{
private:
    typedef cross_track_point_box<CalculationType, Strategy> this_strategy;

    typedef typename this_strategy::template return_type
        <
            P, Box
        >::type return_type;

public:
    template <typename T>
    static inline return_type apply(this_strategy const& strategy,
                                    T const& distance)
    {
        Strategy s(strategy.radius());

        return result_from_distance
            <
                Strategy, P, typename point_type<Box>::type
            >::apply(s, distance);
    }
};


// define cross_track_point_box<default_point_segment_strategy> as
// default point-box strategy for the spherical equatorial coordinate system
template <typename Point, typename Box, typename Strategy>
struct default_strategy
    <
        point_tag, box_tag, Point, Box,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef cross_track_point_box
        <
            void,
            std::conditional_t
                <
                    std::is_void<Strategy>::value,
                    typename default_strategy
                        <
                            point_tag, point_tag,
                            Point, typename point_type<Box>::type,
                            spherical_equatorial_tag, spherical_equatorial_tag
                        >::type,
                    Strategy
                >
        > type;
};


template <typename Box, typename Point, typename Strategy>
struct default_strategy
    <
        box_tag, point_tag, Box, Point,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef typename default_strategy
        <
            point_tag, box_tag, Point, Box,
            spherical_equatorial_tag, spherical_equatorial_tag,
            Strategy
        >::type type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP

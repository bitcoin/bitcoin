// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2016-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_BOX_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_BOX_BOX_HPP


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
class cross_track_box_box_generic
{
public :

    template <typename Point, typename PPStrategy, typename PSStrategy>
    ReturnType static inline diagonal_case(Point topA,
                                           Point topB,
                                           Point bottomA,
                                           Point bottomB,
                                           bool north_shortest,
                                           bool non_overlap,
                                           PPStrategy pp_strategy,
                                           PSStrategy ps_strategy)
    {
        if (north_shortest && non_overlap)
        {
            return pp_strategy.apply(topA, bottomB);
        }
        if (north_shortest && !non_overlap)
        {
            return ps_strategy.apply(topA, topB, bottomB);
        }
        if (!north_shortest && non_overlap)
        {
            return pp_strategy.apply(bottomA, topB);
        }
        return ps_strategy.apply(bottomA, topB, bottomB);
    }


    template
    <
            typename Box1,
            typename Box2,
            typename PPStrategy,
            typename PSStrategy
    >
    ReturnType static inline apply (Box1 const& box1,
                                    Box2 const& box2,
                                    PPStrategy pp_strategy,
                                    PSStrategy ps_strategy)
    {

        // this method assumes that the coordinates of the point and
        // the box are normalized

        typedef typename point_type<Box1>::type box_point_type1;
        typedef typename point_type<Box2>::type box_point_type2;

        box_point_type1 bottom_left1, bottom_right1, top_left1, top_right1;
        geometry::detail::assign_box_corners(box1,
                                             bottom_left1, bottom_right1,
                                             top_left1, top_right1);

        box_point_type2 bottom_left2, bottom_right2, top_left2, top_right2;
        geometry::detail::assign_box_corners(box2,
                                             bottom_left2, bottom_right2,
                                             top_left2, top_right2);

        ReturnType lon_min1 = geometry::get_as_radian<0>(bottom_left1);
        ReturnType const lat_min1 = geometry::get_as_radian<1>(bottom_left1);
        ReturnType lon_max1 = geometry::get_as_radian<0>(top_right1);
        ReturnType const lat_max1 = geometry::get_as_radian<1>(top_right1);

        ReturnType lon_min2 = geometry::get_as_radian<0>(bottom_left2);
        ReturnType const lat_min2 = geometry::get_as_radian<1>(bottom_left2);
        ReturnType lon_max2 = geometry::get_as_radian<0>(top_right2);
        ReturnType const lat_max2 = geometry::get_as_radian<1>(top_right2);

        ReturnType const two_pi = math::two_pi<ReturnType>();

        // Test which sides of the boxes are closer and if boxes cross
        // antimeridian
        bool right_wrap;

        if (lon_min2 > 0 && lon_max2 < 0) // box2 crosses antimeridian
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(box2 crosses antimeridian)";
#endif
            right_wrap = lon_min2 - lon_max1 < lon_min1 - lon_max2;
            lon_max2 += two_pi;
            if (lon_min1 > 0 && lon_max1 < 0) // both boxes crosses antimeridian
            {
                lon_max1 += two_pi;
            }
        }
        else if (lon_min1 > 0 && lon_max1 < 0) // only box1 crosses antimeridian
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(box1 crosses antimeridian)";
#endif
            return apply(box2, box1, pp_strategy, ps_strategy);
        }
        else
        {
            right_wrap = lon_max1 <= lon_min2
                         ? lon_min2 - lon_max1 < two_pi - (lon_max2 - lon_min1)
                         : lon_min1 - lon_max2 > two_pi - (lon_max1 - lon_min2);

        }

        // Check1: if box2 crosses the band defined by the
        // minimum and maximum longitude of box1; if yes, determine
        // if the box2 is above, below or intersects/is inside box1 and compute
        // the distance (easy in this case)

        bool lon_min12 = lon_min1 <= lon_min2;
        bool right = lon_max1 <= lon_min2;
        bool left = lon_min1 >= lon_max2;
        bool lon_max12 = lon_max1 <= lon_max2;

        if ((lon_min12 && !right)
                || (!left && !lon_max12)
                || (!lon_min12 && lon_max12))
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(up-down)\n";
#endif
            if (lat_min1 > lat_max2)
            {
                return geometry::strategy::distance::services::result_from_distance
                    <
                        PSStrategy, box_point_type1, box_point_type2
                    >::apply(ps_strategy, ps_strategy
                               .vertical_or_meridian(lat_min1, lat_max2));
            }
            else if (lat_max1 < lat_min2)
            {
                return geometry::strategy::distance::services::result_from_distance
                    <
                        PSStrategy, box_point_type1, box_point_type2
                    >::apply(ps_strategy, ps_strategy
                             .vertical_or_meridian(lat_min2, lat_max1));
            }
            else
            {
                //BOOST_GEOMETRY_ASSERT(plat >= lat_min && plat <= lat_max);
                return ReturnType(0);
            }
        }

        // Check2: if box2 is right/left of box1
        // the max lat of box2 should be less than the max lat of box1
        bool bottom_max;

        ReturnType top_common = (std::min)(lat_max1, lat_max2);
        ReturnType bottom_common = (std::max)(lat_min1, lat_min2);

        // true if the closest points are on northern hemisphere
        bool north_shortest = top_common + bottom_common > 0;
        // true if box bands do not overlap
        bool non_overlap = top_common < bottom_common;

        if (north_shortest)
        {
            bottom_max = lat_max1 >= lat_max2;
        }
        else
        {
            bottom_max = lat_min1 <= lat_min2;
        }

#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
        std::cout << "(diagonal)";
#endif
        if (bottom_max && !right_wrap)
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(bottom left)";
#endif
            return diagonal_case(top_right2, top_left1,
                                 bottom_right2, bottom_left1,
                                 north_shortest, non_overlap,
                                 pp_strategy, ps_strategy);
        }
        if (bottom_max && right_wrap)
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(bottom right)";
#endif
            return diagonal_case(top_left2, top_right1,
                                 bottom_left2, bottom_right1,
                                 north_shortest, non_overlap,
                                 pp_strategy, ps_strategy);
        }
        if (!bottom_max && !right_wrap)
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(top left)";
#endif
            return diagonal_case(top_left1, top_right2,
                                 bottom_left1, bottom_right2,
                                 north_shortest, non_overlap,
                                 pp_strategy, ps_strategy);
        }
        if (!bottom_max && right_wrap)
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK_BOX_BOX
            std::cout << "(top right)";
#endif
            return diagonal_case(top_right1, top_left2,
                                 bottom_right1, bottom_left2,
                                 north_shortest, non_overlap,
                                 pp_strategy, ps_strategy);
        }
        return ReturnType(0);
    }
};

} //namespace details

/*!
\brief Strategy functor for distance box to box calculation
\ingroup strategies
\details Class which calculates the distance of a box to a box, for
boxes on a sphere or globe
\tparam CalculationType \tparam_calculation
\tparam Strategy underlying point-segment distance strategy, defaults
to cross track
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
class cross_track_box_box
{
public:
    template <typename Box1, typename Box2>
    struct return_type
        : services::return_type<Strategy,
                                typename point_type<Box1>::type,
                                typename point_type<Box2>::type>
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

    inline cross_track_box_box()
    {}

    explicit inline cross_track_box_box(typename Strategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline cross_track_box_box(Strategy const& s)
        : m_strategy(s)
    {}


    // It might be useful in the future
    // to overload constructor with strategy info.
    // crosstrack(...) {}

    template <typename Box1, typename Box2>
    inline typename return_type<Box1, Box2>::type
    apply(Box1 const& box1, Box2 const& box2) const
    {
#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concepts::PointDistanceStrategy
                    <
                        Strategy,
                        typename point_type<Box1>::type,
                        typename point_type<Box2>::type
                    >)
            );
#endif
        typedef typename return_type<Box1, Box2>::type return_type;
        return details::cross_track_box_box_generic
                                <return_type>::apply(box1, box2,
                                                     m_strategy,
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
struct tag<cross_track_box_box<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_box_box type;
};


template <typename CalculationType, typename Strategy, typename Box1, typename Box2>
struct return_type<cross_track_box_box<CalculationType, Strategy>, Box1, Box2>
    : cross_track_box_box
        <
            CalculationType, Strategy
        >::template return_type<Box1, Box2>
{};


template <typename CalculationType, typename Strategy>
struct comparable_type<cross_track_box_box<CalculationType, Strategy> >
{
    typedef cross_track_box_box
        <
            CalculationType, typename comparable_type<Strategy>::type
        > type;
};


template <typename CalculationType, typename Strategy>
struct get_comparable<cross_track_box_box<CalculationType, Strategy> >
{
    typedef cross_track_box_box<CalculationType, Strategy> this_strategy;
    typedef typename comparable_type<this_strategy>::type comparable_type;

public:
    static inline comparable_type apply(this_strategy const& strategy)
    {
        return comparable_type(strategy.radius());
    }
};


template <typename CalculationType, typename Strategy, typename Box1, typename Box2>
struct result_from_distance
    <
        cross_track_box_box<CalculationType, Strategy>, Box1, Box2
    >
{
private:
    typedef cross_track_box_box<CalculationType, Strategy> this_strategy;

    typedef typename this_strategy::template return_type
        <
            Box1, Box2
        >::type return_type;

public:
    template <typename T>
    static inline return_type apply(this_strategy const& strategy,
                                    T const& distance)
    {
        Strategy s(strategy.radius());

        return result_from_distance
            <
                Strategy,
                typename point_type<Box1>::type,
                typename point_type<Box2>::type
            >::apply(s, distance);
    }
};


// define cross_track_box_box<default_point_segment_strategy> as
// default box-box strategy for the spherical equatorial coordinate system
template <typename Box1, typename Box2, typename Strategy>
struct default_strategy
    <
        box_tag, box_tag, Box1, Box2,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef cross_track_box_box
        <
            void,
            std::conditional_t
                <
                    std::is_void<Strategy>::value,
                    typename default_strategy
                        <
                            point_tag, point_tag,
                            typename point_type<Box1>::type, typename point_type<Box2>::type,
                            spherical_equatorial_tag, spherical_equatorial_tag
                        >::type,
                    Strategy
                >
        > type;
};

} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_BOX_BOX_HPP

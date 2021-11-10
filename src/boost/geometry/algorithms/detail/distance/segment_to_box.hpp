// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SEGMENT_TO_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SEGMENT_TO_BOX_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

#include <boost/core/ignore_unused.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/closest_feature/point_to_range.hpp>
#include <boost/geometry/algorithms/detail/disjoint/segment_box.hpp>
#include <boost/geometry/algorithms/detail/distance/is_comparable.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/detail/dummy_geometries.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/util/calculation_type.hpp>
#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/has_nan_coordinate.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/strategies/disjoint.hpp>
#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/tags.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename Segment, typename Box, typename Strategy>
inline bool intersects_segment_box(Segment const& segment, Box const& box,
                                   Strategy const& strategy)
{
    return ! detail::disjoint::disjoint_segment_box::apply(segment, box, strategy);
}

// TODO: segment_to_box_2D_generic is not used anymore. Remove?

// TODO: Furthermore this utility can potentially use different strategy than
//       the one that was passed into bg::distance() but it seems this is by design.

template
<
    typename Segment,
    typename Box,
    typename Strategies,
    bool UsePointBoxStrategy = false // use only PointSegment strategy
>
class segment_to_box_2D_generic
{
private:
    typedef typename point_type<Segment>::type segment_point;
    typedef typename point_type<Box>::type box_point;

    typedef distance::strategy_t<box_point, Segment, Strategies> ps_strategy_type;

    typedef detail::closest_feature::point_to_point_range
        <
            segment_point,
            std::vector<box_point>,
            open
        > point_to_point_range;
    
public:
    // TODO: Or should the return type be defined by sb_strategy_type?
    typedef distance::return_t<box_point, Segment, Strategies> return_type;

    static inline return_type apply(Segment const& segment,
                                    Box const& box,
                                    Strategies const& strategies,
                                    bool check_intersection = true)
    {
        if (check_intersection && intersects_segment_box(segment, box, strategies))
        {
            return return_type(0);
        }

        // get segment points
        segment_point p[2];
        detail::assign_point_from_index<0>(segment, p[0]);
        detail::assign_point_from_index<1>(segment, p[1]);

        // get box points
        std::vector<box_point> box_points(4);
        detail::assign_box_corners_oriented<true>(box, box_points);
 
        ps_strategy_type const strategy = strategies.distance(dummy_point(), dummy_segment());

        auto const cstrategy = strategy::distance::services::get_comparable
                                <
                                    ps_strategy_type
                                >::apply(strategy);

        distance::creturn_t<box_point, Segment, Strategies> cd[6];
        for (unsigned int i = 0; i < 4; ++i)
        {
            cd[i] = cstrategy.apply(box_points[i], p[0], p[1]);
        }

        std::pair
            <
                typename std::vector<box_point>::const_iterator,
                typename std::vector<box_point>::const_iterator
            > bit_min[2];

        bit_min[0] = point_to_point_range::apply(p[0],
                                                 box_points.begin(),
                                                 box_points.end(),
                                                 cstrategy,
                                                 cd[4]);
        bit_min[1] = point_to_point_range::apply(p[1],
                                                 box_points.begin(),
                                                 box_points.end(),
                                                 cstrategy,
                                                 cd[5]);

        unsigned int imin = 0;
        for (unsigned int i = 1; i < 6; ++i)
        {
            if (cd[i] < cd[imin])
            {
                imin = i;
            }
        }

        if (BOOST_GEOMETRY_CONDITION(is_comparable<ps_strategy_type>::value))
        {
            return cd[imin];
        }

        if (imin < 4)
        {
            return strategy.apply(box_points[imin], p[0], p[1]);
        }
        else
        {
            unsigned int bimin = imin - 4;
            return strategy.apply(p[bimin],
                                  *bit_min[bimin].first,
                                  *bit_min[bimin].second);
        }
    }
};


template
<
    typename Segment,
    typename Box,
    typename Strategies
>
class segment_to_box_2D_generic<Segment, Box, Strategies, true> // Use both PointSegment and PointBox strategies
{
private:
    typedef typename point_type<Segment>::type segment_point;
    typedef typename point_type<Box>::type box_point;

    typedef distance::strategy_t<box_point, Segment, Strategies> ps_strategy_type;
    typedef distance::strategy_t<segment_point, Box, Strategies> pb_strategy_type;

public:
    // TODO: Or should the return type be defined by sb_strategy_type?
    typedef distance::return_t<box_point, Segment, Strategies> return_type;
    
    static inline return_type apply(Segment const& segment,
                                    Box const& box,
                                    Strategies const& strategies,
                                    bool check_intersection = true)
    {
        if (check_intersection && intersects_segment_box(segment, box, strategies))
        {
            return return_type(0);
        }

        // get segment points
        segment_point p[2];
        detail::assign_point_from_index<0>(segment, p[0]);
        detail::assign_point_from_index<1>(segment, p[1]);

        // get box points
        std::vector<box_point> box_points(4);
        detail::assign_box_corners_oriented<true>(box, box_points);

        distance::creturn_t<box_point, Segment, Strategies> cd[6];

        ps_strategy_type ps_strategy = strategies.distance(dummy_point(), dummy_segment());
        auto const ps_cstrategy = strategy::distance::services::get_comparable
                                    <
                                        ps_strategy_type
                                    >::apply(ps_strategy);
        boost::ignore_unused(ps_strategy, ps_cstrategy);

        for (unsigned int i = 0; i < 4; ++i)
        {
            cd[i] = ps_cstrategy.apply(box_points[i], p[0], p[1]);
        }

        pb_strategy_type const pb_strategy = strategies.distance(dummy_point(), dummy_box());
        auto const pb_cstrategy = strategy::distance::services::get_comparable
                                    <
                                        pb_strategy_type
                                    >::apply(pb_strategy);
        boost::ignore_unused(pb_strategy, pb_cstrategy);

        cd[4] = pb_cstrategy.apply(p[0], box);
        cd[5] = pb_cstrategy.apply(p[1], box);

        unsigned int imin = 0;
        for (unsigned int i = 1; i < 6; ++i)
        {
            if (cd[i] < cd[imin])
            {
                imin = i;
            }
        }

        if (imin < 4)
        {
            if (is_comparable<ps_strategy_type>::value)
            {
                return cd[imin];
            }

            return ps_strategy.apply(box_points[imin], p[0], p[1]);
        }
        else
        {
            if (is_comparable<pb_strategy_type>::value)
            {
                return cd[imin];
            }

            return pb_strategy.apply(p[imin - 4], box);
        }
    }
};




template
<
    typename ReturnType,
    typename SegmentPoint,
    typename BoxPoint,
    typename Strategies
>
class segment_to_box_2D
{
private:
    template <typename Result>
    struct cast_to_result
    {
        template <typename T>
        static inline Result apply(T const& t)
        {
            return boost::numeric_cast<Result>(t);
        }
    };


    template <typename T, bool IsLess /* true */>
    struct compare_less_equal
    {
        typedef compare_less_equal<T, !IsLess> other;

        template <typename T1, typename T2>
        inline bool operator()(T1 const& t1, T2 const& t2) const
        {
            return std::less_equal<T>()(cast_to_result<T>::apply(t1),
                                        cast_to_result<T>::apply(t2));
        }
    };

    template <typename T>
    struct compare_less_equal<T, false>
    {
        typedef compare_less_equal<T, true> other;

        template <typename T1, typename T2>
        inline bool operator()(T1 const& t1, T2 const& t2) const
        {
            return std::greater_equal<T>()(cast_to_result<T>::apply(t1),
                                           cast_to_result<T>::apply(t2));
        }
    };


    template <typename LessEqual>
    struct other_compare
    {
        typedef typename LessEqual::other type;
    };


    // it is assumed here that p0 lies to the right of the box (so the
    // entire segment lies to the right of the box)
    template <typename LessEqual>
    struct right_of_box
    {
        static inline ReturnType apply(SegmentPoint const& p0,
                                       SegmentPoint const& p1,
                                       BoxPoint const& bottom_right,
                                       BoxPoint const& top_right,
                                       Strategies const& strategies)
        {
            // the implementation below is written for non-negative slope
            // segments
            //
            // for negative slope segments swap the roles of bottom_right
            // and top_right and use greater_equal instead of less_equal.

            typedef cast_to_result<ReturnType> cast;

            LessEqual less_equal;

            auto const ps_strategy = strategies.distance(dummy_point(), dummy_segment());

            if (less_equal(geometry::get<1>(bottom_right), geometry::get<1>(p0)))
            {
                //if p0 is in box's band
                if (less_equal(geometry::get<1>(p0), geometry::get<1>(top_right)))
                {
                    // segment & crosses band (TODO:merge with box-box dist)
                    if (math::equals(geometry::get<0>(p0), geometry::get<0>(p1)))
                    {
                        SegmentPoint high = geometry::get<1>(p1) > geometry::get<1>(p0) ? p1 : p0;
                        if (less_equal(geometry::get<1>(high), geometry::get<1>(top_right)))
                        {
                            return cast::apply(ps_strategy.apply(high, bottom_right, top_right));
                        }
                        return cast::apply(ps_strategy.apply(top_right, p0, p1));
                    }
                    return cast::apply(ps_strategy.apply(p0, bottom_right, top_right));
                }
                // distance is realized between the top-right
                // corner of the box and the segment
                return cast::apply(ps_strategy.apply(top_right, p0, p1));
            }
            else
            {
                // distance is realized between the bottom-right
                // corner of the box and the segment
                return cast::apply(ps_strategy.apply(bottom_right, p0, p1));
            }
        }
    };

    // it is assumed here that p0 lies above the box (so the
    // entire segment lies above the box)

    template <typename LessEqual>
    struct above_of_box
    {

        static inline ReturnType apply(SegmentPoint const& p0,
                                       SegmentPoint const& p1,
                                       BoxPoint const& top_left,
                                       Strategies const& strategies)
        {
            return apply(p0, p1, p0, top_left, strategies);
        }

        static inline ReturnType apply(SegmentPoint const& p0,
                                       SegmentPoint const& p1,
                                       SegmentPoint const& p_max,
                                       BoxPoint const& top_left,
                                       Strategies const& strategies)
        {
            auto const ps_strategy = strategies.distance(dummy_point(), dummy_segment());

            typedef cast_to_result<ReturnType> cast;
            LessEqual less_equal;

            // p0 is above the upper segment of the box (and inside its band)
            // then compute the vertical (i.e. meridian for spherical) distance
            if (less_equal(geometry::get<0>(top_left), geometry::get<0>(p_max)))
            {
                ReturnType diff = ps_strategy.vertical_or_meridian(
                                    geometry::get_as_radian<1>(p_max),
                                    geometry::get_as_radian<1>(top_left));

                return strategy::distance::services::result_from_distance
                    <
                        std::remove_const_t<decltype(ps_strategy)>,
                        SegmentPoint, BoxPoint
                    >::apply(ps_strategy, math::abs(diff));
            }

            // p0 is to the left of the box, but p1 is above the box
            // in this case the distance is realized between the
            // top-left corner of the box and the segment
            return cast::apply(ps_strategy.apply(top_left, p0, p1));
        }
    };

    template <typename LessEqual>
    struct check_right_left_of_box
    {
        static inline bool apply(SegmentPoint const& p0,
                                 SegmentPoint const& p1,
                                 BoxPoint const& top_left,
                                 BoxPoint const& top_right,
                                 BoxPoint const& bottom_left,
                                 BoxPoint const& bottom_right,
                                 Strategies const& strategies,
                                 ReturnType& result)
        {
            // p0 lies to the right of the box
            if (geometry::get<0>(p0) >= geometry::get<0>(top_right))
            {
                result = right_of_box
                    <
                        LessEqual
                    >::apply(p0, p1, bottom_right, top_right,
                             strategies);
                return true;
            }

            // p1 lies to the left of the box
            if (geometry::get<0>(p1) <= geometry::get<0>(bottom_left))
            {
                result = right_of_box
                    <
                        typename other_compare<LessEqual>::type
                    >::apply(p1, p0, top_left, bottom_left,
                             strategies);
                return true;
            }

            return false;
        }
    };

    template <typename LessEqual>
    struct check_above_below_of_box
    {
        static inline bool apply(SegmentPoint const& p0,
                                 SegmentPoint const& p1,
                                 BoxPoint const& top_left,
                                 BoxPoint const& top_right,
                                 BoxPoint const& bottom_left,
                                 BoxPoint const& bottom_right,
                                 Strategies const& strategies,
                                 ReturnType& result)
        {
            typedef compare_less_equal<ReturnType, false> GreaterEqual;

            // the segment lies below the box
            if (geometry::get<1>(p1) < geometry::get<1>(bottom_left))
            {
                auto const sb_strategy = strategies.distance(dummy_segment(), dummy_box());

                // TODO: this strategy calls this algorithm's again, specifically:
                //       geometry::detail::distance::segment_to_box_2D<>::call_above_of_box
                //       If possible rewrite them to avoid this.
                //       For now just pass umbrella strategy.
                result = sb_strategy.template segment_below_of_box
                        <
                            LessEqual,
                            ReturnType
                        >(p0, p1,
                          top_left, top_right,
                          bottom_left, bottom_right,
                          strategies);
                return true;
            }

            // the segment lies above the box
            if (geometry::get<1>(p0) > geometry::get<1>(top_right))
            {
                result = (std::min)(above_of_box
                                    <
                                        LessEqual
                                    >::apply(p0, p1, top_left, strategies),
                                    above_of_box
                                    <
                                        GreaterEqual
                                    >::apply(p1, p0, top_right, strategies));
                return true;
            }
            return false;
        }
    };

    struct check_generic_position
    {
        static inline bool apply(SegmentPoint const& p0,
                                 SegmentPoint const& p1,
                                 BoxPoint const& corner1,
                                 BoxPoint const& corner2,
                                 Strategies const& strategies,
                                 ReturnType& result)
        {
            auto const side_strategy = strategies.side();
            auto const ps_strategy = strategies.distance(dummy_point(), dummy_segment());

            typedef cast_to_result<ReturnType> cast;
            ReturnType diff1 = cast::apply(geometry::get<1>(p1))
                               - cast::apply(geometry::get<1>(p0));

            int sign = diff1 < 0 ? -1 : 1;
            if (side_strategy.apply(p0, p1, corner1) * sign < 0)
            {
                result = cast::apply(ps_strategy.apply(corner1, p0, p1));
                return true;
            }
            if (side_strategy.apply(p0, p1, corner2) * sign > 0)
            {
                result = cast::apply(ps_strategy.apply(corner2, p0, p1));
                return true;
            }
            return false;
        }
    };

    static inline ReturnType
    non_negative_slope_segment(SegmentPoint const& p0,
                               SegmentPoint const& p1,
                               BoxPoint const& top_left,
                               BoxPoint const& top_right,
                               BoxPoint const& bottom_left,
                               BoxPoint const& bottom_right,
                               Strategies const& strategies)
    {
        typedef compare_less_equal<ReturnType, true> less_equal;

        // assert that the segment has non-negative slope
        BOOST_GEOMETRY_ASSERT( ( math::equals(geometry::get<0>(p0), geometry::get<0>(p1))
                              && geometry::get<1>(p0) < geometry::get<1>(p1))
                            ||
                               ( geometry::get<0>(p0) < geometry::get<0>(p1)
                              && geometry::get<1>(p0) <= geometry::get<1>(p1) )
                            || geometry::has_nan_coordinate(p0)
                            || geometry::has_nan_coordinate(p1));

        ReturnType result(0);

        if (check_right_left_of_box
                <
                    less_equal
                >::apply(p0, p1,
                         top_left, top_right, bottom_left, bottom_right,
                         strategies, result))
        {
            return result;
        }

        if (check_above_below_of_box
                <
                    less_equal
                >::apply(p0, p1,
                         top_left, top_right, bottom_left, bottom_right,
                         strategies, result))
        {
            return result;
        }

        if (check_generic_position::apply(p0, p1,
                                          top_left, bottom_right,
                                          strategies, result))
        {
            return result;
        }

        // in all other cases the box and segment intersect, so return 0
        return result;
    }


    static inline ReturnType
    negative_slope_segment(SegmentPoint const& p0,
                           SegmentPoint const& p1,
                           BoxPoint const& top_left,
                           BoxPoint const& top_right,
                           BoxPoint const& bottom_left,
                           BoxPoint const& bottom_right,
                           Strategies const& strategies)
    {
        typedef compare_less_equal<ReturnType, false> greater_equal;

        // assert that the segment has negative slope
        BOOST_GEOMETRY_ASSERT( ( geometry::get<0>(p0) < geometry::get<0>(p1)
                              && geometry::get<1>(p0) > geometry::get<1>(p1) )
                            || geometry::has_nan_coordinate(p0)
                            || geometry::has_nan_coordinate(p1) );

        ReturnType result(0);

        if (check_right_left_of_box
                <
                    greater_equal
                >::apply(p0, p1,
                         bottom_left, bottom_right, top_left, top_right,
                         strategies, result))
        {
            return result;
        }

        if (check_above_below_of_box
                <
                    greater_equal
                >::apply(p1, p0,
                         top_right, top_left, bottom_right, bottom_left,
                         strategies, result))
        {
            return result;
        }

        if (check_generic_position::apply(p0, p1,
                                          bottom_left, top_right,
                                          strategies, result))
        {
            return result;
        }

        // in all other cases the box and segment intersect, so return 0
        return result;
    }

public:
    static inline ReturnType apply(SegmentPoint const& p0,
                                   SegmentPoint const& p1,
                                   BoxPoint const& top_left,
                                   BoxPoint const& top_right,
                                   BoxPoint const& bottom_left,
                                   BoxPoint const& bottom_right,
                                   Strategies const& strategies)
    {
        BOOST_GEOMETRY_ASSERT( (geometry::less<SegmentPoint, -1, typename Strategies::cs_tag>()(p0, p1))
                            || geometry::has_nan_coordinate(p0)
                            || geometry::has_nan_coordinate(p1) );

        if (geometry::get<0>(p0) < geometry::get<0>(p1)
            && geometry::get<1>(p0) > geometry::get<1>(p1))
        {
            return negative_slope_segment(p0, p1,
                                          top_left, top_right,
                                          bottom_left, bottom_right,
                                          strategies);
        }

        return non_negative_slope_segment(p0, p1,
                                          top_left, top_right,
                                          bottom_left, bottom_right,
                                          strategies);
    }

    template <typename LessEqual>
    static inline ReturnType call_above_of_box(SegmentPoint const& p0,
                                               SegmentPoint const& p1,
                                               SegmentPoint const& p_max,
                                               BoxPoint const& top_left,
                                               Strategies const& strategies)
    {
        return above_of_box<LessEqual>::apply(p0, p1, p_max, top_left, strategies);
    }

    template <typename LessEqual>
    static inline ReturnType call_above_of_box(SegmentPoint const& p0,
                                               SegmentPoint const& p1,
                                               BoxPoint const& top_left,
                                               Strategies const& strategies)
    {
        return above_of_box<LessEqual>::apply(p0, p1, top_left, strategies);
    }
};

//=========================================================================

template
<
    typename Segment,
    typename Box,
    typename std::size_t Dimension,
    typename Strategies
>
class segment_to_box
    : not_implemented<Segment, Box>
{};


template
<
    typename Segment,
    typename Box,
    typename Strategies
>
class segment_to_box<Segment, Box, 2, Strategies>
{
    typedef distance::strategy_t<Segment, Box, Strategies> strategy_type;

public:
    typedef distance::return_t<Segment, Box, Strategies> return_type;

    static inline return_type apply(Segment const& segment,
                                    Box const& box,
                                    Strategies const& strategies)
    {
        typedef typename point_type<Segment>::type segment_point;
        typedef typename point_type<Box>::type box_point;

        segment_point p[2];
        detail::assign_point_from_index<0>(segment, p[0]);
        detail::assign_point_from_index<1>(segment, p[1]);

        if (detail::equals::equals_point_point(p[0], p[1], strategies))
        {
            return dispatch::distance
                <
                    segment_point,
                    Box,
                    Strategies
                >::apply(p[0], box, strategies);
        }

        box_point top_left, top_right, bottom_left, bottom_right;
        detail::assign_box_corners(box, bottom_left, bottom_right,
                                   top_left, top_right);

        strategy_type::mirror(p[0], p[1],
                              bottom_left, bottom_right,
                              top_left, top_right);

        typedef geometry::less<segment_point, -1, typename Strategies::cs_tag> less_type;
        if (less_type()(p[0], p[1]))
        {
            return segment_to_box_2D
                <
                    return_type,
                    segment_point,
                    box_point,
                    Strategies
                >::apply(p[0], p[1],
                         top_left, top_right, bottom_left, bottom_right,
                         strategies);
        }
        else
        {
            return segment_to_box_2D
                <
                    return_type,
                    segment_point,
                    box_point,
                    Strategies
                >::apply(p[1], p[0],
                         top_left, top_right, bottom_left, bottom_right,
                         strategies);
        }
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Segment, typename Box, typename Strategies>
struct distance
    <
        Segment, Box, Strategies, segment_tag, box_tag,
        strategy_tag_distance_segment_box, false
    >
{
    static inline auto apply(Segment const& segment, Box const& box,
                             Strategies const& strategies)
    {
        assert_dimension_equal<Segment, Box>();

        return detail::distance::segment_to_box
            <
                Segment,
                Box,
                dimension<Segment>::value,
                Strategies
            >::apply(segment, box, strategies);
    }
};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SEGMENT_TO_BOX_HPP

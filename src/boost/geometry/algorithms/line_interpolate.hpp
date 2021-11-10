// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2018-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_LINE_INTERPOLATE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_LINE_INTERPOLATE_HPP

#include <iterator>
#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/value_type.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/algorithms/detail/dummy_geometries.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/line_interpolate/cartesian.hpp>
#include <boost/geometry/strategies/line_interpolate/geographic.hpp>
#include <boost/geometry/strategies/line_interpolate/spherical.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace line_interpolate
{

struct convert_and_push_back
{
    template <typename Range, typename Point>
    static inline void apply(Point const& p, Range& range)
    {
        typename boost::range_value<Range>::type p2;
        geometry::detail::conversion::convert_point_to_point(p, p2);
        range::push_back(range, p2);
    }
};

struct convert_and_assign
{
    template <typename Point1, typename Point2>
    static inline void apply(Point1 const& p1, Point2& p2)
    {
        geometry::detail::conversion::convert_point_to_point(p1, p2);
    }

};


/*!
\brief Internal, calculates interpolation point of a linestring using iterator pairs and
    specified strategy
*/
template <typename Policy>
struct interpolate_range
{
    template
    <
        typename Range,
        typename Distance,
        typename PointLike,
        typename Strategies
    >
    static inline void apply(Range const& range,
                             Distance const& max_distance,
                             PointLike & pointlike,
                             Strategies const& strategies)
    {
        typedef typename boost::range_iterator<Range const>::type iterator_t;
        typedef typename boost::range_value<Range const>::type point_t;

        iterator_t it = boost::begin(range);
        iterator_t end = boost::end(range);

        if (it == end) // empty(range)
        {
            BOOST_THROW_EXCEPTION(empty_input_exception());
            return;
        }
        if (max_distance <= 0) //non positive distance
        {
            Policy::apply(*it, pointlike);
            return;
        }

        auto const pp_strategy = strategies.distance(dummy_point(), dummy_point());
        auto const strategy = strategies.line_interpolate(range);

        typedef decltype(pp_strategy.apply(
                    std::declval<point_t>(), std::declval<point_t>())) distance_type;

        iterator_t prev = it++;
        distance_type repeated_distance = max_distance;
        distance_type prev_distance = 0;
        distance_type current_distance = 0;
        point_t start_p = *prev;

        for ( ; it != end ; ++it)
        {
            distance_type dist = pp_strategy.apply(*prev, *it);
            current_distance = prev_distance + dist;

            while (current_distance >= repeated_distance)
            {
                point_t p;
                distance_type diff_distance = current_distance - prev_distance;
                BOOST_ASSERT(diff_distance != distance_type(0));
                strategy.apply(start_p, *it,
                               (repeated_distance - prev_distance)/diff_distance,
                               p,
                               diff_distance);
                Policy::apply(p, pointlike);
                if (std::is_same<PointLike, point_t>::value)
                {
                    return;
                }
                start_p = p;
                prev_distance = repeated_distance;
                repeated_distance += max_distance;
            }
            prev_distance = current_distance;
            prev = it;
            start_p = *prev;
        }

        // case when max_distance is larger than linestring's length
        // return the last point in range (range is not empty)
        if (repeated_distance == max_distance)
        {
            Policy::apply(*(end-1), pointlike);
        }
    }
};

template <typename Policy>
struct interpolate_segment
{
    template <typename Segment, typename Distance, typename Pointlike, typename Strategy>
    static inline void apply(Segment const& segment,
                             Distance const& max_distance,
                             Pointlike & point,
                             Strategy const& strategy)
    {
        interpolate_range<Policy>().apply(segment_view<Segment>(segment),
                                          max_distance, point, strategy);
    }
};

}} // namespace detail::line_interpolate
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Pointlike,
    typename Tag1 = typename tag<Geometry>::type,
    typename Tag2 = typename tag<Pointlike>::type
>
struct line_interpolate
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry, Pointlike);
};


template <typename Geometry, typename Pointlike>
struct line_interpolate<Geometry, Pointlike, linestring_tag, point_tag>
    : detail::line_interpolate::interpolate_range
        <
            detail::line_interpolate::convert_and_assign
        >
{};

template <typename Geometry, typename Pointlike>
struct line_interpolate<Geometry, Pointlike, linestring_tag, multi_point_tag>
    : detail::line_interpolate::interpolate_range
        <
            detail::line_interpolate::convert_and_push_back
        >
{};

template <typename Geometry, typename Pointlike>
struct line_interpolate<Geometry, Pointlike, segment_tag, point_tag>
    : detail::line_interpolate::interpolate_segment
        <
            detail::line_interpolate::convert_and_assign
        >
{};

template <typename Geometry, typename Pointlike>
struct line_interpolate<Geometry, Pointlike, segment_tag, multi_point_tag>
    : detail::line_interpolate::interpolate_segment
        <
            detail::line_interpolate::convert_and_push_back
        >
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

template
<
    typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct line_interpolate
{
    template <typename Geometry, typename Distance, typename Pointlike>
    static inline void apply(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike,
                             Strategies const& strategies)
    {
        dispatch::line_interpolate
            <
                Geometry, Pointlike
            >::apply(geometry, max_distance, pointlike, strategies);
    }
};

template <typename Strategy>
struct line_interpolate<Strategy, false>
{
    template <typename Geometry, typename Distance, typename Pointlike>
    static inline void apply(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike,
                             Strategy const& strategy)
    {        
        using strategies::line_interpolate::services::strategy_converter;

        dispatch::line_interpolate
            <
                Geometry, Pointlike
            >::apply(geometry, max_distance, pointlike,
                     strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct line_interpolate<default_strategy, false>
{
    template <typename Geometry, typename Distance, typename Pointlike>
    static inline void apply(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike,
                             default_strategy)
    {        
        typedef typename strategies::line_interpolate::services::default_strategy
            <
                Geometry
            >::type strategy_type;

        dispatch::line_interpolate
            <
                Geometry, Pointlike
            >::apply(geometry, max_distance, pointlike, strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant {

template <typename Geometry>
struct line_interpolate
{
    template <typename Distance, typename Pointlike, typename Strategy>
    static inline void apply(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike,
                             Strategy const& strategy)
    {
        return resolve_strategy::line_interpolate
                <
                    Strategy
                >::apply(geometry, max_distance, pointlike, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct line_interpolate<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Pointlike, typename Strategy>
    struct visitor: boost::static_visitor<void>
    {
        Pointlike const& m_pointlike;
        Strategy const& m_strategy;

        visitor(Pointlike const& pointlike, Strategy const& strategy)
            : m_pointlike(pointlike)
            , m_strategy(strategy)
        {}

        template <typename Geometry, typename Distance>
        void operator()(Geometry const& geometry, Distance const& max_distance) const
        {
            line_interpolate<Geometry>::apply(geometry, max_distance,
                                              m_pointlike, m_strategy);
        }
    };

    template <typename Distance, typename Pointlike, typename Strategy>
    static inline void
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          Distance const& max_distance,
          Pointlike & pointlike,
          Strategy const& strategy)
    {
        boost::apply_visitor(
            visitor<Pointlike, Strategy>(pointlike, strategy),
            geometry,
            max_distance
        );
    }
};

} // namespace resolve_variant

/*!
\brief     Returns one or more points interpolated along a LineString \brief_strategy
\ingroup line_interpolate
\tparam Geometry Any type fulfilling a LineString concept
\tparam Distance A numerical distance measure
\tparam Pointlike Any type fulfilling Point or Multipoint concept
\tparam Strategy A type fulfilling a LineInterpolatePointStrategy concept
\param geometry Input geometry
\param max_distance Distance threshold (in units depending on coordinate system)
representing the spacing between the points
\param pointlike Output: either a Point (exactly one point will be constructed) or
a MultiPoint (depending on the max_distance one or more points will be constructed)
\param strategy line_interpolate strategy to be used for interpolation of
points

\qbk{[include reference/algorithms/line_interpolate.qbk]}

\qbk{distinguish,with strategy}

\qbk{
[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_line_interpolate_cartesian Cartesian]
\* [link geometry.reference.strategies.strategy_line_interpolate_spherical Spherical]
\* [link geometry.reference.strategies.strategy_line_interpolate_geographic Geographic]

[heading Example]
[line_interpolate_strategy]
[line_interpolate_strategy_output]

[heading See also]
\* [link geometry.reference.algorithms.densify densify]
}
 */
template
<
    typename Geometry,
    typename Distance,
    typename Pointlike,
    typename Strategy
>
inline void line_interpolate(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike,
                             Strategy const& strategy)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_variant::line_interpolate<Geometry>
                          ::apply(geometry, max_distance, pointlike, strategy);
}


/*!
\brief     Returns one or more points interpolated along a LineString.
\ingroup line_interpolate
\tparam Geometry Any type fulfilling a LineString concept
\tparam Distance A numerical distance measure
\tparam Pointlike Any type fulfilling Point or Multipoint concept
\param geometry Input geometry
\param max_distance Distance threshold (in units depending on coordinate system)
representing the spacing between the points
\param pointlike Output: either a Point (exactly one point will be constructed) or
a MultiPoint (depending on the max_distance one or more points will be constructed)

\qbk{[include reference/algorithms/line_interpolate.qbk]

[heading Example]
[line_interpolate]
[line_interpolate_output]

[heading See also]
\* [link geometry.reference.algorithms.densify densify]
}
 */
template<typename Geometry, typename Distance, typename Pointlike>
inline void line_interpolate(Geometry const& geometry,
                             Distance const& max_distance,
                             Pointlike & pointlike)
{
    concepts::check<Geometry const>();

    // detail::throw_on_empty_input(geometry);

    return resolve_variant::line_interpolate<Geometry>
                          ::apply(geometry, max_distance, pointlike, default_strategy());
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_LINE_INTERPOLATE_HPP

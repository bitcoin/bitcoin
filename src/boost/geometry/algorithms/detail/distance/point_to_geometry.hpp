// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP

#include <iterator>
#include <type_traits>

#include <boost/core/ignore_unused.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/detail/closest_feature/geometry_to_range.hpp>
#include <boost/geometry/algorithms/detail/closest_feature/point_to_range.hpp>
#include <boost/geometry/algorithms/detail/distance/is_comparable.hpp>
#include <boost/geometry/algorithms/detail/distance/iterator_selector.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/detail/within/point_in_geometry.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/relate/services.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template
<
    typename P1, typename P2, typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct point_to_point
{
    static inline
    auto apply(P1 const& p1, P2 const& p2, Strategies const& strategies)
    {
        boost::ignore_unused(strategies);
        return strategies.distance(p1, p2).apply(p1, p2);
    }
};

// TEMP?
// called by geometry_to_range
template <typename P1, typename P2, typename Strategy>
struct point_to_point<P1, P2, Strategy, false>
{
    static inline
    auto apply(P1 const& p1, P2 const& p2, Strategy const& strategy)
    {
        boost::ignore_unused(strategy);
        return strategy.apply(p1, p2);
    }
};


template
<
    typename Point, typename Segment, typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct point_to_segment
{
    static inline auto apply(Point const& point, Segment const& segment,
                             Strategies const& strategies)
    {
        typename point_type<Segment>::type p[2];
        geometry::detail::assign_point_from_index<0>(segment, p[0]);
        geometry::detail::assign_point_from_index<1>(segment, p[1]);

        boost::ignore_unused(strategies);
        return strategies.distance(point, segment).apply(point, p[0], p[1]);
    }
};

// TEMP?
// called by geometry_to_range
template <typename Point, typename Segment, typename Strategy>
struct point_to_segment<Point, Segment, Strategy, false>
{
    static inline auto apply(Point const& point, Segment const& segment,
                             Strategy const& strategy)
    {
        typename point_type<Segment>::type p[2];
        geometry::detail::assign_point_from_index<0>(segment, p[0]);
        geometry::detail::assign_point_from_index<1>(segment, p[1]);

        boost::ignore_unused(strategy);
        return strategy.apply(point, p[0], p[1]);
    }
};


template
<
    typename Point, typename Box, typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct point_to_box
{
    static inline auto apply(Point const& point, Box const& box,
                             Strategies const& strategies)
    {
        boost::ignore_unused(strategies);
        return strategies.distance(point, box).apply(point, box);
    }
};

// TEMP?
// called by geometry_to_range
template <typename Point, typename Box, typename Strategy>
struct point_to_box<Point, Box, Strategy, false>
{
    static inline auto apply(Point const& point, Box const& box,
                             Strategy const& strategy)
    {
        boost::ignore_unused(strategy);
        return strategy.apply(point, box);
    }
};


template
<
    typename Point,
    typename Range,
    closure_selector Closure,
    typename Strategies
>
class point_to_range
{
private:
    typedef distance::strategy_t<Point, Range, Strategies> strategy_type;

    typedef detail::closest_feature::point_to_point_range
        <
            Point, Range, Closure
        > point_to_point_range;

public:
    typedef distance::return_t<Point, Range, Strategies> return_type;

    static inline return_type apply(Point const& point, Range const& range,
                                    Strategies const& strategies)
    {
        if (boost::size(range) == 0)
        {
            return return_type(0);
        }

        distance::creturn_t<Point, Range, Strategies> cd_min;

        std::pair
            <
                typename boost::range_iterator<Range const>::type,
                typename boost::range_iterator<Range const>::type
            > it_pair
            = point_to_point_range::apply(point,
                                          boost::begin(range),
                                          boost::end(range),
                                          strategy::distance::services::get_comparable
                                              <
                                                  strategy_type
                                              >::apply(strategies.distance(point, range)),
                                          cd_min);

        return
            is_comparable<strategy_type>::value
            ?
            cd_min
            :
            strategies.distance(point, range).apply(point, *it_pair.first, *it_pair.second);
    }
};


template
<
    typename Point,
    typename Ring,
    closure_selector Closure,
    typename Strategies
>
struct point_to_ring
{
    typedef distance::return_t<Point, Ring, Strategies> return_type;

    static inline return_type apply(Point const& point,
                                    Ring const& ring,
                                    Strategies const& strategies)
    {
        if (within::within_point_geometry(point, ring, strategies))
        {
            return return_type(0);
        }

        return point_to_range
            <
                Point, Ring, closure<Ring>::value, Strategies
            >::apply(point, ring, strategies);
    }
};


template
<
    typename Point,
    typename Polygon,
    closure_selector Closure,
    typename Strategies
>
class point_to_polygon
{
public:
    typedef distance::return_t<Point, Polygon, Strategies> return_type;

private:
    typedef point_to_range
        <
            Point, typename ring_type<Polygon>::type, Closure, Strategies
        > per_ring;

    struct distance_to_interior_rings
    {
        template <typename InteriorRingIterator>
        static inline return_type apply(Point const& point,
                                        InteriorRingIterator first,
                                        InteriorRingIterator last,
                                        Strategies const& strategies)
        {
            for (InteriorRingIterator it = first; it != last; ++it)
            {
                if (within::within_point_geometry(point, *it, strategies))
                {
                    // the point is inside a polygon hole, so its distance
                    // to the polygon its distance to the polygon's
                    // hole boundary
                    return per_ring::apply(point, *it, strategies);
                }
            }
            return return_type(0);
        }

        template <typename InteriorRings>
        static inline return_type apply(Point const& point, InteriorRings const& interior_rings,
                                        Strategies const& strategies)
        {
            return apply(point,
                         boost::begin(interior_rings),
                         boost::end(interior_rings),
                         strategies);
        }
    };


public:
    static inline return_type apply(Point const& point,
                                    Polygon const& polygon,
                                    Strategies const& strategies)
    {
        if (! within::covered_by_point_geometry(point, exterior_ring(polygon),
                                                strategies))
        {
            // the point is outside the exterior ring, so its distance
            // to the polygon is its distance to the polygon's exterior ring
            return per_ring::apply(point, exterior_ring(polygon), strategies);
        }

        // Check interior rings
        return distance_to_interior_rings::apply(point,
                                                 interior_rings(polygon),
                                                 strategies);
    }
};


template
<
    typename Point,
    typename MultiGeometry,
    typename Strategies,
    bool CheckCoveredBy = std::is_same
        <
            typename tag<MultiGeometry>::type, multi_polygon_tag
        >::value
>
class point_to_multigeometry
{
private:
    typedef detail::closest_feature::geometry_to_range geometry_to_range;

    typedef distance::strategy_t<Point, MultiGeometry, Strategies> strategy_type;

public:
    typedef distance::return_t<Point, MultiGeometry, Strategies> return_type;

    static inline return_type apply(Point const& point,
                                    MultiGeometry const& multigeometry,
                                    Strategies const& strategies)
    {
        typedef iterator_selector<MultiGeometry const> selector_type;

        distance::creturn_t<Point, MultiGeometry, Strategies> cd;

        typename selector_type::iterator_type it_min
            = geometry_to_range::apply(point,
                                       selector_type::begin(multigeometry),
                                       selector_type::end(multigeometry),
                                       strategy::distance::services::get_comparable
                                           <
                                               strategy_type
                                           >::apply(strategies.distance(point, multigeometry)),
                                       cd);

        // TODO - It would be possible to use a tool similar to result_from_distance
        //        but working in the opposite way, i.e. calculating the distance
        //        value from comparable distance value. This way the additional distance
        //        call would not be needed.
        return
            is_comparable<strategy_type>::value
            ?
            cd
            :
            dispatch::distance
                <
                    Point,
                    typename std::iterator_traits
                        <
                            typename selector_type::iterator_type
                        >::value_type,
                    Strategies
                >::apply(point, *it_min, strategies);
    }
};


// this is called only for multipolygons, hence the change in the
// template parameter name MultiGeometry to MultiPolygon
template <typename Point, typename MultiPolygon, typename Strategies>
struct point_to_multigeometry<Point, MultiPolygon, Strategies, true>
{
    typedef distance::return_t<Point, MultiPolygon, Strategies> return_type;

    static inline return_type apply(Point const& point,
                                    MultiPolygon const& multipolygon,
                                    Strategies const& strategies)
    {
        if (within::covered_by_point_geometry(point, multipolygon, strategies))
        {
            return return_type(0);
        }

        return point_to_multigeometry
            <
                Point, MultiPolygon, Strategies, false
            >::apply(point, multipolygon, strategies);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename P1, typename P2, typename Strategy>
struct distance
    <
        P1, P2, Strategy, point_tag, point_tag,
        strategy_tag_distance_point_point, false
    > : detail::distance::point_to_point<P1, P2, Strategy>
{};


template <typename Point, typename Linestring, typename Strategy>
struct distance
    <
        Point, Linestring, Strategy, point_tag, linestring_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_range<Point, Linestring, closed, Strategy>
{};


template <typename Point, typename Ring, typename Strategy>
struct distance
    <
        Point, Ring, Strategy, point_tag, ring_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_ring
        <
            Point, Ring, closure<Ring>::value, Strategy
        >
{};


template <typename Point, typename Polygon, typename Strategy>
struct distance
    <
        Point, Polygon, Strategy, point_tag, polygon_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_polygon
        <
            Point, Polygon, closure<Polygon>::value, Strategy
        >
{};


template <typename Point, typename Segment, typename Strategy>
struct distance
    <
        Point, Segment, Strategy, point_tag, segment_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_segment<Point, Segment, Strategy>
{};


template <typename Point, typename Box, typename Strategy>
struct distance
    <
         Point, Box, Strategy, point_tag, box_tag,
         strategy_tag_distance_point_box, false
    > : detail::distance::point_to_box<Point, Box, Strategy>
{};


template<typename Point, typename MultiPoint, typename Strategy>
struct distance
    <
        Point, MultiPoint, Strategy, point_tag, multi_point_tag,
        strategy_tag_distance_point_point, false
    > : detail::distance::point_to_multigeometry
        <
            Point, MultiPoint, Strategy
        >
{};


template<typename Point, typename MultiLinestring, typename Strategy>
struct distance
    <
        Point, MultiLinestring, Strategy, point_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_multigeometry
        <
            Point, MultiLinestring, Strategy
        >
{};


template<typename Point, typename MultiPolygon, typename Strategy>
struct distance
    <
        Point, MultiPolygon, Strategy, point_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment, false
    > : detail::distance::point_to_multigeometry
        <
            Point, MultiPolygon, Strategy
        >
{};


template <typename Point, typename Linear, typename Strategy>
struct distance
    <
         Point, Linear, Strategy, point_tag, linear_tag,
         strategy_tag_distance_point_segment, false
    > : distance
        <
            Point, Linear, Strategy,
            point_tag, typename tag<Linear>::type,
            strategy_tag_distance_point_segment, false
        >
{};


template <typename Point, typename Areal, typename Strategy>
struct distance
    <
         Point, Areal, Strategy, point_tag, areal_tag,
         strategy_tag_distance_point_segment, false
    > : distance
        <
            Point, Areal, Strategy,
            point_tag, typename tag<Areal>::type,
            strategy_tag_distance_point_segment, false
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP

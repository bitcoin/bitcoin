// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2013 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2013 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXTREME_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXTREME_POINTS_HPP


#include <cstddef>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/iterators/ever_circling_iterator.hpp>

#include <boost/geometry/strategies/side.hpp>

#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace extreme_points
{

template <std::size_t Dimension>
struct compare
{
    template <typename Point>
    inline bool operator()(Point const& lhs, Point const& rhs)
    {
        return geometry::get<Dimension>(lhs) < geometry::get<Dimension>(rhs);
    }
};


template <std::size_t Dimension, typename PointType, typename CoordinateType>
inline void move_along_vector(PointType& point, PointType const& extreme, CoordinateType const& base_value)
{
    // Moves a point along the vector (point, extreme) in the direction of the extreme  point
    // This adapts the possibly uneven legs of the triangle (or trapezium-like shape)
    //      _____extreme            _____
    //     /     \                 /     \                                    .
    //    /base   \         =>    /       \ point                             .
    //             \ point                                                    .
    //
    // For so-called intruders, it can be used to adapt both legs to the level of "base"
    // For the base, it can be used to adapt both legs to the level of the max-value of the intruders
    // If there are 2 or more extreme values, use the one close to 'point' to have a correct vector

    CoordinateType const value = geometry::get<Dimension>(point);
    //if (geometry::math::equals(value, base_value))
    if (value >= base_value)
    {
        return;
    }

    PointType vector = point;
    subtract_point(vector, extreme);

    CoordinateType const diff = geometry::get<Dimension>(vector);

    // diff should never be zero
    // because of the way our triangle/trapezium is build.
    // We just return if it would be the case.
    if (geometry::math::equals(diff, 0))
    {
        return;
    }

    CoordinateType const base_diff = base_value - geometry::get<Dimension>(extreme);

    multiply_value(vector, base_diff);
    divide_value(vector, diff);

    // The real move:
    point = extreme;
    add_point(point, vector);
}


template <std::size_t Dimension, typename Range, typename CoordinateType>
inline void move_along_vector(Range& range, CoordinateType const& base_value)
{
    if (range.size() >= 3)
    {
        move_along_vector<Dimension>(range.front(), *(range.begin() + 1), base_value);
        move_along_vector<Dimension>(range.back(), *(range.rbegin() + 1), base_value);
    }
}


template<typename Ring, std::size_t Dimension>
struct extreme_points_on_ring
{

    typedef typename geometry::coordinate_type<Ring>::type coordinate_type;
    typedef typename boost::range_iterator<Ring const>::type range_iterator;
    typedef typename geometry::point_type<Ring>::type point_type;

    template <typename CirclingIterator, typename Points>
    static inline bool extend(CirclingIterator& it,
            std::size_t n,
            coordinate_type max_coordinate_value,
            Points& points, int direction)
    {
        std::size_t safe_index = 0;
        do
        {
            it += direction;

            points.push_back(*it);

            if (safe_index++ >= n)
            {
                // E.g.: ring is completely horizontal or vertical (= invalid, but we don't want to have an infinite loop)
                return false;
            }
        } while (geometry::math::equals(geometry::get<Dimension>(*it), max_coordinate_value));

        return true;
    }

    // Overload without adding to poinst
    template <typename CirclingIterator>
    static inline bool extend(CirclingIterator& it,
            std::size_t n,
            coordinate_type max_coordinate_value,
            int direction)
    {
        std::size_t safe_index = 0;
        do
        {
            it += direction;

            if (safe_index++ >= n)
            {
                // E.g.: ring is completely horizontal or vertical (= invalid, but we don't want to have an infinite loop)
                return false;
            }
        } while (geometry::math::equals(geometry::get<Dimension>(*it), max_coordinate_value));

        return true;
    }

    template <typename CirclingIterator>
    static inline bool extent_both_sides(Ring const& ring,
            point_type extreme,
            CirclingIterator& left,
            CirclingIterator& right)
    {
        std::size_t const n = boost::size(ring);
        coordinate_type const max_coordinate_value = geometry::get<Dimension>(extreme);

        if (! extend(left, n, max_coordinate_value, -1))
        {
            return false;
        }
        if (! extend(right, n, max_coordinate_value, +1))
        {
            return false;
        }

        return true;
    }

    template <typename Collection, typename CirclingIterator>
    static inline bool collect(Ring const& ring,
            point_type extreme,
            Collection& points,
            CirclingIterator& left,
            CirclingIterator& right)
    {
        std::size_t const n = boost::size(ring);
        coordinate_type const max_coordinate_value = geometry::get<Dimension>(extreme);

        // Collects first left, which is reversed (if more than one point) then adds the top itself, then right
        if (! extend(left, n, max_coordinate_value, points, -1))
        {
            return false;
        }
        std::reverse(points.begin(), points.end());
        points.push_back(extreme);
        if (! extend(right, n, max_coordinate_value, points, +1))
        {
            return false;
        }

        return true;
    }

    template <typename Extremes, typename Intruders, typename CirclingIterator, typename SideStrategy>
    static inline void get_intruders(Ring const& ring, CirclingIterator left, CirclingIterator right,
            Extremes const& extremes,
            Intruders& intruders,
            SideStrategy const& strategy)
    {
        if (boost::size(extremes) < 3)
        {
            return;
        }
        coordinate_type const min_value = geometry::get<Dimension>(*std::min_element(boost::begin(extremes), boost::end(extremes), compare<Dimension>()));

        // Also select left/right (if Dimension=1)
        coordinate_type const other_min = geometry::get<1 - Dimension>(*std::min_element(boost::begin(extremes), boost::end(extremes), compare<1 - Dimension>()));
        coordinate_type const other_max = geometry::get<1 - Dimension>(*std::max_element(boost::begin(extremes), boost::end(extremes), compare<1 - Dimension>()));

        std::size_t defensive_check_index = 0; // in case we skip over left/right check, collect modifies right too
        std::size_t const n = boost::size(ring);
        while (left != right && defensive_check_index < n)
        {
            coordinate_type const coordinate = geometry::get<Dimension>(*right);
            coordinate_type const other_coordinate = geometry::get<1 - Dimension>(*right);
            if (coordinate > min_value && other_coordinate > other_min && other_coordinate < other_max)
            {
                int const factor = geometry::point_order<Ring>::value == geometry::clockwise ? 1 : -1;
                int const first_side = strategy.apply(*right, extremes.front(), *(extremes.begin() + 1)) * factor;
                int const last_side = strategy.apply(*right, *(extremes.rbegin() + 1), extremes.back()) * factor;

                // If not lying left from any of the extemes side
                if (first_side != 1 && last_side != 1)
                {
                    //std::cout << "first " << first_side << " last " << last_side << std::endl;

                    // we start at this intrusion until it is handled, and don't affect our initial left iterator
                    CirclingIterator left_intrusion_it = right;
                    typename boost::range_value<Intruders>::type intruder;
                    collect(ring, *right, intruder, left_intrusion_it, right);

                    // Also moves these to base-level, makes sorting possible which can be done in case of self-tangencies
                    // (we might postpone this action, it is often not necessary. However it is not time-consuming)
                    move_along_vector<Dimension>(intruder, min_value);
                    intruders.push_back(intruder);
                    --right;
                }
            }
            ++right;
            defensive_check_index++;
        }
    }

    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline void get_intruders(Ring const& ring,
            Extremes const& extremes,
            Intruders& intruders,
            SideStrategy const& strategy)
    {
        std::size_t const n = boost::size(ring);
        if (n >= 3)
        {
            geometry::ever_circling_range_iterator<Ring const> left(ring);
            geometry::ever_circling_range_iterator<Ring const> right(ring);
            ++right;

            get_intruders(ring, left, right, extremes, intruders, strategy);
        }
    }

    template <typename Iterator, typename SideStrategy>
    static inline bool right_turn(Ring const& ring, Iterator it, SideStrategy const& strategy)
    {
        typename std::iterator_traits<Iterator>::difference_type const index
            = std::distance(boost::begin(ring), it);
        geometry::ever_circling_range_iterator<Ring const> left(ring);
        geometry::ever_circling_range_iterator<Ring const> right(ring);
        left += index;
        right += index;

        if (! extent_both_sides(ring, *it, left, right))
        {
            return false;
        }

        int const factor = geometry::point_order<Ring>::value == geometry::clockwise ? 1 : -1;
        int const first_side = strategy.apply(*(right - 1), *right, *left) * factor;
        int const last_side = strategy.apply(*left, *(left + 1), *right) * factor;

//std::cout << "Candidate at " << geometry::wkt(*it) << " first=" << first_side << " last=" << last_side << std::endl;

        // Turn should not be left (actually, it should be right because extent removes horizontal/collinear cases)
        return first_side != 1 && last_side != 1;
    }


    // Gets the extreme segments (top point plus neighbouring points), plus intruders, if any, on the same ring
    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline bool apply(Ring const& ring,
                             Extremes& extremes,
                             Intruders& intruders,
                             SideStrategy const& strategy)
    {
        std::size_t const n = boost::size(ring);
        if (n < 3)
        {
            return false;
        }

        // Get all maxima, usually one. In case of self-tangencies, or self-crossings,
        // the max might be is not valid. A valid max should make a right turn
        range_iterator max_it = boost::begin(ring);
        compare<Dimension> smaller;
        for (range_iterator it = max_it + 1; it != boost::end(ring); ++it)
        {
            if (smaller(*max_it, *it) && right_turn(ring, it, strategy))
            {
                max_it = it;
            }
        }

        if (max_it == boost::end(ring))
        {
            return false;
        }

        typename std::iterator_traits<range_iterator>::difference_type const
            index = std::distance(boost::begin(ring), max_it);
//std::cout << "Extreme point lies at " << index << " having " << geometry::wkt(*max_it) << std::endl;

        geometry::ever_circling_range_iterator<Ring const> left(ring);
        geometry::ever_circling_range_iterator<Ring const> right(ring);
        left += index;
        right += index;

        // Collect all points (often 3) in a temporary vector
        std::vector<point_type> points;
        points.reserve(3);
        if (! collect(ring, *max_it, points, left, right))
        {
            return false;
        }

//std::cout << "Built vector of " << points.size() << std::endl;

        coordinate_type const front_value = geometry::get<Dimension>(points.front());
        coordinate_type const back_value = geometry::get<Dimension>(points.back());
        coordinate_type const base_value = (std::max)(front_value, back_value);
        if (front_value < back_value)
        {
            move_along_vector<Dimension>(points.front(), *(points.begin() + 1), base_value);
        }
        else
        {
            move_along_vector<Dimension>(points.back(), *(points.rbegin() + 1), base_value);
        }

        std::copy(points.begin(), points.end(), std::back_inserter(extremes));

        get_intruders(ring, left, right, extremes, intruders, strategy);

        return true;
    }
};





}} // namespace detail::extreme_points
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    std::size_t Dimension,
    typename GeometryTag = typename tag<Geometry>::type
>
struct extreme_points
{};


template<typename Ring, std::size_t Dimension>
struct extreme_points<Ring, Dimension, ring_tag>
    : detail::extreme_points::extreme_points_on_ring<Ring, Dimension>
{};


template<typename Polygon, std::size_t Dimension>
struct extreme_points<Polygon, Dimension, polygon_tag>
{
    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline bool apply(Polygon const& polygon, Extremes& extremes, Intruders& intruders,
                             SideStrategy const& strategy)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;
        typedef detail::extreme_points::extreme_points_on_ring
            <
                ring_type, Dimension
            > ring_implementation;

        if (! ring_implementation::apply(geometry::exterior_ring(polygon),
                                         extremes, intruders, strategy))
        {
            return false;
        }

        // For a polygon, its interior rings can contain intruders
        typename interior_return_type<Polygon const>::type
            rings = interior_rings(polygon);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            ring_implementation::get_intruders(*it, extremes,  intruders, strategy);
        }

        return true;
    }
};

template<typename Box>
struct extreme_points<Box, 1, box_tag>
{
    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline bool apply(Box const& box, Extremes& extremes, Intruders& ,
                             SideStrategy const& )
    {
        extremes.resize(4);
        geometry::detail::assign_box_corners_oriented<false>(box, extremes);
        // ll,ul,ur,lr, contains too exactly the right info
        return true;
    }
};

template<typename Box>
struct extreme_points<Box, 0, box_tag>
{
    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline bool apply(Box const& box, Extremes& extremes, Intruders& ,
                             SideStrategy const& )
    {
        extremes.resize(4);
        geometry::detail::assign_box_corners_oriented<false>(box, extremes);
        // ll,ul,ur,lr, rotate one to start with UL and end with LL
        std::rotate(extremes.begin(), extremes.begin() + 1, extremes.end());
        return true;
    }
};

template<typename MultiPolygon, std::size_t Dimension>
struct extreme_points<MultiPolygon, Dimension, multi_polygon_tag>
{
    template <typename Extremes, typename Intruders, typename SideStrategy>
    static inline bool apply(MultiPolygon const& multi, Extremes& extremes,
                             Intruders& intruders, SideStrategy const& strategy)
    {
        // Get one for the very first polygon, that is (for the moment) enough.
        // It is not guaranteed the "extreme" then, but for the current purpose
        // (point_on_surface) it can just be this point.
        if (boost::size(multi) >= 1)
        {
            return extreme_points
                <
                    typename boost::range_value<MultiPolygon const>::type,
                    Dimension,
                    polygon_tag
                >::apply(*boost::begin(multi), extremes, intruders, strategy);
        }

        return false;
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Returns extreme points (for Edge=1 in dimension 1, so the top,
       for Edge=0 in dimension 0, the right side)
\note We could specify a strategy (less/greater) to get bottom/left side too. However, until now we don't need that.
 */
template
<
    std::size_t Edge,
    typename Geometry,
    typename Extremes,
    typename Intruders,
    typename SideStrategy
>
inline bool extreme_points(Geometry const& geometry,
                           Extremes& extremes,
                           Intruders& intruders,
                           SideStrategy const& strategy)
{
    concepts::check<Geometry const>();

    // Extremes is not required to follow a geometry concept (but it should support an output iterator),
    // but its elements should fulfil the point-concept
    concepts::check<typename boost::range_value<Extremes>::type>();

    // Intruders should contain collections which value type is point-concept
    // Extremes might be anything (supporting an output iterator), but its elements should fulfil the point-concept
    concepts::check
        <
            typename boost::range_value
                <
                    typename boost::range_value<Intruders>::type
                >::type
            const
        >();

    return dispatch::extreme_points
            <
                Geometry,
                Edge
            >::apply(geometry, extremes, intruders, strategy);
}


template
<
    std::size_t Edge,
    typename Geometry,
    typename Extremes,
    typename Intruders
>
inline bool extreme_points(Geometry const& geometry,
                           Extremes& extremes,
                           Intruders& intruders)
{
    typedef typename strategy::side::services::default_strategy
            <
                typename cs_tag<Geometry>::type
            >::type strategy_type;

    return geometry::extreme_points<Edge>(geometry,extremes, intruders, strategy_type());
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXTREME_POINTS_HPP

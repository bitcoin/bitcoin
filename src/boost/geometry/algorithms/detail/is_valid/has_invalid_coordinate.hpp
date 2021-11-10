// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_INVALID_COORDINATE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_INVALID_COORDINATE_HPP

#include <cstddef>
#include <type_traits>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/validity_failure_type.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>

#include <boost/geometry/util/has_non_finite_coordinate.hpp>

namespace boost { namespace geometry
{
    
#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{

struct always_valid
{
    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const&, VisitPolicy& visitor)
    {
        return ! visitor.template apply<no_failure>();
    }
};

struct point_has_invalid_coordinate
{
    template <typename Point, typename VisitPolicy>
    static inline bool apply(Point const& point, VisitPolicy& visitor)
    {
        boost::ignore_unused(visitor);

        return
            geometry::has_non_finite_coordinate(point)
            ?
            (! visitor.template apply<failure_invalid_coordinate>())
            :
            (! visitor.template apply<no_failure>());
    }

    template <typename Point>
    static inline bool apply(Point const& point)
    {
        return geometry::has_non_finite_coordinate(point);
    }
};

struct indexed_has_invalid_coordinate
{
    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const& geometry, VisitPolicy& visitor)
    {
        geometry::detail::indexed_point_view<Geometry const, 0> p0(geometry);
        geometry::detail::indexed_point_view<Geometry const, 1> p1(geometry);

        return point_has_invalid_coordinate::apply(p0, visitor)
            || point_has_invalid_coordinate::apply(p1, visitor);
    }
};


struct range_has_invalid_coordinate
{
    struct point_has_valid_coordinates
    {
        template <typename Point>
        static inline bool apply(Point const& point)
        {
            return ! point_has_invalid_coordinate::apply(point);
        }
    };

    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const& geometry, VisitPolicy& visitor)
    {
        boost::ignore_unused(visitor);

        bool const has_valid_coordinates = detail::check_iterator_range
            <
                point_has_valid_coordinates,
                true // do not consider an empty range as problematic
            >::apply(geometry::points_begin(geometry),
                     geometry::points_end(geometry));

        return has_valid_coordinates
            ?
            (! visitor.template apply<no_failure>())
            :
            (! visitor.template apply<failure_invalid_coordinate>());
    }
};


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type,
    bool HasFloatingPointCoordinates = std::is_floating_point
        <
            typename coordinate_type<Geometry>::type
        >::value
>
struct has_invalid_coordinate
    : range_has_invalid_coordinate
{};

template <typename Geometry, typename Tag>
struct has_invalid_coordinate<Geometry, Tag, false>
    : always_valid
{};

template <typename Point>
struct has_invalid_coordinate<Point, point_tag, true>
    : point_has_invalid_coordinate
{};

template <typename Segment>
struct has_invalid_coordinate<Segment, segment_tag, true>
    : indexed_has_invalid_coordinate
{};

template <typename Box>
struct has_invalid_coordinate<Box, box_tag, true>
    : indexed_has_invalid_coordinate
{};


}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_HAS_INVALID_COORDINATE_HPP

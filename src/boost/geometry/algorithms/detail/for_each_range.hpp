// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP


#include <type_traits>
#include <utility>

#include <boost/concept/requires.hpp>
#include <boost/core/addressof.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/type_traits.hpp>

#include <boost/geometry/views/box_view.hpp>
#include <boost/geometry/views/segment_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each
{


template <typename Point>
struct fe_range_point
{
    template <typename Functor>
    static inline bool apply(Point& point, Functor&& f)
    {
        Point* ptr = boost::addressof(point);
        return f(std::pair<Point*, Point*>(ptr, ptr + 1));
    }
};


template <typename Segment>
struct fe_range_segment
{
    template <typename Functor>
    static inline bool apply(Segment& segment, Functor&& f)
    {
        return f(segment_view<typename std::remove_const<Segment>::type>(segment));
    }
};


template <typename Range>
struct fe_range_range
{
    template <typename Functor>
    static inline bool apply(Range& range, Functor&& f)
    {
        return f(range);
    }
};


template <typename Polygon>
struct fe_range_polygon
{
    template <typename Functor>
    static inline bool apply(Polygon& polygon, Functor&& f)
    {
        return f(exterior_ring(polygon));

        // TODO: If some flag says true, also do the inner rings.
        // for convex hull, it's not necessary
    }
};

template <typename Box>
struct fe_range_box
{
    template <typename Functor>
    static inline bool apply(Box& box, Functor&& f)
    {
        return f(box_view<typename std::remove_const<Box>::type>(box));
    }
};

template <typename Multi, typename SinglePolicy>
struct fe_range_multi
{
    template <typename Functor>
    static inline bool apply(Multi& multi, Functor&& f)
    {
        auto const end = boost::end(multi);
        for (auto it = boost::begin(multi); it != end; ++it)
        {
            if (! SinglePolicy::apply(*it, f))
            {
                return false;
            }
        }
        return true;
    }
};

}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct for_each_range
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for this Geometry type.",
        Geometry, Tag);
};


template <typename Point>
struct for_each_range<Point, point_tag>
    : detail::for_each::fe_range_point<Point>
{};


template <typename Segment>
struct for_each_range<Segment, segment_tag>
    : detail::for_each::fe_range_segment<Segment>
{};


template <typename Linestring>
struct for_each_range<Linestring, linestring_tag>
    : detail::for_each::fe_range_range<Linestring>
{};


template <typename Ring>
struct for_each_range<Ring, ring_tag>
    : detail::for_each::fe_range_range<Ring>
{};


template <typename Polygon>
struct for_each_range<Polygon, polygon_tag>
    : detail::for_each::fe_range_polygon<Polygon>
{};


template <typename Box>
struct for_each_range<Box, box_tag>
    : detail::for_each::fe_range_box<Box>
{};


template <typename MultiPoint>
struct for_each_range<MultiPoint, multi_point_tag>
    : detail::for_each::fe_range_range<MultiPoint>
{};


template <typename Geometry>
struct for_each_range<Geometry, multi_linestring_tag>
    : detail::for_each::fe_range_multi
        <
            Geometry,
            detail::for_each::fe_range_range
                <
                    util::transcribe_const_t
                        <
                            Geometry,
                            typename boost::range_value<Geometry>::type
                        >
                >
        >
{};


template <typename Geometry>
struct for_each_range<Geometry, multi_polygon_tag>
    : detail::for_each::fe_range_multi
        <
            Geometry,
            detail::for_each::fe_range_polygon
                <
                    util::transcribe_const_t
                        <
                            Geometry,
                            typename boost::range_value<Geometry>::type
                        >
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

namespace detail
{


// Currently for Polygons p is checked only for exterior ring
// Should this function be renamed?
template <typename Geometry, typename UnaryPredicate>
inline bool all_ranges_of(Geometry const& geometry, UnaryPredicate p)
{
    return dispatch::for_each_range<Geometry const>::apply(geometry, p);
}


// Currently for Polygons p is checked only for exterior ring
// Should this function be renamed?
template <typename Geometry, typename UnaryPredicate>
inline bool any_range_of(Geometry const& geometry, UnaryPredicate p)
{
    return ! dispatch::for_each_range<Geometry const>::apply(geometry,
                [&](auto&& range)
                {
                    return ! p(range);
                });
}


// Currently for Polygons p is checked only for exterior ring
// Should this function be renamed?
template <typename Geometry, typename UnaryPredicate>
inline bool none_range_of(Geometry const& geometry, UnaryPredicate p)
{
    return dispatch::for_each_range<Geometry const>::apply(geometry,
                [&](auto&& range)
                {
                    return ! p(range);
                });
}


// Currently for Polygons f is called only for exterior ring
// Should this function be renamed?
template <typename Geometry, typename Functor>
inline Functor for_each_range(Geometry const& geometry, Functor f)
{
    dispatch::for_each_range<Geometry const>::apply(geometry,
        [&](auto&& range)
        {
            f(range);
            // TODO: Implement separate function?
            return true;
        });
    return f;
}


}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP


#include <boost/array.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/range.hpp>
#include <boost/geometry/views/detail/closed_clockwise_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace copy_segments
{

inline signed_size_type circular_offset(signed_size_type segment_count, signed_size_type index,
                                        signed_size_type offset)
{
    signed_size_type result = (index + offset) % segment_count;
    if (result < 0)
    {
       result += segment_count;
    }
    return result;
}

template <typename Range, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_range
{
    static inline bool apply(Range const& range,
            SegmentIdentifier const& seg_id, signed_size_type offset,
            PointOut& point)
    {
        using view_type = detail::closed_clockwise_view
            <
                Range const,
                closure<Range>::value,
                Reverse ? counterclockwise : clockwise
            >;

        view_type view(range);

        std::size_t const segment_count = boost::size(view) - 1;
        signed_size_type const target = circular_offset(segment_count, seg_id.segment_index, offset);
        BOOST_GEOMETRY_ASSERT(target >= 0
                           && std::size_t(target) < boost::size(view));

        geometry::convert(range::at(view, target), point);

        return true;
    }
};


template <typename Polygon, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_polygon
{
    static inline bool apply(Polygon const& polygon,
            SegmentIdentifier const& seg_id, signed_size_type offset,
            PointOut& point)
    {
        // Call ring-version with the right ring
        return copy_segment_point_range
            <
                typename geometry::ring_type<Polygon>::type,
                Reverse,
                SegmentIdentifier,
                PointOut
            >::apply
                (
                    seg_id.ring_index < 0
                        ? geometry::exterior_ring(polygon)
                        : range::at(geometry::interior_rings(polygon), seg_id.ring_index),
                    seg_id, offset,
                    point
                );
    }
};


template <typename Box, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_box
{
    static inline bool apply(Box const& box,
            SegmentIdentifier const& seg_id, signed_size_type offset,
            PointOut& point)
    {
        boost::array<typename point_type<Box>::type, 4> bp;
        assign_box_corners_oriented<Reverse>(box, bp);

        signed_size_type const target = circular_offset(4, seg_id.segment_index, offset);
        BOOST_GEOMETRY_ASSERT(target >= 0
                           && std::size_t(target) < bp.size());

        point = bp[target];
        return true;
    }
};


template
<
    typename MultiGeometry,
    typename SegmentIdentifier,
    typename PointOut,
    typename Policy
>
struct copy_segment_point_multi
{
    static inline bool apply(MultiGeometry const& multi,
                             SegmentIdentifier const& seg_id, signed_size_type offset,
                             PointOut& point)
    {
        BOOST_GEOMETRY_ASSERT(seg_id.multi_index >= 0
               && std::size_t(seg_id.multi_index) < boost::size(multi));

        // Call the single-version
        return Policy::apply(range::at(multi, seg_id.multi_index), seg_id, offset, point);
    }
};


}} // namespace detail::copy_segments
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename GeometryIn,
    bool Reverse,
    typename SegmentIdentifier,
    typename PointOut
>
struct copy_segment_point
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for this Geometry type.",
        Tag, GeometryIn);
};


template <typename LineString, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<linestring_tag, LineString, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_range
        <
            LineString, Reverse, SegmentIdentifier, PointOut
        >
{};


template <typename Ring, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<ring_tag, Ring, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_range
        <
            Ring, Reverse, SegmentIdentifier, PointOut
        >
{};

template <typename Polygon, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<polygon_tag, Polygon, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_polygon
        <
            Polygon, Reverse, SegmentIdentifier, PointOut
        >
{};


template <typename Box, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<box_tag, Box, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_box
        <
            Box, Reverse, SegmentIdentifier, PointOut
        >
{};


template
<
    typename MultiGeometry,
    bool Reverse,
    typename SegmentIdentifier,
    typename PointOut
>
struct copy_segment_point
    <
        multi_polygon_tag,
        MultiGeometry,
        Reverse,
        SegmentIdentifier,
        PointOut
    >
    : detail::copy_segments::copy_segment_point_multi
        <
            MultiGeometry,
            SegmentIdentifier,
            PointOut,
            detail::copy_segments::copy_segment_point_polygon
                <
                    typename boost::range_value<MultiGeometry>::type,
                    Reverse,
                    SegmentIdentifier,
                    PointOut
                >
        >
{};

template
<
    typename MultiGeometry,
    bool Reverse,
    typename SegmentIdentifier,
    typename PointOut
>
struct copy_segment_point
    <
        multi_linestring_tag,
        MultiGeometry,
        Reverse,
        SegmentIdentifier,
        PointOut
    >
    : detail::copy_segments::copy_segment_point_multi
        <
            MultiGeometry,
            SegmentIdentifier,
            PointOut,
            detail::copy_segments::copy_segment_point_range
                <
                    typename boost::range_value<MultiGeometry>::type,
                    Reverse,
                    SegmentIdentifier,
                    PointOut
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Helper function, copies a point from a segment
    \ingroup overlay
 */
template<bool Reverse, typename Geometry, typename SegmentIdentifier, typename PointOut>
inline bool copy_segment_point(Geometry const& geometry,
            SegmentIdentifier const& seg_id, signed_size_type offset,
            PointOut& point_out)
{
    concepts::check<Geometry const>();

    return dispatch::copy_segment_point
        <
            typename tag<Geometry>::type,
            Geometry,
            Reverse,
            SegmentIdentifier,
            PointOut
        >::apply(geometry, seg_id, offset, point_out);
}


/*!
    \brief Helper function, to avoid the same construct several times,
        copies a point, based on a source-index and two geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1, typename Geometry2,
    typename SegmentIdentifier,
    typename PointOut
>
inline bool copy_segment_point(Geometry1 const& geometry1, Geometry2 const& geometry2,
            SegmentIdentifier const& seg_id, signed_size_type offset,
            PointOut& point_out)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    BOOST_GEOMETRY_ASSERT(seg_id.source_index == 0 || seg_id.source_index == 1);

    if (seg_id.source_index == 0)
    {
        return dispatch::copy_segment_point
            <
                typename tag<Geometry1>::type,
                Geometry1,
                Reverse1,
                SegmentIdentifier,
                PointOut
            >::apply(geometry1, seg_id, offset, point_out);
    }
    else if (seg_id.source_index == 1)
    {
        return dispatch::copy_segment_point
            <
                typename tag<Geometry2>::type,
                Geometry2,
                Reverse2,
                SegmentIdentifier,
                PointOut
            >::apply(geometry2, seg_id, offset, point_out);
    }
    // Exception?
    return false;
}


/*!
    \brief Helper function, to avoid the same construct several times,
        copies a point, based on a source-index and two geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1, typename Geometry2,
    typename SegmentIdentifier,
    typename PointOut
>
inline bool copy_segment_points(Geometry1 const& geometry1, Geometry2 const& geometry2,
            SegmentIdentifier const& seg_id,
            PointOut& point1, PointOut& point2)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, 0, point1)
        && copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, 1, point2);
}

/*!
    \brief Helper function, copies three points: two from the specified segment
    (from, to) and the next one
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1, typename Geometry2,
    typename SegmentIdentifier,
    typename PointOut
>
inline bool copy_segment_points(Geometry1 const& geometry1, Geometry2 const& geometry2,
            SegmentIdentifier const& seg_id,
            PointOut& point1, PointOut& point2, PointOut& point3)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, 0, point1)
        && copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, 1, point2)
        && copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, 2, point3);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP

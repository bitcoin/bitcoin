// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP


#include <cstddef>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/algorithms/detail/convert_indexed_to_indexed.hpp>
#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/views/detail/closed_clockwise_view.hpp>


namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
// Silence warning C4512: assignment operator could not be generated
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127 4512)
#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace conversion
{

template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct point_to_box
{
    static inline void apply(Point const& point, Box& box)
    {
        typedef typename coordinate_type<Box>::type coordinate_type;

        set<Index, Dimension>(box,
                boost::numeric_cast<coordinate_type>(get<Dimension>(point)));
        point_to_box
            <
                Point, Box,
                Index, Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t DimensionCount
>
struct point_to_box<Point, Box, Index, DimensionCount, DimensionCount>
{
    static inline void apply(Point const& , Box& )
    {}
};

template <typename Box, typename Range, bool Close, bool Reverse>
struct box_to_range
{
    static inline void apply(Box const& box, Range& range)
    {
        traits::resize<Range>::apply(range, Close ? 5 : 4);
        assign_box_corners_oriented<Reverse>(box, range);
        if (Close)
        {
            range::at(range, 4) = range::at(range, 0);
        }
    }
};

template <typename Segment, typename Range>
struct segment_to_range
{
    static inline void apply(Segment const& segment, Range& range)
    {
        traits::resize<Range>::apply(range, 2);

        typename boost::range_iterator<Range>::type it = boost::begin(range);

        assign_point_from_index<0>(segment, *it);
        ++it;
        assign_point_from_index<1>(segment, *it);
    }
};

template
<
    typename Range1,
    typename Range2,
    bool Reverse = false
>
struct range_to_range
{
    struct default_policy
    {
        template <typename Point1, typename Point2>
        static inline void apply(Point1 const& point1, Point2 & point2)
        {
            geometry::detail::conversion::convert_point_to_point(point1, point2);
        }
    };
    
    static inline void apply(Range1 const& source, Range2& destination)
    {
        apply(source, destination, default_policy());
    }

    template <typename ConvertPointPolicy>
    static inline ConvertPointPolicy apply(Range1 const& source, Range2& destination,
                                           ConvertPointPolicy convert_point)
    {
        geometry::clear(destination);

        using view_type = detail::closed_clockwise_view
            <
                Range1 const,
                geometry::closure<Range1>::value,
                Reverse ? counterclockwise : clockwise
            >;

        // We consider input always as closed, and skip last
        // point for open output.
        view_type const view(source);

        typedef typename boost::range_size<Range1>::type size_type;
        size_type n = boost::size(view);
        if (geometry::closure<Range2>::value == geometry::open)
        {
            n--;
        }

        // If size == 0 && geometry::open <=> n = numeric_limits<size_type>::max()
        // but ok, sice below it == end()

        size_type i = 0;
        for (typename boost::range_iterator<view_type const>::type it
            = boost::begin(view);
            it != boost::end(view) && i < n;
            ++it, ++i)
        {
            typename boost::range_value<Range2>::type point;
            convert_point.apply(*it, point);
            range::push_back(destination, point);
        }

        return convert_point;
    }
};

template <typename Polygon1, typename Polygon2>
struct polygon_to_polygon
{
    typedef range_to_range
        <
            typename geometry::ring_type<Polygon1>::type,
            typename geometry::ring_type<Polygon2>::type,
            geometry::point_order<Polygon1>::value
                != geometry::point_order<Polygon2>::value
        > per_ring;

    static inline void apply(Polygon1 const& source, Polygon2& destination)
    {
        // Clearing managed per ring, and in the resizing of interior rings

        per_ring::apply(geometry::exterior_ring(source),
            geometry::exterior_ring(destination));

        // Container should be resizeable
        traits::resize
            <
                typename std::remove_reference
                <
                    typename traits::interior_mutable_type<Polygon2>::type
                >::type
            >::apply(interior_rings(destination), num_interior_rings(source));

        typename interior_return_type<Polygon1 const>::type
            rings_source = interior_rings(source);
        typename interior_return_type<Polygon2>::type
            rings_dest = interior_rings(destination);

        typename detail::interior_iterator<Polygon1 const>::type
            it_source = boost::begin(rings_source);
        typename detail::interior_iterator<Polygon2>::type
            it_dest = boost::begin(rings_dest);

        for ( ; it_source != boost::end(rings_source); ++it_source, ++it_dest)
        {
            per_ring::apply(*it_source, *it_dest);
        }
    }
};

template <typename Single, typename Multi, typename Policy>
struct single_to_multi: private Policy
{
    static inline void apply(Single const& single, Multi& multi)
    {
        traits::resize<Multi>::apply(multi, 1);
        Policy::apply(single, *boost::begin(multi));
    }
};



template <typename Multi1, typename Multi2, typename Policy>
struct multi_to_multi: private Policy
{
    static inline void apply(Multi1 const& multi1, Multi2& multi2)
    {
        traits::resize<Multi2>::apply(multi2, boost::size(multi1));

        typename boost::range_iterator<Multi1 const>::type it1
                = boost::begin(multi1);
        typename boost::range_iterator<Multi2>::type it2
                = boost::begin(multi2);

        for (; it1 != boost::end(multi1); ++it1, ++it2)
        {
            Policy::apply(*it1, *it2);
        }
    }
};


}} // namespace detail::conversion
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1, typename Geometry2,
    typename Tag1 = typename tag_cast<typename tag<Geometry1>::type, multi_tag>::type,
    typename Tag2 = typename tag_cast<typename tag<Geometry2>::type, multi_tag>::type,
    std::size_t DimensionCount = dimension<Geometry1>::type::value,
    bool UseAssignment = std::is_same<Geometry1, Geometry2>::value
                         && !std::is_array<Geometry1>::value
>
struct convert
    : not_implemented
        <
            Tag1, Tag2,
            std::integral_constant<std::size_t, DimensionCount>
        >
{};


template
<
    typename Geometry1, typename Geometry2,
    typename Tag,
    std::size_t DimensionCount
>
struct convert<Geometry1, Geometry2, Tag, Tag, DimensionCount, true>
{
    // Same geometry type -> copy whole geometry
    static inline void apply(Geometry1 const& source, Geometry2& destination)
    {
        destination = source;
    }
};


template
<
    typename Geometry1, typename Geometry2,
    std::size_t DimensionCount
>
struct convert<Geometry1, Geometry2, point_tag, point_tag, DimensionCount, false>
    : detail::conversion::point_to_point<Geometry1, Geometry2, 0, DimensionCount>
{};


template
<
    typename Box1, typename Box2,
    std::size_t DimensionCount
>
struct convert<Box1, Box2, box_tag, box_tag, DimensionCount, false>
    : detail::conversion::indexed_to_indexed<Box1, Box2, 0, DimensionCount>
{};


template
<
    typename Segment1, typename Segment2,
    std::size_t DimensionCount
>
struct convert<Segment1, Segment2, segment_tag, segment_tag, DimensionCount, false>
    : detail::conversion::indexed_to_indexed<Segment1, Segment2, 0, DimensionCount>
{};


template <typename Segment, typename LineString, std::size_t DimensionCount>
struct convert<Segment, LineString, segment_tag, linestring_tag, DimensionCount, false>
    : detail::conversion::segment_to_range<Segment, LineString>
{};


template <typename Ring1, typename Ring2, std::size_t DimensionCount>
struct convert<Ring1, Ring2, ring_tag, ring_tag, DimensionCount, false>
    : detail::conversion::range_to_range
        <
            Ring1,
            Ring2,
            geometry::point_order<Ring1>::value
                != geometry::point_order<Ring2>::value
        >
{};

template <typename LineString1, typename LineString2, std::size_t DimensionCount>
struct convert<LineString1, LineString2, linestring_tag, linestring_tag, DimensionCount, false>
    : detail::conversion::range_to_range<LineString1, LineString2>
{};

template <typename Polygon1, typename Polygon2, std::size_t DimensionCount>
struct convert<Polygon1, Polygon2, polygon_tag, polygon_tag, DimensionCount, false>
    : detail::conversion::polygon_to_polygon<Polygon1, Polygon2>
{};

template <typename Box, typename Ring>
struct convert<Box, Ring, box_tag, ring_tag, 2, false>
    : detail::conversion::box_to_range
        <
            Box,
            Ring,
            geometry::closure<Ring>::value == closed,
            geometry::point_order<Ring>::value == counterclockwise
        >
{};


template <typename Box, typename Polygon>
struct convert<Box, Polygon, box_tag, polygon_tag, 2, false>
{
    static inline void apply(Box const& box, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                Box, ring_type,
                box_tag, ring_tag,
                2, false
            >::apply(box, exterior_ring(polygon));
    }
};


template <typename Point, typename Box, std::size_t DimensionCount>
struct convert<Point, Box, point_tag, box_tag, DimensionCount, false>
{
    static inline void apply(Point const& point, Box& box)
    {
        detail::conversion::point_to_box
            <
                Point, Box, min_corner, 0, DimensionCount
            >::apply(point, box);
        detail::conversion::point_to_box
            <
                Point, Box, max_corner, 0, DimensionCount
            >::apply(point, box);
    }
};


template <typename Ring, typename Polygon, std::size_t DimensionCount>
struct convert<Ring, Polygon, ring_tag, polygon_tag, DimensionCount, false>
{
    static inline void apply(Ring const& ring, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;
        convert
            <
                Ring, ring_type,
                ring_tag, ring_tag,
                DimensionCount, false
            >::apply(ring, exterior_ring(polygon));
    }
};


template <typename Polygon, typename Ring, std::size_t DimensionCount>
struct convert<Polygon, Ring, polygon_tag, ring_tag, DimensionCount, false>
{
    static inline void apply(Polygon const& polygon, Ring& ring)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                ring_type, Ring,
                ring_tag, ring_tag,
                DimensionCount, false
            >::apply(exterior_ring(polygon), ring);
    }
};


// Dispatch for multi <-> multi, specifying their single-version as policy.
// Note that, even if the multi-types are mutually different, their single
// version types might be the same and therefore we call std::is_same again

template <typename Multi1, typename Multi2, std::size_t DimensionCount>
struct convert<Multi1, Multi2, multi_tag, multi_tag, DimensionCount, false>
    : detail::conversion::multi_to_multi
        <
            Multi1,
            Multi2,
            convert
                <
                    typename boost::range_value<Multi1>::type,
                    typename boost::range_value<Multi2>::type,
                    typename single_tag_of
                                <
                                    typename tag<Multi1>::type
                                >::type,
                    typename single_tag_of
                                <
                                    typename tag<Multi2>::type
                                >::type,
                    DimensionCount
                >
        >
{};


template <typename Single, typename Multi, typename SingleTag, std::size_t DimensionCount>
struct convert<Single, Multi, SingleTag, multi_tag, DimensionCount, false>
    : detail::conversion::single_to_multi
        <
            Single,
            Multi,
            convert
                <
                    Single,
                    typename boost::range_value<Multi>::type,
                    typename tag<Single>::type,
                    typename single_tag_of
                                <
                                    typename tag<Multi>::type
                                >::type,
                    DimensionCount,
                    false
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_variant {

template <typename Geometry1, typename Geometry2>
struct convert
{
    static inline void apply(Geometry1 const& geometry1, Geometry2& geometry2)
    {
        concepts::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2>();
        dispatch::convert<Geometry1, Geometry2>::apply(geometry1, geometry2);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct convert<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    struct visitor: static_visitor<void>
    {
        Geometry2& m_geometry2;

        visitor(Geometry2& geometry2)
            : m_geometry2(geometry2)
        {}

        template <typename Geometry1>
        inline void operator()(Geometry1 const& geometry1) const
        {
            convert<Geometry1, Geometry2>::apply(geometry1, m_geometry2);
        }
    };

    static inline void apply(
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
        Geometry2& geometry2
    )
    {
        boost::apply_visitor(visitor(geometry2), geometry1);
    }
};

}


/*!
\brief Converts one geometry to another geometry
\details The convert algorithm converts one geometry, e.g. a BOX, to another
geometry, e.g. a RING. This only works if it is possible and applicable.
If the point-order is different, or the closure is different between two
geometry types, it will be converted correctly by explicitly reversing the
points or closing or opening the polygon rings.
\ingroup convert
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry (source)
\param geometry2 \param_geometry (target)

\qbk{[include reference/algorithms/convert.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline void convert(Geometry1 const& geometry1, Geometry2& geometry2)
{
    resolve_variant::convert<Geometry1, Geometry2>::apply(geometry1, geometry2);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP

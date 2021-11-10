// Boost.Geometry

// Copyright (c) 2017 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CORRECT_CLOSURE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CORRECT_CLOSURE_HPP

#include <cstddef>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/detail/multi_modify.hpp>

namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace correct_closure
{

template <typename Geometry>
struct nop
{
    static inline void apply(Geometry& )
    {}
};


// Close a ring, if not closed, or open it
template <typename Ring>
struct close_or_open_ring
{
    static inline void apply(Ring& r)
    {
        if (boost::size(r) <= 2)
        {
            return;
        }

        bool const disjoint = geometry::disjoint(*boost::begin(r),
                                                 *(boost::end(r) - 1));
        closure_selector const s = geometry::closure<Ring>::value;

        if (disjoint && s == closed)
        {
            // Close it by adding first point
            geometry::append(r, *boost::begin(r));
        }
        else if (! disjoint && s != closed)
        {
            // Open it by removing last point
            geometry::traits::resize<Ring>::apply(r, boost::size(r) - 1);
        }
    }
};

// Close/open exterior ring and all its interior rings
template <typename Polygon>
struct close_or_open_polygon
{
    typedef typename ring_type<Polygon>::type ring_type;

    static inline void apply(Polygon& poly)
    {
        close_or_open_ring<ring_type>::apply(exterior_ring(poly));

        typename interior_return_type<Polygon>::type
            rings = interior_rings(poly);

        for (typename detail::interior_iterator<Polygon>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            close_or_open_ring<ring_type>::apply(*it);
        }
    }
};

}} // namespace detail::correct_closure
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct correct_closure: not_implemented<Tag>
{};

template <typename Point>
struct correct_closure<Point, point_tag>
    : detail::correct_closure::nop<Point>
{};

template <typename LineString>
struct correct_closure<LineString, linestring_tag>
    : detail::correct_closure::nop<LineString>
{};

template <typename Segment>
struct correct_closure<Segment, segment_tag>
    : detail::correct_closure::nop<Segment>
{};


template <typename Box>
struct correct_closure<Box, box_tag>
    : detail::correct_closure::nop<Box>
{};

template <typename Ring>
struct correct_closure<Ring, ring_tag>
    : detail::correct_closure::close_or_open_ring<Ring>
{};

template <typename Polygon>
struct correct_closure<Polygon, polygon_tag>
    : detail::correct_closure::close_or_open_polygon<Polygon>
{};


template <typename MultiPoint>
struct correct_closure<MultiPoint, multi_point_tag>
    : detail::correct_closure::nop<MultiPoint>
{};


template <typename MultiLineString>
struct correct_closure<MultiLineString, multi_linestring_tag>
    : detail::correct_closure::nop<MultiLineString>
{};


template <typename Geometry>
struct correct_closure<Geometry, multi_polygon_tag>
    : detail::multi_modify
        <
            Geometry,
            detail::correct_closure::close_or_open_polygon
                <
                    typename boost::range_value<Geometry>::type
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_variant
{

template <typename Geometry>
struct correct_closure
{
    static inline void apply(Geometry& geometry)
    {
        concepts::check<Geometry const>();
        dispatch::correct_closure<Geometry>::apply(geometry);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct correct_closure<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor: boost::static_visitor<void>
    {
        template <typename Geometry>
        void operator()(Geometry& geometry) const
        {
            correct_closure<Geometry>::apply(geometry);
        }
    };

    static inline void
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>& geometry)
    {
        visitor vis;
        boost::apply_visitor(vis, geometry);
    }
};

} // namespace resolve_variant


/*!
\brief Closes or opens a geometry, according to its type
\details Corrects a geometry w.r.t. closure points to all rings which do not
    have a closing point and are typed as they should have one, the first point
    is appended.
\ingroup correct_closure
\tparam Geometry \tparam_geometry
\param geometry \param_geometry which will be corrected if necessary
*/
template <typename Geometry>
inline void correct_closure(Geometry& geometry)
{
    resolve_variant::correct_closure<Geometry>::apply(geometry);
}


#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CORRECT_CLOSURE_HPP

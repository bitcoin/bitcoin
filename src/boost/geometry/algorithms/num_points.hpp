// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP

#include <cstddef>

#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/algorithms/detail/counting.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace num_points
{


template <bool AddForOpen>
struct range_count
{
    template <typename Range>
    static inline std::size_t apply(Range const& range)
    {
        std::size_t n = boost::size(range);
        if (AddForOpen
            && n > 0
            && geometry::closure<Range>::value == open
            )
        {
            return n + 1;
        }
        return n;
    }
};

}} // namespace detail::num_points
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    bool AddForOpen,
    typename Tag = typename tag_cast
        <
            typename tag<Geometry>::type, multi_tag
        >::type
>
struct num_points: not_implemented<Tag>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, point_tag>
    : detail::counting::other_count<1>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, box_tag>
    : detail::counting::other_count<(1 << geometry::dimension<Geometry>::value)>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, segment_tag>
    : detail::counting::other_count<2>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, linestring_tag>
    : detail::num_points::range_count<AddForOpen>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, ring_tag>
    : detail::num_points::range_count<AddForOpen>
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, polygon_tag>
    : detail::counting::polygon_count
        <
            detail::num_points::range_count<AddForOpen>
        >
{};

template <typename Geometry, bool AddForOpen>
struct num_points<Geometry, AddForOpen, multi_tag>
    : detail::counting::multi_count
        <
            num_points<typename boost::range_value<Geometry>::type, AddForOpen>
        >
{};

} // namespace dispatch
#endif


namespace resolve_variant
{

template <typename Geometry>
struct num_points
{
    static inline std::size_t apply(Geometry const& geometry,
                                    bool add_for_open)
    {
        concepts::check<Geometry const>();

        return add_for_open
            ? dispatch::num_points<Geometry, true>::apply(geometry)
            : dispatch::num_points<Geometry, false>::apply(geometry);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct num_points<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor: boost::static_visitor<std::size_t>
    {
        bool m_add_for_open;

        visitor(bool add_for_open): m_add_for_open(add_for_open) {}

        template <typename Geometry>
        inline std::size_t operator()(Geometry const& geometry) const
        {
            return num_points<Geometry>::apply(geometry, m_add_for_open);
        }
    };

    static inline std::size_t
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          bool add_for_open)
    {
        return boost::apply_visitor(visitor(add_for_open), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_calc{number of points}
\ingroup num_points
\details \details_calc{num_points, number of points}.
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param add_for_open add one for open geometries (i.e. polygon types which are not closed)
\return \return_calc{number of points}

\qbk{[include reference/algorithms/num_points.qbk]}
*/
template <typename Geometry>
inline std::size_t num_points(Geometry const& geometry, bool add_for_open = false)
{
    return resolve_variant::num_points<Geometry>::apply(geometry, add_for_open);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP

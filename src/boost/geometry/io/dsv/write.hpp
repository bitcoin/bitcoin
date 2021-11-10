// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2018-2020.
// Modifications copyright (c) 2018-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_DSV_WRITE_HPP
#define BOOST_GEOMETRY_IO_DSV_WRITE_HPP

#include <cstddef>
#include <ostream>
#include <string>

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace dsv
{

struct dsv_settings
{
    std::string coordinate_separator;
    std::string point_open;
    std::string point_close;
    std::string point_separator;
    std::string list_open;
    std::string list_close;
    std::string list_separator;

    dsv_settings(std::string const& sep
            , std::string const& open
            , std::string const& close
            , std::string const& psep
            , std::string const& lopen
            , std::string const& lclose
            , std::string const& lsep
            )
        : coordinate_separator(sep)
        , point_open(open)
        , point_close(close)
        , point_separator(psep)
        , list_open(lopen)
        , list_close(lclose)
        , list_separator(lsep)
    {}
};

/*!
\brief Stream coordinate of a point as \ref DSV
*/
template <typename Point, std::size_t Dimension, std::size_t Count>
struct stream_coordinate
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Point const& point,
            dsv_settings const& settings)
    {
        os << (Dimension > 0 ? settings.coordinate_separator : "")
            << get<Dimension>(point);

        stream_coordinate
            <
                Point, Dimension + 1, Count
            >::apply(os, point, settings);
    }
};

template <typename Point, std::size_t Count>
struct stream_coordinate<Point, Count, Count>
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>&,
            Point const&,
            dsv_settings const& )
    {
    }
};

/*!
\brief Stream indexed coordinate of a box/segment as \ref DSV
*/
template
<
    typename Geometry,
    std::size_t Index,
    std::size_t Dimension,
    std::size_t Count
>
struct stream_indexed
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Geometry const& geometry,
            dsv_settings const& settings)
    {
        os << (Dimension > 0 ? settings.coordinate_separator : "")
            << get<Index, Dimension>(geometry);
        stream_indexed
            <
                Geometry, Index, Dimension + 1, Count
            >::apply(os, geometry, settings);
    }
};

template <typename Geometry, std::size_t Index, std::size_t Count>
struct stream_indexed<Geometry, Index, Count, Count>
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>&, Geometry const&,
            dsv_settings const& )
    {
    }
};

/*!
\brief Stream points as \ref DSV
*/
template <typename Point>
struct dsv_point
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Point const& p,
            dsv_settings const& settings)
    {
        os << settings.point_open;
        stream_coordinate<Point, 0, dimension<Point>::type::value>::apply(os, p, settings);
        os << settings.point_close;
    }
};

/*!
\brief Stream ranges as DSV
\note policy is used to stream prefix/postfix, enabling derived classes to override this
*/
template <typename Range>
struct dsv_range
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Range const& range,
            dsv_settings const& settings)
    {
        typedef typename boost::range_iterator<Range const>::type iterator_type;

        bool first = true;

        os << settings.list_open;

        for (iterator_type it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            os << (first ? "" : settings.point_separator)
                << settings.point_open;

            stream_coordinate
                <
                    point_type, 0, dimension<point_type>::type::value
                >::apply(os, *it, settings);
            os << settings.point_close;

            first = false;
        }

        os << settings.list_close;
    }

private:
    typedef typename boost::range_value<Range>::type point_type;
};

/*!
\brief Stream sequence of points as DSV-part, e.g. (1 2),(3 4)
\note Used in polygon, all multi-geometries
*/

template <typename Polygon>
struct dsv_poly
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Polygon const& poly,
                dsv_settings const& settings)
    {
        typedef typename ring_type<Polygon>::type ring;

        os << settings.list_open;

        dsv_range<ring>::apply(os, exterior_ring(poly), settings);

        typename interior_return_type<Polygon const>::type
            rings = interior_rings(poly);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            os << settings.list_separator;
            dsv_range<ring>::apply(os, *it, settings);
        }
        os << settings.list_close;
    }
};

template <typename Geometry, std::size_t Index>
struct dsv_per_index
{
    typedef typename point_type<Geometry>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Geometry const& geometry,
            dsv_settings const& settings)
    {
        os << settings.point_open;
        stream_indexed
            <
                Geometry, Index, 0, dimension<Geometry>::type::value
            >::apply(os, geometry, settings);
        os << settings.point_close;
    }
};

template <typename Geometry>
struct dsv_indexed
{
    typedef typename point_type<Geometry>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
            Geometry const& geometry,
            dsv_settings const& settings)
    {
        os << settings.list_open;
        dsv_per_index<Geometry, 0>::apply(os, geometry, settings);
        os << settings.point_separator;
        dsv_per_index<Geometry, 1>::apply(os, geometry, settings);
        os << settings.list_close;
    }
};

}} // namespace detail::dsv
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Tag, typename Geometry>
struct dsv {};

template <typename Point>
struct dsv<point_tag, Point>
    : detail::dsv::dsv_point<Point>
{};

template <typename Linestring>
struct dsv<linestring_tag, Linestring>
    : detail::dsv::dsv_range<Linestring>
{};

template <typename Box>
struct dsv<box_tag, Box>
    : detail::dsv::dsv_indexed<Box>
{};

template <typename Segment>
struct dsv<segment_tag, Segment>
    : detail::dsv::dsv_indexed<Segment>
{};

template <typename Ring>
struct dsv<ring_tag, Ring>
    : detail::dsv::dsv_range<Ring>
{};

template <typename Polygon>
struct dsv<polygon_tag, Polygon>
    : detail::dsv::dsv_poly<Polygon>
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace dsv
{

// FIXME: This class is not copyable/assignable but it is used as such --mloskot
template <typename Geometry>
class dsv_manipulator
{
public:

    inline dsv_manipulator(Geometry const& g,
            dsv_settings const& settings)
        : m_geometry(g)
        , m_settings(settings)
    {}

    template <typename Char, typename Traits>
    inline friend std::basic_ostream<Char, Traits>& operator<<(
            std::basic_ostream<Char, Traits>& os,
            dsv_manipulator const& m)
    {
        dispatch::dsv
            <
                typename tag_cast
                    <
                        typename tag<Geometry>::type,
                        multi_tag
                    >::type,
                Geometry
            >::apply(os, m.m_geometry, m.m_settings);
        os.flush();
        return os;
    }

private:
    Geometry const& m_geometry;
    dsv_settings m_settings;
};


template <typename MultiGeometry>
struct dsv_multi
{
    typedef dispatch::dsv
                <
                    typename single_tag_of
                        <
                            typename tag<MultiGeometry>::type
                        >::type,
                    typename boost::range_value<MultiGeometry>::type
                > dispatch_one;

    typedef typename boost::range_iterator
        <
            MultiGeometry const
        >::type iterator;


    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                MultiGeometry const& multi,
                dsv_settings const& settings)
    {
        os << settings.list_open;

        bool first = true;
        for(iterator it = boost::begin(multi);
            it != boost::end(multi);
            ++it, first = false)
        {
            os << (first ? "" : settings.list_separator);
            dispatch_one::apply(os, *it, settings);
        }
        os << settings.list_close;
    }
};

}} // namespace detail::dsv
#endif // DOXYGEN_NO_DETAIL

// TODO: The alternative to this could be a forward declaration of dispatch::dsv<>
//       or braking the code into the interface and implementation part
#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry>
struct dsv<multi_tag, Geometry>
    : detail::dsv::dsv_multi<Geometry>
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
\brief Main DSV-streaming function
\details DSV stands for Delimiter Separated Values. Geometries can be streamed
    as DSV. There are defaults for all separators.
\note Useful for examples and testing purposes
\note With this function GeoJSON objects can be created, using the right
    delimiters
\ingroup dsv
*/
template <typename Geometry>
inline detail::dsv::dsv_manipulator<Geometry> dsv(Geometry const& geometry
    , std::string const& coordinate_separator = ", "
    , std::string const& point_open = "("
    , std::string const& point_close = ")"
    , std::string const& point_separator = ", "
    , std::string const& list_open = "("
    , std::string const& list_close = ")"
    , std::string const& list_separator = ", "
    )
{
    concepts::check<Geometry const>();

    return detail::dsv::dsv_manipulator<Geometry>(geometry,
        detail::dsv::dsv_settings(coordinate_separator,
            point_open, point_close, point_separator,
            list_open, list_close, list_separator));
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_DSV_WRITE_HPP

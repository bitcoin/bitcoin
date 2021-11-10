// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2016-2020.
// Modifications copyright (c) 2016-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_SVG_WRITE_HPP
#define BOOST_GEOMETRY_IO_SVG_WRITE_HPP

#include <ostream>
#include <string>

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace svg
{


template <typename Point>
struct svg_point
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Point const& p, std::string const& style, double size)
    {
        os << "<circle cx=\"" << geometry::get<0>(p)
            << "\" cy=\"" << geometry::get<1>(p)
            << "\" r=\"" << (size < 0 ? 5 : size)
            << "\" style=\"" << style << "\"/>";
    }
};


template <typename Box>
struct svg_box
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Box const& box, std::string const& style, double)
    {
        // Prevent invisible boxes, making them >=1, using "max"
        BOOST_USING_STD_MAX();

        typedef typename coordinate_type<Box>::type ct;
        ct x = geometry::get<geometry::min_corner, 0>(box);
        ct y = geometry::get<geometry::min_corner, 1>(box);
        ct width = max BOOST_PREVENT_MACRO_SUBSTITUTION (ct(1),
                    geometry::get<geometry::max_corner, 0>(box) - x);
        ct height = max BOOST_PREVENT_MACRO_SUBSTITUTION (ct(1),
                    geometry::get<geometry::max_corner, 1>(box) - y);

        os << "<rect x=\"" << x << "\" y=\"" << y
           << "\" width=\"" << width << "\" height=\"" << height
           << "\" style=\"" << style << "\"/>";
    }
};

template <typename Segment>
struct svg_segment
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
        Segment const& segment, std::string const& style, double)
    {
        typedef typename coordinate_type<Segment>::type ct;
        ct x1 = geometry::get<0, 0>(segment);
        ct y1 = geometry::get<0, 1>(segment);
        ct x2 = geometry::get<1, 0>(segment);
        ct y2 = geometry::get<1, 1>(segment);
        
        os << "<line x1=\"" << x1 << "\" y1=\"" << y1
            << "\" x2=\"" << x2 << "\" y2=\"" << y2
            << "\" style=\"" << style << "\"/>";
    }
};

/*!
\brief Stream ranges as SVG
\note policy is used to select type (polyline/polygon)
*/
template <typename Range, typename Policy>
struct svg_range
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
        Range const& range, std::string const& style, double)
    {
        typedef typename boost::range_iterator<Range const>::type iterator;

        bool first = true;

        os << "<" << Policy::prefix() << " points=\"";

        for (iterator it = boost::begin(range);
            it != boost::end(range);
            ++it, first = false)
        {
            os << (first ? "" : " " )
                << geometry::get<0>(*it)
                << ","
                << geometry::get<1>(*it);
        }
        os << "\" style=\"" << style << Policy::style() << "\"/>";
    }
};



template <typename Polygon>
struct svg_poly
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
        Polygon const& polygon, std::string const& style, double)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;
        typedef typename boost::range_iterator<ring_type const>::type iterator_type;

        bool first = true;
        os << "<g fill-rule=\"evenodd\"><path d=\"";

        ring_type const& ring = geometry::exterior_ring(polygon);
        for (iterator_type it = boost::begin(ring);
            it != boost::end(ring);
            ++it, first = false)
        {
            os << (first ? "M" : " L") << " "
                << geometry::get<0>(*it)
                << ","
                << geometry::get<1>(*it);
        }

        // Inner rings:
        {
            typename interior_return_type<Polygon const>::type
                rings = interior_rings(polygon);
            for (typename detail::interior_iterator<Polygon const>::type
                    rit = boost::begin(rings); rit != boost::end(rings); ++rit)
            {
                first = true;
                for (typename detail::interior_ring_iterator<Polygon const>::type
                        it = boost::begin(*rit); it != boost::end(*rit);
                    ++it, first = false)
                {
                    os << (first ? "M" : " L") << " "
                        << geometry::get<0>(*it)
                        << ","
                        << geometry::get<1>(*it);
                }
            }
        }
        os << " z \" style=\"" << style << "\"/></g>";

    }
};



struct prefix_linestring
{
    static inline const char* prefix() { return "polyline"; }
    static inline const char* style() { return ";fill:none"; }
};


struct prefix_ring
{
    static inline const char* prefix() { return "polygon"; }
    static inline const char* style() { return ""; }
};


template <typename MultiGeometry, typename Policy>
struct svg_multi
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
        MultiGeometry const& multi, std::string const& style, double size)
    {
        for (typename boost::range_iterator<MultiGeometry const>::type
                    it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            Policy::apply(os, *it, style, size);
        }

    }

};


}} // namespace detail::svg
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

/*!
\brief Dispatching base struct for SVG streaming, specialized below per geometry type
\details Specializations should implement a static method "stream" to stream a geometry
The static method should have the signature:

template <typename Char, typename Traits>
static inline void apply(std::basic_ostream<Char, Traits>& os, G const& geometry)
*/
template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct svg
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for this Geometry type.",
        Geometry, Tag);
};

template <typename Point>
struct svg<Point, point_tag> : detail::svg::svg_point<Point> {};

template <typename Segment>
struct svg<Segment, segment_tag> : detail::svg::svg_segment<Segment> {};

template <typename Box>
struct svg<Box, box_tag> : detail::svg::svg_box<Box> {};

template <typename Linestring>
struct svg<Linestring, linestring_tag>
    : detail::svg::svg_range<Linestring, detail::svg::prefix_linestring> {};

template <typename Ring>
struct svg<Ring, ring_tag>
    : detail::svg::svg_range<Ring, detail::svg::prefix_ring> {};

template <typename Polygon>
struct svg<Polygon, polygon_tag>
    : detail::svg::svg_poly<Polygon> {};

template <typename MultiPoint>
struct svg<MultiPoint, multi_point_tag>
    : detail::svg::svg_multi
        <
            MultiPoint,
            detail::svg::svg_point
                <
                    typename boost::range_value<MultiPoint>::type
                >

        >
{};

template <typename MultiLinestring>
struct svg<MultiLinestring, multi_linestring_tag>
    : detail::svg::svg_multi
        <
            MultiLinestring,
            detail::svg::svg_range
                <
                    typename boost::range_value<MultiLinestring>::type,
                    detail::svg::prefix_linestring
                >

        >
{};

template <typename MultiPolygon>
struct svg<MultiPolygon, multi_polygon_tag>
    : detail::svg::svg_multi
        <
            MultiPolygon,
            detail::svg::svg_poly
                <
                    typename boost::range_value<MultiPolygon>::type
                >

        >
{};


template <typename Geometry>
struct devarianted_svg
{
    template <typename OutputStream>
    static inline void apply(OutputStream& os,
                             Geometry const& geometry,
                             std::string const& style,
                             double size)
    {
        svg<Geometry>::apply(os, geometry, style, size);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct devarianted_svg<variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename OutputStream>
    struct visitor: static_visitor<void>
    {
        OutputStream& m_os;
        std::string const& m_style;
        double m_size;

        visitor(OutputStream& os, std::string const& style, double size)
            : m_os(os)
            , m_style(style)
            , m_size(size)
        {}

        template <typename Geometry>
        inline void operator()(Geometry const& geometry) const
        {
            devarianted_svg<Geometry>::apply(m_os, geometry, m_style, m_size);
        }
    };

    template <typename OutputStream>
    static inline void apply(
        OutputStream& os,
        variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
        std::string const& style,
        double size
    )
    {
        boost::apply_visitor(visitor<OutputStream>(os, style, size), geometry);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Generic geometry template manipulator class, takes corresponding output class from traits class
\ingroup svg
\details Stream manipulator, streams geometry classes as SVG (Scalable Vector Graphics)
*/
template <typename Geometry>
class svg_manipulator
{
public:

    inline svg_manipulator(Geometry const& g, std::string const& style, double size)
        : m_geometry(g)
        , m_style(style)
        , m_size(size)
    {}

    template <typename Char, typename Traits>
    inline friend std::basic_ostream<Char, Traits>& operator<<(
                    std::basic_ostream<Char, Traits>& os, svg_manipulator const& m)
    {
        dispatch::devarianted_svg<Geometry>::apply(os,
                                                   m.m_geometry,
                                                   m.m_style,
                                                   m.m_size);
        os.flush();
        return os;
    }

private:
    Geometry const& m_geometry;
    std::string const& m_style;
    double m_size;
};

/*!
\brief Manipulator to stream geometries as SVG
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param style String containing verbatim SVG style information
\param size Optional size (used for SVG points) in SVG pixels. For linestrings,
    specify linewidth in the SVG style information
\ingroup svg
*/
template <typename Geometry>
inline svg_manipulator<Geometry> svg(Geometry const& geometry,
            std::string const& style, double size = -1.0)
{
    concepts::check<Geometry const>();

    return svg_manipulator<Geometry>(geometry, style, size);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_SVG_WRITE_HPP

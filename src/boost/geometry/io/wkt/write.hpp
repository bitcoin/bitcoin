// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2017 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2017 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2017 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2017 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2020 Baidyanath Kundu, Haldia, India.

// This file was modified by Oracle on 2015-2021.
// Modifications copyright (c) 2015-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKT_WRITE_HPP
#define BOOST_GEOMETRY_IO_WKT_WRITE_HPP

#include <ostream>
#include <string>

#include <boost/array.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_point.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/geometries/ring.hpp>

#include <boost/geometry/io/wkt/detail/prefix.hpp>

#include <boost/geometry/strategies/io/cartesian.hpp>
#include <boost/geometry/strategies/io/geographic.hpp>
#include <boost/geometry/strategies/io/spherical.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

// Silence warning C4512: 'boost::geometry::wkt_manipulator<Geometry>' : assignment operator could not be generated
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4512)  
#endif

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkt
{

template <typename P, int I, int Count>
struct stream_coordinate
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        os << (I > 0 ? " " : "") << get<I>(p);
        stream_coordinate<P, I + 1, Count>::apply(os, p);
    }
};

template <typename P, int Count>
struct stream_coordinate<P, Count, Count>
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>&, P const&)
    {}
};

struct prefix_linestring_par
{
    static inline const char* apply() { return "LINESTRING("; }
};

struct prefix_ring_par_par
{
    // Note, double parentheses are intentional, indicating WKT ring begin/end
    static inline const char* apply() { return "POLYGON(("; }
};

struct opening_parenthesis
{
    static inline const char* apply() { return "("; }
};

struct closing_parenthesis
{
    static inline const char* apply() { return ")"; }
};

struct double_closing_parenthesis
{
    static inline const char* apply() { return "))"; }
};

/*!
\brief Stream points as \ref WKT
*/
template <typename Point, typename Policy>
struct wkt_point
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os, Point const& p, bool)
    {
        os << Policy::apply() << "(";
        stream_coordinate<Point, 0, dimension<Point>::type::value>::apply(os, p);
        os << ")";
    }
};

/*!
\brief Stream ranges as WKT
\note policy is used to stream prefix/postfix, enabling derived classes to override this
*/
template
<
    typename Range,
    bool ForceClosurePossible,
    typename PrefixPolicy,
    typename SuffixPolicy
>
struct wkt_range
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Range const& range, bool force_closure = ForceClosurePossible)
    {
        typedef typename boost::range_iterator<Range const>::type iterator_type;

        typedef stream_coordinate
            <
                point_type, 0, dimension<point_type>::type::value
            > stream_type;

        bool first = true;

        os << PrefixPolicy::apply();

        // TODO: check EMPTY here

        iterator_type begin = boost::begin(range);
        iterator_type end = boost::end(range);
        for (iterator_type it = begin; it != end; ++it)
        {
            os << (first ? "" : ",");
            stream_type::apply(os, *it);
            first = false;
        }

        // optionally, close range to ring by repeating the first point
        if (BOOST_GEOMETRY_CONDITION(ForceClosurePossible)
            && force_closure
            && boost::size(range) > 1
            && wkt_range::disjoint(*begin, *(end - 1)))
        {
            os << ",";
            stream_type::apply(os, *begin);
        }

        os << SuffixPolicy::apply();
    }


private:
    typedef typename boost::range_value<Range>::type point_type;

    static inline bool disjoint(point_type const& p1, point_type const& p2)
    {
        // TODO: pass strategy
        typedef typename strategies::io::services::default_strategy
            <
                point_type
            >::type strategy_type;

        return detail::disjoint::disjoint_point_point(p1, p2, strategy_type());
    }
};

/*!
\brief Stream sequence of points as WKT-part, e.g. (1 2),(3 4)
\note Used in polygon, all multi-geometries
*/
template <typename Range, bool ForceClosurePossible = true>
struct wkt_sequence
    : wkt_range
        <
            Range,
            ForceClosurePossible,
            opening_parenthesis,
            closing_parenthesis
        >
{};

template <typename Polygon, typename PrefixPolicy>
struct wkt_poly
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Polygon const& poly, bool force_closure)
    {
        typedef typename ring_type<Polygon const>::type ring;

        os << PrefixPolicy::apply();
        // TODO: check EMPTY here
        os << "(";
        wkt_sequence<ring>::apply(os, exterior_ring(poly), force_closure);

        typename interior_return_type<Polygon const>::type
            rings = interior_rings(poly);
        for (typename detail::interior_iterator<Polygon const>::type
                it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            os << ",";
            wkt_sequence<ring>::apply(os, *it, force_closure);
        }
        os << ")";
    }

};

template <typename Multi, typename StreamPolicy, typename PrefixPolicy>
struct wkt_multi
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Multi const& geometry, bool force_closure)
    {
        os << PrefixPolicy::apply();
        // TODO: check EMPTY here
        os << "(";

        for (typename boost::range_iterator<Multi const>::type
                    it = boost::begin(geometry);
            it != boost::end(geometry);
            ++it)
        {
            if (it != boost::begin(geometry))
            {
                os << ",";
            }
            StreamPolicy::apply(os, *it, force_closure);
        }

        os << ")";
    }
};

template <typename Box>
struct wkt_box
{
    typedef typename point_type<Box>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Box const& box, bool force_closure)
    {
        // Convert to a clockwire ring, then stream.
        // Never close it based on last point (box might be empty and
        // that should result in POLYGON((0 0,0 0,0 0,0 0, ...)) )
        if (force_closure)
        {
            do_apply<model::ring<point_type, true, true> >(os, box);
        }
        else
        {
            do_apply<model::ring<point_type, true, false> >(os, box);
        }
    }

    private:

        inline wkt_box()
        {
            // Only streaming of boxes with two dimensions is support, otherwise it is a polyhedron!
            //assert_dimension<B, 2>();
        }

        template <typename RingType, typename Char, typename Traits>
        static inline void do_apply(std::basic_ostream<Char, Traits>& os,
                    Box const& box)
        {
            RingType ring;
            geometry::convert(box, ring);
            os << "POLYGON(";
            wkt_sequence<RingType, false>::apply(os, ring);
            os << ")";
        }

};


template <typename Segment>
struct wkt_segment
{
    typedef typename point_type<Segment>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Segment const& segment, bool)
    {
        // Convert to two points, then stream
        typedef boost::array<point_type, 2> sequence;

        sequence points;
        geometry::detail::assign_point_from_index<0>(segment, points[0]);
        geometry::detail::assign_point_from_index<1>(segment, points[1]);

        // In Boost.Geometry a segment is represented
        // in WKT-format like (for 2D): LINESTRING(x y,x y)
        os << "LINESTRING";
        wkt_sequence<sequence, false>::apply(os, points);
    }

    private:

        inline wkt_segment()
        {}
};

}} // namespace detail::wkt
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct wkt: not_implemented<Tag>
{};

template <typename Point>
struct wkt<Point, point_tag>
    : detail::wkt::wkt_point
        <
            Point,
            detail::wkt::prefix_point
        >
{};

template <typename Linestring>
struct wkt<Linestring, linestring_tag>
    : detail::wkt::wkt_range
        <
            Linestring,
            false,
            detail::wkt::prefix_linestring_par,
            detail::wkt::closing_parenthesis
        >
{};

/*!
\brief Specialization to stream a box as WKT
\details A "box" does not exist in WKT.
It is therefore streamed as a polygon
*/
template <typename Box>
struct wkt<Box, box_tag>
    : detail::wkt::wkt_box<Box>
{};

template <typename Segment>
struct wkt<Segment, segment_tag>
    : detail::wkt::wkt_segment<Segment>
{};

/*!
\brief Specialization to stream a ring as WKT
\details A ring or "linear_ring" does not exist in WKT.
A ring is equivalent to a polygon without inner rings
It is therefore streamed as a polygon
*/
template <typename Ring>
struct wkt<Ring, ring_tag>
    : detail::wkt::wkt_range
        <
            Ring,
            true,
            detail::wkt::prefix_ring_par_par,
            detail::wkt::double_closing_parenthesis
        >
{};

/*!
\brief Specialization to stream polygon as WKT
*/
template <typename Polygon>
struct wkt<Polygon, polygon_tag>
    : detail::wkt::wkt_poly
        <
            Polygon,
            detail::wkt::prefix_polygon
        >
{};

template <typename Multi>
struct wkt<Multi, multi_point_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_point
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipoint
        >
{};

template <typename Multi>
struct wkt<Multi, multi_linestring_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_sequence
                <
                    typename boost::range_value<Multi>::type,
                    false
                >,
            detail::wkt::prefix_multilinestring
        >
{};

template <typename Multi>
struct wkt<Multi, multi_polygon_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_poly
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipolygon
        >
{};


template <typename Geometry>
struct wkt<Geometry, dynamic_geometry_tag>
{
    template <typename OutputStream>
    static inline void apply(OutputStream& os, Geometry const& geometry,
                             bool force_closure)
    {
        traits::visit<Geometry>::apply([&](auto const& g)
        {
            wkt<util::remove_cref_t<decltype(g)>>::apply(os, g, force_closure);
        }, geometry);
    }
};

// TODO: Implement non-recursive version
template <typename Geometry>
struct wkt<Geometry, geometry_collection_tag>
{
    template <typename OutputStream>
    static inline void apply(OutputStream& os, Geometry const& geometry,
                             bool force_closure)
    {
        output_or_recursive_call(os, geometry, force_closure);
    }

    template
    <
        typename OutputStream, typename Geom,
        std::enable_if_t<util::is_geometry_collection<Geom>::value, int> = 0
    >
    static void output_or_recursive_call(OutputStream& os, Geom const& geom, bool force_closure)
    {
        os << "GEOMETRYCOLLECTION(";

        bool first = true;
        auto const end = boost::end(geom);
        for (auto it = boost::begin(geom); it != end; ++it)
        {
            if (first)
                first = false;
            else
                os << ',';

            traits::iter_visit<Geom>::apply([&](auto const& g)
            {
                output_or_recursive_call(os, g, force_closure);
            }, it);
        }

        os << ')';
    }

    template
    <
        typename OutputStream, typename Geom,
        std::enable_if_t<! util::is_geometry_collection<Geom>::value, int> = 0
    >
    static void output_or_recursive_call(OutputStream& os, Geom const& geom, bool force_closure)
    {
        wkt<Geom>::apply(os, geom, force_closure);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
\brief Generic geometry template manipulator class, takes corresponding output class from traits class
\ingroup wkt
\details Stream manipulator, streams geometry classes as \ref WKT streams
\par Example:
Small example showing how to use the wkt class
\dontinclude doxygen_1.cpp
\skip example_as_wkt_point
\line {
\until }
*/
template <typename Geometry>
class wkt_manipulator
{
    static const bool is_ring = util::is_ring<Geometry>::value;

public:

    // Boost.Geometry, by default, closes polygons explictly, but not rings
    // NOTE: this might change in the future!
    inline wkt_manipulator(Geometry const& g,
                           bool force_closure = ! is_ring)
        : m_geometry(g)
        , m_force_closure(force_closure)
    {}

    template <typename Char, typename Traits>
    inline friend std::basic_ostream<Char, Traits>& operator<<(
            std::basic_ostream<Char, Traits>& os,
            wkt_manipulator const& m)
    {
        dispatch::wkt<Geometry>::apply(os, m.m_geometry, m.m_force_closure);
        os.flush();
        return os;
    }

private:
    Geometry const& m_geometry;
    bool m_force_closure;
};

/*!
\brief Main WKT-streaming function
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\ingroup wkt
\qbk{[include reference/io/wkt.qbk]}
*/
template <typename Geometry>
inline wkt_manipulator<Geometry> wkt(Geometry const& geometry)
{
    concepts::check<Geometry const>();

    return wkt_manipulator<Geometry>(geometry);
}

/*!
\brief WKT-string formulating function
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param significant_digits Specifies the no of significant digits to use in the output wkt
\ingroup wkt
\qbk{[include reference/io/to_wkt.qbk]}
*/
template <typename Geometry>
inline std::string to_wkt(Geometry const& geometry)
{
    std::stringstream ss;
    ss << boost::geometry::wkt(geometry);
    return ss.str();
}

template <typename Geometry>
inline std::string to_wkt(Geometry const& geometry, int significant_digits)
{
    std::stringstream ss;
    ss.precision(significant_digits);
    ss << boost::geometry::wkt(geometry);
    return ss.str();
}

#if defined(_MSC_VER)
#pragma warning(pop)  
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_WKT_WRITE_HPP

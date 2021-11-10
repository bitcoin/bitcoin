// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2020 Baidyanath Kundu, Haldia, India

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKT_READ_HPP
#define BOOST_GEOMETRY_IO_WKT_READ_HPP

#include <cstddef>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
#include <boost/throw_exception.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_point.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For consistency with other functions
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/io/wkt/detail/prefix.hpp>

#include <boost/geometry/strategies/io/cartesian.hpp>
#include <boost/geometry/strategies/io/geographic.hpp>
#include <boost/geometry/strategies/io/spherical.hpp>

#include <boost/geometry/util/coordinate_cast.hpp>
#include <boost/geometry/util/range.hpp>
#include <boost/geometry/util/sequence.hpp>
#include <boost/geometry/util/type_traits.hpp>

namespace boost { namespace geometry
{

/*!
\brief Exception showing things wrong with WKT parsing
\ingroup wkt
*/
struct read_wkt_exception : public geometry::exception
{
    template <typename Iterator>
    read_wkt_exception(std::string const& msg,
                       Iterator const& it,
                       Iterator const& end,
                       std::string const& wkt)
        : message(msg)
        , wkt(wkt)
    {
        if (it != end)
        {
            source = " at '";
            source += it->c_str();
            source += "'";
        }
        complete = message + source + " in '" + wkt.substr(0, 100) + "'";
    }

    read_wkt_exception(std::string const& msg, std::string const& wkt)
        : message(msg)
        , wkt(wkt)
    {
        complete = message + "' in (" + wkt.substr(0, 100) + ")";
    }

    virtual ~read_wkt_exception() throw() {}

    virtual const char* what() const throw()
    {
        return complete.c_str();
    }
private :
    std::string source;
    std::string message;
    std::string wkt;
    std::string complete;
};


#ifndef DOXYGEN_NO_DETAIL
// (wkt: Well Known Text, defined by OGC for all geometries and implemented by e.g. databases (MySQL, PostGIS))
namespace detail { namespace wkt
{

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

template <typename Point,
          std::size_t Dimension = 0,
          std::size_t DimensionCount = geometry::dimension<Point>::value>
struct parsing_assigner
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             Point& point,
                             std::string const& wkt)
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        // Stop at end of tokens, or at "," ot ")"
        bool finished = (it == end || *it == "," || *it == ")");

        try
        {
            // Initialize missing coordinates to default constructor (zero)
            // OR
            // Use lexical_cast for conversion to double/int
            // Note that it is much slower than atof. However, it is more standard
            // and in parsing the change in performance falls probably away against
            // the tokenizing
            set<Dimension>(point, finished
                    ? coordinate_type()
                    : coordinate_cast<coordinate_type>::apply(*it));
        }
        catch(boost::bad_lexical_cast const& blc)
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception(blc.what(), it, end, wkt));
        }
        catch(std::exception const& e)
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception(e.what(), it, end, wkt));
        }
        catch(...)
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception("", it, end, wkt));
        }

        parsing_assigner<Point, Dimension + 1, DimensionCount>::apply(
                        (finished ? it : ++it), end, point, wkt);
    }
};

template <typename Point, std::size_t DimensionCount>
struct parsing_assigner<Point, DimensionCount, DimensionCount>
{
    static inline void apply(tokenizer::iterator&,
                             tokenizer::iterator const&,
                             Point&,
                             std::string const&)
    {
    }
};



template <typename Iterator>
inline void handle_open_parenthesis(Iterator& it,
                                    Iterator const& end,
                                    std::string const& wkt)
{
    if (it == end || *it != "(")
    {
        BOOST_THROW_EXCEPTION(read_wkt_exception("Expected '('", it, end, wkt));
    }
    ++it;
}


template <typename Iterator>
inline void handle_close_parenthesis(Iterator& it,
                                     Iterator const& end,
                                     std::string const& wkt)
{
    if (it != end && *it == ")")
    {
        ++it;
    }
    else
    {
        BOOST_THROW_EXCEPTION(read_wkt_exception("Expected ')'", it, end, wkt));
    }
}

template <typename Iterator>
inline void check_end(Iterator& it,
                      Iterator const& end,
                      std::string const& wkt)
{
    if (it != end)
    {
        BOOST_THROW_EXCEPTION(read_wkt_exception("Too many tokens", it, end, wkt));
    }
}

/*!
\brief Internal, parses coordinate sequences, strings are formated like "(1 2,3 4,...)"
\param it token-iterator, should be pre-positioned at "(", is post-positions after last ")"
\param end end-token-iterator
\param out Output itererator receiving coordinates
*/
template <typename Point>
struct container_inserter
{
    // Version with output iterator
    template <typename OutputIterator>
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             OutputIterator out)
    {
        handle_open_parenthesis(it, end, wkt);

        Point point;

        // Parse points until closing parenthesis

        while (it != end && *it != ")")
        {
            parsing_assigner<Point>::apply(it, end, point, wkt);
            out = point;
            ++out;
            if (it != end && *it == ",")
            {
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};


template <typename Geometry,
          closure_selector Closure = closure<Geometry>::value>
struct stateful_range_appender
{
    typedef typename geometry::point_type<Geometry>::type point_type;

    // NOTE: Geometry is a reference
    inline void append(Geometry geom, point_type const& point, bool)
    {
        geometry::append(geom, point);
    }
};

template <typename Geometry>
struct stateful_range_appender<Geometry, open>
{
    typedef typename geometry::point_type<Geometry>::type point_type;
    typedef typename boost::range_size
        <
            typename util::remove_cptrref<Geometry>::type
        >::type size_type;

    BOOST_STATIC_ASSERT(( util::is_ring<Geometry>::value ));

    inline stateful_range_appender()
        : pt_index(0)
    {}

    // NOTE: Geometry is a reference
    inline void append(Geometry geom, point_type const& point, bool is_next_expected)
    {
        bool should_append = true;

        if (pt_index == 0)
        {
            first_point = point;
            //should_append = true;
        }
        else
        {
            // NOTE: if there is not enough Points, they're always appended
            should_append
                = is_next_expected
                || pt_index < core_detail::closure::minimum_ring_size<open>::value
                || disjoint(point, first_point);
        }
        ++pt_index;

        if (should_append)
        {
            geometry::append(geom, point);
        }
    }

private:
    static inline bool disjoint(point_type const& p1, point_type const& p2)
    {
        // TODO: pass strategy
        typedef typename strategies::io::services::default_strategy
            <
                point_type
            >::type strategy_type;

        return detail::disjoint::disjoint_point_point(p1, p2, strategy_type());
    }

    size_type pt_index;
    point_type first_point;
};

// Geometry is a value-type or reference-type
template <typename Geometry>
struct container_appender
{
    typedef typename geometry::point_type<Geometry>::type point_type;

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Geometry out)
    {
        handle_open_parenthesis(it, end, wkt);

        stateful_range_appender<Geometry> appender;

        // Parse points until closing parenthesis
        while (it != end && *it != ")")
        {
            point_type point;

            parsing_assigner<point_type>::apply(it, end, point, wkt);

            bool const is_next_expected = it != end && *it == ",";

            appender.append(out, point, is_next_expected);

            if (is_next_expected)
            {
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};

/*!
\brief Internal, parses a point from a string like this "(x y)"
\note used for parsing points and multi-points
*/
template <typename P>
struct point_parser
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             P& point)
    {
        handle_open_parenthesis(it, end, wkt);
        parsing_assigner<P>::apply(it, end, point, wkt);
        handle_close_parenthesis(it, end, wkt);
    }
};


template <typename Geometry>
struct linestring_parser
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Geometry& geometry)
    {
        container_appender<Geometry&>::apply(it, end, wkt, geometry);
    }
};


template <typename Ring>
struct ring_parser
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Ring& ring)
    {
        // A ring should look like polygon((x y,x y,x y...))
        // So handle the extra opening/closing parentheses
        // and in between parse using the container-inserter
        handle_open_parenthesis(it, end, wkt);
        container_appender<Ring&>::apply(it, end, wkt, ring);
        handle_close_parenthesis(it, end, wkt);
    }
};


/*!
\brief Internal, parses a polygon from a string like this "((x y,x y),(x y,x y))"
\note used for parsing polygons and multi-polygons
*/
template <typename Polygon>
struct polygon_parser
{
    typedef typename ring_return_type<Polygon>::type ring_return_type;
    typedef container_appender<ring_return_type> appender;

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Polygon& poly)
    {

        handle_open_parenthesis(it, end, wkt);

        int n = -1;

        // Stop at ")"
        while (it != end && *it != ")")
        {
            // Parse ring
            if (++n == 0)
            {
                appender::apply(it, end, wkt, exterior_ring(poly));
            }
            else
            {
                typename ring_type<Polygon>::type ring;
                appender::apply(it, end, wkt, ring);
                range::push_back(geometry::interior_rings(poly), std::move(ring));
            }

            if (it != end && *it == ",")
            {
                // Skip "," after ring is parsed
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};


inline bool one_of(tokenizer::iterator const& it,
                   std::string const& value,
                   bool& is_present)
{
    if (boost::iequals(*it, value))
    {
        is_present = true;
        return true;
    }
    return false;
}

inline bool one_of(tokenizer::iterator const& it,
                   std::string const& value,
                   bool& present1,
                   bool& present2)
{
    if (boost::iequals(*it, value))
    {
        present1 = true;
        present2 = true;
        return true;
    }
    return false;
}


inline void handle_empty_z_m(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             bool& has_empty,
                             bool& has_z,
                             bool& has_m)
{
    has_empty = false;
    has_z = false;
    has_m = false;

    // WKT can optionally have Z and M (measured) values as in
    // POINT ZM (1 1 5 60), POINT M (1 1 80), POINT Z (1 1 5)
    // GGL supports any of them as coordinate values, but is not aware
    // of any Measured value.
    while (it != end
           && (one_of(it, "M", has_m)
               || one_of(it, "Z", has_z)
               || one_of(it, "EMPTY", has_empty)
               || one_of(it, "MZ", has_m, has_z)
               || one_of(it, "ZM", has_z, has_m)
               )
           )
    {
        ++it;
    }
}


template <typename Geometry, typename Tag = typename geometry::tag<Geometry>::type>
struct dimension
    : geometry::dimension<Geometry>
{};

// TODO: For now assume the dimension of the first type defined for GC
//       This should probably be unified for all algorithms
template <typename Geometry>
struct dimension<Geometry, geometry_collection_tag>
    : geometry::dimension
        <
            typename util::sequence_front
                <
                    typename traits::geometry_types<Geometry>::type
                >::type
        >
{};


/*!
\brief Internal, starts parsing
\param geometry_name string to compare with first token
*/
template <typename Geometry>
inline bool initialize(tokenizer::iterator& it,
                       tokenizer::iterator const& end,
                       std::string const& wkt,
                       std::string const& geometry_name)
{
    if (it == end || ! boost::iequals(*it++, geometry_name))
    {
        BOOST_THROW_EXCEPTION(read_wkt_exception(std::string("Should start with '") + geometry_name + "'", wkt));
    }

    bool has_empty, has_z, has_m;

    handle_empty_z_m(it, end, has_empty, has_z, has_m);

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4127)  
#endif

    if (has_z && dimension<Geometry>::value < 3)
    {
        BOOST_THROW_EXCEPTION(read_wkt_exception("Z only allowed for 3 or more dimensions", wkt));
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    if (has_empty)
    {
        return false;
    }
    // M is ignored at all.
    
    return true;
}


template <typename Geometry, template<typename> class Parser, typename PrefixPolicy>
struct geometry_parser
{
    static inline void apply(std::string const& wkt, Geometry& geometry)
    {
        geometry::clear(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator const end = tokens.end();

        apply(it, end, wkt, geometry);

        check_end(it, end, wkt);
    }

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Geometry& geometry)
    {
        if (initialize<Geometry>(it, end, wkt, PrefixPolicy::apply()))
        {
            Parser<Geometry>::apply(it, end, wkt, geometry);
        }
    }
};


template <typename MultiGeometry, template<typename> class Parser, typename PrefixPolicy>
struct multi_parser
{
    static inline void apply(std::string const& wkt, MultiGeometry& geometry)
    {
        traits::clear<MultiGeometry>::apply(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator const end = tokens.end();

        apply(it, end, wkt, geometry);

        check_end(it, end, wkt);
    }

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             MultiGeometry& geometry)
    {
        if (initialize<MultiGeometry>(it, end, wkt, PrefixPolicy::apply()))
        {
            handle_open_parenthesis(it, end, wkt);

            // Parse sub-geometries
            while(it != end && *it != ")")
            {
                traits::resize<MultiGeometry>::apply(geometry, boost::size(geometry) + 1);
                Parser
                    <
                        typename boost::range_value<MultiGeometry>::type
                    >::apply(it, end, wkt, *(boost::end(geometry) - 1));
                if (it != end && *it == ",")
                {
                    // Skip "," after multi-element is parsed
                    ++it;
                }
            }

            handle_close_parenthesis(it, end, wkt);
        }
    }
};

template <typename P>
struct noparenthesis_point_parser
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             P& point)
    {
        parsing_assigner<P>::apply(it, end, point, wkt);
    }
};

template <typename MultiGeometry, typename PrefixPolicy>
struct multi_point_parser
{
    static inline void apply(std::string const& wkt, MultiGeometry& geometry)
    {
        traits::clear<MultiGeometry>::apply(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator const end = tokens.end();

        apply(it, end, wkt, geometry);

        check_end(it, end, wkt);
    }

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             MultiGeometry& geometry)
    {
        if (initialize<MultiGeometry>(it, end, wkt, PrefixPolicy::apply()))
        {
            handle_open_parenthesis(it, end, wkt);

            // If first point definition starts with "(" then parse points as (x y)
            // otherwise as "x y"
            bool using_brackets = (it != end && *it == "(");

            while(it != end && *it != ")")
            {
                traits::resize<MultiGeometry>::apply(geometry, boost::size(geometry) + 1);

                if (using_brackets)
                {
                    point_parser
                        <
                            typename boost::range_value<MultiGeometry>::type
                        >::apply(it, end, wkt, *(boost::end(geometry) - 1));
                }
                else
                {
                    noparenthesis_point_parser
                        <
                            typename boost::range_value<MultiGeometry>::type
                        >::apply(it, end, wkt, *(boost::end(geometry) - 1));
                }

                if (it != end && *it == ",")
                {
                    // Skip "," after point is parsed
                    ++it;
                }
            }

            handle_close_parenthesis(it, end, wkt);
        }
    }
};


/*!
\brief Supports box parsing
\note OGC does not define the box geometry, and WKT does not support boxes.
    However, to be generic GGL supports reading and writing from and to boxes.
    Boxes are outputted as a standard POLYGON. GGL can read boxes from
    a standard POLYGON, from a POLYGON with 2 points of from a BOX
\tparam Box the box
*/
template <typename Box>
struct box_parser
{
    static inline void apply(std::string const& wkt, Box& box)
    {
        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator end = tokens.end();

        apply(it, end, wkt, box);

        check_end(it, end, wkt);
    }

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Box& box)
    {
        bool should_close = false;
        if (it != end && boost::iequals(*it, "POLYGON"))
        {
            ++it;
            bool has_empty, has_z, has_m;
            handle_empty_z_m(it, end, has_empty, has_z, has_m);
            if (has_empty)
            {
                assign_zero(box);
                return;
            }
            handle_open_parenthesis(it, end, wkt);
            should_close = true;
        }
        else if (it != end && boost::iequals(*it, "BOX"))
        {
            ++it;
        }
        else
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception("Should start with 'POLYGON' or 'BOX'", wkt));
        }

        typedef typename point_type<Box>::type point_type;
        std::vector<point_type> points;
        container_inserter<point_type>::apply(it, end, wkt, std::back_inserter(points));

        if (should_close)
        {
            handle_close_parenthesis(it, end, wkt);
        }

        unsigned int index = 0;
        std::size_t n = boost::size(points);
        if (n == 2)
        {
            index = 1;
        }
        else if (n == 4 || n == 5)
        {
            // In case of 4 or 5 points, we do not check the other ones, just
            // take the opposite corner which is always 2
            index = 2;
        }
        else
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception("Box should have 2,4 or 5 points", wkt));
        }

        geometry::detail::assign_point_to_index<min_corner>(points.front(), box);
        geometry::detail::assign_point_to_index<max_corner>(points[index], box);
    }
};


/*!
\brief Supports segment parsing
\note OGC does not define the segment, and WKT does not support segmentes.
    However, it is useful to implement it, also for testing purposes
\tparam Segment the segment
*/
template <typename Segment>
struct segment_parser
{
    static inline void apply(std::string const& wkt, Segment& segment)
    {
        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator end = tokens.end();

        apply(it, end, wkt, segment);

        check_end(it, end, wkt);
    }

    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Segment& segment)
    {
        if (it != end && (boost::iequals(*it, "SEGMENT") || boost::iequals(*it, "LINESTRING")))
        {
            ++it;
        }
        else
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception("Should start with 'LINESTRING' or 'SEGMENT'", wkt));
        }

        typedef typename point_type<Segment>::type point_type;
        std::vector<point_type> points;
        container_inserter<point_type>::apply(it, end, wkt, std::back_inserter(points));

        if (boost::size(points) == 2)
        {
            geometry::detail::assign_point_to_index<0>(points.front(), segment);
            geometry::detail::assign_point_to_index<1>(points.back(), segment);
        }
        else
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception("Segment should have 2 points", wkt));
        }
    }
};


struct dynamic_move_assign
{
    template <typename DynamicGeometry, typename Geometry>
    static void apply(DynamicGeometry& dynamic_geometry, Geometry & geometry)
    {
        dynamic_geometry = std::move(geometry);
    }
};

struct dynamic_move_emplace_back
{
    template <typename GeometryCollection, typename Geometry>
    static void apply(GeometryCollection& geometry_collection, Geometry & geometry)
    {
        traits::emplace_back<GeometryCollection>::apply(geometry_collection, std::move(geometry));
    }
};

template
<
    typename Geometry,
    template <typename, typename> class ReadWkt,
    typename AppendPolicy
>
struct dynamic_readwkt_caller
{
    static inline void apply(tokenizer::iterator& it,
                             tokenizer::iterator const& end,
                             std::string const& wkt,
                             Geometry& geometry)
    {
        if (boost::iequals(*it, "POINT"))
        {
            parse_geometry<util::is_point>("POINT", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "MULTIPOINT"))
        {
            parse_geometry<util::is_multi_point>("MULTIPOINT", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "SEGMENT"))
        {
            parse_geometry<util::is_segment>("SEGMENT", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "LINESTRING"))
        {
            parse_geometry<util::is_linestring>("LINESTRING", it, end, wkt, geometry, false)
            || parse_geometry<util::is_segment>("LINESTRING", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "MULTILINESTRING"))
        {
            parse_geometry<util::is_multi_linestring>("MULTILINESTRING", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "BOX"))
        {
            parse_geometry<util::is_box>("BOX", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "POLYGON"))
        {
            parse_geometry<util::is_polygon>("POLYGON", it, end, wkt, geometry, false)
            || parse_geometry<util::is_ring>("POLYGON", it, end, wkt, geometry, false)
            || parse_geometry<util::is_box>("POLYGON", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "MULTIPOLYGON"))
        {
            parse_geometry<util::is_multi_polygon>("MULTIPOLYGON", it, end, wkt, geometry);
        }
        else if (boost::iequals(*it, "GEOMETRYCOLLECTION"))
        {
            parse_geometry<util::is_geometry_collection>("GEOMETRYCOLLECTION", it, end, wkt, geometry);
        }
        else
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception(
                "Should start with geometry's name, e.g. 'POINT', 'LINESTRING', 'POLYGON', etc.",
                wkt));
        }
    }

private:
    template
    <
        template <typename> class UnaryPred,
        typename Geom = typename util::sequence_find_if
            <
                typename traits::geometry_types<Geometry>::type, UnaryPred
            >::type,
        std::enable_if_t<! std::is_void<Geom>::value, int> = 0
    >
    static bool parse_geometry(const char * ,
                               tokenizer::iterator& it,
                               tokenizer::iterator const& end,
                               std::string const& wkt,
                               Geometry& geometry,
                               bool = true)
    {
        Geom g;
        ReadWkt<Geom, typename tag<Geom>::type>::apply(it, end, wkt, g);
        AppendPolicy::apply(geometry, g);
        return true;
    }

    template
    <
        template <typename> class UnaryPred,
        typename Geom = typename util::sequence_find_if
            <
                typename traits::geometry_types<Geometry>::type, UnaryPred
            >::type,
        std::enable_if_t<std::is_void<Geom>::value, int> = 0
    >
    static bool parse_geometry(const char * name,
                               tokenizer::iterator& ,
                               tokenizer::iterator const& ,
                               std::string const& wkt,
                               Geometry& ,
                               bool throw_on_misfit = true)
    {
        if (throw_on_misfit)
        {
            std::string msg = std::string("Unable to store '") + name + "' in this geometry";
            BOOST_THROW_EXCEPTION(read_wkt_exception(msg, wkt));
        }

        return false;
    }
};


}} // namespace detail::wkt
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct read_wkt {};


template <typename Point>
struct read_wkt<Point, point_tag>
    : detail::wkt::geometry_parser
        <
            Point,
            detail::wkt::point_parser,
            detail::wkt::prefix_point
        >
{};


template <typename L>
struct read_wkt<L, linestring_tag>
    : detail::wkt::geometry_parser
        <
            L,
            detail::wkt::linestring_parser,
            detail::wkt::prefix_linestring
        >
{};

template <typename Ring>
struct read_wkt<Ring, ring_tag>
    : detail::wkt::geometry_parser
        <
            Ring,
            detail::wkt::ring_parser,
            detail::wkt::prefix_polygon
        >
{};

template <typename Geometry>
struct read_wkt<Geometry, polygon_tag>
    : detail::wkt::geometry_parser
        <
            Geometry,
            detail::wkt::polygon_parser,
            detail::wkt::prefix_polygon
        >
{};


template <typename MultiGeometry>
struct read_wkt<MultiGeometry, multi_point_tag>
    : detail::wkt::multi_point_parser
            <
                MultiGeometry,
                detail::wkt::prefix_multipoint
            >
{};

template <typename MultiGeometry>
struct read_wkt<MultiGeometry, multi_linestring_tag>
    : detail::wkt::multi_parser
            <
                MultiGeometry,
                detail::wkt::linestring_parser,
                detail::wkt::prefix_multilinestring
            >
{};

template <typename MultiGeometry>
struct read_wkt<MultiGeometry, multi_polygon_tag>
    : detail::wkt::multi_parser
            <
                MultiGeometry,
                detail::wkt::polygon_parser,
                detail::wkt::prefix_multipolygon
            >
{};


// Box (Non-OGC)
template <typename Box>
struct read_wkt<Box, box_tag>
    : detail::wkt::box_parser<Box>
{};

// Segment (Non-OGC)
template <typename Segment>
struct read_wkt<Segment, segment_tag>
    : detail::wkt::segment_parser<Segment>
{};


template <typename DynamicGeometry>
struct read_wkt<DynamicGeometry, dynamic_geometry_tag>
{
    static inline void apply(std::string const& wkt, DynamicGeometry& dynamic_geometry)
    {
        detail::wkt::tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        detail::wkt::tokenizer::iterator it = tokens.begin();
        detail::wkt::tokenizer::iterator end = tokens.end();
        if (it == end)
        {
            BOOST_THROW_EXCEPTION(read_wkt_exception(
                "Should start with geometry's name, e.g. 'POINT', 'LINESTRING', 'POLYGON', etc.",
                wkt));
        }

        detail::wkt::dynamic_readwkt_caller
            <
                DynamicGeometry, dispatch::read_wkt, detail::wkt::dynamic_move_assign
            >::apply(it, end, wkt, dynamic_geometry);

        detail::wkt::check_end(it, end, wkt);
    }
};


template <typename Geometry>
struct read_wkt<Geometry, geometry_collection_tag>
{
    static inline void apply(std::string const& wkt, Geometry& geometry)
    {
        range::clear(geometry);

        detail::wkt::tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        detail::wkt::tokenizer::iterator it = tokens.begin();
        detail::wkt::tokenizer::iterator const end = tokens.end();

        apply(it, end, wkt, geometry);

        detail::wkt::check_end(it, end, wkt);
    }

    static inline void apply(detail::wkt::tokenizer::iterator& it,
                             detail::wkt::tokenizer::iterator const& end,
                             std::string const& wkt,
                             Geometry& geometry)
    {
        if (detail::wkt::initialize<Geometry>(it, end, wkt, "GEOMETRYCOLLECTION"))
        {
            detail::wkt::handle_open_parenthesis(it, end, wkt);

            // Stop at ")"
            while (it != end && *it != ")")
            {
                detail::wkt::dynamic_readwkt_caller
                    <
                        Geometry, dispatch::read_wkt, detail::wkt::dynamic_move_emplace_back
                    >::apply(it, end, wkt, geometry);

                if (it != end && *it == ",")
                {
                    // Skip "," after geometry is parsed
                    ++it;
                }
            }

            detail::wkt::handle_close_parenthesis(it, end, wkt);
        }
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
\brief Parses OGC Well-Known Text (\ref WKT) into a geometry (any geometry)
\ingroup wkt
\tparam Geometry \tparam_geometry
\param wkt string containing \ref WKT
\param geometry \param_geometry output geometry
\ingroup wkt
\qbk{[include reference/io/read_wkt.qbk]}
*/
template <typename Geometry>
inline void read_wkt(std::string const& wkt, Geometry& geometry)
{
    geometry::concepts::check<Geometry>();
    dispatch::read_wkt<Geometry>::apply(wkt, geometry);
}

/*!
\brief Parses OGC Well-Known Text (\ref WKT) into a geometry (any geometry) and returns it
\ingroup wkt
\tparam Geometry \tparam_geometry
\param wkt string containing \ref WKT
\ingroup wkt
\qbk{[include reference/io/from_wkt.qbk]}
*/
template <typename Geometry>
inline Geometry from_wkt(std::string const& wkt)
{
    Geometry geometry;
    geometry::concepts::check<Geometry>();
    dispatch::read_wkt<Geometry>::apply(wkt, geometry);
    return geometry;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_WKT_READ_HPP

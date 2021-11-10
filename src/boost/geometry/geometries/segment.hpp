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

#ifndef BOOST_GEOMETRY_GEOMETRIES_SEGMENT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_SEGMENT_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

#include <boost/concept/assert.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/make.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

namespace boost { namespace geometry
{

namespace model
{

/*!
\brief Class segment: small class containing two points
\ingroup geometries
\details From Wikipedia: In geometry, a line segment is a part of a line that is bounded
 by two distinct end points, and contains every point on the line between its end points.
\note There is also a point-referring-segment, class referring_segment,
   containing point references, where points are NOT copied

\qbk{[include reference/geometries/segment.qbk]}
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_segment Segment Concept]
}
*/
template<typename Point>
class segment : public std::pair<Point, Point>
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

public :

    /// \constructor_default_no_init
    constexpr segment() = default;

    /*!
        \brief Constructor taking the first and the second point
    */
    constexpr segment(Point const& p1, Point const& p2)
        : std::pair<Point, Point>(p1, p2)
    {}
};


/*!
\brief Class segment: small class containing two (templatized) point references
\ingroup geometries
\details From Wikipedia: In geometry, a line segment is a part of a line that is bounded
 by two distinct end points, and contains every point on the line between its end points.
\note The structure is like std::pair, and can often be used interchangeable.
Difference is that it refers to points, does not have points.
\note Like std::pair, points are public available.
\note type is const or non const, so geometry::segment<P> or geometry::segment<P const>
\note We cannot derive from std::pair<P&, P&> because of
reference assignments.
\tparam ConstOrNonConstPoint point type of the segment, maybe a point or a const point
*/
template<typename ConstOrNonConstPoint>
class referring_segment
{
    BOOST_CONCEPT_ASSERT( (
        typename std::conditional
            <
                std::is_const<ConstOrNonConstPoint>::value,
                concepts::Point<ConstOrNonConstPoint>,
                concepts::ConstPoint<ConstOrNonConstPoint>
            >
    ) );

    typedef ConstOrNonConstPoint point_type;

public:

    point_type& first;
    point_type& second;

    /*!
        \brief Constructor taking the first and the second point
    */
    inline referring_segment(point_type& p1, point_type& p2)
        : first(p1)
        , second(p2)
    {}
};


} // namespace model


// Traits specializations for segment above
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Point>
struct tag<model::segment<Point> >
{
    typedef segment_tag type;
};

template <typename Point>
struct point_type<model::segment<Point> >
{
    typedef Point type;
};

template <typename Point, std::size_t Dimension>
struct indexed_access<model::segment<Point>, 0, Dimension>
{
    typedef model::segment<Point> segment_type;
    typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

    static constexpr coordinate_type get(segment_type const& s)
    {
        return geometry::get<Dimension>(s.first);
    }

    static void set(segment_type& s, coordinate_type const& value)
    {
        geometry::set<Dimension>(s.first, value);
    }
};


template <typename Point, std::size_t Dimension>
struct indexed_access<model::segment<Point>, 1, Dimension>
{
    typedef model::segment<Point> segment_type;
    typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

    static constexpr coordinate_type get(segment_type const& s)
    {
        return geometry::get<Dimension>(s.second);
    }

    static void set(segment_type& s, coordinate_type const& value)
    {
        geometry::set<Dimension>(s.second, value);
    }
};


template <typename Point>
struct make<model::segment<Point> >
{
    typedef model::segment<Point> segment_type;

    static const bool is_specialized = true;

    static constexpr segment_type apply(Point const& p1, Point const& p2)
    {
        return segment_type(p1, p2);
    }
};



template <typename ConstOrNonConstPoint>
struct tag<model::referring_segment<ConstOrNonConstPoint> >
{
    typedef segment_tag type;
};

template <typename ConstOrNonConstPoint>
struct point_type<model::referring_segment<ConstOrNonConstPoint> >
{
    typedef ConstOrNonConstPoint type;
};

template <typename ConstOrNonConstPoint, std::size_t Dimension>
struct indexed_access<model::referring_segment<ConstOrNonConstPoint>, 0, Dimension>
{
    typedef model::referring_segment<ConstOrNonConstPoint> segment_type;
    typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

    static inline coordinate_type get(segment_type const& s)
    {
        return geometry::get<Dimension>(s.first);
    }

    static inline void set(segment_type& s, coordinate_type const& value)
    {
        geometry::set<Dimension>(s.first, value);
    }
};


template <typename ConstOrNonConstPoint, std::size_t Dimension>
struct indexed_access<model::referring_segment<ConstOrNonConstPoint>, 1, Dimension>
{
    typedef model::referring_segment<ConstOrNonConstPoint> segment_type;
    typedef typename geometry::coordinate_type<segment_type>::type coordinate_type;

    static inline coordinate_type get(segment_type const& s)
    {
        return geometry::get<Dimension>(s.second);
    }

    static inline void set(segment_type& s, coordinate_type const& value)
    {
        geometry::set<Dimension>(s.second, value);
    }
};



} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_SEGMENT_HPP

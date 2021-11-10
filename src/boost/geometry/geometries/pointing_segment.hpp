// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_POINTING_SEGMENT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POINTING_SEGMENT_HPP

#include <cstddef>
#include <type_traits>

#include <boost/concept/assert.hpp>
#include <boost/core/addressof.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

namespace boost { namespace geometry
{

namespace model
{

// const or non-const segment type that is meant to be
// * default constructible
// * copy constructible
// * assignable
// referring_segment does not fit these requirements, hence the
// pointing_segment class
//
// this class is used by the segment_iterator as its value type
template <typename ConstOrNonConstPoint>
class pointing_segment
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
    point_type* first;
    point_type* second;

    inline pointing_segment()
        : first(NULL)
        , second(NULL)
    {}

    inline pointing_segment(point_type const& p1, point_type const& p2)
        : first(boost::addressof(p1))
        , second(boost::addressof(p2))
    {}
};


} // namespace model


// Traits specializations for segment above
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Point>
struct tag<model::pointing_segment<Point> >
{
    typedef segment_tag type;
};

template <typename Point>
struct point_type<model::pointing_segment<Point> >
{
    typedef Point type;
};

template <typename Point, std::size_t Dimension>
struct indexed_access<model::pointing_segment<Point>, 0, Dimension>
{
    typedef model::pointing_segment<Point> segment_type;
    typedef typename geometry::coordinate_type
        <
            segment_type
        >::type coordinate_type;

    static inline coordinate_type get(segment_type const& s)
    {
        BOOST_GEOMETRY_ASSERT( s.first != NULL );
        return geometry::get<Dimension>(*s.first);
    }

    static inline void set(segment_type& s, coordinate_type const& value)
    {
        BOOST_GEOMETRY_ASSERT( s.first != NULL );
        geometry::set<Dimension>(*s.first, value);
    }
};


template <typename Point, std::size_t Dimension>
struct indexed_access<model::pointing_segment<Point>, 1, Dimension>
{
    typedef model::pointing_segment<Point> segment_type;
    typedef typename geometry::coordinate_type
        <
            segment_type
        >::type coordinate_type;

    static inline coordinate_type get(segment_type const& s)
    {
        BOOST_GEOMETRY_ASSERT( s.second != NULL );
        return geometry::get<Dimension>(*s.second);
    }

    static inline void set(segment_type& s, coordinate_type const& value)
    {
        BOOST_GEOMETRY_ASSERT( s.second != NULL );
        geometry::set<Dimension>(*s.second, value);
    }
};



} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POINTING_SEGMENT_HPP

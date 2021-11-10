// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2021.
// Modifications copyright (c) 2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_SEGMENT_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_SEGMENT_CONCEPT_HPP


#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
\brief Segment concept.
\ingroup concepts
\details Formal definition:
The segment concept is defined as following:
- there must be a specialization of traits::tag defining segment_tag as type
- there must be a specialization of traits::point_type to define the
  underlying point type (even if it does not consist of points, it should define
  this type, to indicate the points it can work with)
- there must be a specialization of traits::indexed_access, per index
  and per dimension, with two functions:
  - get to get a coordinate value
  - set to set a coordinate value (this one is not checked for ConstSegment)

\note The segment concept is similar to the box concept, defining another tag.
However, the box concept assumes the index as min_corner, max_corner, while
for the segment concept there is no assumption.
*/
template <typename Geometry>
class Segment
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;

    BOOST_CONCEPT_ASSERT( (concepts::Point<point_type>) );


    template <size_t Index, size_t Dimension, size_t DimensionCount>
    struct dimension_checker
    {
        static void apply()
        {
            Geometry* s = 0;
            geometry::set<Index, Dimension>(*s, geometry::get<Index, Dimension>(*s));
            dimension_checker<Index, Dimension + 1, DimensionCount>::apply();
        }
    };

    template <size_t Index, size_t DimensionCount>
    struct dimension_checker<Index, DimensionCount, DimensionCount>
    {
        static void apply() {}
    };

public :

    BOOST_CONCEPT_USAGE(Segment)
    {
        static const size_t n = dimension<point_type>::type::value;
        dimension_checker<0, 0, n>::apply();
        dimension_checker<1, 0, n>::apply();
    }
#endif
};


/*!
\brief Segment concept (const version).
\ingroup const_concepts
\details The ConstSegment concept verifies the same as the Segment concept,
but does not verify write access.
*/
template <typename Geometry>
class ConstSegment
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;
    typedef typename coordinate_type<Geometry>::type coordinate_type;

    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point_type>) );


    template <size_t Index, size_t Dimension, size_t DimensionCount>
    struct dimension_checker
    {
        static void apply()
        {
            const Geometry* s = 0;
            coordinate_type coord(geometry::get<Index, Dimension>(*s));
            boost::ignore_unused(coord);
            dimension_checker<Index, Dimension + 1, DimensionCount>::apply();
        }
    };

    template <size_t Index, size_t DimensionCount>
    struct dimension_checker<Index, DimensionCount, DimensionCount>
    {
        static void apply() {}
    };

public :

    BOOST_CONCEPT_USAGE(ConstSegment)
    {
        static const size_t n = dimension<point_type>::type::value;
        dimension_checker<0, 0, n>::apply();
        dimension_checker<1, 0, n>::apply();
    }
#endif
};


template <typename Geometry>
struct concept_type<Geometry, segment_tag>
{
    using type = Segment<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, segment_tag>
{
    using type = ConstSegment<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_SEGMENT_CONCEPT_HPP

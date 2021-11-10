// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_BOX_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_BOX_CONCEPT_HPP

#include <cstddef>

#include <boost/concept_check.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/geometries/concepts/concept_type.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
\brief Box concept
\ingroup concepts
\par Formal definition:
The box concept is defined as following:
- there must be a specialization of traits::tag defining box_tag as type
- there must be a specialization of traits::point_type to define the
  underlying point type (even if it does not consist of points, it should define
  this type, to indicate the points it can work with)
- there must be a specialization of traits::indexed_access, per index
  (min_corner, max_corner) and per dimension, with two functions:
  - get to get a coordinate value
  - set to set a coordinate value (this one is not checked for ConstBox)
*/
template <typename Geometry>
class Box
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;


    template
    <
        std::size_t Index,
        std::size_t Dimension,
        std::size_t DimensionCount
    >
    struct dimension_checker
    {
        static void apply()
        {
            Geometry* b = 0;
            geometry::set<Index, Dimension>(*b, geometry::get<Index, Dimension>(*b));
            dimension_checker<Index, Dimension + 1, DimensionCount>::apply();
        }
    };

    template <std::size_t Index, std::size_t DimensionCount>
    struct dimension_checker<Index, DimensionCount, DimensionCount>
    {
        static void apply() {}
    };

public :
    BOOST_CONCEPT_USAGE(Box)
    {
        static const std::size_t n = dimension<Geometry>::type::value;
        dimension_checker<min_corner, 0, n>::apply();
        dimension_checker<max_corner, 0, n>::apply();
    }
#endif
};


/*!
\brief Box concept (const version)
\ingroup const_concepts
\details The ConstBox concept apply the same as the Box concept,
but does not apply write access.
*/
template <typename Geometry>
class ConstBox
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;
    typedef typename coordinate_type<Geometry>::type coordinate_type;

    template
    <
        std::size_t Index,
        std::size_t Dimension,
        std::size_t DimensionCount
    >
    struct dimension_checker
    {
        static void apply()
        {
            const Geometry* b = 0;
            coordinate_type coord(geometry::get<Index, Dimension>(*b));
            boost::ignore_unused(coord);
            dimension_checker<Index, Dimension + 1, DimensionCount>::apply();
        }
    };

    template <std::size_t Index, std::size_t DimensionCount>
    struct dimension_checker<Index, DimensionCount, DimensionCount>
    {
        static void apply() {}
    };

public :
    BOOST_CONCEPT_USAGE(ConstBox)
    {
        static const std::size_t n = dimension<Geometry>::type::value;
        dimension_checker<min_corner, 0, n>::apply();
        dimension_checker<max_corner, 0, n>::apply();
    }
#endif
};


template <typename Geometry>
struct concept_type<Geometry, box_tag>
{
    using type = Box<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, box_tag>
{
    using type = ConstBox<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_BOX_CONCEPT_HPP

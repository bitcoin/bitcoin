// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_RING_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_RING_CONCEPT_HPP


#include <boost/concept_check.hpp>
#include <boost/range/concepts.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
\brief ring concept
\ingroup concepts
\par Formal definition:
The ring concept is defined as following:
- there must be a specialization of traits::tag defining ring_tag as type
- it must behave like a Boost.Range
- there can optionally be a specialization of traits::point_order defining the
  order or orientation of its points, clockwise or counterclockwise.
- it must implement a std::back_insert_iterator
  (This is the same as the for the concept Linestring, and described there)

\note to fulfill the concepts, no traits class has to be specialized to
define the point type.
*/
template <typename Geometry>
class Ring
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;

    BOOST_CONCEPT_ASSERT( (concepts::Point<point_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );

public :

    BOOST_CONCEPT_USAGE(Ring)
    {
        Geometry* ring = 0;
        traits::clear<Geometry>::apply(*ring);
        traits::resize<Geometry>::apply(*ring, 0);
        point_type* point = 0;
        traits::push_back<Geometry>::apply(*ring, *point);
    }
#endif
};


/*!
\brief (linear) ring concept (const version)
\ingroup const_concepts
\details The ConstLinearRing concept check the same as the Geometry concept,
but does not check write access.
*/
template <typename Geometry>
class ConstRing
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename point_type<Geometry>::type point_type;

    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );


public :

    BOOST_CONCEPT_USAGE(ConstRing)
    {
    }
#endif
};


template <typename Geometry>
struct concept_type<Geometry, ring_tag>
{
    using type = Ring<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, ring_tag>
{
    using type = ConstRing<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_RING_CONCEPT_HPP

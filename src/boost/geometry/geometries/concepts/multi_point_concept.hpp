// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POINT_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POINT_CONCEPT_HPP


#include <boost/concept_check.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/mutable_range.hpp>

#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
\brief MultiPoint concept
\ingroup concepts
\par Formal definition:
The multi point concept is defined as following:
- there must be a specialization of traits::tag defining multi_point_tag as type
- it must behave like a Boost.Range
- its range value must fulfil the Point concept

*/
template <typename Geometry>
class MultiPoint
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename boost::range_value<Geometry>::type point_type;

    BOOST_CONCEPT_ASSERT( (concepts::Point<point_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );


public :

    BOOST_CONCEPT_USAGE(MultiPoint)
    {
        Geometry* mp = 0;
        traits::clear<Geometry>::apply(*mp);
        traits::resize<Geometry>::apply(*mp, 0);
        point_type* point = 0;
        traits::push_back<Geometry>::apply(*mp, *point);
    }
#endif
};


/*!
\brief concept for multi-point (const version)
\ingroup const_concepts
*/
template <typename Geometry>
class ConstMultiPoint
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename boost::range_value<Geometry>::type point_type;

    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );


public :

    BOOST_CONCEPT_USAGE(ConstMultiPoint)
    {
    }
#endif
};


template <typename Geometry>
struct concept_type<Geometry, multi_point_tag>
{
    using type = MultiPoint<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, multi_point_tag>
{
    using type = ConstMultiPoint<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POINT_CONCEPT_HPP

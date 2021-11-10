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


#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POLYGON_CONCEPT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POLYGON_CONCEPT_HPP


#include <boost/concept_check.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/geometries/concepts/concept_type.hpp>
#include <boost/geometry/geometries/concepts/polygon_concept.hpp>


namespace boost { namespace geometry { namespace concepts
{


/*!
\brief multi-polygon concept
\ingroup concepts
\par Formal definition:
The multi polygon concept is defined as following:
- there must be a specialization of traits::tag defining multi_polygon_tag
  as type
- it must behave like a Boost.Range
- its range value must fulfil the Polygon concept

*/
template <typename Geometry>
class MultiPolygon
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename boost::range_value<Geometry>::type polygon_type;

    BOOST_CONCEPT_ASSERT( (concepts::Polygon<polygon_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );


public :

    BOOST_CONCEPT_USAGE(MultiPolygon)
    {
        Geometry* mp = 0;
        traits::clear<Geometry>::apply(*mp);
        traits::resize<Geometry>::apply(*mp, 0);
        polygon_type* poly = 0;
        traits::push_back<Geometry>::apply(*mp, *poly);
    }
#endif
};


/*!
\brief concept for multi-polygon (const version)
\ingroup const_concepts
*/
template <typename Geometry>
class ConstMultiPolygon
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
    typedef typename boost::range_value<Geometry>::type polygon_type;

    BOOST_CONCEPT_ASSERT( (concepts::ConstPolygon<polygon_type>) );
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<Geometry>) );


public :

    BOOST_CONCEPT_USAGE(ConstMultiPolygon)
    {
    }
#endif
};


template <typename Geometry>
struct concept_type<Geometry, multi_polygon_tag>
{
    using type = MultiPolygon<Geometry>;
};

template <typename Geometry>
struct concept_type<Geometry const, multi_polygon_tag>
{
    using type = ConstMultiPolygon<Geometry>;
};


}}} // namespace boost::geometry::concepts


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_MULTI_POLYGON_CONCEPT_HPP

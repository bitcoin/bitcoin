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

#ifndef BOOST_GEOMETRY_CORE_INTERIOR_RINGS_HPP
#define BOOST_GEOMETRY_CORE_INTERIOR_RINGS_HPP

#include <cstddef>
#include <type_traits>

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/interior_type.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry
{

namespace traits
{


/*!
    \brief Traits class defining access to interior_rings of a polygon
    \details defines access (const and non const) to interior ring
    \ingroup traits
    \par Geometries:
        - polygon
    \par Specializations should provide:
        - static inline INTERIOR& get(POLY&)
        - static inline const INTERIOR& get(POLY const&)
    \tparam Geometry geometry
*/
template <typename Geometry>
struct interior_rings
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};


} // namespace traits




#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template
<
    typename GeometryTag,
    typename Geometry
>
struct interior_rings {};


template <typename Polygon>
struct interior_rings<polygon_tag, Polygon>
{
    static inline
    typename geometry::interior_return_type<Polygon>::type
                apply(Polygon& polygon)
    {
        return traits::interior_rings
            <
                typename std::remove_const<Polygon>::type
            >::get(polygon);
    }
};


template <typename MultiPolygon>
struct interior_type<multi_polygon_tag, MultiPolygon>
{
    typedef typename core_dispatch::interior_type
        <
            polygon_tag,
            typename boost::range_value<MultiPolygon>::type
        >::type type;
};


} // namespace core_dispatch
#endif



/*!
\brief Function to get the interior rings of a polygon (non const version)
\ingroup interior_rings
\note OGC compliance: instead of InteriorRingN
\tparam Polygon polygon type
\param polygon the polygon to get the interior rings from
\return the interior rings (possibly a reference)
*/

template <typename Polygon>
inline typename interior_return_type<Polygon>::type interior_rings(Polygon& polygon)
{
    return core_dispatch::interior_rings
        <
            typename tag<Polygon>::type,
            Polygon
        >::apply(polygon);
}


/*!
\brief Function to get the interior rings of a polygon (const version)
\ingroup interior_rings
\note OGC compliance: instead of InteriorRingN
\tparam Polygon polygon type
\param polygon the polygon to get the interior rings from
\return the interior rings (possibly a const reference)

\qbk{distinguish,const version}
*/
template <typename Polygon>
inline typename interior_return_type<Polygon const>::type interior_rings(
            Polygon const& polygon)
{
    return core_dispatch::interior_rings
        <
            typename tag<Polygon>::type,
            Polygon const
        >::apply(polygon);
}



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_INTERIOR_RINGS_HPP

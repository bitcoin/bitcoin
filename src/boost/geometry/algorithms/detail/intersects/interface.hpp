// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2013-2017.
// Modifications copyright (c) 2013-2017, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_INTERFACE_HPP


#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/disjoint/interface.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersects
{

// Forward declaration
template <typename Geometry>
struct self_intersects;

}} // namespace detail::intersects
#endif // DOXYGEN_NO_DETAIL


/*!
\brief \brief_check{has at least one intersection (crossing or self-tangency)}
\note This function can be called for one geometry (self-intersection) and
    also for two geometries (intersection)
\ingroup intersects
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is self-intersecting}

\qbk{distinguish,one geometry}
\qbk{[def __one_parameter__]}
\qbk{[include reference/algorithms/intersects.qbk]}
*/
template <typename Geometry>
inline bool intersects(Geometry const& geometry)
{
    return detail::intersects::self_intersects<Geometry>::apply(geometry);
}


/*!
\brief \brief_check2{have at least one intersection}
\ingroup intersects
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy \tparam_strategy{Intersects}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{intersects}
\return \return_check2{intersect each other}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/intersects.qbk]}
 */
template <typename Geometry1, typename Geometry2, typename Strategy>
inline bool intersects(Geometry1 const& geometry1,
                       Geometry2 const& geometry2,
                       Strategy const& strategy)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return ! geometry::disjoint(geometry1, geometry2, strategy);
}


/*!
\brief \brief_check2{have at least one intersection}
\ingroup intersects
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{intersect each other}

\qbk{distinguish,two geometries}
\qbk{[include reference/algorithms/intersects.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline bool intersects(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return ! geometry::disjoint(geometry1, geometry2);
}



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_INTERFACE_HPP

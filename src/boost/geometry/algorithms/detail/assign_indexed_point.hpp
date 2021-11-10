// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2021.
// Modifications copyright (c) 2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_INDEXED_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_INDEXED_POINT_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/algorithm.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

/*!
\brief Assign a box or segment with the value of a point
\ingroup assign
\tparam Index indicates which box-corner, min_corner (0) or max_corner (1)
    or which point of segment (0/1)
\tparam Point \tparam_point
\tparam Geometry \tparam_box_or_segment
\param point \param_point
\param geometry \param_box_or_segment

\qbk{
[heading Example]
[assign_point_to_index] [assign_point_to_index_output]
}
*/
template <std::size_t Index, typename Geometry, typename Point>
inline void assign_point_to_index(Point const& point, Geometry& geometry)
{
    concepts::check<Point const>();
    concepts::check<Geometry>();

    detail::for_each_dimension<Geometry>([&](auto dimension)
    {
        geometry::set<Index, dimension>(geometry,
            boost::numeric_cast
                <
                    typename coordinate_type<Geometry>::type
                >(geometry::get<dimension>(point)));
    });
}


/*!
\brief Assign a point with a point of a box or segment
\ingroup assign
\tparam Index indicates which box-corner, min_corner (0) or max_corner (1)
    or which point of segment (0/1)
\tparam Geometry \tparam_box_or_segment
\tparam Point \tparam_point
\param geometry \param_box_or_segment
\param point \param_point

\qbk{
[heading Example]
[assign_point_from_index] [assign_point_from_index_output]
}
*/
template <std::size_t Index, typename Point, typename Geometry>
inline void assign_point_from_index(Geometry const& geometry, Point& point)
{
    concepts::check<Geometry const>();
    concepts::check<Point>();

    detail::for_each_dimension<Geometry>([&](auto dimension)
    {
        geometry::set<dimension>(point,
            boost::numeric_cast
                <
                    typename coordinate_type<Point>::type
                >(geometry::get<Index, dimension>(geometry)));
    });
}


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_INDEXED_POINT_HPP

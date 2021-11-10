// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_BOX_CORNERS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_BOX_CORNERS_HPP


#include <cstddef>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{
// Note: this is moved to namespace detail because the names and parameter orders
// are not yet 100% clear.

/*!
\brief Assign the four points of a 2D box
\ingroup assign
\note The order is crucial. Most logical is LOWER, UPPER and sub-order LEFT, RIGHT
    so this is how it is implemented.
\tparam Box \tparam_box
\tparam Point \tparam_point
\param box \param_box
\param lower_left point being assigned to lower left coordinates of the box
\param lower_right point being assigned to lower right coordinates of the box
\param upper_left point being assigned to upper left coordinates of the box
\param upper_right point being assigned to upper right coordinates of the box

\qbk{
[heading Example]
[assign_box_corners] [assign_box_corners_output]
}
*/
template <typename Box, typename Point>
inline void assign_box_corners(Box const& box,
        Point& lower_left, Point& lower_right,
        Point& upper_left, Point& upper_right)
{
    concepts::check<Box const>();
    concepts::check<Point>();

    detail::assign::assign_box_2d_corner
            <min_corner, min_corner>(box, lower_left);
    detail::assign::assign_box_2d_corner
            <max_corner, min_corner>(box, lower_right);
    detail::assign::assign_box_2d_corner
            <min_corner, max_corner>(box, upper_left);
    detail::assign::assign_box_2d_corner
            <max_corner, max_corner>(box, upper_right);
}

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif


template <bool Reverse, typename Box, typename Range>
inline void assign_box_corners_oriented(Box const& box, Range& corners)
{
    if (Reverse)
    {
        // make counterclockwise ll,lr,ur,ul
        assign_box_corners(box,
                           range::at(corners, 0), range::at(corners, 1),
                           range::at(corners, 3), range::at(corners, 2));
    }
    else
    {
        // make clockwise ll,ul,ur,lr
        assign_box_corners(box,
                           range::at(corners, 0), range::at(corners, 3),
                           range::at(corners, 1), range::at(corners, 2));
    }
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ASSIGN_BOX_CORNERS_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_HPP

namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{

/*

   A Buffer-join strategy gets 4 input points.
   On the two consecutive segments s1 and s2 (joining at vertex v):

   The lines from parallel at s1, s2 (at buffer-distance) end/start
   in two points perpendicular to the segments: p1 and p2.
   These parallel lines interesct in point ip

             (s2)
              |
              |
              ^
              |
        (p2)  |(v)
        *     +----<--- (s1)

        x(ip) *(p1)


    So, in clockwise order:
        v : vertex point
        p1: perpendicular on left side of segment1<1> (perp1)
        ip: intersection point
        p2: perpendicular on left side of segment2<0> (perp2)
*/



/*!
\brief Enumerates options for side of buffer (left/right w.r.t. directed
    segment)
\ingroup enum
\details Around a linestring, a buffer can be defined left or right.
    Around a polygon, assumed clockwise internally,
    a buffer is either on the left side (inflates the polygon), or on the
    right side (deflates the polygon)
*/
enum buffer_side_selector { buffer_side_left, buffer_side_right };

/*!
\brief Enumerates types of pieces (parts of buffer) around geometries
\ingroup enum
*/
enum piece_type
{
    buffered_segment,
    buffered_join,
    buffered_round_end,
    buffered_flat_end,
    buffered_point,
    buffered_concave,   // always on the inside
    piece_type_unknown
};


/*!
\brief Enumerates types of joins
\ingroup enum
*/
enum join_selector
{
    join_convex,
    join_concave,
    join_continue, // collinear, next segment touches previous segment
    join_spike     // collinear, with overlap, next segment goes back
};

/*!
\brief Enumerates types of result codes from buffer strategies
\ingroup enum
*/
enum result_code
{
    result_normal,
    result_error_numerical,
    result_no_output
};


}} // namespace strategy::buffer


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_HPP

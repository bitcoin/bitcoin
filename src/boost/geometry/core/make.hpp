// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_MAKE_HPP
#define BOOST_GEOMETRY_CORE_MAKE_HPP

namespace boost { namespace geometry
{

namespace traits
{

/*!
\brief Traits class to create an object of Geometry type.
\details This trait is optional and allows to define efficient way of creating Geometries.
\ingroup traits
\par Geometries:
    - points
    - boxes
    - segments
\par Specializations should provide:
    - static const bool is_specialized = true;
    - static member function apply() taking:
      - N coordinates (points)
      - 2 points, min and max (boxes)
      - 2 points, first and second (segments)
\tparam Geometry geometry
*/
template <typename Geometry>
struct make
{
    static const bool is_specialized = false;
};

} // namespace traits


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_MAKE_HPP

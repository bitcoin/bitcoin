// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP
#define BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP


#include <boost/geometry/algorithms/detail/select_geometry_type.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{


namespace resolve_strategy
{

// NOTE: The implementation was simplified greately preserving the old
//   behavior. In general case the result types of Strategies should be
//   taken into account.
// It would probably be enough to use distance_result and
//   default_distance_result here.

} // namespace resolve_strategy


namespace resolve_dynamic
{

template <typename Sequence>
struct default_length_result_impl;

template <typename ...Geometries>
struct default_length_result_impl<util::type_sequence<Geometries...>>
{
    using type = typename select_most_precise
        <
            typename coordinate_type<Geometries>::type...,
            long double
        >::type;
};

template <typename Geometry>
struct default_length_result
    : default_length_result_impl<typename detail::geometry_types<Geometry>::type>
{};


} // namespace resolve_dynamic


/*!
    \brief Meta-function defining return type of length function
    \ingroup length
    \note Length of a line of integer coordinates can be double.
        So we take at least a double. If Big Number types are used,
        we take that type.

 */
template <typename Geometry>
struct default_length_result
    : resolve_dynamic::default_length_result<Geometry>
{};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP

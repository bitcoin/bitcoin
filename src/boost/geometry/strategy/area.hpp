// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_AREA_HPP
#define BOOST_GEOMETRY_STRATEGY_AREA_HPP


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace area
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// If user specified a CalculationType, use that type, whatever it is
//   and whatever the Geometry is.
// Else, use Geometry's coordinate-type promoted to double if needed.
template
<
    typename Geometry,
    typename CalculationType
>
struct result_type
{
    typedef CalculationType type;
};

template
<
    typename Geometry
>
struct result_type<Geometry, void>
    : select_most_precise
        <
            typename coordinate_type<Geometry>::type,
            double
        >
{};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


namespace services
{

/*!
    \brief Traits class binding a default area strategy to a coordinate system
    \ingroup area
    \tparam Tag tag of coordinate system
*/
template <typename Tag>
struct default_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this coordinate system.",
        Tag);
};


} // namespace services

}} // namespace strategy::area


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_AREA_HPP

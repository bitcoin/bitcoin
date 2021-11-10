// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_ALGORITHMS_NOT_IMPLEMENTED_HPP
#define BOOST_GEOMETRY_ALGORITHMS_NOT_IMPLEMENTED_HPP


#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{


namespace nyi
{


struct not_implemented_tag {};

template <typename ...Terms>
struct not_implemented_error
{

#ifndef BOOST_GEOMETRY_IMPLEMENTATION_STATUS_BUILD
# define BOOST_GEOMETRY_IMPLEMENTATION_STATUS_BUILD false
#endif

    BOOST_GEOMETRY_STATIC_ASSERT(
        BOOST_GEOMETRY_IMPLEMENTATION_STATUS_BUILD,
        "This operation is not or not yet implemented.",
        Terms...);
};


}


template <typename ...Terms>
struct not_implemented
    : nyi::not_implemented_tag,
      nyi::not_implemented_error<Terms...>
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_NOT_IMPLEMENTED_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_SELECT_CALCULATION_TYPE_HPP
#define BOOST_GEOMETRY_UTIL_SELECT_CALCULATION_TYPE_HPP


#include <boost/geometry/util/select_coordinate_type.hpp>


namespace boost { namespace geometry
{


/*!
    \brief Meta-function selecting the "calculation" type
    \details Based on two input geometry types, and an input calculation type,
        (which defaults to void in the calling function), this meta-function
        selects the most appropriate:
        - if calculation type is specified, that one is used,
        - if it is void, the most precise of the two points is used
    \ingroup utility
 */
template <typename Geometry1, typename Geometry2, typename CalculationType>
struct select_calculation_type
{
    typedef std::conditional_t
        <
            std::is_void<CalculationType>::value,
            typename select_coordinate_type
                <
                    Geometry1,
                    Geometry2
                >::type,
            CalculationType
        > type;
};

// alternative version supporting more than 2 Geometries
template
<
    typename CalculationType,
    typename ...Geometries
>
struct select_calculation_type_alt
{
    typedef std::conditional_t
        <
            std::is_void<CalculationType>::value,
            typename select_coordinate_type
                <
                    Geometries...
                >::type,
            CalculationType
        > type;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_SELECT_CALCULATION_TYPE_HPP

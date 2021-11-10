// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_INITIALIZE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_INITIALIZE_HPP

#include <cstddef>

#include <boost/numeric/conversion/bounds.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{

template <std::size_t Dimension, std::size_t DimensionCount>
struct initialize_loop
{
    template <typename Box, typename CoordinateType>
    static inline void apply(Box& box,
                             CoordinateType min_value,
                             CoordinateType max_value)
    {
        geometry::set<min_corner, Dimension>(box, min_value);
        geometry::set<max_corner, Dimension>(box, max_value);

        initialize_loop
            <
                Dimension + 1, DimensionCount
            >::apply(box, min_value, max_value);
    }
};

template <std::size_t DimensionCount>
struct initialize_loop<DimensionCount, DimensionCount>
{
    template <typename Box, typename CoordinateType>
    static inline void apply(Box&, CoordinateType, CoordinateType)
    {
    }
};


template
<
    typename Box,
    std::size_t Dimension = 0,
    std::size_t DimensionCount = dimension<Box>::value
>
struct initialize
{
    typedef typename coordinate_type<Box>::type coordinate_type;

    static inline void apply(Box& box,
        coordinate_type min_value
            = boost::numeric::bounds<coordinate_type>::highest(),
        coordinate_type max_value
            = boost::numeric::bounds<coordinate_type>::lowest())
    {
        initialize_loop
            <
                Dimension, DimensionCount
            >::apply(box, min_value, max_value);
    }
};

}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_INITIALIZE_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015, 2016, 2017, 2018.
// Modifications copyright (c) 2015-2018, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_POINT_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_POINT_HPP

#include <cstddef>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>

#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>

#include <boost/geometry/strategy/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{

template <std::size_t Dimension, std::size_t DimensionCount>
struct envelope_one_point
{
    template <std::size_t Index, typename Point, typename Box>
    static inline void apply(Point const& point, Box& mbr)
    {
        detail::indexed_point_view<Box, Index> box_corner(mbr);
        detail::conversion::point_to_point
            <
                Point,
                detail::indexed_point_view<Box, Index>,
                Dimension,
                DimensionCount
            >::apply(point, box_corner);
    }

    template <typename Point, typename Box>
    static inline void apply(Point const& point, Box& mbr)
    {
        apply<min_corner>(point, mbr);
        apply<max_corner>(point, mbr);
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL


namespace strategy { namespace envelope
{

struct cartesian_point
{
    template <typename Point, typename Box>
    static inline void apply(Point const& point, Box& mbr)
    {
        geometry::detail::envelope::envelope_one_point
            <
                0, dimension<Point>::value
            >::apply(point, mbr);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<point_tag, cartesian_tag, CalculationType>
{
    typedef strategy::envelope::cartesian_point type;
};


}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_POINT_HPP

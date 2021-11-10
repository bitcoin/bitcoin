// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_BOX_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_BOX_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/strategy/area.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

template
<
    typename CalculationType = void
>
class cartesian_box
{
public:
    template <typename Box>
    struct result_type
        : strategy::area::detail::result_type
            <
                Box,
                CalculationType
            >
    {};
    
    template <typename Box>
    static inline auto apply(Box const& box)
    {
        typedef typename result_type<Box>::type return_type;

        return return_type(get<max_corner, 0>(box) - get<min_corner, 0>(box))
             * return_type(get<max_corner, 1>(box) - get<min_corner, 1>(box));
    }
};


}} // namespace strategy::area


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_BOX_HPP

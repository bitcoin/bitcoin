// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Bruno Lalande, Paris, France.
// Copyright (c) 2013 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_TYPE_HPP
#define BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_TYPE_HPP

#include <type_traits>

#include <boost/config.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/policies/robustness/rescale_policy_tags.hpp>

namespace boost { namespace geometry { namespace detail
{

// Temporary meta-function to access segment-ratio for a policy
template <typename Point, typename Policy>
struct segment_ratio_type
{
    // Type in segment ratio is either the coordinate type, or for
    // deprecated robust point types it is a long_long type
    typedef std::conditional_t
        <
            std::is_same
                <
                    typename rescale_policy_type<Policy>::type,
                    no_rescale_policy_tag
                >::value,
            typename geometry::coordinate_type<Point>::type,
            boost::long_long_type
        > coordinate_type;

    // Define segment ratio based on the coordinate type
    typedef geometry::segment_ratio<coordinate_type> type;
};


}}} // namespace boost::geometry::deatil


#endif // BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_TYPE_HPP

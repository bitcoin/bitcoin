// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_SEGMENT_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_SEGMENT_HPP

#include <cstddef>
#include <functional>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/algorithms/detail/envelope/box.hpp>
#include <boost/geometry/algorithms/detail/envelope/range_of_boxes.hpp>
#include <boost/geometry/algorithms/detail/envelope/segment.hpp>

#include <boost/geometry/strategy/expand.hpp>
#include <boost/geometry/strategy/spherical/envelope_box.hpp>
#include <boost/geometry/strategy/spherical/envelope_segment.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace expand
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct segment_on_spheroid
{
    template <typename Box, typename Segment, typename Strategy>
    static inline void apply(Box& box, Segment const& segment, Strategy const& strategy)
    {
        Box mbrs[2];

        // compute the envelope of the segment
        geometry::detail::envelope::envelope_segment::apply(segment, mbrs[0], strategy);

        // normalize the box
        strategy::envelope::spherical_box::apply(box, mbrs[1]);

        // compute the envelope of the two boxes
        geometry::detail::envelope::envelope_range_of_boxes::apply(mbrs, box);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template
<
    typename CalculationType = void
>
class spherical_segment
{
public:
    template <typename Box, typename Segment>
    static inline void apply(Box& box, Segment const& segment)
    {
        detail::segment_on_spheroid::apply(box, segment,
            strategy::envelope::spherical_segment<CalculationType>());
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<segment_tag, spherical_equatorial_tag, CalculationType>
{
    typedef spherical_segment<CalculationType> type;
};

template <typename CalculationType>
struct default_strategy<segment_tag, spherical_polar_tag, CalculationType>
{
    typedef spherical_segment<CalculationType> type;
};

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::expand

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_EXPAND_SEGMENT_HPP

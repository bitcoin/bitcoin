// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018-2020.
// Modifications copyright (c) 2018-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTION_BOX_POLICIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTION_BOX_POLICIES_HPP


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/expand.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace section
{

template <typename Strategy>
struct get_section_box
{
    get_section_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename Section>
    inline void apply(Box& total, Section const& section) const
    {
        assert_coordinate_type_equal(total, section.bounding_box);
        geometry::expand(total, section.bounding_box, m_strategy);
    }

    Strategy const& m_strategy;
};

template <typename Strategy>
struct overlaps_section_box
{
    overlaps_section_box(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Box, typename Section>
    inline bool apply(Box const& box, Section const& section) const
    {
        assert_coordinate_type_equal(box, section.bounding_box);
        return ! detail::disjoint::disjoint_box_box(box, section.bounding_box,
                                                    m_strategy);
    }

    Strategy const& m_strategy;
};


}} // namespace detail::section
#endif


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTION_BOX_POLICIES_HPP

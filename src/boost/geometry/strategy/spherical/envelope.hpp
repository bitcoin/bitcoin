// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015, 2016, 2018, 2019.
// Modifications copyright (c) 2015-2019, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_HPP

#include <boost/geometry/algorithms/detail/envelope/initialize.hpp>
#include <boost/geometry/algorithms/detail/envelope/range_of_boxes.hpp>

#include <boost/geometry/iterators/segment_iterator.hpp>

#include <boost/geometry/strategy/spherical/envelope_box.hpp>
#include <boost/geometry/strategy/spherical/envelope_segment.hpp>
#include <boost/geometry/strategy/spherical/expand_box.hpp>
#include <boost/geometry/strategy/spherical/expand_segment.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace envelope
{

template <typename CalculationType = void>
class spherical
{
public:
    typedef spherical_tag cs_tag;

    // Linestring, Ring, Polygon

    template <typename Range>
    static inline geometry::segment_iterator<Range const> begin(Range const& range)
    {
        return geometry::segments_begin(range);
    }

    template <typename Range>
    static inline geometry::segment_iterator<Range const> end(Range const& range)
    {
        return geometry::segments_end(range);
    }

    // MultiLinestring, MultiPolygon

    template <typename Box>
    struct multi_state
    {
        void apply(Box const& single_box)
        {
            m_boxes.push_back(single_box);
        }

        void result(Box & box)
        {
            if (!m_boxes.empty())
            {
                geometry::detail::envelope::envelope_range_of_boxes::apply(m_boxes, box);
            }
            else
            {
                geometry::detail::envelope::initialize<Box, 0, dimension<Box>::value>::apply(box);
            }
        }

    private:
        std::vector<Box> m_boxes;
    };
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename Tag, typename CalculationType>
struct default_strategy<Tag, spherical_equatorial_tag, CalculationType>
{
    typedef strategy::envelope::spherical<CalculationType> type;
};

template <typename Tag, typename CalculationType>
struct default_strategy<Tag, spherical_polar_tag, CalculationType>
{
    typedef strategy::envelope::spherical<CalculationType> type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope

}} //namepsace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_HPP

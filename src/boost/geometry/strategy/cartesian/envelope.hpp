// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/algorithms/detail/envelope/initialize.hpp>

#include <boost/geometry/strategy/cartesian/envelope_box.hpp>
#include <boost/geometry/strategy/cartesian/envelope_segment.hpp>
#include <boost/geometry/strategy/cartesian/expand_box.hpp>
#include <boost/geometry/strategy/cartesian/expand_segment.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace envelope
{

template <typename CalculationType = void>
class cartesian
{
public:
    typedef cartesian_tag cs_tag;

    // Linestring, Ring, Polygon

    template <typename Range>
    static inline typename boost::range_iterator<Range const>::type begin(Range const& range)
    {
        return boost::begin(range);
    }

    template <typename Range>
    static inline typename boost::range_iterator<Range const>::type end(Range const& range)
    {
        return boost::end(range);
    }

    // MultiLinestring, MultiPolygon

    template <typename Box>
    struct multi_state
    {
        multi_state()
            : m_initialized(false)
        {}

        void apply(Box const& single_box)
        {
            if (! m_initialized)
            {
                m_box = single_box;
                m_initialized = true;
            }
            else
            {
                strategy::expand::cartesian_box::apply(m_box, single_box);
            }
        }

        void result(Box & box)
        {
            if (m_initialized)
            {
                box = m_box;
            }
            else
            {
                geometry::detail::envelope::initialize<Box, 0, dimension<Box>::value>::apply(box);
            }
        }

    private:
        bool m_initialized;
        Box m_box;
    };
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename Tag, typename CalculationType>
struct default_strategy<Tag, cartesian_tag, CalculationType>
{
    typedef strategy::envelope::cartesian<CalculationType> type;
};

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope

}} //namepsace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_HPP

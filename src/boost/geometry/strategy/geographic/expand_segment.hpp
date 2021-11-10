// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2015, 2016, 2017, 2018.
// Modifications copyright (c) 2015-2018, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_EXPAND_SEGMENT_HPP
#define BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_EXPAND_SEGMENT_HPP

#include <cstddef>
#include <functional>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/envelope/box.hpp>
#include <boost/geometry/algorithms/detail/envelope/range_of_boxes.hpp>
#include <boost/geometry/algorithms/detail/envelope/segment.hpp>

#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/strategy/expand.hpp>
#include <boost/geometry/strategy/geographic/envelope_segment.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>
#include <boost/geometry/strategy/spherical/expand_segment.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace expand
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = geometry::srs::spheroid<double>,
    typename CalculationType = void
>
class geographic_segment
{
public:
    inline geographic_segment()
        : m_envelope_strategy()
    {}

    explicit inline geographic_segment(Spheroid const& spheroid)
        : m_envelope_strategy(spheroid)
    {}

    template <typename Box, typename Segment>
    inline void apply(Box& box, Segment const& segment) const
    {
        detail::segment_on_spheroid::apply(box, segment, m_envelope_strategy);
    }

    Spheroid model() const
    {
        return m_envelope_strategy.model();
    }

private:
    strategy::envelope::geographic_segment
        <
            FormulaPolicy, Spheroid, CalculationType
        > m_envelope_strategy;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<segment_tag, geographic_tag, CalculationType>
{
    typedef geographic_segment
        <
            strategy::andoyer,
            geometry::srs::spheroid<double>,
            CalculationType
        > type;
};

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::expand

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_EXPAND_SEGMENT_HPP

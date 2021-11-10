// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2018-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_MULTIPOINT_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_MULTIPOINT_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/envelope/initialize.hpp>

#include <boost/geometry/strategy/cartesian/envelope.hpp>
#include <boost/geometry/strategy/cartesian/envelope_point.hpp>
#include <boost/geometry/strategy/cartesian/expand_point.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace envelope
{

class cartesian_multipoint
{
public:
    template <typename MultiPoint, typename Box>
    static inline void apply(MultiPoint const& multipoint, Box& mbr)
    {
        apply(boost::begin(multipoint), boost::end(multipoint), mbr);
    }

private:
    template <typename Iterator, typename Box>
    static inline void apply(Iterator it,
                             Iterator last,
                             Box& mbr)
    {
        geometry::detail::envelope::initialize<Box, 0, dimension<Box>::value>::apply(mbr);

        if (it != last)
        {
            strategy::envelope::cartesian_point::apply(*it, mbr);

            for (++it; it != last; ++it)
            {
                strategy::expand::cartesian_point::apply(mbr, *it);
            }
        }
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<multi_point_tag, cartesian_tag, CalculationType>
{
    typedef strategy::envelope::cartesian_multipoint type;
};


} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_ENVELOPE_MULTIPOINT_HPP

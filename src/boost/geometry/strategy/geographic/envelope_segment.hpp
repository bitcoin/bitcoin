// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_ENVELOPE_SEGMENT_HPP
#define BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_ENVELOPE_SEGMENT_HPP


#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/strategy/cartesian/envelope_segment.hpp>
#include <boost/geometry/strategy/envelope.hpp>
#include <boost/geometry/strategies/geographic/azimuth.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>
#include <boost/geometry/strategies/normalize.hpp>
#include <boost/geometry/strategy/spherical/envelope_segment.hpp>
#include <boost/geometry/strategy/spherical/expand_box.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace envelope
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
    typedef Spheroid model_type;

    inline geographic_segment()
        : m_spheroid()
    {}

    explicit inline geographic_segment(Spheroid const& spheroid)
        : m_spheroid(spheroid)
    {}

    template <typename Point, typename Box>
    inline void apply(Point const& point1, Point const& point2, Box& box) const
    {
        Point p1_normalized, p2_normalized;
        strategy::normalize::spherical_point::apply(point1, p1_normalized);
        strategy::normalize::spherical_point::apply(point2, p2_normalized);

        geometry::strategy::azimuth::geographic
            <
                FormulaPolicy,
                Spheroid,
                CalculationType
            > azimuth_geographic(m_spheroid);

        typedef typename geometry::detail::cs_angular_units
            <
                Point
            >::type units_type;

        // first compute the envelope range for the first two coordinates
        strategy::envelope::detail::envelope_segment_impl
            <
                geographic_tag
            >::template apply<units_type>(geometry::get<0>(p1_normalized),
                                          geometry::get<1>(p1_normalized),
                                          geometry::get<0>(p2_normalized),
                                          geometry::get<1>(p2_normalized),
                                          box,
                                          azimuth_geographic);

        // now compute the envelope range for coordinates of
        // dimension 2 and higher
        strategy::envelope::detail::envelope_one_segment
            <
                2, dimension<Point>::value
            >::apply(point1, point2, box);
    }

    Spheroid model() const
    {
        return m_spheroid;
    }

private:
    Spheroid m_spheroid;
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<segment_tag, geographic_tag, CalculationType>
{
    typedef strategy::envelope::geographic_segment
        <
            strategy::andoyer,
            srs::spheroid<double>,
            CalculationType
        > type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope

}} //namepsace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_ENVELOPE_SEGMENT_HPP

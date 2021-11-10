// Boost.Geometry

// Copyright (c) 2017-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_POINT_IN_POLY_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_POINT_IN_POLY_WINDING_HPP


#include <boost/geometry/strategies/geographic/side.hpp>
#include <boost/geometry/strategies/spherical/point_in_poly_winding.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{


/*!
\brief Within detection using winding rule in geographic coordinate system.
\ingroup strategies
\tparam Point \tparam_point
\tparam PointOfSegment \tparam_segment_point
\tparam FormulaPolicy Geodesic formula policy
\tparam Spheroid Spheroid model
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    typename Point = void, // for backward compatibility
    typename PointOfSegment = Point, // for backward compatibility
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic_winding
    : public within::detail::spherical_winding_base
        <
            side::geographic<FormulaPolicy, Spheroid, CalculationType>,
            CalculationType
        >
{
    typedef within::detail::spherical_winding_base
        <
            side::geographic<FormulaPolicy, Spheroid, CalculationType>,
            CalculationType
        > base_t;

public:
    geographic_winding()
    {}

    explicit geographic_winding(Spheroid const& model)
        : base_t(model)
    {}

    Spheroid const& model() const
    {
        return base_t::m_side_strategy.model();
    }
};


}} // namespace strategy::within


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_POINT_IN_POLY_WINDING_HPP

// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_INDEX_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_INDEX_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/distance/geographic.hpp>
#include <boost/geometry/strategies/index/services.hpp>


namespace boost { namespace geometry { namespace strategies { namespace index
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
    : public distance::geographic<FormulaPolicy, Spheroid, CalculationType>
{
    typedef distance::geographic<FormulaPolicy, Spheroid, CalculationType> base_t;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    using type = strategies::index::geographic<>;
};


} // namespace services


}}}} // namespace boost::geometry::strategy::index

#endif // BOOST_GEOMETRY_STRATEGIES_INDEX_GEOGRAPHIC_HPP

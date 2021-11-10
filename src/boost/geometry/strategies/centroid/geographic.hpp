// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CENTROID_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_CENTROID_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/strategies/centroid/services.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace centroid
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
    : public strategies::detail::geographic_base<Spheroid>
{
    using base_t = strategies::detail::geographic_base<Spheroid>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    // TODO: Box and Segment should have proper strategies.
    template <typename Geometry, typename Point>
    static auto centroid(Geometry const&, Point const&,
                         std::enable_if_t
                            <
                                util::is_segment<Geometry>::value
                             || util::is_box<Geometry>::value
                            > * = nullptr)
    {
        return strategy::centroid::not_applicable_strategy();
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    using type = strategies::centroid::geographic<>;
};

} // namespace services

}} // namespace strategies::centroid

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CENTROID_GEOGRAPHIC_HPP

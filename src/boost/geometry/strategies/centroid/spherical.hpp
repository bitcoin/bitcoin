// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CENTROID_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_CENTROID_SPHERICAL_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/strategies/centroid/services.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace centroid
{

template
<
    typename CalculationType = void
>
class spherical
    : public strategies::detail::spherical_base<void>
{
    using base_t = strategies::detail::spherical_base<void>;

public:
    spherical() = default;

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
struct default_strategy<Geometry, spherical_equatorial_tag>
{
    using type = strategies::centroid::spherical<>;
};

} // namespace services

}} // namespace strategies::centroid

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CENTROID_SPHERICAL_HPP

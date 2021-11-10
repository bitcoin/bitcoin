// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CENTROID_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_CENTROID_CARTESIAN_HPP


#include <boost/geometry/strategies/cartesian/centroid_average.hpp>
#include <boost/geometry/strategies/cartesian/centroid_bashein_detmer.hpp>
#include <boost/geometry/strategies/cartesian/centroid_weighted_length.hpp>

#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/centroid/services.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace centroid
{

template <typename CalculationType = void>
struct cartesian
    : public strategies::detail::cartesian_base
{
    template <typename Geometry>
    static auto centroid(Geometry const&,
                         std::enable_if_t
                            <
                                util::is_pointlike<Geometry>::value
                            > * = nullptr)
    {
        return strategy::centroid::average<>();
    }

    template <typename Geometry>
    static auto centroid(Geometry const&,
                         std::enable_if_t
                            <
                                util::is_polylinear<Geometry>::value
                            > * = nullptr)
    {
        return strategy::centroid::weighted_length<void, void, CalculationType>();
    }

    template <typename Geometry>
    static auto centroid(Geometry const&,
                         std::enable_if_t
                            <
                                util::is_polygonal<Geometry>::value
                             // TODO: This condition was used for the legacy default strategy
                             // && geometry::dimension<Geometry>::value == 2
                            > * = nullptr)
    {
        return strategy::centroid::bashein_detmer<void, void, CalculationType>();
    }

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
struct default_strategy<Geometry, cartesian_tag>
{
    using type = strategies::centroid::cartesian<>;
};


template <typename PC, typename PG>
struct strategy_converter<strategy::centroid::average<PC, PG> >
{
    static auto get(strategy::centroid::average<PC, PG> const&)
    {
        return strategies::centroid::cartesian<>();
    }
};

template <typename PC, typename PG>
struct strategy_converter<strategy::centroid::weighted_length<PC, PG> >
{
    static auto get(strategy::centroid::weighted_length<PC, PG> const&)
    {
        return strategies::centroid::cartesian<>();
    }
};

template <typename PC, typename PG, typename CT>
struct strategy_converter<strategy::centroid::bashein_detmer<PC, PG, CT> >
{
    static auto get(strategy::centroid::bashein_detmer<PC, PG, CT> const&)
    {
        return strategies::centroid::cartesian<CT>();
    }
};

} // namespace services

}} // namespace strategies::centroid

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CENTROID_CARTESIAN_HPP

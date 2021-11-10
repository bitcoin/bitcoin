// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DENSIFY_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_DENSIFY_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/densify/services.hpp>

#include <boost/geometry/strategies/geographic/densify.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace densify
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

    template <typename Geometry>
    auto densify(Geometry const&) const
    {
        return strategy::densify::geographic
                <
                    FormulaPolicy, Spheroid, CalculationType
                >(base_t::m_spheroid);
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    using type = strategies::densify::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::densify::geographic<FP, S, CT> >
{
    static auto get(strategy::densify::geographic<FP, S, CT> const& s)
    {
        return strategies::densify::geographic<FP, S, CT>(s.model());
    }
};


} // namespace services

}} // namespace strategies::densify

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DENSIFY_GEOGRAPHIC_HPP

// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_LENGTH_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_LENGTH_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/length/services.hpp>

#include <boost/geometry/strategies/geographic/distance.hpp>
// TODO - for backwards compatibility, remove?
#include <boost/geometry/strategies/geographic/distance_andoyer.hpp>
#include <boost/geometry/strategies/geographic/distance_thomas.hpp>
#include <boost/geometry/strategies/geographic/distance_vincenty.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace length
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

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  distance::detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic
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
    using type = strategies::length::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic<FP, S, CT> >
{
    static auto get(strategy::distance::geographic<FP, S, CT> const& s)
    {
        return strategies::length::geographic<FP, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::andoyer<S, CT> >
{
    static auto get(strategy::distance::andoyer<S, CT> const& s)
    {
        return strategies::length::geographic<strategy::andoyer, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::thomas<S, CT> >
{
    static auto get(strategy::distance::thomas<S, CT> const& s)
    {
        return strategies::length::geographic<strategy::thomas, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::vincenty<S, CT> >
{
    static auto get(strategy::distance::vincenty<S, CT> const& s)
    {
        return strategies::length::geographic<strategy::vincenty, S, CT>(s.model());
    }
};


} // namespace services

}} // namespace strategies::length

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_LENGTH_GEOGRAPHIC_HPP

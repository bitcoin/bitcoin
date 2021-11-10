// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_AREA_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_AREA_GEOGRAPHIC_HPP


#include <boost/geometry/strategy/geographic/area.hpp>
#include <boost/geometry/strategy/geographic/area_box.hpp>

#include <boost/geometry/strategies/area/services.hpp>
#include <boost/geometry/strategies/detail.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace area
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
    auto area(Geometry const&,
              std::enable_if_t<! util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::geographic
            <
                FormulaPolicy,
                strategy::default_order<FormulaPolicy>::value,
                Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    template <typename Geometry>
    auto area(Geometry const&,
              std::enable_if_t<util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::geographic_box
            <
                Spheroid, CalculationType
            >(base_t::m_spheroid);
    }
};


namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    using type = strategies::area::geographic<>;
};


template <typename FP, std::size_t SO, typename S, typename CT>
struct strategy_converter<strategy::area::geographic<FP, SO, S, CT> >
{
    struct altered_strategy
        : strategies::area::geographic<FP, S, CT>
    {
        explicit altered_strategy(S const& spheroid)
            : strategies::area::geographic<FP, S, CT>(spheroid)
        {}

        template <typename Geometry>
        auto area(Geometry const&) const
        {
            return strategy::area::geographic<FP, SO, S, CT>(this->m_spheroid);
        }
    };

    static auto get(strategy::area::geographic<FP, SO, S, CT> const& strategy)
    {
        return altered_strategy(strategy.model());
    }
};

} // namespace services

}} // namespace strategies::area

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_AREA_GEOGRAPHIC_HPP

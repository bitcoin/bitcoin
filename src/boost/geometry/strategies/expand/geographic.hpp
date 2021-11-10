// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_EXPAND_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_EXPAND_GEOGRAPHIC_HPP


#include <type_traits>

#include <boost/geometry/strategy/geographic/expand_segment.hpp>

#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/expand/spherical.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace expand
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

    template <typename Box, typename Geometry>
    static auto expand(Box const&, Geometry const&,
                       typename util::enable_if_point_t<Geometry> * = nullptr)
    {
        return strategy::expand::spherical_point();
    }

    template <typename Box, typename Geometry>
    static auto expand(Box const&, Geometry const&,
                       typename util::enable_if_box_t<Geometry> * = nullptr)
    {
        return strategy::expand::spherical_box();
    }

    template <typename Box, typename Geometry>
    auto expand(Box const&, Geometry const&,
                typename util::enable_if_segment_t<Geometry> * = nullptr) const
    {
        return strategy::expand::geographic_segment
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }
};


namespace services
{

template <typename Box, typename Geometry>
struct default_strategy<Box, Geometry, geographic_tag>
{
    using type = strategies::expand::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::expand::geographic_segment<FP, S, CT> >
{
    static auto get(strategy::expand::geographic_segment<FP, S, CT> const& s)
    {
        return strategies::expand::geographic<FP, S, CT>(s.model());
    }
};


} // namespace services

}} // namespace strategies::envelope

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_EXPAND_GEOGRAPHIC_HPP

// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategies/distance/detail.hpp>

#include <boost/geometry/strategies/geographic/distance.hpp>
#include <boost/geometry/strategies/geographic/line_interpolate.hpp>

#include <boost/geometry/strategies/line_interpolate/services.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace line_interpolate
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

    template <typename Geometry>
    auto line_interpolate(Geometry const&) const
    {
        return strategy::line_interpolate::geographic
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
    using type = strategies::line_interpolate::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::line_interpolate::geographic<FP, S, CT> >
{
    static auto get(strategy::line_interpolate::geographic<FP, S, CT> const& s)
    {
        return strategies::line_interpolate::geographic<FP, S, CT>(s.model());
    }
};


} // namespace services

}} // namespace strategies::line_interpolate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_LINE_INTERPOLATE_GEOGRAPHIC_HPP

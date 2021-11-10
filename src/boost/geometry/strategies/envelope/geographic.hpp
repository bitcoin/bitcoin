// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_ENVELOPE_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_ENVELOPE_GEOGRAPHIC_HPP


#include <type_traits>

#include <boost/geometry/strategy/geographic/envelope.hpp>
#include <boost/geometry/strategy/geographic/envelope_segment.hpp>

#include <boost/geometry/strategy/geographic/expand_segment.hpp> // TEMP

#include <boost/geometry/strategies/envelope/spherical.hpp>
#include <boost/geometry/strategies/expand/geographic.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace envelope
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
    : public strategies::expand::geographic<FormulaPolicy, Spheroid, CalculationType>
{
    using base_t = strategies::expand::geographic<FormulaPolicy, Spheroid, CalculationType>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    template <typename Geometry, typename Box>
    static auto envelope(Geometry const&, Box const&,
                         typename util::enable_if_point_t<Geometry> * = nullptr)
    {
        return strategy::envelope::spherical_point();
    }

    template <typename Geometry, typename Box>
    static auto envelope(Geometry const&, Box const&,
                         typename util::enable_if_multi_point_t<Geometry> * = nullptr)
    {
        return strategy::envelope::spherical_multipoint();
    }

    template <typename Geometry, typename Box>
    static auto envelope(Geometry const&, Box const&,
                         typename util::enable_if_box_t<Geometry> * = nullptr)
    {
        return strategy::envelope::spherical_box();
    }

    template <typename Geometry, typename Box>
    auto envelope(Geometry const&, Box const&,
                  typename util::enable_if_segment_t<Geometry> * = nullptr) const
    {
        return strategy::envelope::geographic_segment
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    template <typename Geometry, typename Box>
    auto envelope(Geometry const&, Box const&,
                  typename util::enable_if_polysegmental_t<Geometry> * = nullptr) const
    {
        return strategy::envelope::geographic
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }
};


namespace services
{

template <typename Geometry, typename Box>
struct default_strategy<Geometry, Box, geographic_tag>
{
    using type = strategies::envelope::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::envelope::geographic_segment<FP, S, CT> >
{
    static auto get(strategy::envelope::geographic_segment<FP, S, CT> const& s)
    {
        return strategies::envelope::geographic<FP, S, CT>(s.model());
    }
};

template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::envelope::geographic<FP, S, CT> >
{
    static auto get(strategy::envelope::geographic<FP, S, CT> const& s)
    {
        return strategies::envelope::geographic<FP, S, CT>(s.model());
    }
};

} // namespace services

}} // namespace strategies::envelope

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_ENVELOPE_GEOGRAPHIC_HPP

// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_BACKWARD_COMPATIBILITY_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_BACKWARD_COMPATIBILITY_HPP


#include <boost/geometry/strategies/distance/cartesian.hpp>
#include <boost/geometry/strategies/distance/comparable.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/distance/geographic.hpp>
#include <boost/geometry/strategies/distance/spherical.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace distance
{  

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template
<
    typename Strategy,
    typename CSTag,
    typename StrategyTag = typename strategy::distance::services::tag<Strategy>::type
>
struct custom_strategy
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Strategy.",
        Strategy);
};

// NOTES:
// - There is no guarantee that any of the custom strategies are compatible with other
//   strategies.
// - In many cases it is not possible to pass the custom strategy into other strategies.
// - The old backward compatibility code creating default Pt/Seg strategy from custom
//   Pt/Pt strategy worked the same way. The custom strategy is default-created.
// - There are two versions of algorithm for Polyseg/Box, one for Seg/Box strategy the
//   other one for Pt/Seg strategy. However there is only one version of algorithm for
//   Seg/Box taking Seg/Box strategy.
// - Geographic strategies are default-created which means WGS84 spheroid is used.
// - Currently only Pt/Pt custom strategies are supported because this is what the old
//   backward compatibility code was addressing.

template <typename Strategy>
struct custom_strategy<Strategy, cartesian_tag, strategy_tag_distance_point_point>
    : cartesian<>
{
    custom_strategy(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return m_strategy;
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::projected_point<void, Strategy>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::pythagoras_point_box<>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_sb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cartesian_segment_box<void, Strategy>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_bb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::pythagoras_box_box<>();
    }

private:
    Strategy const& m_strategy;
};

template <typename Strategy>
struct custom_strategy<Strategy, spherical_tag, strategy_tag_distance_point_point>
    : detail::spherical<void, void>
{
    custom_strategy(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return m_strategy;
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track<void, Strategy>(m_strategy);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track_point_box<void, Strategy>(m_strategy);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_sb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::spherical_segment_box<void, Strategy>(m_strategy);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_bb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track_box_box<void, Strategy>(m_strategy);
    }

private:
    Strategy const& m_strategy;
};

template <typename Strategy>
struct custom_strategy<Strategy, geographic_tag, strategy_tag_distance_point_point>
    : geographic<>
{
    custom_strategy(Strategy const& strategy)
        : m_strategy(strategy)
    {}

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return m_strategy;
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track<>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_pb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track_point_box<>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_sb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_segment_box<>();
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  enable_if_bb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track_box_box<>();
    }

private:
    Strategy const& m_strategy;
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

namespace services
{

template <typename Geometry1, typename Geometry2, typename Strategy>
struct custom_strategy_converter<Geometry1, Geometry2, Strategy, cartesian_tag, cartesian_tag>
{
    static auto get(Strategy const& strategy)
    {
        return detail::custom_strategy<Strategy, cartesian_tag>(strategy);
    }
};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct custom_strategy_converter<Geometry1, Geometry2, Strategy, spherical_equatorial_tag, spherical_equatorial_tag>
{
    static auto get(Strategy const& strategy)
    {
        return detail::custom_strategy<Strategy, spherical_tag>(strategy);
    }
};

template <typename Geometry1, typename Geometry2, typename Strategy>
struct custom_strategy_converter<Geometry1, Geometry2, Strategy, geographic_tag, geographic_tag>
{
    static auto get(Strategy const& strategy)
    {
        return detail::custom_strategy<Strategy, geographic_tag>(strategy);
    }
};


} // namespace services

}} // namespace strategies::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_BACKWARD_COMPATIBILITY_HPP

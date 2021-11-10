// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_SPHERICAL_HPP


#include <boost/geometry/strategies/distance/comparable.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/distance/services.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategies/normalize.hpp>
#include <boost/geometry/strategies/relate/spherical.hpp>

#include <boost/geometry/strategies/spherical/azimuth.hpp>

#include <boost/geometry/strategies/spherical/distance_cross_track.hpp>
#include <boost/geometry/strategies/spherical/distance_cross_track_box_box.hpp>
#include <boost/geometry/strategies/spherical/distance_cross_track_point_box.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/distance_segment_box.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace distance
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// TODO: azimuth and normalize getters would not be needed if distance_segment_box was implemented differently
//       right now it calls disjoint algorithm details.

template <typename RadiusTypeOrSphere, typename CalculationType>
class spherical
    : public strategies::relate::detail::spherical<RadiusTypeOrSphere, CalculationType>
{
    using base_t = strategies::relate::detail::spherical<RadiusTypeOrSphere, CalculationType>;

public:
    spherical() = default;

    template <typename RadiusOrSphere>
    explicit spherical(RadiusOrSphere const& radius_or_sphere)
        : base_t(radius_or_sphere)
    {}

    // azimuth

    static auto azimuth()
    {
        return strategy::azimuth::spherical<CalculationType>();
    }

    // distance

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::haversine
                <
                    typename base_t::radius_type, CalculationType
                >(base_t::radius());
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track
            <
                CalculationType,
                strategy::distance::haversine<typename base_t::radius_type, CalculationType>
            >(base_t::radius());
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_pb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track_point_box
            <
                CalculationType,
                strategy::distance::haversine<typename base_t::radius_type, CalculationType>
            >(base_t::radius());
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_sb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::spherical_segment_box
            <
                CalculationType,
                strategy::distance::haversine<typename base_t::radius_type, CalculationType>
            >(base_t::radius());
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_bb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::cross_track_box_box
            <
                CalculationType,
                strategy::distance::haversine<typename base_t::radius_type, CalculationType>
            >(base_t::radius());
    }

    // normalize

    template <typename Geometry>
    static auto normalize(Geometry const&,
                          std::enable_if_t
                            <
                                util::is_point<Geometry>::value
                            > * = nullptr)
    {
        return strategy::normalize::spherical_point();
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template
<
    typename RadiusTypeOrSphere = double,
    typename CalculationType = void
>
class spherical
    : public strategies::distance::detail::spherical<RadiusTypeOrSphere, CalculationType>
{
    using base_t = strategies::distance::detail::spherical<RadiusTypeOrSphere, CalculationType>;

public:
    spherical() = default;

    template <typename RadiusOrSphere>
    explicit spherical(RadiusOrSphere const& radius_or_sphere)
        : base_t(radius_or_sphere)
    {}
};


namespace services
{

template <typename Geometry1, typename Geometry2>
struct default_strategy
    <
        Geometry1, Geometry2,
        spherical_equatorial_tag, spherical_equatorial_tag
    >
{
    using type = strategies::distance::spherical<>;
};


template <typename R, typename CT>
struct strategy_converter<strategy::distance::haversine<R, CT> >
{
    template <typename S>
    static auto get(S const& s)
    {
        return strategies::distance::spherical<R, CT>(s.radius());
    }
};

template <typename CT, typename PPS>
struct strategy_converter<strategy::distance::cross_track<CT, PPS> >
    : strategy_converter<PPS>
{};

template <typename CT, typename PPS>
struct strategy_converter<strategy::distance::cross_track_point_box<CT, PPS> >
    : strategy_converter<PPS>
{};

template <typename CT, typename PPS>
struct strategy_converter<strategy::distance::spherical_segment_box<CT, PPS> >
    : strategy_converter<PPS>
{};

template <typename CT, typename PPS>
struct strategy_converter<strategy::distance::cross_track_box_box<CT, PPS> >
    : strategy_converter<PPS>
{};


template <typename R, typename CT>
struct strategy_converter<strategy::distance::comparable::haversine<R, CT> >
{
    template <typename S>
    static auto get(S const& s)
    {
        return strategies::distance::detail::make_comparable(
                strategies::distance::spherical<R, CT>(s.radius()));
    }
};

template <typename CT, typename PPS>
struct strategy_converter<strategy::distance::comparable::cross_track<CT, PPS> >
    : strategy_converter<PPS>
{};


} // namespace services

}} // namespace strategies::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_SPHERICAL_HPP

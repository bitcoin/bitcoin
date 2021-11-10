// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/distance/comparable.hpp>
#include <boost/geometry/strategies/distance/detail.hpp>
#include <boost/geometry/strategies/distance/services.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategies/geographic/azimuth.hpp>

#include <boost/geometry/strategies/geographic/distance.hpp>
#include <boost/geometry/strategies/geographic/distance_cross_track.hpp>
#include <boost/geometry/strategies/geographic/distance_cross_track_box_box.hpp>
#include <boost/geometry/strategies/geographic/distance_cross_track_point_box.hpp>
#include <boost/geometry/strategies/geographic/distance_segment_box.hpp>
// TODO - for backwards compatibility, remove?
#include <boost/geometry/strategies/geographic/distance_andoyer.hpp>
#include <boost/geometry/strategies/geographic/distance_thomas.hpp>
#include <boost/geometry/strategies/geographic/distance_vincenty.hpp>

#include <boost/geometry/strategies/normalize.hpp>
#include <boost/geometry/strategies/relate/geographic.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace distance
{

// TODO: azimuth and normalize getters would not be needed if distance_segment_box was implemented differently
//       right now it calls disjoint algorithm details.

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
    : public strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType>
{
    using base_t = strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    // azimuth

    auto azimuth() const
    {
        return strategy::azimuth::geographic
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    // distance

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_pp_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic
                <
                    FormulaPolicy, Spheroid, CalculationType
                >(base_t::m_spheroid);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_pb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track_point_box
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_sb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_segment_box
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    template <typename Geometry1, typename Geometry2>
    auto distance(Geometry1 const&, Geometry2 const&,
                  detail::enable_if_bb_t<Geometry1, Geometry2> * = nullptr) const
    {
        return strategy::distance::geographic_cross_track_box_box
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
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


namespace services
{

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, geographic_tag, geographic_tag>
{
    using type = strategies::distance::geographic<>;
};


template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic<FP, S, CT> >
{
    static auto get(strategy::distance::geographic<FP, S, CT> const& s)
    {
        return strategies::distance::geographic<FP, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::andoyer<S, CT> >
{
    static auto get(strategy::distance::andoyer<S, CT> const& s)
    {
        return strategies::distance::geographic<strategy::andoyer, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::thomas<S, CT> >
{
    static auto get(strategy::distance::thomas<S, CT> const& s)
    {
        return strategies::distance::geographic<strategy::thomas, S, CT>(s.model());
    }
};
// TODO - for backwards compatibility, remove?
template <typename S, typename CT>
struct strategy_converter<strategy::distance::vincenty<S, CT> >
{
    static auto get(strategy::distance::vincenty<S, CT> const& s)
    {
        return strategies::distance::geographic<strategy::vincenty, S, CT>(s.model());
    }
};

template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic_cross_track<FP, S, CT> >
{
    static auto get(strategy::distance::geographic_cross_track<FP, S, CT> const& s)
    {
        return strategies::distance::geographic<FP, S, CT>(s.model());
    }
};

template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic_cross_track_point_box<FP, S, CT> >
{
    static auto get(strategy::distance::geographic_cross_track_point_box<FP, S, CT> const& s)
    {
        return strategies::distance::geographic<FP, S, CT>(s.model());
    }
};

template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic_segment_box<FP, S, CT> >
{
    static auto get(strategy::distance::geographic_segment_box<FP, S, CT> const& s)
    {
        return strategies::distance::geographic<FP, S, CT>(s.model());
    }
};

template <typename FP, typename S, typename CT>
struct strategy_converter<strategy::distance::geographic_cross_track_box_box<FP, S, CT> >
{
    static auto get(strategy::distance::geographic_cross_track_box_box<FP, S, CT> const& s)
    {
        return strategies::distance::geographic<FP, S, CT>(s.model());
    }
};


// details

// TODO: This specialization wouldn't be needed if strategy::distance::geographic_cross_track was implemented as an alias
template <typename FP, typename S, typename CT, bool B, bool ECP>
struct strategy_converter<strategy::distance::detail::geographic_cross_track<FP, S, CT, B, ECP> >
{
    struct altered_strategy
        : strategies::distance::geographic<FP, S, CT>
    {
        typedef strategies::distance::geographic<FP, S, CT> base_t;

        explicit altered_strategy(S const& s) : base_t(s) {}

        using base_t::distance;

        template <typename Geometry1, typename Geometry2>
        auto distance(Geometry1 const&, Geometry2 const&,
                      detail::enable_if_ps_t<Geometry1, Geometry2> * = nullptr) const
        {
            return strategy::distance::detail::geographic_cross_track
                <
                    FP, S, CT, B, ECP
                >(base_t::m_spheroid);
        }
    };

    static auto get(strategy::distance::detail::geographic_cross_track<FP, S, CT, B, ECP> const& s)
    {
        return altered_strategy(s.model());
    }
};


} // namespace services

}} // namespace strategies::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_GEOGRAPHIC_HPP

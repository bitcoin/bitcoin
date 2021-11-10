// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_RELATE_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_RELATE_GEOGRAPHIC_HPP


// TEMP - move to strategy
#include <boost/geometry/strategies/agnostic/point_in_box_by_side.hpp>
#include <boost/geometry/strategies/geographic/intersection.hpp>
#include <boost/geometry/strategies/geographic/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>
#include <boost/geometry/strategies/spherical/disjoint_box_box.hpp>

#include <boost/geometry/strategies/envelope/geographic.hpp>
#include <boost/geometry/strategies/relate/services.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategy/geographic/area.hpp>
#include <boost/geometry/strategy/geographic/area_box.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace relate
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
    : public strategies::envelope::geographic<FormulaPolicy, Spheroid, CalculationType>
{
    using base_t = strategies::envelope::geographic<FormulaPolicy, Spheroid, CalculationType>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    // area

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

    // covered_by

    template <typename Geometry1, typename Geometry2>
    static auto covered_by(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                                <
                                    util::is_pointlike<Geometry1>::value
                                 && util::is_box<Geometry2>::value
                                > * = nullptr)
    {
        return strategy::covered_by::spherical_point_box();
    }

    template <typename Geometry1, typename Geometry2>
    static auto covered_by(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                            <
                                util::is_box<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::covered_by::spherical_box_box();
    }

    // disjoint

    template <typename Geometry1, typename Geometry2>
    static auto disjoint(Geometry1 const&, Geometry2 const&,
                         std::enable_if_t
                            <
                                util::is_box<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::disjoint::spherical_box_box();
    }

    template <typename Geometry1, typename Geometry2>
    auto disjoint(Geometry1 const&, Geometry2 const&,
                  std::enable_if_t
                    <
                        util::is_segment<Geometry1>::value
                        && util::is_box<Geometry2>::value
                    > * = nullptr) const
    {
        // NOTE: Inconsistent name
        // The only disjoint(Seg, Box) strategy that takes CalculationType.
        return strategy::disjoint::segment_box_geographic
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    // relate

    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && util::is_pointlike<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::spherical_point_point();
    }

    template <typename Geometry1, typename Geometry2>
    auto relate(Geometry1 const&, Geometry2 const&,
                std::enable_if_t
                    <
                        util::is_pointlike<Geometry1>::value
                        && ( util::is_linear<Geometry2>::value
                        || util::is_polygonal<Geometry2>::value )
                    > * = nullptr) const
    {
        return strategy::within::geographic_winding
                <
                    void, void, FormulaPolicy, Spheroid, CalculationType
                >(base_t::m_spheroid);
    }

    //template <typename Geometry1, typename Geometry2>
    auto relate(/*Geometry1 const&, Geometry2 const&,
                std::enable_if_t
                    <
                        ( util::is_linear<Geometry1>::value
                        || util::is_polygonal<Geometry1>::value )
                        && ( util::is_linear<Geometry2>::value
                        || util::is_polygonal<Geometry2>::value )
                    > * = nullptr*/) const
    {
        return strategy::intersection::geographic_segments
            <
                FormulaPolicy,
                strategy::default_order<FormulaPolicy>::value,
                Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    // side

    auto side() const
    {
        return strategy::side::geographic
            <
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }

    // within

    template <typename Geometry1, typename Geometry2>
    static auto within(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                                && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::spherical_point_box();
    }

    template <typename Geometry1, typename Geometry2>
    static auto within(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_box<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::spherical_box_box();
    }
};


namespace services
{

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, geographic_tag, geographic_tag>
{
    using type = strategies::relate::geographic<>;
};


template <typename FormulaPolicy, typename Spheroid, typename CalculationType>
struct strategy_converter<strategy::disjoint::segment_box_geographic<FormulaPolicy, Spheroid, CalculationType>>
{
    static auto get(strategy::disjoint::segment_box_geographic<FormulaPolicy, Spheroid, CalculationType> const& s)
    {
        return strategies::relate::geographic
            <
                FormulaPolicy,
                Spheroid,
                CalculationType
            >(s.model());
    }
};

template <typename P1, typename P2, typename FormulaPolicy, typename Spheroid, typename CalculationType>
struct strategy_converter<strategy::within::geographic_winding<P1, P2, FormulaPolicy, Spheroid, CalculationType>>
{
    static auto get(strategy::within::geographic_winding<P1, P2, FormulaPolicy, Spheroid, CalculationType> const& s)
    {
        return strategies::relate::geographic
            <
                FormulaPolicy,
                Spheroid,
                CalculationType
            >(s.model());
    }
};

template <typename FormulaPolicy, std::size_t SeriesOrder, typename Spheroid, typename CalculationType>
struct strategy_converter<strategy::intersection::geographic_segments<FormulaPolicy, SeriesOrder, Spheroid, CalculationType>>
{
    struct altered_strategy
        : strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType>
    {
        typedef strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType> base_t;

        explicit altered_strategy(Spheroid const& spheroid)
            : base_t(spheroid)
        {}

        template <typename Geometry>
        auto area(Geometry const&) const
        {
            return strategy::area::geographic
                <
                    FormulaPolicy, SeriesOrder, Spheroid, CalculationType
                >(base_t::m_spheroid);
        }

        using base_t::relate;

        auto relate(/*...*/) const
        {
            return strategy::intersection::geographic_segments
                <
                    FormulaPolicy, SeriesOrder, Spheroid, CalculationType
                >(base_t::m_spheroid);
        }
    };

    static auto get(strategy::intersection::geographic_segments<FormulaPolicy, SeriesOrder, Spheroid, CalculationType> const& s)
    {
        return altered_strategy(s.model());
    }
};

template <typename FormulaPolicy, typename Spheroid, typename CalculationType>
struct strategy_converter<strategy::within::geographic_point_box_by_side<FormulaPolicy, Spheroid, CalculationType>>
{
    struct altered_strategy
        : strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType>
    {
        altered_strategy(Spheroid const& spheroid)
            : strategies::relate::geographic<FormulaPolicy, Spheroid, CalculationType>(spheroid)
        {}

        template <typename Geometry1, typename Geometry2>
        auto covered_by(Geometry1 const&, Geometry2 const&,
                        std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                                && util::is_box<Geometry2>::value
                            > * = nullptr) const
        {
            return strategy::covered_by::geographic_point_box_by_side
                <
                    FormulaPolicy, Spheroid, CalculationType
                >(this->model());
        }

        template <typename Geometry1, typename Geometry2>
        auto within(Geometry1 const&, Geometry2 const&,
                    std::enable_if_t
                        <
                            util::is_pointlike<Geometry1>::value
                            && util::is_box<Geometry2>::value
                        > * = nullptr) const
        {
            return strategy::within::geographic_point_box_by_side
                <
                    FormulaPolicy, Spheroid, CalculationType
                >(this->model());
        }
    };

    static auto get(strategy::covered_by::geographic_point_box_by_side<FormulaPolicy, Spheroid, CalculationType> const& s)
    {
        return altered_strategy(s.model());
    }

    static auto get(strategy::within::geographic_point_box_by_side<FormulaPolicy, Spheroid, CalculationType> const& s)
    {
        return altered_strategy(s.model());
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::covered_by::geographic_point_box_by_side<CalculationType>>
    : strategy_converter<strategy::within::geographic_point_box_by_side<CalculationType>>
{};


} // namespace services

}} // namespace strategies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_RELATE_GEOGRAPHIC_HPP

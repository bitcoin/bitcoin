// Boost.Geometry

// Copyright (c) 2020-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_RELATE_SPHERICAL_HPP
#define BOOST_GEOMETRY_STRATEGIES_RELATE_SPHERICAL_HPP


// TEMP - move to strategy
#include <boost/geometry/strategies/agnostic/point_in_box_by_side.hpp>
#include <boost/geometry/strategies/cartesian/box_in_box.hpp>
#include <boost/geometry/strategies/spherical/intersection.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>
#include <boost/geometry/strategies/spherical/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/spherical/disjoint_box_box.hpp>

#include <boost/geometry/strategies/envelope/spherical.hpp>
#include <boost/geometry/strategies/relate/services.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategy/spherical/area.hpp>
#include <boost/geometry/strategy/spherical/area_box.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace relate
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename RadiusTypeOrSphere, typename CalculationType>
class spherical
    : public strategies::envelope::detail::spherical<RadiusTypeOrSphere, CalculationType>
{
    using base_t = strategies::envelope::detail::spherical<RadiusTypeOrSphere, CalculationType>;

public:

    spherical() = default;

    template <typename RadiusOrSphere>
    explicit spherical(RadiusOrSphere const& radius_or_sphere)
        : strategies::envelope::detail::spherical<RadiusTypeOrSphere, CalculationType>(radius_or_sphere)
    {}

    // area

    template <typename Geometry>
    auto area(Geometry const&,
              std::enable_if_t<! util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::spherical
            <
                typename base_t::radius_type, CalculationType
            >(base_t::radius());
    }

    template <typename Geometry>
    auto area(Geometry const&,
              std::enable_if_t<util::is_box<Geometry>::value> * = nullptr) const
    {
        return strategy::area::spherical_box
            <
                typename base_t::radius_type, CalculationType
            >(base_t::radius());
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
    static auto disjoint(Geometry1 const&, Geometry2 const&,
                         std::enable_if_t
                            <
                                util::is_segment<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        // NOTE: Inconsistent name.
        return strategy::disjoint::segment_box_spherical();
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
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && ( util::is_linear<Geometry2>::value
                               || util::is_polygonal<Geometry2>::value )
                            > * = nullptr)
    {
        return strategy::within::spherical_winding<void, void, CalculationType>();
    }

    //template <typename Geometry1, typename Geometry2>
    static auto relate(/*Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                ( util::is_linear<Geometry1>::value
                               || util::is_polygonal<Geometry1>::value )
                             && ( util::is_linear<Geometry2>::value
                               || util::is_polygonal<Geometry2>::value )
                            > * = nullptr*/)
    {
        return strategy::intersection::spherical_segments<CalculationType>();
    }

    // side

    static auto side()
    {
        return strategy::side::spherical_side_formula<CalculationType>();
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


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template <typename CalculationType = void>
class spherical
    : public strategies::relate::detail::spherical<void, CalculationType>
{};


namespace services
{

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, spherical_tag, spherical_tag>
{
    using type = strategies::relate::spherical<>;
};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, spherical_equatorial_tag, spherical_equatorial_tag>
{
    using type = strategies::relate::spherical<>;
};

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, spherical_polar_tag, spherical_polar_tag>
{
    using type = strategies::relate::spherical<>;
};


template <>
struct strategy_converter<strategy::within::spherical_point_point>
{
    static auto get(strategy::within::spherical_point_point const& )
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::within::spherical_point_box>
{
    static auto get(strategy::within::spherical_point_box const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::covered_by::spherical_point_box>
{
    static auto get(strategy::covered_by::spherical_point_box const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::covered_by::spherical_box_box>
{
    static auto get(strategy::covered_by::spherical_box_box const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::disjoint::spherical_box_box>
{
    static auto get(strategy::disjoint::spherical_box_box const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::disjoint::segment_box_spherical>
{
    static auto get(strategy::disjoint::segment_box_spherical const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <>
struct strategy_converter<strategy::within::spherical_box_box>
{
    static auto get(strategy::within::spherical_box_box const&)
    {
        return strategies::relate::spherical<>();
    }
};

template <typename P1, typename P2, typename CalculationType>
struct strategy_converter<strategy::within::spherical_winding<P1, P2, CalculationType>>
{
    static auto get(strategy::within::spherical_winding<P1, P2, CalculationType> const& )
    {
        return strategies::relate::spherical<CalculationType>();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::intersection::spherical_segments<CalculationType>>
{
    static auto get(strategy::intersection::spherical_segments<CalculationType> const& )
    {
        return strategies::relate::spherical<CalculationType>();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::within::spherical_point_box_by_side<CalculationType>>
{
    struct altered_strategy
        : strategies::relate::spherical<CalculationType>
    {
        template <typename Geometry1, typename Geometry2>
        static auto covered_by(Geometry1 const&, Geometry2 const&,
                               std::enable_if_t
                                    <
                                        util::is_pointlike<Geometry1>::value
                                     && util::is_box<Geometry2>::value
                                    > * = nullptr)
        {
            return strategy::covered_by::spherical_point_box_by_side<CalculationType>();
        }

        template <typename Geometry1, typename Geometry2>
        static auto within(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                                <
                                    util::is_pointlike<Geometry1>::value
                                    && util::is_box<Geometry2>::value
                                > * = nullptr)
        {
            return strategy::within::spherical_point_box_by_side<CalculationType>();
        }
    };

    static auto get(strategy::covered_by::spherical_point_box_by_side<CalculationType> const&)
    {
        return altered_strategy();
    }

    static auto get(strategy::within::spherical_point_box_by_side<CalculationType> const&)
    {
        return altered_strategy();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::covered_by::spherical_point_box_by_side<CalculationType>>
    : strategy_converter<strategy::within::spherical_point_box_by_side<CalculationType>>
{};

// TEMP used in distance segment/box
template <typename CalculationType>
struct strategy_converter<strategy::side::spherical_side_formula<CalculationType>>
{
    static auto get(strategy::side::spherical_side_formula<CalculationType> const& )
    {
        return strategies::relate::spherical<CalculationType>();
    }
};


} // namespace services

}} // namespace strategies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_RELATE_SPHERICAL_HPP

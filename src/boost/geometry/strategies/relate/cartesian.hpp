// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_RELATE_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_RELATE_CARTESIAN_HPP


// TEMP - move to strategy
#include <boost/geometry/strategies/agnostic/point_in_box_by_side.hpp>
#include <boost/geometry/strategies/cartesian/intersection.hpp>
#include <boost/geometry/strategies/cartesian/box_in_box.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/cartesian/point_in_poly_crossings_multiply.hpp>
#include <boost/geometry/strategies/cartesian/point_in_poly_franklin.hpp>
#include <boost/geometry/strategies/cartesian/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/cartesian/disjoint_box_box.hpp>

#include <boost/geometry/strategies/envelope/cartesian.hpp>
#include <boost/geometry/strategies/relate/services.hpp>
#include <boost/geometry/strategies/detail.hpp>

#include <boost/geometry/strategy/cartesian/area.hpp>
#include <boost/geometry/strategy/cartesian/area_box.hpp>

#include <boost/geometry/util/type_traits.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace relate
{

template <typename CalculationType = void>
class cartesian
    : public strategies::envelope::cartesian<CalculationType>
{
public:
    //area

    template <typename Geometry>
    static auto area(Geometry const&,
                     std::enable_if_t<! util::is_box<Geometry>::value> * = nullptr)
    {
        return strategy::area::cartesian<CalculationType>();
    }

    template <typename Geometry>
    static auto area(Geometry const&,
                     std::enable_if_t<util::is_box<Geometry>::value> * = nullptr)
    {
        return strategy::area::cartesian_box<CalculationType>();
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
        return strategy::covered_by::cartesian_point_box();
    }

    template <typename Geometry1, typename Geometry2>
    static auto covered_by(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                            <
                                util::is_box<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::covered_by::cartesian_box_box();
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
        return strategy::disjoint::cartesian_box_box();
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
        return strategy::disjoint::segment_box();
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
        return strategy::within::cartesian_point_point();
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
        return strategy::within::cartesian_winding<void, void, CalculationType>();
    }

    // The problem is that this strategy is often used with non-geometry ranges.
    // So dispatching only by geometry categories is impossible.
    // In the past it was taking two segments, now it takes 3-point sub-ranges.
    // So dispatching by segments is impossible.
    // It could be dispatched by (linear || polygonal || non-geometry point range).
    // For now implement as 0-parameter, special case relate.

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
        return strategy::intersection::cartesian_segments<CalculationType>();
    }

    // side

    static auto side()
    {
        return strategy::side::side_by_triangle<CalculationType>();
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
        return strategy::within::cartesian_point_box();
    }

    template <typename Geometry1, typename Geometry2>
    static auto within(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_box<Geometry1>::value
                             && util::is_box<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::cartesian_box_box();
    }
};


namespace services
{

template <typename Geometry1, typename Geometry2>
struct default_strategy<Geometry1, Geometry2, cartesian_tag, cartesian_tag>
{
    using type = strategies::relate::cartesian<>;
};


template <>
struct strategy_converter<strategy::within::cartesian_point_point>
{
    static auto get(strategy::within::cartesian_point_point const& )
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::within::cartesian_point_box>
{
    static auto get(strategy::within::cartesian_point_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::covered_by::cartesian_point_box>
{
    static auto get(strategy::covered_by::cartesian_point_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::covered_by::cartesian_box_box>
{
    static auto get(strategy::covered_by::cartesian_box_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::disjoint::cartesian_box_box>
{
    static auto get(strategy::disjoint::cartesian_box_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::disjoint::segment_box>
{
    static auto get(strategy::disjoint::segment_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::within::cartesian_box_box>
{
    static auto get(strategy::within::cartesian_box_box const&)
    {
        return strategies::relate::cartesian<>();
    }
};

template <typename P1, typename P2, typename CalculationType>
struct strategy_converter<strategy::within::cartesian_winding<P1, P2, CalculationType>>
{
    static auto get(strategy::within::cartesian_winding<P1, P2, CalculationType> const& )
    {
        return strategies::relate::cartesian<CalculationType>();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::intersection::cartesian_segments<CalculationType>>
{
    static auto get(strategy::intersection::cartesian_segments<CalculationType> const& )
    {
        return strategies::relate::cartesian<CalculationType>();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::within::cartesian_point_box_by_side<CalculationType>>
{
    struct altered_strategy
        : strategies::relate::cartesian<CalculationType>
    {
        template <typename Geometry1, typename Geometry2>
        static auto covered_by(Geometry1 const&, Geometry2 const&,
                               std::enable_if_t
                                    <
                                        util::is_pointlike<Geometry1>::value
                                     && util::is_box<Geometry2>::value
                                    > * = nullptr)
        {
            return strategy::covered_by::cartesian_point_box_by_side<CalculationType>();
        }

        template <typename Geometry1, typename Geometry2>
        static auto within(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                                <
                                    util::is_pointlike<Geometry1>::value
                                    && util::is_box<Geometry2>::value
                                > * = nullptr)
        {
            return strategy::within::cartesian_point_box_by_side<CalculationType>();
        }
    };

    static auto get(strategy::covered_by::cartesian_point_box_by_side<CalculationType> const&)
    {
        return altered_strategy();
    }

    static auto get(strategy::within::cartesian_point_box_by_side<CalculationType> const&)
    {
        return altered_strategy();
    }
};

template <typename CalculationType>
struct strategy_converter<strategy::covered_by::cartesian_point_box_by_side<CalculationType>>
    : strategy_converter<strategy::within::cartesian_point_box_by_side<CalculationType>>
{};

template <typename P1, typename P2, typename CalculationType>
struct strategy_converter<strategy::within::franklin<P1, P2, CalculationType>>
{
    struct altered_strategy
        : strategies::relate::cartesian<CalculationType>
    {
        template <typename Geometry1, typename Geometry2>
        static auto relate(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                                <
                                    util::is_pointlike<Geometry1>::value
                                 && ( util::is_linear<Geometry2>::value
                                   || util::is_polygonal<Geometry2>::value )
                                > * = nullptr)
        {
            return strategy::within::franklin<void, void, CalculationType>();
        }
    };

    static auto get(strategy::within::franklin<P1, P2, CalculationType> const&)
    {
        return altered_strategy();
    }
};

template <typename P1, typename P2, typename CalculationType>
struct strategy_converter<strategy::within::crossings_multiply<P1, P2, CalculationType>>
{
    struct altered_strategy
        : strategies::relate::cartesian<CalculationType>
    {
        template <typename Geometry1, typename Geometry2>
        static auto relate(Geometry1 const&, Geometry2 const&,
                           std::enable_if_t
                                <
                                    util::is_pointlike<Geometry1>::value
                                 && ( util::is_linear<Geometry2>::value
                                   || util::is_polygonal<Geometry2>::value )
                                > * = nullptr)
        {
            return strategy::within::crossings_multiply<void, void, CalculationType>();
        }
    };

    static auto get(strategy::within::crossings_multiply<P1, P2, CalculationType> const&)
    {
        return altered_strategy();
    }
};

// TEMP used in distance segment/box
template <typename CalculationType>
struct strategy_converter<strategy::side::side_by_triangle<CalculationType>>
{
    static auto get(strategy::side::side_by_triangle<CalculationType> const&)
    {
        return strategies::relate::cartesian<CalculationType>();
    }
};


} // namespace services

}} // namespace strategies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_RELATE_CARTESIAN_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2014 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_INTERFACE_HPP

#include <boost/concept_check.hpp>

#include <boost/geometry/algorithms/detail/throw_on_empty_input.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>

// TODO: move these to algorithms
#include <boost/geometry/strategies/default_distance_result.hpp>
#include <boost/geometry/strategies/distance_result.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/distance/services.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// If reversal is needed, perform it
template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    typename Tag1, typename Tag2, typename StrategyTag
>
struct distance
<
    Geometry1, Geometry2, Strategy,
    Tag1, Tag2, StrategyTag,
    true
>
    : distance<Geometry2, Geometry1, Strategy, Tag2, Tag1, StrategyTag, false>
{
    static inline auto apply(Geometry1 const& g1, Geometry2 const& g2,
                             Strategy const& strategy)
    {
        return distance
            <
                Geometry2, Geometry1, Strategy,
                Tag2, Tag1, StrategyTag,
                false
            >::apply(g2, g1, strategy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct distance
{
    template <typename Geometry1, typename Geometry2>
    static inline typename distance_result<Geometry1, Geometry2, Strategy>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        return dispatch::distance
            <
                Geometry1, Geometry2, Strategy
            >::apply(geometry1, geometry2, strategy);
    }
};

template <typename Strategy>
struct is_strategy_converter_specialized
{
    typedef strategies::distance::services::strategy_converter<Strategy> converter;
    static const bool value = ! std::is_same
        <
            decltype(converter::get(std::declval<Strategy>())),
            strategies::detail::not_implemented
        >::value;
};

template <typename Strategy>
struct distance<Strategy, false>
{
    template
    <
        typename Geometry1, typename Geometry2, typename S,
        std::enable_if_t<is_strategy_converter_specialized<S>::value, int> = 0
    >
    static inline
    typename distance_result<Geometry1, Geometry2, S>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          S const& strategy)
    {
        typedef strategies::distance::services::strategy_converter<Strategy> converter;
        typedef decltype(converter::get(strategy)) strategy_type;

        return dispatch::distance
            <
                Geometry1, Geometry2, strategy_type
            >::apply(geometry1, geometry2, converter::get(strategy));
    }

    template
    <
        typename Geometry1, typename Geometry2, typename S,
        std::enable_if_t<! is_strategy_converter_specialized<S>::value, int> = 0
    >
    static inline
    typename distance_result<Geometry1, Geometry2, S>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          S const& strategy)
    {
        typedef strategies::distance::services::custom_strategy_converter
            <
                Geometry1, Geometry2, Strategy
            > converter;
        typedef decltype(converter::get(strategy)) strategy_type;

        return dispatch::distance
            <
                Geometry1, Geometry2, strategy_type
            >::apply(geometry1, geometry2, converter::get(strategy));
    }
};

template <>
struct distance<default_strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline
    typename distance_result<Geometry1, Geometry2, default_strategy>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          default_strategy)
    {
        typedef typename strategies::distance::services::default_strategy
            <
                Geometry1, Geometry2
            >::type strategy_type;

        return dispatch::distance
            <
                Geometry1, Geometry2, strategy_type
            >::apply(geometry1, geometry2, strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant
{


template <typename Geometry1, typename Geometry2>
struct distance
{
    template <typename Strategy>
    static inline typename distance_result<Geometry1, Geometry2, Strategy>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        return resolve_strategy::distance
            <
                Strategy
            >::apply(geometry1, geometry2, strategy);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct distance<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename distance_result
                <
                    variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
                    Geometry2,
                    Strategy
                >::type
        >
    {
        Geometry2 const& m_geometry2;
        Strategy const& m_strategy;

        visitor(Geometry2 const& geometry2,
                Strategy const& strategy)
            : m_geometry2(geometry2),
              m_strategy(strategy)
        {}

        template <typename Geometry1>
        typename distance_result<Geometry1, Geometry2, Strategy>::type
        operator()(Geometry1 const& geometry1) const
        {
            return distance
                <
                    Geometry1,
                    Geometry2
                >::template apply
                    <
                        Strategy
                    >(geometry1, m_geometry2, m_strategy);
        }
    };

    template <typename Strategy>
    static inline typename distance_result
        <
            variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
            Geometry2,
            Strategy
        >::type
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<Strategy>(geometry2, strategy), geometry1);
    }
};


template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct distance<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename distance_result
                <
                    Geometry1,
                    variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
                    Strategy
                >::type
        >
    {
        Geometry1 const& m_geometry1;
        Strategy const& m_strategy;

        visitor(Geometry1 const& geometry1,
                Strategy const& strategy)
            : m_geometry1(geometry1),
              m_strategy(strategy)
        {}

        template <typename Geometry2>
        typename distance_result<Geometry1, Geometry2, Strategy>::type
        operator()(Geometry2 const& geometry2) const
        {
            return distance
                <
                    Geometry1,
                    Geometry2
                >::template apply
                <
                    Strategy
                >(m_geometry1, geometry2, m_strategy);
        }
    };

    template <typename Strategy>
    static inline typename distance_result
        <
            Geometry1,
            variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
            Strategy
        >::type
    apply(
        Geometry1 const& geometry1,
        const variant<BOOST_VARIANT_ENUM_PARAMS(T)>& geometry2,
        Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<Strategy>(geometry1, strategy), geometry2);
    }
};


template
<
    BOOST_VARIANT_ENUM_PARAMS(typename T1),
    BOOST_VARIANT_ENUM_PARAMS(typename T2)
>
struct distance
    <
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)>,
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)>
    >
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename distance_result
                <
                    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)>,
                    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)>,
                    Strategy
                >::type
        >
    {
        Strategy const& m_strategy;

        visitor(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Geometry1, typename Geometry2>
        typename distance_result<Geometry1, Geometry2, Strategy>::type
        operator()(Geometry1 const& geometry1, Geometry2 const& geometry2) const
        {
            return distance
                <
                    Geometry1,
                    Geometry2
                >::template apply
                <
                    Strategy
                >(geometry1, geometry2, m_strategy);
        }
    };

    template <typename Strategy>
    static inline typename distance_result
        <
            boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)>,
            boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)>,
            Strategy
        >::type
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)> const& geometry1,
          boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<Strategy>(strategy), geometry1, geometry2);
    }
};

} // namespace resolve_variant


/*!
\brief Calculate the distance between two geometries \brief_strategy
\ingroup distance
\details
\details The free function distance calculates the distance between two geometries \brief_strategy. \details_strategy_reasons

\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy \tparam_strategy{Distance}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{distance}
\return \return_calc{distance}
\note The strategy can be a point-point strategy. In case of distance point-line/point-polygon
    it may also be a point-segment strategy.

\qbk{distinguish,with strategy}

\qbk{
[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_distance_pythagoras Pythagoras (cartesian)]
\* [link geometry.reference.strategies.strategy_distance_haversine Haversine (spherical)]
\* [link geometry.reference.strategies.strategy_distance_cross_track Cross track (spherical\, point-to-segment)]
\* [link geometry.reference.strategies.strategy_distance_projected_point Projected point (cartesian\, point-to-segment)]
\* more (currently extensions): Vincenty\, Andoyer (geographic)
}
 */

/*
Note, in case of a Compilation Error:
if you get:
 - "Failed to specialize function template ..."
 - "error: no matching function for call to ..."
for distance, it is probably so that there is no specialization
for return_type<...> for your strategy.
*/
template <typename Geometry1, typename Geometry2, typename Strategy>
inline typename distance_result<Geometry1, Geometry2, Strategy>::type
distance(Geometry1 const& geometry1,
         Geometry2 const& geometry2,
         Strategy const& strategy)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    detail::throw_on_empty_input(geometry1);
    detail::throw_on_empty_input(geometry2);

    return resolve_variant::distance
               <
                   Geometry1,
                   Geometry2
               >::apply(geometry1, geometry2, strategy);
}


/*!
\brief Calculate the distance between two geometries.
\ingroup distance
\details The free function distance calculates the distance between two geometries. \details_default_strategy

\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_calc{distance}

\qbk{[include reference/algorithms/distance.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline typename default_distance_result<Geometry1, Geometry2>::type
distance(Geometry1 const& geometry1,
         Geometry2 const& geometry2)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return geometry::distance(geometry1, geometry2, default_strategy());
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_INTERFACE_HPP

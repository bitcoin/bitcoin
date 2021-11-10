// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_COMPARABLE_DISTANCE_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_COMPARABLE_DISTANCE_INTERFACE_HPP


#include <boost/geometry/algorithms/detail/distance/interface.hpp>

#include <boost/geometry/geometries/adapted/boost_variant.hpp> // For backward compatibility
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/comparable_distance_result.hpp>
#include <boost/geometry/strategies/default_comparable_distance_result.hpp>
#include <boost/geometry/strategies/distance.hpp>

#include <boost/geometry/strategies/distance/comparable.hpp>
#include <boost/geometry/strategies/distance/services.hpp>


namespace boost { namespace geometry
{


namespace resolve_strategy
{


template
<
    typename Strategies,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategies>::value
>
struct comparable_distance
{
    template <typename Geometry1, typename Geometry2>
    static inline
    typename comparable_distance_result<Geometry1, Geometry2, Strategies>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          Strategies const& strategies)
    {
        return dispatch::distance
            <
                Geometry1, Geometry2,
                strategies::distance::detail::comparable<Strategies>
            >::apply(geometry1,
                     geometry2,
                     strategies::distance::detail::comparable<Strategies>(strategies));
    }
};

template <typename Strategy>
struct comparable_distance<Strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline
    typename comparable_distance_result<Geometry1, Geometry2, Strategy>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        using strategies::distance::services::strategy_converter;

        typedef decltype(strategy_converter<Strategy>::get(strategy))
            strategies_type;
        typedef strategies::distance::detail::comparable
            <
                strategies_type
            > comparable_strategies_type;
        
        return dispatch::distance
            <
                Geometry1, Geometry2,
                comparable_strategies_type
            >::apply(geometry1,
                     geometry2,
                     comparable_strategies_type(
                         strategy_converter<Strategy>::get(strategy)));
    }
};

template <>
struct comparable_distance<default_strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline typename comparable_distance_result
        <
            Geometry1, Geometry2, default_strategy
        >::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          default_strategy)
    {
        typedef strategies::distance::detail::comparable
            <
                typename strategies::distance::services::default_strategy
                    <
                        Geometry1, Geometry2
                    >::type
            > comparable_strategy_type;

        return dispatch::distance
            <
                Geometry1, Geometry2, comparable_strategy_type
            >::apply(geometry1, geometry2, comparable_strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant
{


template <typename Geometry1, typename Geometry2>
struct comparable_distance
{
    template <typename Strategy>
    static inline
    typename comparable_distance_result<Geometry1, Geometry2, Strategy>::type
    apply(Geometry1 const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        return resolve_strategy::comparable_distance
            <
                Strategy
            >::apply(geometry1, geometry2, strategy);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct comparable_distance
    <
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
        Geometry2
    >
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename comparable_distance_result
                <
                    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
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
        typename comparable_distance_result
            <
                Geometry1, Geometry2, Strategy
            >::type
        operator()(Geometry1 const& geometry1) const
        {
            return comparable_distance
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
    static inline typename comparable_distance_result
        <
            boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
            Geometry2,
            Strategy
        >::type
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
          Geometry2 const& geometry2,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<Strategy>(geometry2, strategy), geometry1);
    }
};


template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct comparable_distance
    <
        Geometry1,
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>
    >
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename comparable_distance_result
                <
                    Geometry1,
                    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
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
        typename comparable_distance_result
            <
                Geometry1, Geometry2, Strategy
            >::type
        operator()(Geometry2 const& geometry2) const
        {
            return comparable_distance
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
    static inline typename comparable_distance_result
        <
            Geometry1,
            boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>,
            Strategy
        >::type
    apply(Geometry1 const& geometry1,
          boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2,
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
struct comparable_distance
    <
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)>,
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)>
    >
{
    template <typename Strategy>
    struct visitor: static_visitor
        <
            typename comparable_distance_result
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
        typename comparable_distance_result
            <
                Geometry1, Geometry2, Strategy
            >::type
        operator()(Geometry1 const& geometry1, Geometry2 const& geometry2) const
        {
            return comparable_distance
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
    static inline typename comparable_distance_result
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
\brief \brief_calc2{comparable distance measurement} \brief_strategy
\ingroup distance
\details The free function comparable_distance does not necessarily calculate the distance,
    but it calculates a distance measure such that two distances are comparable to each other.
    For example: for the Cartesian coordinate system, Pythagoras is used but the square root
    is not taken, which makes it faster and the results of two point pairs can still be
    compared to each other.
\tparam Geometry1 first geometry type
\tparam Geometry2 second geometry type
\tparam Strategy \tparam_strategy{Distance}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{distance}
\return \return_calc{comparable distance}

\qbk{distinguish,with strategy}
 */
template <typename Geometry1, typename Geometry2, typename Strategy>
inline typename comparable_distance_result<Geometry1, Geometry2, Strategy>::type
comparable_distance(Geometry1 const& geometry1, Geometry2 const& geometry2,
                    Strategy const& strategy)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return resolve_variant::comparable_distance
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, strategy);
}



/*!
\brief \brief_calc2{comparable distance measurement}
\ingroup distance
\details The free function comparable_distance does not necessarily calculate the distance,
    but it calculates a distance measure such that two distances are comparable to each other.
    For example: for the Cartesian coordinate system, Pythagoras is used but the square root
    is not taken, which makes it faster and the results of two point pairs can still be
    compared to each other.
\tparam Geometry1 first geometry type
\tparam Geometry2 second geometry type
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_calc{comparable distance}

\qbk{[include reference/algorithms/comparable_distance.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline typename default_comparable_distance_result<Geometry1, Geometry2>::type
comparable_distance(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    return geometry::comparable_distance(geometry1, geometry2, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_COMPARABLE_DISTANCE_INTERFACE_HPP

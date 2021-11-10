// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_INTERFACE_HPP

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/dispatch/is_simple.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/services.hpp>


namespace boost { namespace geometry
{

namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct is_simple
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry,
                             Strategy const& strategy)
    {
        return dispatch::is_simple<Geometry>::apply(geometry, strategy);
    }
};

template <typename Strategy>
struct is_simple<Strategy, false>
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry,
                             Strategy const& strategy)
    {
        using strategies::relate::services::strategy_converter;
        return dispatch::is_simple
            <
                Geometry
            >::apply(geometry, strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct is_simple<default_strategy, false>
{
    template <typename Geometry>
    static inline bool apply(Geometry const& geometry,
                             default_strategy)
    {
        // NOTE: Currently the strategy is only used for Linear geometries
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry, Geometry
            >::type strategy_type;

        return dispatch::is_simple<Geometry>::apply(geometry, strategy_type());
    }
};

} // namespace resolve_strategy

namespace resolve_variant
{

template <typename Geometry>
struct is_simple
{
    template <typename Strategy>
    static inline bool apply(Geometry const& geometry, Strategy const& strategy)
    {
        concepts::check<Geometry const>();

        return resolve_strategy::is_simple<Strategy>::apply(geometry, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct is_simple<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Strategy>
    struct visitor : boost::static_visitor<bool>
    {
        Strategy const& m_strategy;

        visitor(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Geometry>
        bool operator()(Geometry const& geometry) const
        {
            return is_simple<Geometry>::apply(geometry, m_strategy);
        }
    };

    template <typename Strategy>
    static inline bool
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<Strategy>(strategy), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_check{is simple}
\ingroup is_simple
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Is_simple}
\param geometry \param_geometry
\param strategy \param_strategy{is_simple}
\return \return_check{is simple}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/is_simple.qbk]}
*/
template <typename Geometry, typename Strategy>
inline bool is_simple(Geometry const& geometry, Strategy const& strategy)
{
    return resolve_variant::is_simple<Geometry>::apply(geometry, strategy);
}


/*!
\brief \brief_check{is simple}
\ingroup is_simple
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is simple}

\qbk{[include reference/algorithms/is_simple.qbk]}
*/
template <typename Geometry>
inline bool is_simple(Geometry const& geometry)
{
    return resolve_variant::is_simple<Geometry>::apply(geometry, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_INTERFACE_HPP

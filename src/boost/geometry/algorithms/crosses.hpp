// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP

#include <cstddef>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/relate.hpp>
#include <boost/geometry/algorithms/detail/relate/relate_impl.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/cartesian.hpp>
#include <boost/geometry/strategies/relate/geographic.hpp>
#include <boost/geometry/strategies/relate/spherical.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry1,
    typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type
>
struct crosses
    : detail::relate::relate_impl
        <
            detail::de9im::static_mask_crosses_type,
            Geometry1,
            Geometry2
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct crosses
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        concepts::check<Geometry1 const>();
        concepts::check<Geometry2 const>();

        return dispatch::crosses
            <
                Geometry1, Geometry2
            >::apply(geometry1, geometry2, strategy);
    }
};

template <typename Strategy>
struct crosses<Strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        //using strategies::crosses::services::strategy_converter;
        using strategies::relate::services::strategy_converter;
        return crosses
            <
                decltype(strategy_converter<Strategy>::get(strategy))
            >::apply(geometry1, geometry2,
                     strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct crosses<default_strategy, false>
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             default_strategy)
    {
        //typedef typename strategies::crosses::services::default_strategy
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry1,
                Geometry2
            >::type strategy_type;

        return crosses
            <
                strategy_type
            >::apply(geometry1, geometry2, strategy_type());
    }
};

} // namespace resolve_strategy


namespace resolve_variant
{
    template <typename Geometry1, typename Geometry2>
    struct crosses
    {
        template <typename Strategy>
        static inline bool apply(Geometry1 const& geometry1,
                                 Geometry2 const& geometry2,
                                 Strategy const& strategy)
        {
            return resolve_strategy::crosses
                <
                    Strategy
                >::apply(geometry1, geometry2, strategy);
        }
    };
    
    
    template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
    struct crosses<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
    {
        template <typename Strategy>
        struct visitor: static_visitor<bool>
        {
            Geometry2 const& m_geometry2;
            Strategy const& m_strategy;
            
            visitor(Geometry2 const& geometry2, Strategy const& strategy)
                : m_geometry2(geometry2)
                , m_strategy(strategy)
            {}
            
            template <typename Geometry1>
            result_type operator()(Geometry1 const& geometry1) const
            {
                return crosses
                    <
                        Geometry1,
                        Geometry2
                    >::apply(geometry1, m_geometry2, m_strategy);
            }
        };
        
        template <typename Strategy>
        static inline bool apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
                                 Geometry2 const& geometry2,
                                 Strategy const& strategy)
        {
            return boost::apply_visitor(visitor<Strategy>(geometry2, strategy), geometry1);
        }
    };
    
    
    template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct crosses<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    {
        template <typename Strategy>
        struct visitor: static_visitor<bool>
        {
            Geometry1 const& m_geometry1;
            Strategy const& m_strategy;
            
            visitor(Geometry1 const& geometry1, Strategy const& strategy)
                : m_geometry1(geometry1)
                , m_strategy(strategy)
            {}
            
            template <typename Geometry2>
            result_type operator()(Geometry2 const& geometry2) const
            {
                return crosses
                    <
                        Geometry1,
                        Geometry2
                    >::apply(m_geometry1, geometry2, m_strategy);
            }
        };
        
        template <typename Strategy>
        static inline bool apply(Geometry1 const& geometry1,
                                 variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2,
                                 Strategy const& strategy)
        {
            return boost::apply_visitor(visitor<Strategy>(geometry1, strategy), geometry2);
        }
    };
    
    
    template <BOOST_VARIANT_ENUM_PARAMS(typename T1), BOOST_VARIANT_ENUM_PARAMS(typename T2)>
    struct crosses<variant<BOOST_VARIANT_ENUM_PARAMS(T1)>, variant<BOOST_VARIANT_ENUM_PARAMS(T2)> >
    {
        template <typename Strategy>
        struct visitor: static_visitor<bool>
        {
            Strategy const& m_strategy;

            visitor(Strategy const& strategy)
                : m_strategy(strategy)
            {}

            template <typename Geometry1, typename Geometry2>
            result_type operator()(Geometry1 const& geometry1,
                                   Geometry2 const& geometry2) const
            {
                return crosses
                    <
                        Geometry1,
                        Geometry2
                    >::apply(geometry1, geometry2, m_strategy);
            }
        };
        
        template <typename Strategy>
        static inline bool apply(variant<BOOST_VARIANT_ENUM_PARAMS(T1)> const& geometry1,
                                 variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2,
                                  Strategy const& strategy)
        {
            return boost::apply_visitor(visitor<Strategy>(strategy), geometry1, geometry2);
        }
    };
    
} // namespace resolve_variant
    
    
/*!
\brief \brief_check2{crosses}
\ingroup crosses
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy \tparam_strategy{Crosses}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{crosses}
\return \return_check2{crosses}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/crosses.qbk]}
*/
template <typename Geometry1, typename Geometry2, typename Strategy>
inline bool crosses(Geometry1 const& geometry1,
                    Geometry2 const& geometry2,
                    Strategy const& strategy)
{
    return resolve_variant::crosses
            <
                Geometry1, Geometry2
            >::apply(geometry1, geometry2, strategy);
}

/*!
\brief \brief_check2{crosses}
\ingroup crosses
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{crosses}

\qbk{[include reference/algorithms/crosses.qbk]}
\qbk{
[heading Examples]
[crosses]
[crosses_output]
}
*/
template <typename Geometry1, typename Geometry2>
inline bool crosses(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    return resolve_variant::crosses
            <
                Geometry1, Geometry2
            >::apply(geometry1, geometry2, default_strategy());
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP

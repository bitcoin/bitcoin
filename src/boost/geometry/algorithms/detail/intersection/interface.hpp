// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP


#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/detail/overlay/intersection_insert.hpp>
#include <boost/geometry/algorithms/detail/tupled_output.hpp>
#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/services.hpp>
#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// By default, all is forwarded to the intersection_insert-dispatcher
template
<
    typename Geometry1, typename Geometry2,
    typename Tag1 = typename geometry::tag<Geometry1>::type,
    typename Tag2 = typename geometry::tag<Geometry2>::type,
    bool Reverse = reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct intersection
{
    template <typename RobustPolicy, typename GeometryOut, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            RobustPolicy const& robust_policy,
            GeometryOut& geometry_out,
            Strategy const& strategy)
    {
        typedef typename geometry::detail::output_geometry_value
            <
                GeometryOut
            >::type SingleOut;

        intersection_insert
            <
                Geometry1, Geometry2, SingleOut,
                overlay_intersection
            >::apply(geometry1, geometry2, robust_policy,
                     geometry::detail::output_geometry_back_inserter(geometry_out),
                     strategy);

        return true;
    }

};


// If reversal is needed, perform it
template
<
    typename Geometry1, typename Geometry2,
    typename Tag1, typename Tag2
>
struct intersection
<
    Geometry1, Geometry2,
    Tag1, Tag2,
    true
>
    : intersection<Geometry2, Geometry1, Tag2, Tag1, false>
{
    template <typename RobustPolicy, typename GeometryOut, typename Strategy>
    static inline bool apply(
        Geometry1 const& g1,
        Geometry2 const& g2,
        RobustPolicy const& robust_policy,
        GeometryOut& out,
        Strategy const& strategy)
    {
        return intersection
            <
                Geometry2, Geometry1,
                Tag2, Tag1,
                false
            >::apply(g2, g1, robust_policy, out, strategy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_strategy {

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct intersection
{
    template
    <
        typename Geometry1,
        typename Geometry2,
        typename GeometryOut
    >
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             GeometryOut & geometry_out,
                             Strategy const& strategy)
    {
        typedef typename geometry::rescale_overlay_policy_type
            <
                Geometry1,
                Geometry2,
                typename Strategy::cs_tag
            >::type rescale_policy_type;
        
        rescale_policy_type robust_policy
            = geometry::get_rescale_policy<rescale_policy_type>(
                    geometry1, geometry2, strategy);

        return dispatch::intersection
            <
                Geometry1,
                Geometry2
            >::apply(geometry1, geometry2, robust_policy, geometry_out,
                     strategy);
    }
};

template <typename Strategy>
struct intersection<Strategy, false>
{
    template
    <
        typename Geometry1,
        typename Geometry2,
        typename GeometryOut
    >
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             GeometryOut & geometry_out,
                             Strategy const& strategy)
    {
        using strategies::relate::services::strategy_converter;
        return intersection
            <
                decltype(strategy_converter<Strategy>::get(strategy))
            >::apply(geometry1, geometry2, geometry_out,
                     strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct intersection<default_strategy, false>
{
    template
    <
        typename Geometry1,
        typename Geometry2,
        typename GeometryOut
    >
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             GeometryOut & geometry_out,
                             default_strategy)
    {
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry1, Geometry2
            >::type strategy_type;

        return intersection
            <
                strategy_type
            >::apply(geometry1, geometry2, geometry_out, strategy_type());
    }
};

} // resolve_strategy


namespace resolve_variant
{
    
template <typename Geometry1, typename Geometry2>
struct intersection
{
    template <typename GeometryOut, typename Strategy>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             GeometryOut& geometry_out,
                             Strategy const& strategy)
    {
        concepts::check<Geometry1 const>();
        concepts::check<Geometry2 const>();
        
        return resolve_strategy::intersection
            <
                Strategy
            >::apply(geometry1, geometry2, geometry_out, strategy);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct intersection<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    template <typename GeometryOut, typename Strategy>
    struct visitor: static_visitor<bool>
    {
        Geometry2 const& m_geometry2;
        GeometryOut& m_geometry_out;
        Strategy const& m_strategy;
        
        visitor(Geometry2 const& geometry2,
                GeometryOut& geometry_out,
                Strategy const& strategy)
            : m_geometry2(geometry2)
            , m_geometry_out(geometry_out)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry1>
        bool operator()(Geometry1 const& geometry1) const
        {
            return intersection
                <
                    Geometry1,
                    Geometry2
                >::apply(geometry1, m_geometry2, m_geometry_out, m_strategy);
        }
    };
    
    template <typename GeometryOut, typename Strategy>
    static inline bool
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
          Geometry2 const& geometry2,
          GeometryOut& geometry_out,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<GeometryOut, Strategy>(geometry2,
                                                                   geometry_out,
                                                                   strategy),
                                    geometry1);
    }
};


template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct intersection<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename GeometryOut, typename Strategy>
    struct visitor: static_visitor<bool>
    {
        Geometry1 const& m_geometry1;
        GeometryOut& m_geometry_out;
        Strategy const& m_strategy;
        
        visitor(Geometry1 const& geometry1,
                GeometryOut& geometry_out,
                Strategy const& strategy)
            : m_geometry1(geometry1)
            , m_geometry_out(geometry_out)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry2>
        bool operator()(Geometry2 const& geometry2) const
        {
            return intersection
                <
                    Geometry1,
                    Geometry2
                >::apply(m_geometry1, geometry2, m_geometry_out, m_strategy);
        }
    };
    
    template <typename GeometryOut, typename Strategy>
    static inline bool
    apply(Geometry1 const& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2,
          GeometryOut& geometry_out,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<GeometryOut, Strategy>(geometry1,
                                                                   geometry_out,
                                                                   strategy),
                                    geometry2);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T1), BOOST_VARIANT_ENUM_PARAMS(typename T2)>
struct intersection<variant<BOOST_VARIANT_ENUM_PARAMS(T1)>, variant<BOOST_VARIANT_ENUM_PARAMS(T2)> >
{
    template <typename GeometryOut, typename Strategy>
    struct visitor: static_visitor<bool>
    {
        GeometryOut& m_geometry_out;
        Strategy const& m_strategy;
        
        visitor(GeometryOut& geometry_out, Strategy const& strategy)
            : m_geometry_out(geometry_out)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry1, typename Geometry2>
        bool operator()(Geometry1 const& geometry1,
                        Geometry2 const& geometry2) const
        {
            return intersection
                <
                    Geometry1,
                    Geometry2
                >::apply(geometry1, geometry2, m_geometry_out, m_strategy);
        }
    };
    
    template <typename GeometryOut, typename Strategy>
    static inline bool
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T1)> const& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2,
          GeometryOut& geometry_out,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<GeometryOut, Strategy>(geometry_out,
                                                                   strategy),
                                    geometry1, geometry2);
    }
};
    
} // namespace resolve_variant
    

/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection, spatial set theoretic intersection}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam GeometryOut Collection of geometries (e.g. std::vector, std::deque, boost::geometry::multi*) of which
    the value_type fulfills a \p_l_or_c concept, or it is the output geometry (e.g. for a box)
\tparam Strategy \tparam_strategy{Intersection}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry_out The output geometry, either a multi_point, multi_polygon,
    multi_linestring, or a box (for intersection of two boxes)
\param strategy \param_strategy{intersection}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename Geometry1,
    typename Geometry2,
    typename GeometryOut,
    typename Strategy
>
inline bool intersection(Geometry1 const& geometry1,
                         Geometry2 const& geometry2,
                         GeometryOut& geometry_out,
                         Strategy const& strategy)
{
    return resolve_variant::intersection
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, geometry_out, strategy);
}


/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection, spatial set theoretic intersection}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam GeometryOut Collection of geometries (e.g. std::vector, std::deque, boost::geometry::multi*) of which
    the value_type fulfills a \p_l_or_c concept, or it is the output geometry (e.g. for a box)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry_out The output geometry, either a multi_point, multi_polygon,
    multi_linestring, or a box (for intersection of two boxes)

\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename Geometry1,
    typename Geometry2,
    typename GeometryOut
>
inline bool intersection(Geometry1 const& geometry1,
                         Geometry2 const& geometry2,
                         GeometryOut& geometry_out)
{
    return resolve_variant::intersection
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, geometry_out, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_INTERFACE_HPP

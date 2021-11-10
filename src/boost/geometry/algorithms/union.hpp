// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_UNION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_UNION_HPP


#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/intersection/multi.hpp>
#include <boost/geometry/algorithms/detail/overlay/intersection_insert.hpp>
#include <boost/geometry/algorithms/detail/overlay/linear_linear.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_pointlike.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/cartesian.hpp>
#include <boost/geometry/strategies/relate/geographic.hpp>
#include <boost/geometry/strategies/relate/spherical.hpp>
#include <boost/geometry/util/range.hpp>



namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1 = typename tag<Geometry1>::type,
    typename TagIn2 = typename tag<Geometry2>::type,
    typename TagOut = typename detail::setop_insert_output_tag<GeometryOut>::type,
    typename CastedTagIn1 = typename geometry::tag_cast<TagIn1, areal_tag, linear_tag, pointlike_tag>::type,
    typename CastedTagIn2 = typename geometry::tag_cast<TagIn2, areal_tag, linear_tag, pointlike_tag>::type,
    typename CastedTagOut = typename geometry::tag_cast<TagOut, areal_tag, linear_tag, pointlike_tag>::type,
    bool Reverse = geometry::reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct union_insert: not_implemented<TagIn1, TagIn2, TagOut>
{};


// If reversal is needed, perform it first

template
<
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1, typename TagIn2, typename TagOut,
    typename CastedTagIn1, typename CastedTagIn2, typename CastedTagOut
>
struct union_insert
    <
        Geometry1, Geometry2, GeometryOut,
        TagIn1, TagIn2, TagOut,
        CastedTagIn1, CastedTagIn2, CastedTagOut,
        true
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                                       Geometry2 const& g2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return union_insert
            <
                Geometry2, Geometry1, GeometryOut
            >::apply(g2, g1, robust_policy, out, strategy);
    }
};


template
<
    typename Geometry1, typename Geometry2, typename GeometryOut,
    typename TagIn1, typename TagIn2, typename TagOut
>
struct union_insert
    <
        Geometry1, Geometry2, GeometryOut,
        TagIn1, TagIn2, TagOut,
        areal_tag, areal_tag, areal_tag,
        false
    > : detail::overlay::overlay
        <
            Geometry1, Geometry2,
            detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
            detail::overlay::do_reverse<geometry::point_order<Geometry2>::value>::value,
            detail::overlay::do_reverse<geometry::point_order<GeometryOut>::value>::value,
            GeometryOut,
            overlay_union
        >
{};


// dispatch for union of linear geometries
template
<
    typename Linear1, typename Linear2, typename LineStringOut,
    typename TagIn1, typename TagIn2
>
struct union_insert
    <
        Linear1, Linear2, LineStringOut,
        TagIn1, TagIn2, linestring_tag,
        linear_tag, linear_tag, linear_tag,
        false
    > : detail::overlay::linear_linear_linestring
        <
            Linear1, Linear2, LineStringOut, overlay_union
        >
{};


// dispatch for point-like geometries
template
<
    typename PointLike1, typename PointLike2, typename PointOut,
    typename TagIn1, typename TagIn2
>
struct union_insert
    <
        PointLike1, PointLike2, PointOut,
        TagIn1, TagIn2, point_tag,
        pointlike_tag, pointlike_tag, pointlike_tag,
        false
    > : detail::overlay::union_pointlike_pointlike_point
        <
            PointLike1, PointLike2, PointOut
        >
{};


template
<
    typename Geometry1, typename Geometry2, typename SingleTupledOut,
    typename TagIn1, typename TagIn2,
    typename CastedTagIn
>
struct union_insert
    <
        Geometry1, Geometry2, SingleTupledOut,
        TagIn1, TagIn2, detail::tupled_output_tag,
        CastedTagIn, CastedTagIn, detail::tupled_output_tag,
        false
    >
{
    typedef typename geometry::detail::single_tag_from_base_tag
        <
            CastedTagIn
        >::type single_tag;

    typedef detail::expect_output
        <
            Geometry1, Geometry2, SingleTupledOut, single_tag
        > expect_check;

    typedef typename geometry::detail::output_geometry_access
        <
            SingleTupledOut, single_tag, single_tag
        > access;

    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                                       Geometry2 const& g2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        access::get(out) = union_insert
            <
                Geometry2, Geometry1, typename access::type
            >::apply(g2, g1, robust_policy, access::get(out), strategy);

        return out;
    }
};


template
<
    typename Geometry1, typename Geometry2, typename SingleTupledOut,
    typename SingleTag1, typename SingleTag2,
    bool Geometry1LesserTopoDim = (topological_dimension<Geometry1>::value
                                    < topological_dimension<Geometry2>::value)
>
struct union_insert_tupled_different
{
    typedef typename geometry::detail::output_geometry_access
        <
            SingleTupledOut, SingleTag1, SingleTag1
        > access1;

    typedef typename geometry::detail::output_geometry_access
        <
            SingleTupledOut, SingleTag2, SingleTag2
        > access2;

    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                                       Geometry2 const& g2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        access1::get(out) = geometry::dispatch::intersection_insert
            <
                Geometry1, Geometry2,
                typename access1::type,
                overlay_difference,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry2>::value, true>::value
            >::apply(g1, g2, robust_policy, access1::get(out), strategy);

        access2::get(out) = geometry::detail::convert_to_output
            <
                Geometry2,
                typename access2::type
            >::apply(g2, access2::get(out));

        return out;
    }
};


template
<
    typename Geometry1, typename Geometry2, typename SingleTupledOut,
    typename SingleTag1, typename SingleTag2
>
struct union_insert_tupled_different
    <
        Geometry1, Geometry2, SingleTupledOut, SingleTag1, SingleTag2, false
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                                       Geometry2 const& g2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return union_insert_tupled_different
            <
                Geometry2, Geometry1, SingleTupledOut, SingleTag2, SingleTag1, true
            >::apply(g2, g1, robust_policy, out, strategy);
    }
};


template
<
    typename Geometry1, typename Geometry2, typename SingleTupledOut,
    typename TagIn1, typename TagIn2,
    typename CastedTagIn1, typename CastedTagIn2
>
struct union_insert
    <
        Geometry1, Geometry2, SingleTupledOut,
        TagIn1, TagIn2, detail::tupled_output_tag,
        CastedTagIn1, CastedTagIn2, detail::tupled_output_tag,
        false
    >
{
    typedef typename geometry::detail::single_tag_from_base_tag
        <
            CastedTagIn1
        >::type single_tag1;

    typedef detail::expect_output
        <
            Geometry1, Geometry2, SingleTupledOut, single_tag1
        > expect_check1;

    typedef typename geometry::detail::single_tag_from_base_tag
        <
            CastedTagIn2
        >::type single_tag2;

    typedef detail::expect_output
        <
            Geometry1, Geometry2, SingleTupledOut, single_tag2
        > expect_check2;

    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                                       Geometry2 const& g2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return union_insert_tupled_different
            <
                Geometry1, Geometry2, SingleTupledOut, single_tag1, single_tag2
            >::apply(g1, g2, robust_policy, out, strategy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace union_
{

/*!
\brief_calc2{union}
\ingroup union
\details \details_calc2{union_insert, spatial set theoretic union}.
    \details_insert{union}
\tparam GeometryOut output geometry type, must be specified
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam OutputIterator output iterator
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{union}
\return \return_out
*/
template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename OutputIterator
>
inline OutputIterator union_insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();
    geometry::detail::output_geometry_concept_check<GeometryOut>::apply();

    typename strategies::relate::services::default_strategy
        <
            Geometry1, Geometry2
        >::type strategy;

    typedef typename geometry::rescale_overlay_policy_type
        <
            Geometry1,
            Geometry2
        >::type rescale_policy_type;

    rescale_policy_type robust_policy
            = geometry::get_rescale_policy<rescale_policy_type>(
                geometry1, geometry2, strategy);

    return dispatch::union_insert
           <
               Geometry1, Geometry2, GeometryOut
           >::apply(geometry1, geometry2, robust_policy, out, strategy);
}


}} // namespace detail::union_
#endif // DOXYGEN_NO_DETAIL


namespace resolve_strategy {

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct union_
{
    template <typename Geometry1, typename Geometry2, typename Collection>
    static inline void apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Collection & output_collection,
                             Strategy const& strategy)
    {
        typedef typename geometry::detail::output_geometry_value
            <
                Collection
            >::type single_out;

        typedef typename geometry::rescale_overlay_policy_type
            <
                Geometry1,
                Geometry2,
                typename Strategy::cs_tag
            >::type rescale_policy_type;

        rescale_policy_type robust_policy
                = geometry::get_rescale_policy<rescale_policy_type>(
                    geometry1, geometry2, strategy);

        dispatch::union_insert
           <
               Geometry1, Geometry2, single_out
           >::apply(geometry1, geometry2, robust_policy,
                    geometry::detail::output_geometry_back_inserter(output_collection),
                    strategy);
    }
};

template <typename Strategy>
struct union_<Strategy, false>
{
    template <typename Geometry1, typename Geometry2, typename Collection>
    static inline void apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Collection & output_collection,
                             Strategy const& strategy)
    {
        using strategies::relate::services::strategy_converter;

        union_
            <
                decltype(strategy_converter<Strategy>::get(strategy))
            >::apply(geometry1, geometry2, output_collection,
                     strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct union_<default_strategy, false>
{
    template <typename Geometry1, typename Geometry2, typename Collection>
    static inline void apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Collection & output_collection,
                             default_strategy)
    {
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry1,
                Geometry2
            >::type strategy_type;

        union_
            <
                strategy_type
            >::apply(geometry1, geometry2, output_collection, strategy_type());
    }
};

} // resolve_strategy


namespace resolve_variant
{
    
template <typename Geometry1, typename Geometry2>
struct union_
{
    template <typename Collection, typename Strategy>
    static inline void apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Collection& output_collection,
                             Strategy const& strategy)
    {
        concepts::check<Geometry1 const>();
        concepts::check<Geometry2 const>();
        //concepts::check<typename boost::range_value<Collection>::type>();
        geometry::detail::output_geometry_concept_check
            <
                typename geometry::detail::output_geometry_value
                    <
                        Collection
                    >::type
            >::apply();

        resolve_strategy::union_
            <
                Strategy
            >::apply(geometry1, geometry2, output_collection, strategy);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct union_<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    template <typename Collection, typename Strategy>
    struct visitor: static_visitor<>
    {
        Geometry2 const& m_geometry2;
        Collection& m_output_collection;
        Strategy const& m_strategy;
        
        visitor(Geometry2 const& geometry2,
                Collection& output_collection,
                Strategy const& strategy)
            : m_geometry2(geometry2)
            , m_output_collection(output_collection)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry1>
        void operator()(Geometry1 const& geometry1) const
        {
            union_
                <
                    Geometry1,
                    Geometry2
                >::apply(geometry1, m_geometry2, m_output_collection, m_strategy);
        }
    };
    
    template <typename Collection, typename Strategy>
    static inline void
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
          Geometry2 const& geometry2,
          Collection& output_collection,
          Strategy const& strategy)
    {
        boost::apply_visitor(visitor<Collection, Strategy>(geometry2,
                                                           output_collection,
                                                           strategy),
                             geometry1);
    }
};


template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct union_<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Collection, typename Strategy>
    struct visitor: static_visitor<>
    {
        Geometry1 const& m_geometry1;
        Collection& m_output_collection;
        Strategy const& m_strategy;
        
        visitor(Geometry1 const& geometry1,
                Collection& output_collection,
                Strategy const& strategy)
            : m_geometry1(geometry1)
            , m_output_collection(output_collection)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry2>
        void operator()(Geometry2 const& geometry2) const
        {
            union_
                <
                    Geometry1,
                    Geometry2
                >::apply(m_geometry1, geometry2, m_output_collection, m_strategy);
        }
    };
    
    template <typename Collection, typename Strategy>
    static inline void
    apply(Geometry1 const& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2,
          Collection& output_collection,
          Strategy const& strategy)
    {
        boost::apply_visitor(visitor<Collection, Strategy>(geometry1,
                                                           output_collection,
                                                           strategy),
                             geometry2);
    }
};


template <BOOST_VARIANT_ENUM_PARAMS(typename T1), BOOST_VARIANT_ENUM_PARAMS(typename T2)>
struct union_<variant<BOOST_VARIANT_ENUM_PARAMS(T1)>, variant<BOOST_VARIANT_ENUM_PARAMS(T2)> >
{
    template <typename Collection, typename Strategy>
    struct visitor: static_visitor<>
    {
        Collection& m_output_collection;
        Strategy const& m_strategy;
        
        visitor(Collection& output_collection, Strategy const& strategy)
            : m_output_collection(output_collection)
            , m_strategy(strategy)
        {}
        
        template <typename Geometry1, typename Geometry2>
        void operator()(Geometry1 const& geometry1,
                        Geometry2 const& geometry2) const
        {
            union_
                <
                    Geometry1,
                    Geometry2
                >::apply(geometry1, geometry2, m_output_collection, m_strategy);
        }
    };
    
    template <typename Collection, typename Strategy>
    static inline void
    apply(variant<BOOST_VARIANT_ENUM_PARAMS(T1)> const& geometry1,
          variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2,
          Collection& output_collection,
          Strategy const& strategy)
    {
        boost::apply_visitor(visitor<Collection, Strategy>(output_collection,
                                                           strategy),
                             geometry1, geometry2);
    }
};
    
} // namespace resolve_variant


/*!
\brief Combines two geometries which each other
\ingroup union
\details \details_calc2{union, spatial set theoretic union}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Collection output collection, either a multi-geometry,
    or a std::vector<Geometry> / std::deque<Geometry> etc
\tparam Strategy \tparam_strategy{Union_}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param output_collection the output collection
\param strategy \param_strategy{union_}
\note Called union_ because union is a reserved word.

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/union.qbk]}
*/
template
<
    typename Geometry1,
    typename Geometry2,
    typename Collection,
    typename Strategy
>
inline void union_(Geometry1 const& geometry1,
                   Geometry2 const& geometry2,
                   Collection& output_collection,
                   Strategy const& strategy)
{
    resolve_variant::union_
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, output_collection, strategy);
}


/*!
\brief Combines two geometries which each other
\ingroup union
\details \details_calc2{union, spatial set theoretic union}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Collection output collection, either a multi-geometry,
    or a std::vector<Geometry> / std::deque<Geometry> etc
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param output_collection the output collection
\note Called union_ because union is a reserved word.

\qbk{[include reference/algorithms/union.qbk]}
*/
template
<
    typename Geometry1,
    typename Geometry2,
    typename Collection
>
inline void union_(Geometry1 const& geometry1,
                   Geometry2 const& geometry2,
                   Collection& output_collection)
{
    resolve_variant::union_
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, output_collection, default_strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_UNION_HPP

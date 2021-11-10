// Boost.Geometry Index
//
// Spatial query predicates definition and checks.
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2021.
// Modifications copyright (c) 2019-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_PREDICATES_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_PREDICATES_HPP

#include <tuple>
#include <type_traits>
//#include <utility>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/tags.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

namespace predicates {

// ------------------------------------------------------------------ //
// predicates
// ------------------------------------------------------------------ //

template <typename Fun, bool IsFunction>
struct satisfies_impl
{
    satisfies_impl() : fun(NULL) {}
    satisfies_impl(Fun f) : fun(f) {}
    Fun * fun;
};

template <typename Fun>
struct satisfies_impl<Fun, false>
{
    satisfies_impl() {}
    satisfies_impl(Fun const& f) : fun(f) {}
    Fun fun;
};

template <typename Fun, bool Negated>
struct satisfies
    : satisfies_impl<Fun, ::boost::is_function<Fun>::value>
{
    typedef satisfies_impl<Fun, ::boost::is_function<Fun>::value> base;

    satisfies() {}
    satisfies(Fun const& f) : base(f) {}
    satisfies(base const& b) : base(b) {}
};

// ------------------------------------------------------------------ //

struct contains_tag {};
struct covered_by_tag {};
struct covers_tag {};
struct disjoint_tag {};
struct intersects_tag {};
struct overlaps_tag {};
struct touches_tag {};
struct within_tag {};

template <typename Geometry, typename Tag, bool Negated>
struct spatial_predicate
{
    spatial_predicate() {}
    spatial_predicate(Geometry const& g) : geometry(g) {}
    Geometry geometry;
};

// ------------------------------------------------------------------ //

// CONSIDER: separated nearest<> and path<> may be replaced by
//           nearest_predicate<Geometry, Tag>
//           where Tag = point_tag | path_tag
// IMPROVEMENT: user-defined nearest predicate allowing to define
//              all or only geometrical aspects of the search

template <typename PointOrRelation>
struct nearest
{
    nearest()
//        : count(0)
    {}
    nearest(PointOrRelation const& por, std::size_t k)
        : point_or_relation(por)
        , count(k)
    {}
    PointOrRelation point_or_relation;
    std::size_t count;
};

template <typename SegmentOrLinestring>
struct path
{
    path()
//        : count(0)
    {}
    path(SegmentOrLinestring const& g, std::size_t k)
        : geometry(g)
        , count(k)
    {}
    SegmentOrLinestring geometry;
    std::size_t count;
};

} // namespace predicates

// ------------------------------------------------------------------ //
// predicate_check
// ------------------------------------------------------------------ //

template <typename Predicate, typename Tag>
struct predicate_check
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Predicate or Tag.",
        Predicate, Tag);
};

// ------------------------------------------------------------------ //

template <typename Fun>
struct predicate_check<predicates::satisfies<Fun, false>, value_tag>
{
    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(predicates::satisfies<Fun, false> const& p, Value const& v, Indexable const& , Strategy const&)
    {
        return p.fun(v);
    }
};

template <typename Fun>
struct predicate_check<predicates::satisfies<Fun, true>, value_tag>
{
    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(predicates::satisfies<Fun, true> const& p, Value const& v, Indexable const& , Strategy const&)
    {
        return !p.fun(v);
    }
};

// ------------------------------------------------------------------ //

template <typename Tag>
struct spatial_predicate_call
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag.",
        Tag);
};

template <>
struct spatial_predicate_call<predicates::contains_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::within(g2, g1);
    }
};

template <>
struct spatial_predicate_call<predicates::covered_by_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::covered_by(g1, g2);
    }
};

template <>
struct spatial_predicate_call<predicates::covers_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::covered_by(g2, g1);
    }
};

template <>
struct spatial_predicate_call<predicates::disjoint_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::disjoint(g1, g2);
    }
};

// TEMP: used to implement CS-specific intersects predicate for certain
// combinations of geometries until umbrella strategies are implemented
template
<
    typename G1, typename G2,
    typename Tag1 = typename tag<G1>::type,
    typename Tag2 = typename tag<G2>::type
>
struct spatial_predicate_intersects
{
    template <typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::intersects(g1, g2);
    }
};
// TEMP: used in within and relate
template <typename G1, typename G2>
struct spatial_predicate_intersects<G1, G2, box_tag, point_tag>
{
    static inline bool apply(G1 const& g1, G2 const& g2, default_strategy const&)
    {
        return geometry::intersects(g1, g2);
    }

    template <typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const& s)
    {
        return geometry::intersects(g1, g2, s);
    }
};

template <>
struct spatial_predicate_call<predicates::intersects_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const& s)
    {
        return spatial_predicate_intersects<G1, G2>::apply(g1, g2, s);
    }
};

template <>
struct spatial_predicate_call<predicates::overlaps_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::overlaps(g1, g2);
    }
};

template <>
struct spatial_predicate_call<predicates::touches_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::touches(g1, g2);
    }
};

template <>
struct spatial_predicate_call<predicates::within_tag>
{
    template <typename G1, typename G2, typename S>
    static inline bool apply(G1 const& g1, G2 const& g2, S const&)
    {
        return geometry::within(g1, g2);
    }
};

// ------------------------------------------------------------------ //

// spatial predicate
template <typename Geometry, typename Tag>
struct predicate_check<predicates::spatial_predicate<Geometry, Tag, false>, value_tag>
{
    typedef predicates::spatial_predicate<Geometry, Tag, false> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return spatial_predicate_call<Tag>::apply(i, p.geometry, s);
    }
};

// negated spatial predicate
template <typename Geometry, typename Tag>
struct predicate_check<predicates::spatial_predicate<Geometry, Tag, true>, value_tag>
{
    typedef predicates::spatial_predicate<Geometry, Tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return !spatial_predicate_call<Tag>::apply(i, p.geometry, s);
    }
};

// ------------------------------------------------------------------ //

template <typename DistancePredicates>
struct predicate_check<predicates::nearest<DistancePredicates>, value_tag>
{
    template <typename Value, typename Box, typename Strategy>
    static inline bool apply(predicates::nearest<DistancePredicates> const&, Value const&, Box const&, Strategy const&)
    {
        return true;
    }
};

template <typename Linestring>
struct predicate_check<predicates::path<Linestring>, value_tag>
{
    template <typename Value, typename Box, typename Strategy>
    static inline bool apply(predicates::path<Linestring> const&, Value const&, Box const&, Strategy const&)
    {
        return true;
    }
};

// ------------------------------------------------------------------ //
// predicates_check for bounds
// ------------------------------------------------------------------ //

template <typename Fun, bool Negated>
struct predicate_check<predicates::satisfies<Fun, Negated>, bounds_tag>
{
    template <typename Value, typename Box, typename Strategy>
    static bool apply(predicates::satisfies<Fun, Negated> const&, Value const&, Box const&, Strategy const&)
    {
        return true;
    }
};

// ------------------------------------------------------------------ //

// NOT NEGATED
// value_tag        bounds_tag
// ---------------------------
// contains(I,G)    covers(I,G)
// covered_by(I,G)  intersects(I,G)
// covers(I,G)      covers(I,G)
// disjoint(I,G)    !covered_by(I,G)
// intersects(I,G)  intersects(I,G)
// overlaps(I,G)    intersects(I,G)  - possibly change to the version without border case, e.g. intersects_without_border(0,0x1,1, 1,1x2,2) should give false
// touches(I,G)     intersects(I,G)
// within(I,G)      intersects(I,G)  - possibly change to the version without border case, e.g. intersects_without_border(0,0x1,1, 1,1x2,2) should give false

// spatial predicate - default
template <typename Geometry, typename Tag>
struct predicate_check<predicates::spatial_predicate<Geometry, Tag, false>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, Tag, false> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return spatial_predicate_call<predicates::intersects_tag>::apply(i, p.geometry, s);
    }
};

// spatial predicate - contains
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::contains_tag, false>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::contains_tag, false> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return spatial_predicate_call<predicates::covers_tag>::apply(i, p.geometry, s);
    }
};

// spatial predicate - covers
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::covers_tag, false>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::covers_tag, false> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return spatial_predicate_call<predicates::covers_tag>::apply(i, p.geometry, s);
    }
};

// spatial predicate - disjoint
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::disjoint_tag, false>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::disjoint_tag, false> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return !spatial_predicate_call<predicates::covered_by_tag>::apply(i, p.geometry, s);
    }
};

// NEGATED
// value_tag        bounds_tag
// ---------------------------
// !contains(I,G)   TRUE
// !covered_by(I,G) !covered_by(I,G)
// !covers(I,G)     TRUE
// !disjoint(I,G)   !disjoint(I,G)
// !intersects(I,G) !covered_by(I,G)
// !overlaps(I,G)   TRUE
// !touches(I,G)    !intersects(I,G)
// !within(I,G)     !within(I,G)

// negated spatial predicate - default
template <typename Geometry, typename Tag>
struct predicate_check<predicates::spatial_predicate<Geometry, Tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, Tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return !spatial_predicate_call<Tag>::apply(i, p.geometry, s);
    }
};

// negated spatial predicate - contains
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::contains_tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::contains_tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& , Value const&, Indexable const&, Strategy const&)
    {
        return true;
    }
};

// negated spatial predicate - covers
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::covers_tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::covers_tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& , Value const&, Indexable const&, Strategy const&)
    {
        return true;
    }
};

// negated spatial predicate - intersects
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::intersects_tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::intersects_tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const& s)
    {
        return !spatial_predicate_call<predicates::covered_by_tag>::apply(i, p.geometry, s);
    }
};

// negated spatial predicate - overlaps
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::overlaps_tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::overlaps_tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& , Value const&, Indexable const&, Strategy const&)
    {
        return true;
    }
};

// negated spatial predicate - touches
template <typename Geometry>
struct predicate_check<predicates::spatial_predicate<Geometry, predicates::touches_tag, true>, bounds_tag>
{
    typedef predicates::spatial_predicate<Geometry, predicates::touches_tag, true> Pred;

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Pred const& p, Value const&, Indexable const& i, Strategy const&)
    {
        return !spatial_predicate_call<predicates::intersects_tag>::apply(i, p.geometry);
    }
};

// ------------------------------------------------------------------ //

template <typename DistancePredicates>
struct predicate_check<predicates::nearest<DistancePredicates>, bounds_tag>
{
    template <typename Value, typename Box, typename Strategy>
    static inline bool apply(predicates::nearest<DistancePredicates> const&, Value const&, Box const&, Strategy const&)
    {
        return true;
    }
};

template <typename Linestring>
struct predicate_check<predicates::path<Linestring>, bounds_tag>
{
    template <typename Value, typename Box, typename Strategy>
    static inline bool apply(predicates::path<Linestring> const&, Value const&, Box const&, Strategy const&)
    {
        return true;
    }
};

// ------------------------------------------------------------------ //
// predicates_length
// ------------------------------------------------------------------ //

template <typename T>
struct predicates_length
{
    static const std::size_t value = 1;
};

template <typename ...Ts>
struct predicates_length<std::tuple<Ts...>>
{
    static const std::size_t value = std::tuple_size<std::tuple<Ts...>>::value;
};

// ------------------------------------------------------------------ //
// predicates_element
// ------------------------------------------------------------------ //

template <std::size_t I, typename T>
struct predicates_element
{
    BOOST_GEOMETRY_STATIC_ASSERT((I < 1),
        "Invalid I index.",
        std::integral_constant<std::size_t, I>);

    typedef T type;
    static type const& get(T const& p) { return p; }
};

template <std::size_t I, typename ...Ts>
struct predicates_element<I, std::tuple<Ts...>>
{
    typedef std::tuple<Ts...> predicate_type;

    typedef typename std::tuple_element<I, predicate_type>::type type;
    static type const& get(predicate_type const& p) { return std::get<I>(p); }
};

// ------------------------------------------------------------------ //
// predicates_check
// ------------------------------------------------------------------ //

template <typename TuplePredicates, typename Tag, std::size_t First, std::size_t Last>
struct predicates_check_tuple
{
    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(TuplePredicates const& p, Value const& v, Indexable const& i, Strategy const& s)
    {
        return predicate_check
                <
                    typename std::tuple_element<First, TuplePredicates>::type,
                    Tag
                >::apply(std::get<First>(p), v, i, s)
            && predicates_check_tuple<TuplePredicates, Tag, First+1, Last>::apply(p, v, i, s);
    }
};

template <typename TuplePredicates, typename Tag, std::size_t First>
struct predicates_check_tuple<TuplePredicates, Tag, First, First>
{
    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(TuplePredicates const& , Value const& , Indexable const& , Strategy const& )
    {
        return true;
    }
};

template <typename Predicate, typename Tag, std::size_t First, std::size_t Last>
struct predicates_check_impl
{
    static const bool check = First < 1 && Last <= 1 && First <= Last;
    BOOST_GEOMETRY_STATIC_ASSERT((check),
        "Invalid First or Last index.",
        std::integer_sequence<std::size_t, First, Last>);

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(Predicate const& p, Value const& v, Indexable const& i, Strategy const& s)
    {
        return predicate_check<Predicate, Tag>::apply(p, v, i, s);
    }
};

template <typename ...Ts, typename Tag, std::size_t First, std::size_t Last>
struct predicates_check_impl<std::tuple<Ts...>, Tag, First, Last>
{
    typedef std::tuple<Ts...> predicates_type;

    static const std::size_t pred_len = std::tuple_size<predicates_type>::value;
    static const bool check = First < pred_len && Last <= pred_len && First <= Last;
    BOOST_GEOMETRY_STATIC_ASSERT((check),
        "Invalid First or Last index.",
        std::integer_sequence<std::size_t, First, Last>);

    template <typename Value, typename Indexable, typename Strategy>
    static inline bool apply(predicates_type const& p, Value const& v, Indexable const& i, Strategy const& s)
    {
        return predicates_check_tuple
                <
                    predicates_type,
                    Tag, First, Last
                >::apply(p, v, i, s);
    }
};

template <typename Tag, std::size_t First, std::size_t Last, typename Predicates, typename Value, typename Indexable, typename Strategy>
inline bool predicates_check(Predicates const& p, Value const& v, Indexable const& i, Strategy const& s)
{
    return detail::predicates_check_impl<Predicates, Tag, First, Last>
        ::apply(p, v, i, s);
}

// ------------------------------------------------------------------ //
// nearest predicate helpers
// ------------------------------------------------------------------ //

// predicates_is_nearest

template <typename P>
struct predicates_is_distance
{
    static const std::size_t value = 0;
};

template <typename DistancePredicates>
struct predicates_is_distance< predicates::nearest<DistancePredicates> >
{
    static const std::size_t value = 1;
};

template <typename Linestring>
struct predicates_is_distance< predicates::path<Linestring> >
{
    static const std::size_t value = 1;
};

// predicates_count_nearest

template <typename T>
struct predicates_count_distance
{
    static const std::size_t value = predicates_is_distance<T>::value;
};

template <typename Tuple, std::size_t N>
struct predicates_count_distance_tuple
{
    static const std::size_t value =
        predicates_is_distance<typename std::tuple_element<N-1, Tuple>::type>::value
        + predicates_count_distance_tuple<Tuple, N-1>::value;
};

template <typename Tuple>
struct predicates_count_distance_tuple<Tuple, 1>
{
    static const std::size_t value =
        predicates_is_distance<typename std::tuple_element<0, Tuple>::type>::value;
};

template <typename ...Ts>
struct predicates_count_distance<std::tuple<Ts...>>
{
    static const std::size_t value = predicates_count_distance_tuple<
        std::tuple<Ts...>,
        std::tuple_size<std::tuple<Ts...>>::value
    >::value;
};

// predicates_find_nearest

template <typename T>
struct predicates_find_distance
{
    static const std::size_t value = predicates_is_distance<T>::value ? 0 : 1;
};

template <typename Tuple, std::size_t N>
struct predicates_find_distance_tuple
{
    static const bool is_found = predicates_find_distance_tuple<Tuple, N-1>::is_found
                                || predicates_is_distance<typename std::tuple_element<N-1, Tuple>::type>::value;

    static const std::size_t value = predicates_find_distance_tuple<Tuple, N-1>::is_found ?
        predicates_find_distance_tuple<Tuple, N-1>::value :
        (predicates_is_distance<typename std::tuple_element<N-1, Tuple>::type>::value ?
            N-1 : std::tuple_size<Tuple>::value);
};

template <typename Tuple>
struct predicates_find_distance_tuple<Tuple, 1>
{
    static const bool is_found = predicates_is_distance<typename std::tuple_element<0, Tuple>::type>::value;
    static const std::size_t value = is_found ? 0 : std::tuple_size<Tuple>::value;
};

template <typename ...Ts>
struct predicates_find_distance<std::tuple<Ts...>>
{
    static const std::size_t value = predicates_find_distance_tuple<
        std::tuple<Ts...>,
        std::tuple_size<std::tuple<Ts...>>::value
    >::value;
};

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_PREDICATES_HPP

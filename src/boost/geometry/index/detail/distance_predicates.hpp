// Boost.Geometry Index
//
// Spatial index distance predicates, calculators and checkers
// used in nearest query - specialized for envelopes
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_DISTANCE_PREDICATES_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_DISTANCE_PREDICATES_HPP

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/algorithms/comparable_distance_near.hpp>
#include <boost/geometry/index/detail/algorithms/comparable_distance_far.hpp>
#include <boost/geometry/index/detail/algorithms/comparable_distance_centroid.hpp>
#include <boost/geometry/index/detail/algorithms/path_intersection.hpp>

#include <boost/geometry/index/detail/tags.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

// ------------------------------------------------------------------ //
// relations
// ------------------------------------------------------------------ //

template <typename T>
struct to_nearest
{
    to_nearest(T const& v) : value(v) {}
    T value;
};

template <typename T>
struct to_centroid
{
    to_centroid(T const& v) : value(v) {}
    T value;
};

template <typename T>
struct to_furthest
{
    to_furthest(T const& v) : value(v) {}
    T value;
};

// tags

struct to_nearest_tag {};
struct to_centroid_tag {};
struct to_furthest_tag {};

// ------------------------------------------------------------------ //
// relation traits and access
// ------------------------------------------------------------------ //

template <typename T>
struct relation
{
    typedef T value_type;
    typedef to_nearest_tag tag;
    static inline T const& value(T const& v) { return v; }
    static inline T & value(T & v) { return v; }
};

template <typename T>
struct relation< to_nearest<T> >
{
    typedef T value_type;
    typedef to_nearest_tag tag;
    static inline T const& value(to_nearest<T> const& r) { return r.value; }
    static inline T & value(to_nearest<T> & r) { return r.value; }
};

template <typename T>
struct relation< to_centroid<T> >
{
    typedef T value_type;
    typedef to_centroid_tag tag;
    static inline T const& value(to_centroid<T> const& r) { return r.value; }
    static inline T & value(to_centroid<T> & r) { return r.value; }
};

template <typename T>
struct relation< to_furthest<T> >
{
    typedef T value_type;
    typedef to_furthest_tag tag;
    static inline T const& value(to_furthest<T> const& r) { return r.value; }
    static inline T & value(to_furthest<T> & r) { return r.value; }
};

// ------------------------------------------------------------------ //

template
<
    typename G1, typename G2, typename Strategy
>
struct comparable_distance_call
{
    typedef typename geometry::comparable_distance_result
        <
            G1, G2, Strategy
        >::type result_type;

    static inline result_type apply(G1 const& g1, G2 const& g2, Strategy const& s)
    {
        return geometry::comparable_distance(g1, g2, s);
    }
};

template
<
    typename G1, typename G2
>
struct comparable_distance_call<G1, G2, default_strategy>
{
    typedef typename geometry::default_comparable_distance_result
        <
            G1, G2
        >::type result_type;

    static inline result_type apply(G1 const& g1, G2 const& g2, default_strategy const&)
    {
        return geometry::comparable_distance(g1, g2);
    }
};

// ------------------------------------------------------------------ //
// calculate_distance
// ------------------------------------------------------------------ //

template <typename Predicate, typename Indexable, typename Strategy, typename Tag>
struct calculate_distance
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Invalid Predicate or Tag.",
        Predicate, Indexable, Strategy, Tag);
};

// this handles nearest() with default Point parameter, to_nearest() and bounds
template <typename PointRelation, typename Indexable, typename Strategy, typename Tag>
struct calculate_distance< predicates::nearest<PointRelation>, Indexable, Strategy, Tag>
{
    typedef detail::relation<PointRelation> relation;
    typedef comparable_distance_call
        <
            typename relation::value_type,
            Indexable,
            Strategy
        > call_type;
    typedef typename call_type::result_type result_type;

    static inline bool apply(predicates::nearest<PointRelation> const& p, Indexable const& i,
                             Strategy const& s, result_type & result)
    {
        result = call_type::apply(relation::value(p.point_or_relation), i, s);
        return true;
    }
};

template <typename Point, typename Indexable, typename Strategy>
struct calculate_distance< predicates::nearest< to_centroid<Point> >, Indexable, Strategy, value_tag>
{
    typedef Point point_type;
    typedef typename geometry::default_comparable_distance_result
        <
            point_type, Indexable
        >::type result_type;

    static inline bool apply(predicates::nearest< to_centroid<Point> > const& p, Indexable const& i,
                             Strategy const& , result_type & result)
    {
        result = index::detail::comparable_distance_centroid(p.point_or_relation.value, i);
        return true;
    }
};

template <typename Point, typename Indexable, typename Strategy>
struct calculate_distance< predicates::nearest< to_furthest<Point> >, Indexable, Strategy, value_tag>
{
    typedef Point point_type;
    typedef typename geometry::default_comparable_distance_result
        <
            point_type, Indexable
        >::type result_type;

    static inline bool apply(predicates::nearest< to_furthest<Point> > const& p, Indexable const& i,
                             Strategy const& , result_type & result)
    {
        result = index::detail::comparable_distance_far(p.point_or_relation.value, i);
        return true;
    }
};

template <typename SegmentOrLinestring, typename Indexable, typename Strategy, typename Tag>
struct calculate_distance< predicates::path<SegmentOrLinestring>, Indexable, Strategy, Tag>
{
    typedef typename index::detail::default_path_intersection_distance_type<
        Indexable, SegmentOrLinestring
    >::type result_type;

    static inline bool apply(predicates::path<SegmentOrLinestring> const& p, Indexable const& i,
                             Strategy const& , result_type & result)
    {
        return index::detail::path_intersection(i, p.geometry, result);
    }
};

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_RTREE_DISTANCE_PREDICATES_HPP

// Boost.Geometry Index
//
// n-dimensional bounds
//
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP

#include <boost/geometry/index/detail/bounded_view.hpp>

namespace boost { namespace geometry { namespace index { namespace detail
{

namespace dispatch
{

template <typename Geometry,
          typename Bounds,
          typename TagGeometry = typename geometry::tag<Geometry>::type,
          typename TagBounds = typename geometry::tag<Bounds>::type>
struct bounds
{
    template <typename Strategy>
    static inline void apply(Geometry const& g, Bounds & b, Strategy const& )
    {
        geometry::convert(g, b);
    }
};

template <typename Geometry, typename Bounds>
struct bounds<Geometry, Bounds, segment_tag, box_tag>
{
    template <typename Strategy>
    static inline void apply(Geometry const& g, Bounds & b, Strategy const& s)
    {
        index::detail::bounded_view<Geometry, Bounds, Strategy> v(g, s);
        geometry::convert(v, b);
    }
};


} // namespace dispatch


template <typename Geometry, typename Bounds, typename Strategy>
inline void bounds(Geometry const& g, Bounds & b, Strategy const& s)
{
    concepts::check_concepts_and_equal_dimensions<Geometry const, Bounds>();
    dispatch::bounds<Geometry, Bounds>::apply(g, b, s);
}


namespace dispatch
{

template <typename Bounds,
          typename Geometry,
          typename TagBounds = typename geometry::tag<Bounds>::type,
          typename TagGeometry = typename geometry::tag<Geometry>::type>
struct expand
{
    // STATIC ASSERT
};

template <typename Bounds, typename Geometry>
struct expand<Bounds, Geometry, box_tag, point_tag>
{
    static inline void apply(Bounds & b, Geometry const& g)
    {
        geometry::expand(b, g);
    }

    template <typename Strategy>
    static inline void apply(Bounds & b, Geometry const& g, Strategy const& s)
    {
        geometry::expand(b, g, s);
    }
};

template <typename Bounds, typename Geometry>
struct expand<Bounds, Geometry, box_tag, box_tag>
{
    static inline void apply(Bounds & b, Geometry const& g)
    {
        geometry::expand(b, g);
    }

    template <typename Strategy>
    static inline void apply(Bounds & b, Geometry const& g, Strategy const& s)
    {
        geometry::expand(b, g, s);
    }
};

template <typename Bounds, typename Geometry>
struct expand<Bounds, Geometry, box_tag, segment_tag>
{
    static inline void apply(Bounds & b, Geometry const& g)
    {
        geometry::expand(b, g);
    }

    template <typename Strategy>
    static inline void apply(Bounds & b, Geometry const& g, Strategy const& s)
    {
        geometry::expand(b, geometry::return_envelope<Bounds>(g, s), s);
        // requires additional strategy
        //geometry::expand(b, g, s);
    }
};


} // namespace dispatch


template <typename Bounds, typename Geometry, typename Strategy>
inline void expand(Bounds & b, Geometry const& g, Strategy const& s)
{
    dispatch::expand<Bounds, Geometry>::apply(b, g, s);
}

template <typename Bounds, typename Geometry>
inline void expand(Bounds & b, Geometry const& g, default_strategy const& )
{
    dispatch::expand<Bounds, Geometry>::apply(b, g);
}


namespace dispatch
{


template <typename Geometry,
          typename Bounds,
          typename TagGeometry = typename geometry::tag<Geometry>::type,
          typename TagBounds = typename geometry::tag<Bounds>::type>
struct covered_by_bounds
{};

template <typename Geometry, typename Bounds>
struct covered_by_bounds<Geometry, Bounds, point_tag, box_tag>
{
    static inline bool apply(Geometry const& g, Bounds & b)
    {
        return geometry::covered_by(g, b);
    }

    template <typename Strategy>
    static inline bool apply(Geometry const& g, Bounds & b, Strategy const& s)
    {
        return geometry::covered_by(g, b, s);
    }
};

template <typename Geometry, typename Bounds>
struct covered_by_bounds<Geometry, Bounds, box_tag, box_tag>
{
    static inline bool apply(Geometry const& g, Bounds & b)
    {
        return geometry::covered_by(g, b);
    }

    template <typename Strategy>
    static inline bool apply(Geometry const& g, Bounds & b, Strategy const& s)
    {
        return geometry::covered_by(g, b, s);
    }
};

template <typename Geometry, typename Bounds>
struct covered_by_bounds<Geometry, Bounds, segment_tag, box_tag>
{
    static inline bool apply(Geometry const& g, Bounds & b)
    {
        typedef typename point_type<Geometry>::type point_type;
        typedef geometry::model::box<point_type> bounds_type;
        typedef index::detail::bounded_view<Geometry, bounds_type, default_strategy> view_type;

        return geometry::covered_by(view_type(g, default_strategy()), b);
    }

    template <typename Strategy>
    static inline bool apply(Geometry const& g, Bounds & b, Strategy const& strategy)
    {
        typedef typename point_type<Geometry>::type point_type;
        typedef geometry::model::box<point_type> bounds_type;
        typedef index::detail::bounded_view<Geometry, bounds_type, Strategy> view_type;

        return geometry::covered_by(view_type(g, strategy), b, strategy);
    }
};


} // namespace dispatch


template <typename Geometry, typename Bounds, typename Strategy>
inline bool covered_by_bounds(Geometry const& g, Bounds & b, Strategy const& s)
{
    return dispatch::covered_by_bounds<Geometry, Bounds>::apply(g, b, s);
}

template <typename Geometry, typename Bounds>
inline bool covered_by_bounds(Geometry const& g, Bounds & b, default_strategy const& )
{
    return dispatch::covered_by_bounds<Geometry, Bounds>::apply(g, b);
}


}}}} // namespace boost::geometry::index::detail


#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP

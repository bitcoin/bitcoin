// Boost.Geometry Index
//
// minmaxdist used in R-tree k nearest neighbors query
//
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MINMAXDIST_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MINMAXDIST_HPP

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/algorithms/diff_abs.hpp>
#include <boost/geometry/index/detail/algorithms/sum_for_indexable.hpp>
#include <boost/geometry/index/detail/algorithms/smallest_for_indexable.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

struct minmaxdist_tag {};

template <
    typename Point,
    typename BoxIndexable,
    size_t DimensionIndex>
struct smallest_for_indexable_dimension<Point, BoxIndexable, box_tag, minmaxdist_tag, DimensionIndex>
{
    typedef typename geometry::default_comparable_distance_result<Point, BoxIndexable>::type result_type;

    inline static result_type apply(Point const& pt, BoxIndexable const& i, result_type const& maxd)
    {
        typedef typename coordinate_type<Point>::type point_coord_t;
        typedef typename coordinate_type<BoxIndexable>::type indexable_coord_t;

        point_coord_t pt_c = geometry::get<DimensionIndex>(pt);
        indexable_coord_t ind_c_min = geometry::get<geometry::min_corner, DimensionIndex>(i);
        indexable_coord_t ind_c_max = geometry::get<geometry::max_corner, DimensionIndex>(i);

        indexable_coord_t ind_c_avg = ind_c_min + (ind_c_max - ind_c_min) / 2;
        // TODO: awulkiew - is (ind_c_min + ind_c_max) / 2 safe?

        // TODO: awulkiew - optimize! don't calculate 2x pt_c <= ind_c_avg
        // take particular case pt_c == ind_c_avg into account

        result_type closer_comp = 0;
        if ( pt_c <= ind_c_avg )
            closer_comp = detail::diff_abs(pt_c, ind_c_min); // unsigned values protection
        else
            closer_comp = ind_c_max - pt_c;
        
        result_type further_comp = 0;
        if ( ind_c_avg <= pt_c )
            further_comp = pt_c - ind_c_min;
        else
            further_comp = detail::diff_abs(pt_c, ind_c_max); // unsigned values protection

        return (maxd + closer_comp * closer_comp) - further_comp * further_comp;
    }
};

template <typename Point, typename Indexable, typename IndexableTag>
struct minmaxdist_impl
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Indexable type.",
        Point, Indexable, IndexableTag);
};

template <typename Point, typename Indexable>
struct minmaxdist_impl<Point, Indexable, point_tag>
{
    typedef typename geometry::default_comparable_distance_result<Point, Indexable>::type result_type;

    inline static result_type apply(Point const& pt, Indexable const& i)
    {
        return geometry::comparable_distance(pt, i);
    }
};

template <typename Point, typename Indexable>
struct minmaxdist_impl<Point, Indexable, box_tag>
{
    typedef typename geometry::default_comparable_distance_result<Point, Indexable>::type result_type;

    inline static result_type apply(Point const& pt, Indexable const& i)
    {
        result_type maxd = geometry::comparable_distance(pt, i);

        return smallest_for_indexable<
            Point,
            Indexable,
            box_tag,
            minmaxdist_tag,
            dimension<Indexable>::value
        >::apply(pt, i, maxd);
    }
};

/**
 * This is comparable distace.
 */
template <typename Point, typename Indexable>
typename geometry::default_comparable_distance_result<Point, Indexable>::type
minmaxdist(Point const& pt, Indexable const& i)
{
    return detail::minmaxdist_impl<
        Point,
        Indexable,
        typename tag<Indexable>::type
    >::apply(pt, i);
}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_MINMAXDIST_HPP

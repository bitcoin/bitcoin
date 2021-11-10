// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015-2018.
// Modifications copyright (c) 2015-2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>


namespace boost { namespace geometry { namespace strategy
{

namespace within
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct within_coord
{
    template <typename Value1, typename Value2>
    static inline bool apply(Value1 const& value, Value2 const& min_value, Value2 const& max_value)
    {
        return value > min_value && value < max_value;
    }
};

struct covered_by_coord
{
    template <typename Value1, typename Value2>
    static inline bool apply(Value1 const& value, Value2 const& min_value, Value2 const& max_value)
    {
        return value >= min_value && value <= max_value;
    }
};

template <typename Geometry, std::size_t Dimension, typename CSTag>
struct within_range
    : within_coord
{};


template <typename Geometry, std::size_t Dimension, typename CSTag>
struct covered_by_range
    : covered_by_coord
{};


// NOTE: the result would be the same if instead of structs defined below
// the above xxx_range were used with the following arguments:
// (min_value + diff_min, min_value, max_value)
struct within_longitude_diff
{
    template <typename CalcT>
    static inline bool apply(CalcT const& diff_min, CalcT const& min_value, CalcT const& max_value)
    {
        CalcT const c0 = 0;
        return diff_min > c0
            && (min_value + diff_min < max_value
             /*|| max_value - diff_min > min_value*/);
    }
};

struct covered_by_longitude_diff
{
    template <typename CalcT>
    static inline bool apply(CalcT const& diff_min, CalcT const& min_value, CalcT const& max_value)
    {
        return min_value + diff_min <= max_value
            /*|| max_value - diff_min >= min_value*/;
    }
};


template <typename Geometry,
          typename CoordCheck,
          typename DiffCheck>
struct longitude_range
{
    template <typename Value1, typename Value2>
    static inline bool apply(Value1 const& value, Value2 const& min_value, Value2 const& max_value)
    {
        typedef typename select_most_precise
            <
                Value1, Value2
            >::type calc_t;
        typedef typename geometry::detail::cs_angular_units<Geometry>::type units_t;
        typedef math::detail::constants_on_spheroid<calc_t, units_t> constants;

        if (CoordCheck::apply(value, min_value, max_value))
        {
            return true;
        }

        // min <= max <=> diff >= 0
        calc_t const diff_ing = max_value - min_value;

        // if containing covers the whole globe it contains all
        if (diff_ing >= constants::period())
        {
            return true;
        }

        // calculate positive longitude translation with min_value as origin
        calc_t const diff_min = math::longitude_distance_unsigned<units_t, calc_t>(min_value, value);

        return DiffCheck::template apply<calc_t>(diff_min, min_value, max_value);
    }
};


// spherical_equatorial_tag, spherical_polar_tag and geographic_cat are casted to spherical_tag
template <typename Geometry>
struct within_range<Geometry, 0, spherical_tag>
    : longitude_range<Geometry, within_coord, within_longitude_diff>
{};


template <typename Geometry>
struct covered_by_range<Geometry, 0, spherical_tag>
    : longitude_range<Geometry, covered_by_coord, covered_by_longitude_diff>
{};


template
<
    template <typename, std::size_t, typename> class SubStrategy,
    typename CSTag, // cartesian_tag or spherical_tag
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct relate_point_box_loop
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        if (! SubStrategy<Point, Dimension, CSTag>::apply(get<Dimension>(point),
                    get<min_corner, Dimension>(box),
                    get<max_corner, Dimension>(box))
            )
        {
            return false;
        }

        return relate_point_box_loop
            <
                SubStrategy,
                CSTag,
                Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template
<
    template <typename, std::size_t, typename> class SubStrategy,
    typename CSTag,
    std::size_t DimensionCount
>
struct relate_point_box_loop<SubStrategy, CSTag, DimensionCount, DimensionCount>
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& , Box const& )
    {
        return true;
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

struct cartesian_point_box
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return detail::relate_point_box_loop
            <
                detail::within_range,
                cartesian_tag,
                0, dimension<Point>::value
            >::apply(point, box);
    }
};

struct spherical_point_box
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return detail::relate_point_box_loop
            <
                detail::within_range,
                spherical_tag,
                0, dimension<Point>::value
            >::apply(point, box);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename Point, typename Box>
struct default_strategy
    <
        Point, Box,
        point_tag, box_tag,
        pointlike_tag, areal_tag,
        cartesian_tag, cartesian_tag
    >
{
    typedef within::cartesian_point_box type;
};

// spherical_equatorial_tag, spherical_polar_tag and geographic_cat are casted to spherical_tag
template <typename Point, typename Box>
struct default_strategy
    <
        Point, Box,
        point_tag, box_tag,
        pointlike_tag, areal_tag,
        spherical_tag, spherical_tag
    >
{
    typedef within::spherical_point_box type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

} // namespace within

namespace covered_by
{

struct cartesian_point_box
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return within::detail::relate_point_box_loop
            <
                within::detail::covered_by_range,
                cartesian_tag,
                0, dimension<Point>::value
            >::apply(point, box);
    }
};

struct spherical_point_box
{
    template <typename Point, typename Box>
    static inline bool apply(Point const& point, Box const& box)
    {
        return within::detail::relate_point_box_loop
            <
                within::detail::covered_by_range,
                spherical_tag,
                0, dimension<Point>::value
            >::apply(point, box);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{


template <typename Point, typename Box>
struct default_strategy
    <
        Point, Box,
        point_tag, box_tag,
        pointlike_tag, areal_tag,
        cartesian_tag, cartesian_tag
    >
{
    typedef covered_by::cartesian_point_box type;
};

// spherical_equatorial_tag, spherical_polar_tag and geographic_cat are casted to spherical_tag
template <typename Point, typename Box>
struct default_strategy
    <
        Point, Box,
        point_tag, box_tag,
        pointlike_tag, areal_tag,
        spherical_tag, spherical_tag
    >
{
    typedef covered_by::spherical_point_box type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


} // namespace covered_by


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP

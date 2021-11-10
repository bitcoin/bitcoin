// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2013 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2013 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_POINT_ON_SURFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_POINT_ON_SURFACE_HPP


#include <cstddef>
#include <numeric>

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/extreme_points.hpp>
#include <boost/geometry/algorithms/detail/signed_size_type.hpp>

#include <boost/geometry/strategies/cartesian/centroid_bashein_detmer.hpp>
#include <boost/geometry/strategies/side.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace point_on_surface
{

template <typename CoordinateType, int Dimension>
struct specific_coordinate_first
{
    CoordinateType const m_value_to_be_first;

    inline specific_coordinate_first(CoordinateType value_to_be_skipped)
        : m_value_to_be_first(value_to_be_skipped)
    {}

    template <typename Point>
    inline bool operator()(Point const& lhs, Point const& rhs)
    {
        CoordinateType const lh = geometry::get<Dimension>(lhs);
        CoordinateType const rh = geometry::get<Dimension>(rhs);

        // If both lhs and rhs equal m_value_to_be_first,
        // we should handle conform if lh < rh = FALSE
        // The first condition meets that, keep it first
        if (geometry::math::equals(rh, m_value_to_be_first))
        {
            // Handle conform lh < -INF, which is always false
            return false;
        }

        if (geometry::math::equals(lh, m_value_to_be_first))
        {
            // Handle conform -INF < rh, which is always true
            return true;
        }

        return lh < rh;
    }
};

template <int Dimension, typename Collection, typename Value, typename Predicate>
inline bool max_value(Collection const& collection, Value& the_max, Predicate const& predicate)
{
    bool first = true;
    for (typename Collection::const_iterator it = collection.begin(); it != collection.end(); ++it)
    {
        if (! it->empty())
        {
            Value the_value = geometry::get<Dimension>(*std::max_element(it->begin(), it->end(), predicate));
            if (first || the_value > the_max)
            {
                the_max = the_value;
                first = false;
            }
        }
    }
    return ! first;
}


template <int Dimension, typename Value>
struct select_below
{
    Value m_value;
    inline select_below(Value const& v)
        : m_value(v)
    {}

    template <typename Intruder>
    inline bool operator()(Intruder const& intruder) const
    {
        if (intruder.empty())
        {
            return true;
        }
        Value max = geometry::get<Dimension>(*std::max_element(intruder.begin(), intruder.end(), detail::extreme_points::compare<Dimension>()));
        return geometry::math::equals(max, m_value) || max < m_value;
    }
};

template <int Dimension, typename Value>
struct adapt_base
{
    Value m_value;
    inline adapt_base(Value const& v)
        : m_value(v)
    {}

    template <typename Intruder>
    inline void operator()(Intruder& intruder) const
    {
        if (intruder.size() >= 3)
        {
            detail::extreme_points::move_along_vector<Dimension>(intruder, m_value);
        }
    }
};

template <int Dimension, typename Value>
struct min_of_intruder
{
    template <typename Intruder>
    inline bool operator()(Intruder const& lhs, Intruder const& rhs) const
    {
        Value lhs_min = geometry::get<Dimension>(*std::min_element(lhs.begin(), lhs.end(), detail::extreme_points::compare<Dimension>()));
        Value rhs_min = geometry::get<Dimension>(*std::min_element(rhs.begin(), rhs.end(), detail::extreme_points::compare<Dimension>()));
        return lhs_min < rhs_min;
    }
};


template <typename Point, typename P>
inline void calculate_average(Point& point, std::vector<P> const& points)
{
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;
    typedef typename std::vector<P>::const_iterator iterator_type;

    coordinate_type x = 0;
    coordinate_type y = 0;

    iterator_type end = points.end();
    for ( iterator_type it = points.begin() ; it != end ; ++it)
    {
        x += geometry::get<0>(*it);
        y += geometry::get<1>(*it);
    }

    signed_size_type const count = points.size();
    geometry::set<0>(point, x / count);
    geometry::set<1>(point, y / count);
}


template <int Dimension, typename Extremes, typename Intruders, typename CoordinateType>
inline void replace_extremes_for_self_tangencies(Extremes& extremes, Intruders& intruders, CoordinateType const& max_intruder)
{
    // This function handles self-tangencies.
    // Self-tangencies use, as usual, the major part of code...

    //        ___ e
    //       /|\ \                                                            .
    //      / | \ \                                                           .
    //     /  |  \ \                                                          .
    //    /   |   \ \                                                         .
    //   / /\ |    \ \                                                        .
    //     i2    i1

    // The picture above shows the extreme (outside, "e") and two intruders ("i1","i2")
    // Assume that "i1" is self-tangent with the extreme, in one point at the top
    // Now the "penultimate" value is searched, this is is the top of i2
    // Then everything including and below (this is "i2" here) is removed
    // Then the base of "i1" and of "e" is adapted to this penultimate value
    // It then looks like:

    //      b ___ e
    //       /|\ \                                                            .
    //      / | \ \                                                           .
    //     /  |  \ \                                                          .
    //    /   |   \ \                                                         .
    //   a    c i1

    // Then intruders (here "i1" but there may be more) are sorted from left to right
    // Finally points "a","b" and "c" (in this order) are selected as a new triangle.
    // This triangle will have a centroid which is inside (assumed that intruders left segment
    // is not equal to extremes left segment, but that polygon would be invalid)

    // Find highest non-self tangent intrusion, if any
    CoordinateType penultimate_value;
    specific_coordinate_first<CoordinateType, Dimension> pu_compare(max_intruder);
    if (max_value<Dimension>(intruders, penultimate_value, pu_compare))
    {
        // Throw away all intrusions <= this value, and of the kept one set this as base.
        select_below<Dimension, CoordinateType> predicate(penultimate_value);
        intruders.erase
            (
                std::remove_if(boost::begin(intruders), boost::end(intruders), predicate),
                boost::end(intruders)
            );
        adapt_base<Dimension, CoordinateType> fe_predicate(penultimate_value);
        // Sort from left to right (or bottom to top if Dimension=0)
        std::for_each(boost::begin(intruders), boost::end(intruders), fe_predicate);

        // Also adapt base of extremes
        detail::extreme_points::move_along_vector<Dimension>(extremes, penultimate_value);
    }
    // Then sort in 1-Dim. Take first to calc centroid.
    std::sort(boost::begin(intruders), boost::end(intruders), min_of_intruder<1 - Dimension, CoordinateType>());

    Extremes triangle;
    triangle.reserve(3);

    // Make a triangle of first two points of extremes (the ramp, from left to right), and last point of first intruder (which goes from right to left)
    std::copy(extremes.begin(), extremes.begin() + 2, std::back_inserter(triangle));
    triangle.push_back(intruders.front().back());

    // (alternatively we could use the last two points of extremes, and first point of last intruder...):
    //// ALTERNATIVE: std::copy(extremes.rbegin(), extremes.rbegin() + 2, std::back_inserter(triangle));
    //// ALTERNATIVE: triangle.push_back(intruders.back().front());

    // Now replace extremes with this smaller subset, a triangle, such that centroid calculation will result in a point inside
    extremes = triangle;
}

template <int Dimension, typename Geometry, typename Point, typename SideStrategy>
inline bool calculate_point_on_surface(Geometry const& geometry, Point& point,
                                       SideStrategy const& strategy)
{
    typedef typename geometry::point_type<Geometry>::type point_type;
    typedef typename geometry::coordinate_type<Geometry>::type coordinate_type;
    std::vector<point_type> extremes;

    typedef std::vector<std::vector<point_type> > intruders_type;
    intruders_type intruders;
    geometry::extreme_points<Dimension>(geometry, extremes, intruders, strategy);

    if (extremes.size() < 3)
    {
        return false;
    }

    // If there are intruders, find the max.
    if (! intruders.empty())
    {
        coordinate_type max_intruder;
        detail::extreme_points::compare<Dimension> compare;
        if (max_value<Dimension>(intruders, max_intruder, compare))
        {
            coordinate_type max_extreme = geometry::get<Dimension>(*std::max_element(extremes.begin(), extremes.end(), detail::extreme_points::compare<Dimension>()));
            if (max_extreme > max_intruder)
            {
                detail::extreme_points::move_along_vector<Dimension>(extremes, max_intruder);
            }
            else
            {
                replace_extremes_for_self_tangencies<Dimension>(extremes, intruders, max_intruder);
            }
        }
    }

    // Now calculate the average/centroid of the (possibly adapted) extremes
    calculate_average(point, extremes);

    return true;
}

}} // namespace detail::point_on_surface
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Assigns a Point guaranteed to lie on the surface of the Geometry
\tparam Geometry geometry type. This also defines the type of the output point
\param geometry Geometry to take point from
\param point Point to assign
\param strategy side strategy
 */
template <typename Geometry, typename Point, typename SideStrategy>
inline void point_on_surface(Geometry const& geometry, Point & point,
                             SideStrategy const& strategy)
{
    concepts::check<Point>();
    concepts::check<Geometry const>();

    // First try in Y-direction (which should always succeed for valid polygons)
    if (! detail::point_on_surface::calculate_point_on_surface<1>(geometry, point, strategy))
    {
        // For invalid polygons, we might try X-direction
        detail::point_on_surface::calculate_point_on_surface<0>(geometry, point, strategy);
    }
}

/*!
\brief Assigns a Point guaranteed to lie on the surface of the Geometry
\tparam Geometry geometry type. This also defines the type of the output point
\param geometry Geometry to take point from
\param point Point to assign
 */
template <typename Geometry, typename Point>
inline void point_on_surface(Geometry const& geometry, Point & point)
{
    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<Geometry>::type
        >::type strategy_type;

    point_on_surface(geometry, point, strategy_type());
}


/*!
\brief Returns point guaranteed to lie on the surface of the Geometry
\tparam Geometry geometry type. This also defines the type of the output point
\param geometry Geometry to take point from
\param strategy side strategy
\return The Point guaranteed to lie on the surface of the Geometry
 */
template<typename Geometry, typename SideStrategy>
inline typename geometry::point_type<Geometry>::type
return_point_on_surface(Geometry const& geometry, SideStrategy const& strategy)
{
    typename geometry::point_type<Geometry>::type result;
    geometry::point_on_surface(geometry, result, strategy);
    return result;
}

/*!
\brief Returns point guaranteed to lie on the surface of the Geometry
\tparam Geometry geometry type. This also defines the type of the output point
\param geometry Geometry to take point from
\return The Point guaranteed to lie on the surface of the Geometry
 */
template<typename Geometry>
inline typename geometry::point_type<Geometry>::type
return_point_on_surface(Geometry const& geometry)
{
    typename geometry::point_type<Geometry>::type result;
    geometry::point_on_surface(geometry, result);
    return result;
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_POINT_ON_SURFACE_HPP

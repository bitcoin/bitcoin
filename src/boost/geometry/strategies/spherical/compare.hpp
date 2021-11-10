// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_COMPARE_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_COMPARE_HPP


#include <type_traits>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/compare.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace compare
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <std::size_t I, typename P>
static inline typename geometry::coordinate_type<P>::type
get(P const& p, std::true_type /*same units*/)
{
    return geometry::get<I>(p);
}

template <std::size_t I, typename P>
static inline typename geometry::coordinate_type<P>::type
get(P const& p, std::false_type /*different units*/)
{
    return geometry::get_as_radian<I>(p);
}

template
<
    typename ComparePolicy,
    typename Point1,
    typename Point2,
    std::size_t DimensionCount
>
struct spherical_latitude
{
    typedef typename geometry::coordinate_type<Point1>::type coordinate1_type;
    typedef typename geometry::detail::cs_angular_units<Point1>::type units1_type;
    typedef typename geometry::coordinate_type<Point2>::type coordinate2_type;
    typedef typename geometry::detail::cs_angular_units<Point2>::type units2_type;
    typedef std::is_same<units1_type, units2_type> same_units_type;

    template <typename T1, typename T2>
    static inline bool apply(Point1 const& left, Point2 const& right,
                             T1 const& l1, T2 const& r1)
    {
        // latitudes equal
        if (math::equals(l1, r1))
        {
            return compare::detail::compare_loop
                <
                    ComparePolicy, 2, DimensionCount
                >::apply(left, right);
        }
        else
        {
            return ComparePolicy::apply(l1, r1);
        }
    }

    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        coordinate1_type const& l1 = compare::detail::get<1>(left, same_units_type());
        coordinate2_type const& r1 = compare::detail::get<1>(right, same_units_type());

        return apply(left, right, l1, r1);
    }
};

template
<
    typename ComparePolicy,
    typename Point1,
    typename Point2
>
struct spherical_latitude<ComparePolicy, Point1, Point2, 1>
{
    template <typename T1, typename T2>
    static inline bool apply(Point1 const& left, Point2 const& right,
                             T1 const& , T2 const& )
    {
        return apply(left, right);
    }

    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        return compare::detail::compare_loop
            <
                ComparePolicy, 1, 1
            >::apply(left, right);
    }
};

template
<
    typename ComparePolicy,
    typename Point1,
    typename Point2,
    std::size_t DimensionCount
>
struct spherical_longitude
{
    typedef typename geometry::coordinate_type<Point1>::type coordinate1_type;
    typedef typename geometry::detail::cs_angular_units<Point1>::type units1_type;
    typedef typename geometry::coordinate_type<Point2>::type coordinate2_type;
    typedef typename geometry::detail::cs_angular_units<Point2>::type units2_type;
    typedef std::is_same<units1_type, units2_type> same_units_type;
    typedef std::conditional_t<same_units_type::value, units1_type, geometry::radian> units_type;

    static const bool is_equatorial = ! std::is_same
                                        <
                                            typename geometry::cs_tag<Point1>::type,
                                            geometry::spherical_polar_tag
                                        >::value;

    static inline bool are_both_at_antimeridian(coordinate1_type const& l0,
                                                coordinate2_type const& r0,
                                                bool & is_left_at,
                                                bool & is_right_at)
    {
        is_left_at = math::is_longitude_antimeridian<units_type>(l0);
        is_right_at = math::is_longitude_antimeridian<units_type>(r0);
        return is_left_at && is_right_at;
    }

    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        // if units are different the coordinates are in radians
        coordinate1_type const& l0 = compare::detail::get<0>(left, same_units_type());
        coordinate2_type const& r0 = compare::detail::get<0>(right, same_units_type());
        coordinate1_type const& l1 = compare::detail::get<1>(left, same_units_type());
        coordinate2_type const& r1 = compare::detail::get<1>(right, same_units_type());

        bool is_left_at_antimeridian = false;
        bool is_right_at_antimeridian = false;
        
            // longitudes equal
        if (math::equals(l0, r0)
               // both at antimeridian
            || are_both_at_antimeridian(l0, r0, is_left_at_antimeridian, is_right_at_antimeridian)
               // both at pole
            || (math::equals(l1, r1)
                && math::is_latitude_pole<units_type, is_equatorial>(l1)))
        {
            return spherical_latitude
                <
                    ComparePolicy, Point1, Point2, DimensionCount
                >::apply(left, right, l1, r1);
        }
        // if left is at antimeridian and right is not at antimeridian
        // then left is greater than right
        else if (is_left_at_antimeridian)
        {
            // less/equal_to -> false, greater -> true
            return ComparePolicy::apply(1, 0);
        }
        // if right is at antimeridian and left is not at antimeridian
        // then left is lesser than right
        else if (is_right_at_antimeridian)
        {
            // less -> true, equal_to/greater -> false
            return ComparePolicy::apply(0, 1);
        }
        else
        {
            return ComparePolicy::apply(l0, r0);
        }
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Compare strategy for spherical coordinates
\ingroup strategies
\tparam Point point-type
\tparam Dimension dimension
*/
template
<
    typename ComparePolicy,
    int Dimension = -1
>
struct spherical
    : cartesian<ComparePolicy, Dimension>
{};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
// all dimensions starting from longitude
template <typename ComparePolicy>
struct spherical<ComparePolicy, -1>
{
    template <typename Point1, typename Point2>
    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        return compare::detail::spherical_longitude
            <
                ComparePolicy,
                Point1,
                Point2,
                std::conditional_t
                    <
                        (dimension<Point1>::value < dimension<Point2>::value),
                        std::integral_constant<std::size_t, dimension<Point1>::value>,
                        std::integral_constant<std::size_t, dimension<Point2>::value>
                    >::value
            >::apply(left, right);
    }
};

// only longitudes (and latitudes to check poles)
template <typename ComparePolicy>
struct spherical<ComparePolicy, 0>
{
    template <typename Point1, typename Point2>
    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        return compare::detail::spherical_longitude
            <
                ComparePolicy, Point1, Point2, 1
            >::apply(left, right);
    }
};

// only latitudes
template <typename ComparePolicy>
struct spherical<ComparePolicy, 1>
{
    template <typename Point1, typename Point2>
    static inline bool apply(Point1 const& left, Point2 const& right)
    {
        return compare::detail::spherical_latitude
            <
                ComparePolicy, Point1, Point2, 2
            >::apply(left, right);
    }
};

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace services
{


template <typename ComparePolicy, typename Point1, typename Point2, int Dimension>
struct default_strategy
    <
        ComparePolicy, Point1, Point2, Dimension,
        spherical_tag, spherical_tag
    >
{
    typedef compare::spherical<ComparePolicy, Dimension> type;
};

template <typename ComparePolicy, typename Point1, typename Point2, int Dimension>
struct default_strategy
    <
        ComparePolicy, Point1, Point2, Dimension,
        spherical_polar_tag, spherical_polar_tag
    >
{
    typedef compare::spherical<ComparePolicy, Dimension> type;
};

template <typename ComparePolicy, typename Point1, typename Point2, int Dimension>
struct default_strategy
    <
        ComparePolicy, Point1, Point2, Dimension,
        spherical_equatorial_tag, spherical_equatorial_tag
    >
{
    typedef compare::spherical<ComparePolicy, Dimension> type;
};

template <typename ComparePolicy, typename Point1, typename Point2, int Dimension>
struct default_strategy
    <
        ComparePolicy, Point1, Point2, Dimension,
        geographic_tag, geographic_tag
    >
{
    typedef compare::spherical<ComparePolicy, Dimension> type;
};


} // namespace services


}} // namespace strategy::compare


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_COMPARE_HPP

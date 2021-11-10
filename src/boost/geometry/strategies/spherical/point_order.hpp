// Boost.Geometry

// Copyright (c) 2019-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_POINT_ORDER_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_POINT_ORDER_HPP


//#include <type_traits>


#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/formulas/spherical.hpp>

#include <boost/geometry/strategy/spherical/area.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>
#include <boost/geometry/strategies/point_order.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace point_order
{

//template <typename CalculationType = void>
//struct spherical
//{
//    typedef azimuth_tag version_tag;
//
//    template <typename Geometry>
//    struct result_type
//    {
//        typedef typename geometry::select_calculation_type_alt
//            <
//                CalculationType, Geometry
//            >::type type;
//    };
//
//    template <typename Point>
//    inline bool apply(Point const& p1, Point const& p2,
//                      typename result_type<Point>::type & azi,
//                      typename result_type<Point>::type & razi) const
//    {
//        typedef typename result_type<Point>::type calc_t;
//
//        if (equals_point_point(p1, p2))
//        {
//            return false;
//        }
//
//        calc_t lon1 = geometry::get_as_radian<0>(p1);
//        calc_t lat1 = geometry::get_as_radian<1>(p1);
//        calc_t lon2 = geometry::get_as_radian<0>(p2);
//        calc_t lat2 = geometry::get_as_radian<1>(p2);
//
//        convert_latitudes<Point>(lat1, lat2);
//
//        formula::result_spherical<calc_t>
//            res = formula::spherical_azimuth<calc_t, true>(lon1, lat1, lon2, lat2);
//
//        azi = res.azimuth;
//        razi = res.reverse_azimuth;
//
//        return true;
//    }
//
//    template <typename Point>
//    inline typename result_type<Point>::type
//    apply(Point const& /*p0*/, Point const& /*p1*/, Point const& /*p2*/,
//          typename result_type<Point>::type const& azi1,
//          typename result_type<Point>::type const& azi2) const
//    {
//        // TODO: support poles
//        return math::longitude_distance_signed<radian>(azi1, azi2);
//    }
//
//private:
//    template <typename Point>
//    static bool equals_point_point(Point const& p0, Point const& p1)
//    {
//        return strategy::within::spherical_point_point::apply(p0, p1);
//    }
//
//    template <typename Point, typename CalcT>
//    static void convert_latitudes(CalcT & lat1, CalcT & lat2)
//    {
//        static const bool is_polar = std::is_same
//            <
//                typename geometry::cs_tag<Point>::type,
//                spherical_polar_tag
//            >::value;
//
//        if (BOOST_GEOMETRY_CONDITION(is_polar))
//        {
//            CalcT pi_half = math::half_pi<CalcT>();
//            lat1 = pi_half - lat1;
//            lat2 = pi_half - lat2;
//        }
//    }
//};

template <typename CalculationType = void>
struct spherical
    : strategy::area::spherical<double, CalculationType>
{
    typedef area_tag version_tag;

    // TEMP
    static strategy::area::spherical<double, CalculationType> get_area_strategy()
    {
        return strategy::area::spherical<double, CalculationType>();
    }
};

namespace services
{

template <>
struct default_strategy<spherical_equatorial_tag>
{
    typedef spherical<> type;
};

/*template <>
struct default_strategy<spherical_polar_tag>
{
    typedef spherical<> type;
};*/

} // namespace services

}} // namespace strategy::point_order

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_POINT_ORDER_HPP

// Boost.Geometry

// Copyright (c) 2016-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_SPHERICAL_HPP
#define BOOST_GEOMETRY_FORMULAS_SPHERICAL_HPP

#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/radius.hpp>

//#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/formulas/result_direct.hpp>

namespace boost { namespace geometry {
    
namespace formula {

template <typename T>
struct result_spherical
{
    result_spherical()
        : azimuth(0)
        , reverse_azimuth(0)
    {}

    T azimuth;
    T reverse_azimuth;
};

template <typename T>
static inline void sph_to_cart3d(T const& lon, T const& lat, T & x, T & y, T & z)
{
    T const cos_lat = cos(lat);
    x = cos_lat * cos(lon);
    y = cos_lat * sin(lon);
    z = sin(lat);
}

template <typename Point3d, typename PointSph>
static inline Point3d sph_to_cart3d(PointSph const& point_sph)
{
    typedef typename coordinate_type<Point3d>::type calc_t;

    calc_t const lon = get_as_radian<0>(point_sph);
    calc_t const lat = get_as_radian<1>(point_sph);
    calc_t x, y, z;
    sph_to_cart3d(lon, lat, x, y, z);

    Point3d res;
    set<0>(res, x);
    set<1>(res, y);
    set<2>(res, z);

    return res;
}

template <typename T>
static inline void cart3d_to_sph(T const& x, T const& y, T const& z, T & lon, T & lat)
{
    lon = atan2(y, x);
    lat = asin(z);
}

template <typename PointSph, typename Point3d>
static inline PointSph cart3d_to_sph(Point3d const& point_3d)
{
    typedef typename coordinate_type<PointSph>::type coord_t;
    typedef typename coordinate_type<Point3d>::type calc_t;

    calc_t const x = get<0>(point_3d);
    calc_t const y = get<1>(point_3d);
    calc_t const z = get<2>(point_3d);
    calc_t lonr, latr;
    cart3d_to_sph(x, y, z, lonr, latr);

    PointSph res;
    set_from_radian<0>(res, lonr);
    set_from_radian<1>(res, latr);

    coord_t lon = get<0>(res);
    coord_t lat = get<1>(res);

    math::normalize_spheroidal_coordinates
        <
            typename geometry::detail::cs_angular_units<PointSph>::type,
            coord_t
        >(lon, lat);

    set<0>(res, lon);
    set<1>(res, lat);

    return res;
}

// -1 right
// 1 left
// 0 on
template <typename Point3d1, typename Point3d2>
static inline int sph_side_value(Point3d1 const& norm, Point3d2 const& pt)
{
    typedef typename select_coordinate_type<Point3d1, Point3d2>::type calc_t;
    calc_t c0 = 0;
    calc_t d = dot_product(norm, pt);
    return math::equals(d, c0) ? 0
        : d > c0 ? 1
        : -1; // d < 0
}

template <typename CT, bool ReverseAzimuth, typename T1, typename T2>
static inline result_spherical<CT> spherical_azimuth(T1 const& lon1,
                                                     T1 const& lat1,
                                                     T2 const& lon2,
                                                     T2 const& lat2)
{
    typedef result_spherical<CT> result_type;
    result_type result;

    // http://williams.best.vwh.net/avform.htm#Crs
    // https://en.wikipedia.org/wiki/Great-circle_navigation
    CT dlon = lon2 - lon1;

    // An optimization which should kick in often for Boxes
    //if ( math::equals(dlon, ReturnType(0)) )
    //if ( get<0>(p1) == get<0>(p2) )
    //{
    //    return - sin(get_as_radian<1>(p1)) * cos_p2lat);
    //}

    CT const cos_dlon = cos(dlon);
    CT const sin_dlon = sin(dlon);
    CT const cos_lat1 = cos(lat1);
    CT const cos_lat2 = cos(lat2);
    CT const sin_lat1 = sin(lat1);
    CT const sin_lat2 = sin(lat2);

    {
        // "An alternative formula, not requiring the pre-computation of d"
        // In the formula below dlon is used as "d"
        CT const y = sin_dlon * cos_lat2;
        CT const x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;
        result.azimuth = atan2(y, x);
    }

    if (ReverseAzimuth)
    {
        CT const y = sin_dlon * cos_lat1;
        CT const x = sin_lat2 * cos_lat1 * cos_dlon - cos_lat2 * sin_lat1;
        result.reverse_azimuth = atan2(y, x);
    }

    return result;
}

template <typename ReturnType, typename T1, typename T2>
inline ReturnType spherical_azimuth(T1 const& lon1, T1 const& lat1,
                                    T2 const& lon2, T2 const& lat2)
{
    return spherical_azimuth<ReturnType, false>(lon1, lat1, lon2, lat2).azimuth;
}

template <typename T>
inline T spherical_azimuth(T const& lon1, T const& lat1, T const& lon2, T const& lat2)
{
    return spherical_azimuth<T, false>(lon1, lat1, lon2, lat2).azimuth;
}

template <typename T>
inline int azimuth_side_value(T const& azi_a1_p, T const& azi_a1_a2)
{
    T const c0 = 0;
    T const pi = math::pi<T>();

    // instead of the formula from XTD
    //calc_t a_diff = asin(sin(azi_a1_p - azi_a1_a2));

    T a_diff = azi_a1_p - azi_a1_a2;
    // normalize, angle in (-pi, pi]
    math::detail::normalize_angle_loop<radian>(a_diff);

    // NOTE: in general it shouldn't be required to support the pi/-pi case
    // because in non-cartesian systems it makes sense to check the side
    // only "between" the endpoints.
    // However currently the winding strategy calls the side strategy
    // for vertical segments to check if the point is "between the endpoints.
    // This could be avoided since the side strategy is not required for that
    // because meridian is the shortest path. So a difference of
    // longitudes would be sufficient (of course normalized to (-pi, pi]).

    // NOTE: with the above said, the pi/-pi check is temporary
    // however in case if this was required
    // the geodesics on ellipsoid aren't "symmetrical"
    // therefore instead of comparing a_diff to pi and -pi
    // one should probably use inverse azimuths and compare
    // the difference to 0 as well

    // positive azimuth is on the right side
    return math::equals(a_diff, c0)
        || math::equals(a_diff, pi)
        || math::equals(a_diff, -pi) ? 0
        : a_diff > 0 ? -1 // right
        : 1; // left
}

template
<
    bool Coordinates,
    bool ReverseAzimuth,
    typename CT,
    typename Sphere
>
inline result_direct<CT> spherical_direct(CT const& lon1,
                                          CT const& lat1,
                                          CT const& sig12,
                                          CT const& alp1,
                                          Sphere const& sphere)
{
    result_direct<CT> result;

    CT const sin_alp1 = sin(alp1);
    CT const sin_lat1 = sin(lat1);
    CT const cos_alp1 = cos(alp1);
    CT const cos_lat1 = cos(lat1);

    CT const norm = math::sqrt(cos_alp1 * cos_alp1 + sin_alp1 * sin_alp1
                                                   * sin_lat1 * sin_lat1);
    CT const alp0 = atan2(sin_alp1 * cos_lat1, norm);
    CT const sig1 = atan2(sin_lat1, cos_alp1 * cos_lat1);
    CT const sig2 = sig1 + sig12 / get_radius<0>(sphere);

    CT const cos_sig2 = cos(sig2);
    CT const sin_alp0 = sin(alp0);
    CT const cos_alp0 = cos(alp0);

    if (Coordinates)
    {
        CT const sin_sig2 = sin(sig2);
        CT const sin_sig1 = sin(sig1);
        CT const cos_sig1 = cos(sig1);

        CT const norm2 = math::sqrt(cos_alp0 * cos_alp0 * cos_sig2 * cos_sig2
                                    + sin_alp0 * sin_alp0);
        CT const lat2 = atan2(cos_alp0 * sin_sig2, norm2);

        CT const omg1 = atan2(sin_alp0 * sin_sig1, cos_sig1);
        CT const lon2 = atan2(sin_alp0 * sin_sig2, cos_sig2);

        result.lon2 = lon1 + lon2 - omg1;
        result.lat2 = lat2;

        // For longitudes close to the antimeridian the result can be out
        // of range. Therefore normalize.
        math::detail::normalize_angle_cond<radian>(result.lon2);
    }

    if (ReverseAzimuth)
    {
        CT const alp2 = atan2(sin_alp0, cos_alp0 * cos_sig2);
        result.reverse_azimuth = alp2;
    }

    return result;
}

} // namespace formula

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_FORMULAS_SPHERICAL_HPP

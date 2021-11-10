// Boost.Geometry

// Copyright (c) 2016-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_FORMULAS_GEOGRAPHIC_HPP

#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/arithmetic/normalize.hpp>

#include <boost/geometry/formulas/eccentricity_sqr.hpp>
#include <boost/geometry/formulas/flattening.hpp>
#include <boost/geometry/formulas/unit_spheroid.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>

namespace boost { namespace geometry {
    
namespace formula {

template <typename Point3d, typename PointGeo, typename Spheroid>
inline Point3d geo_to_cart3d(PointGeo const& point_geo, Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type calc_t;

    calc_t const c1 = 1;
    calc_t const e_sqr = eccentricity_sqr<calc_t>(spheroid);

    calc_t const lon = get_as_radian<0>(point_geo);
    calc_t const lat = get_as_radian<1>(point_geo);

    Point3d res;

    calc_t const sin_lat = sin(lat);

    // "unit" spheroid, a = 1
    calc_t const N = c1 / math::sqrt(c1 - e_sqr * math::sqr(sin_lat));
    calc_t const N_cos_lat = N * cos(lat);

    set<0>(res, N_cos_lat * cos(lon));
    set<1>(res, N_cos_lat * sin(lon));
    set<2>(res, N * (c1 - e_sqr) * sin_lat);

    return res;
}

template <typename PointGeo, typename Spheroid, typename Point3d>
inline void geo_to_cart3d(PointGeo const& point_geo, Point3d & result, Point3d & north, Point3d & east, Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type calc_t;

    calc_t const c1 = 1;
    calc_t const e_sqr = eccentricity_sqr<calc_t>(spheroid);

    calc_t const lon = get_as_radian<0>(point_geo);
    calc_t const lat = get_as_radian<1>(point_geo);

    calc_t const sin_lon = sin(lon);
    calc_t const cos_lon = cos(lon);
    calc_t const sin_lat = sin(lat);
    calc_t const cos_lat = cos(lat);

    // "unit" spheroid, a = 1
    calc_t const N = c1 / math::sqrt(c1 - e_sqr * math::sqr(sin_lat));
    calc_t const N_cos_lat = N * cos_lat;

    set<0>(result, N_cos_lat * cos_lon);
    set<1>(result, N_cos_lat * sin_lon);
    set<2>(result, N * (c1 - e_sqr) * sin_lat);

    set<0>(east, -sin_lon);
    set<1>(east, cos_lon);
    set<2>(east, 0);

    set<0>(north, -sin_lat * cos_lon);
    set<1>(north, -sin_lat * sin_lon);
    set<2>(north, cos_lat);
}

template <typename PointGeo, typename Point3d, typename Spheroid>
inline PointGeo cart3d_to_geo(Point3d const& point_3d, Spheroid const& spheroid)
{
    typedef typename coordinate_type<PointGeo>::type coord_t;
    typedef typename coordinate_type<Point3d>::type calc_t;

    calc_t const c1 = 1;
    //calc_t const c2 = 2;
    calc_t const e_sqr = eccentricity_sqr<calc_t>(spheroid);

    calc_t const x = get<0>(point_3d);
    calc_t const y = get<1>(point_3d);
    calc_t const z = get<2>(point_3d);
    calc_t const xy_l = math::sqrt(math::sqr(x) + math::sqr(y));

    calc_t const lonr = atan2(y, x);
    
    // NOTE: Alternative version
    // http://www.iag-aig.org/attach/989c8e501d9c5b5e2736955baf2632f5/V60N2_5FT.pdf
    // calc_t const lonr = c2 * atan2(y, x + xy_l);
    
    calc_t const latr = atan2(z, (c1 - e_sqr) * xy_l);

    // NOTE: If h is equal to 0 then there is no need to improve value of latitude
    //       because then N_i / (N_i + h_i) = 1
    // http://www.navipedia.net/index.php/Ellipsoidal_and_Cartesian_Coordinates_Conversion

    PointGeo res;

    set_from_radian<0>(res, lonr);
    set_from_radian<1>(res, latr);

    coord_t lon = get<0>(res);
    coord_t lat = get<1>(res);

    math::normalize_spheroidal_coordinates
        <
            typename coordinate_system<PointGeo>::type::units,
            coord_t
        >(lon, lat);

    set<0>(res, lon);
    set<1>(res, lat);

    return res;
}

template <typename Point3d, typename Spheroid>
inline Point3d projected_to_xy(Point3d const& point_3d, Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;    
    
    // len_xy = sqrt(x^2 + y^2)
    // r = len_xy - |z / tan(lat)|
    // assuming h = 0
    // lat = atan2(z, (1 - e^2) * len_xy);
    // |z / tan(lat)| = (1 - e^2) * len_xy
    // r = e^2 * len_xy
    // x_res = r * cos(lon) = e^2 * len_xy * x / len_xy = e^2 * x
    // y_res = r * sin(lon) = e^2 * len_xy * y / len_xy = e^2 * y
    
    coord_t const c0 = 0;
    coord_t const e_sqr = formula::eccentricity_sqr<coord_t>(spheroid);

    Point3d res;

    set<0>(res, e_sqr * get<0>(point_3d));
    set<1>(res, e_sqr * get<1>(point_3d));
    set<2>(res, c0);

    return res;
}

template <typename Point3d, typename Spheroid>
inline Point3d projected_to_surface(Point3d const& direction, Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    //coord_t const c0 = 0;
    coord_t const c2 = 2;
    coord_t const c4 = 4;

    // calculate the point of intersection of a ray and spheroid's surface
    // the origin is the origin of the coordinate system
    //(x*x+y*y)/(a*a) + z*z/(b*b) = 1
    // x = d.x * t
    // y = d.y * t
    // z = d.z * t        
    coord_t const dx = get<0>(direction);
    coord_t const dy = get<1>(direction);
    coord_t const dz = get<2>(direction);

    //coord_t const a_sqr = math::sqr(get_radius<0>(spheroid));
    //coord_t const b_sqr = math::sqr(get_radius<2>(spheroid));
    // "unit" spheroid, a = 1
    coord_t const a_sqr = 1;
    coord_t const b_sqr = math::sqr(formula::unit_spheroid_b<coord_t>(spheroid));

    coord_t const param_a = (dx*dx + dy*dy) / a_sqr + dz*dz / b_sqr;
    coord_t const delta = c4 * param_a;
    // delta >= 0
    coord_t const t = math::sqrt(delta) / (c2 * param_a);

    // result = direction * t
    Point3d result = direction;
    multiply_value(result, t);

    return result;
}

template <typename Point3d, typename Spheroid>
inline bool projected_to_surface(Point3d const& origin, Point3d const& direction,
                                 Point3d & result1, Point3d & result2,
                                 Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    coord_t const c0 = 0;
    coord_t const c1 = 1;
    coord_t const c2 = 2;
    coord_t const c4 = 4;

    // calculate the point of intersection of a ray and spheroid's surface
    //(x*x+y*y)/(a*a) + z*z/(b*b) = 1
    // x = o.x + d.x * t
    // y = o.y + d.y * t
    // z = o.z + d.z * t        
    coord_t const ox = get<0>(origin);
    coord_t const oy = get<1>(origin);
    coord_t const oz = get<2>(origin);
    coord_t const dx = get<0>(direction);
    coord_t const dy = get<1>(direction);
    coord_t const dz = get<2>(direction);

    //coord_t const a_sqr = math::sqr(get_radius<0>(spheroid));
    //coord_t const b_sqr = math::sqr(get_radius<2>(spheroid));
    // "unit" spheroid, a = 1
    coord_t const a_sqr = 1;
    coord_t const b_sqr = math::sqr(formula::unit_spheroid_b<coord_t>(spheroid));

    coord_t const param_a = (dx*dx + dy*dy) / a_sqr + dz*dz / b_sqr;
    coord_t const param_b = c2 * ((ox*dx + oy*dy) / a_sqr + oz*dz / b_sqr);
    coord_t const param_c = (ox*ox + oy*oy) / a_sqr + oz*oz / b_sqr - c1;

    coord_t const delta = math::sqr(param_b) - c4 * param_a*param_c;

    // equals() ?
    if (delta < c0 || param_a == 0)
    {
        return false;
    }

    // result = origin + direction * t

    coord_t const sqrt_delta = math::sqrt(delta);
    coord_t const two_a = c2 * param_a;

    coord_t const t1 = (-param_b + sqrt_delta) / two_a;
    coord_t const t2 = (-param_b - sqrt_delta) / two_a;
    geometry::detail::for_each_dimension<Point3d>([&](auto index)
    {
        set<index>(result1, get<index>(origin) + get<index>(direction) * t1);
        set<index>(result2, get<index>(origin) + get<index>(direction) * t2);
    });

    return true;
}

template <typename Point3d, typename Spheroid>
inline bool great_elliptic_intersection(Point3d const& a1, Point3d const& a2,
                                        Point3d const& b1, Point3d const& b2,
                                        Point3d & result,
                                        Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    coord_t c0 = 0;
    coord_t c1 = 1;

    Point3d n1 = cross_product(a1, a2);
    Point3d n2 = cross_product(b1, b2);

    // intersection direction
    Point3d id = cross_product(n1, n2);
    coord_t id_len_sqr = dot_product(id, id);

    if (math::equals(id_len_sqr, c0))
    {
        return false;
    }

    // no need to normalize a1 and a2 because the intersection point on
    // the opposite side of the globe is at the same distance from the origin
    coord_t cos_a1i = dot_product(a1, id);
    coord_t cos_a2i = dot_product(a2, id);
    coord_t gri = math::detail::greatest(cos_a1i, cos_a2i);
    Point3d neg_id = id;
    multiply_value(neg_id, -c1);
    coord_t cos_a1ni = dot_product(a1, neg_id);
    coord_t cos_a2ni = dot_product(a2, neg_id);
    coord_t grni = math::detail::greatest(cos_a1ni, cos_a2ni);

    if (gri >= grni)
    {
        result = projected_to_surface(id, spheroid);
    }
    else
    {
        result = projected_to_surface(neg_id, spheroid);
    }

    return true;
}

template <typename Point3d1, typename Point3d2>
static inline int elliptic_side_value(Point3d1 const& origin, Point3d1 const& norm, Point3d2 const& pt)
{
    typedef typename coordinate_type<Point3d1>::type calc_t;
    calc_t c0 = 0;

    // vector oposite to pt - origin
    // only for the purpose of assigning origin
    Point3d1 vec = origin;
    subtract_point(vec, pt);

    calc_t d = dot_product(norm, vec);

    // since the vector is opposite the signs are opposite
    return math::equals(d, c0) ? 0
        : d < c0 ? 1
        : -1; // d > 0
}

template <typename Point3d, typename Spheroid>
inline bool planes_spheroid_intersection(Point3d const& o1, Point3d const& n1,
                                         Point3d const& o2, Point3d const& n2,
                                         Point3d & ip1, Point3d & ip2,
                                         Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    coord_t c0 = 0;
    coord_t c1 = 1;

    // Below
    // n . (p - o) = 0
    // n . p - n . o = 0
    // n . p + d = 0
    // n . p = h

    // intersection direction
    Point3d id = cross_product(n1, n2);

    if (math::equals(dot_product(id, id), c0))
    {
        return false;
    }

    coord_t dot_n1_n2 = dot_product(n1, n2);
    coord_t dot_n1_n2_sqr = math::sqr(dot_n1_n2);

    coord_t h1 = dot_product(n1, o1);
    coord_t h2 = dot_product(n2, o2);

    coord_t denom = c1 - dot_n1_n2_sqr;
    coord_t C1 = (h1 - h2 * dot_n1_n2) / denom;
    coord_t C2 = (h2 - h1 * dot_n1_n2) / denom;

    // C1 * n1 + C2 * n2
    Point3d io;
    geometry::detail::for_each_dimension<Point3d>([&](auto index)
    {
        set<index>(io, C1 * get<index>(n1) + C2 * get<index>(n2));
    });

    if (! projected_to_surface(io, id, ip1, ip2, spheroid))
    {
        return false;
    }

    return true;
}


template <typename Point3d, typename Spheroid>
inline void experimental_elliptic_plane(Point3d const& p1, Point3d const& p2,
                                        Point3d & v1, Point3d & v2,
                                        Point3d & origin, Point3d & normal,
                                        Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    Point3d xy1 = projected_to_xy(p1, spheroid);
    Point3d xy2 = projected_to_xy(p2, spheroid);

    // origin = (xy1 + xy2) / 2
    // v1 = p1 - origin
    // v2 = p2 - origin
    coord_t const half = coord_t(0.5);
    geometry::detail::for_each_dimension<Point3d>([&](auto index)
    {
        coord_t const o = (get<index>(xy1) + get<index>(xy2)) * half;
        set<index>(origin, o);
        set<index>(v1, get<index>(p1) - o);
        set<index>(v2, get<index>(p1) - o);
    });

    normal = cross_product(v1, v2);
}

template <typename Point3d, typename Spheroid>
inline void experimental_elliptic_plane(Point3d const& p1, Point3d const& p2,
                                        Point3d & origin, Point3d & normal,
                                        Spheroid const& spheroid)
{
    Point3d v1, v2;
    experimental_elliptic_plane(p1, p2, v1, v2, origin, normal, spheroid);
}

template <typename Point3d, typename Spheroid>
inline bool experimental_elliptic_intersection(Point3d const& a1, Point3d const& a2,
                                               Point3d const& b1, Point3d const& b2,
                                               Point3d & result,
                                               Spheroid const& spheroid)
{
    typedef typename coordinate_type<Point3d>::type coord_t;

    coord_t c0 = 0;
    coord_t c1 = 1;

    Point3d a1v, a2v, o1, n1;
    experimental_elliptic_plane(a1, a2, a1v, a2v, o1, n1, spheroid);
    Point3d b1v, b2v, o2, n2;
    experimental_elliptic_plane(b1, b2, b1v, b2v, o2, n2, spheroid);

    if (! geometry::detail::vec_normalize(n1) || ! geometry::detail::vec_normalize(n2))
    {
        return false;
    }

    Point3d ip1_s, ip2_s;
    if (! planes_spheroid_intersection(o1, n1, o2, n2, ip1_s, ip2_s, spheroid))
    {
        return false;
    }

    // NOTE: simplified test, may not work in all cases
    coord_t dot_a1i1 = dot_product(a1, ip1_s);
    coord_t dot_a2i1 = dot_product(a2, ip1_s);
    coord_t gri1 = math::detail::greatest(dot_a1i1, dot_a2i1);
    coord_t dot_a1i2 = dot_product(a1, ip2_s);
    coord_t dot_a2i2 = dot_product(a2, ip2_s);
    coord_t gri2 = math::detail::greatest(dot_a1i2, dot_a2i2);

    result = gri1 >= gri2 ? ip1_s : ip2_s;

    return true;
}

} // namespace formula

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_FORMULAS_GEOGRAPHIC_HPP

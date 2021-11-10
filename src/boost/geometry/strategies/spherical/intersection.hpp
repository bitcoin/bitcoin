// Boost.Geometry

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2016-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_INTERSECTION_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_INTERSECTION_HPP

#include <algorithm>
#include <type_traits>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/arithmetic/normalize.hpp>
#include <boost/geometry/formulas/spherical.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/policies/robustness/segment_ratio.hpp>

#include <boost/geometry/strategy/spherical/area.hpp>
#include <boost/geometry/strategy/spherical/envelope.hpp>
#include <boost/geometry/strategy/spherical/expand_box.hpp>
#include <boost/geometry/strategy/spherical/expand_segment.hpp>

#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/intersection.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/strategies/spherical/disjoint_box_box.hpp>
#include <boost/geometry/strategies/spherical/disjoint_segment_box.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>
#include <boost/geometry/strategies/spherical/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>
#include <boost/geometry/strategies/within.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace intersection
{

// NOTE:
// The coordinates of crossing IP may be calculated with small precision in some cases.
// For double, near the equator noticed error ~1e-9 so far greater than
// machine epsilon which is ~1e-16. This error is ~0.04m.
// E.g. consider two cases, one near the origin and the second one rotated by 90 deg around Z or SN axis.
// After the conversion from spherical degrees to cartesian 3d the following coordinates
// are calculated:
// for sph (-1 -1,  1 1) deg cart3d ys are -0.017449748351250485 and  0.017449748351250485
// for sph (89 -1, 91 1) deg cart3d xs are  0.017449748351250571 and -0.017449748351250450
// During the conversion degrees must first be converted to radians and then radians
// are passed into trigonometric functions. The error may have several causes:
// 1. Radians cannot represent exactly the same angles as degrees.
// 2. Different longitudes are passed into sin() for x, corresponding to cos() for y,
//    and for different angle the error of the result may be different.
// 3. These non-corresponding cartesian coordinates are used in calculation,
//    e.g. multiplied several times in cross and dot products.
// If it was a problem this strategy could e.g. "normalize" longitudes before the conversion using the source units
// by rotating the globe around Z axis, so moving longitudes always the same way towards the origin,
// assuming this could help which is not clear.
// For now, intersection points near the endpoints are checked explicitly if needed (if the IP is near the endpoint)
// to generate precise result for them. Only the crossing (i) case may suffer from lower precision.

template
<
    typename CalcPolicy,
    typename CalculationType = void
>
struct ecef_segments
{
    typedef spherical_tag cs_tag;

    enum intersection_point_flag { ipi_inters = 0, ipi_at_a1, ipi_at_a2, ipi_at_b1, ipi_at_b2 };

    // segment_intersection_info cannot outlive relate_ecef_segments
    template <typename CoordinateType, typename SegmentRatio, typename Vector3d>
    struct segment_intersection_info
    {
        segment_intersection_info(CalcPolicy const& calc)
            : calc_policy(calc)
        {}

        template <typename Point, typename Segment1, typename Segment2>
        void calculate(Point& point, Segment1 const& a, Segment2 const& b) const
        {
            if (ip_flag == ipi_inters)
            {
                // TODO: assign the rest of coordinates
                point = calc_policy.template from_cart3d<Point>(intersection_point);
            }
            else if (ip_flag == ipi_at_a1)
            {
                detail::assign_point_from_index<0>(a, point);
            }
            else if (ip_flag == ipi_at_a2)
            {
                detail::assign_point_from_index<1>(a, point);
            }
            else if (ip_flag == ipi_at_b1)
            {
                detail::assign_point_from_index<0>(b, point);
            }
            else // ip_flag == ipi_at_b2
            {
                detail::assign_point_from_index<1>(b, point);
            }
        }

        Vector3d intersection_point;
        SegmentRatio robust_ra;
        SegmentRatio robust_rb;
        intersection_point_flag ip_flag;

        CalcPolicy const& calc_policy;
    };

    // Relate segments a and b
    template
    <
        typename UniqueSubRange1,
        typename UniqueSubRange2,
        typename Policy
    >
    static inline typename Policy::return_type
        apply(UniqueSubRange1 const& range_p, UniqueSubRange2 const& range_q,
              Policy const&)
    {
        // For now create it using default constructor. In the future it could
        //  be stored in strategy. However then apply() wouldn't be static and
        //  all relops and setops would have to take the strategy or model.
        // Initialize explicitly to prevent compiler errors in case of PoD type
        CalcPolicy const calc_policy = CalcPolicy();

        typedef typename UniqueSubRange1::point_type point1_type;
        typedef typename UniqueSubRange2::point_type point2_type;

        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point1_type>) );
        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point2_type>) );

        point1_type const& a1 = range_p.at(0);
        point1_type const& a2 = range_p.at(1);
        point2_type const& b1 = range_q.at(0);
        point2_type const& b2 = range_q.at(1);

        typedef model::referring_segment<point1_type const> segment1_type;
        typedef model::referring_segment<point2_type const> segment2_type;
        segment1_type const a(a1, a2);
        segment2_type const b(b1, b2);

        // TODO: check only 2 first coordinates here?
        bool a_is_point = equals_point_point(a1, a2);
        bool b_is_point = equals_point_point(b1, b2);

        if(a_is_point && b_is_point)
        {
            return equals_point_point(a1, b2)
                ? Policy::degenerate(a, true)
                : Policy::disjoint()
                ;
        }

        typedef typename select_calculation_type
            <segment1_type, segment2_type, CalculationType>::type calc_t;

        calc_t const c0 = 0;
        calc_t const c1 = 1;

        typedef model::point<calc_t, 3, cs::cartesian> vec3d_t;

        vec3d_t const a1v = calc_policy.template to_cart3d<vec3d_t>(a1);
        vec3d_t const a2v = calc_policy.template to_cart3d<vec3d_t>(a2);
        vec3d_t const b1v = calc_policy.template to_cart3d<vec3d_t>(b1);
        vec3d_t const b2v = calc_policy.template to_cart3d<vec3d_t>(b2);
        
        bool degen_neq_coords = false;
        side_info sides;

        typename CalcPolicy::template plane<vec3d_t>
            plane2 = calc_policy.get_plane(b1v, b2v);

        calc_t dist_b1_b2 = 0;
        if (! b_is_point)
        {
            calculate_dist(b1v, b2v, plane2, dist_b1_b2);
            if (math::equals(dist_b1_b2, c0))
            {
                degen_neq_coords = true;
                b_is_point = true;
                dist_b1_b2 = 0;
            }
            else
            {
                // not normalized normals, the same as in side strategy
                sides.set<0>(plane2.side_value(a1v), plane2.side_value(a2v));
                if (sides.same<0>())
                {
                    // Both points are at same side of other segment, we can leave
                    return Policy::disjoint();
                }
            }
        }

        typename CalcPolicy::template plane<vec3d_t>
            plane1 = calc_policy.get_plane(a1v, a2v);

        calc_t dist_a1_a2 = 0;
        if (! a_is_point)
        {
            calculate_dist(a1v, a2v, plane1, dist_a1_a2);
            if (math::equals(dist_a1_a2, c0))
            {
                degen_neq_coords = true;
                a_is_point = true;
                dist_a1_a2 = 0;
            }
            else
            {
                // not normalized normals, the same as in side strategy
                sides.set<1>(plane1.side_value(b1v), plane1.side_value(b2v));
                if (sides.same<1>())
                {
                    // Both points are at same side of other segment, we can leave
                    return Policy::disjoint();
                }
            }
        }

        // NOTE: at this point the segments may still be disjoint

        calc_t len1 = 0;
        // point or opposite sides of a sphere/spheroid, assume point
        if (! a_is_point && ! detail::vec_normalize(plane1.normal, len1))
        {
            a_is_point = true;
            if (sides.get<0, 0>() == 0 || sides.get<0, 1>() == 0)
            {
                sides.set<0>(0, 0);
            }
        }

        calc_t len2 = 0;
        if (! b_is_point && ! detail::vec_normalize(plane2.normal, len2))
        {
            b_is_point = true;
            if (sides.get<1, 0>() == 0 || sides.get<1, 1>() == 0)
            {
                sides.set<1>(0, 0);
            }
        }

        // check both degenerated once more
        if (a_is_point && b_is_point)
        {
            return equals_point_point(a1, b2)
                ? Policy::degenerate(a, true)
                : Policy::disjoint()
                ;
        }

        // NOTE: at this point the segments may still be disjoint
        // NOTE: at this point one of the segments may be degenerated

        bool collinear = sides.collinear();       

        if (! collinear)
        {
            // NOTE: for some approximations it's possible that both points may lie
            // on the same geodesic but still some of the sides may be != 0.
            // This is e.g. true for long segments represented as elliptic arcs
            // with origin different than the center of the coordinate system.
            // So make the sides consistent

            // WARNING: the side strategy doesn't have the info about the other
            // segment so it may return results inconsistent with this intersection
            // strategy, as it checks both segments for consistency

            if (sides.get<0, 0>() == 0 && sides.get<0, 1>() == 0)
            {
                collinear = true;
                sides.set<1>(0, 0);
            }
            else if (sides.get<1, 0>() == 0 && sides.get<1, 1>() == 0)
            {
                collinear = true;
                sides.set<0>(0, 0);
            }
        }

        calc_t dot_n1n2 = dot_product(plane1.normal, plane2.normal);

        // NOTE: this is technically not needed since theoretically above sides
        //       are calculated, but just in case check the normals.
        //       Have in mind that SSF side strategy doesn't check this.
        // collinear if normals are equal or opposite: cos(a) in {-1, 1}
        if (! collinear && math::equals(math::abs(dot_n1n2), c1))
        {
            collinear = true;
            sides.set<0>(0, 0);
            sides.set<1>(0, 0);
        }
        
        if (collinear)
        {
            if (a_is_point)
            {
                return collinear_one_degenerated<Policy, calc_t>(a, true, b1, b2, a1, a2, b1v, b2v,
                                                                 plane2, a1v, a2v, dist_b1_b2, degen_neq_coords);
            }
            else if (b_is_point)
            {
                // b2 used to be consistent with (degenerated) checks above (is it needed?)
                return collinear_one_degenerated<Policy, calc_t>(b, false, a1, a2, b1, b2, a1v, a2v,
                                                                 plane1, b1v, b2v, dist_a1_a2, degen_neq_coords);
            }
            else
            {
                calc_t dist_a1_b1, dist_a1_b2;
                calc_t dist_b1_a1, dist_b1_a2;
                calculate_collinear_data(a1, a2, b1, b2, a1v, a2v, plane1, b1v, b2v, dist_a1_a2, dist_a1_b1);
                calculate_collinear_data(a1, a2, b2, b1, a1v, a2v, plane1, b2v, b1v, dist_a1_a2, dist_a1_b2);
                calculate_collinear_data(b1, b2, a1, a2, b1v, b2v, plane2, a1v, a2v, dist_b1_b2, dist_b1_a1);
                calculate_collinear_data(b1, b2, a2, a1, b1v, b2v, plane2, a2v, a1v, dist_b1_b2, dist_b1_a2);
                // NOTE: The following optimization causes problems with consitency
                // It may either be caused by numerical issues or the way how distance is coded:
                //   as cosine of angle scaled and translated, see: calculate_dist()
                /*dist_b1_b2 = dist_a1_b2 - dist_a1_b1;
                dist_b1_a1 = -dist_a1_b1;
                dist_b1_a2 = dist_a1_a2 - dist_a1_b1;
                dist_a1_a2 = dist_b1_a2 - dist_b1_a1;
                dist_a1_b1 = -dist_b1_a1;
                dist_a1_b2 = dist_b1_b2 - dist_b1_a1;*/

                segment_ratio<calc_t> ra_from(dist_b1_a1, dist_b1_b2);
                segment_ratio<calc_t> ra_to(dist_b1_a2, dist_b1_b2);
                segment_ratio<calc_t> rb_from(dist_a1_b1, dist_a1_a2);
                segment_ratio<calc_t> rb_to(dist_a1_b2, dist_a1_a2);
                
                // NOTE: this is probably not needed
                int const a1_wrt_b = position_value(c0, dist_a1_b1, dist_a1_b2);
                int const a2_wrt_b = position_value(dist_a1_a2, dist_a1_b1, dist_a1_b2);
                int const b1_wrt_a = position_value(c0, dist_b1_a1, dist_b1_a2);
                int const b2_wrt_a = position_value(dist_b1_b2, dist_b1_a1, dist_b1_a2);

                if (a1_wrt_b == 1)
                {
                    ra_from.assign(0, dist_b1_b2);
                    rb_from.assign(0, dist_a1_a2);
                }
                else if (a1_wrt_b == 3)
                {
                    ra_from.assign(dist_b1_b2, dist_b1_b2);
                    rb_to.assign(0, dist_a1_a2);
                }

                if (a2_wrt_b == 1)
                {
                    ra_to.assign(0, dist_b1_b2);
                    rb_from.assign(dist_a1_a2, dist_a1_a2);
                }
                else if (a2_wrt_b == 3)
                {
                    ra_to.assign(dist_b1_b2, dist_b1_b2);
                    rb_to.assign(dist_a1_a2, dist_a1_a2);
                }

                if ((a1_wrt_b < 1 && a2_wrt_b < 1) || (a1_wrt_b > 3 && a2_wrt_b > 3))
                {
                    return Policy::disjoint();
                }

                bool const opposite = dot_n1n2 < c0;

                return Policy::segments_collinear(a, b, opposite,
                    a1_wrt_b, a2_wrt_b, b1_wrt_a, b2_wrt_a,
                    ra_from, ra_to, rb_from, rb_to);
            }
        }
        else // crossing
        {
            if (a_is_point || b_is_point)
            {
                return Policy::disjoint();
            }

            vec3d_t i1;
            intersection_point_flag ip_flag;
            calc_t dist_a1_i1, dist_b1_i1;
            if (calculate_ip_data(a1, a2, b1, b2, a1v, a2v, b1v, b2v,
                                  plane1, plane2, calc_policy,
                                  sides, dist_a1_a2, dist_b1_b2,
                                  i1, dist_a1_i1, dist_b1_i1, ip_flag))
            {
                // intersects
                segment_intersection_info
                    <
                        calc_t,
                        segment_ratio<calc_t>,
                        vec3d_t
                    > sinfo(calc_policy);

                sinfo.robust_ra.assign(dist_a1_i1, dist_a1_a2);
                sinfo.robust_rb.assign(dist_b1_i1, dist_b1_b2);
                sinfo.intersection_point = i1;
                sinfo.ip_flag = ip_flag;

                return Policy::segments_crosses(sides, sinfo, a, b);
            }
            else
            {
                return Policy::disjoint();
            }
        }
    }

private:
    template <typename Policy, typename CalcT, typename Segment, typename Point1, typename Point2, typename Vec3d, typename Plane>
    static inline typename Policy::return_type
        collinear_one_degenerated(Segment const& segment, bool degenerated_a,
                                  Point1 const& a1, Point1 const& a2,
                                  Point2 const& b1, Point2 const& b2,
                                  Vec3d const& a1v, Vec3d const& a2v,
                                  Plane const& plane,
                                  Vec3d const& b1v, Vec3d const& b2v,
                                  CalcT const& dist_1_2,
                                  bool degen_neq_coords)
    {
        CalcT dist_1_o;
        return ! calculate_collinear_data(a1, a2, b1, b2, a1v, a2v, plane, b1v, b2v, dist_1_2, dist_1_o, degen_neq_coords)
                ? Policy::disjoint()
                : Policy::one_degenerate(segment, segment_ratio<CalcT>(dist_1_o, dist_1_2), degenerated_a);
    }

    template <typename Point1, typename Point2, typename Vec3d, typename Plane, typename CalcT>
    static inline bool calculate_collinear_data(Point1 const& a1, Point1 const& a2, // in
                                                Point2 const& b1, Point2 const& /*b2*/, // in
                                                Vec3d const& a1v,                   // in
                                                Vec3d const& a2v,                   // in
                                                Plane const& plane1,                // in
                                                Vec3d const& b1v,                   // in
                                                Vec3d const& b2v,                   // in
                                                CalcT const& dist_a1_a2,            // in
                                                CalcT& dist_a1_b1,                  // out
                                                bool degen_neq_coords = false)      // in
    {
        // calculate dist_a1_b1
        calculate_dist(a1v, a2v, plane1, b1v, dist_a1_b1);

        // if b1 is equal to a1
        if (is_endpoint_equal(dist_a1_b1, a1, b1))
        {
            dist_a1_b1 = 0;
            return true;
        }
        // or b1 is equal to a2
        else if (is_endpoint_equal(dist_a1_a2 - dist_a1_b1, a2, b1))
        {
            dist_a1_b1 = dist_a1_a2;
            return true;
        }

        // check the other endpoint of degenerated segment near a pole
        if (degen_neq_coords)
        {
            static CalcT const c0 = 0;

            CalcT dist_a1_b2 = 0;
            calculate_dist(a1v, a2v, plane1, b2v, dist_a1_b2);

            if (math::equals(dist_a1_b2, c0))
            {
                dist_a1_b1 = 0;
                return true;
            }
            else if (math::equals(dist_a1_a2 - dist_a1_b2, c0))
            {
                dist_a1_b1 = dist_a1_a2;
                return true;
            }
        }

        // or i1 is on b
        return segment_ratio<CalcT>(dist_a1_b1, dist_a1_a2).on_segment();
    }

    template <typename Point1, typename Point2, typename Vec3d, typename Plane, typename CalcT>
    static inline bool calculate_ip_data(Point1 const& a1, Point1 const& a2, // in
                                         Point2 const& b1, Point2 const& b2, // in
                                         Vec3d const& a1v, Vec3d const& a2v, // in
                                         Vec3d const& b1v, Vec3d const& b2v, // in
                                         Plane const& plane1,                // in
                                         Plane const& plane2,                // in
                                         CalcPolicy const& calc_policy,      // in
                                         side_info const& sides,             // in
                                         CalcT const& dist_a1_a2,            // in
                                         CalcT const& dist_b1_b2,            // in
                                         Vec3d & ip,                         // out
                                         CalcT& dist_a1_ip,                  // out
                                         CalcT& dist_b1_ip,                  // out
                                         intersection_point_flag& ip_flag)   // out
    {
        Vec3d ip1, ip2;
        calc_policy.intersection_points(plane1, plane2, ip1, ip2);
        
        calculate_dist(a1v, a2v, plane1, ip1, dist_a1_ip);
        ip = ip1;

        // choose the opposite side of the globe if the distance is shorter
        {
            CalcT const d = abs_distance(dist_a1_a2, dist_a1_ip);
            if (d > CalcT(0))
            {
                // TODO: this should be ok not only for sphere
                //       but requires more investigation
                CalcT const dist_a1_i2 = dist_of_i2(dist_a1_ip);
                CalcT const d2 = abs_distance(dist_a1_a2, dist_a1_i2);
                if (d2 < d)
                {
                    dist_a1_ip = dist_a1_i2;
                    ip = ip2;
                }
            }
        }

        bool is_on_a = false, is_near_a1 = false, is_near_a2 = false;
        if (! is_potentially_crossing(dist_a1_a2, dist_a1_ip, is_on_a, is_near_a1, is_near_a2))
        {
            return false;
        }

        calculate_dist(b1v, b2v, plane2, ip, dist_b1_ip);

        bool is_on_b = false, is_near_b1 = false, is_near_b2 = false;
        if (! is_potentially_crossing(dist_b1_b2, dist_b1_ip, is_on_b, is_near_b1, is_near_b2))
        {
            return false;
        }

        // reassign the IP if some endpoints overlap
        if (is_near_a1)
        {
            if (is_near_b1 && equals_point_point(a1, b1))
            {
                dist_a1_ip = 0;
                dist_b1_ip = 0;
                //i1 = a1v;
                ip_flag = ipi_at_a1;
                return true;
            }
            
            if (is_near_b2 && equals_point_point(a1, b2))
            {
                dist_a1_ip = 0;
                dist_b1_ip = dist_b1_b2;
                //i1 = a1v;
                ip_flag = ipi_at_a1;
                return true;
            }
        }

        if (is_near_a2)
        {
            if (is_near_b1 && equals_point_point(a2, b1))
            {
                dist_a1_ip = dist_a1_a2;
                dist_b1_ip = 0;
                //i1 = a2v;
                ip_flag = ipi_at_a2;
                return true;
            }

            if (is_near_b2 && equals_point_point(a2, b2))
            {
                dist_a1_ip = dist_a1_a2;
                dist_b1_ip = dist_b1_b2;
                //i1 = a2v;
                ip_flag = ipi_at_a2;
                return true;
            }
        }

        // at this point we know that the endpoints doesn't overlap
        // reassign IP and distance if the IP is on a segment and one of
        //   the endpoints of the other segment lies on the former segment
        if (is_on_a)
        {
            if (is_near_b1 && sides.template get<1, 0>() == 0) // b1 wrt a
            {
                calculate_dist(a1v, a2v, plane1, b1v, dist_a1_ip); // for consistency
                dist_b1_ip = 0;
                //i1 = b1v;
                ip_flag = ipi_at_b1;
                return true;
            }

            if (is_near_b2 && sides.template get<1, 1>() == 0) // b2 wrt a
            {
                calculate_dist(a1v, a2v, plane1, b2v, dist_a1_ip); // for consistency
                dist_b1_ip = dist_b1_b2;
                //i1 = b2v;
                ip_flag = ipi_at_b2;
                return true;
            }
        }

        if (is_on_b)
        {
            if (is_near_a1 && sides.template get<0, 0>() == 0) // a1 wrt b
            {
                dist_a1_ip = 0;
                calculate_dist(b1v, b2v, plane2, a1v, dist_b1_ip); // for consistency
                //i1 = a1v;
                ip_flag = ipi_at_a1;
                return true;
            }

            if (is_near_a2 && sides.template get<0, 1>() == 0) // a2 wrt b
            {
                dist_a1_ip = dist_a1_a2;
                calculate_dist(b1v, b2v, plane2, a2v, dist_b1_ip); // for consistency
                //i1 = a2v;
                ip_flag = ipi_at_a2;
                return true;
            }
        }

        ip_flag = ipi_inters;

        return is_on_a && is_on_b;
    }

    template <typename Vec3d, typename Plane, typename CalcT>
    static inline void calculate_dist(Vec3d const& a1v,    // in
                                      Vec3d const& a2v,    // in
                                      Plane const& plane1, // in
                                      CalcT& dist_a1_a2)   // out
    {
        static CalcT const c1 = 1;
        CalcT const cos_a1_a2 = plane1.cos_angle_between(a1v, a2v);
        dist_a1_a2 = -cos_a1_a2 + c1; // [1, -1] -> [0, 2] representing [0, pi]
    }

    template <typename Vec3d, typename Plane, typename CalcT>
    static inline void calculate_dist(Vec3d const& a1v,     // in
                                      Vec3d const& /*a2v*/, // in
                                      Plane const& plane1,  // in
                                      Vec3d const& i1,      // in
                                      CalcT& dist_a1_i1)    // out
    {
        static CalcT const c1 = 1;
        static CalcT const c2 = 2;
        static CalcT const c4 = 4;

        bool is_forward = true;
        CalcT cos_a1_i1 = plane1.cos_angle_between(a1v, i1, is_forward);
        dist_a1_i1 = -cos_a1_i1 + c1; // [0, 2] representing [0, pi]
        if (! is_forward) // left or right of a1 on a
        {
            dist_a1_i1 = -dist_a1_i1; // [0, 2] -> [0, -2] representing [0, -pi]
        }
        if (dist_a1_i1 <= -c2) // <= -pi
        {
            dist_a1_i1 += c4; // += 2pi
        }
    }
    /*
    template <typename Vec3d, typename Plane, typename CalcT>
    static inline void calculate_dists(Vec3d const& a1v,    // in
                                       Vec3d const& a2v,    // in
                                       Plane const& plane1, // in
                                       Vec3d const& i1,     // in
                                       CalcT& dist_a1_a2, // out
                                       CalcT& dist_a1_i1) // out
    {
        calculate_dist(a1v, a2v, plane1, dist_a1_a2);
        calculate_dist(a1v, a2v, plane1, i1, dist_a1_i1);
    }
    */
    // the dist of the ip on the other side of the sphere
    template <typename CalcT>
    static inline CalcT dist_of_i2(CalcT const& dist_a1_i1)
    {
        CalcT const c2 = 2;
        CalcT const c4 = 4;

        CalcT dist_a1_i2 = dist_a1_i1 - c2; // dist_a1_i2 = dist_a1_i1 - pi;
        if (dist_a1_i2 <= -c2)          // <= -pi
        {
            dist_a1_i2 += c4;           // += 2pi;
        }
        return dist_a1_i2;
    }

    template <typename CalcT>
    static inline CalcT abs_distance(CalcT const& dist_a1_a2, CalcT const& dist_a1_i1)
    {
        if (dist_a1_i1 < CalcT(0))
            return -dist_a1_i1;
        else if (dist_a1_i1 > dist_a1_a2)
            return dist_a1_i1 - dist_a1_a2;
        else
            return CalcT(0);
    }

    template <typename CalcT>
    static inline bool is_potentially_crossing(CalcT const& dist_a1_a2, CalcT const& dist_a1_i1, // in
                                               bool& is_on_a, bool& is_near_a1, bool& is_near_a2) // out
    {
        is_on_a = segment_ratio<CalcT>(dist_a1_i1, dist_a1_a2).on_segment();
        is_near_a1 = is_near(dist_a1_i1);
        is_near_a2 = is_near(dist_a1_a2 - dist_a1_i1);
        return is_on_a || is_near_a1 || is_near_a2;
    }

    template <typename CalcT, typename P1, typename P2>
    static inline bool is_endpoint_equal(CalcT const& dist,
                                         P1 const& ai, P2 const& b1)
    {
        static CalcT const c0 = 0;
        return is_near(dist) && (math::equals(dist, c0) || equals_point_point(ai, b1));
    }

    template <typename CalcT>
    static inline bool is_near(CalcT const& dist)
    {
        CalcT const small_number = CalcT(std::is_same<CalcT, float>::value ? 0.0001 : 0.00000001);
        return math::abs(dist) <= small_number;
    }

    template <typename ProjCoord1, typename ProjCoord2>
    static inline int position_value(ProjCoord1 const& ca1,
                                     ProjCoord2 const& cb1,
                                     ProjCoord2 const& cb2)
    {
        // S1x  0   1    2     3   4
        // S2       |---------->
        return math::equals(ca1, cb1) ? 1
             : math::equals(ca1, cb2) ? 3
             : cb1 < cb2 ?
                ( ca1 < cb1 ? 0
                : ca1 > cb2 ? 4
                : 2 )
              : ( ca1 > cb1 ? 0
                : ca1 < cb2 ? 4
                : 2 );
    }

    template <typename Point1, typename Point2>
    static inline bool equals_point_point(Point1 const& point1, Point2 const& point2)
    {
        return strategy::within::spherical_point_point::apply(point1, point2);
    }
};

struct spherical_segments_calc_policy
{
    template <typename Point, typename Point3d>
    static Point from_cart3d(Point3d const& point_3d)
    {
        return formula::cart3d_to_sph<Point>(point_3d);
    }

    template <typename Point3d, typename Point>
    static Point3d to_cart3d(Point const& point)
    {
        return formula::sph_to_cart3d<Point3d>(point);
    }

    template <typename Point3d>
    struct plane
    {
        typedef typename coordinate_type<Point3d>::type coord_t;

        // not normalized
        plane(Point3d const& p1, Point3d const& p2)
            : normal(cross_product(p1, p2))
        {}

        int side_value(Point3d const& pt) const
        {
            return formula::sph_side_value(normal, pt);
        }

        static coord_t cos_angle_between(Point3d const& p1, Point3d const& p2)
        {
            return dot_product(p1, p2);
        }

        coord_t cos_angle_between(Point3d const& p1, Point3d const& p2, bool & is_forward) const
        {
            coord_t const c0 = 0;
            is_forward = dot_product(normal, cross_product(p1, p2)) >= c0;
            return dot_product(p1, p2);
        }

        Point3d normal;
    };

    template <typename Point3d>
    static plane<Point3d> get_plane(Point3d const& p1, Point3d const& p2)
    {
        return plane<Point3d>(p1, p2);
    }

    template <typename Point3d>
    static bool intersection_points(plane<Point3d> const& plane1,
                                    plane<Point3d> const& plane2,
                                    Point3d & ip1, Point3d & ip2)
    {
        typedef typename coordinate_type<Point3d>::type coord_t;

        ip1 = cross_product(plane1.normal, plane2.normal);
        // NOTE: the length should be greater than 0 at this point
        //       if the normals were not normalized and their dot product
        //       not checked before this function is called the length
        //       should be checked here (math::equals(len, c0))
        coord_t const len = math::sqrt(dot_product(ip1, ip1));
        geometry::detail::for_each_dimension<Point3d>([&](auto index)
        {
            coord_t const coord = get<index>(ip1) / len; // normalize
            set<index>(ip1, coord);
            set<index>(ip2, -coord);
        });

        return true;
    }    
};


template
<
    typename CalculationType = void
>
struct spherical_segments
    : ecef_segments
        <
            spherical_segments_calc_policy,
            CalculationType
        >
{};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

/*template <typename CalculationType>
struct default_strategy<spherical_polar_tag, CalculationType>
{
    typedef spherical_segments<CalculationType> type;
};*/

template <typename CalculationType>
struct default_strategy<spherical_equatorial_tag, CalculationType>
{
    typedef spherical_segments<CalculationType> type;
};

template <typename CalculationType>
struct default_strategy<geographic_tag, CalculationType>
{
    // NOTE: Spherical strategy returns the same result as the geographic one
    // representing segments as great elliptic arcs. If the elliptic arcs are
    // not great elliptic arcs (the origin not in the center of the coordinate
    // system) then there may be problems with consistency of the side and
    // intersection strategies.
    typedef spherical_segments<CalculationType> type;
};

} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::intersection


namespace strategy
{

namespace within { namespace services
{

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

}} // within::services

namespace covered_by { namespace services
{

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef strategy::intersection::spherical_segments<> type;
};

}} // within::services

} // strategy


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_INTERSECTION_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2021.
// Modifications copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP

#include <algorithm>

#include <boost/geometry/core/exception.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_integral.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategy/cartesian/area.hpp>
#include <boost/geometry/strategy/cartesian/envelope.hpp>
#include <boost/geometry/strategy/cartesian/expand_box.hpp>
#include <boost/geometry/strategy/cartesian/expand_segment.hpp>

#include <boost/geometry/strategies/cartesian/disjoint_box_box.hpp>
#include <boost/geometry/strategies/cartesian/disjoint_segment_box.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/cartesian/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/intersection.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/strategies/within.hpp>

#include <boost/geometry/policies/robustness/rescale_policy_tags.hpp>
#include <boost/geometry/policies/robustness/robust_point_type.hpp>


#if defined(BOOST_GEOMETRY_DEBUG_ROBUSTNESS)
#  include <boost/geometry/io/wkt/write.hpp>
#endif


namespace boost { namespace geometry
{


namespace strategy { namespace intersection
{


/*!
    \see http://mathworld.wolfram.com/Line-LineIntersection.html
 */
template
<
    typename CalculationType = void
>
struct cartesian_segments
{
    typedef cartesian_tag cs_tag;

    template <typename CoordinateType, typename SegmentRatio>
    struct segment_intersection_info
    {
    private :
        typedef typename select_most_precise
            <
                CoordinateType, double
            >::type promoted_type;

        promoted_type comparable_length_a() const
        {
            return dx_a * dx_a + dy_a * dy_a;
        }

        promoted_type comparable_length_b() const
        {
            return dx_b * dx_b + dy_b * dy_b;
        }

        template <typename Point, typename Segment1, typename Segment2>
        void assign_a(Point& point, Segment1 const& a, Segment2 const& ) const
        {
            assign(point, a, dx_a, dy_a, robust_ra);
        }
        template <typename Point, typename Segment1, typename Segment2>
        void assign_b(Point& point, Segment1 const& , Segment2 const& b) const
        {
            assign(point, b, dx_b, dy_b, robust_rb);
        }

        template <typename Point, typename Segment>
        void assign(Point& point, Segment const& segment,
                    CoordinateType const& dx, CoordinateType const& dy,
                    SegmentRatio const& ratio) const
        {
            // Calculate the intersection point based on segment_ratio
            // Up to now, division was postponed. Here we divide using numerator/
            // denominator. In case of integer this results in an integer
            // division.
            BOOST_GEOMETRY_ASSERT(ratio.denominator() != 0);

            typedef typename promote_integral<CoordinateType>::type calc_type;

            calc_type const numerator
                = boost::numeric_cast<calc_type>(ratio.numerator());
            calc_type const denominator
                = boost::numeric_cast<calc_type>(ratio.denominator());
            calc_type const dx_calc = boost::numeric_cast<calc_type>(dx);
            calc_type const dy_calc = boost::numeric_cast<calc_type>(dy);

            set<0>(point, get<0, 0>(segment)
                   + boost::numeric_cast<CoordinateType>(numerator * dx_calc
                                                         / denominator));
            set<1>(point, get<0, 1>(segment)
                   + boost::numeric_cast<CoordinateType>(numerator * dy_calc
                                                         / denominator));
        }

        template <int Index, int Dim, typename Point, typename Segment>
        static bool exceeds_side_in_dimension(Point& p, Segment const& s)
        {
            // Situation a (positive)
            //     0>-------------->1     segment
            // *                          point left of segment<I> in D x or y
            // Situation b (negative)
            //     1<--------------<0     segment
            // *                          point right of segment<I>
            // Situation c (degenerate), return false (check other dimension)
            auto const& c = get<Dim>(p);
            auto const& c0 = get<Index, Dim>(s);
            auto const& c1 = get<1 - Index, Dim>(s);
            return c0 < c1 ? math::smaller(c, c0)
                 : c0 > c1 ? math::larger(c, c0)
                 : false;
        }

        template <int Index, typename Point, typename Segment>
        static bool exceeds_side_of_segment(Point& p, Segment const& s)
        {
            return exceeds_side_in_dimension<Index, 0>(p, s)
                || exceeds_side_in_dimension<Index, 1>(p, s);
        }

        template <typename Point, typename Segment>
        static void assign_if_exceeds(Point& point, Segment const& s)
        {
            if (exceeds_side_of_segment<0>(point, s))
            {
                detail::assign_point_from_index<0>(s, point);
            }
            else if (exceeds_side_of_segment<1>(point, s))
            {
                detail::assign_point_from_index<1>(s, point);
            }
        }

    public :
        template <typename Point, typename Segment1, typename Segment2>
        void calculate(Point& point, Segment1 const& a, Segment2 const& b) const
        {
            bool use_a = true;

            // Prefer one segment if one is on or near an endpoint
            bool const a_near_end = robust_ra.near_end();
            bool const b_near_end = robust_rb.near_end();
            if (a_near_end && ! b_near_end)
            {
                use_a = true;
            }
            else if (b_near_end && ! a_near_end)
            {
                use_a = false;
            }
            else
            {
                // Prefer shorter segment
                promoted_type const len_a = comparable_length_a();
                promoted_type const len_b = comparable_length_b();
                if (len_b < len_a)
                {
                    use_a = false;
                }
                // else use_a is true but was already assigned like that
            }

            if (use_a)
            {
                assign_a(point, a, b);
            }
            else
            {
                assign_b(point, a, b);
            }

#if defined(BOOST_GEOMETRY_USE_RESCALING)
            return;
#endif

            // Verify nearly collinear cases (the threshold is arbitrary
            // but influences performance). If the intersection is located
            // outside the segments, then it should be moved.
            if (robust_ra.possibly_collinear(1.0e-3)
                && robust_rb.possibly_collinear(1.0e-3))
            {
                // The segments are nearly collinear and because of the calculation
                // method with very small denominator, the IP appears outside the
                // segment(s). Correct it to the end point.
                // Because they are nearly collinear, it doesn't really matter to
                // to which endpoint (or it is corrected twice).
                assign_if_exceeds(point, a);
                assign_if_exceeds(point, b);
            }
        }

        CoordinateType dx_a, dy_a;
        CoordinateType dx_b, dy_b;
        SegmentRatio robust_ra;
        SegmentRatio robust_rb;
    };

    template <typename D, typename W, typename ResultType>
    static inline void cramers_rule(D const& dx_a, D const& dy_a,
        D const& dx_b, D const& dy_b, W const& wx, W const& wy,
        // out:
        ResultType& nominator, ResultType& denominator)
    {
        // Cramers rule
        nominator = geometry::detail::determinant<ResultType>(dx_b, dy_b, wx, wy);
        denominator = geometry::detail::determinant<ResultType>(dx_a, dy_a, dx_b, dy_b);
        // Ratio r = nominator/denominator
        // Collinear if denominator == 0, intersecting if 0 <= r <= 1
        // IntersectionPoint = (x1 + r * dx_a, y1 + r * dy_a)
    }

    // Version for non-rescaled policies
    template
    <
        typename UniqueSubRange1,
        typename UniqueSubRange2,
        typename Policy
    >
    static inline typename Policy::return_type
        apply(UniqueSubRange1 const& range_p,
              UniqueSubRange2 const& range_q,
              Policy const& policy)
    {
        // Pass the same ranges both as normal ranges and as modelled ranges
        return apply(range_p, range_q, policy, range_p, range_q);
    }

    // Version for non rescaled versions.
    // The "modelled" parameter might be rescaled (will be removed later)
    template
    <
        typename UniqueSubRange1,
        typename UniqueSubRange2,
        typename Policy,
        typename ModelledUniqueSubRange1,
        typename ModelledUniqueSubRange2
    >
    static inline typename Policy::return_type
        apply(UniqueSubRange1 const& range_p,
              UniqueSubRange2 const& range_q,
              Policy const& policy,
              ModelledUniqueSubRange1 const& modelled_range_p,
              ModelledUniqueSubRange2 const& modelled_range_q)
    {
        typedef typename UniqueSubRange1::point_type point1_type;
        typedef typename UniqueSubRange2::point_type point2_type;

        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point1_type>) );
        BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<point2_type>) );

        point1_type const& p1 = range_p.at(0);
        point1_type const& p2 = range_p.at(1);
        point2_type const& q1 = range_q.at(0);
        point2_type const& q2 = range_q.at(1);

        // Declare segments, currently necessary for the policies
        // (segment_crosses, segment_colinear, degenerate, one_degenerate, etc)
        model::referring_segment<point1_type const> const p(p1, p2);
        model::referring_segment<point2_type const> const q(q1, q2);

        typedef typename select_most_precise
            <
                typename geometry::coordinate_type<typename ModelledUniqueSubRange1::point_type>::type,
                typename geometry::coordinate_type<typename ModelledUniqueSubRange1::point_type>::type
            >::type modelled_coordinate_type;

        typedef segment_ratio<modelled_coordinate_type> ratio_type;
        segment_intersection_info
            <
                typename select_calculation_type<point1_type, point2_type, CalculationType>::type,
                ratio_type
            > sinfo;

        sinfo.dx_a = get<0>(p2) - get<0>(p1); // distance in x-dir
        sinfo.dx_b = get<0>(q2) - get<0>(q1);
        sinfo.dy_a = get<1>(p2) - get<1>(p1); // distance in y-dir
        sinfo.dy_b = get<1>(q2) - get<1>(q1);

        return unified<ratio_type>(sinfo, p, q, policy, modelled_range_p, modelled_range_q);
    }

    //! Returns true if two segments do not overlap.
    //! If not, then no further calculations need to be done.
    template
    <
        std::size_t Dimension,
        typename PointP,
        typename PointQ
    >
    static inline bool disjoint_by_range(PointP const& p1, PointP const& p2,
                                         PointQ const& q1, PointQ const& q2)
    {
        auto minp = get<Dimension>(p1);
        auto maxp = get<Dimension>(p2);
        auto minq = get<Dimension>(q1);
        auto maxq = get<Dimension>(q2);
        if (minp > maxp)
        {
            std::swap(minp, maxp);
        }
        if (minq > maxq)
        {
            std::swap(minq, maxq);
        }

        // In this case, max(p) < min(q)
        //     P         Q
        // <-------> <------->
        // (and the space in between is not extremely small)
        return math::smaller(maxp, minq) || math::smaller(maxq, minp);
    }

    // Implementation for either rescaled or non rescaled versions.
    template
    <
        typename RatioType,
        typename SegmentInfo,
        typename Segment1,
        typename Segment2,
        typename Policy,
        typename UniqueSubRange1,
        typename UniqueSubRange2
    >
    static inline typename Policy::return_type
        unified(SegmentInfo& sinfo,
                Segment1 const& p, Segment2 const& q, Policy const&,
                UniqueSubRange1 const& range_p,
                UniqueSubRange2 const& range_q)
    {
        typedef typename UniqueSubRange1::point_type point1_type;
        typedef typename UniqueSubRange2::point_type point2_type;
        typedef typename select_most_precise
            <
                typename geometry::coordinate_type<point1_type>::type,
                typename geometry::coordinate_type<point2_type>::type
            >::type coordinate_type;

        point1_type const& p1 = range_p.at(0);
        point1_type const& p2 = range_p.at(1);
        point2_type const& q1 = range_q.at(0);
        point2_type const& q2 = range_q.at(1);

        bool const p_is_point = equals_point_point(p1, p2);
        bool const q_is_point = equals_point_point(q1, q2);

        if (p_is_point && q_is_point)
        {
            return equals_point_point(p1, q2)
                ? Policy::degenerate(p, true)
                : Policy::disjoint()
                ;
        }

        if (disjoint_by_range<0>(p1, p2, q1, q2)
         || disjoint_by_range<1>(p1, p2, q1, q2))
        {
            return Policy::disjoint();
        }

        typedef side::side_by_triangle<CalculationType> side_strategy_type;
        side_info sides;
        sides.set<0>(side_strategy_type::apply(q1, q2, p1),
                     side_strategy_type::apply(q1, q2, p2));

        if (sides.same<0>())
        {
            // Both points are at same side of other segment, we can leave
            return Policy::disjoint();
        }

        sides.set<1>(side_strategy_type::apply(p1, p2, q1),
                     side_strategy_type::apply(p1, p2, q2));
        
        if (sides.same<1>())
        {
            // Both points are at same side of other segment, we can leave
            return Policy::disjoint();
        }

        bool collinear = sides.collinear();

        // Calculate the differences again
        // (for rescaled version, this is different from dx_p etc)
        coordinate_type const dx_p = get<0>(p2) - get<0>(p1);
        coordinate_type const dx_q = get<0>(q2) - get<0>(q1);
        coordinate_type const dy_p = get<1>(p2) - get<1>(p1);
        coordinate_type const dy_q = get<1>(q2) - get<1>(q1);

        // r: ratio 0-1 where intersection divides A/B
        // (only calculated for non-collinear segments)
        if (! collinear)
        {
            coordinate_type denominator_a, nominator_a;
            coordinate_type denominator_b, nominator_b;

            cramers_rule(dx_p, dy_p, dx_q, dy_q,
                get<0>(p1) - get<0>(q1),
                get<1>(p1) - get<1>(q1),
                nominator_a, denominator_a);

            cramers_rule(dx_q, dy_q, dx_p, dy_p,
                get<0>(q1) - get<0>(p1),
                get<1>(q1) - get<1>(p1),
                nominator_b, denominator_b);

            math::detail::equals_factor_policy<coordinate_type>
                policy(dx_p, dy_p, dx_q, dy_q);

            coordinate_type const zero = 0;
            if (math::detail::equals_by_policy(denominator_a, zero, policy)
             || math::detail::equals_by_policy(denominator_b, zero, policy))
            {
                // If this is the case, no rescaling is done for FP precision.
                // We set it to collinear, but it indicates a robustness issue.
                sides.set<0>(0, 0);
                sides.set<1>(0, 0);
                collinear = true;
            }
            else
            {
                sinfo.robust_ra.assign(nominator_a, denominator_a);
                sinfo.robust_rb.assign(nominator_b, denominator_b);
            }
        }

        if (collinear)
        {
            std::pair<bool, bool> const collinear_use_first
                    = is_x_more_significant(geometry::math::abs(dx_p),
                                            geometry::math::abs(dy_p),
                                            geometry::math::abs(dx_q),
                                            geometry::math::abs(dy_q),
                                            p_is_point, q_is_point);

            if (collinear_use_first.second)
            {
                // Degenerate cases: segments of single point, lying on other segment, are not disjoint
                // This situation is collinear too

                if (collinear_use_first.first)
                {
                    return relate_collinear<0, Policy, RatioType>(p, q,
                            p1, p2, q1, q2,
                            p_is_point, q_is_point);
                }
                else
                {
                    // Y direction contains larger segments (maybe dx is zero)
                    return relate_collinear<1, Policy, RatioType>(p, q,
                            p1, p2, q1, q2,
                            p_is_point, q_is_point);
                }
            }
        }

        return Policy::segments_crosses(sides, sinfo, p, q);
    }

private:
    // first is true if x is more significant
    // second is true if the more significant difference is not 0
    template <typename CoordinateType>
    static inline std::pair<bool, bool>
        is_x_more_significant(CoordinateType const& abs_dx_a,
                              CoordinateType const& abs_dy_a,
                              CoordinateType const& abs_dx_b,
                              CoordinateType const& abs_dy_b,
                              bool const a_is_point,
                              bool const b_is_point)
    {
        //BOOST_GEOMETRY_ASSERT_MSG(!(a_is_point && b_is_point), "both segments shouldn't be degenerated");

        // for degenerated segments the second is always true because this function
        // shouldn't be called if both segments were degenerated

        if (a_is_point)
        {
            return std::make_pair(abs_dx_b >= abs_dy_b, true);
        }
        else if (b_is_point)
        {
            return std::make_pair(abs_dx_a >= abs_dy_a, true);
        }
        else
        {
            CoordinateType const min_dx = (std::min)(abs_dx_a, abs_dx_b);
            CoordinateType const min_dy = (std::min)(abs_dy_a, abs_dy_b);
            return min_dx == min_dy ?
                    std::make_pair(true, min_dx > CoordinateType(0)) :
                    std::make_pair(min_dx > min_dy, true);
        }
    }

    template
    <
        std::size_t Dimension,
        typename Policy,
        typename RatioType,
        typename Segment1,
        typename Segment2,
        typename RobustPoint1,
        typename RobustPoint2
    >
    static inline typename Policy::return_type
        relate_collinear(Segment1 const& a,
                         Segment2 const& b,
                         RobustPoint1 const& robust_a1, RobustPoint1 const& robust_a2,
                         RobustPoint2 const& robust_b1, RobustPoint2 const& robust_b2,
                         bool a_is_point, bool b_is_point)
    {
        if (a_is_point)
        {
            return relate_one_degenerate<Policy, RatioType>(a,
                get<Dimension>(robust_a1),
                get<Dimension>(robust_b1), get<Dimension>(robust_b2),
                true);
        }
        if (b_is_point)
        {
            return relate_one_degenerate<Policy, RatioType>(b,
                get<Dimension>(robust_b1),
                get<Dimension>(robust_a1), get<Dimension>(robust_a2),
                false);
        }
        return relate_collinear<Policy, RatioType>(a, b,
                                get<Dimension>(robust_a1),
                                get<Dimension>(robust_a2),
                                get<Dimension>(robust_b1),
                                get<Dimension>(robust_b2));
    }

    /// Relate segments known collinear
    template
    <
        typename Policy,
        typename RatioType,
        typename Segment1,
        typename Segment2,
        typename Type1,
        typename Type2
    >
    static inline typename Policy::return_type
        relate_collinear(Segment1 const& a, Segment2 const& b,
                         Type1 oa_1, Type1 oa_2,
                         Type2 ob_1, Type2 ob_2)
    {
        // Calculate the ratios where a starts in b, b starts in a
        //         a1--------->a2         (2..7)
        //                b1----->b2      (5..8)
        // length_a: 7-2=5
        // length_b: 8-5=3
        // b1 is located w.r.t. a at ratio: (5-2)/5=3/5 (on a)
        // b2 is located w.r.t. a at ratio: (8-2)/5=6/5 (right of a)
        // a1 is located w.r.t. b at ratio: (2-5)/3=-3/3 (left of b)
        // a2 is located w.r.t. b at ratio: (7-5)/3=2/3 (on b)
        // A arrives (a2 on b), B departs (b1 on a)

        // If both are reversed:
        //         a2<---------a1         (7..2)
        //                b2<-----b1      (8..5)
        // length_a: 2-7=-5
        // length_b: 5-8=-3
        // b1 is located w.r.t. a at ratio: (8-7)/-5=-1/5 (before a starts)
        // b2 is located w.r.t. a at ratio: (5-7)/-5=2/5 (on a)
        // a1 is located w.r.t. b at ratio: (7-8)/-3=1/3 (on b)
        // a2 is located w.r.t. b at ratio: (2-8)/-3=6/3 (after b ends)

        // If both one is reversed:
        //         a1--------->a2         (2..7)
        //                b2<-----b1      (8..5)
        // length_a: 7-2=+5
        // length_b: 5-8=-3
        // b1 is located w.r.t. a at ratio: (8-2)/5=6/5 (after a ends)
        // b2 is located w.r.t. a at ratio: (5-2)/5=3/5 (on a)
        // a1 is located w.r.t. b at ratio: (2-8)/-3=6/3 (after b ends)
        // a2 is located w.r.t. b at ratio: (7-8)/-3=1/3 (on b)
        Type1 const length_a = oa_2 - oa_1; // no abs, see above
        Type2 const length_b = ob_2 - ob_1;

        RatioType ra_from(oa_1 - ob_1, length_b);
        RatioType ra_to(oa_2 - ob_1, length_b);
        RatioType rb_from(ob_1 - oa_1, length_a);
        RatioType rb_to(ob_2 - oa_1, length_a);

        // use absolute measure to detect endpoints intersection
        // NOTE: it'd be possible to calculate bx_wrt_a using ax_wrt_b values
        int const a1_wrt_b = position_value(oa_1, ob_1, ob_2);
        int const a2_wrt_b = position_value(oa_2, ob_1, ob_2);
        int const b1_wrt_a = position_value(ob_1, oa_1, oa_2);
        int const b2_wrt_a = position_value(ob_2, oa_1, oa_2);
        
        // fix the ratios if necessary
        // CONSIDER: fixing ratios also in other cases, if they're inconsistent
        // e.g. if ratio == 1 or 0 (so IP at the endpoint)
        // but position value indicates that the IP is in the middle of the segment
        // because one of the segments is very long
        // In such case the ratios could be moved into the middle direction
        // by some small value (e.g. EPS+1ULP)
        if (a1_wrt_b == 1)
        {
            ra_from.assign(0, 1);
            rb_from.assign(0, 1);
        }
        else if (a1_wrt_b == 3)
        {
            ra_from.assign(1, 1);
            rb_to.assign(0, 1);
        } 

        if (a2_wrt_b == 1)
        {
            ra_to.assign(0, 1);
            rb_from.assign(1, 1);
        }
        else if (a2_wrt_b == 3)
        {
            ra_to.assign(1, 1);
            rb_to.assign(1, 1);
        }

        if ((a1_wrt_b < 1 && a2_wrt_b < 1) || (a1_wrt_b > 3 && a2_wrt_b > 3))
        //if ((ra_from.left() && ra_to.left()) || (ra_from.right() && ra_to.right()))
        {
            return Policy::disjoint();
        }

        bool const opposite = math::sign(length_a) != math::sign(length_b);

        return Policy::segments_collinear(a, b, opposite,
                                          a1_wrt_b, a2_wrt_b, b1_wrt_a, b2_wrt_a,
                                          ra_from, ra_to, rb_from, rb_to);
    }

    /// Relate segments where one is degenerate
    template
    <
        typename Policy,
        typename RatioType,
        typename DegenerateSegment,
        typename Type1,
        typename Type2
    >
    static inline typename Policy::return_type
        relate_one_degenerate(DegenerateSegment const& degenerate_segment,
                              Type1 d, Type2 s1, Type2 s2,
                              bool a_degenerate)
    {
        // Calculate the ratios where ds starts in s
        //         a1--------->a2         (2..6)
        //              b1/b2      (4..4)
        // Ratio: (4-2)/(6-2)
        RatioType const ratio(d - s1, s2 - s1);

        if (!ratio.on_segment())
        {
            return Policy::disjoint();
        }

        return Policy::one_degenerate(degenerate_segment, ratio, a_degenerate);
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
        return strategy::within::cartesian_point_point::apply(point1, point2);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType>
struct default_strategy<cartesian_tag, CalculationType>
{
    typedef cartesian_segments<CalculationType> type;
};

} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::intersection

namespace strategy
{

namespace within { namespace services
{

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

}} // within::services

namespace covered_by { namespace services
{

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, linear_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

template <typename Geometry1, typename Geometry2, typename AnyTag1, typename AnyTag2>
struct default_strategy<Geometry1, Geometry2, AnyTag1, AnyTag2, polygonal_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef strategy::intersection::cartesian_segments<> type;
};

}} // within::services

} // strategy

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP

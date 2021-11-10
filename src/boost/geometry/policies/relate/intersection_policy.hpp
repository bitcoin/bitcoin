// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2020 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POLICY_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POLICY_HPP


#include <string>
#include <tuple>

#include <boost/geometry/policies/relate/direction.hpp>
#include <boost/geometry/policies/relate/intersection_points.hpp>

#include <boost/geometry/strategies/side_info.hpp>

namespace boost { namespace geometry
{

namespace policies { namespace relate
{


template <typename IntersectionPointsReturnType>
struct segments_intersection_policy
{
private:
    typedef policies::relate::segments_intersection_points
        <
            IntersectionPointsReturnType
        > pts_policy;
    typedef policies::relate::segments_direction dir_policy;

public:
    struct return_type
    {
        typedef typename pts_policy::return_type intersection_points_type;
        typedef typename dir_policy::return_type direction_type;

        return_type(intersection_points_type const& pts_result,
                    direction_type const& dir_result)
            : intersection_points(pts_result)
            , direction(dir_result)
        {}

        intersection_points_type intersection_points;
        direction_type direction;
    };

    template <typename Segment1, typename Segment2, typename SegmentIntersectionInfo>
    static inline return_type segments_crosses(side_info const& sides,
                                        SegmentIntersectionInfo const& sinfo,
                                        Segment1 const& s1, Segment2 const& s2)
    {
        return return_type
            (
                pts_policy::segments_crosses(sides, sinfo, s1, s2),
                dir_policy::segments_crosses(sides, sinfo, s1, s2)
            );
    }

    template <typename Segment1, typename Segment2, typename Ratio>
    static inline return_type segments_collinear(
                                        Segment1 const& segment1,
                                        Segment2 const& segment2,
                                        bool opposite,
                                        int pa1, int pa2, int pb1, int pb2,
                                        Ratio const& ra1, Ratio const& ra2,
                                        Ratio const& rb1, Ratio const& rb2)
    {
        return return_type
            (
                pts_policy::segments_collinear(segment1, segment2,
                                               opposite,
                                               pa1, pa2, pb1, pb2,
                                               ra1, ra2, rb1, rb2),
                dir_policy::segments_collinear(segment1, segment2,
                                               opposite,
                                               pa1, pa2, pb1, pb2,
                                               ra1, ra2, rb1, rb2)
            );
    }

    template <typename Segment>
    static inline return_type degenerate(Segment const& segment,
                                         bool a_degenerate)
    {
        return return_type
            (
                pts_policy::degenerate(segment, a_degenerate),
                dir_policy::degenerate(segment, a_degenerate)
            );
    }

    template <typename Segment, typename Ratio>
    static inline return_type one_degenerate(Segment const& segment,
                                             Ratio const& ratio,
                                             bool a_degenerate)
    {
        return return_type
            (
                pts_policy::one_degenerate(segment, ratio, a_degenerate),
                dir_policy::one_degenerate(segment, ratio, a_degenerate)
            );
    }

    static inline return_type disjoint()
    {
        return return_type
            (
                pts_policy::disjoint(),
                dir_policy::disjoint()
            );
    }

    static inline return_type error(std::string const& msg)
    {
        return return_type
            (
                pts_policy::error(msg),
                dir_policy::error(msg)
            );
    }
};


}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POLICY_HPP

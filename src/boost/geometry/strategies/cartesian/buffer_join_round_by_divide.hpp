// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_BY_DIVIDE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_BY_DIVIDE_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
#include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{


namespace strategy { namespace buffer
{


class join_round_by_divide
{
public :

    inline join_round_by_divide(std::size_t max_level = 4)
        : m_max_level(max_level)
    {}

    template
    <
        typename PromotedType,
        typename Point,
        typename DistanceType,
        typename RangeOut
    >
    inline void mid_points(Point const& vertex,
                Point const& p1, Point const& p2,
                DistanceType const& buffer_distance,
                RangeOut& range_out,
                std::size_t level = 1) const
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        // Generate 'vectors'
        coordinate_type const vp1_x = get<0>(p1) - get<0>(vertex);
        coordinate_type const vp1_y = get<1>(p1) - get<1>(vertex);

        coordinate_type const vp2_x = (get<0>(p2) - get<0>(vertex));
        coordinate_type const vp2_y = (get<1>(p2) - get<1>(vertex));

        // Average them to generate vector in between
        coordinate_type const two = 2;
        coordinate_type const v_x = (vp1_x + vp2_x) / two;
        coordinate_type const v_y = (vp1_y + vp2_y) / two;

        PromotedType const length2 = geometry::math::sqrt(v_x * v_x + v_y * v_y);

        PromotedType prop = buffer_distance / length2;

        Point mid_point;
        set<0>(mid_point, get<0>(vertex) + v_x * prop);
        set<1>(mid_point, get<1>(vertex) + v_y * prop);

        if (level < m_max_level)
        {
            mid_points<PromotedType>(vertex, p1, mid_point, buffer_distance, range_out, level + 1);
        }
        range_out.push_back(mid_point);
        if (level < m_max_level)
        {
            mid_points<PromotedType>(vertex, mid_point, p2, buffer_distance, range_out, level + 1);
        }
    }

    template <typename Point, typename DistanceType, typename RangeOut>
    inline bool apply(Point const& ip, Point const& vertex,
                Point const& perp1, Point const& perp2,
                DistanceType const& buffer_distance,
                RangeOut& range_out) const
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        typedef typename geometry::select_most_precise
            <
                coordinate_type,
                double
            >::type promoted_type;

        geometry::equal_to<Point> equals;

        if (equals(perp1, perp2))
        {
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
            std::cout << "Corner for equal points " << geometry::wkt(ip) << " " << geometry::wkt(perp1) << std::endl;
#endif
            return false;
        }

        // Generate 'vectors'
        coordinate_type const vix = (get<0>(ip) - get<0>(vertex));
        coordinate_type const viy = (get<1>(ip) - get<1>(vertex));

        promoted_type const length_i = geometry::math::sqrt(vix * vix + viy * viy);

        promoted_type const bd = geometry::math::abs(buffer_distance);
        promoted_type prop = bd / length_i;

        Point bp;
        set<0>(bp, get<0>(vertex) + vix * prop);
        set<1>(bp, get<1>(vertex) + viy * prop);

        range_out.push_back(perp1);

        if (m_max_level > 1)
        {
            mid_points<promoted_type>(vertex, perp1, bp, bd, range_out);
            range_out.push_back(bp);
            mid_points<promoted_type>(vertex, bp, perp2, bd, range_out);
        }
        else if (m_max_level == 1)
        {
            range_out.push_back(bp);
        }

        range_out.push_back(perp2);
        return true;
    }

    template <typename NumericType>
    static inline NumericType max_distance(NumericType const& distance)
    {
        return distance;
    }

private :
    std::size_t m_max_level;
};


}} // namespace strategy::buffer


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_BY_DIVIDE_HPP

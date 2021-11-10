// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP


#include <cstddef>
#include <string>

#include <boost/concept_check.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/strategies/side_info.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{


namespace policies { namespace relate
{

struct direction_type
{
    // NOTE: "char" will be replaced by enum in future version

    inline direction_type(side_info const& s, char h,
                int ha, int hb,
                int da = 0, int db = 0,
                bool op = false)
        : how(h)
        , opposite(op)
        , how_a(ha)
        , how_b(hb)
        , dir_a(da)
        , dir_b(db)
        , sides(s)
    {
        arrival[0] = ha;
        arrival[1] = hb;
    }

    inline direction_type(char h, bool op, int ha = 0, int hb = 0)
        : how(h)
        , opposite(op)
        , how_a(ha)
        , how_b(hb)
        , dir_a(0)
        , dir_b(0)
    {
        arrival[0] = ha;
        arrival[1] = hb;
    }


    // TODO: replace this
    // NOTE: "char" will be replaced by enum in future version
    // "How" is the intersection formed?
    char how;

    // Is it opposite (for collinear/equal cases)
    bool opposite;

    // Information on how A arrives at intersection, how B arrives at intersection
    // 1: arrives at intersection
    // -1: starts from intersection
    int how_a;
    int how_b;

    // Direction: how is A positioned from B
    // 1: points left, seen from IP
    // -1: points right, seen from IP
    // In case of intersection: B's TO direction
    // In case that B's TO direction is at A: B's from direction
    // In collinear cases: it is 0
    int dir_a; // Direction of A-s TO from IP
    int dir_b; // Direction of B-s TO from IP

    // New information
    side_info sides;
    // THIS IS EQUAL TO arrival_a, arrival_b - they probably can go now we have robust fractions
    int arrival[2]; // 1=arrival, -1=departure, 0=neutral; == how_a//how_b


    // About arrival[0] (== arrival of a2 w.r.t. b) for COLLINEAR cases
    // Arrival  1: a1--------->a2         (a arrives within b)
    //                      b1----->b2

    // Arrival  1: (a in b)
    //


    // Arrival -1:      a1--------->a2     (a does not arrive within b)
    //             b1----->b2

    // Arrival -1: (b in a)               a_1-------------a_2
    //                                         b_1---b_2

    // Arrival  0:                        a1------->a2  (a arrives at TO-border of b)
    //                                        b1--->b2

};

struct segments_direction
{
    typedef direction_type return_type;

    template
    <
        typename Segment1,
        typename Segment2,
        typename SegmentIntersectionInfo
    >
    static inline return_type segments_crosses(side_info const& sides,
                    SegmentIntersectionInfo const& ,
                    Segment1 const& , Segment2 const& )
    {
        bool const ra0 = sides.get<0,0>() == 0;
        bool const ra1 = sides.get<0,1>() == 0;
        bool const rb0 = sides.get<1,0>() == 0;
        bool const rb1 = sides.get<1,1>() == 0;

        return
            // opposite and same starting point (FROM)
            ra0 && rb0 ? calculate_side<1>(sides, 'f', -1, -1)

            // opposite and point to each other (TO)
            : ra1 && rb1 ? calculate_side<0>(sides, 't', 1, 1)

            // not opposite, forming an angle, first a then b,
            // directed either both left, or both right
            // Check side of B2 from A. This is not calculated before
            : ra1 && rb0 ? angle<1>(sides, 'a', 1, -1)

            // not opposite, forming a angle, first b then a,
            // directed either both left, or both right
            : ra0 && rb1 ? angle<0>(sides, 'a', -1, 1)

            // b starts from interior of a
            : rb0 ? starts_from_middle(sides, 'B', 0, -1)

            // a starts from interior of b (#39)
            : ra0 ? starts_from_middle(sides, 'A', -1, 0)

            // b ends at interior of a, calculate direction of A from IP
            : rb1 ? b_ends_at_middle(sides)

            // a ends at interior of b
            : ra1 ? a_ends_at_middle(sides)

            // normal intersection
            : calculate_side<1>(sides, 'i', -1, -1)
            ;
    }

    template <typename Ratio>
    static inline int arrival_value(Ratio const& r_from, Ratio const& r_to)
    {
        //     a1--------->a2
        // b1----->b2
        // a departs: -1

        // a1--------->a2
        //         b1----->b2
        // a arrives: 1

        // a1--------->a2
        //     b1----->b2
        // both arrive there -> r-to = 1/1, or 0/1 (on_segment)

        // First check the TO (for arrival), then FROM (for departure)
        return r_to.in_segment() ? 1
            : r_to.on_segment() ? 0
            : r_from.on_segment() ? -1
            : -1
            ;
    }

    template <typename Ratio>
    static inline void analyze(Ratio const& r,
        int& in_segment_count,
        int& on_end_count,
        int& outside_segment_count)
    {
        if (r.on_end())
        {
            on_end_count++;
        }
        else if (r.in_segment())
        {
            in_segment_count++;
        }
        else
        {
            outside_segment_count++;
        }
    }

    static inline int arrival_from_position_value(int /*v_from*/, int v_to)
    {
        return v_to == 2 ? 1
             : v_to == 1 || v_to == 3 ? 0
             //: v_from >= 1 && v_from <= 3 ? -1
             : -1;

        // NOTE: this should be an equivalent of the above for the other order
        /* (v_from < 3 && v_to > 3) || (v_from > 3 && v_to < 3) ? 1
         : v_from == 3 || v_to == 3 ? 0
         : -1;*/
    }

    static inline void analyse_position_value(int pos_val,
                                              int & in_segment_count,
                                              int & on_end_count,
                                              int & outside_segment_count)
    {
        if ( pos_val == 1 || pos_val == 3 )
        {
            on_end_count++;
        }
        else if ( pos_val == 2 )
        {
            in_segment_count++;
        }
        else
        {
            outside_segment_count++;
        }
    }

    template <typename Segment1, typename Segment2, typename Ratio>
    static inline return_type segments_collinear(
        Segment1 const& , Segment2 const& , bool opposite,
        int a1_wrt_b, int a2_wrt_b, int b1_wrt_a, int b2_wrt_a,
        Ratio const& /*ra_from_wrt_b*/, Ratio const& /*ra_to_wrt_b*/,
        Ratio const& /*rb_from_wrt_a*/, Ratio const& /*rb_to_wrt_a*/)
    {
        return_type r('c', opposite);

        // IMPORTANT: the order of conditions is different as in intersection_points.hpp
        // We assign A in 0 and B in 1
        r.arrival[0] = arrival_from_position_value(a1_wrt_b, a2_wrt_b);
        r.arrival[1] = arrival_from_position_value(b1_wrt_a, b2_wrt_a);

        // Analyse them
        int a_in_segment_count = 0;
        int a_on_end_count = 0;
        int a_outside_segment_count = 0;
        int b_in_segment_count = 0;
        int b_on_end_count = 0;
        int b_outside_segment_count = 0;
        analyse_position_value(a1_wrt_b,
            a_in_segment_count, a_on_end_count, a_outside_segment_count);
        analyse_position_value(a2_wrt_b,
            a_in_segment_count, a_on_end_count, a_outside_segment_count);
        analyse_position_value(b1_wrt_a,
            b_in_segment_count, b_on_end_count, b_outside_segment_count);
        analyse_position_value(b2_wrt_a,
            b_in_segment_count, b_on_end_count, b_outside_segment_count);

        if (a_on_end_count == 1
            && b_on_end_count == 1
            && a_outside_segment_count == 1
            && b_outside_segment_count == 1)
        {
            // This is a collinear touch
            // -------->             A (or B)
            //         <----------   B (or A)
            // We adapt the "how"
            // TODO: how was to be refactored anyway,
            if (! opposite)
            {
                r.how = 'a';
            }
            else
            {
                r.how = r.arrival[0] == 0 ? 't' : 'f';
            }
        }
        else if (a_on_end_count == 2
                 && b_on_end_count == 2)
        {
            r.how = 'e';
        }

        return r;
    }

    template <typename Segment>
    static inline return_type degenerate(Segment const& , bool)
    {
        return return_type('0', false);
    }

    template <typename Segment, typename Ratio>
    static inline return_type one_degenerate(Segment const& ,
            Ratio const& ,
            bool)
    {
        // To be decided
        return return_type('0', false);
    }

    static inline return_type disjoint()
    {
        return return_type('d', false);
    }

    static inline return_type error(std::string const&)
    {
        // Return "E" to denote error
        // This will throw an error in get_turn_info
        // TODO: change to enum or similar
        return return_type('E', false);
    }

private :

    template <std::size_t I>
    static inline return_type calculate_side(side_info const& sides,
                char how, int how_a, int how_b)
    {
        int const dir = sides.get<1, I>() == 1 ? 1 : -1;
        return return_type(sides, how, how_a, how_b, -dir, dir);
    }

    template <std::size_t I>
    static inline return_type angle(side_info const& sides,
                char how, int how_a, int how_b)
    {
        int const dir = sides.get<1, I>() == 1 ? 1 : -1;
        return return_type(sides, how, how_a, how_b, dir, dir);
    }


    static inline return_type starts_from_middle(side_info const& sides,
                char which,
                int how_a, int how_b)
    {
        // Calculate ARROW of b segment w.r.t. s1
        int dir = sides.get<1, 1>() == 1 ? 1 : -1;

        // From other perspective, then reverse
        bool const is_a = which == 'A';
        if (is_a)
        {
            dir = -dir;
        }

        return return_type(sides, 's',
            how_a,
            how_b,
            is_a ? dir : -dir,
            ! is_a ? dir : -dir);
    }



    // To be harmonized
    static inline return_type a_ends_at_middle(side_info const& sides)
    {
        // Ending at the middle, one ARRIVES, the other one is NEUTRAL
        // (because it both "arrives"  and "departs" there)
        int const dir = sides.get<1, 1>() == 1 ? 1 : -1;
        return return_type(sides, 'm', 1, 0, dir, dir);
    }


    static inline return_type b_ends_at_middle(side_info const& sides)
    {
        int const dir = sides.get<0, 1>() == 1 ? 1 : -1;
        return return_type(sides, 'm', 0, 1, dir, dir);
    }

};

}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_ORIENTED_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_ORIENTED_WINDING_HPP


#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{

/*!
\brief Within detection using winding rule, but checking if enclosing ring is
    counter clockwise and, if so, reverses the result
\ingroup strategies
\tparam Point \tparam_point
\tparam Reverse True if parameter should be reversed
\tparam PointOfSegment \tparam_segment_point
\tparam CalculationType \tparam_calculation
\author Barend Gehrels
\note Only dependant on "side", -> agnostic, suitable for spherical/latlong

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    bool Reverse,
    typename Point,
    typename PointOfSegment = Point,
    typename CalculationType = void
>
class oriented_winding
{
    typedef typename select_calculation_type
        <
            Point,
            PointOfSegment,
            CalculationType
        >::type calculation_type;


    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<Point>::type
        >::type strategy_side_type;


    /*! subclass to keep state */
    class counter
    {
        int m_count;
        bool m_touches;
        calculation_type m_sum_area;

        inline int code() const
        {
            return m_touches ? 0 : m_count == 0 ? -1 : 1;
        }
        inline int clockwise_oriented_code() const
        {
            return (m_sum_area > 0) ? code() : -code();
        }
        inline int oriented_code() const
        {
            return Reverse
                ? -clockwise_oriented_code()
                : clockwise_oriented_code();
        }

    public :
        friend class oriented_winding;

        inline counter()
            : m_count(0)
            , m_touches(false)
            , m_sum_area(0)
        {}

        inline void add_to_area(calculation_type triangle)
        {
            m_sum_area += triangle;
        }

    };


    template <size_t D>
    static inline int check_touch(Point const& point,
                PointOfSegment const& seg1, PointOfSegment const& seg2,
                counter& state)
    {
        calculation_type const p = get<D>(point);
        calculation_type const s1 = get<D>(seg1);
        calculation_type const s2 = get<D>(seg2);
        if ((s1 <= p && s2 >= p) || (s2 <= p && s1 >= p))
        {
            state.m_touches = true;
        }
        return 0;
    }


    template <size_t D>
    static inline int check_segment(Point const& point,
                PointOfSegment const& seg1, PointOfSegment const& seg2,
                counter& state)
    {
        calculation_type const p = get<D>(point);
        calculation_type const s1 = get<D>(seg1);
        calculation_type const s2 = get<D>(seg2);


        // Check if one of segment endpoints is at same level of point
        bool eq1 = math::equals(s1, p);
        bool eq2 = math::equals(s2, p);

        if (eq1 && eq2)
        {
            // Both equal p -> segment is horizontal (or vertical for D=0)
            // The only thing which has to be done is check if point is ON segment
            return check_touch<1 - D>(point, seg1, seg2, state);
        }

        return
              eq1 ? (s2 > p ?  1 : -1)  // Point on level s1, UP/DOWN depending on s2
            : eq2 ? (s1 > p ? -1 :  1)  // idem
            : s1 < p && s2 > p ?  2     // Point between s1 -> s2 --> UP
            : s2 < p && s1 > p ? -2     // Point between s2 -> s1 --> DOWN
            : 0;
    }




public :

    // Typedefs and static methods to fulfill the concept
    typedef Point point_type;
    typedef PointOfSegment segment_point_type;
    typedef counter state_type;

    static inline bool apply(Point const& point,
                PointOfSegment const& s1, PointOfSegment const& s2,
                counter& state)
    {
        state.add_to_area(get<0>(s2) * get<1>(s1) - get<0>(s1) * get<1>(s2));

        int count = check_segment<1>(point, s1, s2, state);
        if (count != 0)
        {
            int side = strategy_side_type::apply(s1, s2, point);
            if (side == 0)
            {
                // Point is lying on segment
                state.m_touches = true;
                state.m_count = 0;
                return false;
            }

            // Side is NEG for right, POS for left.
            // The count is -2 for down, 2 for up (or -1/1)
            // Side positive thus means UP and LEFTSIDE or DOWN and RIGHTSIDE
            // See accompagnying figure (TODO)
            if (side * count > 0)
            {
                state.m_count += count;
            }
        }
        return ! state.m_touches;
    }

    static inline int result(counter const& state)
    {
        return state.oriented_code();
    }
};


}} // namespace strategy::within


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_ORIENTED_WINDING_HPP

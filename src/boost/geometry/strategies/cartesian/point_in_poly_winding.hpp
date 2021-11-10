// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_POINT_IN_POLY_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_POINT_IN_POLY_WINDING_HPP


#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategy/cartesian/expand_point.hpp>

#include <boost/geometry/strategies/cartesian/point_in_box.hpp>
#include <boost/geometry/strategies/cartesian/disjoint_box_box.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{


/*!
\brief Within detection using winding rule in cartesian coordinate system.
\ingroup strategies
\tparam Point_ \tparam_point
\tparam PointOfSegment_ \tparam_segment_point
\tparam CalculationType \tparam_calculation
\author Barend Gehrels

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    typename Point_ = void, // for backward compatibility
    typename PointOfSegment_ = Point_, // for backward compatibility
    typename CalculationType = void
>
class cartesian_winding
{
    template <typename Point, typename PointOfSegment>
    struct calculation_type
        : select_calculation_type
            <
                Point,
                PointOfSegment,
                CalculationType
            >
    {};
    
    /*! subclass to keep state */
    class counter
    {
        int m_count;
        bool m_touches;

        inline int code() const
        {
            return m_touches ? 0 : m_count == 0 ? -1 : 1;
        }

    public :
        friend class cartesian_winding;

        inline counter()
            : m_count(0)
            , m_touches(false)
        {}

    };

public:
    typedef cartesian_tag cs_tag;

    // Typedefs and static methods to fulfill the concept
    typedef counter state_type;

    template <typename Point, typename PointOfSegment>
    static inline bool apply(Point const& point,
                             PointOfSegment const& s1, PointOfSegment const& s2,
                             counter& state)
    {
        bool eq1 = false;
        bool eq2 = false;

        int count = check_segment(point, s1, s2, state, eq1, eq2);
        if (count != 0)
        {
            int side = 0;
            if (count == 1 || count == -1)
            {
                side = side_equal(point, eq1 ? s1 : s2, count);
            }
            else // count == 2 || count == -2
            {
                // 1 left, -1 right
                typedef side::side_by_triangle<CalculationType> side_strategy_type;
                side = side_strategy_type::apply(s1, s2, point);
            }
            
            if (side == 0)
            {
                // Point is lying on segment
                state.m_touches = true;
                state.m_count = 0;
                return false;
            }

            // Side is NEG for right, POS for left.
            // The count is -2 for left, 2 for right (or -1/1)
            // Side positive thus means RIGHT and LEFTSIDE or LEFT and RIGHTSIDE
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
        return state.code();
    }

private:
    template <typename Point, typename PointOfSegment>
    static inline int check_segment(Point const& point,
                                    PointOfSegment const& seg1,
                                    PointOfSegment const& seg2,
                                    counter& state,
                                    bool& eq1, bool& eq2)
    {
        if (check_touch(point, seg1, seg2, state, eq1, eq2))
        {
            return 0;
        }

        return calculate_count(point, seg1, seg2, eq1, eq2);
    }

    template <typename Point, typename PointOfSegment>
    static inline bool check_touch(Point const& point,
                                   PointOfSegment const& seg1,
                                   PointOfSegment const& seg2,
                                   counter& state,
                                   bool& eq1, bool& eq2)
    {
        typedef typename calculation_type<Point, PointOfSegment>::type calc_t;

        calc_t const px = get<0>(point);
        calc_t const s1x = get<0>(seg1);
        calc_t const s2x = get<0>(seg2);

        eq1 = math::equals(s1x, px);
        eq2 = math::equals(s2x, px);

        // Both equal p -> segment vertical
        // The only thing which has to be done is check if point is ON segment
        if (eq1 && eq2)
        {
            calc_t const py = get<1>(point);
            calc_t const s1y = get<1>(seg1);
            calc_t const s2y = get<1>(seg2);
            if ((s1y <= py && s2y >= py) || (s2y <= py && s1y >= py))
            {
                state.m_touches = true;
            }
            return true;
        }
        return false;
    }

    template <typename Point, typename PointOfSegment>
    static inline int calculate_count(Point const& point,
                                      PointOfSegment const& seg1,
                                      PointOfSegment const& seg2,
                                      bool eq1, bool eq2)
    {
        typedef typename calculation_type<Point, PointOfSegment>::type calc_t;

        calc_t const p = get<0>(point);
        calc_t const s1 = get<0>(seg1);
        calc_t const s2 = get<0>(seg2);

        return eq1 ? (s2 > p ?  1 : -1)  // Point on level s1, E/W depending on s2
             : eq2 ? (s1 > p ? -1 :  1)  // idem
             : s1 < p && s2 > p ?  2     // Point between s1 -> s2 --> E
             : s2 < p && s1 > p ? -2     // Point between s2 -> s1 --> W
             : 0;
    }

    template <typename Point, typename PointOfSegment>
    static inline int side_equal(Point const& point,
                                 PointOfSegment const& se,
                                 int count)
    {
        // NOTE: for D=0 the signs would be reversed
        return math::equals(get<1>(point), get<1>(se)) ?
                0 :
                get<1>(point) < get<1>(se) ?
                    // assuming count is equal to 1 or -1
                    -count : // ( count > 0 ? -1 : 1) :
                    count;   // ( count > 0 ? 1 : -1) ;
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef cartesian_winding<> type;
};

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef cartesian_winding<> type;
};

} // namespace services

#endif


}} // namespace strategy::within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace strategy { namespace covered_by { namespace services
{

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, polygonal_tag, cartesian_tag, cartesian_tag>
{
    typedef within::cartesian_winding<> type;
};

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, linear_tag, cartesian_tag, cartesian_tag>
{
    typedef within::cartesian_winding<> type;
};

}}} // namespace strategy::covered_by::services
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_POINT_IN_POLY_WINDING_HPP

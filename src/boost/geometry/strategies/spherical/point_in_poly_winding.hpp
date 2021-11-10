// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013-2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_POINT_IN_POLY_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_POINT_IN_POLY_WINDING_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

#include <boost/geometry/strategy/spherical/expand_point.hpp>

#include <boost/geometry/strategies/cartesian/point_in_box.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/spherical/disjoint_box_box.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename SideStrategy, typename CalculationType>
class spherical_winding_base
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
        //int m_count_n;
        int m_count_s;
        int m_raw_count;
        int m_raw_count_anti;
        bool m_touches;

        inline int code() const
        {
            if (m_touches)
            {
                return 0;
            }
            
            if (m_raw_count != 0 && m_raw_count_anti != 0)
            {
                if (m_raw_count > 0) // right, wrap around south pole
                {
                    return (m_count + m_count_s) == 0 ? -1 : 1;
                }
                else // left, wrap around north pole
                {
                    //return (m_count + m_count_n) == 0 ? -1 : 1;
                    // m_count_n is 0
                    return m_count == 0 ? -1 : 1;
                }
            }

            return m_count == 0 ? -1 : 1;
        }

    public :
        friend class spherical_winding_base;

        inline counter()
            : m_count(0)
            //, m_count_n(0)
            , m_count_s(0)
            , m_raw_count(0)
            , m_raw_count_anti(0)
            , m_touches(false)
        {}

    };

    struct count_info
    {
        explicit count_info(int c = 0, bool ia = false)
            : count(c)
            , is_anti(ia)
        {}

        int count;
        bool is_anti;
    };

public:
    typedef typename SideStrategy::cs_tag cs_tag;

    spherical_winding_base() = default;

    template <typename Model>
    explicit spherical_winding_base(Model const& model)
        : m_side_strategy(model)
    {}

    // Typedefs and static methods to fulfill the concept
    typedef counter state_type;

    template <typename Point, typename PointOfSegment>
    inline bool apply(Point const& point,
                      PointOfSegment const& s1, PointOfSegment const& s2,
                      counter& state) const
    {
        typedef typename calculation_type<Point, PointOfSegment>::type calc_t;
        typedef typename geometry::detail::cs_angular_units<Point>::type units_t;
        typedef math::detail::constants_on_spheroid<calc_t, units_t> constants;

        bool eq1 = false;
        bool eq2 = false;
        bool s_antipodal = false;

        count_info ci = check_segment(point, s1, s2, state, eq1, eq2, s_antipodal);
        if (ci.count != 0)
        {
            if (! ci.is_anti)
            {
                int side = 0;
                if (ci.count == 1 || ci.count == -1)
                {
                    side = side_equal(point, eq1 ? s1 : s2, ci);
                }
                else // count == 2 || count == -2
                {
                    if (! s_antipodal)
                    {
                        // 1 left, -1 right
                        side = m_side_strategy.apply(s1, s2, point);
                    }
                    else
                    {
                        calc_t const pi = constants::half_period();
                        calc_t const s1_lat = get<1>(s1);
                        calc_t const s2_lat = get<1>(s2);

                        side = math::sign(ci.count)
                             * (pi - s1_lat - s2_lat <= pi // segment goes through north pole
                                ? -1 // going right all points will be on right side
                                : 1); // going right all points will be on left side
                    }
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
                if (side * ci.count > 0)
                {
                    state.m_count += ci.count;
                }

                state.m_raw_count += ci.count;
            }
            else
            {
                // Count negated because the segment is on the other side of the globe
                // so it is reversed to match this side of the globe

                // Assuming geometry wraps around north pole, for segments on the other side of the globe
                // the point will always be RIGHT+RIGHTSIDE or LEFT+LEFTSIDE, so side*-count always < 0
                //state.m_count_n -= 0;

                // Assuming geometry wraps around south pole, for segments on the other side of the globe
                // the point will always be RIGHT+LEFTSIDE or LEFT+RIGHTSIDE, so side*-count always > 0
                state.m_count_s -= ci.count;

                state.m_raw_count_anti -= ci.count;
            }
        }
        return ! state.m_touches;
    }

    static inline int result(counter const& state)
    {
        return state.code();
    }

protected:
    template <typename Point, typename PointOfSegment>
    static inline count_info check_segment(Point const& point,
                                    PointOfSegment const& seg1,
                                    PointOfSegment const& seg2,
                                    counter& state,
                                    bool& eq1, bool& eq2, bool& s_antipodal)
    {
        if (check_touch(point, seg1, seg2, state, eq1, eq2, s_antipodal))
        {
            return count_info(0, false);
        }

        return calculate_count(point, seg1, seg2, eq1, eq2, s_antipodal);
    }

    template <typename Point, typename PointOfSegment>
    static inline int check_touch(Point const& point,
                                  PointOfSegment const& seg1,
                                  PointOfSegment const& seg2,
                                  counter& state,
                                  bool& eq1,
                                  bool& eq2,
                                  bool& s_antipodal)
    {
        typedef typename calculation_type<Point, PointOfSegment>::type calc_t;
        typedef typename geometry::detail::cs_angular_units<Point>::type units_t;
        typedef math::detail::constants_on_spheroid<calc_t, units_t> constants;

        calc_t const c0 = 0;
        calc_t const c2 = 2;
        calc_t const pi = constants::half_period();
        calc_t const half_pi = pi / c2;

        calc_t const p_lon = get<0>(point);
        calc_t const s1_lon = get<0>(seg1);
        calc_t const s2_lon = get<0>(seg2);
        calc_t const p_lat = get<1>(point);
        calc_t const s1_lat = get<1>(seg1);
        calc_t const s2_lat = get<1>(seg2);

        // NOTE: lat in {-90, 90} and arbitrary lon
        //  it doesn't matter what lon it is if it's a pole
        //  so e.g. if one of the segment endpoints is a pole
        //  then only the other lon matters
        
        bool eq1_strict = longitudes_equal<units_t>(s1_lon, p_lon);
        bool eq2_strict = longitudes_equal<units_t>(s2_lon, p_lon);
        bool eq1_anti = false;
        bool eq2_anti = false;

        calc_t const anti_p_lon = p_lon + (p_lon <= c0 ? pi : -pi);

        eq1 = eq1_strict // lon strictly equal to s1
            || (eq1_anti = longitudes_equal<units_t>(s1_lon, anti_p_lon)) // anti-lon strictly equal to s1
            || math::equals(math::abs(s1_lat), half_pi); // s1 is pole
        eq2 = eq2_strict // lon strictly equal to s2
            || (eq2_anti = longitudes_equal<units_t>(s2_lon, anti_p_lon)) // anti-lon strictly equal to s2
            || math::equals(math::abs(s2_lat), half_pi); // s2 is pole

        // segment overlapping pole
        calc_t const s_lon_diff = math::longitude_distance_signed<units_t>(s1_lon, s2_lon);
        s_antipodal = math::equals(s_lon_diff, pi);
        if (s_antipodal)
        {
            eq1 = eq2 = eq1 || eq2;

            // segment overlapping pole and point is pole
            if (math::equals(math::abs(p_lat), half_pi))
            {
                eq1 = eq2 = true;
            }
        }
        
        // Both equal p -> segment vertical
        // The only thing which has to be done is check if point is ON segment
        if (eq1 && eq2)
        {
            // segment endpoints on the same sides of the globe
            if (! s_antipodal)
            {
                // p's lat between segment endpoints' lats
                if ( (s1_lat <= p_lat && s2_lat >= p_lat) || (s2_lat <= p_lat && s1_lat >= p_lat) )
                {
                    if (!eq1_anti || !eq2_anti)
                    {
                        state.m_touches = true;
                    }
                }
            }
            else
            {
                // going through north or south pole?
                if (pi - s1_lat - s2_lat <= pi)
                {
                    if ( (eq1_strict && s1_lat <= p_lat) || (eq2_strict && s2_lat <= p_lat) // north
                            || math::equals(p_lat, half_pi) ) // point on north pole
                    {
                        state.m_touches = true;
                    }
                    else if (! eq1_strict && ! eq2_strict && math::equals(p_lat, -half_pi) ) // point on south pole
                    {
                        return false;
                    }
                }
                else // south pole
                {
                    if ( (eq1_strict && s1_lat >= p_lat) || (eq2_strict && s2_lat >= p_lat) // south
                            || math::equals(p_lat, -half_pi) ) // point on south pole
                    {
                        state.m_touches = true;
                    }
                    else if (! eq1_strict && ! eq2_strict && math::equals(p_lat, half_pi) ) // point on north pole
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        return false;
    }

    // Called if point is not aligned with a vertical segment
    template <typename Point, typename PointOfSegment>
    static inline count_info calculate_count(Point const& point,
                                             PointOfSegment const& seg1,
                                             PointOfSegment const& seg2,
                                             bool eq1, bool eq2, bool s_antipodal)
    {
        typedef typename calculation_type<Point, PointOfSegment>::type calc_t;
        typedef typename geometry::detail::cs_angular_units<Point>::type units_t;
        typedef math::detail::constants_on_spheroid<calc_t, units_t> constants;

        // If both segment endpoints were poles below checks wouldn't be enough
        // but this means that either both are the same or that they are N/S poles
        // and therefore the segment is not valid.
        // If needed (eq1 && eq2 ? 0) could be returned

        calc_t const c0 = 0;
        calc_t const pi = constants::half_period();

        calc_t const p = get<0>(point);
        calc_t const s1 = get<0>(seg1);
        calc_t const s2 = get<0>(seg2);

        calc_t const s1_p = math::longitude_distance_signed<units_t>(s1, p);
        
        if (s_antipodal)
        {
            return count_info(s1_p < c0 ? -2 : 2, false); // choose W/E
        }

        calc_t const s1_s2 = math::longitude_distance_signed<units_t>(s1, s2);

        if (eq1 || eq2) // Point on level s1 or s2
        {
            return count_info(s1_s2 < c0 ? -1 : 1, // choose W/E
                              longitudes_equal<units_t>(p + pi, (eq1 ? s1 : s2)));
        }

        // Point between s1 and s2
        if ( math::sign(s1_p) == math::sign(s1_s2)
          && math::abs(s1_p) < math::abs(s1_s2) )
        {
            return count_info(s1_s2 < c0 ? -2 : 2, false); // choose W/E
        }
        
        calc_t const s1_p_anti = math::longitude_distance_signed<units_t>(s1, p + pi);

        // Anti-Point between s1 and s2
        if ( math::sign(s1_p_anti) == math::sign(s1_s2)
          && math::abs(s1_p_anti) < math::abs(s1_s2) )
        {
            return count_info(s1_s2 < c0 ? -2 : 2, true); // choose W/E
        }

        return count_info(0, false);
    }


    // Fix for https://svn.boost.org/trac/boost/ticket/9628
    // For floating point coordinates, the <D> coordinate of a point is compared
    // with the segment's points using some EPS. If the coordinates are "equal"
    // the sides are calculated. Therefore we can treat a segment as a long areal
    // geometry having some width. There is a small ~triangular area somewhere
    // between the segment's effective area and a segment's line used in sides
    // calculation where the segment is on the one side of the line but on the
    // other side of a segment (due to the width).
    // Below picture assuming D = 1, if D = 0 horiz<->vert, E<->N, RIGHT<->UP.
    // For the s1 of a segment going NE the real side is RIGHT but the point may
    // be detected as LEFT, like this:
    //                     RIGHT
    //                 ___----->
    //                  ^      O Pt  __ __
    //                 EPS     __ __
    //                  v__ __ BUT DETECTED AS LEFT OF THIS LINE
    //             _____7
    //       _____/
    // _____/
    // In the code below actually D = 0, so segments are nearly-vertical
    // Called when the point is on the same level as one of the segment's points
    // but the point is not aligned with a vertical segment
    template <typename Point, typename PointOfSegment>
    inline int side_equal(Point const& point,
                          PointOfSegment const& se,
                          count_info const& ci) const
    {
        typedef typename coordinate_type<PointOfSegment>::type scoord_t;
        typedef typename geometry::detail::cs_angular_units<Point>::type units_t;

        if (math::equals(get<1>(point), get<1>(se)))
        {
            return 0;
        }

        // Create a horizontal segment intersecting the original segment's endpoint
        // equal to the point, with the derived direction (E/W).
        PointOfSegment ss1, ss2;
        set<1>(ss1, get<1>(se));
        set<0>(ss1, get<0>(se));
        set<1>(ss2, get<1>(se));
        scoord_t ss20 = get<0>(se);
        if (ci.count > 0)
        {
            ss20 += small_angle<scoord_t, units_t>();
        }
        else
        {
            ss20 -= small_angle<scoord_t, units_t>();
        }
        math::normalize_longitude<units_t>(ss20);
        set<0>(ss2, ss20);

        // Check the side using this vertical segment
        return m_side_strategy.apply(ss1, ss2, point);
    }

    // 1 deg or pi/180 rad
    template <typename CalcT, typename Units>
    static inline CalcT small_angle()
    {
        typedef math::detail::constants_on_spheroid<CalcT, Units> constants;

        return constants::half_period() / CalcT(180);
    }

    template <typename Units, typename CalcT>
    static inline bool longitudes_equal(CalcT const& lon1, CalcT const& lon2)
    {
        return math::equals(
                math::longitude_distance_signed<Units>(lon1, lon2),
                CalcT(0));
    }

    SideStrategy m_side_strategy;
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Within detection using winding rule in spherical coordinate system.
\ingroup strategies
\tparam Point \tparam_point
\tparam PointOfSegment \tparam_segment_point
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    typename Point = void, // for backward compatibility
    typename PointOfSegment = Point, // for backward compatibility
    typename CalculationType = void
>
class spherical_winding
    : public within::detail::spherical_winding_base
        <
            side::spherical_side_formula<CalculationType>,
            CalculationType
        >
{};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef within::spherical_winding<> type;
};

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef within::spherical_winding<> type;
};

} // namespace services

#endif


}} // namespace strategy::within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace strategy { namespace covered_by { namespace services
{

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, polygonal_tag, spherical_tag, spherical_tag>
{
    typedef within::spherical_winding<> type;
};

template <typename PointLike, typename Geometry, typename AnyTag1, typename AnyTag2>
struct default_strategy<PointLike, Geometry, AnyTag1, AnyTag2, pointlike_tag, linear_tag, spherical_tag, spherical_tag>
{
    typedef within::spherical_winding<> type;
};

}}} // namespace strategy::covered_by::services
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_POINT_IN_POLY_WINDING_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_SEGMENT_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_SEGMENT_HPP


#include <cstddef>
#include <utility>

#include <boost/core/ignore_unused.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/detail/envelope/transform_units.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/formulas/meridian_segment.hpp>
#include <boost/geometry/formulas/vertex_latitude.hpp>

#include <boost/geometry/geometries/helper_geometry.hpp>

#include <boost/geometry/strategy/cartesian/envelope_segment.hpp>
#include <boost/geometry/strategy/envelope.hpp>
#include <boost/geometry/strategies/normalize.hpp>
#include <boost/geometry/strategies/spherical/azimuth.hpp>
#include <boost/geometry/strategy/spherical/expand_box.hpp>

#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry { namespace strategy { namespace envelope
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename CalculationType, typename CS_Tag>
struct envelope_segment_call_vertex_latitude
{
    template <typename T1, typename T2, typename Strategy>
    static inline CalculationType apply(T1 const& lat1,
                                        T2 const& alp1,
                                        Strategy const& )
    {
        return geometry::formula::vertex_latitude<CalculationType, CS_Tag>
            ::apply(lat1, alp1);
    }
};

template <typename CalculationType>
struct envelope_segment_call_vertex_latitude<CalculationType, geographic_tag>
{
    template <typename T1, typename T2, typename Strategy>
    static inline CalculationType apply(T1 const& lat1,
                                        T2 const& alp1,
                                        Strategy const& strategy)
    {
        return geometry::formula::vertex_latitude<CalculationType, geographic_tag>
            ::apply(lat1, alp1, strategy.model());
    }
};

template <typename Units, typename CS_Tag>
struct envelope_segment_convert_polar
{
    template <typename T>
    static inline void pre(T & , T & ) {}

    template <typename T>
    static inline void post(T & , T & ) {}
};

template <typename Units>
struct envelope_segment_convert_polar<Units, spherical_polar_tag>
{
    template <typename T>
    static inline void pre(T & lat1, T & lat2)
    {
        lat1 = math::latitude_convert_ep<Units>(lat1);
        lat2 = math::latitude_convert_ep<Units>(lat2);
    }

    template <typename T>
    static inline void post(T & lat1, T & lat2)
    {
        lat1 = math::latitude_convert_ep<Units>(lat1);
        lat2 = math::latitude_convert_ep<Units>(lat2);
        std::swap(lat1, lat2);
    }
};

template <typename CS_Tag>
class envelope_segment_impl
{
private:

    // degrees or radians
    template <typename CalculationType>
    static inline void swap(CalculationType& lon1,
                            CalculationType& lat1,
                            CalculationType& lon2,
                            CalculationType& lat2)
    {
        std::swap(lon1, lon2);
        std::swap(lat1, lat2);
    }

    // radians
    template <typename CalculationType>
    static inline bool contains_pi_half(CalculationType const& a1,
                                        CalculationType const& a2)
    {
        // azimuths a1 and a2 are assumed to be in radians
        BOOST_GEOMETRY_ASSERT(! math::equals(a1, a2));

        static CalculationType const pi_half = math::half_pi<CalculationType>();

        return (a1 < a2)
                ? (a1 < pi_half && pi_half < a2)
                : (a1 > pi_half && pi_half > a2);
    }

    // radians or degrees
    template <typename Units, typename CoordinateType>
    static inline bool crosses_antimeridian(CoordinateType const& lon1,
                                            CoordinateType const& lon2)
    {
        typedef math::detail::constants_on_spheroid
            <
                CoordinateType, Units
            > constants;

        return math::abs(lon1 - lon2) > constants::half_period(); // > pi
    }

    // degrees or radians
    template <typename Units, typename CalculationType, typename Strategy>
    static inline void compute_box_corners(CalculationType& lon1,
                                           CalculationType& lat1,
                                           CalculationType& lon2,
                                           CalculationType& lat2,
                                           CalculationType a1,
                                           CalculationType a2,
                                           Strategy const& strategy)
    {
        // coordinates are assumed to be in radians
        BOOST_GEOMETRY_ASSERT(lon1 <= lon2);
        boost::ignore_unused(lon1, lon2);

        CalculationType lat1_rad = math::as_radian<Units>(lat1);
        CalculationType lat2_rad = math::as_radian<Units>(lat2);

        if (math::equals(a1, a2))
        {
            // the segment must lie on the equator or is very short or is meridian
            return;
        }

        if (lat1 > lat2)
        {
            std::swap(lat1, lat2);
            std::swap(lat1_rad, lat2_rad);
            std::swap(a1, a2);
        }

        if (contains_pi_half(a1, a2))
        {
            CalculationType p_max = envelope_segment_call_vertex_latitude
                <CalculationType, CS_Tag>::apply(lat1_rad, a1, strategy);

            CalculationType const mid_lat = lat1 + lat2;
            if (mid_lat < 0)
            {
                // update using min latitude
                CalculationType const lat_min_rad = -p_max;
                CalculationType const lat_min
                    = math::from_radian<Units>(lat_min_rad);

                if (lat1 > lat_min)
                {
                    lat1 = lat_min;
                }
            }
            else
            {
                // update using max latitude
                CalculationType const lat_max_rad = p_max;
                CalculationType const lat_max
                    = math::from_radian<Units>(lat_max_rad);

                if (lat2 < lat_max)
                {
                    lat2 = lat_max;
                }
            }
        }
    }

    template <typename Units, typename CalculationType>
    static inline void special_cases(CalculationType& lon1,
                                     CalculationType& lat1,
                                     CalculationType& lon2,
                                     CalculationType& lat2)
    {
        typedef math::detail::constants_on_spheroid
            <
                CalculationType, Units
            > constants;

        bool is_pole1 = math::equals(math::abs(lat1), constants::max_latitude());
        bool is_pole2 = math::equals(math::abs(lat2), constants::max_latitude());

        if (is_pole1 && is_pole2)
        {
            // both points are poles; nothing more to do:
            // longitudes are already normalized to 0
            // but just in case
            lon1 = 0;
            lon2 = 0;
        }
        else if (is_pole1 && !is_pole2)
        {
            // first point is a pole, second point is not:
            // make the longitude of the first point the same as that
            // of the second point
            lon1 = lon2;
        }
        else if (!is_pole1 && is_pole2)
        {
            // second point is a pole, first point is not:
            // make the longitude of the second point the same as that
            // of the first point
            lon2 = lon1;
        }

        if (lon1 == lon2)
        {
            // segment lies on a meridian
            if (lat1 > lat2)
            {
                std::swap(lat1, lat2);
            }
            return;
        }

        BOOST_GEOMETRY_ASSERT(!is_pole1 && !is_pole2);

        if (lon1 > lon2)
        {
            swap(lon1, lat1, lon2, lat2);
        }

        if (crosses_antimeridian<Units>(lon1, lon2))
        {
            lon1 += constants::period();
            swap(lon1, lat1, lon2, lat2);
        }
    }

    template
    <
        typename Units,
        typename CalculationType,
        typename Box
    >
    static inline void create_box(CalculationType lon1,
                                  CalculationType lat1,
                                  CalculationType lon2,
                                  CalculationType lat2,
                                  Box& mbr)
    {
        typedef typename coordinate_type<Box>::type box_coordinate_type;

        typedef typename helper_geometry
            <
                Box, box_coordinate_type, Units
            >::type helper_box_type;

        helper_box_type helper_mbr;

        geometry::set
            <
                min_corner, 0
            >(helper_mbr, boost::numeric_cast<box_coordinate_type>(lon1));

        geometry::set
            <
                min_corner, 1
            >(helper_mbr, boost::numeric_cast<box_coordinate_type>(lat1));

        geometry::set
            <
                max_corner, 0
            >(helper_mbr, boost::numeric_cast<box_coordinate_type>(lon2));

        geometry::set
            <
                max_corner, 1
            >(helper_mbr, boost::numeric_cast<box_coordinate_type>(lat2));

        geometry::detail::envelope::transform_units(helper_mbr, mbr);
    }


    template <typename Units, typename CalculationType, typename Strategy>
    static inline void apply(CalculationType& lon1,
                             CalculationType& lat1,
                             CalculationType& lon2,
                             CalculationType& lat2,
                             Strategy const& strategy)
    {
        special_cases<Units>(lon1, lat1, lon2, lat2);

        CalculationType lon1_rad = math::as_radian<Units>(lon1);
        CalculationType lat1_rad = math::as_radian<Units>(lat1);
        CalculationType lon2_rad = math::as_radian<Units>(lon2);
        CalculationType lat2_rad = math::as_radian<Units>(lat2);
        CalculationType alp1, alp2;
        strategy.apply(lon1_rad, lat1_rad, lon2_rad, lat2_rad, alp1, alp2);

        compute_box_corners<Units>(lon1, lat1, lon2, lat2, alp1, alp2, strategy);
    }

public:
    template
    <
        typename Units,
        typename CalculationType,
        typename Box,
        typename Strategy
    >
    static inline void apply(CalculationType lon1,
                             CalculationType lat1,
                             CalculationType lon2,
                             CalculationType lat2,
                             Box& mbr,
                             Strategy const& strategy)
    {
        typedef envelope_segment_convert_polar<Units, typename cs_tag<Box>::type> convert_polar;

        convert_polar::pre(lat1, lat2);

        apply<Units>(lon1, lat1, lon2, lat2, strategy);

        convert_polar::post(lat1, lat2);

        create_box<Units>(lon1, lat1, lon2, lat2, mbr);
    }

};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template
<
    typename CalculationType = void
>
class spherical_segment
{
public:
    template <typename Point, typename Box>
    static inline void apply(Point const& point1, Point const& point2,
                             Box& box)
    {
        Point p1_normalized, p2_normalized;
        strategy::normalize::spherical_point::apply(point1, p1_normalized);
        strategy::normalize::spherical_point::apply(point2, p2_normalized);

        geometry::strategy::azimuth::spherical<CalculationType> azimuth_spherical;

        typedef typename geometry::detail::cs_angular_units<Point>::type units_type;

        // first compute the envelope range for the first two coordinates
        strategy::envelope::detail::envelope_segment_impl
            <
                spherical_equatorial_tag
            >::template apply<units_type>(geometry::get<0>(p1_normalized),
                                          geometry::get<1>(p1_normalized),
                                          geometry::get<0>(p2_normalized),
                                          geometry::get<1>(p2_normalized),
                                          box,
                                          azimuth_spherical);

        // now compute the envelope range for coordinates of
        // dimension 2 and higher
        strategy::envelope::detail::envelope_one_segment
            <
                2, dimension<Point>::value
            >::apply(point1, point2, box);
  }
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename CalculationType>
struct default_strategy<segment_tag, spherical_equatorial_tag, CalculationType>
{
    typedef strategy::envelope::spherical_segment<CalculationType> type;
};


template <typename CalculationType>
struct default_strategy<segment_tag, spherical_polar_tag, CalculationType>
{
    typedef strategy::envelope::spherical_segment<CalculationType> type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::envelope

}} //namepsace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_ENVELOPE_SEGMENT_HPP


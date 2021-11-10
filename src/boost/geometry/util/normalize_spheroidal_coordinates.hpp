// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Adeel Ahmad, as part of Google Summer of Code 2018 program

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP
#define BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

namespace math 
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// CoordinateType, radian, true
template <typename CoordinateType, typename Units, bool IsEquatorial = true>
struct constants_on_spheroid
{
    static inline CoordinateType period()
    {
        return math::two_pi<CoordinateType>();
    }

    static inline CoordinateType half_period()
    {
        return math::pi<CoordinateType>();
    }

    static inline CoordinateType quarter_period()
    {
        static CoordinateType const
            pi_half = math::pi<CoordinateType>() / CoordinateType(2);
        return pi_half;
    }

    static inline CoordinateType min_longitude()
    {
        static CoordinateType const minus_pi = -math::pi<CoordinateType>();
        return minus_pi;
    }

    static inline CoordinateType max_longitude()
    {
        return math::pi<CoordinateType>();
    }

    static inline CoordinateType min_latitude()
    {
        static CoordinateType const minus_half_pi
            = -math::half_pi<CoordinateType>();
        return minus_half_pi;
    }

    static inline CoordinateType max_latitude()
    {
        return math::half_pi<CoordinateType>();
    }
};

template <typename CoordinateType>
struct constants_on_spheroid<CoordinateType, radian, false>
    : constants_on_spheroid<CoordinateType, radian, true>
{
    static inline CoordinateType min_latitude()
    {
        return CoordinateType(0);
    }

    static inline CoordinateType max_latitude()
    {
        return math::pi<CoordinateType>();
    }
};

template <typename CoordinateType>
struct constants_on_spheroid<CoordinateType, degree, true>
{
    static inline CoordinateType period()
    {
        return CoordinateType(360.0);
    }

    static inline CoordinateType half_period()
    {
        return CoordinateType(180.0);
    }

    static inline CoordinateType quarter_period()
    {
        return CoordinateType(90.0);
    }

    static inline CoordinateType min_longitude()
    {
        return CoordinateType(-180.0);
    }

    static inline CoordinateType max_longitude()
    {
        return CoordinateType(180.0);
    }

    static inline CoordinateType min_latitude()
    {
        return CoordinateType(-90.0);
    }

    static inline CoordinateType max_latitude()
    {
        return CoordinateType(90.0);
    }
};

template <typename CoordinateType>
struct constants_on_spheroid<CoordinateType, degree, false>
    : constants_on_spheroid<CoordinateType, degree, true>
{
    static inline CoordinateType min_latitude()
    {
        return CoordinateType(0);
    }

    static inline CoordinateType max_latitude()
    {
        return CoordinateType(180.0);
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template <typename Units, typename CoordinateType>
inline CoordinateType latitude_convert_ep(CoordinateType const& lat)
{
    typedef math::detail::constants_on_spheroid
            <
                CoordinateType,
                Units
            > constants;

    return constants::quarter_period() - lat;
}


template <typename Units, bool IsEquatorial, typename T>
static bool is_latitude_pole(T const& lat)
{
    typedef math::detail::constants_on_spheroid
        <
            T,
            Units
        > constants;

    return math::equals(math::abs(IsEquatorial
                                    ? lat
                                    : math::latitude_convert_ep<Units>(lat)),
                        constants::quarter_period());

}


template <typename Units, typename T>
static bool is_longitude_antimeridian(T const& lon)
{
    typedef math::detail::constants_on_spheroid
        <
            T,
            Units
        > constants;

    return math::equals(math::abs(lon), constants::half_period());

}


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Units, bool IsEquatorial>
struct latitude_convert_if_polar
{
    template <typename T>
    static inline void apply(T & /*lat*/) {}
};

template <typename Units>
struct latitude_convert_if_polar<Units, false>
{
    template <typename T>
    static inline void apply(T & lat)
    {
        lat = latitude_convert_ep<Units>(lat);
    }
};


template <typename Units, typename CoordinateType, bool IsEquatorial = true>
class normalize_spheroidal_coordinates
{
    typedef constants_on_spheroid<CoordinateType, Units> constants;

protected:
    static inline CoordinateType normalize_up(CoordinateType const& value)
    {
        return
            math::mod(value + constants::half_period(), constants::period())
            - constants::half_period();            
    }

    static inline CoordinateType normalize_down(CoordinateType const& value)
    {
        return
            math::mod(value - constants::half_period(), constants::period())
            + constants::half_period();            
    }

public:
    static inline void apply(CoordinateType& longitude)
    {
        // normalize longitude
        if (math::equals(math::abs(longitude), constants::half_period()))
        {
            longitude = constants::half_period();
        }
        else if (longitude > constants::half_period())
        {
            longitude = normalize_up(longitude);
            if (math::equals(longitude, -constants::half_period()))
            {
                longitude = constants::half_period();
            }
        }
        else if (longitude < -constants::half_period())
        {
            longitude = normalize_down(longitude);
        }
    }

    static inline void apply(CoordinateType& longitude,
                             CoordinateType& latitude,
                             bool normalize_poles = true)
    {
        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude);

#ifdef BOOST_GEOMETRY_NORMALIZE_LATITUDE
        // normalize latitude
        if (math::larger(latitude, constants::half_period()))
        {
            latitude = normalize_up(latitude);
        }
        else if (math::smaller(latitude, -constants::half_period()))
        {
            latitude = normalize_down(latitude);
        }

        // fix latitude range
        if (latitude < constants::min_latitude())
        {
            latitude = -constants::half_period() - latitude;
            longitude -= constants::half_period();
        }
        else if (latitude > constants::max_latitude())
        {
            latitude = constants::half_period() - latitude;
            longitude -= constants::half_period();
        }
#endif // BOOST_GEOMETRY_NORMALIZE_LATITUDE

        // normalize longitude
        apply(longitude);

        // finally normalize poles
        if (normalize_poles)
        {
            if (math::equals(math::abs(latitude), constants::max_latitude()))
            {
                // for the north and south pole we set the longitude to 0
                // (works for both radians and degrees)
                longitude = CoordinateType(0);
            }
        }

        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude);

#ifdef BOOST_GEOMETRY_NORMALIZE_LATITUDE
        BOOST_GEOMETRY_ASSERT(! math::larger(constants::min_latitude(), latitude));
        BOOST_GEOMETRY_ASSERT(! math::larger(latitude, constants::max_latitude()));
#endif // BOOST_GEOMETRY_NORMALIZE_LATITUDE

        BOOST_GEOMETRY_ASSERT(math::smaller(constants::min_longitude(), longitude));
        BOOST_GEOMETRY_ASSERT(! math::larger(longitude, constants::max_longitude()));
    }
};


template <typename Units, typename CoordinateType>
inline void normalize_angle_loop(CoordinateType& angle)
{
    typedef constants_on_spheroid<CoordinateType, Units> constants;
    CoordinateType const pi = constants::half_period();
    CoordinateType const two_pi = constants::period();
    while (angle > pi)
        angle -= two_pi;
    while (angle <= -pi)
        angle += two_pi;
}

template <typename Units, typename CoordinateType>
inline void normalize_angle_cond(CoordinateType& angle)
{
    typedef constants_on_spheroid<CoordinateType, Units> constants;
    CoordinateType const pi = constants::half_period();
    CoordinateType const two_pi = constants::period();
    if (angle > pi)
        angle -= two_pi;
    else if (angle <= -pi)
        angle += two_pi;
}


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Short utility to normalize the coordinates on a spheroid
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude Longitude
\param latitude Latitude
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_spheroidal_coordinates(CoordinateType& longitude,
                                             CoordinateType& latitude)
{
    detail::normalize_spheroidal_coordinates
        <
            Units, CoordinateType
        >::apply(longitude, latitude);
}

template <typename Units, bool IsEquatorial, typename CoordinateType>
inline void normalize_spheroidal_coordinates(CoordinateType& longitude,
                                             CoordinateType& latitude)
{
    detail::normalize_spheroidal_coordinates
        <
            Units, CoordinateType, IsEquatorial
        >::apply(longitude, latitude);
}

/*!
\brief Short utility to normalize the longitude on a spheroid.
       Note that in general both coordinates should be normalized at once.
       This utility is suitable e.g. for normalization of the difference of longitudes.
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude Longitude
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_longitude(CoordinateType& longitude)
{
    detail::normalize_spheroidal_coordinates
        <
            Units, CoordinateType
        >::apply(longitude);
}

/*!
\brief Short utility to normalize the azimuth on a spheroid
       in the range (-180, 180].
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param angle Angle
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_azimuth(CoordinateType& angle)
{
    normalize_longitude<Units, CoordinateType>(angle);
}

/*!
\brief Normalize the given values.
\tparam ValueType The type of the values
\param x Value x
\param y Value y
TODO: adl1995 - Merge this function with
formulas/vertex_longitude.hpp
*/
template<typename ValueType>
inline void normalize_unit_vector(ValueType& x, ValueType& y)
{
    ValueType h = boost::math::hypot(x, y);

    BOOST_GEOMETRY_ASSERT(h > 0);

    x /= h; y /= h;
}

/*!
\brief Short utility to calculate difference between two longitudes
       normalized in range (-180, 180].
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude1 Longitude 1
\param longitude2 Longitude 2
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline CoordinateType longitude_distance_signed(CoordinateType const& longitude1,
                                                CoordinateType const& longitude2)
{
    CoordinateType diff = longitude2 - longitude1;
    math::normalize_longitude<Units, CoordinateType>(diff);
    return diff;
}


/*!
\brief Short utility to calculate difference between two longitudes
       normalized in range [0, 360).
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude1 Longitude 1
\param longitude2 Longitude 2
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline CoordinateType longitude_distance_unsigned(CoordinateType const& longitude1,
                                                  CoordinateType const& longitude2)
{
    typedef math::detail::constants_on_spheroid
        <
            CoordinateType, Units
        > constants;

    CoordinateType const c0 = 0;
    CoordinateType diff = longitude_distance_signed<Units>(longitude1, longitude2);
    if (diff < c0) // (-180, 180] -> [0, 360)
    {
        diff += constants::period();
    }
    return diff;
}

/*!
\brief The abs difference between longitudes in range [0, 180].
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude1 Longitude 1
\param longitude2 Longitude 2
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline CoordinateType longitude_difference(CoordinateType const& longitude1,
                                           CoordinateType const& longitude2)
{
    return math::abs(math::longitude_distance_signed<Units>(longitude1, longitude2));
}

template <typename Units, typename CoordinateType>
inline CoordinateType longitude_interval_distance_signed(CoordinateType const& longitude_a1,
                                                         CoordinateType const& longitude_a2,
                                                         CoordinateType const& longitude_b)
{
    CoordinateType const c0 = 0;
    CoordinateType dist_a12 = longitude_distance_signed<Units>(longitude_a1, longitude_a2);
    CoordinateType dist_a1b = longitude_distance_signed<Units>(longitude_a1, longitude_b);
    if (dist_a12 < c0)
    {
        dist_a12 = -dist_a12;
        dist_a1b = -dist_a1b;
    }
    
    return dist_a1b < c0 ? dist_a1b
         : dist_a1b > dist_a12 ? dist_a1b - dist_a12
         : c0;
}


} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP

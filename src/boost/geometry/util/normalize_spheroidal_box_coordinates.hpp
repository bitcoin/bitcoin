// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2017, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_BOX_COORDINATES_HPP
#define BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_BOX_COORDINATES_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>


namespace boost { namespace geometry
{

namespace math 
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Units, typename CoordinateType, bool IsEquatorial = true>
class normalize_spheroidal_box_coordinates
{
private:
    typedef normalize_spheroidal_coordinates<Units, CoordinateType> normalize;
    typedef constants_on_spheroid<CoordinateType, Units> constants;

    static inline bool is_band(CoordinateType const& longitude1,
                               CoordinateType const& longitude2)
    {
        return ! math::smaller(math::abs(longitude1 - longitude2),
                               constants::period());
    }

public:
    static inline void apply(CoordinateType& longitude1,
                             CoordinateType& latitude1,
                             CoordinateType& longitude2,
                             CoordinateType& latitude2,
                             bool band)
    {
        normalize::apply(longitude1, latitude1, false);
        normalize::apply(longitude2, latitude2, false);

        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude1);
        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude2);

        if (math::equals(latitude1, constants::min_latitude())
            && math::equals(latitude2, constants::min_latitude()))
        {
            // box degenerates to the south pole
            longitude1 = longitude2 = CoordinateType(0);
        }
        else if (math::equals(latitude1, constants::max_latitude())
                 && math::equals(latitude2, constants::max_latitude()))
        {
            // box degenerates to the north pole
            longitude1 = longitude2 = CoordinateType(0);
        }
        else if (band)
        {
            // the box is a band between two small circles (parallel
            // to the equator) on the spheroid
            longitude1 = constants::min_longitude();
            longitude2 = constants::max_longitude();
        }
        else if (longitude1 > longitude2)
        {
            // the box crosses the antimeridian, so we need to adjust
            // the longitudes
            longitude2 += constants::period();
        }

        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude1);
        latitude_convert_if_polar<Units, IsEquatorial>::apply(latitude2);

#ifdef BOOST_GEOMETRY_NORMALIZE_LATITUDE
        BOOST_GEOMETRY_ASSERT(! math::larger(latitude1, latitude2));
        BOOST_GEOMETRY_ASSERT(! math::smaller(latitude1, constants::min_latitude()));
        BOOST_GEOMETRY_ASSERT(! math::larger(latitude2, constants::max_latitude()));
#endif

        BOOST_GEOMETRY_ASSERT(! math::larger(longitude1, longitude2));
        BOOST_GEOMETRY_ASSERT(! math::smaller(longitude1, constants::min_longitude()));
        BOOST_GEOMETRY_ASSERT
            (! math::larger(longitude2 - longitude1, constants::period()));
    }

    static inline void apply(CoordinateType& longitude1,
                             CoordinateType& latitude1,
                             CoordinateType& longitude2,
                             CoordinateType& latitude2)
    {
        bool const band = is_band(longitude1, longitude2);

        apply(longitude1, latitude1, longitude2, latitude2, band);
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Short utility to normalize the coordinates of a box on a spheroid
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude1 Minimum longitude of the box
\param latitude1 Minimum latitude of the box
\param longitude2 Maximum longitude of the box
\param latitude2 Maximum latitude of the box
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_spheroidal_box_coordinates(CoordinateType& longitude1,
                                                 CoordinateType& latitude1,
                                                 CoordinateType& longitude2,
                                                 CoordinateType& latitude2)
{
    detail::normalize_spheroidal_box_coordinates
        <
            Units, CoordinateType
        >::apply(longitude1, latitude1, longitude2, latitude2);
}

template <typename Units, bool IsEquatorial, typename CoordinateType>
inline void normalize_spheroidal_box_coordinates(CoordinateType& longitude1,
                                                 CoordinateType& latitude1,
                                                 CoordinateType& longitude2,
                                                 CoordinateType& latitude2)
{
    detail::normalize_spheroidal_box_coordinates
        <
            Units, CoordinateType, IsEquatorial
        >::apply(longitude1, latitude1, longitude2, latitude2);
}

/*!
\brief Short utility to normalize the coordinates of a box on a spheroid
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude1 Minimum longitude of the box
\param latitude1 Minimum latitude of the box
\param longitude2 Maximum longitude of the box
\param latitude2 Maximum latitude of the box
\param band Indicates whether the box should be treated as a band or
       not and avoid the computation done in the other version of the function
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_spheroidal_box_coordinates(CoordinateType& longitude1,
                                                 CoordinateType& latitude1,
                                                 CoordinateType& longitude2,
                                                 CoordinateType& latitude2,
                                                 bool band)
{
    detail::normalize_spheroidal_box_coordinates
        <
            Units, CoordinateType
        >::apply(longitude1, latitude1, longitude2, latitude2, band);
}

template <typename Units, bool IsEquatorial, typename CoordinateType>
inline void normalize_spheroidal_box_coordinates(CoordinateType& longitude1,
                                                 CoordinateType& latitude1,
                                                 CoordinateType& longitude2,
                                                 CoordinateType& latitude2,
                                                 bool band)
{
    detail::normalize_spheroidal_box_coordinates
        <
            Units, CoordinateType, IsEquatorial
        >::apply(longitude1, latitude1, longitude2, latitude2, band);
}


} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_BOX_COORDINATES_HPP

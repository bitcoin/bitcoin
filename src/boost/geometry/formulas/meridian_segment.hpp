// Boost.Geometry

// Copyright (c) 2017-2018 Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_MERIDIAN_SEGMENT_HPP
#define BOOST_GEOMETRY_FORMULAS_MERIDIAN_SEGMENT_HPP

#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>

namespace boost { namespace geometry { namespace formula
{

/*!
\brief Test if a segment is meridian or not.
*/

class meridian_segment
{

public :

    enum SegmentType {NonMeridian, MeridianCrossingPole, MeridianNotCrossingPole};

    template <typename T>
    static inline SegmentType is_meridian(T lon1, T lat1, T lon2, T lat2)
    {
        SegmentType res = NonMeridian;
        T diff = geometry::math::longitude_distance_signed<geometry::radian>(lon1, lon2);

        if ( meridian_not_crossing_pole(lat1, lat2, diff) )
        {
            res = MeridianNotCrossingPole;
        }
        else if ( meridian_crossing_pole(diff) )
        {
            res = MeridianCrossingPole;
        }
        return res;
    }

    template <typename T>
    static bool meridian_not_crossing_pole(T lat1, T lat2, T diff)
    {
        T half_pi = math::half_pi<T>();
        return math::equals(diff, T(0)) ||
               (math::equals(lat2, half_pi) && math::equals(lat1, -half_pi));
    }

    template <typename T>
    static bool meridian_crossing_pole(T diff)
    {
        return math::equals(math::abs(diff), math::pi<T>());
    }

};

}}} // namespace boost::geometry::formula

#endif //BOOST_GEOMETRY_FORMULAS_MERIDIAN_SEGMENT_HPP

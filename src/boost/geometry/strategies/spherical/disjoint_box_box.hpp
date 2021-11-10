// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2015 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2018.
// Modifications copyright (c) 2013-2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_BOX_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_BOX_BOX_HPP

#include <cstddef>

#include <boost/geometry/core/cs.hpp>

#include <boost/geometry/strategies/cartesian/disjoint_box_box.hpp>
#include <boost/geometry/strategies/disjoint.hpp>

#include <boost/geometry/util/normalize_spheroidal_coordinates.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry { namespace strategy { namespace disjoint
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct box_box_on_spheroid
{
    template <typename Box1, typename Box2>
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        typedef typename geometry::select_most_precise
            <
                typename coordinate_type<Box1>::type,
                typename coordinate_type<Box2>::type
            >::type calc_t;
        typedef typename geometry::detail::cs_angular_units<Box1>::type units_t;
        typedef math::detail::constants_on_spheroid<calc_t, units_t> constants;

        calc_t const b1_min = get<min_corner, 0>(box1);
        calc_t const b1_max = get<max_corner, 0>(box1);
        calc_t const b2_min = get<min_corner, 0>(box2);
        calc_t const b2_max = get<max_corner, 0>(box2);

        // min <= max <=> diff >= 0
        calc_t const diff1 = b1_max - b1_min;
        calc_t const diff2 = b2_max - b2_min;

        // check the intersection if neither box cover the whole globe
        if (diff1 < constants::period() && diff2 < constants::period())
        {
            // calculate positive longitude translation with b1_min as origin
            calc_t const diff_min = math::longitude_distance_unsigned<units_t>(b1_min, b2_min);
            calc_t const b2_min_transl = b1_min + diff_min; // always right of b1_min
            calc_t b2_max_transl = b2_min_transl - constants::period() + diff2;

            // if the translation is too close then use the original point
            // note that math::abs(b2_max_transl - b2_max) takes values very
            // close to k*2*constants::period() for k=0,1,2,...
            if (math::abs(b2_max_transl - b2_max) < constants::period() / 2)
            {
                b2_max_transl = b2_max;
            }

            if (b2_min_transl > b1_max  // b2_min right of b1_max
             && b2_max_transl < b1_min) // b2_max left of b1_min
            {
                return true;
            }
        }

        return box_box
            <
                Box1, Box2, 1
            >::apply(box1, box2);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


struct spherical_box_box
{
    template <typename Box1, typename Box2>
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        return detail::box_box_on_spheroid::apply(box1, box2);
    }
};


namespace services
{

template <typename Box1, typename Box2, int TopDim1, int TopDim2>
struct default_strategy<Box1, Box2, box_tag, box_tag, TopDim1, TopDim2, spherical_equatorial_tag, spherical_equatorial_tag>
{
    typedef disjoint::spherical_box_box type;
};

template <typename Box1, typename Box2, int TopDim1, int TopDim2>
struct default_strategy<Box1, Box2, box_tag, box_tag, TopDim1, TopDim2, spherical_polar_tag, spherical_polar_tag>
{
    typedef disjoint::spherical_box_box type;
};

template <typename Box1, typename Box2, int TopDim1, int TopDim2>
struct default_strategy<Box1, Box2, box_tag, box_tag, TopDim1, TopDim2, geographic_tag, geographic_tag>
{
    typedef disjoint::spherical_box_box type;
};

} // namespace services

}}}} // namespace boost::geometry::strategy::disjoint


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISJOINT_BOX_BOX_HPP

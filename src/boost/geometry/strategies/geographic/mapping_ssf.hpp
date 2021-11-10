// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014, 2017.
// Modifications copyright (c) 2014-2017 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_MAPPING_SSF_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_MAPPING_SSF_HPP


#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace side
{


// An enumeration type defining types of mapping of geographical
// latitude to spherical latitude.
// See: http://en.wikipedia.org/wiki/Great_ellipse
//      http://en.wikipedia.org/wiki/Latitude#Auxiliary_latitudes
enum mapping_type { mapping_geodetic, mapping_reduced, mapping_geocentric };


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Spheroid, mapping_type Mapping>
struct mapper
{
    explicit inline mapper(Spheroid const& /*spheroid*/) {}

    template <typename CalculationType>
    static inline CalculationType const& apply(CalculationType const& lat)
    {
        return lat;
    }
};

template <typename Spheroid>
struct mapper<Spheroid, mapping_reduced>
{
    typedef typename promote_floating_point
        <
            typename radius_type<Spheroid>::type
        >::type fraction_type;

    explicit inline mapper(Spheroid const& spheroid)
    {
        fraction_type const a = geometry::get_radius<0>(spheroid);
        fraction_type const b = geometry::get_radius<2>(spheroid);
        b_div_a = b / a;
    }

    template <typename CalculationType>
    inline CalculationType apply(CalculationType const& lat) const
    {
        return atan(static_cast<CalculationType>(b_div_a) * tan(lat));
    }

    fraction_type b_div_a;
};

template <typename Spheroid>
struct mapper<Spheroid, mapping_geocentric>
{
    typedef typename promote_floating_point
        <
            typename radius_type<Spheroid>::type
        >::type fraction_type;

    explicit inline mapper(Spheroid const& spheroid)
    {
        fraction_type const a = geometry::get_radius<0>(spheroid);
        fraction_type const b = geometry::get_radius<2>(spheroid);
        sqr_b_div_a = b / a;
        sqr_b_div_a *= sqr_b_div_a;
    }

    template <typename CalculationType>
    inline CalculationType apply(CalculationType const& lat) const
    {
        return atan(static_cast<CalculationType>(sqr_b_div_a) * tan(lat));
    }

    fraction_type sqr_b_div_a;
};

}
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Check at which side of a geographical segment a point lies
         left of segment (> 0), right of segment (< 0), on segment (0).
         The check is performed by mapping the geographical coordinates
         to spherical coordinates and using spherical_side_formula.
\ingroup strategies
\tparam Spheroid The reference spheroid model
\tparam Mapping The type of mapping of geographical to spherical latitude
\tparam CalculationType \tparam_calculation
 */
template <typename Spheroid,
          mapping_type Mapping = mapping_geodetic,
          typename CalculationType = void>
class mapping_spherical_side_formula
{

public :
    inline mapping_spherical_side_formula()
        : m_mapper(Spheroid())
    {}

    explicit inline mapping_spherical_side_formula(Spheroid const& spheroid)
        : m_mapper(spheroid)
    {}

    template <typename P1, typename P2, typename P>
    inline int apply(P1 const& p1, P2 const& p2, P const& p) const
    {
        typedef typename promote_floating_point
            <
                typename select_calculation_type_alt
                    <
                        CalculationType,
                        P1, P2, P
                    >::type
            >::type calculation_type;

        calculation_type lon1 = get_as_radian<0>(p1);
        calculation_type lat1 = m_mapper.template apply<calculation_type>(get_as_radian<1>(p1));
        calculation_type lon2 = get_as_radian<0>(p2);
        calculation_type lat2 = m_mapper.template apply<calculation_type>(get_as_radian<1>(p2));
        calculation_type lon = get_as_radian<0>(p);
        calculation_type lat = m_mapper.template apply<calculation_type>(get_as_radian<1>(p));

        return detail::spherical_side_formula(lon1, lat1, lon2, lat2, lon, lat);
    }

private:
    side::detail::mapper<Spheroid, Mapping> const m_mapper;
};

// The specialization for geodetic latitude which can be used directly
template <typename Spheroid,
          typename CalculationType>
class mapping_spherical_side_formula<Spheroid, mapping_geodetic, CalculationType>
{

public :
    inline mapping_spherical_side_formula() {}
    explicit inline mapping_spherical_side_formula(Spheroid const& /*spheroid*/) {}

    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        return spherical_side_formula<CalculationType>::apply(p1, p2, p);
    }
};

}} // namespace strategy::side

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_MAPPING_SSF_HPP

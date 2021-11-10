// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2016-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AZIMUTH_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AZIMUTH_HPP


#include <type_traits>

#include <boost/geometry/formulas/spherical.hpp>

#include <boost/geometry/strategies/azimuth.hpp>

#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace azimuth
{

template <typename CalculationType = void>
class spherical
{
public:
    template <typename T1, typename T2>
    struct result_type
        : geometry::select_most_precise
              <
                  T1, T2, CalculationType
              >
    {};

    template <typename T1, typename T2, typename Result>
    static inline void apply(T1 const& lon1_rad, T1 const& lat1_rad,
                             T2 const& lon2_rad, T2 const& lat2_rad,
                             Result& a1, Result& a2)
    {
        compute<true, true>(lon1_rad, lat1_rad,
                            lon2_rad, lat2_rad,
                            a1, a2);
    }
    template <typename T1, typename T2, typename Result>
    static inline void apply(T1 const& lon1_rad, T1 const& lat1_rad,
                             T2 const& lon2_rad, T2 const& lat2_rad,
                             Result& a1)
    {
        compute<true, false>(lon1_rad, lat1_rad,
                             lon2_rad, lat2_rad,
                             a1, a1);
    }
    template <typename T1, typename T2, typename Result>
    static inline void apply_reverse(T1 const& lon1_rad, T1 const& lat1_rad,
                                     T2 const& lon2_rad, T2 const& lat2_rad,
                                     Result& a2)
    {
        compute<false, true>(lon1_rad, lat1_rad,
                             lon2_rad, lat2_rad,
                             a2, a2);
    }

private:
    template
    <
        bool EnableAzimuth,
        bool EnableReverseAzimuth,
        typename T1, typename T2, typename Result
    >
    static inline void compute(T1 const& lon1_rad, T1 const& lat1_rad,
                               T2 const& lon2_rad, T2 const& lat2_rad,
                               Result& a1, Result& a2)
    {
        typedef typename result_type<T1, T2>::type calc_t;

        geometry::formula::result_spherical<calc_t>
            result = geometry::formula::spherical_azimuth
                     <
                        calc_t,
                        EnableReverseAzimuth
                     >(calc_t(lon1_rad), calc_t(lat1_rad),
                       calc_t(lon2_rad), calc_t(lat2_rad));

        if (EnableAzimuth)
        {
            a1 = result.azimuth;
        }
        if (EnableReverseAzimuth)
        {
            a2 = result.reverse_azimuth;
        }
    }
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <>
struct default_strategy<spherical_equatorial_tag>
{
    typedef strategy::azimuth::spherical<> type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

}} // namespace strategy::azimuth


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AZIMUTH_HPP

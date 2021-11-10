// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2016-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_AZIMUTH_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_AZIMUTH_HPP


#include <type_traits>

#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/strategies/azimuth.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>

#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace azimuth
{

template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
{
public:
    template <typename T1, typename T2>
    struct result_type
        : geometry::select_most_precise
              <
                  T1, T2, CalculationType
              >
    {};

    typedef Spheroid model_type;

    inline geographic()
        : m_spheroid()
    {}

    explicit inline geographic(Spheroid const& spheroid)
        : m_spheroid(spheroid)
    {}

    inline model_type const& model() const
    {
        return m_spheroid;
    }

    template <typename T1, typename T2, typename Result>
    inline void apply(T1 const& lon1_rad, T1 const& lat1_rad,
                      T2 const& lon2_rad, T2 const& lat2_rad,
                      Result& a1, Result& a2) const
    {
        compute<true, true>(lon1_rad, lat1_rad,
                            lon2_rad, lat2_rad,
                            a1, a2);
    }
    template <typename T1, typename T2, typename Result>
    inline void apply(T1 const& lon1_rad, T1 const& lat1_rad,
                      T2 const& lon2_rad, T2 const& lat2_rad,
                      Result& a1) const
    {
        compute<true, false>(lon1_rad, lat1_rad,
                             lon2_rad, lat2_rad,
                             a1, a1);
    }
    template <typename T1, typename T2, typename Result>
    inline void apply_reverse(T1 const& lon1_rad, T1 const& lat1_rad,
                              T2 const& lon2_rad, T2 const& lat2_rad,
                              Result& a2) const
    {
        compute<false, true>(lon1_rad, lat1_rad,
                             lon2_rad, lat2_rad,
                             a2, a2);
    }

private :

    template
    <
        bool EnableAzimuth,
        bool EnableReverseAzimuth,
        typename T1, typename T2, typename Result
    >
    inline void compute(T1 const& lon1_rad, T1 const& lat1_rad,
                        T2 const& lon2_rad, T2 const& lat2_rad,
                        Result& a1, Result& a2) const
    {
        typedef typename result_type<T1, T2>::type calc_t;

        typedef typename FormulaPolicy::template inverse
            <
                calc_t,
                false,
                EnableAzimuth,
                EnableReverseAzimuth,
                false,
                false
            > inverse_type;
        typedef typename inverse_type::result_type inverse_result;
        inverse_result i_res = inverse_type::apply(calc_t(lon1_rad), calc_t(lat1_rad),
                                                   calc_t(lon2_rad), calc_t(lat2_rad),
                                                   m_spheroid);
        if (EnableAzimuth)
        {
            a1 = i_res.azimuth;
        }
        if (EnableReverseAzimuth)
        {
            a2 = i_res.reverse_azimuth;
        }
    }

    Spheroid m_spheroid;
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <>
struct default_strategy<geographic_tag>
{
    typedef strategy::azimuth::geographic
        <
            strategy::andoyer,
            srs::spheroid<double>
        > type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

}} // namespace strategy::azimuth


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_AZIMUTH_HPP

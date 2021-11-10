// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2016-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_AREA_HPP
#define BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_AREA_HPP


#include <type_traits>

#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/formulas/area_formulas.hpp>
#include <boost/geometry/formulas/authalic_radius_sqr.hpp>
#include <boost/geometry/formulas/eccentricity_sqr.hpp>

#include <boost/geometry/strategy/area.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

/*!
\brief Geographic area calculation
\ingroup strategies
\details Geographic area calculation by trapezoidal rule plus integral
         approximation that gives the ellipsoidal correction
\tparam FormulaPolicy Formula used to calculate azimuths
\tparam SeriesOrder The order of approximation of the geodesic integral
\tparam Spheroid The spheroid model
\tparam CalculationType \tparam_calculation
\author See
- Danielsen JS, The area under the geodesic. Surv Rev 30(232): 61–66, 1989
- Charles F.F Karney, Algorithms for geodesics, 2011 https://arxiv.org/pdf/1109.4448.pdf

\qbk{
[heading See also]
\* [link geometry.reference.algorithms.area.area_2_with_strategy area (with strategy)]
\* [link geometry.reference.srs.srs_spheroid srs::spheroid]
}
*/
template
<
    typename FormulaPolicy = strategy::andoyer,
    std::size_t SeriesOrder = strategy::default_order<FormulaPolicy>::value,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
{
    // Switch between two kinds of approximation(series in eps and n v.s.series in k ^ 2 and e'^2)
    static const bool ExpandEpsN = true;
    // LongSegment Enables special handling of long segments
    static const bool LongSegment = false;
    // Area formula is implemented for a maximum series order 5
    static constexpr auto SeriesOrderNorm = SeriesOrder > 5 ? 5 : SeriesOrder;

    //Select default types in case they are not set

public:
    template <typename Geometry>
    struct result_type
        : strategy::area::detail::result_type
            <
                Geometry,
                CalculationType
            >
    {};

protected :
    struct spheroid_constants
    {
        typedef std::conditional_t
            <
                std::is_void<CalculationType>::value,
                typename geometry::radius_type<Spheroid>::type,
                CalculationType
            > calc_t;

        Spheroid m_spheroid;
        calc_t const m_a2;  // squared equatorial radius
        calc_t const m_e2;  // squared eccentricity
        calc_t const m_ep2; // squared second eccentricity
        calc_t const m_ep;  // second eccentricity
        calc_t const m_c2;  // squared authalic radius
        calc_t const m_f;   // the flattening
        calc_t m_coeffs_var[((SeriesOrderNorm+2)*(SeriesOrderNorm+1))/2];

        inline spheroid_constants(Spheroid const& spheroid)
            : m_spheroid(spheroid)
            , m_a2(math::sqr(get_radius<0>(spheroid)))
            , m_e2(formula::eccentricity_sqr<calc_t>(spheroid))
            , m_ep2(m_e2 / (calc_t(1.0) - m_e2))
            , m_ep(math::sqrt(m_ep2))
            , m_c2(formula_dispatch::authalic_radius_sqr
                    <
                        calc_t, Spheroid, srs_spheroid_tag
                    >::apply(m_a2, m_e2))
            , m_f(formula::flattening<calc_t>(spheroid))
        {
            typedef geometry::formula::area_formulas
                <
                    calc_t, SeriesOrderNorm, ExpandEpsN
                > area_formulas;

            calc_t const n = m_f / (calc_t(2) - m_f);

            // Generate and evaluate the polynomials on n
            // to get the series coefficients (that depend on eps)
            area_formulas::evaluate_coeffs_n(n, m_coeffs_var);
        }
    };

public:
    template <typename Geometry>
    class state
    {
        friend class geographic;

        typedef typename result_type<Geometry>::type return_type;

    public:
        inline state()
            : m_excess_sum(0)
            , m_correction_sum(0)
            , m_crosses_prime_meridian(0)
        {}

    private:
        inline return_type area(spheroid_constants const& spheroid_const) const
        {
            return_type result;

            return_type const spherical_term = spheroid_const.m_c2 * m_excess_sum;
            return_type const ellipsoidal_term = spheroid_const.m_e2
                * spheroid_const.m_a2 * m_correction_sum;

            // ignore ellipsoidal term if is large (probably from an azimuth
            // inaccuracy)
            return_type sum = math::abs(ellipsoidal_term/spherical_term) > 0.01
                ? spherical_term : spherical_term + ellipsoidal_term;

            // If encircles some pole
            if (m_crosses_prime_meridian % 2 == 1)
            {
                std::size_t times_crosses_prime_meridian
                        = 1 + (m_crosses_prime_meridian / 2);

                result = return_type(2.0)
                         * geometry::math::pi<return_type>()
                         * spheroid_const.m_c2
                         * return_type(times_crosses_prime_meridian)
                         - geometry::math::abs(sum);

                if (geometry::math::sign<return_type>(sum) == 1)
                {
                    result = - result;
                }

            }
            else
            {
                result = sum;
            }

            return result;
        }

        return_type m_excess_sum;
        return_type m_correction_sum;

        // Keep track if encircles some pole
        std::size_t m_crosses_prime_meridian;
    };

public :
    explicit inline geographic(Spheroid const& spheroid = Spheroid())
        : m_spheroid_constants(spheroid)
    {}

    template <typename PointOfSegment, typename Geometry>
    inline void apply(PointOfSegment const& p1,
                      PointOfSegment const& p2,
                      state<Geometry>& st) const
    {
        using CT = typename result_type<Geometry>::type;

        // if the segment in not on a meridian
        if (! geometry::math::equals(get<0>(p1), get<0>(p2)))
        {
            typedef geometry::formula::area_formulas
                <
                    CT, SeriesOrderNorm, ExpandEpsN
                > area_formulas;

            // Keep track whenever a segment crosses the prime meridian
            if (area_formulas::crosses_prime_meridian(p1, p2))
            {
                st.m_crosses_prime_meridian++;
            }

            // if the segment in not on equator
            if (! (geometry::math::equals(get<1>(p1), 0)
                && geometry::math::equals(get<1>(p2), 0)))
            {
                auto result = area_formulas::template ellipsoidal
                    <
                        FormulaPolicy::template inverse
                    >(p1, p2, m_spheroid_constants);

                st.m_excess_sum += result.spherical_term;
                st.m_correction_sum += result.ellipsoidal_term;
            }
        }
    }

    template <typename Geometry>
    inline typename result_type<Geometry>::type
        result(state<Geometry> const& st) const
    {
        return st.area(m_spheroid_constants);
    }

    Spheroid model() const
    {
        return m_spheroid_constants.m_spheroid;
    }

private:
    spheroid_constants m_spheroid_constants;

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{


template <>
struct default_strategy<geographic_tag>
{
    typedef strategy::area::geographic<> type;
};

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

}

}} // namespace strategy::area




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_GEOGRAPHIC_AREA_HPP

// Boost.Geometry

// Copyright (c) 2018-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_LINE_INTERPOLATE_HPP
#define BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_LINE_INTERPOLATE_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/srs/spheroid.hpp>
#include <boost/geometry/strategies/line_interpolate.hpp>
#include <boost/geometry/strategies/geographic/parameters.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace line_interpolate
{


/*!
\brief Interpolate point on a geographic segment.
\ingroup strategies
\tparam FormulaPolicy The geodesic formulas used internally.
\tparam Spheroid The spheroid model.
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
\* [link geometry.reference.algorithms.line_interpolate.line_interpolate_4_with_strategy line_interpolate (with strategy)]
\* [link geometry.reference.srs.srs_spheroid srs::spheroid]
}
 */
template
<
    typename FormulaPolicy = strategy::andoyer,
    typename Spheroid = srs::spheroid<double>,
    typename CalculationType = void
>
class geographic
{
public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : m_spheroid(spheroid)
    {}

    template <typename Point, typename Fraction, typename Distance>
    inline void apply(Point const& p0,
                      Point const& p1,
                      Fraction const& fraction, //fraction of segment
                      Point & p,
                      Distance const& distance) const
    {
        typedef typename select_calculation_type_alt
            <
                CalculationType,
                Point
            >::type calc_t;

        typedef typename FormulaPolicy::template inverse
                <calc_t, false, true, false, false, false> inverse_t;

        calc_t azimuth = inverse_t::apply(get_as_radian<0>(p0), get_as_radian<1>(p0),
                                          get_as_radian<0>(p1), get_as_radian<1>(p1),
                                          m_spheroid).azimuth;

        typedef typename FormulaPolicy::template direct
                <calc_t, true, false, false, false> direct_t;

        typename direct_t::result_type
        dir_r = direct_t::apply(get_as_radian<0>(p0), get_as_radian<1>(p0),
                                distance * fraction, azimuth,
                                m_spheroid);

        set_from_radian<0>(p, dir_r.lon2);
        set_from_radian<1>(p, dir_r.lat2);
    }

    inline Spheroid model() const
    {
        return m_spheroid;
    }

private:
    Spheroid m_spheroid;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <>
struct default_strategy<geographic_tag>
{
    typedef strategy::line_interpolate::geographic<> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::line_interpolate


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_GEOGRAPHIC_LINE_INTERPOLATE_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2016-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_HPP
#define BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_HPP


#include <boost/geometry/formulas/area_formulas.hpp>
#include <boost/geometry/srs/sphere.hpp>
#include <boost/geometry/strategy/area.hpp>
#include <boost/geometry/strategies/spherical/get_radius.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{


/*!
\brief Spherical area calculation
\ingroup strategies
\details Calculates area on the surface of a sphere using the trapezoidal rule
\tparam RadiusTypeOrSphere \tparam_radius_or_sphere
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.area.area_2_with_strategy area (with strategy)]
}
*/
template
<
    typename RadiusTypeOrSphere = double,
    typename CalculationType = void
>
class spherical
{
    typedef typename strategy_detail::get_radius
        <
            RadiusTypeOrSphere
        >::type radius_type;

    // Enables special handling of long segments
    static const bool LongSegment = false;

public:
    template <typename Geometry>
    struct result_type
        : strategy::area::detail::result_type
            <
                Geometry,
                CalculationType
            >
    {};

    template <typename Geometry>
    class state
    {
        friend class spherical;

        typedef typename result_type<Geometry>::type return_type;

    public:
        inline state()
            : m_sum(0)
            , m_crosses_prime_meridian(0)
        {}

    private:
        template <typename RadiusType>
        inline return_type area(RadiusType const& r) const
        {
            return_type result;
            return_type radius = r;

            // Encircles pole
            if(m_crosses_prime_meridian % 2 == 1)
            {
                size_t times_crosses_prime_meridian
                        = 1 + (m_crosses_prime_meridian / 2);

                result = return_type(2)
                         * geometry::math::pi<return_type>()
                         * times_crosses_prime_meridian
                         - geometry::math::abs(m_sum);

                if(geometry::math::sign<return_type>(m_sum) == 1)
                {
                    result = - result;
                }

            } else {
                result =  m_sum;
            }

            result *= radius * radius;

            return result;
        }

        return_type m_sum;

        // Keep track if encircles some pole
        size_t m_crosses_prime_meridian;
    };

public :

    // For backward compatibility reasons the radius is set to 1
    inline spherical()
        : m_radius(1.0)
    {}

    template <typename RadiusOrSphere>
    explicit inline spherical(RadiusOrSphere const& radius_or_sphere)
        : m_radius(strategy_detail::get_radius
                    <
                        RadiusOrSphere
                    >::apply(radius_or_sphere))
    {}

    template <typename PointOfSegment, typename Geometry>
    inline void apply(PointOfSegment const& p1,
                      PointOfSegment const& p2,
                      state<Geometry>& st) const
    {
        if (! geometry::math::equals(get<0>(p1), get<0>(p2)))
        {
            typedef geometry::formula::area_formulas
                <
                    typename result_type<Geometry>::type
                > area_formulas;

            st.m_sum += area_formulas::template spherical<LongSegment>(p1, p2);

            // Keep track whenever a segment crosses the prime meridian
            if (area_formulas::crosses_prime_meridian(p1, p2))
            {
                st.m_crosses_prime_meridian++;
            }
        }
    }

    template <typename Geometry>
    inline typename result_type<Geometry>::type
        result(state<Geometry> const& st) const
    {
        return st.area(m_radius);
    }

    srs::sphere<radius_type> model() const
    {
        return srs::sphere<radius_type>(m_radius);
    }

private :
    radius_type m_radius;
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{


template <>
struct default_strategy<spherical_equatorial_tag>
{
    typedef strategy::area::spherical<> type;
};

// Note: spherical polar coordinate system requires "get_as_radian_equatorial"
template <>
struct default_strategy<spherical_polar_tag>
{
    typedef strategy::area::spherical<> type;
};

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::area




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGY_SPHERICAL_AREA_HPP

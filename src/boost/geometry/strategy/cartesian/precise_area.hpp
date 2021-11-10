// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_PRECISE_AREA_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_PRECISE_AREA_HPP

#include <boost/mpl/if.hpp>

//#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategy/area.hpp>
#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/util/precise_math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace area
{

/*!
\brief Cartesian area calculation
\ingroup strategies
\details Calculates cartesian area using the trapezoidal rule and precise
         summation (useful to increase precision with floating point arithmetic)
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.area.area_2_with_strategy area (with strategy)]
}

*/
template
<
    typename CalculationType = void
>
class precise_cartesian
{
public :
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
        friend class precise_cartesian;

        typedef typename result_type<Geometry>::type return_type;

    public:
        inline state()
            : sum1(0)
            , sum2(0)
        {
            // Strategy supports only 2D areas
            assert_dimension<Geometry, 2>();
        }

    private:
        inline return_type area() const
        {
            return_type const two = 2;
            return (sum1 + sum2) / two;
        }

        return_type sum1;
        return_type sum2;
    };

    template <typename PointOfSegment, typename Geometry>
    static inline void apply(PointOfSegment const& p1,
                             PointOfSegment const& p2,
                             state<Geometry>& st)
    {
        typedef typename state<Geometry>::return_type return_type;

        auto const det = (return_type(get<0>(p1)) + return_type(get<0>(p2)))
                * (return_type(get<1>(p1)) - return_type(get<1>(p2)));

        auto const res = boost::geometry::detail::precise_math::two_sum(st.sum1, det);

        st.sum1 = res[0];
        st.sum2 += res[1];
    }

    template <typename Geometry>
    static inline auto result(state<Geometry>& st)
    {
        return st.area();
    }

};


}} // namespace strategy::area



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_PRECISE_AREA_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2016-2020.
// Modifications copyright (c) 2016-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_HPP
#define BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_HPP


//#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategy/area.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

/*!
\brief Cartesian area calculation
\ingroup strategies
\details Calculates cartesian area using the trapezoidal rule
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
class cartesian
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
        friend class cartesian;

        typedef typename result_type<Geometry>::type return_type;

    public:        
        inline state()
            : sum(0)
        {
            // Strategy supports only 2D areas
            assert_dimension<Geometry, 2>();
        }

    private:
        inline return_type area() const
        {
            return_type const two = 2;
            return sum / two;
        }

        return_type sum;
    };

    template <typename PointOfSegment, typename Geometry>
    static inline void apply(PointOfSegment const& p1,
                             PointOfSegment const& p2,
                             state<Geometry>& st)
    {
        typedef typename state<Geometry>::return_type return_type;

        // Below formulas are equivalent, however the two lower ones
        // suffer less from accuracy loss for great values of coordinates.
        // See: https://svn.boost.org/trac/boost/ticket/11928

        // SUM += x2 * y1 - x1 * y2;
        // state.sum += detail::determinant<return_type>(p2, p1);

        // SUM += (x2 - x1) * (y2 + y1)
        //state.sum += (return_type(get<0>(p2)) - return_type(get<0>(p1)))
        //           * (return_type(get<1>(p2)) + return_type(get<1>(p1)));

        // SUM += (x1 + x2) * (y1 - y2)
        st.sum += (return_type(get<0>(p1)) + return_type(get<0>(p2)))
                * (return_type(get<1>(p1)) - return_type(get<1>(p2)));
    }

    template <typename Geometry>
    static inline auto result(state<Geometry>& st)
    {
        return st.area();
    }

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{
    template <>
    struct default_strategy<cartesian_tag>
    {
        typedef strategy::area::cartesian<> type;
    };

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::area



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_CARTESIAN_AREA_HPP

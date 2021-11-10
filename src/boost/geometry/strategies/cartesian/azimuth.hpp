// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2016-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fisikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AZIMUTH_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AZIMUTH_HPP

#include <cmath>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/azimuth.hpp>

#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace azimuth
{

template <typename CalculationType = void>
class cartesian
{
public:
    template <typename T1, typename T2>
    struct result_type
        : geometry::select_most_precise
              <
                  // NOTE: this promotes any integer type to double
                  typename geometry::promote_floating_point<T1, double>::type,
                  typename geometry::promote_floating_point<T2, double>::type,
                  CalculationType
              >
    {};

    template <typename T1, typename T2, typename Result>
    static inline void apply(T1 const& x1, T1 const& y1,
                             T2 const& x2, T2 const& y2,
                             Result& a1, Result& a2)
    {
        compute(x1, y1, x2, y2, a1, a2);
    }
    template <typename T1, typename T2, typename Result>
    static inline void apply(T1 const& x1, T1 const& y1,
                             T2 const& x2, T2 const& y2,
                             Result& a1)
    {
        compute(x1, y1, x2, y2, a1, a1);
    }
    template <typename T1, typename T2, typename Result>
    static inline void apply_reverse(T1 const& x1, T1 const& y1,
                                     T2 const& x2, T2 const& y2,
                                     Result& a2)
    {
        compute(x1, y1, x2, y2, a2, a2);
    }

private:
    template <typename T1, typename T2, typename Result>
    static inline void compute(T1 const& x1, T1 const& y1,
                               T2 const& x2, T2 const& y2,
                               Result& a1, Result& a2)
    {
        typedef typename result_type<T1, T2>::type calc_t;

        // NOTE: azimuth 0 is at Y axis, increasing right
        // as in spherical/geographic where 0 is at North axis
        a1 = a2 = atan2(calc_t(x2) - calc_t(x1), calc_t(y2) - calc_t(y1));
    }
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <>
struct default_strategy<cartesian_tag>
{
    typedef strategy::azimuth::cartesian<> type;
};

}

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

}} // namespace strategy::azimuth


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AZIMUTH_HPP

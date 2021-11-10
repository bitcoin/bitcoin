// Boost.Geometry

// Copyright (c) 2019-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_IO_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_IO_CARTESIAN_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/io/services.hpp>

#include <boost/geometry/strategies/cartesian/point_order.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/cartesian/point_in_poly_winding.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace io
{

template <typename CalculationType = void>
class cartesian
    : public strategies::detail::cartesian_base
{
public:
    static auto point_order()
    {
        return strategy::point_order::cartesian<CalculationType>();
    }

    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && util::is_pointlike<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::cartesian_point_point();
    }

    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && ( util::is_linear<Geometry2>::value
                               || util::is_polygonal<Geometry2>::value )
                            > * = nullptr)
    {
        return strategy::within::cartesian_winding<void, void, CalculationType>();
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, cartesian_tag>
{
    typedef cartesian<> type;
};

} // namespace services

}} // namespace strategies::io

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_IO_CARTESIAN_HPP

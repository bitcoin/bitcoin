// Boost.Geometry

// Copyright (c) 2019-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_IO_GEOGRAPHIC_HPP
#define BOOST_GEOMETRY_STRATEGIES_IO_GEOGRAPHIC_HPP


#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/io/services.hpp>

#include <boost/geometry/strategies/geographic/point_order.hpp>
#include <boost/geometry/strategies/geographic/point_in_poly_winding.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>


namespace boost { namespace geometry
{

namespace strategies { namespace io
{

template
    <
        typename FormulaPolicy = strategy::andoyer,
        typename Spheroid = srs::spheroid<double>,
        typename CalculationType = void
    >
class geographic
    : public strategies::detail::geographic_base<Spheroid>
{
    using base_t = strategies::detail::geographic_base<Spheroid>;

public:
    geographic() = default;

    explicit geographic(Spheroid const& spheroid)
        : base_t(spheroid)
    {}

    auto point_order() const
    {
        return strategy::point_order::geographic
                <
                    FormulaPolicy, Spheroid, CalculationType
                >(base_t::m_spheroid);
    }

    template <typename Geometry1, typename Geometry2>
    static auto relate(Geometry1 const&, Geometry2 const&,
                       std::enable_if_t
                            <
                                util::is_pointlike<Geometry1>::value
                             && util::is_pointlike<Geometry2>::value
                            > * = nullptr)
    {
        return strategy::within::spherical_point_point();
    }

    template <typename Geometry1, typename Geometry2>
    auto relate(Geometry1 const&, Geometry2 const&,
                std::enable_if_t
                    <
                        util::is_pointlike<Geometry1>::value
                        && ( util::is_linear<Geometry2>::value
                        || util::is_polygonal<Geometry2>::value )
                    > * = nullptr) const
    {
        return strategy::within::geographic_winding
            <
                void, void,
                FormulaPolicy, Spheroid, CalculationType
            >(base_t::m_spheroid);
    }
};

namespace services
{

template <typename Geometry>
struct default_strategy<Geometry, geographic_tag>
{
    typedef geographic<> type;
};

} // namespace services

}} // namespace strategies::io

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_IO_GEOGRAPHIC_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2020 Digvijay Janartha, Hamirpur, India.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_POINT_XYZ_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POINT_XYZ_HPP

#include <cstddef>
#include <type_traits>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>

namespace boost { namespace geometry
{

namespace model { namespace d3
{

/*!
\brief 3D point in Cartesian coordinate system
\tparam CoordinateType numeric type, for example, double, float, int
\tparam CoordinateSystem coordinate system, defaults to cs::cartesian

\qbk{[include reference/geometries/point_xyz.qbk]}
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_point Point Concept]
}

\qbk{[include reference/geometries/point_assign_warning.qbk]}

*/
template<typename CoordinateType, typename CoordinateSystem = cs::cartesian>
class point_xyz : public model::point<CoordinateType, 3, CoordinateSystem>
{
public:
    /// \constructor_default_no_init
    constexpr point_xyz() = default;

    /// Constructor with x/y/z values
    constexpr point_xyz(CoordinateType const& x, CoordinateType const& y, CoordinateType const& z)
        : model::point<CoordinateType, 3, CoordinateSystem>(x, y, z)
    {}

    /// Get x-value
    constexpr CoordinateType const& x() const
    { return this->template get<0>(); }

    /// Get y-value
    constexpr CoordinateType const& y() const
    { return this->template get<1>(); }

    /// Get z-value
    constexpr CoordinateType const& z() const
    { return this->template get<2>(); }

    /// Set x-value
    void x(CoordinateType const& v)
    { this->template set<0>(v); }

    /// Set y-value
    void y(CoordinateType const& v)
    { this->template set<1>(v); }
    
    /// Set z-value
    void z(CoordinateType const& v)
    { this->template set<2>(v); }
};


}} // namespace model::d3


// Adapt the point_xyz to the concept
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType, typename CoordinateSystem>
struct tag<model::d3::point_xyz<CoordinateType, CoordinateSystem> >
{
    typedef point_tag type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct coordinate_type<model::d3::point_xyz<CoordinateType, CoordinateSystem> >
{
    typedef CoordinateType type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct coordinate_system<model::d3::point_xyz<CoordinateType, CoordinateSystem> >
{
    typedef CoordinateSystem type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct dimension<model::d3::point_xyz<CoordinateType, CoordinateSystem> >
    : std::integral_constant<std::size_t, 3>
{};

template<typename CoordinateType, typename CoordinateSystem, std::size_t Dimension>
struct access<model::d3::point_xyz<CoordinateType, CoordinateSystem>, Dimension>
{
    static constexpr CoordinateType get(
        model::d3::point_xyz<CoordinateType, CoordinateSystem> const& p)
    {
        return p.template get<Dimension>();
    }

    static void set(model::d3::point_xyz<CoordinateType, CoordinateSystem>& p,
        CoordinateType const& value)
    {
        p.template set<Dimension>(value);
    }
};

template<typename CoordinateType, typename CoordinateSystem>
struct make<model::d3::point_xyz<CoordinateType, CoordinateSystem> >
{
    typedef model::d3::point_xyz<CoordinateType, CoordinateSystem> point_type;

    static const bool is_specialized = true;

    static constexpr point_type apply(CoordinateType const& x,
                                      CoordinateType const& y,
                                      CoordinateType const& z)
    {
        return point_type(x, y, z);
    }
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POINT_XYZ_HPP

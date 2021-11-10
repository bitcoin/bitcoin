// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_POINT_XY_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POINT_XY_HPP

#include <cstddef>
#include <type_traits>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>

namespace boost { namespace geometry
{

namespace model { namespace d2
{

/*!
\brief 2D point in Cartesian coordinate system
\tparam CoordinateType numeric type, for example, double, float, int
\tparam CoordinateSystem coordinate system, defaults to cs::cartesian

\qbk{[include reference/geometries/point_xy.qbk]}
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_point Point Concept]
}

\qbk{[include reference/geometries/point_assign_warning.qbk]}

*/
template<typename CoordinateType, typename CoordinateSystem = cs::cartesian>
class point_xy : public model::point<CoordinateType, 2, CoordinateSystem>
{
public:
    /// \constructor_default_no_init
    constexpr point_xy() = default;

    /// Constructor with x/y values
    constexpr point_xy(CoordinateType const& x, CoordinateType const& y)
        : model::point<CoordinateType, 2, CoordinateSystem>(x, y)
    {}

    /// Get x-value
    constexpr CoordinateType const& x() const
    { return this->template get<0>(); }

    /// Get y-value
    constexpr CoordinateType const& y() const
    { return this->template get<1>(); }

    /// Set x-value
    void x(CoordinateType const& v)
    { this->template set<0>(v); }

    /// Set y-value
    void y(CoordinateType const& v)
    { this->template set<1>(v); }
};


}} // namespace model::d2


// Adapt the point_xy to the concept
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType, typename CoordinateSystem>
struct tag<model::d2::point_xy<CoordinateType, CoordinateSystem> >
{
    typedef point_tag type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct coordinate_type<model::d2::point_xy<CoordinateType, CoordinateSystem> >
{
    typedef CoordinateType type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct coordinate_system<model::d2::point_xy<CoordinateType, CoordinateSystem> >
{
    typedef CoordinateSystem type;
};

template<typename CoordinateType, typename CoordinateSystem>
struct dimension<model::d2::point_xy<CoordinateType, CoordinateSystem> >
    : std::integral_constant<std::size_t, 2>
{};

template<typename CoordinateType, typename CoordinateSystem, std::size_t Dimension>
struct access<model::d2::point_xy<CoordinateType, CoordinateSystem>, Dimension >
{
    static constexpr CoordinateType get(
        model::d2::point_xy<CoordinateType, CoordinateSystem> const& p)
    {
        return p.template get<Dimension>();
    }

    static void set(model::d2::point_xy<CoordinateType, CoordinateSystem>& p,
        CoordinateType const& value)
    {
        p.template set<Dimension>(value);
    }
};

template<typename CoordinateType, typename CoordinateSystem>
struct make<model::d2::point_xy<CoordinateType, CoordinateSystem> >
{
    typedef model::d2::point_xy<CoordinateType, CoordinateSystem> point_type;

    static const bool is_specialized = true;

    static constexpr point_type apply(CoordinateType const& x,
                                      CoordinateType const& y)
    {
        return point_type(x, y);
    }
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POINT_XY_HPP

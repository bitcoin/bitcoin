// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_POINT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POINT_HPP

#include <cstddef>
#include <type_traits>

#include <boost/static_assert.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/make.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
#include <algorithm>
#include <boost/geometry/core/assert.hpp>
#endif

namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

namespace detail
{

template <typename Dummy, std::size_t N, std::size_t DimensionCount>
struct is_coordinates_number_leq
{
    static const bool value = (N <= DimensionCount);
};

template <typename Dummy, std::size_t N, std::size_t DimensionCount>
struct is_coordinates_number_eq
{
    static const bool value = (N == DimensionCount);
};

} // namespace detail


namespace model
{

/*!
\brief Basic point class, having coordinates defined in a neutral way
\details Defines a neutral point class, fulfilling the Point Concept.
    Library users can use this point class, or use their own point classes.
    This point class is used in most of the samples and tests of Boost.Geometry
    This point class is used occasionally within the library, where a temporary
    point class is necessary.
\ingroup geometries
\tparam CoordinateType \tparam_numeric
\tparam DimensionCount number of coordinates, usually 2 or 3
\tparam CoordinateSystem coordinate system, for example cs::cartesian

\qbk{[include reference/geometries/point.qbk]}
\qbk{before.synopsis, [heading Model of]}
\qbk{before.synopsis, [link geometry.reference.concepts.concept_point Point Concept]}


*/
template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
class point
{
    BOOST_STATIC_ASSERT(DimensionCount > 0);

    // The following enum is used to fully instantiate the
    // CoordinateSystem class and check the correctness of the units
    // passed for non-Cartesian coordinate systems.
    enum { cs_check = sizeof(CoordinateSystem) };

public:

    // TODO: constexpr requires LiteralType and until C++20
    // it has to have trivial destructor which makes access
    // debugging impossible with constexpr.

#if !defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    /// \constructor_default_no_init
    constexpr point()
// Workaround for VS2015
#if defined(_MSC_VER) && (_MSC_VER < 1910)
        : m_values{} {}
#else
        = default;
#endif
#else // defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    point()
    {
        m_created = 1;
        std::fill_n(m_values_initialized, DimensionCount, 0);
    }
    ~point()
    {
        m_created = 0;
        std::fill_n(m_values_initialized, DimensionCount, 0);
    }
#endif

    /// @brief Constructor to set one value
    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_leq<C, 1, DimensionCount>::value, int> = 0
    >
#if ! defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    constexpr
#endif
    explicit point(CoordinateType const& v0)
        : m_values{v0}
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        m_created = 1;
        std::fill_n(m_values_initialized, DimensionCount, 1);
#endif
    }

    /// @brief Constructor to set two values
    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_leq<C, 2, DimensionCount>::value, int> = 0
    >
#if ! defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    constexpr
#endif
    point(CoordinateType const& v0, CoordinateType const& v1)
        : m_values{ v0, v1 }
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        m_created = 1;
        std::fill_n(m_values_initialized, DimensionCount, 1);
#endif
    }

    /// @brief Constructor to set three values
    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_leq<C, 3, DimensionCount>::value, int> = 0
    >
#if ! defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    constexpr
#endif
    point(CoordinateType const& v0, CoordinateType const& v1, CoordinateType const& v2)
        : m_values{ v0, v1, v2 }
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        m_created = 1;
        std::fill_n(m_values_initialized, DimensionCount, 1);
#endif
    }

    /// @brief Get a coordinate
    /// @tparam K coordinate to get
    /// @return the coordinate
    template <std::size_t K>
#if ! defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    constexpr
#endif
    CoordinateType const& get() const
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        BOOST_GEOMETRY_ASSERT(m_created == 1);
        BOOST_GEOMETRY_ASSERT(m_values_initialized[K] == 1);
#endif
        BOOST_STATIC_ASSERT(K < DimensionCount);
        return m_values[K];
    }

    /// @brief Set a coordinate
    /// @tparam K coordinate to set
    /// @param value value to set
    template <std::size_t K>
    void set(CoordinateType const& value)
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        BOOST_GEOMETRY_ASSERT(m_created == 1);
        m_values_initialized[K] = 1;
#endif
        BOOST_STATIC_ASSERT(K < DimensionCount);
        m_values[K] = value;
    }

private:

    CoordinateType m_values[DimensionCount];

#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    int m_created;
    int m_values_initialized[DimensionCount];
#endif
};


} // namespace model

// Adapt the point to the concept
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{
template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct tag<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef point_tag type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct coordinate_type<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef CoordinateType type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct coordinate_system<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef CoordinateSystem type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct dimension<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
    : std::integral_constant<std::size_t, DimensionCount>
{};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem,
    std::size_t Dimension
>
struct access<model::point<CoordinateType, DimensionCount, CoordinateSystem>, Dimension>
{
    static constexpr CoordinateType get(
        model::point<CoordinateType, DimensionCount, CoordinateSystem> const& p)
    {
        return p.template get<Dimension>();
    }

    static void set(
        model::point<CoordinateType, DimensionCount, CoordinateSystem>& p,
        CoordinateType const& value)
    {
        p.template set<Dimension>(value);
    }
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct make<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef model::point<CoordinateType, DimensionCount, CoordinateSystem> point_type;

    static const bool is_specialized = true;

    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_eq<C, 1, DimensionCount>::value, int> = 0
    >
    static constexpr point_type apply(CoordinateType const& v0)
    {
        return point_type(v0);
    }

    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_eq<C, 2, DimensionCount>::value, int> = 0
    >
    static constexpr point_type apply(CoordinateType const& v0,
                                      CoordinateType const& v1)
    {
        return point_type(v0, v1);
    }

    template
    <
        typename C = CoordinateType,
        std::enable_if_t<geometry::detail::is_coordinates_number_eq<C, 3, DimensionCount>::value, int> = 0
    >
    static constexpr point_type apply(CoordinateType const& v0,
                                      CoordinateType const& v1,
                                      CoordinateType const& v2)
    {
        return point_type(v0, v1, v2);
    }
};


} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POINT_HPP

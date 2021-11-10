// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_CORE_VISIT_HPP
#define BOOST_GEOMETRY_CORE_VISIT_HPP

#include <utility>

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry { namespace traits
{

// TODO: Alternatives:
// - free function
//    template <typename Visitor, typename ...Variants>
//    auto visit(Visitor &&, Variants && ...) {}
//
// - additional Enable tparam
//    template <bool Enable, typename ...DynamicGeometries>
//    struct visit {};

template <typename ...DynamicGeometries>
struct visit
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these DynamicGeometries types.",
        DynamicGeometries...);
};

// By default call 1-parameter visit for each geometry
template <typename DynamicGeometry1, typename DynamicGeometry2>
struct visit<DynamicGeometry1, DynamicGeometry2>
{
    template <typename Function, typename Variant1, typename Variant2>
    static void apply(Function && function, Variant1 && variant1, Variant2 && variant2)
    {
        visit<util::remove_cref_t<Variant1>>::apply([&](auto && g1)
        {
            using ref1_t = decltype(g1);
            visit<util::remove_cref_t<Variant2>>::apply([&](auto && g2)
            {
                function(std::forward<ref1_t>(g1),
                         std::forward<decltype(g2)>(g2));
            }, std::forward<Variant2>(variant2));
        }, std::forward<Variant1>(variant1));
    }
};

// By default treat GeometryCollection as a range of DynamicGeometries
template <typename GeometryCollection>
struct iter_visit
{
    template <typename Function, typename Iterator>
    static void apply(Function && function, Iterator iterator)
    {
        using value_t = typename boost::range_value<GeometryCollection>::type;
        using reference_t = typename std::iterator_traits<Iterator>::reference;
        visit<value_t>::apply(std::forward<Function>(function),
                              std::forward<reference_t>(*iterator));
    }
};

}}} // namespace boost::geometry::traits

#endif // BOOST_GEOMETRY_CORE_VISIT_HPP

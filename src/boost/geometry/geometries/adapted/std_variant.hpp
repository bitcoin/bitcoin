// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_STD_VARIANT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_STD_VARIANT_HPP


#include <boost/config.hpp>

#ifndef BOOST_NO_CXX17_HDR_VARIANT


#include <utility>
#include <variant>

#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>
#include <boost/geometry/util/sequence.hpp>


namespace boost { namespace geometry
{

namespace traits
{

template <typename ...Ts>
struct tag<std::variant<Ts...>>
{
    using type = dynamic_geometry_tag;
};

template <typename ...Ts>
struct visit<std::variant<Ts...>>
{
    template <typename Function, typename Variant>
    static void apply(Function && function, Variant && variant)
    {
        std::visit(std::forward<Function>(function),
                   std::forward<Variant>(variant));
    }
};

template <typename ...Ts, typename ...Us>
struct visit<std::variant<Ts...>, std::variant<Us...>>
{
    template <typename Function, typename Variant1, typename Variant2>
    static void apply(Function && function, Variant1 && variant1, Variant2 && variant2)
    {
        std::visit(std::forward<Function>(function),
                   std::forward<Variant1>(variant1),
                   std::forward<Variant2>(variant2));
    }
};

template <typename ...Ts>
struct geometry_types<std::variant<Ts...>>
{
    using type = util::type_sequence<Ts...>;
};


} // namespace traits


}} // namespace boost::geometry


#endif // BOOST_NO_CXX17_HDR_VARIANT


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_STD_VARIANT_HPP

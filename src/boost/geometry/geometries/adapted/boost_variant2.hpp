// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT2_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT2_HPP


#include <utility>

#include <boost/variant2/variant.hpp>

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
struct tag<boost::variant2::variant<Ts...>>
{
    using type = dynamic_geometry_tag;
};

template <typename ...Ts>
struct visit<boost::variant2::variant<Ts...>>
{
    template <typename Function, typename Variant>
    static void apply(Function && function, Variant && variant)
    {
        boost::variant2::visit(std::forward<Function>(function),
                               std::forward<Variant>(variant));
    }
};

template <typename ...Ts, typename ...Us>
struct visit<boost::variant2::variant<Ts...>, boost::variant2::variant<Us...>>
{
    template <typename Function, typename Variant1, typename Variant2>
    static void apply(Function && function, Variant1 && variant1, Variant2 && variant2)
    {
        boost::variant2::visit(std::forward<Function>(function),
                               std::forward<Variant1>(variant1),
                               std::forward<Variant2>(variant2));
    }
};

template <typename ...Ts>
struct geometry_types<boost::variant2::variant<Ts...>>
{
    using type = util::type_sequence<Ts...>;
};


} // namespace traits


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT2_HPP

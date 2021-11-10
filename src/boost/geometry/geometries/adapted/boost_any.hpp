// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ANY_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ANY_HPP


#include <utility>

#include <boost/any.hpp>

#include <boost/geometry/geometries/adapted/detail/any.hpp>

#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>


namespace boost { namespace geometry
{

namespace detail
{


struct boost_any_cast_policy
{
    template <typename T, typename Any>
    static inline T * apply(Any * any_ptr)
    {
        return boost::any_cast<T>(any_ptr);
    }
};


} // namespace detail

namespace traits
{

template <>
struct tag<boost::any>
{
    using type = dynamic_geometry_tag;
};

template <>
struct visit<boost::any>
{
    template <typename Function, typename Any>
    static void apply(Function && function, Any && any)
    {
        using types_t = typename geometry_types<util::remove_cref_t<Any>>::type;
        geometry::detail::visit_any
            <
                geometry::detail::boost_any_cast_policy, types_t
            >::template apply<0>(std::forward<Function>(function), std::forward<Any>(any));
    }
};


} // namespace traits


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_ANY_HPP

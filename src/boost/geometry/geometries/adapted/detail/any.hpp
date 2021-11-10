// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_DETAIL_ANY_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_DETAIL_ANY_HPP


#include <utility>

#include <boost/geometry/util/sequence.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry
{

namespace detail
{

template
<
    typename CastPolicy,
    typename TypeSequence,
    std::size_t N = util::sequence_size<TypeSequence>::value
>
struct visit_any
{
    static const std::size_t M = N / 2;

    template <std::size_t Offset, typename Function, typename Any>
    static bool apply(Function && function, Any && any)
    {
        return visit_any<CastPolicy, TypeSequence, M>::template apply<Offset>(
                    std::forward<Function>(function), std::forward<Any>(any))
            || visit_any<CastPolicy, TypeSequence, N - M>::template apply<Offset + M>(
                    std::forward<Function>(function), std::forward<Any>(any));
    }
};

template <typename CastPolicy, typename TypeSequence>
struct visit_any<CastPolicy, TypeSequence, 1>
{
    template <std::size_t Offset, typename Function, typename Any>
    static bool apply(Function && function, Any && any)
    {
        using elem_t = typename util::sequence_element<Offset, TypeSequence>::type;
        using geom_t = util::transcribe_const_t<std::remove_reference_t<Any>, elem_t>;
        geom_t * g = CastPolicy::template apply<geom_t>(&any);
        if (g != nullptr)
        {
            using geom_ref_t = std::conditional_t
                <
                    std::is_rvalue_reference<decltype(any)>::value,
                    geom_t&&, geom_t&
                >;
            function(static_cast<geom_ref_t>(*g));
            return true;
        }
        else
        {
            return false;
        }
    }
};

template <typename CastPolicy, typename TypeSequence>
struct visit_any<CastPolicy, TypeSequence, 0>
{
    template <std::size_t Offset, typename Function, typename Any>
    static bool apply(Function &&, Any &&)
    {
        return false;
    }
};

} // namespace detail

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_DETAIL_ANY_HPP

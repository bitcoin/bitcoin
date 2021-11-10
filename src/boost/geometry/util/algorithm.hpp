// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_ALGORITHM_HPP
#define BOOST_GEOMETRY_UTIL_ALGORITHM_HPP


#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/util/type_traits_std.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// Other implementations can be found in the history of this file.
// The discussion and benchmarks can be found here:
// https://github.com/boostorg/geometry/pull/827

// O(logN) version 2

template <std::size_t N>
struct for_each_index_impl2
{
    static const std::size_t N1 = N / 2;
    static const std::size_t N2 = N - N1;

    template <std::size_t Offset, typename UnaryFunction>
    constexpr static inline void apply(UnaryFunction& function)
    {
        for_each_index_impl2<N1>::template apply<Offset>(function);
        for_each_index_impl2<N2>::template apply<Offset + N1>(function);
    }
};

template <>
struct for_each_index_impl2<3>
{
    template <std::size_t Offset, typename UnaryFunction>
    constexpr static inline void apply(UnaryFunction& function)
    {
        function(util::index_constant<Offset>());
        function(util::index_constant<Offset + 1>());
        function(util::index_constant<Offset + 2>());
    }
};

template <>
struct for_each_index_impl2<2>
{
    template <std::size_t Offset, typename UnaryFunction>
    constexpr static inline void apply(UnaryFunction& function)
    {
        function(util::index_constant<Offset>());
        function(util::index_constant<Offset + 1>());
    }
};

template <>
struct for_each_index_impl2<1>
{
    template <std::size_t Offset, typename UnaryFunction>
    constexpr static inline void apply(UnaryFunction& function)
    {
        function(util::index_constant<Offset>());
    }
};

template <>
struct for_each_index_impl2<0>
{
    template <std::size_t Offset, typename UnaryFunction>
    constexpr static inline void apply(UnaryFunction& )
    {}
};

// Interface

template <std::size_t N, typename UnaryFunction>
constexpr inline UnaryFunction for_each_index(UnaryFunction function)
{
    for_each_index_impl2
        <
            N
        >::template apply<0>(function);
    return function;
}

template <typename Geometry, typename UnaryFunction>
constexpr inline UnaryFunction for_each_dimension(UnaryFunction function)
{
    for_each_index_impl2
        <
            geometry::dimension<Geometry>::value
        >::template apply<0>(function);
    return function;
}

// ----------------------------------------------------------------------------

// O(logN) version 2

template <std::size_t N>
struct all_indexes_of_impl2
{
    static const std::size_t N1 = N / 2;
    static const std::size_t N2 = N - N1;

    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return all_indexes_of_impl2<N1>::template apply<Offset>(predicate)
            && all_indexes_of_impl2<N2>::template apply<Offset + N1>(predicate);
    }
};

template <>
struct all_indexes_of_impl2<3>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>())
            && predicate(util::index_constant<Offset + 1>())
            && predicate(util::index_constant<Offset + 2>());
    }
};

template <>
struct all_indexes_of_impl2<2>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>())
            && predicate(util::index_constant<Offset + 1>());
    }
};

template <>
struct all_indexes_of_impl2<1>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>());
    }
};

template <>
struct all_indexes_of_impl2<0>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& )
    {
        return true;
    }
};

// Interface

template <std::size_t N, typename UnaryPredicate>
constexpr inline bool all_indexes_of(UnaryPredicate predicate)
{
    return all_indexes_of_impl2<N>::template apply<0>(predicate);
}

template <typename Geometry, typename UnaryPredicate>
constexpr inline bool all_dimensions_of(UnaryPredicate predicate)
{
    return all_indexes_of_impl2
        <
            geometry::dimension<Geometry>::value
        >::template apply<0>(predicate);
}

// ----------------------------------------------------------------------------

// O(logN) version 2

template <std::size_t N>
struct any_index_of_impl2
{
    static const std::size_t N1 = N / 2;
    static const std::size_t N2 = N - N1;

    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return any_index_of_impl2<N1>::template apply<Offset>(predicate)
            || any_index_of_impl2<N2>::template apply<Offset + N1>(predicate);
    }
};

template <>
struct any_index_of_impl2<3>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>())
            || predicate(util::index_constant<Offset + 1>())
            || predicate(util::index_constant<Offset + 2>());
    }
};

template <>
struct any_index_of_impl2<2>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>())
            || predicate(util::index_constant<Offset + 1>());
    }
};

template <>
struct any_index_of_impl2<1>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& predicate)
    {
        return predicate(util::index_constant<Offset>());
    }
};

template <>
struct any_index_of_impl2<0>
{
    template <std::size_t Offset, typename UnaryPredicate>
    constexpr static inline bool apply(UnaryPredicate& )
    {
        return false;
    }
};

// Interface

template <std::size_t N, typename UnaryPredicate>
constexpr inline bool any_index_of(UnaryPredicate predicate)
{
    return any_index_of_impl2<N>::template apply<0>(predicate);
}

template <typename Geometry, typename UnaryPredicate>
constexpr inline bool any_dimension_of(UnaryPredicate predicate)
{
    return any_index_of_impl2
        <
            geometry::dimension<Geometry>::value
        >::template apply<0>(predicate);
}

template <std::size_t N, typename UnaryPredicate>
constexpr inline bool none_index_of(UnaryPredicate predicate)
{
    return ! any_index_of_impl2<N>::template apply<0>(predicate);
}

template <typename Geometry, typename UnaryPredicate>
constexpr inline bool none_dimension_of(UnaryPredicate predicate)
{
    return ! any_index_of_impl2
        <
            geometry::dimension<Geometry>::value
        >::template apply<0>(predicate);
}

// ----------------------------------------------------------------------------


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_ALGORITHM_HPP

//
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_VALUE_TRAITS_HPP
#define BOOST_JSON_DETAIL_VALUE_TRAITS_HPP

#include <boost/json/detail/config.hpp>
#include <type_traits>
#include <utility>

BOOST_JSON_NS_BEGIN
namespace detail {

template<std::size_t N>
struct priority_tag
    : priority_tag<N - 1> { };

template<>
struct priority_tag<0> { };

using std::begin;
using std::end;
#ifdef __cpp_lib_nonmember_container_access
using std::size;
#endif

template<typename T, typename = void>
struct container_traits
{
    static constexpr bool is_container = false;
};

template<typename T>
struct container_traits<T, typename std::enable_if<
    std::is_same<decltype(begin(std::declval<T&>())),
        decltype(end(std::declval<T&>()))>::value>::type>
{
private:
    template<typename U, typename std::enable_if<
        std::is_convertible<decltype(std::declval<U&>().size()),
            std::size_t>::value>::type* = nullptr>
    static
    std::size_t
    size_impl(
        U&& cont,
        priority_tag<2>)
    {
        return cont.size();
    }

    template<typename U, typename std::enable_if<
        std::is_convertible<decltype(size(std::declval<U&>())),
            std::size_t>::value>::type* = nullptr>
    static
    std::size_t
    size_impl(
        U& cont,
        priority_tag<1>)
    {
        return size(cont);
    }

    template<typename U, std::size_t N>
    static
    std::size_t
    size_impl(
        U(&)[N],
        priority_tag<1>)
    {
        return N;
    }

    template<typename U>
    static
    std::size_t
    size_impl(U&, priority_tag<0>)
    {
        return 0;
    }

public:
    static constexpr bool is_container = true;
    using value_type = remove_cvref<
        decltype(*begin(std::declval<T&>()))>;

    template<typename U>
    static
    std::size_t
    try_size(U& cont)
    {
        return container_traits::size_impl(
            cont, priority_tag<2>());
    }
};

template<typename T, typename = void>
struct map_traits
{
    static constexpr bool is_map = false;
    static constexpr bool has_unique_keys = false;
};

template<typename T>
struct map_traits<T, void_t<typename remove_cvref<T>::key_type,
    typename std::enable_if<container_traits<T>::is_container &&
        std::tuple_size<typename remove_cvref<T>::
    value_type>::value == 2>::type>>
{
private:
    template<typename U, typename = void>
    struct unique_keys : std::false_type { };

    template<typename U>
    struct unique_keys<U, typename std::enable_if<
        (std::tuple_size<remove_cvref<decltype(std::declval<
            remove_cvref<U>&>().emplace(std::declval<typename
        remove_cvref<U>::value_type>()))>>::value > 0)>::type>
            : std::true_type { };
public:
    static constexpr bool is_map = true;
    static constexpr bool has_unique_keys = unique_keys<T>::value;
    using pair_key_type = typename std::tuple_element<
        0, typename remove_cvref<T>::value_type>::type;
    using pair_value_type = typename std::tuple_element<
        1, typename remove_cvref<T>::value_type>::type;
    static constexpr bool key_converts_to_string =
        std::is_convertible<pair_key_type, string_view>::value;
};

// does not include std::nullptr_t
template<class T>
using value_constructible = std::integral_constant<bool,
    std::is_same<detail::remove_cvref<T>, value>::value ||
        std::is_same<detail::remove_cvref<T>, object>::value ||
    std::is_same<detail::remove_cvref<T>, array>::value ||
        std::is_same<detail::remove_cvref<T>, string>::value ||
    std::is_same<detail::remove_cvref<T>, string_view>::value ||
        std::is_arithmetic<detail::remove_cvref<T>>::value ||
    std::is_same<detail::remove_cvref<T>, char const*>::value ||
    std::is_same<detail::remove_cvref<T>,
        std::initializer_list<value_ref>>::value ||
    std::is_same<detail::remove_cvref<T>, value_ref>::value>;

BOOST_STATIC_ASSERT(value_constructible<value>::value);

} // detail
BOOST_JSON_NS_END

#endif

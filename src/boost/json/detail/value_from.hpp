//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_VALUE_FROM_HPP
#define BOOST_JSON_DETAIL_VALUE_FROM_HPP

#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/json/detail/value_traits.hpp>

BOOST_JSON_NS_BEGIN

struct value_from_tag { };

template<class T, class = void>
struct has_value_from;

namespace detail {

// The integral_constant parameter here is an
// rvalue reference to make the standard conversion
// sequence to that parameter better, see
// http://eel.is/c++draft/over.ics.rank#3.2.6
template<std::size_t N, class T>
void
tuple_to_array(
    T&&,
    array&,
    std::integral_constant<std::size_t, N>&&)
{
}

template<std::size_t N, std::size_t I, class T>
void
tuple_to_array(
    T&& t,
    array& arr,
    const std::integral_constant<std::size_t, I>&)
{
    using std::get;
    arr.emplace_back(value_from(
        get<I>(std::forward<T>(t)), arr.storage()));
    return detail::tuple_to_array<N>(std::forward<T>(t),
        arr, std::integral_constant<std::size_t, I + 1>());
}

//----------------------------------------------------------
// User-provided conversion

template<class T, void_t<decltype(tag_invoke(value_from_tag(),
    std::declval<value&>(), std::declval<T&&>()))>* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<5>)
{
    tag_invoke(value_from_tag(), jv, std::forward<T>(from));
}


//----------------------------------------------------------
// Native conversion

template<class T, typename std::enable_if<
    detail::value_constructible<T>::value>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<4>)
{
    jv = std::forward<T>(from);
}

template<class T, typename std::enable_if<
    std::is_same<detail::remove_cvref<T>,
        std::nullptr_t>::value>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&&,
    priority_tag<4>)
{
    // do nothing
    BOOST_ASSERT(jv.is_null());
    (void)jv;
}

//----------------------------------------------------------
// Generic conversions

// string-like types
// NOTE: original check for size used is_convertible but
// MSVC-140 selects wrong specialisation if used
template<class T, typename std::enable_if<
    std::is_constructible<remove_cvref<T>, const char*, std::size_t>::value &&
    std::is_convertible<decltype(std::declval<T&>().data()), const char*>::value &&
    std::is_integral<decltype(std::declval<T&>().size())>::value
>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<3>)
{
    jv.emplace_string().assign(
        from.data(), from.size());
}

// map-like types; should go before ranges, so that we can differentiate
// map-like and other ranges
template<class T, typename std::enable_if<
    map_traits<T>::has_unique_keys &&
        has_value_from<typename map_traits<T>::pair_value_type>::value &&
    std::is_convertible<typename map_traits<T>::pair_key_type,
        string_view>::value>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<2>)
{
    using std::get;
    object& obj = jv.emplace_object();
    obj.reserve(container_traits<T>::try_size(from));
    for (auto&& elem : from)
        obj.emplace(get<0>(elem), value_from(
            get<1>(elem), obj.storage()));
}

// ranges; should go before tuple-like in order for std::array being handled
// by this overload
template<class T, typename std::enable_if<
    has_value_from<typename container_traits<T>::
        value_type>::value>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<1>)
{
    array& result = jv.emplace_array();
    result.reserve(container_traits<T>::try_size(from));
    for (auto&& elem : from)
        result.emplace_back(
            value_from(elem, result.storage()));
}

// tuple-like types
template<class T, typename std::enable_if<
    (std::tuple_size<remove_cvref<T>>::value > 0)>::type* = nullptr>
void
value_from_helper(
    value& jv,
    T&& from,
    priority_tag<0>)
{
    constexpr std::size_t n =
        std::tuple_size<remove_cvref<T>>::value;
    array& arr = jv.emplace_array();
    arr.reserve(n);
    detail::tuple_to_array<n>(std::forward<T>(from),
        arr, std::integral_constant<std::size_t, 0>());
}

//----------------------------------------------------------

// Calls to value_from are forwarded to this function
// so we can use ADL and hide the built-in tag_invoke
// overloads in the detail namespace
template<class T, class = void_t<
    decltype(detail::value_from_helper(std::declval<value&>(),
        std::declval<T&&>(), priority_tag<5>()))>>
value
value_from_impl(
    T&& from,
    storage_ptr sp)
{
    value jv(std::move(sp));
    detail::value_from_helper(jv, std::forward<T>(from), priority_tag<5>());
    return jv;
}

} // detail
BOOST_JSON_NS_END

#endif

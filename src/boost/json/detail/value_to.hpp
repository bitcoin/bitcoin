//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
// Copyright (c) 2021 Dmitry Arkhipov (grisumbras@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_VALUE_TO_HPP
#define BOOST_JSON_DETAIL_VALUE_TO_HPP

#include <boost/json/value.hpp>
#include <boost/json/detail/index_sequence.hpp>
#include <boost/json/detail/value_traits.hpp>

#include <type_traits>

BOOST_JSON_NS_BEGIN

template<class>
struct value_to_tag { };

template<class, class = void>
struct has_value_to;

template<class T, class U,
    typename std::enable_if<
        ! std::is_reference<T>::value &&
    std::is_same<U, value>::value>::type>
T value_to(U const&);

namespace detail {

template<class T, typename std::enable_if<
    (std::tuple_size<remove_cvref<T>>::value > 0)>::type* = nullptr>
void
try_reserve(
    T&,
    std::size_t size,
    priority_tag<2>)
{
    constexpr std::size_t N = std::tuple_size<remove_cvref<T>>::value;
    if ( N != size )
    {
        detail::throw_invalid_argument(
            "target array size does not match source array size",
            BOOST_JSON_SOURCE_POS);
    }
}

template<typename T, void_t<decltype(
    std::declval<T&>().reserve(0))>* = nullptr>
void
try_reserve(
    T& cont,
    std::size_t size,
    priority_tag<1>)
{
    cont.reserve(size);
}

template<typename T>
void
try_reserve(
    T&,
    std::size_t,
    priority_tag<0>)
{
}

template<class T, typename std::enable_if<
    (std::tuple_size<remove_cvref<T>>::value > 0)>::type* = nullptr>
typename remove_cvref<T>::iterator
inserter(
    T& target,
    priority_tag<1>)
{
    return target.begin();
}

template<class T>
std::insert_iterator<T>
inserter(
    T& target,
    priority_tag<0>)
{
    return std::inserter(target, end(target));
}

//----------------------------------------------------------
// Use native conversion

// identity conversion
inline
value
tag_invoke(
    value_to_tag<value>,
    value const& jv)
{
    return jv;
}

// object
inline
object
tag_invoke(
    value_to_tag<object>,
    value const& jv)
{
    return jv.as_object();
}

// array
inline
array
tag_invoke(
    value_to_tag<array>,
    value const& jv)
{
    return jv.as_array();
}

// string
inline
string
tag_invoke(
    value_to_tag<string>,
    value const& jv)
{
    return jv.as_string();
}

// bool
inline
bool
tag_invoke(
    value_to_tag<bool>,
    value const& jv)
{
    return jv.as_bool();
}

// integral and floating point
template<class T, typename std::enable_if<
    std::is_arithmetic<T>::value>::type* = nullptr>
T
tag_invoke(
    value_to_tag<T>,
    value const& jv)
{
    return jv.to_number<T>();
}

//----------------------------------------------------------
// Use generic conversion

// string-like types
// NOTE: original check for size used is_convertible but
// MSVC-140 selects wrong specialisation if used
template<class T, typename std::enable_if<
    std::is_constructible<T, const char*, std::size_t>::value &&
    std::is_convertible<decltype(std::declval<T&>().data()), const char*>::value &&
    std::is_integral<decltype(std::declval<T&>().size())>::value
>::type* = nullptr>
T
value_to_generic(
    const value& jv,
    priority_tag<3>)
{
    auto& str = jv.as_string();
    return T(str.data(), str.size());
}

// map-like containers; should go before other containers in order to be able
// to tell them apart
template<class T, typename std::enable_if<
    has_value_to<typename map_traits<T>::pair_value_type>::value &&
        std::is_constructible<typename map_traits<T>::pair_key_type,
    string_view>::value>::type* = nullptr>
T
value_to_generic(
    const value& jv,
    priority_tag<2>)
{
    using value_type = typename
        container_traits<T>::value_type;
    const object& obj = jv.as_object();
    T result;
    try_reserve(result, obj.size(), priority_tag<2>());
    for (const auto& val : obj)
        result.insert(value_type{typename map_traits<T>::
            pair_key_type(val.key()), value_to<typename
                map_traits<T>::pair_value_type>(val.value())});
    return result;
}

// all other containers; should go before tuple-like in order to handle
// std::array with this overload
template<class T, typename std::enable_if<
    has_value_to<typename container_traits<T>::
        value_type>::value>::type* = nullptr>
T
value_to_generic(
    const value& jv,
    priority_tag<1>)
{
    const array& arr = jv.as_array();
    T result;
    detail::try_reserve(result, arr.size(), priority_tag<2>());
    std::transform(arr.begin(), arr.end(),
        detail::inserter(result, priority_tag<1>()), [](value const& val) {
            return value_to<typename container_traits<T>::value_type>(val);
        });
    return result;
}

template <class T, std::size_t... Is>
T
make_tuple_like(const array& arr, index_sequence<Is...>)
{
    return T(value_to<typename std::tuple_element<Is, T>::type>(arr[Is])...);
}

// tuple-like types
template<class T, typename std::enable_if<
    (std::tuple_size<remove_cvref<T>>::value > 0)>::type* = nullptr>
T
value_to_generic(
    const value& jv,
    priority_tag<0>)
{
    auto& arr = jv.as_array();
    constexpr std::size_t N = std::tuple_size<remove_cvref<T>>::value;
    if ( N != arr.size() )
    {
        detail::throw_invalid_argument(
            "array size does not match tuple size",
            BOOST_JSON_SOURCE_POS);
    }

    return make_tuple_like<T>(arr, make_index_sequence<N>());
}

// Matches containers
template<class T, void_t<typename std::enable_if<
    !std::is_constructible<T, const value&>::value &&
        !std::is_arithmetic<T>::value>::type, decltype(
    value_to_generic<T>(std::declval<const value&>(),
        priority_tag<2>()))>* = nullptr>
T
tag_invoke(
    value_to_tag<T>,
    value const& jv)
{
    return value_to_generic<T>(
        jv, priority_tag<3>());
}

//----------------------------------------------------------

// Calls to value_to are forwarded to this function
// so we can use ADL and hide the built-in tag_invoke
// overloads in the detail namespace
template<class T, void_t<
    decltype(tag_invoke(std::declval<value_to_tag<T>&>(),
        std::declval<const value&>()))>* = nullptr>
T
value_to_impl(
    value_to_tag<T> tag,
    value const& jv)
{
    return tag_invoke(tag, jv);
}

} // detail
BOOST_JSON_NS_END

#endif

// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_TRAITS_DECL_HPP_
#define BOOST_PROCESS_DETAIL_TRAITS_DECL_HPP_

#include <boost/process/detail/config.hpp>
#include <boost/none.hpp>
#include <type_traits>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/handler.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/handler.hpp>
#endif


namespace boost { namespace process { namespace detail {


template<typename T>
struct is_initializer : std::is_base_of<handler_base, T> {};


template<typename T>
struct is_initializer<T&> : std::is_base_of<handler_base, T> {};


template<typename T>
struct initializer_tag;// { typedef void type; };


//remove const
template<typename T>
struct initializer_tag<const T> { typedef typename initializer_tag<T>::type type; };

//remove &
template<typename T>
struct initializer_tag<T&> { typedef typename initializer_tag<T>::type type; };

//remove const &
template<typename T>
struct initializer_tag<const T&> { typedef typename initializer_tag<T>::type type; };

template<typename T>
struct initializer_builder;


template<typename First, typename ...Args>
struct valid_argument_list;

template<typename First>
struct valid_argument_list<First>
{
    constexpr static bool value = is_initializer<First>::value || !std::is_void<typename initializer_tag<First>::type>::value;
    typedef std::integral_constant<bool, value> type;
};

template<typename First, typename ...Args>
struct valid_argument_list
{
    constexpr static bool my_value = is_initializer<First>::value || !std::is_void<typename initializer_tag<First>::type>::value;
    constexpr static bool value = valid_argument_list<Args...>::value && my_value;
    typedef std::integral_constant<bool, value> type;
};



}}}



#endif /* BOOST_PROCESS_DETAIL_HANDLER_HPP_ */

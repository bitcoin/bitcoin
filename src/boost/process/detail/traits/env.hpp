// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_TRAITS_ENV_HPP_
#define BOOST_PROCESS_DETAIL_TRAITS_ENV_HPP_


#include <boost/process/detail/traits/decl.hpp>


namespace boost { namespace process {

template<typename Char>
class basic_environment;

template<typename Char>
class basic_native_environment;

namespace detail {

template<typename Char>
struct env_tag {};




template<typename Char> struct env_set;
template<typename Char> struct env_append;

template<typename Char> struct env_reset;
template<typename Char> struct env_init;


template<typename Char> struct initializer_tag<env_set<Char>>    { typedef env_tag<Char> type; };
template<typename Char> struct initializer_tag<env_append<Char>> { typedef env_tag<Char> type; };

template<typename Char> struct initializer_tag<env_reset<Char>> { typedef env_tag<Char> type;};
template<typename Char> struct initializer_tag<env_init <Char>> { typedef env_tag<Char> type;};

template<typename Char>  struct initializer_tag<::boost::process::basic_environment<Char>>           { typedef env_tag<Char> type; };
template<typename Char>  struct initializer_tag<::boost::process::basic_native_environment<Char>> { typedef env_tag<Char> type; };

template<> struct initializer_builder<env_tag<char>>;
template<> struct initializer_builder<env_tag<wchar_t>>;

}


}}

#endif /* INCLUDE_BOOST_PROCESS_DETAIL_ENV_HPP_ */

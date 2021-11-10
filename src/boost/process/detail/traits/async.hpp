// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_TRAITS_ASYNC_HPP_
#define BOOST_PROCESS_DETAIL_TRAITS_ASYNC_HPP_

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/traits/decl.hpp>

namespace boost { namespace asio {

class io_context;
}}

namespace boost { namespace process { namespace detail {

struct async_tag {};

template<>
struct initializer_builder<async_tag>;

template<> struct initializer_tag<::boost::asio::io_context> { typedef async_tag type;};




}}}



#endif /* BOOST_PROCESS_DETAIL_HANDLER_HPP_ */

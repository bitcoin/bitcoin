// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_
#define BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_

#include <memory>
#include <boost/asio/ts/netfwd.hpp>

namespace boost { namespace asio {

class mutable_buffer;
class mutable_buffers_1;

class const_buffer;
class const_buffers_1;

template<typename Allocator>
class basic_streambuf;

typedef basic_streambuf<std::allocator<char>> streambuf;

template <typename Executor>
class basic_signal_set;
typedef basic_signal_set<any_io_executor> signal_set;

template <typename Handler>
class basic_yield_context;

namespace posix {

template <typename Executor>
class basic_stream_descriptor;
typedef basic_stream_descriptor<any_io_executor> stream_descriptor;

} //posix
} //asio

namespace process { namespace detail { namespace posix {

class async_pipe;

template<typename T>
struct async_in_buffer;

template<int p1, int p2, typename Buffer>
struct async_out_buffer;

template<int p1, int p2, typename Type>
struct async_out_future;

} // posix
} // detail

using ::boost::process::detail::posix::async_pipe;

} // process
} // boost




#endif /* BOOST_PROCESS_DETAIL_POSIX_ASIO_FWD_HPP_ */

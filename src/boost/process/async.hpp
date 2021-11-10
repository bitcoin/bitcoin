// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/** \file boost/process/async.hpp

The header which provides the basic asynchrounous features.
It provides the on_exit property, which allows callbacks when the process exits.
It also implements the necessary traits for passing an boost::asio::io_context,
which is needed for asynchronous communication.

It also pulls the [boost::asio::buffer](http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/buffer.html)
into the boost::process namespace for convenience.

\xmlonly
<programlisting>
namespace boost {
  namespace process {
    <emphasis>unspecified</emphasis> <ulink url="http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio/reference/buffer.html">buffer</ulink>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::on_exit">on_exit</globalname>;
  }
}
</programlisting>

\endxmlonly
  */

#ifndef BOOST_PROCESS_ASYNC_HPP_
#define BOOST_PROCESS_ASYNC_HPP_

#include <boost/process/detail/traits.hpp>
#include <boost/process/detail/on_exit.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffer.hpp>
#include <type_traits>
#include <boost/fusion/iterator/deref.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/io_context_ref.hpp>
#include <boost/process/detail/posix/async_in.hpp>
#include <boost/process/detail/posix/async_out.hpp>
#include <boost/process/detail/posix/on_exit.hpp>

#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/io_context_ref.hpp>
#include <boost/process/detail/windows/async_in.hpp>
#include <boost/process/detail/windows/async_out.hpp>
#include <boost/process/detail/windows/on_exit.hpp>
#endif

namespace boost { namespace process { namespace detail {

struct async_tag;

template<typename T>
struct is_io_context : std::false_type {};
template<>
struct is_io_context<api::io_context_ref> : std::true_type {};

template<typename Tuple>
inline asio::io_context& get_io_context(const Tuple & tup)
{
    auto& ref = *boost::fusion::find_if<is_io_context<boost::mpl::_>>(tup);
    return ref.get();
}

struct async_builder
{
    boost::asio::io_context * ios;

    void operator()(boost::asio::io_context & ios_) {this->ios = &ios_;};

    typedef api::io_context_ref result_type;
    api::io_context_ref get_initializer() {return api::io_context_ref (*ios);};
};


template<>
struct initializer_builder<async_tag>
{
    typedef async_builder type;
};

}

using ::boost::asio::buffer;


#if defined(BOOST_PROCESS_DOXYGEN)
/** When an io_context is passed, the on_exit property can be used, to be notified
    when the child process exits.


The following syntax is valid

\code{.cpp}
on_exit=function;
on_exit(function);
\endcode

with `function` being a callable object with the signature `(int, const std::error_code&)` or an
`std::future<int>`.

\par Example

\code{.cpp}
io_context ios;

child c("ls", ios, on_exit=[](int exit, const std::error_code& ec_in){});

std::future<int> exit_code;
chlid c2("ls", ios, on_exit=exit_code);

\endcode

\note The handler is not invoked when the launch fails.
\warning When used \ref ignore_error it might get invoked on error.
\warning `on_exit` uses `boost::asio::signal_set` to listen for `SIGCHLD` on posix, and so has the
same restrictions as that class (do not register a handler for `SIGCHLD` except by using
`boost::asio::signal_set`).
 */
constexpr static ::boost::process::detail::on_exit_ on_exit{};
#endif

}}



#endif /* INCLUDE_BOOST_PROCESS_DETAIL_ASYNC_HPP_ */

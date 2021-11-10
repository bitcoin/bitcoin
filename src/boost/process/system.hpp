// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/system.hpp
 *
 * Defines a system function.
 */

#ifndef BOOST_PROCESS_SYSTEM_HPP
#define BOOST_PROCESS_SYSTEM_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/on_exit.hpp>
#include <boost/process/child.hpp>
#include <boost/process/detail/async_handler.hpp>
#include <boost/process/detail/execute_impl.hpp>
#include <boost/asio/post.hpp>
#include <type_traits>
#include <mutex>
#include <condition_variable>

#if defined(BOOST_POSIX_API)
#include <boost/process/posix.hpp>
#endif

namespace boost {

namespace process {

namespace detail
{

struct system_impl_success_check : handler
{
    bool succeeded = false;

    template<typename Exec>
    void on_success(Exec &) { succeeded = true; }
};

template<typename IoService, typename ...Args>
inline int system_impl(
        std::true_type, /*needs ios*/
        std::true_type, /*has io_context*/
        Args && ...args)
{
    IoService & ios = ::boost::process::detail::get_io_context_var(args...);

    system_impl_success_check check;

    std::atomic_bool exited{false};

    child c(std::forward<Args>(args)...,
            check,
            ::boost::process::on_exit(
                [&](int, const std::error_code&)
                {
                    boost::asio::post(ios.get_executor(), [&]{exited.store(true);});
                }));
    if (!c.valid() || !check.succeeded)
        return -1;

    while (!exited.load())
        ios.poll();

    return c.exit_code();
}

template<typename IoService, typename ...Args>
inline int system_impl(
        std::true_type,  /*needs ios */
        std::false_type, /*has io_context*/
        Args && ...args)
{
    IoService ios;
    child c(ios, std::forward<Args>(args)...);
    if (!c.valid())
        return -1;

    ios.run();
    if (c.running())
        c.wait();
    return c.exit_code();
}


template<typename IoService, typename ...Args>
inline int system_impl(
        std::false_type, /*needs ios*/
        std::true_type, /*has io_context*/
        Args && ...args)
{
    child c(std::forward<Args>(args)...);
    if (!c.valid())
        return -1;
    c.wait();
    return c.exit_code();
}

template<typename IoService, typename ...Args>
inline int system_impl(
        std::false_type, /*has async */
        std::false_type, /*has io_context*/
        Args && ...args)
{
    child c(std::forward<Args>(args)...
#if defined(BOOST_POSIX_API)
            ,::boost::process::posix::sig.dfl()
#endif
            );
    if (!c.valid())
        return -1;
    c.wait();
    return c.exit_code();
}

}

/** Launches a process and waits for its exit.
It works as std::system, though it allows
all the properties boost.process provides. It will execute the process and wait for it's exit; then return the exit_code.

\code{.cpp}
int ret = system("ls");
\endcode

\attention Using this function with synchronous pipes leads to many potential deadlocks.

When using this function with an asynchronous properties and NOT passing an io_context object,
the system function will create one and run it. When the io_context is passed to the function,
the system function will check if it is active, and call the io_context::run function if not.

*/
template<typename ...Args>
inline int system(Args && ...args)
{
    typedef typename ::boost::process::detail::needs_io_context<Args...>::type
            need_ios;
    typedef typename ::boost::process::detail::has_io_context<Args...>::type
            has_ios;
    return ::boost::process::detail::system_impl<boost::asio::io_context>(
            need_ios(), has_ios(),
            std::forward<Args>(args)...);
}


}}
#endif


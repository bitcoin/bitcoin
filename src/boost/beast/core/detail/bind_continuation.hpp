//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_BIND_CONTINUATION_HPP
#define BOOST_BEAST_DETAIL_BIND_CONTINUATION_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/remap_post_to_defer.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/core/empty_value.hpp>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace detail {

#if 0
/** Mark a completion handler as a continuation.

    This function wraps a completion handler to associate it with an
    executor whose `post` operation is remapped to the `defer` operation.
    It is used by composed asynchronous operation implementations to
    indicate that a completion handler submitted to an initiating
    function represents a continuation of the current asynchronous
    flow of control.

    @param handler The handler to wrap.
    The implementation takes ownership of the handler by performing a decay-copy.

    @see

    @li <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4242.html">[N4242] Executors and Asynchronous Operations, Revision 1</a>
*/
template<class CompletionHandler>
#if BOOST_BEAST_DOXYGEN
__implementation_defined__
#else
net::executor_binder<
    typename std::decay<CompletionHandler>::type,
    detail::remap_post_to_defer<
        net::associated_executor_t<CompletionHandler>>>
#endif
bind_continuation(CompletionHandler&& handler)
{
    return net::bind_executor(
        detail::remap_post_to_defer<
            net::associated_executor_t<CompletionHandler>>(
                net::get_associated_executor(handler)),
        std::forward<CompletionHandler>(handler));
}

/** Mark a completion handler as a continuation.

    This function wraps a completion handler to associate it with an
    executor whose `post` operation is remapped to the `defer` operation.
    It is used by composed asynchronous operation implementations to
    indicate that a completion handler submitted to an initiating
    function represents a continuation of the current asynchronous
    flow of control.

    @param ex The executor to use

    @param handler The handler to wrap
    The implementation takes ownership of the handler by performing a decay-copy.

    @see

    @li <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4242.html">[N4242] Executors and Asynchronous Operations, Revision 1</a>
*/
template<class Executor, class CompletionHandler>
#if BOOST_BEAST_DOXYGEN
__implementation_defined__
#else
net::executor_binder<typename
    std::decay<CompletionHandler>::type,
    detail::remap_post_to_defer<Executor>>
#endif
bind_continuation(
    Executor const& ex, CompletionHandler&& handler)
{
    return net::bind_executor(
        detail::remap_post_to_defer<Executor>(ex),
        std::forward<CompletionHandler>(handler));
}
#else
// VFALCO I turned these off at the last minute because they cause
//        the completion handler to be moved before the initiating
//        function is invoked rather than after, which is a foot-gun.
//
// REMINDER: Uncomment the tests when this is put back
template<class F>
F&&
bind_continuation(F&& f)
{
    return std::forward<F>(f);
}
#endif

} // detail
} // beast
} // boost

#endif

//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_IS_INVOCABLE_HPP
#define BOOST_BEAST_DETAIL_IS_INVOCABLE_HPP

#include <boost/asio/async_result.hpp>
#include <boost/type_traits/make_void.hpp>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace detail {

template<class R, class C, class ...A>
auto
is_invocable_test(C&& c, int, A&& ...a)
    -> decltype(std::is_convertible<
        decltype(c(std::forward<A>(a)...)), R>::value ||
            std::is_same<R, void>::value,
                std::true_type());

template<class R, class C, class ...A>
std::false_type
is_invocable_test(C&& c, long, A&& ...a);

/** Metafunction returns `true` if F callable as R(A...)

    Example:

    @code
    is_invocable<T, void(std::string)>::value
    @endcode
*/
/** @{ */
template<class C, class F>
struct is_invocable : std::false_type
{
};

template<class C, class R, class ...A>
struct is_invocable<C, R(A...)>
    : decltype(is_invocable_test<R>(
        std::declval<C>(), 1, std::declval<A>()...))
{
};
/** @} */

template<class CompletionToken, class Signature, class = void>
struct is_completion_token_for : std::false_type
{
};

struct any_initiation
{
    template<class...AnyArgs>
    void operator()(AnyArgs&&...);
};

template<class CompletionToken, class R, class...Args>
struct is_completion_token_for<
    CompletionToken, R(Args...), boost::void_t<decltype(
        boost::asio::async_initiate<CompletionToken, R(Args...)>(
            any_initiation(), std::declval<CompletionToken&>())
        )>> : std::true_type
{
};

} // detail
} // beast
} // boost

#endif

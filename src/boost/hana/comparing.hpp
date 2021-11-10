/*!
@file
Defines `boost::hana::comparing`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_COMPARING_HPP
#define BOOST_HANA_COMPARING_HPP

#include <boost/hana/fwd/comparing.hpp>

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/equal.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    namespace detail {
        template <typename F>
        struct equal_by {
            F f;

            template <typename X, typename Y>
            constexpr auto operator()(X&& x, Y&& y) const&
            { return hana::equal(f(static_cast<X&&>(x)), f(static_cast<Y&&>(y))); }

            template <typename X, typename Y>
            constexpr auto operator()(X&& x, Y&& y) &
            { return hana::equal(f(static_cast<X&&>(x)), f(static_cast<Y&&>(y))); }
        };
    }

    //! @cond
    template <typename F>
    constexpr auto comparing_t::operator()(F&& f) const {
        return detail::equal_by<typename detail::decay<F>::type>{static_cast<F&&>(f)};
    }
    //! @endcond
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_COMPARING_HPP

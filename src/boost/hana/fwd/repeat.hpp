/*!
@file
Forward declares `boost::hana::repeat`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_REPEAT_HPP
#define BOOST_HANA_FWD_REPEAT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Invokes a nullary function `n` times.
    //! @ingroup group-IntegralConstant
    //!
    //! Given an `IntegralConstant` `n` and a nullary function `f`,
    //! `repeat(n, f)` will call `f` `n` times. In particular, any
    //! decent compiler should expand `repeat(n, f)` to
    //! @code
    //!     f(); f(); ... f(); // n times total
    //! @endcode
    //!
    //!
    //! @param n
    //! An `IntegralConstant` holding a non-negative value representing
    //! the number of times `f` should be repeatedly invoked.
    //!
    //! @param f
    //! A function to repeatedly invoke `n` times. `f` is allowed to have
    //! side effects.
    //!
    //!
    //! Example
    //! -------
    //! @include example/repeat.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto repeat = [](auto const& n, auto&& f) -> void {
        f(); f(); ... f(); // n times total
    };
#else
    template <typename N, typename = void>
    struct repeat_impl : repeat_impl<N, when<true>> { };

    struct repeat_t {
        template <typename N, typename F>
        constexpr void operator()(N const& n, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr repeat_t repeat{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_REPEAT_HPP

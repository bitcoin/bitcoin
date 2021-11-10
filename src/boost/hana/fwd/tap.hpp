/*!
@file
Forward declares `boost::hana::tap`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_TAP_HPP
#define BOOST_HANA_FWD_TAP_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Tap inside a monadic chain.
    //! @ingroup group-Monad
    //!
    //! Given a function `f`, `tap<M>` returns a new function which performs
    //! `f` on its argument and then returns the argument lifted in the `M`
    //! `Monad`. Combined with the property that `chain(m, lift<M>) == m`,
    //! this provides a way of executing an action inside a monadic chain
    //! without influencing its overall result. This is useful to e.g. insert
    //! debug statements or perform actions that are not tied to the chain but
    //! that need to be executed inside of it.
    //!
    //! @note
    //! Since C++ is not a pure language, it is possible to perform side
    //! effects inside the `f` function. Actually, side effects are the
    //! only reason why one might want to use `tap`. However, one should
    //! not rely on the side effects being done in any specific order.
    //!
    //!
    //! @tparam M
    //! The tag (a `Monad`) of the monads in the tapped monadic chain.
    //!
    //! @param f
    //! A function to be executed inside a monadic chain. It will be called
    //! as `f(x)`, where `x` is a value inside the previous monad in the
    //! chain. The result of `f` is always discarded.
    //!
    //!
    //! Example
    //! -------
    //! @include example/tap.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename M>
    constexpr auto tap = [](auto&& f) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct tap_impl : tap_impl<M, when<true>> { };

    template <typename M>
    struct tap_t {
        template <typename F>
        constexpr auto operator()(F&& f) const;
    };

    template <typename M>
    BOOST_HANA_INLINE_VARIABLE constexpr tap_t<M> tap{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_TAP_HPP

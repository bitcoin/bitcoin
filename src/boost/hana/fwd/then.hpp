/*!
@file
Forward declares `boost::hana::then`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_THEN_HPP
#define BOOST_HANA_FWD_THEN_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Sequentially compose two monadic actions, discarding any value
    //! produced by the first but not its effects.
    //! @ingroup group-Monad
    //!
    //!
    //! @param before
    //! The first `Monad` in the monadic composition chain. The result of
    //! this monad is ignored, but its effects are combined with that of the
    //! second monad.
    //!
    //! @param xs
    //! The second `Monad` in the monadic composition chain.
    //!
    //!
    //! Example
    //! -------
    //! @include example/then.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto then = [](auto&& before, auto&& xs) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct then_impl : then_impl<M, when<true>> { };

    struct then_t {
        template <typename Before, typename Xs>
        constexpr decltype(auto) operator()(Before&& before, Xs&& xs) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr then_t then{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_THEN_HPP

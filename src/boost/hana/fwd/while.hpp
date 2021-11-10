/*!
@file
Forward declares `boost::hana::while_`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_WHILE_HPP
#define BOOST_HANA_FWD_WHILE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Apply a function to an initial state while some predicate is satisfied.
    //! @ingroup group-Logical
    //!
    //! This method is a natural extension of the `while` language construct
    //! to manipulate a state whose type may change from one iteration to
    //! another. However, note that having a state whose type changes from
    //! one iteration to the other is only possible as long as the predicate
    //! returns a `Logical` whose truth value is known at compile-time.
    //!
    //! Specifically, `while_(pred, state, f)` is equivalent to
    //! @code
    //!     f(...f(f(state)))
    //! @endcode
    //! where `f` is iterated as long as `pred(f(...))` is a true-valued
    //! `Logical`.
    //!
    //!
    //! @param pred
    //! A predicate called on the state or on the result of applying `f` a
    //! certain number of times to the state, and returning whether `f`
    //! should be applied one more time.
    //!
    //! @param state
    //! The initial state on which `f` is applied.
    //!
    //! @param f
    //! A function that is iterated on the initial state. Note that the
    //! return type of `f` may change from one iteration to the other,
    //! but only while `pred` returns a compile-time `Logical`. In other
    //! words, `decltype(f(stateN))` may differ from `decltype(f(stateN+1))`,
    //! but only if `pred(f(stateN))` returns a compile-time `Logical`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/while.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto while_ = [](auto&& pred, auto&& state, auto&& f) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename L, typename = void>
    struct while_impl : while_impl<L, when<true>> { };

    struct while_t {
        template <typename Pred, typename State, typename F>
        constexpr decltype(auto) operator()(Pred&& pred, State&& state, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr while_t while_{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_WHILE_HPP

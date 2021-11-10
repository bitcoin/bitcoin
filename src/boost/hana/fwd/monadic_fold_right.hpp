/*!
@file
Forward declares `boost::hana::monadic_fold_right`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_MONADIC_FOLD_RIGHT_HPP
#define BOOST_HANA_FWD_MONADIC_FOLD_RIGHT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Monadic right-fold of a structure with a binary operation and an
    //! optional initial reduction state.
    //! @ingroup group-Foldable
    //!
    //! @note
    //! This assumes the reader to be accustomed to non-monadic right-folds as
    //! explained by `hana::fold_right`, and to have read the [primer]
    //! (@ref monadic-folds) on monadic folds.
    //!
    //! `monadic_fold_right<M>` is a right-associative monadic fold. Given a
    //! structure containing `x1, ..., xn`, a function `f` and an optional
    //! initial state, `monadic_fold_right<M>` applies `f` as follows
    //! @code
    //!     // with state
    //!     (f(x1, -) | (f(x2, -) | (f(x3, -) | (... | f(xn, state)))))
    //!
    //!     // without state
    //!     (f(x1, -) | (f(x2, -) | (f(x3, -) | (... | f(xn-1, xn)))))
    //! @endcode
    //!
    //! where `f(xk, -)` denotes the partial application of `f` to `xk`,
    //! and `|` is just the operator version of the monadic `chain`.
    //! It is worth noting that the order in which the binary function should
    //! expect its arguments is reversed from `monadic_fold_left<M>`.
    //!
    //! When the structure is empty, one of two things may happen. If an
    //! initial state was provided, it is lifted to the given Monad and
    //! returned as-is. Otherwise, if the no-state version of the function
    //! was used, an error is triggered. When the stucture contains a single
    //! element and the no-state version of the function was used, that
    //! single element is lifted into the given Monad and returned as is.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Monad` `M`, a `Foldable` `F`, an initial state of tag `S`,
    //! and a function @f$ f : T \times S \to M(S) @f$, the signatures of
    //! `monadic_fold_right<M>` are
    //! \f[
    //!     \mathtt{monadic\_fold\_right}_M :
    //!         F(T) \times S \times (T \times S \to M(S)) \to M(S)
    //! \f]
    //!
    //! for the version with an initial state, and
    //! \f[
    //!     \mathtt{monadic\_fold\_right}_M :
    //!         F(T) \times (T \times T \to M(T)) \to M(T)
    //! \f]
    //!
    //! for the version without an initial state.
    //!
    //! @tparam M
    //! The Monad representing the monadic context in which the fold happens.
    //! The return type of `f` must be in that Monad.
    //!
    //! @param xs
    //! The structure to fold.
    //!
    //! @param state
    //! The initial value used for folding. If the structure is empty, this
    //! value is lifted in to the `M` Monad and then returned as-is.
    //!
    //! @param f
    //! A binary function called as `f(x, state)`, where `state` is the result
    //! accumulated so far and `x` is an element in the structure. The
    //! function must return its result inside the `M` Monad.
    //!
    //!
    //! Example
    //! -------
    //! @include example/monadic_fold_right.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename M>
    constexpr auto monadic_fold_right = [](auto&& xs[, auto&& state], auto&& f) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct monadic_fold_right_impl : monadic_fold_right_impl<T, when<true>> { };

    template <typename M>
    struct monadic_fold_right_t {
        template <typename Xs, typename State, typename F>
        constexpr decltype(auto) operator()(Xs&& xs, State&& state, F&& f) const;

        template <typename Xs, typename F>
        constexpr decltype(auto) operator()(Xs&& xs, F&& f) const;
    };

    template <typename M>
    BOOST_HANA_INLINE_VARIABLE constexpr monadic_fold_right_t<M> monadic_fold_right{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_MONADIC_FOLD_RIGHT_HPP

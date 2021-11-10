/*!
@file
Forward declares `boost::hana::reverse_fold`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_REVERSE_FOLD_HPP
#define BOOST_HANA_FWD_REVERSE_FOLD_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Equivalent to `reverse_fold` in Boost.Fusion and Boost.MPL.
    //! @ingroup group-Foldable
    //!
    //! This method has the same semantics as `reverse_fold` in Boost.Fusion
    //! and Boost.MPL, with the extension that an initial state is not
    //! required. This method is equivalent to `fold_right`, except that
    //! the accumulating function must take its arguments in reverse order,
    //! to match the order used in Fusion. In other words,
    //! @code
    //!     reverse_fold(sequence, state, f) == fold_right(sequence, state, flip(f))
    //!     reverse_fold(sequence, f) == fold_right(sequence, flip(f))
    //! @endcode
    //!
    //! @note
    //! This method is a convenience alias to `fold_right`. As an alias,
    //! `reverse_fold` is not tag-dispatched on its own and `fold_right`
    //! should be customized instead.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Foldable` `F` and an optional initial state of tag `S`,
    //! the signatures for `reverse_fold` are
    //! \f[
    //!     \mathtt{reverse\_fold} : F(T) \times S \times (S \times T \to S) \to S
    //! \f]
    //!
    //! for the variant with an initial state, and
    //! \f[
    //!     \mathtt{reverse\_fold} : F(T) \times (T \times T \to T) \to T
    //! \f]
    //!
    //! for the variant without an initial state.
    //!
    //! @param xs
    //! The structure to fold.
    //!
    //! @param state
    //! The initial value used for folding.
    //!
    //! @param f
    //! A binary function called as `f(state, x)`, where `state` is the
    //! result accumulated so far and `x` is an element in the structure.
    //! For reverse folds without an initial state, the function is called as
    //! `f(x1, x2)`, where `x1` and `x2` are elements of the structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/reverse_fold.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto reverse_fold = [](auto&& xs[, auto&& state], auto&& f) -> decltype(auto) {
        return fold_right(forwarded(xs), forwarded(state), flip(forwarded(f)));
    };
#else
    struct reverse_fold_t {
        template <typename Xs, typename S, typename F>
        constexpr decltype(auto) operator()(Xs&& xs, S&& s, F&& f) const;

        template <typename Xs, typename F>
        constexpr decltype(auto) operator()(Xs&& xs, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr reverse_fold_t reverse_fold{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_REVERSE_FOLD_HPP

/*!
@file
Forward declares `boost::hana::unfold_right`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_UNFOLD_RIGHT_HPP
#define BOOST_HANA_FWD_UNFOLD_RIGHT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Dual operation to `fold_right` for sequences.
    //! @ingroup group-Sequence
    //!
    //! While `fold_right` reduces a structure to a summary value from the
    //! right, `unfold_right` builds a sequence from a seed value and a
    //! function, starting from the right.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Sequence` `S`, an initial value `state` of tag `I`, an
    //! arbitrary Product `P` and a function \f$ f : I \to P(T, I) \f$,
    //! `unfold_right<S>` has the following signature:
    //! \f[
    //!     \mathtt{unfold\_right}_S : I \times (I \to P(T, I)) \to S(T)
    //! \f]
    //!
    //! @tparam S
    //! The tag of the sequence to build up.
    //!
    //! @param state
    //! An initial value to build the sequence from.
    //!
    //! @param f
    //! A function called as `f(state)`, where `state` is an initial value,
    //! and returning
    //! 1. `nothing` if it is done producing the sequence.
    //! 2. otherwise, `just(make<P>(x, state))`, where `state` is the new
    //!    initial value used in the next call to `f`, `x` is an element to
    //!    be prepended to the resulting sequence, and `P` is an arbitrary
    //!    `Product`.
    //!
    //!
    //! Fun fact
    //! ---------
    //! In some cases, `unfold_right` can undo a `fold_right` operation:
    //! @code
    //!     unfold_right<S>(fold_right(xs, state, f), g) == xs
    //! @endcode
    //!
    //! if the following holds
    //! @code
    //!     g(f(x, y)) == just(make_pair(x, y))
    //!     g(state) == nothing
    //! @endcode
    //!
    //!
    //! Example
    //! -------
    //! @include example/unfold_right.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename S>
    constexpr auto unfold_right = [](auto&& state, auto&& f) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct unfold_right_impl : unfold_right_impl<S, when<true>> { };

    template <typename S>
    struct unfold_right_t;

    template <typename S>
    BOOST_HANA_INLINE_VARIABLE constexpr unfold_right_t<S> unfold_right{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_UNFOLD_RIGHT_HPP

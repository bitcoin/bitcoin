/*!
@file
Forward declares `boost::hana::unfold_left`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_UNFOLD_LEFT_HPP
#define BOOST_HANA_FWD_UNFOLD_LEFT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Dual operation to `fold_left` for sequences.
    //! @ingroup group-Sequence
    //!
    //! While `fold_left` reduces a structure to a summary value from the left,
    //! `unfold_left` builds a sequence from a seed value and a function,
    //! starting from the left.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Sequence` `S`, an initial value `state` of tag `I`, an
    //! arbitrary Product `P` and a function \f$ f : I \to P(I, T) \f$,
    //! `unfold_left<S>` has the following signature:
    //! \f[
    //!     \mathtt{unfold\_left}_S : I \times (I \to P(I, T)) \to S(T)
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
    //! 2. otherwise, `just(make<P>(state, x))`, where `state` is the new
    //!    initial value used in the next call to `f`, `x` is an element to
    //!    be appended to the resulting sequence, and `P` is an arbitrary
    //!    `Product`.
    //!
    //!
    //! Fun fact
    //! ---------
    //! In some cases, `unfold_left` can undo a `fold_left` operation:
    //! @code
    //!     unfold_left<S>(fold_left(xs, state, f), g) == xs
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
    //! @include example/unfold_left.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename S>
    constexpr auto unfold_left = [](auto&& state, auto&& f) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct unfold_left_impl : unfold_left_impl<S, when<true>> { };

    template <typename S>
    struct unfold_left_t;

    template <typename S>
    BOOST_HANA_INLINE_VARIABLE constexpr unfold_left_t<S> unfold_left{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_UNFOLD_LEFT_HPP

/*!
@file
Forward declares `boost::hana::scan_left`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SCAN_LEFT_HPP
#define BOOST_HANA_FWD_SCAN_LEFT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Fold a Sequence to the left and return a list containing the
    //! successive reduction states.
    //! @ingroup group-Sequence
    //!
    //! Like `fold_left`, `scan_left` reduces a sequence to a single value
    //! using a binary operation. However, unlike `fold_left`, it builds up
    //! a sequence of the intermediary results computed along the way and
    //! returns that instead of only the final reduction state. Like
    //! `fold_left`, `scan_left` can be used with or without an initial
    //! reduction state.
    //!
    //! When the sequence is empty, two things may arise. If an initial state
    //! was provided, a singleton list containing that state is returned.
    //! Otherwise, if no initial state was provided, an empty list is
    //! returned. In particular, unlike for `fold_left`, using `scan_left`
    //! on an empty sequence without an initial state is not an error.
    //!
    //! More specifically, `scan_left([x1, ..., xn], state, f)` is a sequence
    //! whose `i`th element is equivalent to `fold_left([x1, ..., xi], state, f)`.
    //! The no-state variant is handled in an analogous way. For illustration,
    //! consider this left fold on a short sequence:
    //! @code
    //!     fold_left([x1, x2, x3], state, f) == f(f(f(state, x1), x2), x3)
    //! @endcode
    //!
    //! The analogous sequence generated with `scan_left` will be
    //! @code
    //!     scan_left([x1, x2, x3], state, f) == [
    //!         state,
    //!         f(state, x1),
    //!         f(f(state, x1), x2),
    //!         f(f(f(state, x1), x2), x3)
    //!     ]
    //! @endcode
    //!
    //! Similarly, consider this left fold (without an initial state) on
    //! a short sequence:
    //! @code
    //!     fold_left([x1, x2, x3, x4], f) == f(f(f(x1, x2), x3), x4)
    //! @endcode
    //!
    //! The analogous sequence generated with `scan_left` will be
    //! @code
    //!     scan_left([x1, x2, x3, x4], f) == [
    //!         x1,
    //!         f(x1, x2),
    //!         f(f(x1, x2), x3),
    //!         f(f(f(x1, x2), x3), x4)
    //!     ]
    //! @endcode
    //!
    //! @param xs
    //! The sequence to scan from the left.
    //!
    //! @param state
    //! The (optional) initial reduction state.
    //!
    //! @param f
    //! A binary function called as `f(state, x)`, where `state` is the
    //! result accumulated so far and `x` is an element in the sequence.
    //! If no initial state is provided, `f` is called as `f(x1, x2)`,
    //! where `x1` and `x2` are both elements of the sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/scan_left.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto scan_left = [](auto&& xs[, auto&& state], auto const& f) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct scan_left_impl : scan_left_impl<S, when<true>> { };

    struct scan_left_t {
        template <typename Xs, typename State, typename F>
        constexpr auto operator()(Xs&& xs, State&& state, F const& f) const;

        template <typename Xs, typename F>
        constexpr auto operator()(Xs&& xs, F const& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr scan_left_t scan_left{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SCAN_LEFT_HPP

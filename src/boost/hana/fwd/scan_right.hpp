/*!
@file
Forward declares `boost::hana::scan_right`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SCAN_RIGHT_HPP
#define BOOST_HANA_FWD_SCAN_RIGHT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Fold a Sequence to the right and return a list containing the
    //! successive reduction states.
    //! @ingroup group-Sequence
    //!
    //! Like `fold_right`, `scan_right` reduces a sequence to a single value
    //! using a binary operation. However, unlike `fold_right`, it builds up
    //! a sequence of the intermediary results computed along the way and
    //! returns that instead of only the final reduction state. Like
    //! `fold_right`, `scan_right` can be used with or without an initial
    //! reduction state.
    //!
    //! When the sequence is empty, two things may arise. If an initial state
    //! was provided, a singleton list containing that state is returned.
    //! Otherwise, if no initial state was provided, an empty list is
    //! returned. In particular, unlike for `fold_right`, using `scan_right`
    //! on an empty sequence without an initial state is not an error.
    //!
    //! More specifically, `scan_right([x1, ..., xn], state, f)` is a sequence
    //! whose `i`th element is equivalent to `fold_right([x1, ..., xi], state, f)`.
    //! The no-state variant is handled in an analogous way. For illustration,
    //! consider this right fold on a short sequence:
    //! @code
    //!     fold_right([x1, x2, x3], state, f) == f(x1, f(x2, f(x3, state)))
    //! @endcode
    //!
    //! The analogous sequence generated with `scan_right` will be
    //! @code
    //!     scan_right([x1, x2, x3], state, f) == [
    //!         f(x1, f(x2, f(x3, state))),
    //!               f(x2, f(x3, state)),
    //!                     f(x3, state),
    //!                           state
    //!     ]
    //! @endcode
    //!
    //! Similarly, consider this right fold (without an initial state) on
    //! a short sequence:
    //! @code
    //!     fold_right([x1, x2, x3, x4], f) == f(x1, f(x2, f(x3, x4)))
    //! @endcode
    //!
    //! The analogous sequence generated with `scan_right` will be
    //! @code
    //!     scan_right([x1, x2, x3, x4], f) == [
    //!         f(x1, f(x2, f(x3, x4))),
    //!               f(x2, f(x3, x4)),
    //!                     f(x3, x4),
    //!                           x4
    //!     ]
    //! @endcode
    //!
    //! @param xs
    //! The sequence to scan from the right.
    //!
    //! @param state
    //! The (optional) initial reduction state.
    //!
    //! @param f
    //! A binary function called as `f(x, state)`, where `state` is the
    //! result accumulated so far and `x` is an element in the sequence.
    //! When no initial state is provided, `f` is called as `f(x1, x2)`,
    //! where `x1` and `x2` are elements of the sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/scan_right.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto scan_right = [](auto&& xs[, auto&& state], auto const& f) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct scan_right_impl : scan_right_impl<S, when<true>> { };

    struct scan_right_t {
        template <typename Xs, typename State, typename F>
        constexpr auto operator()(Xs&& xs, State&& state, F const& f) const;

        template <typename Xs, typename F>
        constexpr auto operator()(Xs&& xs, F const& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr scan_right_t scan_right{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SCAN_RIGHT_HPP

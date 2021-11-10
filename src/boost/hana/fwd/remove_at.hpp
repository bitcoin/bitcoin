/*!
@file
Forward declares `boost::hana::remove_at` and `boost::hana::remove_at_c`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_REMOVE_AT_HPP
#define BOOST_HANA_FWD_REMOVE_AT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>

#include <cstddef>


BOOST_HANA_NAMESPACE_BEGIN
    //! Remove the element at a given index from a sequence.
    //! @ingroup group-Sequence
    //!
    //! `remove_at` returns a new sequence identical to the original, except
    //! that the element at the given index is removed. Specifically,
    //! `remove_at([x0, ..., xn-1, xn, xn+1, ..., xm], n)` is a new
    //! sequence equivalent to `[x0, ..., xn-1, xn+1, ..., xm]`.
    //!
    //! @note
    //! The behavior is undefined if the index is out of the bounds of the
    //! sequence.
    //!
    //!
    //! @param xs
    //! A sequence from which an element is to be removed.
    //!
    //! @param n
    //! An non-negative `IntegralConstant` representing the index of the
    //! element to be removed from the sequence. The behavior is undefined
    //! if that index is not in the bounds of the sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/remove_at.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto remove_at = [](auto&& xs, auto const& n) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct remove_at_impl : remove_at_impl<S, when<true>> { };

    struct remove_at_t {
        template <typename Xs, typename N>
        constexpr auto operator()(Xs&& xs, N const& n) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr remove_at_t remove_at{};
#endif

    //! Equivalent to `remove_at`; provided for convenience.
    //! @ingroup group-Sequence
    //!
    //!
    //! Example
    //! -------
    //! @include example/remove_at_c.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <std::size_t n>
    constexpr auto remove_at_c = [](auto&& xs) {
        return hana::remove_at(forwarded(xs), hana::size_c<n>);
    };
#else
    template <std::size_t n>
    struct remove_at_c_t;

    template <std::size_t n>
    BOOST_HANA_INLINE_VARIABLE constexpr remove_at_c_t<n> remove_at_c{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_REMOVE_AT_HPP

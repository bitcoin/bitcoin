/*!
@file
Forward declares `boost::hana::insert_range`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_INSERT_RANGE_HPP
#define BOOST_HANA_FWD_INSERT_RANGE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Insert several values at a given index in a sequence.
    //! @ingroup group-Sequence
    //!
    //! Given a sequence, an index and any `Foldable` containing elements to
    //! insert, `insert_range` inserts the elements in the `Foldable` at the
    //! given index of the sequence.
    //!
    //! @param xs
    //! The sequence in which values should be inserted.
    //!
    //! @param n
    //! The index at which elements should be inserted. This must be a
    //! non-negative `Constant` of an integral type, and it must also be
    //! true that `n < length(xs)` if `xs` is a finite sequence.
    //!
    //! @param elements
    //! A `Foldable` containing elements to insert in the sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/insert_range.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto insert_range = [](auto&& xs, auto&& n, auto&& elements) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct insert_range_impl : insert_range_impl<S, when<true>> { };

    struct insert_range_t {
        template <typename Xs, typename N, typename Elements>
        constexpr auto operator()(Xs&& xs, N&& n, Elements&& elements) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr insert_range_t insert_range{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_INSERT_RANGE_HPP

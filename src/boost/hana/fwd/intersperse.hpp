/*!
@file
Forward declares `boost::hana::intersperse`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_INTERSPERSE_HPP
#define BOOST_HANA_FWD_INTERSPERSE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Insert a value between each pair of elements in a finite sequence.
    //! @ingroup group-Sequence
    //!
    //! Given a finite `Sequence` `xs` with a linearization of
    //! `[x1, x2, ..., xn]`, `intersperse(xs, z)` is a new sequence with a
    //! linearization of `[x1, z, x2, z, x3, ..., xn-1, z, xn]`. In other
    //! words, it inserts the `z` element between every pair of elements of
    //! the original sequence. If the sequence is empty or has a single
    //! element, `intersperse` returns the sequence as-is. In all cases,
    //! the sequence must be finite.
    //!
    //!
    //! @param xs
    //! The sequence in which a value is interspersed.
    //!
    //! @param z
    //! The value to be inserted between every pair of elements of the sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/intersperse.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto intersperse = [](auto&& xs, auto&& z) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct intersperse_impl : intersperse_impl<S, when<true>> { };

    struct intersperse_t {
        template <typename Xs, typename Z>
        constexpr auto operator()(Xs&& xs, Z&& z) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr intersperse_t intersperse{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_INTERSPERSE_HPP

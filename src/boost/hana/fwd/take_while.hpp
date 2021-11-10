/*!
@file
Forward declares `boost::hana::take_while`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_TAKE_WHILE_HPP
#define BOOST_HANA_FWD_TAKE_WHILE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Take elements from a sequence while the `predicate` is satisfied.
    //! @ingroup group-Sequence
    //!
    //! Specifically, `take_while` returns a new sequence containing the
    //! longest prefix of `xs` in which all the elements satisfy the given
    //! predicate.
    //!
    //!
    //! @param xs
    //! The sequence to take elements from.
    //!
    //! @param predicate
    //! A function called as `predicate(x)`, where `x` is an element of the
    //! sequence, and returning a `Logical` representing whether `x` should be
    //! included in the resulting sequence. In the current version of the
    //! library, `predicate` has to return a `Constant Logical`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/take_while.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto take_while = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct take_while_impl : take_while_impl<S, when<true>> { };

    struct take_while_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr take_while_t take_while{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_TAKE_WHILE_HPP

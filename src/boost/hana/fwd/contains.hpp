/*!
@file
Forward declares `boost::hana::contains` and `boost::hana::in`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONTAINS_HPP
#define BOOST_HANA_FWD_CONTAINS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/functional/flip.hpp>
#include <boost/hana/functional/infix.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns whether the key occurs in the structure.
    //! @ingroup group-Searchable
    //!
    //! Given a `Searchable` structure `xs` and a `key`, `contains` returns
    //! whether any of the keys of the structure is equal to the given `key`.
    //! If the structure is not finite, an equal key has to appear at a finite
    //! position in the structure for this method to finish. For convenience,
    //! `contains` can also be applied in infix notation.
    //!
    //!
    //! @param xs
    //! The structure to search.
    //!
    //! @param key
    //! A key to be searched for in the structure. The key has to be
    //! `Comparable` with the other keys of the structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/contains.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto contains = [](auto&& xs, auto&& key) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct contains_impl : contains_impl<S, when<true>> { };

    struct contains_t {
        template <typename Xs, typename Key>
        constexpr auto operator()(Xs&& xs, Key&& key) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr auto contains = hana::infix(contains_t{});
#endif

    //! Return whether the key occurs in the structure.
    //! @ingroup group-Searchable
    //!
    //! Specifically, this is equivalent to `contains`, except `in` takes its
    //! arguments in reverse order. Like `contains`, `in` can also be applied
    //! in infix notation for increased expressiveness. This function is not a
    //! method that can be overriden; it is just a convenience function
    //! provided with the concept.
    //!
    //!
    //! Example
    //! -------
    //! @include example/in.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto in = hana::infix(hana::flip(hana::contains));
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONTAINS_HPP

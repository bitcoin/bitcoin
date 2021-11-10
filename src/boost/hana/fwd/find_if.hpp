/*!
@file
Forward declares `boost::hana::find_if`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_FIND_IF_HPP
#define BOOST_HANA_FWD_FIND_IF_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Finds the value associated to the first key satisfying a predicate.
    //! @ingroup group-Searchable
    //!
    //! Given a `Searchable` structure `xs` and a predicate `pred`,
    //! `find_if(xs, pred)` returns `just` the first element whose key
    //! satisfies the predicate, or `nothing` if there is no such element.
    //!
    //!
    //! @param xs
    //! The structure to be searched.
    //!
    //! @param predicate
    //! A function called as `predicate(k)`, where `k` is a key of the
    //! structure, and returning whether `k` is the key of the element
    //! being searched for. In the current version of the library, the
    //! predicate has to return an `IntegralConstant` holding a value
    //! that can be converted to `bool`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/find_if.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto find_if = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct find_if_impl : find_if_impl<S, when<true>> { };

    struct find_if_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr find_if_t find_if{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_FIND_IF_HPP

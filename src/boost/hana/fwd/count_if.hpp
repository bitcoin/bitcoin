/*!
@file
Forward declares `boost::hana::count_if`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_COUNT_IF_HPP
#define BOOST_HANA_FWD_COUNT_IF_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return the number of elements in the structure for which the
    //! `predicate` is satisfied.
    //! @ingroup group-Foldable
    //!
    //! Specifically, returns an object of an unsigned integral type, or
    //! a `Constant` holding such an object, which represents the number
    //! of elements in the structure satisfying the given `predicate`.
    //!
    //!
    //! @param xs
    //! The structure whose elements are counted.
    //!
    //! @param predicate
    //! A function called as `predicate(x)`, where `x` is an element of the
    //! structure, and returning a `Logical` representing whether `x` should
    //! be counted.
    //!
    //!
    //! Example
    //! -------
    //! @include example/count_if.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto count_if = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct count_if_impl : count_if_impl<T, when<true>> { };

    struct count_if_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr count_if_t count_if{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_COUNT_IF_HPP

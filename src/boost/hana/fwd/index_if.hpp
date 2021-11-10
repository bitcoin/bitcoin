/*!
@file
Forward declares `boost::hana::index_if`.

@copyright Louis Dionne 2013-2017
@copyright Jason Rice 2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_INDEX_IF_HPP
#define BOOST_HANA_FWD_INDEX_IF_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Finds the value associated to the first key satisfying a predicate.
    //! @ingroup group-Iterable
    //!
    //! Given an `Iterable` structure `xs` and a predicate `pred`,
    //! `index_if(xs, pred)` returns a `hana::optional` containing an `IntegralConstant`
    //! of the index of the first element that satisfies the predicate or nothing
    //! if no element satisfies the predicate.
    //!
    //!
    //! @param xs
    //! The structure to be searched.
    //!
    //! @param predicate
    //! A function called as `predicate(x)`, where `x` is an element of the
    //! `Iterable` structure and returning whether `x` is the element being
    //! searched for. In the current version of the library, the predicate
    //! has to return an `IntegralConstant` holding a value that can be
    //! converted to `bool`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/index_if.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto index_if = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct index_if_impl : index_if_impl<S, when<true>> { };

    struct index_if_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr index_if_t index_if{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_INDEX_IF_HPP


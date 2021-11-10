/*!
@file
Forward declares `boost::hana::first`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_FIRST_HPP
#define BOOST_HANA_FWD_FIRST_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns the first element of a pair.
    //! @ingroup group-Product
    //!
    //! Note that if the `Product` actually stores the elements it contains,
    //! `hana::first` is required to return a lvalue reference, a lvalue
    //! reference to const or a rvalue reference to the first element, where
    //! the type of reference must match that of the pair passed to `first`.
    //! If the `Product` does not store the elements it contains (i.e. it
    //! generates them on demand), this requirement is dropped.
    //!
    //!
    //! Example
    //! -------
    //! @include example/first.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto first = [](auto&& product) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename P, typename = void>
    struct first_impl : first_impl<P, when<true>> { };

    struct first_t {
        template <typename Pair>
        constexpr decltype(auto) operator()(Pair&& pair) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr first_t first{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_FIRST_HPP

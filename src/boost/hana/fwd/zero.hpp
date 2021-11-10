/*!
@file
Forward declares `boost::hana::zero`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_ZERO_HPP
#define BOOST_HANA_FWD_ZERO_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Identity of `plus`.
    //! @ingroup group-Monoid
    //!
    //! @tparam M
    //! The tag (a `Monoid`) of the returned identity.
    //!
    //!
    //! Example
    //! -------
    //! @include example/zero.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename M>
    constexpr auto zero = []() -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct zero_impl : zero_impl<M, when<true>> { };

    template <typename M>
    struct zero_t {
        constexpr decltype(auto) operator()() const;
    };

    template <typename M>
    BOOST_HANA_INLINE_VARIABLE constexpr zero_t<M> zero{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_ZERO_HPP

/*!
@file
Forward declares `boost::hana::or_`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_OR_HPP
#define BOOST_HANA_FWD_OR_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return whether any of the arguments is true-valued.
    //! @ingroup group-Logical
    //!
    //! `or_` can be called with one argument or more. When called with
    //! two arguments, `or_` uses tag-dispatching to find the right
    //! implementation. Otherwise,
    //! @code
    //!     or_(x) == x
    //!     or_(x, y, ...z) == or_(or_(x, y), z...)
    //! @endcode
    //!
    //!
    //! Example
    //! -------
    //! @include example/or.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto or_ = [](auto&& x, auto&& ...y) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename L, typename = void>
    struct or_impl : or_impl<L, when<true>> { };

    struct or_t {
        template <typename X, typename Y>
        constexpr decltype(auto) operator()(X&& x, Y&& y) const;

        template <typename X, typename ...Y>
        constexpr decltype(auto) operator()(X&& x, Y&& ...y) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr or_t or_{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_OR_HPP

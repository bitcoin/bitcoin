/*!
@file
Forward declares `boost::hana::is_empty`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_IS_EMPTY_HPP
#define BOOST_HANA_FWD_IS_EMPTY_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns whether the iterable is empty.
    //! @ingroup group-Iterable
    //!
    //! Given an `Iterable` `xs`, `is_empty` returns whether `xs` contains
    //! no more elements. In other words, it returns whether trying to
    //! extract the tail of `xs` would be an error. In the current version
    //! of the library, `is_empty` must return an `IntegralConstant` holding
    //! a value convertible to `bool`. This is because only compile-time
    //! `Iterable`s are supported right now.
    //!
    //!
    //! Example
    //! -------
    //! @include example/is_empty.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto is_empty = [](auto const& xs) {
        return tag-dispatched;
    };
#else
    template <typename It, typename = void>
    struct is_empty_impl : is_empty_impl<It, when<true>> { };

    struct is_empty_t {
        template <typename Xs>
        constexpr auto operator()(Xs const& xs) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr is_empty_t is_empty{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_IS_EMPTY_HPP

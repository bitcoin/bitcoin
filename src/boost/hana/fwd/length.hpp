/*!
@file
Forward declares `boost::hana::length`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_LENGTH_HPP
#define BOOST_HANA_FWD_LENGTH_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return the number of elements in a foldable structure.
    //! @ingroup group-Foldable
    //!
    //! Given a `Foldable` `xs`, `length(xs)` must return an object of an
    //! unsigned integral type, or an `IntegralConstant` holding such an
    //! object, which represents the number of elements in the structure.
    //!
    //! @note
    //! Since only compile-time `Foldable`s are supported in the library
    //! right now, `length` must always return an `IntegralConstant`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/length.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto length = [](auto const& xs) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct length_impl : length_impl<T, when<true>> { };

    struct length_t {
        template <typename Xs>
        constexpr auto operator()(Xs const& xs) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr length_t length{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_LENGTH_HPP

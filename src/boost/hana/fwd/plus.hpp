/*!
@file
Forward declares `boost::hana::plus`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PLUS_HPP
#define BOOST_HANA_FWD_PLUS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Associative binary operation on a `Monoid`.
    //! @ingroup group-Monoid
    //!
    //! @param x, y
    //! Two objects to combine with the `Monoid`'s binary operation.
    //!
    //!
    //! Cross-type version of the method
    //! --------------------------------
    //! The `plus` method is "overloaded" to handle distinct data types
    //! with certain properties. Specifically, `plus` is defined for
    //! _distinct_ data types `A` and `B` such that
    //! 1. `A` and `B` share a common data type `C`, as determined by the
    //!    `common` metafunction
    //! 2. `A`, `B` and `C` are all `Monoid`s when taken individually
    //! 3. `to<C> : A -> B` and `to<C> : B -> C` are `Monoid`-embeddings, as
    //!    determined by the `is_embedding` metafunction.
    //!
    //! The definition of `plus` for data types satisfying the above
    //! properties is obtained by setting
    //! @code
    //!     plus(x, y) = plus(to<C>(x), to<C>(y))
    //! @endcode
    //!
    //!
    //! Example
    //! -------
    //! @include example/plus.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto plus = [](auto&& x, auto&& y) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename T, typename U, typename = void>
    struct plus_impl : plus_impl<T, U, when<true>> { };

    struct plus_t {
        template <typename X, typename Y>
        constexpr decltype(auto) operator()(X&& x, Y&& y) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr plus_t plus{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PLUS_HPP

/*!
@file
Forward declares `boost::hana::not_equal`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_NOT_EQUAL_HPP
#define BOOST_HANA_FWD_NOT_EQUAL_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/nested_to_fwd.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns a `Logical` representing whether `x` is not equal to `y`.
    //! @ingroup group-Comparable
    //!
    //! The `not_equal` function can be called in two different ways. First,
    //! it can be called like a normal function:
    //! @code
    //!     not_equal(x, y)
    //! @endcode
    //!
    //! However, it may also be partially applied to an argument by using
    //! `not_equal.to`:
    //! @code
    //!     not_equal.to(x)(y) == not_equal(x, y)
    //! @endcode
    //!
    //! In other words, `not_equal.to(x)` is a function object that is
    //! equivalent to `partial(not_equal, x)`. This is provided to enhance
    //! the readability of some constructs, especially when using higher
    //! order algorithms.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a Logical `Bool` and two Comparables `A` and `B` that
    //! share a common embedding, the signature is
    //! @f$ \mathtt{not\_equal} : A \times B \to Bool @f$.
    //!
    //! @param x, y
    //! Two objects to compare for inequality.
    //!
    //!
    //! Example
    //! -------
    //! @include example/not_equal.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto not_equal = [](auto&& x, auto&& y) {
        return tag-dispatched;
    };
#else
    template <typename T, typename U, typename = void>
    struct not_equal_impl : not_equal_impl<T, U, when<true>> { };

    struct not_equal_t : detail::nested_to<not_equal_t> {
        template <typename X, typename Y>
        constexpr auto operator()(X&& x, Y&& y) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr not_equal_t not_equal{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_NOT_EQUAL_HPP

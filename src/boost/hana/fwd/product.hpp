/*!
@file
Forward declares `boost::hana::product`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PRODUCT_HPP
#define BOOST_HANA_FWD_PRODUCT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/fwd/integral_constant.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Compute the product of the numbers of a structure.
    //! @ingroup group-Foldable
    //!
    //! More generally, `product` will take any foldable structure containing
    //! objects forming a Ring and reduce them using the Ring's binary
    //! operation. The initial state for folding is the identity of the
    //! Ring's operation. It is sometimes necessary to specify the Ring to
    //! use; this is possible by using `product<R>`. If no Ring is specified,
    //! the structure will use the Ring formed by the elements it contains
    //! (if it knows it), or `integral_constant_tag<int>` otherwise.
    //! Hence,
    //! @code
    //!     product<R>(xs) = fold_left(xs, one<R or inferred Ring>(), mult)
    //!     product<> = product<integral_constant_tag<int>>
    //! @endcode
    //!
    //! For numbers, this will just compute the product of the numbers in the
    //! `xs` structure.
    //!
    //! @note
    //! The elements of the structure are not actually required to be in the
    //! same Ring, but it must be possible to perform `mult` on any two
    //! adjacent elements of the structure, which requires each pair of
    //! adjacent element to at least have a common Ring embedding. The
    //! meaning of "adjacent" as used here is that two elements of the
    //! structure `x` and `y` are adjacent if and only if they are adjacent
    //! in the linearization of that structure, as documented by the Iterable
    //! concept.
    //!
    //! @note
    //! See the documentation for `sum` to understand why the Ring must
    //! sometimes be specified explicitly.
    //!
    //!
    //! Example
    //! -------
    //! @include example/product.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto product = see documentation;
#else
    template <typename T, typename = void>
    struct product_impl : product_impl<T, when<true>> { };

    template <typename R>
    struct product_t {
        template <typename Xs>
        constexpr decltype(auto) operator()(Xs&& xs) const;
    };

    template <typename R = integral_constant_tag<int>>
    BOOST_HANA_INLINE_VARIABLE constexpr product_t<R> product{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PRODUCT_HPP

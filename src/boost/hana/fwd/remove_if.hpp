/*!
@file
Forward declares `boost::hana::remove_if`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_REMOVE_IF_HPP
#define BOOST_HANA_FWD_REMOVE_IF_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Remove all the elements of a monadic structure that satisfy some
    //! predicate.
    //! @ingroup group-MonadPlus
    //!
    //! Given a monadic structure `xs` and a unary predicate, `remove_if`
    //! returns a new monadic structure equal to `xs` without all its elements
    //! that satisfy the predicate. This is equivalent to `filter` with a
    //! negated predicate, i.e.
    //! @code
    //!     remove_if(xs, predicate) == filter(xs, negated predicated)
    //! @endcode
    //!
    //!
    //! Signature
    //! ---------
    //! Given a MonadPlus `M` and a predicate of type \f$ T \to Bool \f$ for
    //! some compile-time Logical `Bool`, the signature is
    //! \f$
    //!     \mathrm{remove\_if} : M(T) \times (T \to Bool) \to M(T)
    //! \f$
    //!
    //! @param xs
    //! A monadic structure to remove some elements from.
    //!
    //! @param predicate
    //! A unary predicate called as `predicate(x)`, where `x` is an element
    //! of the structure, and returning whether `x` should be removed from
    //! the structure. In the current version of the library, `predicate`
    //! must return a compile-time Logical.
    //!
    //!
    //! Example
    //! -------
    //! @include example/remove_if.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto remove_if = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct remove_if_impl : remove_if_impl<M, when<true>> { };

    struct remove_if_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr remove_if_t remove_if{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_REMOVE_IF_HPP

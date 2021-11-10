/*!
@file
Forward declares `boost::hana::chain`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CHAIN_HPP
#define BOOST_HANA_FWD_CHAIN_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Feed a monadic value into a monadic computation.
    //! @ingroup group-Monad
    //!
    //! Given a monadic value and a monadic function, `chain` feeds the
    //! monadic value into the function, thus performing some Monad-specific
    //! effects, and returns the result. An implementation of `chain` must
    //! satisfy
    //! @code
    //!     chain(xs, f) == flatten(transform(xs, f))
    //! @endcode
    //!
    //!
    //! Signature
    //! ---------
    //! For a monad `M`, given a monadic value of type `M(A)` and a monadic
    //! function @f$ f : A \to M(B) @f$, `chain` has the signature
    //! @f$
    //!     \mathtt{chain} : M(A) \times (A \to M(B)) \to M(B)
    //! @f$.
    //!
    //! @param xs
    //! A monadic value to be fed to the function `f`.
    //!
    //! @param f
    //! A function taking a normal value in the `xs` structure, and returning
    //! a monadic value. This function is called as  `f(x)`, where `x` is an
    //! element of the structure `xs`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/chain.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto chain = [](auto&& xs, auto&& f) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct chain_impl : chain_impl<M, when<true>> { };

    struct chain_t {
        template <typename Xs, typename F>
        constexpr decltype(auto) operator()(Xs&& xs, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr chain_t chain{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CHAIN_HPP

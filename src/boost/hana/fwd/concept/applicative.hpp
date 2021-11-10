/*!
@file
Forward declares `boost::hana::Applicative`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_APPLICATIVE_HPP
#define BOOST_HANA_FWD_CONCEPT_APPLICATIVE_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Applicative Applicative
    //! The `Applicative` concept represents `Functor`s with the ability
    //! to lift values and combine computations.
    //!
    //! A `Functor` can only take a normal function and map it over a
    //! structure containing values to obtain a new structure containing
    //! values. Intuitively, an `Applicative` can also take a value and
    //! lift it into the structure. In addition, an `Applicative` can take
    //! a structure containing functions and apply it to a structure
    //! containing values to obtain a new structure containing values.
    //! By currying the function(s) inside the structure, it is then
    //! also possible to apply n-ary functions to n structures containing
    //! values.
    //!
    //! @note
    //! This documentation does not go into much details about the nature
    //! of applicatives. However, the [Typeclassopedia][1] is a nice
    //! Haskell-oriented resource where such information can be found.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `lift` and `ap` satisfying the laws below. An `Applicative` must
    //! also be a `Functor`.
    //!
    //!
    //! Laws
    //! ----
    //! Given an `Applicative` `F`, the following laws must be satisfied:
    //! 1. Identity\n
    //! For all objects `xs` of tag `F(A)`,
    //! @code
    //!     ap(lift<F>(id), xs) == xs
    //! @endcode
    //!
    //! 2. Composition\n
    //! For all objects `xs` of tag `F(A)` and functions-in-an-applicative
    //! @f$ fs : F(B \to C) @f$,
    //! @f$ gs : F(A \to B) @f$,
    //! @code
    //!     ap(ap(lift<F>(compose), fs, gs), xs) == ap(fs, ap(gs, xs))
    //! @endcode
    //!
    //! 3. Homomorphism\n
    //! For all objects `x` of tag `A` and functions @f$ f : A \to B @f$,
    //! @code
    //!     ap(lift<F>(f), lift<F>(x)) == lift<F>(f(x))
    //! @endcode
    //!
    //! 4. Interchange\n
    //! For all objects `x` of tag `A` and functions-in-an-applicative
    //! @f$ fs : F(A \to B) @f$,
    //! @code
    //!     ap(fs, lift<F>(x)) == ap(lift<F>(apply(-, x)), fs)
    //! @endcode
    //! where `apply(-, x)` denotes the partial application of the `apply`
    //! function from the @ref group-functional module to the `x` argument.
    //!
    //! As a consequence of these laws, the model of `Functor` for `F` will
    //! satisfy the following for all objects `xs` of tag `F(A)` and functions
    //! @f$ f : A \to B @f$:
    //! @code
    //!     transform(xs, f) == ap(lift<F>(f), xs)
    //! @endcode
    //!
    //!
    //! Refined concept
    //! ---------------
    //! 1. `Functor` (free model)\n
    //! As a consequence of the laws, any `Applicative F` can be made a
    //! `Functor` by setting
    //! @code
    //!     transform(xs, f) = ap(lift<F>(f), xs)
    //! @endcode
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::lazy`, `hana::optional`, `hana::tuple`
    //!
    //!
    //! @anchor applicative-transformation
    //! Structure-preserving functions
    //! ------------------------------
    //! An _applicative transformation_ is a function @f$ t : F(X) \to G(X) @f$
    //! between two Applicatives `F` and `G`, where `X` can be any tag, and
    //! which preserves the operations of an Applicative. In other words, for
    //! all objects `x` of tag `X`, functions-in-an-applicative
    //! @f$ fs : F(X \to Y) @f$ and objects `xs` of tag `F(X)`,
    //! @code
    //!     t(lift<F>(x)) == lift<G>(x)
    //!     t(ap(fs, xs)) == ap(t(fs), t(xs))
    //! @endcode
    //!
    //! [1]: https://wiki.haskell.org/Typeclassopedia#Applicative
    template <typename A>
    struct Applicative;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_APPLICATIVE_HPP

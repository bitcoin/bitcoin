/*!
@file
Forward declares `boost::hana::Comonad`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_COMONAD_HPP
#define BOOST_HANA_FWD_CONCEPT_COMONAD_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    // Note: We use a multiline C++ comment because there's a double backslash
    // symbol in the documentation (for LaTeX), which triggers
    //      warning: multi-line comment [-Wcomment]
    // on GCC.

    /*!
    @ingroup group-concepts
    @defgroup group-Comonad Comonad
    The `Comonad` concept represents context-sensitive computations and
    data.

    Formally, the Comonad concept is dual to the Monad concept.
    But unless you're a mathematician, you don't care about that and it's
    fine. So intuitively, a Comonad represents context sensitive values
    and computations. First, Comonads make it possible to extract
    context-sensitive values from their context with `extract`.
    In contrast, Monads make it possible to wrap raw values into
    a given context with `lift` (from Applicative).

    Secondly, Comonads make it possible to apply context-sensitive values
    to functions accepting those, and to return the result as a
    context-sensitive value using `extend`. In contrast, Monads make
    it possible to apply a monadic value to a function accepting a normal
    value and returning a monadic value, and to return the result as a
    monadic value (with `chain`).

    Finally, Comonads make it possible to wrap a context-sensitive value
    into an extra layer of context using `duplicate`, while Monads make
    it possible to take a value with an extra layer of context and to
    strip it with `flatten`.

    Whereas `lift`, `chain` and `flatten` from Applicative and Monad have
    signatures
    \f{align*}{
        \mathtt{lift}_M &: T \to M(T) \\
        \mathtt{chain} &: M(T) \times (T \to M(U)) \to M(U) \\
        \mathtt{flatten} &: M(M(T)) \to M(T)
    \f}

    `extract`, `extend` and `duplicate` from Comonad have signatures
    \f{align*}{
        \mathtt{extract} &: W(T) \to T \\
        \mathtt{extend} &: W(T) \times (W(T) \to U) \to W(U) \\
        \mathtt{duplicate} &: W(T) \to W(W(T))
    \f}

    Notice how the "arrows" are reversed. This symmetry is essentially
    what we mean by Comonad being the _dual_ of Monad.

    @note
    The [Typeclassopedia][1] is a nice Haskell-oriented resource for further
    reading about Comonads.


    Minimal complete definition
    ---------------------------
    `extract` and (`extend` or `duplicate`) satisfying the laws below.
    A `Comonad` must also be a `Functor`.


    Laws
    ----
    For all Comonads `w`, the following laws must be satisfied:
    @code
        extract(duplicate(w)) == w
        transform(duplicate(w), extract) == w
        duplicate(duplicate(w)) == transform(duplicate(w), duplicate)
    @endcode

    @note
    There are several equivalent ways of defining Comonads, and this one
    is just one that was picked arbitrarily for simplicity.


    Refined concept
    ---------------
    1. Functor\n
    Every Comonad is also required to be a Functor. At first, one might think
    that it should instead be some imaginary concept CoFunctor. However, it
    turns out that a CoFunctor is the same as a `Functor`, hence the
    requirement that a `Comonad` also is a `Functor`.


    Concrete models
    ---------------
    `hana::lazy`

    [1]: https://wiki.haskell.org/Typeclassopedia#Comonad

    */
    template <typename W>
    struct Comonad;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_COMONAD_HPP

/*!
@file
Forward declares `boost::hana::Monad`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_MONAD_HPP
#define BOOST_HANA_FWD_CONCEPT_MONAD_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Monad Monad
    //! The `Monad` concept represents `Applicative`s with the ability to
    //! flatten nested levels of structure.
    //!
    //! Historically, Monads are a construction coming from category theory,
    //! an abstract branch of mathematics. The functional programming
    //! community eventually discovered how Monads could be used to
    //! formalize several useful things like side effects, which led
    //! to the wide adoption of Monads in that community. However, even
    //! in a multi-paradigm language like C++, there are several constructs
    //! which turn out to be Monads, like `std::optional`, `std::vector` and
    //! others.
    //!
    //! Everybody tries to introduce `Monad`s with a different analogy, and
    //! most people fail. This is called the [Monad tutorial fallacy][1]. We
    //! will try to avoid this trap by not presenting a specific intuition,
    //! and we will instead present what monads are mathematically.
    //! For specific intuitions, we will let readers who are new to this
    //! concept read one of the many excellent tutorials available online.
    //! Understanding Monads might take time at first, but once you get it,
    //! a lot of patterns will become obvious Monads; this enlightening will
    //! be your reward for the hard work.
    //!
    //! There are different ways of defining a Monad; Haskell uses a function
    //! called `bind` (`>>=`) and another one called `return` (it has nothing
    //! to do with C++'s `return` statement). They then introduce relationships
    //! that must be satisfied for a type to be a Monad with those functions.
    //! Mathematicians sometimes use a function called `join` and another one
    //! called `unit`, or they also sometimes use other category theoretic
    //! constructions like functor adjunctions and the Kleisli category.
    //!
    //! This library uses a composite approach. First, we use the `flatten`
    //! function (equivalent to `join`) along with the `lift` function from
    //! `Applicative` (equivalent to `unit`) to introduce the notion of
    //! monadic function composition. We then write the properties that must
    //! be satisfied by a Monad using this monadic composition operator,
    //! because we feel it shows the link between Monads and Monoids more
    //! clearly than other approaches.
    //!
    //! Roughly speaking, we will say that a `Monad` is an `Applicative` which
    //! also defines a way to compose functions returning a monadic result,
    //! as opposed to only being able to compose functions returning a normal
    //! result. We will then ask for this composition to be associative and to
    //! have a neutral element, just like normal function composition. For
    //! usual composition, the neutral element is the identity function `id`.
    //! For monadic composition, the neutral element is the `lift` function
    //! defined by `Applicative`. This construction is made clearer in the
    //! laws below.
    //!
    //! @note
    //! Monads are known to be a big chunk to swallow. However, it is out of
    //! the scope of this documentation to provide a full-blown explanation
    //! of the concept. The [Typeclassopedia][2] is a nice Haskell-oriented
    //! resource where more information about Monads can be found.
    //!
    //!
    //! Minimal complete definitions
    //! ----------------------------
    //! First, a `Monad` must be both a `Functor` and an `Applicative`.
    //! Also, an implementation of `flatten` or `chain` satisfying the
    //! laws below for monadic composition must be provided.
    //!
    //! @note
    //! The `ap` method for `Applicatives` may be derived from the minimal
    //! complete definition of `Monad` and `Functor`; see below for more
    //! information.
    //!
    //!
    //! Laws
    //! ----
    //! To simplify writing the laws, we use the comparison between functions.
    //! For two functions `f` and `g`, we define
    //! @code
    //!     f == g  if and only if  f(x) == g(x) for all x
    //! @endcode
    //!
    //! With the usual composition of functions, we are given two functions
    //! @f$ f : A \to B @f$ and @f$ g : B \to C @f$, and we must produce a
    //! new function @f$ compose(g, f) : A \to C @f$. This composition of
    //! functions is associative, which means that
    //! @code
    //!     compose(h, compose(g, f)) == compose(compose(h, g), f)
    //! @endcode
    //!
    //! Also, this composition has an identity element, which is the identity
    //! function. This simply means that
    //! @code
    //!     compose(f, id) == compose(id, f) == f
    //! @endcode
    //!
    //! This is probably nothing new if you are reading the `Monad` laws.
    //! Now, we can observe that the above is equivalent to saying that
    //! functions with the composition operator form a `Monoid`, where the
    //! neutral element is the identity function.
    //!
    //! Given an `Applicative` `F`, what if we wanted to compose two functions
    //! @f$ f : A \to F(B) @f$ and @f$ g : B \to F(C) @f$? When the
    //! `Applicative` `F` is also a `Monad`, such functions taking normal
    //! values but returning monadic values are called _monadic functions_.
    //! To compose them, we obviously can't use normal function composition,
    //! since the domains and codomains of `f` and `g` do not match properly.
    //! Instead, we'll need a new operator -- let's call it `monadic_compose`:
    //! @f[
    //!     \mathtt{monadic\_compose} :
    //!         (B \to F(C)) \times (A \to F(B)) \to (A \to F(C))
    //! @f]
    //!
    //! How could we go about implementing this function? Well, since we know
    //! `F` is an `Applicative`, the only functions we have are `transform`
    //! (from `Functor`), and `lift` and `ap` (from `Applicative`). Hence,
    //! the only thing we can do at this point while respecting the signatures
    //! of `f` and `g` is to set (for `x` of type `A`)
    //! @code
    //!     monadic_compose(g, f)(x) = transform(f(x), g)
    //! @endcode
    //!
    //! Indeed, `f(x)` is of type `F(B)`, so we can map `g` (which takes `B`'s)
    //! on it. Doing so will leave us with a result of type `F(F(C))`, but what
    //! we wanted was a result of type `F(C)` to respect the signature of
    //! `monadic_compose`. If we had a joker of type @f$ F(F(C)) \to F(C) @f$,
    //! we could simply set
    //! @code
    //!     monadic_compose(g, f)(x) = joker(transform(f(x), g))
    //! @endcode
    //!
    //! and we would be happy. It turns out that `flatten` is precisely this
    //! joker. Now, we'll want our joker to satisfy some properties to make
    //! sure this composition is associative, just like our normal composition
    //! was. These properties are slightly cumbersome to specify, so we won't
    //! do it here. Also, we'll need some kind of neutral element for the
    //! composition. This neutral element can't be the usual identity function,
    //! because it does not have the right type: our neutral element needs to
    //! be a function of type @f$ X \to F(X) @f$ but the identity function has
    //! type @f$ X \to X @f$. It is now the right time to observe that `lift`
    //! from `Applicative` has exactly the right signature, and so we'll take
    //! this for our neutral element.
    //!
    //! We are now ready to formulate the `Monad` laws using this composition
    //! operator. For a `Monad` `M` and functions @f$ f : A \to M(B) @f$,
    //! @f$ g : B \to M(C) @f$ and @f$ h : C \to M(D) @f$, the following
    //! must be satisfied:
    //! @code
    //!     // associativity
    //!     monadic_compose(h, monadic_compose(g, f)) == monadic_compose(monadic_compose(h, g), f)
    //!
    //!     // right identity
    //!     monadic_compose(f, lift<M(A)>) == f
    //!
    //!     // left identity
    //!     monadic_compose(lift<M(B)>, f) == f
    //! @endcode
    //!
    //! which is to say that `M` along with monadic composition is a Monoid
    //! where the neutral element is `lift`.
    //!
    //!
    //! Refined concepts
    //! ----------------
    //! 1. `Functor`
    //! 2. `Applicative` (free implementation of `ap`)\n
    //! When the minimal complete definition for `Monad` and `Functor` are
    //! both satisfied, it is possible to implement `ap` by setting
    //! @code
    //!     ap(fs, xs) = chain(fs, [](auto f) {
    //!         return transform(xs, f);
    //!     })
    //! @endcode
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::lazy`, `hana::optional`, `hana::tuple`
    //!
    //!
    //! [1]: https://byorgey.wordpress.com/2009/01/12/abstraction-intuition-and-the-monad-tutorial-fallacy/
    //! [2]: https://wiki.haskell.org/Typeclassopedia#Monad
    template <typename M>
    struct Monad;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_MONAD_HPP

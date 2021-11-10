/*!
@file
Forward declares `boost::hana::Group`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_GROUP_HPP
#define BOOST_HANA_FWD_CONCEPT_GROUP_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Group Group
    //! The `Group` concept represents `Monoid`s where all objects have
    //! an inverse w.r.t. the `Monoid`'s binary operation.
    //!
    //! A [Group][1] is an algebraic structure built on top of a `Monoid`
    //! which adds the ability to invert the action of the `Monoid`'s binary
    //! operation on any element of the set. Specifically, a `Group` is a
    //! `Monoid` `(S, +)` such that every element `s` in `S` has an inverse
    //! (say `s'`) which is such that
    //! @code
    //!     s + s' == s' + s == identity of the Monoid
    //! @endcode
    //!
    //! There are many examples of `Group`s, one of which would be the
    //! additive `Monoid` on integers, where the inverse of any integer
    //! `n` is the integer `-n`. The method names used here refer to
    //! exactly this model.
    //!
    //!
    //! Minimal complete definitions
    //! ----------------------------
    //! 1. `minus`\n
    //! When `minus` is specified, the `negate` method is defaulted by setting
    //! @code
    //!     negate(x) = minus(zero<G>(), x)
    //! @endcode
    //!
    //! 2. `negate`\n
    //! When `negate` is specified, the `minus` method is defaulted by setting
    //! @code
    //!     minus(x, y) = plus(x, negate(y))
    //! @endcode
    //!
    //!
    //! Laws
    //! ----
    //! For all objects `x` of a `Group` `G`, the following laws must be
    //! satisfied:
    //! @code
    //!     plus(x, negate(x)) == zero<G>() // right inverse
    //!     plus(negate(x), x) == zero<G>() // left inverse
    //! @endcode
    //!
    //!
    //! Refined concept
    //! ---------------
    //! `Monoid`
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::integral_constant`
    //!
    //!
    //! Free model for non-boolean arithmetic data types
    //! ------------------------------------------------
    //! A data type `T` is arithmetic if `std::is_arithmetic<T>::%value` is
    //! true. For a non-boolean arithmetic data type `T`, a model of `Group`
    //! is automatically defined by setting
    //! @code
    //!     minus(x, y) = (x - y)
    //!     negate(x) = -x
    //! @endcode
    //!
    //! @note
    //! The rationale for not providing a Group model for `bool` is the same
    //! as for not providing a `Monoid` model.
    //!
    //!
    //! Structure-preserving functions
    //! ------------------------------
    //! Let `A` and `B` be two `Group`s. A function `f : A -> B` is said to
    //! be a [Group morphism][2] if it preserves the group structure between
    //! `A` and `B`. Rigorously, for all objects `x, y` of data type `A`,
    //! @code
    //!     f(plus(x, y)) == plus(f(x), f(y))
    //! @endcode
    //! Because of the `Group` structure, it is easy to prove that the
    //! following will then also be satisfied:
    //! @code
    //!     f(negate(x)) == negate(f(x))
    //!     f(zero<A>()) == zero<B>()
    //! @endcode
    //! Functions with these properties interact nicely with `Group`s, which
    //! is why they are given such a special treatment.
    //!
    //!
    //! [1]: http://en.wikipedia.org/wiki/Group_(mathematics)
    //! [2]: http://en.wikipedia.org/wiki/Group_homomorphism
    template <typename G>
    struct Group;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_GROUP_HPP

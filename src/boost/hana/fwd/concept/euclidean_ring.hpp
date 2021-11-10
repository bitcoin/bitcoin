/*!
@file
Forward declares `boost::hana::EuclideanRing`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_EUCLIDEAN_RING_HPP
#define BOOST_HANA_FWD_CONCEPT_EUCLIDEAN_RING_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-EuclideanRing Euclidean Ring
    //! The `EuclideanRing` concept represents a commutative `Ring` that
    //! can also be endowed with a division algorithm.
    //!
    //! A Ring defines a binary operation often called _multiplication_ that
    //! can be used to combine two elements of the ring into a new element of
    //! the ring. An [Euclidean ring][1], also called an Euclidean domain, adds
    //! the ability to define a special function that generalizes the Euclidean
    //! division of integers.
    //!
    //! However, an Euclidean ring must also satisfy one more property, which
    //! is that of having no non-zero zero divisors. In a Ring `(R, +, *)`, it
    //! follows quite easily from the axioms that `x * 0 == 0` for any ring
    //! element `x`. However, there is nothing that mandates `0` to be the
    //! only ring element sending other elements to `0`. Hence, in some Rings,
    //! it is possible to have elements `x` and `y` such that `x * y == 0`
    //! while not having `x == 0` nor `y == 0`. We call these elements divisors
    //! of zero, or zero divisors. For example, this situation arises in the
    //! Ring of integers modulo 4 (the set `{0, 1, 2, 3}`) with addition and
    //! multiplication `mod 4` as binary operations. In this case, we have that
    //! @code
    //!     2 * 2 == 4
    //!           == 0 (mod 4)
    //! @endcode
    //! even though `2 != 0 (mod 4)`.
    //!
    //! Following this line of thought, an Euclidean ring requires its only
    //! zero divisor is zero itself. In other words, the multiplication in an
    //! Euclidean won't send two non-zero elements to zero. Also note that
    //! since multiplication in a `Ring` is not necessarily commutative, it
    //! is not always the case that
    //! @code
    //!     x * y == 0  implies  y * x == 0
    //! @endcode
    //! To be rigorous, we should then distinguish between elements that are
    //! zero divisors when multiplied to the right and to the left.
    //! Fortunately, the concept of an Euclidean ring requires the Ring
    //! multiplication to be commutative. Hence,
    //! @code
    //!     x * y == y * x
    //! @endcode
    //! and we do not have to distinguish between left and right zero divisors.
    //!
    //! Typical examples of Euclidean rings include integers and polynomials
    //! over a field. The method names used here refer to the Euclidean ring
    //! of integers under the usual addition, multiplication and division
    //! operations.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `div` and `mod` satisfying the laws below
    //!
    //!
    //! Laws
    //! ----
    //! To simplify the reading, we will use the `+`, `*`, `/` and `%`
    //! operators with infix notation to denote the application of the
    //! corresponding methods in Monoid, Group, Ring and EuclideanRing.
    //! For all objects `a` and `b` of an `EuclideanRing` `R`, the
    //! following laws must be satisfied:
    //! @code
    //!     a * b == b * a // commutativity
    //!     (a / b) * b + a % b == a    if b is non-zero
    //!     zero<R>() % b == zero<R>()  if b is non-zero
    //! @endcode
    //!
    //!
    //! Refined concepts
    //! ----------------
    //! `Monoid`, `Group`, `Ring`
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::integral_constant`
    //!
    //!
    //! Free model for non-boolean integral data types
    //! ----------------------------------------------
    //! A data type `T` is integral if `std::is_integral<T>::%value` is true.
    //! For a non-boolean integral data type `T`, a model of `EuclideanRing`
    //! is automatically defined by using the `Ring` model provided for
    //! arithmetic data types and setting
    //! @code
    //!     div(x, y) = (x / y)
    //!     mod(x, y)  = (x % y)
    //! @endcode
    //!
    //! @note
    //! The rationale for not providing an EuclideanRing model for `bool` is
    //! the same as for not providing Monoid, Group and Ring models.
    //!
    //!
    //! [1]: https://en.wikipedia.org/wiki/Euclidean_domain
    template <typename R>
    struct EuclideanRing;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_EUCLIDEAN_RING_HPP

/*!
@file
Forward declares `boost::hana::maximum`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_MAXIMUM_HPP
#define BOOST_HANA_FWD_MAXIMUM_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/nested_by_fwd.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return the greatest element of a non-empty structure with respect to
    //! a `predicate`, by default `less`.
    //! @ingroup group-Foldable
    //!
    //! Given a non-empty structure and an optional binary predicate
    //! (`less` by default), `maximum` returns the greatest element of
    //! the structure, i.e. an element which is greater than or equal to
    //! every other element in the structure, according to the predicate.
    //!
    //! If the structure contains heterogeneous objects, then the predicate
    //! must return a compile-time `Logical`. If no predicate is provided,
    //! the elements in the structure must be Orderable, or compile-time
    //! Orderable if the structure is heterogeneous.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a Foldable `F`, a Logical `Bool` and a predicate
    //! \f$ \mathtt{pred} : T \times T \to Bool \f$, `maximum` has the
    //! following signatures. For the variant with a provided predicate,
    //! \f[
    //!     \mathtt{maximum} : F(T) \times (T \times T \to Bool) \to T
    //! \f]
    //!
    //! for the variant without a custom predicate, `T` is required to be
    //! Orderable. The signature is then
    //! \f[
    //!     \mathtt{maximum} : F(T) \to T
    //! \f]
    //!
    //! @param xs
    //! The structure to find the greatest element of.
    //!
    //! @param predicate
    //! A function called as `predicate(x, y)`, where `x` and `y` are elements
    //! of the structure. `predicate` should be a strict weak ordering on the
    //! elements of the structure and its return value should be a Logical,
    //! or a compile-time Logical if the structure is heterogeneous.
    //!
    //! ### Example
    //! @include example/maximum.cpp
    //!
    //!
    //! Syntactic sugar (`maximum.by`)
    //! ------------------------------
    //! `maximum` can be called in a third way, which provides a nice syntax
    //! especially when working with the `ordering` combinator:
    //! @code
    //!     maximum.by(predicate, xs) == maximum(xs, predicate)
    //!     maximum.by(predicate) == maximum(-, predicate)
    //! @endcode
    //!
    //! where `maximum(-, predicate)` denotes the partial application of
    //! `maximum` to `predicate`.
    //!
    //! ### Example
    //! @include example/maximum_by.cpp
    //!
    //!
    //! Tag dispatching
    //! ---------------
    //! Both the non-predicated version and the predicated versions of
    //! `maximum` are tag-dispatched methods, and hence they can be
    //! customized independently. One reason for this is that some
    //! structures are able to provide a much more efficient implementation
    //! of `maximum` when the `less` predicate is used. Here is how the
    //! different versions of `maximum` are dispatched:
    //! @code
    //!     maximum(xs) -> maximum_impl<tag of xs>::apply(xs)
    //!     maximum(xs, pred) -> maximum_pred_impl<tag of xs>::apply(xs, pred)
    //! @endcode
    //!
    //! Also note that `maximum.by` is not tag-dispatched on its own, since it
    //! is just syntactic sugar for calling the corresponding `maximum`.
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto maximum = [](auto&& xs[, auto&& predicate]) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct maximum_impl : maximum_impl<T, when<true>> { };

    template <typename T, typename = void>
    struct maximum_pred_impl : maximum_pred_impl<T, when<true>> { };

    struct maximum_t : detail::nested_by<maximum_t> {
        template <typename Xs>
        constexpr decltype(auto) operator()(Xs&& xs) const;

        template <typename Xs, typename Predicate>
        constexpr decltype(auto) operator()(Xs&& xs, Predicate&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr maximum_t maximum{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_MAXIMUM_HPP

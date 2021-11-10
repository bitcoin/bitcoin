/*!
@file
Forward declares `boost::hana::filter`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_FILTER_HPP
#define BOOST_HANA_FWD_FILTER_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Filter a monadic structure using a custom predicate.
    //! @ingroup group-MonadPlus
    //!
    //! Given a monadic structure and a predicate, `filter` returns a new
    //! monadic structure containing only those elements that satisfy the
    //! predicate. This is a generalization of the usual `filter` function
    //! for sequences; it works for any MonadPlus. Intuitively, `filter` is
    //! somewhat equivalent to:
    //! @code
    //!     filter(xs, pred) == flatten(transform(xs, [](auto x) {
    //!         return pred(x) ? lift<Xs>(x) : empty<Xs>();
    //!     })
    //! @endcode
    //! In other words, we basically turn a monadic structure containing
    //! `[x1, ..., xn]` into a monadic structure containing
    //! @code
    //!     [
    //!         pred(x1) ? [x1] : [],
    //!         pred(x2) ? [x2] : [],
    //!         ...
    //!         pred(xn) ? [xn] : []
    //!     ]
    //! @endcode
    //! and we then `flatten` that.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `MonadPlus` `M` and an `IntegralConstant` `Bool` holding a
    //! value of type `bool`, the signature is
    //! @f$ \mathtt{filter} : M(T) \times (T \to \mathtt{Bool}) \to M(T) @f$.
    //!
    //! @param xs
    //! The monadic structure to filter.
    //!
    //! @param pred
    //! A function called as `pred(x)` for each element `x` in the monadic
    //! structure and returning whether that element should be __kept__ in
    //! the resulting structure. In the current version of the library, the
    //! predicate has to return an `IntegralConstant` holding a value
    //! convertible to a `bool`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/filter.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto filter = [](auto&& xs, auto&& pred) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct filter_impl : filter_impl<M, when<true>> { };

    struct filter_t {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr filter_t filter{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_FILTER_HPP

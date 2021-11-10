/*!
@file
Forward declares `boost::hana::span`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SPAN_HPP
#define BOOST_HANA_FWD_SPAN_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/nested_by_fwd.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns a `Product` containing the longest prefix of a sequence
    //! satisfying a predicate, and the rest of the sequence.
    //! @ingroup group-Sequence
    //!
    //! The first component of the returned `Product` is a sequence for which
    //! all elements satisfy the given predicate. The second component of the
    //! returned `Product` is a sequence containing the remainder of the
    //! argument. Both or either sequences may be empty, depending on the
    //! input argument. More specifically,
    //! @code
    //!     span(xs, predicate) == make_pair(take_while(xs, predicate),
    //!                                      drop_while(xs, predicate))
    //! @endcode
    //! except that `make_pair` may be an arbitrary `Product`.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Sequence` `S(T)`, a `Logical` `Bool` and a predicate
    //! \f$ T \to Bool \f$, `span` has the following signature:
    //! \f[
    //!     \mathtt{span} : S(T) \times (T \to Bool) \to S(T) \times S(T)
    //! \f]
    //!
    //! @param xs
    //! The sequence to break into two parts.
    //!
    //! @param predicate
    //! A function called as `predicate(x)`, where `x` is an element of the
    //! sequence, and returning a `Logical. In the current implementation of
    //! the library, `predicate` has to return a compile-time `Logical`.
    //!
    //!
    //! Syntactic sugar (`span.by`)
    //! ---------------------------
    //! `span` can be called in an alternate way, which provides a nice syntax
    //! in some cases where the predicate is short:
    //! @code
    //!     span.by(predicate, xs) == span(xs, predicate)
    //!     span.by(predicate) == span(-, predicate)
    //! @endcode
    //!
    //! where `span(-, predicate)` denotes the partial application of
    //! `span` to `predicate`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/span.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto span = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct span_impl : span_impl<S, when<true>> { };

    struct span_t : detail::nested_by<span_t> {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr span_t span{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SPAN_HPP

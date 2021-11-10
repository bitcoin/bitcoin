/*!
@file
Forward declares `boost::hana::partition`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PARTITION_HPP
#define BOOST_HANA_FWD_PARTITION_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/nested_by_fwd.hpp>



BOOST_HANA_NAMESPACE_BEGIN
    //! Partition a sequence based on a `predicate`.
    //! @ingroup group-Sequence
    //!
    //! Specifically, returns an unspecified `Product` whose first element is
    //! a sequence of the elements satisfying the predicate, and whose second
    //! element is a sequence of the elements that do not satisfy the predicate.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a Sequence `S(T)`, an `IntegralConstant` `Bool` holding a value
    //! of type `bool`, and a predicate \f$ T \to Bool \f$, `partition` has
    //! the following signature:
    //! \f[
    //!     \mathtt{partition} : S(T) \times (T \to Bool) \to S(T) \times S(T)
    //! \f]
    //!
    //! @param xs
    //! The sequence to be partitioned.
    //!
    //! @param predicate
    //! A function called as `predicate(x)` for each element `x` in the
    //! sequence, and returning whether `x` should be added to the sequence
    //! in the first component or in the second component of the resulting
    //! pair. In the current version of the library, `predicate` must return
    //! an `IntegralConstant` holding a value convertible to `bool`.
    //!
    //!
    //! Syntactic sugar (`partition.by`)
    //! --------------------------------
    //! `partition` can be called in an alternate way, which provides a nice
    //! syntax in some cases where the predicate is short:
    //! @code
    //!     partition.by(predicate, xs) == partition(xs, predicate)
    //!     partition.by(predicate) == partition(-, predicate)
    //! @endcode
    //!
    //! where `partition(-, predicate)` denotes the partial application of
    //! `partition` to `predicate`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/partition.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto partition = [](auto&& xs, auto&& predicate) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct partition_impl : partition_impl<S, when<true>> { };

    struct partition_t : detail::nested_by<partition_t> {
        template <typename Xs, typename Pred>
        constexpr auto operator()(Xs&& xs, Pred&& pred) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr partition_t partition{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PARTITION_HPP

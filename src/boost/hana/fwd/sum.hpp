/*!
@file
Forward declares `boost::hana::sum`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SUM_HPP
#define BOOST_HANA_FWD_SUM_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/fwd/integral_constant.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Compute the sum of the numbers of a structure.
    //! @ingroup group-Foldable
    //!
    //! More generally, `sum` will take any foldable structure containing
    //! objects forming a Monoid and reduce them using the Monoid's binary
    //! operation. The initial state for folding is the identity of the
    //! Monoid. It is sometimes necessary to specify the Monoid to use;
    //! this is possible by using `sum<M>`. If no Monoid is specified,
    //! the structure will use the Monoid formed by the elements it contains
    //! (if it knows it), or `integral_constant_tag<int>` otherwise. Hence,
    //! @code
    //!     sum<M>(xs) = fold_left(xs, zero<M or inferred Monoid>(), plus)
    //!     sum<> = sum<integral_constant_tag<int>>
    //! @endcode
    //!
    //! For numbers, this will just compute the sum of the numbers in the
    //! `xs` structure.
    //!
    //!
    //! @note
    //! The elements of the structure are not actually required to be in the
    //! same Monoid, but it must be possible to perform `plus` on any two
    //! adjacent elements of the structure, which requires each pair of
    //! adjacent element to at least have a common Monoid embedding. The
    //! meaning of "adjacent" as used here is that two elements of the
    //! structure `x` and `y` are adjacent if and only if they are adjacent
    //! in the linearization of that structure, as documented by the Iterable
    //! concept.
    //!
    //!
    //! Why must we sometimes specify the `Monoid` by using `sum<M>`?
    //! -------------------------------------------------------------
    //! This is because sequence tags like `tuple_tag` are not parameterized
    //! (by design). Hence, we do not know what kind of objects are in the
    //! sequence, so we can't know a `0` value of which type should be
    //! returned when the sequence is empty. Therefore, the type of the
    //! `0` to return in the empty case must be specified explicitly. Other
    //! foldable structures like `hana::range`s will ignore the suggested
    //! Monoid because they know the tag of the objects they contain. This
    //! inconsistent behavior is a limitation of the current design with
    //! non-parameterized tags, but we have no good solution for now.
    //!
    //!
    //! Example
    //! -------
    //! @include example/sum.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto sum = see documentation;
#else
    template <typename T, typename = void>
    struct sum_impl : sum_impl<T, when<true>> { };

    template <typename M>
    struct sum_t {
        template <typename Xs>
        constexpr decltype(auto) operator()(Xs&& xs) const;
    };

    template <typename M = integral_constant_tag<int>>
    BOOST_HANA_INLINE_VARIABLE constexpr sum_t<M> sum{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SUM_HPP

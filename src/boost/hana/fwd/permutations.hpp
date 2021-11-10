/*!
@file
Forward declares `boost::hana::permutations`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PERMUTATIONS_HPP
#define BOOST_HANA_FWD_PERMUTATIONS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return a sequence of all the permutations of the given sequence.
    //! @ingroup group-Sequence
    //!
    //! Specifically, `permutations(xs)` is a sequence whose elements are
    //! permutations of the original sequence `xs`. The permutations are not
    //! guaranteed to be in any specific order. Also note that the number
    //! of permutations grows very rapidly as the length of the original
    //! sequence increases. The growth rate is `O(length(xs)!)`; with a
    //! sequence `xs` of length only 8, `permutations(xs)` contains over
    //! 40 000 elements!
    //!
    //!
    //! Example
    //! -------
    //! @include example/permutations.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto permutations = [](auto&& xs) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct permutations_impl : permutations_impl<S, when<true>> { };

    struct permutations_t {
        template <typename Xs>
        constexpr auto operator()(Xs&& xs) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr permutations_t permutations{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PERMUTATIONS_HPP

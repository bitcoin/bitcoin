/*!
@file
Forward declares `boost::hana::zip_with`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_ZIP_WITH_HPP
#define BOOST_HANA_FWD_ZIP_WITH_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Zip one sequence or more with a given function.
    //! @ingroup group-Sequence
    //!
    //! Given a `n`-ary function `f` and `n` sequences `s1, ..., sn`,
    //! `zip_with` produces a sequence whose `i`-th element is
    //! `f(s1[i], ..., sn[i])`, where `sk[i]` denotes the `i`-th element of
    //! the `k`-th sequence. In other words, `zip_with` produces a sequence
    //! of the form
    //! @code
    //!     [
    //!         f(s1[0], ..., sn[0]),
    //!         f(s1[1], ..., sn[1]),
    //!         ...
    //!         f(s1[M], ..., sn[M])
    //!     ]
    //! @endcode
    //! where `M` is the length of the sequences, which are all assumed to
    //! have the same length. Assuming the sequences to all have the same size
    //! allows the library to perform some optimizations. To zip sequences
    //! that may have different lengths, `zip_shortest_with` should be used
    //! instead. Also note that it is an error to provide no sequence at all,
    //! i.e. `zip_with` expects at least one sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/zip_with.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto zip_with = [](auto&& f, auto&& x1, ..., auto&& xn) {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct zip_with_impl : zip_with_impl<S, when<true>> { };

    struct zip_with_t {
        template <typename F, typename Xs, typename ...Ys>
        constexpr auto operator()(F&& f, Xs&& xs, Ys&& ...ys) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr zip_with_t zip_with{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_ZIP_WITH_HPP

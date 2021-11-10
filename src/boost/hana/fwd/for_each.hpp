/*!
@file
Forward declares `boost::hana::for_each`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_FOR_EACH_HPP
#define BOOST_HANA_FWD_FOR_EACH_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Perform an action on each element of a foldable, discarding
    //! the result each time.
    //! @ingroup group-Foldable
    //!
    //! Iteration is done from left to right, i.e. in the same order as when
    //! using `fold_left`. If the structure is not finite, this method will
    //! not terminate.
    //!
    //!
    //! @param xs
    //! The structure to iterate over.
    //!
    //! @param f
    //! A function called as `f(x)` for each element `x` of the structure.
    //! The result of `f(x)`, whatever it is, is ignored.
    //!
    //!
    //! Example
    //! -------
    //! @include example/for_each.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto for_each = [](auto&& xs, auto&& f) -> void {
        tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct for_each_impl : for_each_impl<T, when<true>> { };

    struct for_each_t {
        template <typename Xs, typename F>
        constexpr void operator()(Xs&& xs, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr for_each_t for_each{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_FOR_EACH_HPP

/*!
@file
Forward declares `boost::hana::count`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_COUNT_HPP
#define BOOST_HANA_FWD_COUNT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return the number of elements in the structure that compare equal to
    //! a given value.
    //! @ingroup group-Foldable
    //!
    //! Given a Foldable structure `xs` and a value `value`, `count` returns
    //! an unsigned integral, or a Constant thereof, representing the number
    //! of elements of `xs` that compare equal to `value`. For this method to
    //! be well-defined, all the elements of the structure must be Comparable
    //! with the given value.
    //!
    //!
    //! @param xs
    //! The structure whose elements are counted.
    //!
    //! @param value
    //! A value compared with each element in the structure. Elements
    //! that compare equal to this value are counted, others are not.
    //!
    //!
    //! Example
    //! -------
    //! @include example/count.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto count = [](auto&& xs, auto&& value) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct count_impl : count_impl<T, when<true>> { };

    struct count_t {
        template <typename Xs, typename Value>
        constexpr auto operator()(Xs&& xs, Value&& value) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr count_t count{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_COUNT_HPP

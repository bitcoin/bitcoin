/*!
@file
Forward declares `boost::hana::accessors`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_ACCESSORS_HPP
#define BOOST_HANA_FWD_ACCESSORS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns a `Sequence` of pairs representing the accessors of the
    //! data structure.
    //! @ingroup group-Struct
    //!
    //! Given a `Struct` `S`, `accessors<S>()` is a `Sequence` of `Product`s
    //! where the first element of each pair is the "name" of a member of
    //! the `Struct`, and the second element of each pair is a function that
    //! can be used to access that member when given an object of the proper
    //! data type. As described in the global documentation for `Struct`, the
    //! accessor functions in this sequence must be move-independent.
    //!
    //!
    //! Example
    //! -------
    //! @include example/accessors.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename S>
    constexpr auto accessors = []() {
        return tag-dispatched;
    };
#else
    template <typename S, typename = void>
    struct accessors_impl : accessors_impl<S, when<true>> { };

    template <typename S>
    struct accessors_t;

    template <typename S>
    BOOST_HANA_INLINE_VARIABLE constexpr accessors_t<S> accessors{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_ACCESSORS_HPP

/*!
@file
Forward declares `boost::hana::size`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_SIZE_HPP
#define BOOST_HANA_FWD_SIZE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/length.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Equivalent to `length`; provided for consistency with the
    //! standard library.
    //! @ingroup group-Foldable
    //!
    //! This method is an alias to `length` provided for convenience and
    //! consistency with the standard library. As an alias, `size` is not
    //! tag-dispatched on its own and `length` should be customized instead.
    //!
    //!
    //! Example
    //! -------
    //! @include example/size.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto size = hana::length;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_SIZE_HPP

/*!
@file
Defines an equivalent to the proposed `std::void_t`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_VOID_T_HPP
#define BOOST_HANA_DETAIL_VOID_T_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename ...>
    using void_t = void;
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_VOID_T_HPP

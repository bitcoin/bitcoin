/*!
@file
Defines `boost::hana::detail::create`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_CREATE_HPP
#define BOOST_HANA_DETAIL_CREATE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    //! @ingroup group-details
    //! Implementation of the generic `std::make_xxx` pattern for arbitrary
    //! `xxx`s.
    template <template <typename ...> class T>
    struct create {
        template <typename ...X>
        constexpr T<typename detail::decay<X>::type...>
        operator()(X&& ...x) const {
            return T<typename detail::decay<X>::type...>{
                static_cast<X&&>(x)...
            };
        }
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_CREATE_HPP

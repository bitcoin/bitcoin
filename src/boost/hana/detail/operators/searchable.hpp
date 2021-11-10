/*!
@file
Defines operators for Searchables.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_OPERATORS_SEARCHABLE_HPP
#define BOOST_HANA_DETAIL_OPERATORS_SEARCHABLE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/at_key.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename Derived>
    struct searchable_operators {
        template <typename Key>
        constexpr decltype(auto) operator[](Key&& key) & {
            return hana::at_key(static_cast<Derived&>(*this),
                                static_cast<Key&&>(key));
        }

        template <typename Key>
        constexpr decltype(auto) operator[](Key&& key) && {
            return hana::at_key(static_cast<Derived&&>(*this),
                                static_cast<Key&&>(key));
        }

        template <typename Key>
        constexpr decltype(auto) operator[](Key&& key) const& {
            return hana::at_key(static_cast<Derived const&>(*this),
                                static_cast<Key&&>(key));
        }
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_OPERATORS_SEARCHABLE_HPP

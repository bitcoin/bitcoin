/*!
@file
Defines `boost::hana::accessors`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_ACCESSORS_HPP
#define BOOST_HANA_ACCESSORS_HPP

#include <boost/hana/fwd/accessors.hpp>

#include <boost/hana/concept/struct.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    template <typename S>
    struct accessors_t {
    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Struct<S>::value,
        "hana::accessors<S> requires 'S' to be a Struct");
    #endif

        constexpr decltype(auto) operator()() const {
            using Accessors = BOOST_HANA_DISPATCH_IF(accessors_impl<S>,
                hana::Struct<S>::value
            );

            return Accessors::apply();
        }
    };

    template <typename S, bool condition>
    struct accessors_impl<S, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    namespace struct_detail {
        template <typename ...>
        struct is_valid { static constexpr bool value = true; };
    }

    template <typename S>
    struct accessors_impl<S, when<
        struct_detail::is_valid<typename S::hana_accessors_impl>::value
    >>
        : S::hana_accessors_impl
    { };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_ACCESSORS_HPP

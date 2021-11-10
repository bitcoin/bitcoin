/*!
@file
Defines `boost::hana::make`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CORE_MAKE_HPP
#define BOOST_HANA_CORE_MAKE_HPP

#include <boost/hana/fwd/core/make.hpp>

#include <boost/hana/config.hpp>
#include <boost/hana/core/default.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Datatype, typename>
    struct make_impl : make_impl<Datatype, when<true>> { };
    //! @endcond

    template <typename Datatype, bool condition>
    struct make_impl<Datatype, when<condition>> : default_ {
        template <typename ...X>
        static constexpr auto make_helper(int, X&& ...x)
            -> decltype(Datatype(static_cast<X&&>(x)...))
        { return Datatype(static_cast<X&&>(x)...); }

        template <typename ...X>
        static constexpr auto make_helper(long, X&& ...) {
            static_assert((sizeof...(X), false),
            "there exists no constructor for the given data type");
        }

        template <typename ...X>
        static constexpr decltype(auto) apply(X&& ...x)
        { return make_helper(int{}, static_cast<X&&>(x)...); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CORE_MAKE_HPP

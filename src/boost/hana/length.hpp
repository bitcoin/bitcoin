/*!
@file
Defines `boost::hana::length`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_LENGTH_HPP
#define BOOST_HANA_LENGTH_HPP

#include <boost/hana/fwd/length.hpp>

#include <boost/hana/concept/foldable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/unpack.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs>
    constexpr auto length_t::operator()(Xs const& xs) const {
        using S = typename hana::tag_of<Xs>::type;
        using Length = BOOST_HANA_DISPATCH_IF(length_impl<S>,
            hana::Foldable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Foldable<S>::value,
        "hana::length(xs) requires 'xs' to be Foldable");
    #endif

        return Length::apply(xs);
    }
    //! @endcond

    namespace detail {
        struct argn {
            template <typename ...Xs>
            constexpr hana::size_t<sizeof...(Xs)> operator()(Xs const& ...) const
            { return {}; }
        };
    }

    template <typename T, bool condition>
    struct length_impl<T, when<condition>> : default_ {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return hana::unpack(xs, detail::argn{});
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_LENGTH_HPP

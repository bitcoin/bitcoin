/*!
@file
Defines `boost::hana::not_`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_NOT_HPP
#define BOOST_HANA_NOT_HPP

#include <boost/hana/fwd/not.hpp>

#include <boost/hana/concept/constant.hpp>
#include <boost/hana/concept/logical.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/detail/canonical_constant.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename X>
    constexpr decltype(auto) not_t::operator()(X&& x) const {
        using Bool = typename hana::tag_of<X>::type;
        using Not = BOOST_HANA_DISPATCH_IF(hana::not_impl<Bool>,
            hana::Logical<Bool>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Logical<Bool>::value,
        "hana::not_(cond) requires 'cond' to be a Logical");
    #endif

        return Not::apply(static_cast<X&&>(x));
    }
    //! @endcond

    template <typename L, bool condition>
    struct not_impl<L, when<condition>> : hana::default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    template <typename L>
    struct not_impl<L, hana::when<std::is_arithmetic<L>::value>> {
        template <typename Cond>
        static constexpr Cond apply(Cond const& cond)
        { return static_cast<Cond>(cond ? false : true); }
    };

    namespace detail {
        template <typename C, typename X>
        struct constant_from_not {
            static constexpr auto value = hana::not_(hana::value<X>());
            using hana_tag = detail::CanonicalConstant<typename C::value_type>;
        };
    }

    template <typename C>
    struct not_impl<C, hana::when<
        hana::Constant<C>::value &&
        hana::Logical<typename C::value_type>::value
    >> {
        template <typename Cond>
        static constexpr auto apply(Cond const&)
        { return hana::to<C>(detail::constant_from_not<C, Cond>{}); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_NOT_HPP

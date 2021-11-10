/*!
@file
Defines `boost::hana::detail::CanonicalConstant`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_CANONICAL_CONSTANT_HPP
#define BOOST_HANA_DETAIL_CANONICAL_CONSTANT_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    //! @ingroup group-details
    //! Tag representing a canonical `Constant`.
    //!
    //! This is an implementation detail used to provide many models for
    //! stuff like `Monoid`, `Group`, etc. To create a `CanonicalConstant`,
    //! simply create an object with a nested `hana_tag` equal to the proper
    //! specialization of `CanonicalConstant<T>`, and then also provide a
    //! `constexpr` static member `::%value` holding the value of the constant.
    template <typename T>
    struct CanonicalConstant {
        using value_type = T;
    };
} BOOST_HANA_NAMESPACE_END


#include <boost/hana/concept/constant.hpp>
#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/when.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // Constant
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct value_impl<detail::CanonicalConstant<T>> {
        template <typename X>
        static constexpr decltype(auto) apply()
        { return X::value; }
    };

    namespace detail {
        template <typename T, typename X>
        struct canonical_constant {
            static constexpr auto value = hana::to<T>(hana::value<X>());
            using hana_tag = detail::CanonicalConstant<T>;
        };
    }

    template <typename T, typename C>
    struct to_impl<detail::CanonicalConstant<T>, C, when<
        hana::Constant<C>::value &&
        is_convertible<typename C::value_type, T>::value
    >>
        : embedding<is_embedded<typename C::value_type, T>::value>
    {
        template <typename X>
        static constexpr detail::canonical_constant<T, X> apply(X const&)
        { return {}; }
    };

    //////////////////////////////////////////////////////////////////////////
    // IntegralConstant (when value_type is integral)
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct IntegralConstant<detail::CanonicalConstant<T>> {
        static constexpr bool value = std::is_integral<T>::value;
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_CANONICAL_CONSTANT_HPP

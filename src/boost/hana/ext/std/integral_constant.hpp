/*!
@file
Adapts `std::integral_constant` for use with Hana.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXT_STD_INTEGRAL_CONSTANT_HPP
#define BOOST_HANA_EXT_STD_INTEGRAL_CONSTANT_HPP

#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/fwd/core/to.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/fwd/integral_constant.hpp>

#include <type_traits>


#ifdef BOOST_HANA_DOXYGEN_INVOKED
namespace std {
    //! @ingroup group-ext-std
    //! Adapter for `std::integral_constant`s.
    //!
    //! Provided models
    //! ---------------
    //! 1. `Constant` and `IntegralConstant`\n
    //! A `std::integral_constant` is a model of the `IntegralConstant` and
    //! `Constant` concepts, just like `hana::integral_constant`s are. As a
    //! consequence, they are also implicitly a model of the concepts provided
    //! for all models of `Constant`.
    //! @include example/ext/std/integral_constant.cpp
    template <typename T, T v>
    struct integral_constant { };
}
#endif


BOOST_HANA_NAMESPACE_BEGIN
    namespace ext { namespace std {
        template <typename T>
        struct integral_constant_tag { using value_type = T; };
    }}

    namespace detail {
        template <typename T, T v>
        constexpr bool
        is_std_integral_constant(std::integral_constant<T, v>*)
        { return true; }

        constexpr bool is_std_integral_constant(...)
        { return false; }


        template <typename T, T v>
        constexpr bool
        is_hana_integral_constant(hana::integral_constant<T, v>*)
        { return true; }

        constexpr bool is_hana_integral_constant(...)
        { return false; }
    }

    template <typename T>
    struct tag_of<T, when<
        detail::is_std_integral_constant((T*)0) &&
        !detail::is_hana_integral_constant((T*)0)
    >> {
        using type = ext::std::integral_constant_tag<
            typename hana::tag_of<typename T::value_type>::type
        >;
    };

    //////////////////////////////////////////////////////////////////////////
    // Constant/IntegralConstant
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct IntegralConstant<ext::std::integral_constant_tag<T>> {
        static constexpr bool value = true;
    };

    template <typename T, typename C>
    struct to_impl<ext::std::integral_constant_tag<T>, C, when<
        hana::IntegralConstant<C>::value
    >> : embedding<is_embedded<typename C::value_type, T>::value> {
        template <typename N>
        static constexpr auto apply(N const&) {
            return std::integral_constant<T, N::value>{};
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXT_STD_INTEGRAL_CONSTANT_HPP

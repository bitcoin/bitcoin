/*!
@file
Defines `boost::hana::common` and `boost::hana::common_t`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CORE_COMMON_HPP
#define BOOST_HANA_CORE_COMMON_HPP

#include <boost/hana/fwd/core/common.hpp>

#include <boost/hana/concept/constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/canonical_constant.hpp>
#include <boost/hana/detail/std_common_type.hpp>
#include <boost/hana/detail/void_t.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // common
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    template <typename T, typename U, typename>
    struct common : common<T, U, when<true>> { };
    //! @endcond

    template <typename T, typename U, bool condition>
    struct common<T, U, when<condition>>
        : detail::std_common_type<T, U>
    { };

    template <typename T>
    struct common<T, T> {
        using type = T;
    };

    //////////////////////////////////////////////////////////////////////////
    // has_common
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename U, typename>
    struct has_common : std::false_type { };

    template <typename T, typename U>
    struct has_common<T, U, detail::void_t<typename common<T, U>::type>>
        : std::true_type
    { };

    //////////////////////////////////////////////////////////////////////////
    // Provided common data types for Constants
    //////////////////////////////////////////////////////////////////////////
    namespace constant_detail {
        //! @todo
        //! This is an awful hack to avoid having
        //! @code
        //!     common<integral_constant_tag<int>, integral_constant_tag<long>>
        //!         ==
        //!     CanonicalConstant<long>
        //! @endcode
        template <typename A, typename B, typename C>
        struct which {
            using type = detail::CanonicalConstant<C>;
        };

        template <template <typename ...> class A, typename T, typename U, typename C>
        struct which<A<T>, A<U>, C> {
            using type = A<C>;
        };
    }

    template <typename A, typename B>
    struct common<A, B, when<
        hana::Constant<A>::value &&
        hana::Constant<B>::value &&
        has_common<typename A::value_type, typename B::value_type>::value
    >> {
        using type = typename constant_detail::which<
            A, B,
            typename common<typename A::value_type,
                            typename B::value_type>::type
        >::type;
    };

    template <typename A, typename B>
    struct common<A, B, when<
        hana::Constant<A>::value &&
        !hana::Constant<B>::value &&
        has_common<typename A::value_type, B>::value
    >> {
        using type = typename common<typename A::value_type, B>::type;
    };

    template <typename A, typename B>
    struct common<A, B, when<
        !hana::Constant<A>::value &&
        hana::Constant<B>::value &&
        has_common<A, typename B::value_type>::value
    >> {
        using type = typename common<A, typename B::value_type>::type;
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CORE_COMMON_HPP

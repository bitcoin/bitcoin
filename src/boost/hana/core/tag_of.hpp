/*!
@file
Defines `boost::hana::tag_of` and `boost::hana::tag_of_t`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CORE_TAG_OF_HPP
#define BOOST_HANA_CORE_TAG_OF_HPP

#include <boost/hana/fwd/core/tag_of.hpp>

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename T, typename>
    struct tag_of : tag_of<T, when<true>> { };
    //! @endcond

    namespace core_detail {
        template <typename ...>
        struct is_valid { static constexpr bool value = true; };
    }

    template <typename T, bool condition>
    struct tag_of<T, when<condition>> {
        using type = T;
    };

    template <typename T>
    struct tag_of<T, when<
        core_detail::is_valid<typename T::hana_tag>::value
    >> {
        using type = typename T::hana_tag;
    };

    template <typename T> struct tag_of<T const> : tag_of<T> { };
    template <typename T> struct tag_of<T volatile> : tag_of<T> { };
    template <typename T> struct tag_of<T const volatile> : tag_of<T> { };
    template <typename T> struct tag_of<T&> : tag_of<T> { };
    template <typename T> struct tag_of<T&&> : tag_of<T> { };

    namespace detail {
        template <typename T>
        struct has_idempotent_tag
            : std::is_same<hana::tag_of_t<T>,
                           std::remove_const_t<std::remove_reference_t<T>>>
        { };
    }
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CORE_TAG_OF_HPP

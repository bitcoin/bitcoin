/*!
@file
Defines `BOOST_HANA_DISPATCH_IF`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_DISPATCH_IF_HPP
#define BOOST_HANA_DETAIL_DISPATCH_IF_HPP

#include <boost/hana/config.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    struct deleted_implementation {
        template <typename ...T>
        static constexpr auto apply(T&& ...) = delete;
    };

    //! @ingroup group-details
    //! Dispatch to the given implementation method only when a condition is
    //! satisfied.
    //!
    //! If the condition is satisfied, this macro is equivalent to the type
    //! `IMPL`. Otherwise, it is equivalent to a type with a deleted static
    //! function named `apply`. When a tag-dispatching error happens, the
    //! condition should be false and the deleted static function `apply`
    //! will prevent the compiler from generating too much garbage.
    //!
    //! @note
    //! When `BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS` is defined, the
    //! condition is always ignored and this macro expands to the
    //! implementation only.
    //!
    //! @remark
    //! This must be implemented as a macro, because we don't want the
    //! condition to be evaluated at all when
    //! `BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS` is defined.
#ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
    #define BOOST_HANA_DISPATCH_IF(IMPL, ...)                               \
        ::std::conditional_t<                                               \
            (__VA_ARGS__),                                                  \
            IMPL,                                                           \
            ::boost::hana::deleted_implementation                           \
        >                                                                   \
    /**/
#else
    #define BOOST_HANA_DISPATCH_IF(IMPL, ...) IMPL
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_DISPATCH_IF_HPP

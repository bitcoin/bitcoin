/*!
@file
Defines `boost::hana::detail::has_[nontrivial_]common_embedding`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_HAS_COMMON_EMBEDDING_HPP
#define BOOST_HANA_DETAIL_HAS_COMMON_EMBEDDING_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/common.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/detail/void_t.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <template <typename...> class Concept, typename T, typename U, typename = void>
    struct has_common_embedding_impl : std::false_type { };

    template <template <typename...> class Concept, typename T, typename U>
    struct has_common_embedding_impl<Concept, T, U, detail::void_t<
        typename common<T, U>::type
    >> {
        using Common = typename common<T, U>::type;
        using type = std::integral_constant<bool,
            Concept<T>::value &&
            Concept<U>::value &&
            Concept<Common>::value &&
            is_embedded<T, Common>::value &&
            is_embedded<U, Common>::value
        >;
    };

    //! @ingroup group-details
    //! Returns whether `T` and `U` both have an embedding into a
    //! common type.
    //!
    //! If `T` and `U` do not have a common-type, this metafunction returns
    //! false.
    template <template <typename...> class Concept, typename T, typename U>
    using has_common_embedding = typename has_common_embedding_impl<Concept, T, U>::type;

    template <template <typename...> class Concept, typename T, typename U>
    struct has_nontrivial_common_embedding_impl
        : has_common_embedding_impl<Concept, T, U>
    { };

    template <template <typename...> class Concept, typename T>
    struct has_nontrivial_common_embedding_impl<Concept, T, T>
        : std::false_type
    { };

    //! @ingroup group-details
    //! Returns whether `T` and `U` are distinct and both have an embedding
    //! into a common type.
    //!
    //! If `T` and `U` do not have a common-type, this metafunction returns
    //! false.
    template <template <typename...> class Concept, typename T, typename U>
    using has_nontrivial_common_embedding =
        typename has_nontrivial_common_embedding_impl<Concept, T, U>::type;
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_HAS_COMMON_EMBEDDING_HPP

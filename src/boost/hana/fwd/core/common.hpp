/*!
@file
Forward declares `boost::hana::common` and `boost::hana::common_t`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CORE_COMMON_HPP
#define BOOST_HANA_FWD_CORE_COMMON_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-core
    //! %Metafunction returning the common data type between two data types.
    //!
    //! `common` is a natural extension of the `std::common_type` metafunction
    //! to data types. Given two data types `T` and `U`, we say that they share
    //! a common type `C` if both objects of data type `T` and objects of data
    //! type `U` may be converted (using `to`) to an object of data type `C`,
    //! and if that conversion is equality preserving. In other words, this
    //! means that for any objects `t1, t2` of data type `T` and `u1, u2` of
    //! data type `U`, the following law is satisfied:
    //! @code
    //!     to<C>(t1) == to<C>(t2)  if and only if  t1 == t2
    //!     to<C>(u1) == to<C>(u2)  if and only if  u1 == u2
    //! @endcode
    //!
    //! The role of `common` is to provide an alias to such a `C` if it exists.
    //! In other words, if `T` and `U` have a common data type `C`,
    //! `common<T, U>::%type` is an alias to `C`. Otherwise, `common<T, U>`
    //! has no nested `type` and can be used in dependent contexts to exploit
    //! SFINAE. By default, the exact steps followed by `common` to determine
    //! the common type `C` of `T` and `U` are
    //! 1. If `T` and `U` are the same, then `C` is `T`.
    //! 2. Otherwise, if `true ? std::declval<T>() : std::declval<U>()` is
    //!    well-formed, then `C` is the type of this expression after using
    //!    `std::decay` on it. This is exactly the type that would have been
    //!    returned by `std::common_type`, except that custom specializations
    //!    of `std::common_type` are not taken into account.
    //! 3. Otherwise, no common data type is detected and `common<T, U>` does
    //!    not have a nested `type` alias, unless it is specialized explicitly.
    //!
    //! As point 3 suggests, it is also possible (and sometimes necessary) to
    //! specialize `common` in the `boost::hana` namespace for pairs of custom
    //! data types when the default behavior of `common` is not sufficient.
    //! Note that `when`-based specialization is supported when specializing
    //! `common` in the `boost::hana` namespace.
    //!
    //! > #### Rationale for requiring the conversion to be equality-preserving
    //! > This decision is aligned with a proposed concept design for the
    //! > standard library ([N3351][1]). Also, if we did not require this,
    //! > then all data types would trivially share the common data type
    //! > `void`, since all objects can be converted to it.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/common/common.cpp
    //!
    //!
    //! [1]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3351.pdf
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, typename U, optional when-based enabler>
    struct common { see documentation };
#else
    template <typename T, typename U, typename = void>
    struct common;
#endif

    //! @ingroup group-core
    //! %Metafunction returning whether two data types share a common data type.
    //!
    //! Given two data types `T` and `U`, this metafunction simply returns
    //! whether `common<T, U>::%type` is well-formed.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/common/has_common.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, typename U>
    struct has_common { whether common<T, U>::type is well-formed };
#else
    template <typename T, typename U, typename = void>
    struct has_common;
#endif

    //! @ingroup group-core
    //! Alias to `common<T, U>::%type`, provided for convenience.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/common/common_t.cpp
    template <typename T, typename U>
    using common_t = typename common<T, U>::type;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CORE_COMMON_HPP

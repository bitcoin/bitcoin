/*!
@file
Forward declares `boost::hana::tag_of` and `boost::hana::tag_of_t`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CORE_TAG_OF_HPP
#define BOOST_HANA_FWD_CORE_TAG_OF_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-core
    //! %Metafunction returning the tag associated to `T`.
    //!
    //! There are several ways to specify the tag of a C++ type. If it's a
    //! user-defined type, one can define a nested `hana_tag` alias:
    //! @code
    //!     struct MyUserDefinedType {
    //!         using hana_tag = MyTag;
    //!     };
    //! @endcode
    //!
    //! Sometimes, however, the C++ type can't be modified (if it's in a
    //! foreign library) or simply can't have nested types (if it's not a
    //! struct or class). In those cases, using a nested alias is impossible
    //! and so ad-hoc customization is also supported by specializing
    //! `tag_of` in the `boost::hana` namespace:
    //! @code
    //!     struct i_cant_modify_this;
    //!
    //!     namespace boost { namespace hana {
    //!         template <>
    //!         struct tag_of<i_cant_modify_this> {
    //!             using type = MyTag;
    //!         };
    //!     }}
    //! @endcode
    //!
    //! `tag_of` can also be specialized for all C++ types satisfying some
    //! boolean condition using `when`. `when` accepts a single compile-time
    //! boolean and enables the specialization of `tag_of` if and only if
    //! that boolean is `true`. This is similar to the well known C++ idiom
    //! of using a dummy template parameter with `std::enable_if` and relying
    //! on SFINAE. For example, we could specify the tag of all
    //! `fusion::vector`s by doing:
    //! @code
    //!     struct BoostFusionVector;
    //!
    //!     namespace boost { namespace hana {
    //!         template <typename T>
    //!         struct tag_of<T, when<
    //!             std::is_same<
    //!                 typename fusion::traits::tag_of<T>::type,
    //!                 fusion::traits::tag_of<fusion::vector<>>::type
    //!             >::value
    //!         >> {
    //!             using type = BoostFusionVector;
    //!         };
    //!     }}
    //! @endcode
    //!
    //! Also, when it is not specialized and when the given C++ type does not
    //! have a nested `hana_tag` alias, `tag_of<T>` returns `T` itself. This
    //! makes tags a simple extension of normal C++ types. This is _super_
    //! useful, mainly for two reasons. First, this allows Hana to adopt a
    //! reasonable default behavior for some operations involving types that
    //! have no notion of tags. For example, Hana allows comparing with `equal`
    //! any two objects for which a valid `operator==` is defined, and that
    //! without any work on the user side. Second, it also means that you can
    //! ignore tags completely if you don't need their functionality; just use
    //! the normal C++ type of your objects and everything will "just work".
    //!
    //! Finally, also note that `tag_of<T>` is always equivalent to `tag_of<U>`,
    //! where `U` is the type `T` after being stripped of all references and
    //! cv-qualifiers. This makes it unnecessary to specialize `tag_of` for
    //! all reference and cv combinations, which would be a real pain. Also,
    //! `tag_of` is required to be idempotent. In other words, it must always
    //! be the case that `tag_of<tag_of<T>::%type>::%type` is equivalent to
    //! `tag_of<T>::%type`.
    //!
    //! > __Tip 1__\n
    //! > If compile-time performance is a serious concern, consider
    //! > specializing the `tag_of` metafunction in Hana's namespace.
    //! > When unspecialized, the metafunction has to use SFINAE, which
    //! > tends to incur a larger compile-time overhead. For heavily used
    //! > templated types, this can potentially make a difference.
    //!
    //! > __Tip 2__\n
    //! > Consider using `tag_of_t` alias instead of `tag_of`, which
    //! > reduces the amount of typing in dependent contexts.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/tag_of.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, optional when-based enabler>
    struct tag_of { unspecified };
#else
    template <typename T, typename = void>
    struct tag_of;
#endif

    //! @ingroup group-core
    //! Alias to `tag_of<T>::%type`, provided for convenience.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/tag_of_t.cpp
    template <typename T>
    using tag_of_t = typename hana::tag_of<T>::type;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CORE_TAG_OF_HPP

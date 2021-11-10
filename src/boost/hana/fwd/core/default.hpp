/*!
@file
Forward declares `boost::hana::default_` and `boost::hana::is_default`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CORE_DEFAULT_HPP
#define BOOST_HANA_FWD_CORE_DEFAULT_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-core
    //! Mark a tag-dispatched method implementation as a default implementation.
    //!
    //! When defining a new concept with tag-dispatched methods, it is
    //! sometimes possible to provide a default implementation for some
    //! method(s). Making `default_` a base class of such a default
    //! implementation makes it possible to detect whether the method
    //! was dispatched to the default implementation afterwards.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/default.cpp
    struct default_ { };

    //! @ingroup group-core
    //! Returns whether a tag-dispatched method implementation is a default
    //! implementation.
    //!
    //! Given a tag-dispatched method implementation `method_impl<T...>`,
    //! `is_default<method_impl<T...>>` returns whether `method_impl<T...>`
    //! is a default implementation. Note that if there is no default
    //! implementation for the method, then `is_default` should not be
    //! used unless a static assertion saying that "the method is not
    //! implemented" is acceptable.
    //!
    //!
    //! Example
    //! -------
    //! @include example/core/default.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename Method>
    struct is_default { see documentation };
#else
    template <typename T, typename = void>
    struct is_default;
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CORE_DEFAULT_HPP

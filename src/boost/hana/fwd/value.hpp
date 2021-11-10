/*!
@file
Forward declares `boost::hana::value`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_VALUE_HPP
#define BOOST_HANA_FWD_VALUE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Return the compile-time value associated to a constant.
    //! @ingroup group-Constant
    //!
    //! This function returns the value associated to a `Constant`. That
    //! value is always a constant expression. The normal way of using
    //! `value` on an object `c` is
    //! @code
    //!     constexpr auto result = hana::value<decltype(c)>();
    //! @endcode
    //!
    //! However, for convenience, an overload of `value` is provided so that
    //! it can be called as:
    //! @code
    //!     constexpr auto result = hana::value(c);
    //! @endcode
    //!
    //! This overload works by taking a `const&` to its argument, and then
    //! forwarding to the first version of `value`. Since it does not use
    //! its argument, the result can still be a constant expression, even
    //! if the argument is not a constant expression.
    //!
    //! @note
    //! `value<T>()` is tag-dispatched as `value_impl<C>::%apply<T>()`, where
    //! `C` is the tag of `T`.
    //!
    //! @note
    //! `hana::value` is an overloaded function, not a function object.
    //! Hence, it can't be passed to higher-order algorithms. If you need
    //! an equivalent function object, use `hana::value_of` instead.
    //!
    //!
    //! Example
    //! -------
    //! @include example/value.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T>
    constexpr auto value = []() -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename C, typename = void>
    struct value_impl : value_impl<C, when<true>> { };

    template <typename T>
    constexpr decltype(auto) value();

    template <typename T>
    constexpr decltype(auto) value(T const&)
    { return hana::value<T>(); }
#endif

    //! Equivalent to `value`, but can be passed to higher-order algorithms.
    //! @ingroup group-Constant
    //!
    //! This function object is equivalent to `value`, except it can be passed
    //! to higher order algorithms because it is a function object. `value`
    //! can't be passed to higher-order algorithms because it is implemented
    //! as an overloaded function.
    //!
    //! @note
    //! This function is a simple alias to `value`, and hence it is not
    //! tag-dispatched and can't be customized.
    //!
    //!
    //! Example
    //! -------
    //! @include example/value_of.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto value_of = [](auto const& c) -> decltype(auto) {
        return hana::value(c);
    };
#else
    struct value_of_t {
        template <typename T>
        constexpr decltype(auto) operator()(T const&) const
        { return hana::value<T>(); }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr value_of_t value_of{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_VALUE_HPP

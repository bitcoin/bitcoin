/*!
@file
Defines `boost::hana::apply`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_APPLY_HPP
#define BOOST_HANA_FUNCTIONAL_APPLY_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Invokes a Callable with the given arguments.
    //!
    //! This is equivalent to [std::invoke][1] that will be added in C++17.
    //! However, `apply` is a function object instead of a function, which
    //! makes it possible to pass it to higher-order algorithms.
    //!
    //!
    //! @param f
    //! A [Callable][2] to be invoked with the given arguments.
    //!
    //! @param x...
    //! The arguments to call `f` with. The number of `x...` must match the
    //! arity of `f`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/functional/apply.cpp
    //!
    //! [1]: http://en.cppreference.com/w/cpp/utility/functional/invoke
    //! [2]: http://en.cppreference.com/w/cpp/named_req/Callable
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto apply = [](auto&& f, auto&& ...x) -> decltype(auto) {
        return forwarded(f)(forwarded(x)...);
    };
#else
    struct apply_t {
        template <typename F, typename... Args>
        constexpr auto operator()(F&& f, Args&&... args) const ->
            decltype(static_cast<F&&>(f)(static_cast<Args&&>(args)...))
        {
            return static_cast<F&&>(f)(static_cast<Args&&>(args)...);
        }

        template <typename Base, typename T, typename Derived>
        constexpr auto operator()(T Base::*pmd, Derived&& ref) const ->
            decltype(static_cast<Derived&&>(ref).*pmd)
        {
            return static_cast<Derived&&>(ref).*pmd;
        }

        template <typename PMD, typename Pointer>
        constexpr auto operator()(PMD pmd, Pointer&& ptr) const ->
            decltype((*static_cast<Pointer&&>(ptr)).*pmd)
        {
            return (*static_cast<Pointer&&>(ptr)).*pmd;
        }

        template <typename Base, typename T, typename Derived, typename... Args>
        constexpr auto operator()(T Base::*pmf, Derived&& ref, Args&&... args) const ->
            decltype((static_cast<Derived&&>(ref).*pmf)(static_cast<Args&&>(args)...))
        {
            return (static_cast<Derived&&>(ref).*pmf)(static_cast<Args&&>(args)...);
        }

        template <typename PMF, typename Pointer, typename... Args>
        constexpr auto operator()(PMF pmf, Pointer&& ptr, Args&& ...args) const ->
            decltype(((*static_cast<Pointer&&>(ptr)).*pmf)(static_cast<Args&&>(args)...))
        {
            return ((*static_cast<Pointer&&>(ptr)).*pmf)(static_cast<Args&&>(args)...);
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr apply_t apply{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_APPLY_HPP

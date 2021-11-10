/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    static_const_var.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_STATIC_CONST_H
#define BOOST_HOF_GUARD_STATIC_CONST_H

#include <boost/hof/detail/intrinsics.hpp>

namespace boost { namespace hof { namespace detail {

template<class T>
struct static_const_storage
{
    static constexpr T value = T();
};

template<class T>
constexpr T static_const_storage<T>::value;

struct static_const_var_factory
{
    constexpr static_const_var_factory()
    {}

    template<class T>
    constexpr const T& operator=(const T&) const
    {
        static_assert(BOOST_HOF_IS_DEFAULT_CONSTRUCTIBLE(T), "Static const variable must be default constructible");
        return static_const_storage<T>::value;
    }
};
}

template<class T>
constexpr const T& static_const_var()
{
    return detail::static_const_storage<T>::value;
}


}} // namespace boost::hof

#if BOOST_HOF_HAS_RELAXED_CONSTEXPR || defined(_MSC_VER)
#define BOOST_HOF_STATIC_CONSTEXPR const constexpr
#else
#define BOOST_HOF_STATIC_CONSTEXPR static constexpr
#endif

#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_STATIC_AUTO_REF extern __attribute__((weak)) constexpr auto
#else
#define BOOST_HOF_STATIC_AUTO_REF static constexpr auto&
#endif

// On gcc 4.6 use weak variables
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_STATIC_CONST_VAR(name) extern __attribute__((weak)) constexpr auto name
#else
#define BOOST_HOF_STATIC_CONST_VAR(name) static constexpr auto& name = boost::hof::detail::static_const_var_factory()
#endif

#define BOOST_HOF_DECLARE_STATIC_VAR(name, ...) BOOST_HOF_STATIC_CONST_VAR(name) = __VA_ARGS__{}

#endif

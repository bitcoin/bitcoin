/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    constexpr_deduce.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_CONSTEXPR_DEDUCE_H
#define BOOST_HOF_GUARD_FUNCTION_CONSTEXPR_DEDUCE_H

#include <boost/hof/config.hpp>

#define BOOST_HOF_CONST_FOLD(x) (__builtin_constant_p(x) ? (x) : (x))

#ifndef BOOST_HOF_HAS_CONST_FOLD
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_HAS_CONST_FOLD 0
#elif defined(__clang__) || defined (__GNUC__)
#define BOOST_HOF_HAS_CONST_FOLD 1
#else
#define BOOST_HOF_HAS_CONST_FOLD 0
#endif
#endif


namespace boost { namespace hof {

namespace detail {

struct constexpr_deduce
{
    constexpr constexpr_deduce()
    {}
    template<class F>
    constexpr operator F() const
    {
        return F();
    }
};

template<class T>
struct constexpr_deduce_unique
{
    constexpr constexpr_deduce_unique()
    {}
#if BOOST_HOF_HAS_CONST_FOLD
    template<class F>
    constexpr operator const F&() const
    {
        static_assert(BOOST_HOF_IS_EMPTY(F), "Function or lambda expression must be empty");
        return BOOST_HOF_CONST_FOLD(reinterpret_cast<const F&>(static_const_var<T>()));
    }
#else
    template<class F>
    constexpr operator F() const
    {
        // static_assert(std::is_default_constructible<F>::value, "Function not default constructible");
        return F();
    }
#endif
};

}}} // namespace boost::hof

#define BOOST_HOF_DETAIL_CONSTEXPR_DEDUCE true ? boost::hof::detail::constexpr_deduce() :
#define BOOST_HOF_DETAIL_CONSTEXPR_DEDUCE_UNIQUE(T) true ? boost::hof::detail::constexpr_deduce_unique<T>() :

#ifdef _MSC_VER
#define BOOST_HOF_DETAIL_MSVC_CONSTEXPR_DEDUCE BOOST_HOF_DETAIL_CONSTEXPR_DEDUCE
#else
#define BOOST_HOF_DETAIL_MSVC_CONSTEXPR_DEDUCE
#endif

#endif

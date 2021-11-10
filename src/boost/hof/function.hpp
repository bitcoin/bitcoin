/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    function.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_FUNCTION_H
#define BOOST_HOF_GUARD_FUNCTION_FUNCTION_H

/// BOOST_HOF_STATIC_FUNCTION
/// ===================
/// 
/// Description
/// -----------
/// 

/// The `BOOST_HOF_STATIC_FUNCTION` macro allows initializing a function object from a
/// `constexpr` expression. It uses the best practices as outlined in
/// [N4381](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html).
/// This includes using `const` to avoid global state, compile-time
/// initialization of the function object to avoid the [static initialization
/// order fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order), and an
/// external address of the function object that is the same across translation
/// units to avoid possible One-Definition-Rule(ODR) violations.
/// 
/// In C++17, this achieved using the `inline` keyword. However, on older
/// compilers it is initialized using a reference to a static member variable.
/// The static member variable is default constructed, as such the user variable
/// is always default constructed regardless of the expression.
/// 
/// By default, all functions defined with `BOOST_HOF_STATIC_FUNCTION` use the
/// [`boost::hof::reveal`](/include/boost/hof/reveal) adaptor to improve error messages.
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     struct sum_f
///     {
///         template<class T, class U>
///         T operator()(T x, U y) const
///         {
///             return x+y;
///         }
///     };
/// 
///     BOOST_HOF_STATIC_FUNCTION(sum) = sum_f();
///     BOOST_HOF_STATIC_FUNCTION(partial_sum) = boost::hof::partial(sum_f());
/// 
///     int main() {
///         assert(sum(1, 2) == partial_sum(1)(2));
///     }
/// 

#include <boost/hof/reveal.hpp>
#if !BOOST_HOF_HAS_INLINE_VARIABLES
#include <boost/hof/detail/static_const_var.hpp>
#include <boost/hof/detail/constexpr_deduce.hpp>
#endif

namespace boost { namespace hof {

namespace detail {

struct reveal_static_const_factory
{
    constexpr reveal_static_const_factory()
    {}
    template<class F>
    constexpr reveal_adaptor<F> operator=(const F& f) const
    {
#if BOOST_HOF_HAS_INLINE_VARIABLES
#else
        static_assert(BOOST_HOF_IS_DEFAULT_CONSTRUCTIBLE(F), "Static functions must be default constructible");
#endif
        return reveal_adaptor<F>(f);
    }
};
}}} // namespace boost::hof

#if BOOST_HOF_HAS_INLINE_VARIABLES
#define BOOST_HOF_STATIC_FUNCTION(name) inline const constexpr auto name = boost::hof::detail::reveal_static_const_factory()
#else
#define BOOST_HOF_STATIC_FUNCTION(name) BOOST_HOF_STATIC_CONST_VAR(name) = BOOST_HOF_DETAIL_MSVC_CONSTEXPR_DEDUCE boost::hof::detail::reveal_static_const_factory()
#endif

#endif

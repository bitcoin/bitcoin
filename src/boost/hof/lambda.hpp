/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    lambda.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_LAMBDA_H
#define BOOST_HOF_GUARD_FUNCTION_LAMBDA_H

/// BOOST_HOF_STATIC_LAMBDA
/// =================
/// 
/// Description
/// -----------
/// 
/// The `BOOST_HOF_STATIC_LAMBDA` macro allows initializing non-capturing lambdas at
/// compile-time in a `constexpr` expression.
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     const constexpr auto add_one = BOOST_HOF_STATIC_LAMBDA(int x)
///     {
///         return x + 1;
///     };
/// 
///     int main() {
///         assert(3 == add_one(2));
///     }
/// 
/// BOOST_HOF_STATIC_LAMBDA_FUNCTION
/// ==========================
/// 
/// Description
/// -----------
/// 
/// The `BOOST_HOF_STATIC_LAMBDA_FUNCTION` macro allows initializing a global
/// function object that contains non-capturing lambdas. It also ensures that
/// the global function object has a unique address across translation units.
/// This helps prevent possible ODR-violations.
///
/// By default, all functions defined with `BOOST_HOF_STATIC_LAMBDA_FUNCTION` use
/// the `boost::hof::reveal` adaptor to improve error messages.
/// 
/// Note: due to compiler limitations, a global function declared with
/// `BOOST_HOF_STATIC_LAMBDA_FUNCTION` is not guaranteed to have a unique 
/// address across translation units when compiled with pre-C++17 MSVC.
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     BOOST_HOF_STATIC_LAMBDA_FUNCTION(add_one) = [](int x)
///     {
///         return x + 1;
///     };
///     int main() {
///         assert(3 == add_one(2));
///     }
/// 

#include <boost/hof/config.hpp>

// TODO: Move this to a detail header
#if !BOOST_HOF_HAS_CONSTEXPR_LAMBDA || !BOOST_HOF_HAS_INLINE_LAMBDAS

#include <type_traits>
#include <utility>
#include <boost/hof/detail/result_of.hpp>
#include <boost/hof/reveal.hpp>
#include <boost/hof/detail/constexpr_deduce.hpp>
#include <boost/hof/function.hpp>


#ifndef BOOST_HOF_REWRITE_STATIC_LAMBDA
#ifdef _MSC_VER
#define BOOST_HOF_REWRITE_STATIC_LAMBDA 1
#else
#define BOOST_HOF_REWRITE_STATIC_LAMBDA 0
#endif
#endif

namespace boost { namespace hof {

namespace detail {

template<class F>
struct static_function_wrapper
{
    // Default constructor necessary for MSVC
    constexpr static_function_wrapper()
    {}

    static_assert(BOOST_HOF_IS_EMPTY(F), "Function or lambda expression must be empty");

    struct failure
    : failure_for<F>
    {};

    template<class... Ts>
    const F& base_function(Ts&&...) const
    {
        return reinterpret_cast<const F&>(*this);
    }

    BOOST_HOF_RETURNS_CLASS(static_function_wrapper);

    template<class... Ts>
    BOOST_HOF_SFINAE_RESULT(const F&, id_<Ts>...) 
    operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        BOOST_HOF_RETURNS_REINTERPRET_CAST(const F&)(*BOOST_HOF_CONST_THIS)(BOOST_HOF_FORWARD(Ts)(xs)...)
    );
};

struct static_function_wrapper_factor
{
    constexpr static_function_wrapper_factor()
    {}
    template<class F>
    constexpr static_function_wrapper<F> operator= (const F&) const
    {
        // static_assert(std::is_literal_type<static_function_wrapper<F>>::value, "Function wrapper not a literal type");
        return {};
    }
};

#if BOOST_HOF_REWRITE_STATIC_LAMBDA
template<class T, class=void>
struct is_rewritable
: std::false_type
{};

template<class T>
struct is_rewritable<T, typename detail::holder<
    typename T::fit_rewritable_tag
>::type>
: std::is_same<typename T::fit_rewritable_tag, T>
{};

template<class T, class=void>
struct is_rewritable1
: std::false_type
{};

template<class T>
struct is_rewritable1<T, typename detail::holder<
    typename T::fit_rewritable1_tag
>::type>
: std::is_same<typename T::fit_rewritable1_tag, T>
{};


template<class T, class=void>
struct rewrite_lambda;

template<template<class...> class Adaptor, class... Ts>
struct rewrite_lambda<Adaptor<Ts...>, typename std::enable_if<
    is_rewritable<Adaptor<Ts...>>::value
>::type>
{
    typedef Adaptor<typename rewrite_lambda<Ts>::type...> type;
};

template<template<class...> class Adaptor, class T, class... Ts>
struct rewrite_lambda<Adaptor<T, Ts...>, typename std::enable_if<
    is_rewritable1<Adaptor<T, Ts...>>::value
>::type>
{
    typedef Adaptor<typename rewrite_lambda<T>::type, Ts...> type;
};

template<class T>
struct rewrite_lambda<T, typename std::enable_if<
    std::is_empty<T>::value && 
    !is_rewritable<T>::value && 
    !is_rewritable1<T>::value
>::type>
{
    typedef static_function_wrapper<T> type;
};

template<class T>
struct rewrite_lambda<T, typename std::enable_if<
    !std::is_empty<T>::value && 
    !is_rewritable<T>::value && 
    !is_rewritable1<T>::value
>::type>
{
    typedef T type;
};

#endif

template<class T>
struct reveal_static_lambda_function_wrapper_factor
{
    constexpr reveal_static_lambda_function_wrapper_factor()
    {}
#if BOOST_HOF_REWRITE_STATIC_LAMBDA
    template<class F>
    constexpr reveal_adaptor<typename rewrite_lambda<F>::type> 
    operator=(const F&) const
    {
        return reveal_adaptor<typename rewrite_lambda<F>::type>();
    }
#elif BOOST_HOF_HAS_CONST_FOLD
    template<class F>
    constexpr const reveal_adaptor<F>& operator=(const F&) const
    {
        return reinterpret_cast<const reveal_adaptor<F>&>(static_const_var<T>());
    }
#else
    template<class F>
    constexpr reveal_adaptor<static_function_wrapper<F>> operator=(const F&) const
    {
        return {};
    }
#endif
};

}}} // namespace boost::hof

#endif

#if BOOST_HOF_HAS_CONSTEXPR_LAMBDA
#define BOOST_HOF_STATIC_LAMBDA []
#else
#define BOOST_HOF_DETAIL_MAKE_STATIC BOOST_HOF_DETAIL_CONSTEXPR_DEDUCE boost::hof::detail::static_function_wrapper_factor()
#define BOOST_HOF_STATIC_LAMBDA BOOST_HOF_DETAIL_MAKE_STATIC = []
#endif

#if BOOST_HOF_HAS_INLINE_LAMBDAS
#define BOOST_HOF_STATIC_LAMBDA_FUNCTION BOOST_HOF_STATIC_FUNCTION
#else
#define BOOST_HOF_DETAIL_MAKE_REVEAL_STATIC(T) BOOST_HOF_DETAIL_CONSTEXPR_DEDUCE_UNIQUE(T) boost::hof::detail::reveal_static_lambda_function_wrapper_factor<T>()
#define BOOST_HOF_STATIC_LAMBDA_FUNCTION(name) \
struct fit_private_static_function_ ## name {}; \
BOOST_HOF_STATIC_AUTO_REF name = BOOST_HOF_DETAIL_MAKE_REVEAL_STATIC(fit_private_static_function_ ## name)
#endif

#endif

/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    lift.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_LIFT_H
#define BOOST_HOF_GUARD_FUNCTION_LIFT_H

/// BOOST_HOF_LIFT
/// ========
/// 
/// Description
/// -----------
/// 
/// The macros `BOOST_HOF_LIFT` and `BOOST_HOF_LIFT_CLASS` provide a lift operator that
/// will wrap a template function in a function object so it can be passed to
/// higher-order functions. The `BOOST_HOF_LIFT` macro will wrap the function using
/// a generic lambda. As such, it will not preserve `constexpr`. The
/// `BOOST_HOF_LIFT_CLASS` can be used to declare a class that will wrap function.
/// This will preserve `constexpr` and it can be used on older compilers that
/// don't support generic lambdas yet.
/// 
/// Limitation
/// ----------
/// 
/// In C++14, `BOOST_HOF_LIFT` doesn't support `constexpr` due to using a generic
/// lambda. Instead, `BOOST_HOF_LIFT_CLASS` can be used. In C++17, there is no such
/// limitation.
/// 
/// Synopsis
/// --------
/// 
///     // Wrap the function in a generic lambda
///     #define BOOST_HOF_LIFT(...)
/// 
///     // Declare a class named `name` that will forward to the function
///     #define BOOST_HOF_LIFT_CLASS(name, ...)
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
///     #include <algorithm>
/// 
///     // Declare the class `max_f`
///     BOOST_HOF_LIFT_CLASS(max_f, std::max);
/// 
///     int main() {
///         auto my_max = BOOST_HOF_LIFT(std::max);
///         assert(my_max(3, 4) == std::max(3, 4));
///         assert(max_f()(3, 4) == std::max(3, 4));
///     }
/// 

#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/returns.hpp>
#include <boost/hof/lambda.hpp>
#include <boost/hof/detail/forward.hpp>

namespace boost { namespace hof { namespace detail {

template<class F, class NoExcept>
struct lift_noexcept : F
{
    BOOST_HOF_INHERIT_CONSTRUCTOR(lift_noexcept, F);

    template<class... Ts>
    constexpr auto operator()(Ts&&... xs) const
    noexcept(decltype(std::declval<NoExcept>()(BOOST_HOF_FORWARD(Ts)(xs)...)){})
    -> decltype(std::declval<F>()(BOOST_HOF_FORWARD(Ts)(xs)...))
    { return F(*this)(BOOST_HOF_FORWARD(Ts)(xs)...);}
};

template<class F, class NoExcept>
constexpr lift_noexcept<F, NoExcept> make_lift_noexcept(F f, NoExcept)
{
    return {f};
}

}

}} // namespace boost::hof

#define BOOST_HOF_LIFT_IS_NOEXCEPT(...) std::integral_constant<bool, noexcept(decltype(__VA_ARGS__)(__VA_ARGS__))>{}

#if defined (_MSC_VER)
#define BOOST_HOF_LIFT(...) (BOOST_HOF_STATIC_LAMBDA { BOOST_HOF_LIFT_CLASS(fit_local_lift_t, __VA_ARGS__); return fit_local_lift_t(); }())
#elif defined (__clang__)
#define BOOST_HOF_LIFT(...) (boost::hof::detail::make_lift_noexcept( \
    BOOST_HOF_STATIC_LAMBDA(auto&&... xs) \
    -> decltype((__VA_ARGS__)(BOOST_HOF_FORWARD(decltype(xs))(xs)...)) \
    { return (__VA_ARGS__)(BOOST_HOF_FORWARD(decltype(xs))(xs)...); }, \
    BOOST_HOF_STATIC_LAMBDA(auto&&... xs) { return BOOST_HOF_LIFT_IS_NOEXCEPT((__VA_ARGS__)(BOOST_HOF_FORWARD(decltype(xs))(xs)...)); } \
))
#else
#define BOOST_HOF_LIFT(...) (BOOST_HOF_STATIC_LAMBDA(auto&&... xs) BOOST_HOF_RETURNS((__VA_ARGS__)(BOOST_HOF_FORWARD(decltype(xs))(xs)...)))
#endif

#define BOOST_HOF_LIFT_CLASS(name, ...) \
struct name \
{ \
    template<class... Ts> \
    constexpr auto operator()(Ts&&... xs) const \
    BOOST_HOF_RETURNS((__VA_ARGS__)(BOOST_HOF_FORWARD(Ts)(xs)...)) \
}

#endif

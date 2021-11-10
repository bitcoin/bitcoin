/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    repeat.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_REPEAT_H
#define BOOST_HOF_GUARD_REPEAT_H

/// repeat
/// ======
/// 
/// Description
/// -----------
/// 
/// The `repeat` function decorator will repeatedly apply a function a given
/// number of times.
/// 
/// 
/// Synopsis
/// --------
/// 
///     template<class Integral>
///     constexpr auto repeat(Integral);
/// 
/// Semantics
/// ---------
/// 
///     assert(repeat(std::integral_constant<int, 0>{})(f)(xs...) == f(xs...));
///     assert(repeat(std::integral_constant<int, 1>{})(f)(xs...) == f(f(xs...)));
///     assert(repeat(0)(f)(xs...) == f(xs...));
///     assert(repeat(1)(f)(xs...) == f(f(xs...)));
/// 
/// Requirements
/// ------------
/// 
/// Integral must be:
/// 
/// * Integral
/// 
/// Or:
/// 
/// * IntegralConstant
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     struct increment
///     {
///         template<class T>
///         constexpr T operator()(T x) const
///         {
///             return x + 1;
///         }
///     };
/// 
///     int main() {
///         auto increment_by_5 = boost::hof::repeat(std::integral_constant<int, 5>())(increment());
///         assert(increment_by_5(1) == 6);
///     }
/// 

#include <boost/hof/always.hpp>
#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/result_of.hpp>
#include <boost/hof/detail/move.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <boost/hof/decorate.hpp>
#include <boost/hof/first_of.hpp>
#include <boost/hof/detail/recursive_constexpr_depth.hpp>

namespace boost { namespace hof { namespace detail {

template<int N>
struct repeater
{
    template<class F, class... Ts>
    constexpr BOOST_HOF_SFINAE_RESULT(repeater<N-1>, id_<const F&>, result_of<const F&, id_<Ts>...>) 
    operator()(const F& f, Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        repeater<N-1>()(f, f(BOOST_HOF_FORWARD(Ts)(xs)...))
    );
};

template<>
struct repeater<0>
{
    template<class F, class T>
    constexpr T operator()(const F&, T&& x) const
    BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(T(x))
    {
        return x;
    }
};

struct repeat_constant_decorator
{
    template<class Integral, class F, class... Ts>
    constexpr auto operator()(Integral, const F& f, Ts&&... xs) const BOOST_HOF_RETURNS
    (
        detail::repeater<Integral::type::value>()
        (
            f, 
            BOOST_HOF_FORWARD(Ts)(xs)...
        )
    );
};

template<int Depth>
struct repeat_integral_decorator
{
    template<class Integral, class F, class T, class... Ts, class Self=repeat_integral_decorator<Depth-1>>
    constexpr auto operator()(Integral n, const F& f, T&& x, Ts&&... xs) const BOOST_HOF_RETURNS
    (
        (n) ? 
            Self()(n-1, f, f(BOOST_HOF_FORWARD(T)(x), BOOST_HOF_FORWARD(Ts)(xs)...)) :
            BOOST_HOF_FORWARD(T)(x)
    );
};

template<>
struct repeat_integral_decorator<0>
{
    template<class Integral, class F, class T, class Self=repeat_integral_decorator<0>>
#if BOOST_HOF_HAS_RELAXED_CONSTEXPR
    constexpr
#endif
    auto operator()(Integral n, const F& f, T x) const 
    BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT((n--, f(BOOST_HOF_FORWARD(T)(x))))
    -> decltype(f(BOOST_HOF_FORWARD(T)(x)))
    {
        while(n > 0)
        {
            n--;
            x = f(BOOST_HOF_FORWARD(T)(x));
        }
        return x;
    }
    // TODO: Add overload for lvalue
};

}

#if BOOST_HOF_HAS_RELAXED_CONSTEXPR
#define BOOST_HOF_REPEAT_CONSTEXPR_DEPTH 1
#else
#define BOOST_HOF_REPEAT_CONSTEXPR_DEPTH BOOST_HOF_RECURSIVE_CONSTEXPR_DEPTH
#endif

BOOST_HOF_DECLARE_STATIC_VAR(repeat, decorate_adaptor<
    boost::hof::first_of_adaptor<
    detail::repeat_constant_decorator, 
    detail::repeat_integral_decorator<BOOST_HOF_REPEAT_CONSTEXPR_DEPTH>
>>);

}} // namespace boost::hof

#endif

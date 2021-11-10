/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    apply_eval.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_APPLY_EVAL_H
#define BOOST_HOF_GUARD_APPLY_EVAL_H

/// apply_eval
/// ==========
/// 
/// Description
/// -----------
/// 
/// The `apply_eval` function work like [`apply`](/include/boost/hof/apply), except it calls
/// [`eval`](/include/boost/hof/eval) on each of its arguments. Each [`eval`](/include/boost/hof/eval) call is
/// always ordered from left-to-right.
/// 
/// Synopsis
/// --------
/// 
///     template<class F, class... Ts>
///     constexpr auto apply_eval(F&& f, Ts&&... xs);
/// 
/// Semantics
/// ---------
/// 
///     assert(apply_eval(f)(xs...) == f(eval(xs)...));
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [ConstInvocable](ConstInvocable)
/// 
/// Ts must be:
/// 
/// * [EvaluatableFunctionObject](EvaluatableFunctionObject)
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
///     int main() {
///         assert(boost::hof::apply_eval(sum_f(), []{ return 1; }, []{ return 2; }) == 3);
///     }
/// 

#include <boost/hof/config.hpp>
#include <boost/hof/returns.hpp>
#include <boost/hof/detail/forward.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <boost/hof/apply.hpp>
#include <boost/hof/eval.hpp>

#if BOOST_HOF_NO_ORDERED_BRACE_INIT
#include <boost/hof/pack.hpp>
#include <boost/hof/capture.hpp>
#endif

namespace boost { namespace hof {

namespace detail {

#if BOOST_HOF_NO_ORDERED_BRACE_INIT
template<class R, class F, class Pack>
constexpr R eval_ordered(const F& f, Pack&& p)
{
    return p(f);
}

template<class R, class F, class Pack, class T, class... Ts>
constexpr R eval_ordered(const F& f, Pack&& p, T&& x, Ts&&... xs)
{
    return boost::hof::detail::eval_ordered<R>(f, boost::hof::pack_join(BOOST_HOF_FORWARD(Pack)(p), boost::hof::pack_forward(boost::hof::eval(x))), BOOST_HOF_FORWARD(Ts)(xs)...);
}
#else
template<class R>
struct eval_helper
{
    R result;

    template<class F, class... Ts>
    constexpr eval_helper(const F& f, Ts&&... xs) : result(boost::hof::apply(f, BOOST_HOF_FORWARD(Ts)(xs)...))
    {}
};

template<>
struct eval_helper<void>
{
    int x;
    template<class F, class... Ts>
    constexpr eval_helper(const F& f, Ts&&... xs) : x((boost::hof::apply(f, BOOST_HOF_FORWARD(Ts)(xs)...), 0))
    {}
};
#endif

struct apply_eval_f
{
    template<class F, class... Ts, class R=decltype(
        boost::hof::apply(std::declval<const F&>(), boost::hof::eval(std::declval<Ts>())...)
    ),
    class=typename std::enable_if<(!std::is_void<R>::value)>::type 
    >
    constexpr R operator()(const F& f, Ts&&... xs) const BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(boost::hof::apply(f, boost::hof::eval(BOOST_HOF_FORWARD(Ts)(xs))...))
    {
        return
#if BOOST_HOF_NO_ORDERED_BRACE_INIT
        boost::hof::detail::eval_ordered<R>
            (f, boost::hof::pack(), BOOST_HOF_FORWARD(Ts)(xs)...);
#else
        boost::hof::detail::eval_helper<R>
            {f, boost::hof::eval(BOOST_HOF_FORWARD(Ts)(xs))...}.result;
#endif
    }

    template<class F, class... Ts, class R=decltype(
        boost::hof::apply(std::declval<const F&>(), boost::hof::eval(std::declval<Ts>())...)
    ),
    class=typename std::enable_if<(std::is_void<R>::value)>::type 
    >
    constexpr typename detail::holder<Ts...>::type 
    operator()(const F& f, Ts&&... xs) const BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(boost::hof::apply(f, boost::hof::eval(BOOST_HOF_FORWARD(Ts)(xs))...))
    {
        return (typename detail::holder<Ts...>::type)
#if BOOST_HOF_NO_ORDERED_BRACE_INIT
        boost::hof::detail::eval_ordered<R>
            (f, boost::hof::pack(), BOOST_HOF_FORWARD(Ts)(xs)...);
#else
        boost::hof::detail::eval_helper<R>
            {f, boost::hof::eval(BOOST_HOF_FORWARD(Ts)(xs))...};
#endif
    }
};

}

BOOST_HOF_DECLARE_STATIC_VAR(apply_eval, detail::apply_eval_f);

}} // namespace boost::hof

#endif

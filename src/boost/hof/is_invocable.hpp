/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    is_invocable.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_IS_CALLABLE_H
#define BOOST_HOF_GUARD_IS_CALLABLE_H

/// is_invocable
/// ===========
/// 
/// Description
/// -----------
/// 
/// The `is_invocable` metafunction checks if the function is callable with
/// certain parameters.
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [Invocable](Invocable)
/// 
/// Synopsis
/// --------
/// 
///     template<class F, class... Ts>
///     struct is_invocable;
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     using namespace boost::hof;
/// 
///     struct is_invocable_class
///     {
///         void operator()(int) const
///         {
///         }
///     };
///     static_assert(is_invocable<is_invocable_class, int>(), "Not callable");
/// 
///     int main() {}
/// 


#include <boost/hof/detail/can_be_called.hpp>
#include <boost/hof/apply.hpp>

namespace boost { namespace hof {

template<class F, class... Ts>
struct is_invocable 
: detail::can_be_called<detail::apply_f, F, Ts...>
{};

template<class F, class... Ts, class... Us>
struct is_invocable<F(Ts...), Us...>
{
    static_assert(!std::is_same<F, F>::value, 
        "The is_invocable<F(Args...)> form is not supported because it is problematic."
        "Please use is_invocable<F, Args...> instead."
    );
};

}} // namespace boost::hof

#endif

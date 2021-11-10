/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    static.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_STATIC_H
#define BOOST_HOF_GUARD_FUNCTION_STATIC_H

/// static
/// ======
/// 
/// Description
/// -----------
/// 
/// The `static_` adaptor is a static function adaptor that allows any
/// default-constructible function object to be static-initialized. Functions
/// initialized by `static_` cannot be used in `constexpr` functions. If the
/// function needs to be statically initialized and called in a `constexpr`
/// context, then a `constexpr` constructor needs to be used rather than
/// `static_`.
/// 
/// Synopsis
/// --------
/// 
///     template<class F>
///     class static_;
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [ConstFunctionObject](ConstFunctionObject)
/// * DefaultConstructible
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
///     using namespace boost::hof;
/// 
///     // In C++ this class can't be static-initialized, because of the non-
///     // trivial default constructor.
///     struct times_function
///     {
///         double factor;
///         times_function() : factor(2)
///         {}
///         template<class T>
///         T operator()(T x) const
///         {
///             return x*factor;
///         }
///     };
/// 
///     static constexpr static_<times_function> times2 = {};
/// 
///     int main() {
///         assert(6 == times2(3));
///     }
/// 

#include <boost/hof/detail/result_of.hpp>
#include <boost/hof/reveal.hpp>

namespace boost { namespace hof { 

template<class F>
struct static_
{

    struct failure
    : failure_for<F>
    {};

    const F& base_function() const
    BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(F)
    {
        static F f;
        return f;
    }

    BOOST_HOF_RETURNS_CLASS(static_);

    template<class... Ts>
    BOOST_HOF_SFINAE_RESULT(F, id_<Ts>...) 
    operator()(Ts && ... xs) const
    BOOST_HOF_SFINAE_RETURNS(BOOST_HOF_CONST_THIS->base_function()(BOOST_HOF_FORWARD(Ts)(xs)...));
};


}} // namespace boost::hof

#endif

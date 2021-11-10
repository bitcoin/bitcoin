/*=============================================================================
    Copyright (c) 2016 Paul Fultz II
    limit.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_LIMIT_H
#define BOOST_HOF_GUARD_LIMIT_H

/// limit
/// =====
/// 
/// Description
/// -----------
/// 
/// The `limit` function decorator annotates the function with the max number
/// of parameters. The `limit_c` version can be used to give the max number
/// directly(instead of relying on an integral constant). The parameter limit
/// can be read by using the [`function_param_limit`](function_param_limit)
/// trait. Using `limit` is useful to improve error reporting with partially
/// evaluated functions.
/// 
/// Synopsis
/// --------
/// 
///     template<class IntegralConstant>
///     constexpr auto limit(IntegralConstant);
/// 
///     template<std::size_t N, class F>
///     constexpr auto limit_c(F);
/// 
/// Requirements
/// ------------
/// 
/// IntegralConstant must be:
/// 
/// * IntegralConstant
/// 
/// F must be:
/// 
/// * [ConstInvocable](ConstInvocable)
/// * MoveConstructible
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
///     using namespace boost::hof;
/// 
///     struct sum_f
///     {
///         template<class T>
///         int operator()(T x, T y) const
///         {
///             return x+y;
///         }
///     };
///     BOOST_HOF_STATIC_FUNCTION(sum) = limit_c<2>(sum_f());
/// 
///     int main() {
///         assert(3 == sum(1, 2));
///     }
/// 
/// See Also
/// --------
/// 
/// * [Partial function evaluation](<Partial function evaluation>)
/// * [function_param_limit](function_param_limit)
/// 

#include <boost/hof/detail/callable_base.hpp>
#include <boost/hof/detail/forward.hpp>
#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/move.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <boost/hof/always.hpp>
#include <boost/hof/function_param_limit.hpp>

namespace boost { namespace hof {

namespace detail {
// TODO: Make this work with fit_rewritable1_tag
template<std::size_t N, class F>
struct limit_adaptor : detail::callable_base<F>
{
    typedef std::integral_constant<std::size_t, N> fit_function_param_limit;
    BOOST_HOF_INHERIT_CONSTRUCTOR(limit_adaptor, detail::callable_base<F>)

    template<class... Ts>
    constexpr const detail::callable_base<F>& base_function(Ts&&... xs) const
    {
        return boost::hof::always_ref(*this)(xs...);
    }

    BOOST_HOF_RETURNS_CLASS(limit_adaptor);

    template<class... Ts, class=typename std::enable_if<(sizeof...(Ts) <= N)>::type>
    constexpr BOOST_HOF_SFINAE_RESULT(const detail::callable_base<F>&, id_<Ts>...) 
    operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        (BOOST_HOF_MANGLE_CAST(const detail::callable_base<F>&)(BOOST_HOF_CONST_THIS->base_function(xs...)))
            (BOOST_HOF_FORWARD(Ts)(xs)...)
    );

};

template<std::size_t N>
struct make_limit_f
{
    constexpr make_limit_f()
    {}
    template<class F>
    constexpr limit_adaptor<N, F> operator()(F f) const
    {
        return limit_adaptor<N, F>(static_cast<F&&>(f));
    }
};

struct limit_f
{
    template<class IntegralConstant, std::size_t N=IntegralConstant::type::value>
    constexpr make_limit_f<N> operator()(IntegralConstant) const
    {
        return {};
    }
};

}

template<std::size_t N, class F>
constexpr detail::limit_adaptor<N, F> limit_c(F f)
{
    return detail::limit_adaptor<N, F>(static_cast<F&&>(f));
}

BOOST_HOF_DECLARE_STATIC_VAR(limit, detail::limit_f);

}} // namespace boost::hof

#endif

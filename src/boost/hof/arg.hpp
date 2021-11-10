/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    arg.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_ARGS_H
#define BOOST_HOF_GUARD_FUNCTION_ARGS_H

#include <boost/hof/detail/seq.hpp>
#include <boost/hof/returns.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <utility>

/// arg
/// ===
/// 
/// Description
/// -----------
/// 
/// The `arg` function returns a function object that returns the Nth argument
/// passed to it. It actually starts at 1, so it is not the zero-based index
/// of the argument.
/// 
/// Synopsis
/// --------
/// 
///     template<class IntegralConstant>
///     constexpr auto arg(IntegralConstant);
/// 
///     template<std::size_t N, class... Ts>
///     constexpr auto arg_c(Ts&&...);
/// 
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
///     using namespace boost::hof;
/// 
///     int main() {
///         assert(arg(std::integral_constant<int, 3>())(1,2,3,4,5) == 3);
///     }
/// 

namespace boost { namespace hof {

namespace detail {

template<class T>
struct perfect_ref
{
    typedef T type;
    typedef typename std::remove_reference<T>::type value_type;
    T&& value;
    constexpr perfect_ref(value_type& x) noexcept
    : value(BOOST_HOF_FORWARD(T)(x))
    {}
};

template<std::size_t N>
struct ignore
{
    template<class T>
    constexpr ignore(T&&...) noexcept
    {}
};

template<std::size_t... N>
struct args_at
{
    template<class T, class... Ts>
    constexpr auto operator()(ignore<N>..., T x, Ts...) const 
    BOOST_HOF_RETURNS(BOOST_HOF_FORWARD(typename T::type)(x.value));
};

template<std::size_t... N>
constexpr args_at<N...> make_args_at(seq<N...>) noexcept
{
    return {};
}

template<std::size_t N, class... Ts>
constexpr auto get_args(Ts&&... xs) BOOST_HOF_RETURNS
(
    boost::hof::detail::make_args_at(typename gens<N>::type())(nullptr, BOOST_HOF_RETURNS_CONSTRUCT(perfect_ref<Ts>)(xs)...)
);

template<class T, T N>
struct make_args_f
{
    template<class... Ts, class=typename std::enable_if<(N <= sizeof...(Ts))>::type>
    constexpr auto operator()(Ts&&... xs) const BOOST_HOF_RETURNS
    (
        boost::hof::detail::get_args<N>(BOOST_HOF_FORWARD(Ts)(xs)...)
    );
};

struct arg_f
{
    template<class IntegralConstant>
    constexpr make_args_f<std::size_t, IntegralConstant::value> operator()(IntegralConstant) const noexcept
    {
        return make_args_f<std::size_t, IntegralConstant::value>();
    }
};

}
#if BOOST_HOF_HAS_VARIABLE_TEMPLATES
template<std::size_t N>
BOOST_HOF_STATIC_CONSTEXPR detail::make_args_f<std::size_t, N> arg_c = {};
#else
template<std::size_t N, class... Ts>
constexpr auto arg_c(Ts&&... xs) BOOST_HOF_RETURNS
(
    boost::hof::detail::get_args<N>(BOOST_HOF_FORWARD(Ts)(xs)...)
);
#endif

BOOST_HOF_DECLARE_STATIC_VAR(arg, detail::arg_f);

}} // namespace boost::hof

#endif

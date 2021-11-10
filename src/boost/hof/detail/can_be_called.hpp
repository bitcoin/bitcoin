/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    can_be_called.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_CAN_BE_CALLED_H
#define BOOST_HOF_GUARD_CAN_BE_CALLED_H

#include <boost/hof/config.hpp>
#include <boost/hof/detail/and.hpp>
#include <boost/hof/detail/holder.hpp>
#include <boost/hof/detail/using.hpp>

namespace boost { namespace hof { namespace detail {

#if BOOST_HOF_NO_EXPRESSION_SFINAE
struct dont_care
{
    dont_care(...);
};

template<class T>
struct never_care
{
    typedef dont_care type;
};

struct cant_be_called_type
{};

struct no_type
{};

template<class F>
struct is_callable_wrapper_fallback
{
    template<class... Ts>
    auto operator()(Ts&&...) const 
    -> decltype(std::declval<F>()(std::declval<Ts>()...));
};

template<class T, class U=typename std::remove_cv<typename std::remove_reference<T>::type>::type>
struct is_callable_wrapper_base
: std::conditional<BOOST_HOF_IS_CLASS(U) && !BOOST_HOF_IS_FINAL(U), U, is_callable_wrapper_fallback<U>>
{};

template<class F, class... Ts>
struct is_callable_wrapper : is_callable_wrapper_base<F>::type
{
    is_callable_wrapper();
    typedef cant_be_called_type const &(*pointer_to_function)(typename never_care<Ts>::type...);
    operator pointer_to_function() const;
};

template<class T>
struct not_
: std::integral_constant<bool, !T::value>
{};

template<class F, class... Ts>
struct can_be_called
: not_<std::is_same<cant_be_called_type, typename std::decay<decltype(
    is_callable_wrapper<F, Ts...>()(std::declval<Ts>()...)
)>::type>>
{};

template<class F, class... Ts>
struct check_args;

template<class Res, class... Ts, class... Us>
struct check_args<Res(Us...), Ts...>
: and_<std::is_convertible<Ts, Us>...>
{};

template<class Res, class... Ts, class... Us>
struct can_be_called<Res(*)(Us...), Ts...>
: std::conditional<sizeof...(Ts) == sizeof...(Us), 
    check_args<Res(Us...), Ts...>, 
    std::false_type
>::type
{};

template<class Res, class... Ts, class... Us>
struct can_be_called<Res(Us...), Ts...>
: std::conditional<sizeof...(Ts) == sizeof...(Us), 
    check_args<Res(Us...), Ts...>, 
    std::false_type
>::type
{};

#else

template<class T>
T&& called_val() noexcept;

template<class... Ts>
struct callable_args
{};

template<class F, class Args, class=void>
struct can_be_called_impl
: std::false_type
{};

template<class F, class... Args>
struct can_be_called_impl<F, callable_args<Args...>, typename detail::holder<
    decltype( boost::hof::detail::called_val<F>()(boost::hof::detail::called_val<Args>()...) )
>::type>
: std::true_type
{};

template<class F, class... Ts>
BOOST_HOF_USING(can_be_called, can_be_called_impl<F, detail::callable_args<Ts...>>);

#endif

}}} // namespace boost::hof

#endif

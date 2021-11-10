/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    reveal.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_REVEAL_H
#define BOOST_HOF_GUARD_FUNCTION_REVEAL_H

/// reveal
/// ======
/// 
/// Description
/// -----------
/// 
/// The `reveal` function adaptor helps shows the error messages that get
/// masked on some compilers. Sometimes an error in a function that causes a
/// substitution failure, will remove the function from valid overloads. On
/// compilers without a backtrace for substitution failure, this will mask the
/// error inside the function. The `reveal` adaptor will expose these error
/// messages while still keeping the function SFINAE-friendly.
/// 
/// Sample
/// ------
/// 
/// If we take the `print` example from the quick start guide like this:
/// 
///     namespace adl {
/// 
///     using std::begin;
/// 
///     template<class R>
///     auto adl_begin(R&& r) BOOST_HOF_RETURNS(begin(r));
///     }
/// 
///     BOOST_HOF_STATIC_LAMBDA_FUNCTION(for_each_tuple) = [](const auto& sequence, auto f) BOOST_HOF_RETURNS
///     (
///         boost::hof::unpack(boost::hof::proj(f))(sequence)
///     );
/// 
///     auto print = boost::hof::fix(boost::hof::first_of(
///         [](auto, const auto& x) -> decltype(std::cout << x, void())
///         {
///             std::cout << x << std::endl;
///         },
///         [](auto self, const auto& range) -> decltype(self(*adl::adl_begin(range)), void())
///         {
///             for(const auto& x:range) self(x);
///         },
///         [](auto self, const auto& tuple) -> decltype(for_each_tuple(tuple, self), void())
///         {
///             return for_each_tuple(tuple, self);
///         }
///     ));
/// 
/// Which prints numbers and vectors:
/// 
///     print(5);
/// 
///     std::vector<int> v = { 1, 2, 3, 4 };
///     print(v);
/// 
/// However, if we pass a type that can't be printed, we get an error like
/// this:
/// 
///     print.cpp:49:5: error: no matching function for call to object of type 'boost::hof::fix_adaptor<boost::hof::first_of_adaptor<(lambda at print.cpp:29:9), (lambda at print.cpp:33:9), (lambda at print.cpp:37:9)> >'
///         print(foo{});
///         ^~~~~
///     fix.hpp:158:5: note: candidate template ignored: substitution failure [with Ts = <foo>]: no matching function for call to object of type 'const boost::hof::first_of_adaptor<(lambda at
///           print.cpp:29:9), (lambda at print.cpp:33:9), (lambda at print.cpp:37:9)>'
///         operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
/// 
/// Which is short and gives very little information why it can't be called.
/// It doesn't even show the overloads that were try. However, using the
/// `reveal` adaptor we can get more info about the error like this:
/// 
///     print.cpp:49:5: error: no matching function for call to object of type 'boost::hof::reveal_adaptor<boost::hof::fix_adaptor<boost::hof::first_of_adaptor<(lambda at print.cpp:29:9), (lambda at print.cpp:33:9),
///           (lambda at print.cpp:37:9)> >, boost::hof::fix_adaptor<boost::hof::first_of_adaptor<(lambda at print.cpp:29:9), (lambda at print.cpp:33:9), (lambda at print.cpp:37:9)> > >'
///         boost::hof::reveal(print)(foo{});
///         ^~~~~~~~~~~~~~~~~~
///     reveal.hpp:149:20: note: candidate template ignored: substitution failure [with Ts = <foo>, $1 = void]: no matching function for call to object of type '(lambda at print.cpp:29:9)'
///         constexpr auto operator()(Ts&&... xs) const
///                        ^
///     reveal.hpp:149:20: note: candidate template ignored: substitution failure [with Ts = <foo>, $1 = void]: no matching function for call to object of type '(lambda at print.cpp:33:9)'
///         constexpr auto operator()(Ts&&... xs) const
///                        ^
///     reveal.hpp:149:20: note: candidate template ignored: substitution failure [with Ts = <foo>, $1 = void]: no matching function for call to object of type '(lambda at print.cpp:37:9)'
///         constexpr auto operator()(Ts&&... xs) const
///                        ^
///     fix.hpp:158:5: note: candidate template ignored: substitution failure [with Ts = <foo>]: no matching function for call to object of type 'const boost::hof::first_of_adaptor<(lambda at
///           print.cpp:29:9), (lambda at print.cpp:33:9), (lambda at print.cpp:37:9)>'
///         operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
/// 
/// So now the error has a note for each of the lambda overloads it tried. Of
/// course this can be improved even further by providing custom reporting of
/// failures.
/// 
/// Synopsis
/// --------
/// 
///     template<class F>
///     reveal_adaptor<F> reveal(F f);
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [ConstInvocable](ConstInvocable)
/// * MoveConstructible
/// 
/// Reporting Failures
/// ------------------
/// 
/// By default, `reveal` reports the substitution failure by trying to call
/// the function. However, more detail expressions can be be reported from a
/// template alias by using `as_failure`. This is done by defining a nested
/// `failure` struct in the function object and then inheriting from
/// `as_failure`. Also multiple failures can be reported by using
/// `with_failures`.
/// 
/// Synopsis
/// --------
/// 
///     // Report failure by instantiating the Template
///     template<template<class...> class Template>
///     struct as_failure;
/// 
///     // Report multiple falures
///     template<class... Failures>
///     struct with_failures;
/// 
///     // Report the failure for each function
///     template<class... Fs>
///     struct failure_for;
/// 
///     // Get the failure of a function
///     template<class F>
///     struct get_failure;
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
///         using sum_failure = decltype(std::declval<T>()+std::declval<U>());
/// 
///         struct failure
///         : boost::hof::as_failure<sum_failure>
///         {};
/// 
///         template<class T, class U>
///         auto operator()(T x, U y) const BOOST_HOF_RETURNS(x+y);
///     };
/// 
///     int main() {
///         assert(sum_f()(1, 2) == 3);
///     }
/// 

#include <boost/hof/always.hpp>
#include <boost/hof/returns.hpp>
#include <boost/hof/is_invocable.hpp>
#include <boost/hof/identity.hpp>
#include <boost/hof/detail/move.hpp>
#include <boost/hof/detail/callable_base.hpp>
#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/holder.hpp>
#include <boost/hof/detail/join.hpp>
#include <boost/hof/detail/make.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <boost/hof/detail/using.hpp>

#ifndef BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS
#ifdef __clang__
#define BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS 1
#else
#define BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS 0
#endif
#endif

namespace boost { namespace hof { 

namespace detail {


template<class T, class=void>
struct has_failure
: std::false_type
{};

template<class T>
struct has_failure<T, typename holder<
    typename T::failure
>::type>
: std::true_type
{};

struct identity_failure
{
    template<class T>
    T operator()(T&& x);

    template<class T>
    static T&& val();
#if BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS
    template<template<class...> class Template, class... Ts>
    BOOST_HOF_USING(defer, Template<Ts...>);
#else
    template<template<class...> class Template, class... Ts>
    static auto defer(Ts&&...) -> Template<Ts...>;
#endif

};

}

template<class F, class=void>
struct get_failure
{
    template<class... Ts>
    struct of
    {
#if BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS
        template<class Id>
        using apply = decltype(Id()(std::declval<F>())(std::declval<Ts>()...));
#else
        template<class Id>
        static auto apply(Id id) -> decltype(id(std::declval<F>())(std::declval<Ts>()...));
#endif
    };
};

template<class F>
struct get_failure<F, typename std::enable_if<detail::has_failure<F>::value>::type>
: F::failure
{};

template<template<class...> class Template>
struct as_failure
{
    template<class... Ts>
    struct of
    {
#if BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS
        template<class Id>
        using apply = typename Id::template defer<Template, Ts...>;
#else
        template<class Id>
        static auto apply(Id) -> decltype(Id::template defer<Template, Ts...>());
#endif
    };
};

namespace detail {
template<class Failure, class... Ts>
BOOST_HOF_USING_TYPENAME(apply_failure, Failure::template of<Ts...>);

template<class F, class Failure>
struct reveal_failure
{
    // Add default constructor to make clang 3.4 happy
    constexpr reveal_failure()
    {}
    // This is just a placeholder to produce a note in the compiler, it is
    // never called
    template<
        class... Ts, 
        class=typename std::enable_if<(!is_invocable<F, Ts...>::value)>::type
    >
    constexpr auto operator()(Ts&&... xs) const
#if BOOST_HOF_REVEAL_USE_TEMPLATE_ALIAS
        -> typename apply_failure<Failure, Ts...>::template apply<boost::hof::detail::identity_failure>;
#else
        -> decltype(apply_failure<Failure, Ts...>::apply(boost::hof::detail::identity_failure()));
#endif
};

template<class F, class Failure=get_failure<F>, class=void>
struct traverse_failure 
: reveal_failure<F, Failure>
{
    constexpr traverse_failure()
    {}
};

template<class F, class Failure>
struct traverse_failure<F, Failure, typename holder< 
    typename Failure::children
>::type> 
: Failure::children::template overloads<F>
{
    constexpr traverse_failure()
    {}
};

template<class Failure, class Transform, class=void>
struct transform_failures 
: Transform::template apply<Failure>
{};

template<class Failure, class Transform>
struct transform_failures<Failure, Transform, typename holder< 
    typename Failure::children
>::type> 
: Failure::children::template transform<Transform>
{};

}

template<class Failure, class... Failures>
struct failures;

template<class... Fs>
struct with_failures
{
    typedef BOOST_HOF_JOIN(failures, Fs...) children;
};

template<class Failure, class... Failures>
struct failures 
{
    template<class Transform>
    BOOST_HOF_USING(transform, with_failures<detail::transform_failures<Failure, Transform>, detail::transform_failures<Failures, Transform>...>);

    template<class F, class FailureBase=BOOST_HOF_JOIN(failures, Failures...)>
    struct overloads
    : detail::traverse_failure<F, Failure>, FailureBase::template overloads<F>
    {
        constexpr overloads()
        {}
        using detail::traverse_failure<F, Failure>::operator();
        using FailureBase::template overloads<F>::operator();
    };
};

template<class Failure>
struct failures<Failure>
{
    template<class Transform>
    BOOST_HOF_USING(transform, with_failures<detail::transform_failures<Failure, Transform>>);

    template<class F>
    BOOST_HOF_USING(overloads, detail::traverse_failure<F, Failure>);
};

template<class Transform, class... Fs>
struct failure_map
: with_failures<detail::transform_failures<get_failure<Fs>, Transform>...>
{};

template<class... Fs>
struct failure_for
: with_failures<get_failure<Fs>...>
{};

template<class F, class Base=detail::callable_base<F>>
struct reveal_adaptor
: detail::traverse_failure<Base>, Base
{
    typedef reveal_adaptor fit_rewritable1_tag;
    using detail::traverse_failure<Base>::operator();
    using Base::operator();

    BOOST_HOF_INHERIT_CONSTRUCTOR(reveal_adaptor, Base);
};
// Avoid double reveals, it causes problem on gcc 4.6
template<class F>
struct reveal_adaptor<reveal_adaptor<F>>
: reveal_adaptor<F>
{
    typedef reveal_adaptor fit_rewritable1_tag;
    BOOST_HOF_INHERIT_CONSTRUCTOR(reveal_adaptor, reveal_adaptor<F>);
};

BOOST_HOF_DECLARE_STATIC_VAR(reveal, detail::make<reveal_adaptor>);

}} // namespace boost::hof

#endif

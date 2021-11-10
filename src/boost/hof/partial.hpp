/*=============================================================================
    Copyright (c) 2012 Paul Fultz II
    partial.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_PARTIAL_H
#define BOOST_HOF_GUARD_FUNCTION_PARTIAL_H

/// partial
/// ========
/// 
/// Description
/// -----------
/// 
/// The `partial` function adaptor allows partial application of the function.
/// If the function can not be called with all the parameters, it will return
/// another function. It will repeatedly do this until the function can
/// finally be called. By default, `partial` captures all of its variables by
/// value, just like bind. As such all parameters must be `MoveConstructible`
/// when the function is aprtial application. `std::ref` can be used to
/// capture references instead.
/// 
/// Synopsis
/// --------
/// 
///     template<class F>
///     constexpr partial_adaptor<F> partial(F f);
/// 
/// Semantics
/// ---------
/// 
///     assert(partial(f)(xs...)(ys...) == f(xs..., ys...));
/// 
/// Requirements
/// ------------
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
///     struct sum
///     {
///         template<class T, class U>
///         T operator()(T x, U y) const
///         {
///             return x+y;
///         }
///     };
/// 
///     int main() {
///         assert(3 == partial(sum())(1)(2));
///     }
/// 
/// References
/// ----------
/// 
/// * [Partial application](https://en.wikipedia.org/wiki/Partial_application)
/// * [Currying](https://en.wikipedia.org/wiki/Currying)
/// 

#include <boost/hof/first_of.hpp>
#include <boost/hof/static.hpp>
#include <boost/hof/pipable.hpp>
#include <boost/hof/detail/make.hpp>
#include <boost/hof/detail/static_const_var.hpp>


namespace boost { namespace hof { 

// TODO: Get rid of sequence parameter
// Forward declare partial_adaptor, since it will be used below
template<class F, class Pack=void >
struct partial_adaptor;

BOOST_HOF_DECLARE_STATIC_VAR(partial, detail::make<partial_adaptor>);

namespace detail {

template<class Derived, class F, class Pack>
struct partial_adaptor_invoke
{
    template<class... Ts>
    constexpr const F& get_function(Ts&&...) const noexcept
    {
        return static_cast<const F&>(static_cast<const Derived&>(*this));
    }

    template<class... Ts>
    constexpr const Pack& get_pack(Ts&&...) const noexcept
    {
        return static_cast<const Pack&>(static_cast<const Derived&>(*this));
    }

    BOOST_HOF_RETURNS_CLASS(partial_adaptor_invoke);

    template<class... Ts>
    constexpr BOOST_HOF_SFINAE_RESULT
    (
        typename result_of<decltype(boost::hof::pack_join), 
            id_<const Pack&>, 
            result_of<decltype(boost::hof::pack_forward), id_<Ts>...> 
        >::type,
        id_<F&&>
    ) 
    operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        boost::hof::pack_join
        (
            BOOST_HOF_MANGLE_CAST(const Pack&)(BOOST_HOF_CONST_THIS->get_pack(xs...)), 
            boost::hof::pack_forward(BOOST_HOF_FORWARD(Ts)(xs)...)
        )
        (BOOST_HOF_RETURNS_C_CAST(F&&)(BOOST_HOF_CONST_THIS->get_function(xs...)))
    );
};

#ifdef _MSC_VER
#define BOOST_HOF_PARTIAL_RETURNS(...) -> decltype(__VA_ARGS__) { return (__VA_ARGS__); }
#else
#define BOOST_HOF_PARTIAL_RETURNS BOOST_HOF_SFINAE_RETURNS
#endif

template<class Derived, class F, class Pack>
struct partial_adaptor_join
{
    template<class... Ts>
    constexpr const F& get_function(Ts&&...) const noexcept
    {
        return static_cast<const F&>(static_cast<const Derived&>(*this));
    }

    template<class... Ts>
    constexpr const Pack& get_pack(Ts&&...) const noexcept
    {
        return static_cast<const Pack&>(static_cast<const Derived&>(*this));
    }

    BOOST_HOF_RETURNS_CLASS(partial_adaptor_join);

    template<class... Ts, class=typename std::enable_if<
        ((sizeof...(Ts) + Pack::fit_function_param_limit::value) < function_param_limit<F>::value)
    >::type>
    constexpr auto operator()(Ts&&... xs) const 
#ifdef _MSC_VER
    // Workaround ICE on MSVC
    noexcept(BOOST_HOF_IS_NOTHROW_CONSTRUCTIBLE(F, F&&) && noexcept(boost::hof::pack_join(std::declval<const Pack&>(), boost::hof::pack(BOOST_HOF_FORWARD(Ts)(xs)...))))
#endif
    BOOST_HOF_PARTIAL_RETURNS
    (
        boost::hof::partial
        (
            BOOST_HOF_RETURNS_C_CAST(F&&)(BOOST_HOF_CONST_THIS->get_function(xs...)), 
            boost::hof::pack_join(BOOST_HOF_MANGLE_CAST(const Pack&)(BOOST_HOF_CONST_THIS->get_pack(xs...)), boost::hof::pack(BOOST_HOF_FORWARD(Ts)(xs)...))
        )
    );
};

template<class Derived, class F>
struct partial_adaptor_pack
{

    constexpr partial_adaptor_pack() noexcept
    {}
    
    template<class... Ts>
    constexpr const F& get_function(Ts&&...) const noexcept
    {
        return static_cast<const F&>(static_cast<const Derived&>(*this));
    }

    BOOST_HOF_RETURNS_CLASS(partial_adaptor_pack);

    template<class... Ts, class=typename std::enable_if<
        (sizeof...(Ts) < function_param_limit<F>::value)
    >::type>
    constexpr auto operator()(Ts&&... xs) const 
#ifdef _MSC_VER
    // Workaround ICE on MSVC
    noexcept(BOOST_HOF_IS_NOTHROW_CONSTRUCTIBLE(F, F&&) && noexcept(boost::hof::pack(BOOST_HOF_FORWARD(Ts)(xs)...)))
#endif
    BOOST_HOF_PARTIAL_RETURNS
    (
        boost::hof::partial
        (
            BOOST_HOF_RETURNS_C_CAST(F&&)(BOOST_HOF_CONST_THIS->get_function(xs...)), 
            boost::hof::pack(BOOST_HOF_FORWARD(Ts)(xs)...)
        )
    );
};
template<class F, class Pack>
struct partial_adaptor_base 
{
    typedef basic_first_of_adaptor
    <
        partial_adaptor_invoke<partial_adaptor<F, Pack>, F, Pack>,
        partial_adaptor_join<partial_adaptor<F, Pack>, F, Pack> 
    > type;
};

template<class Derived, class F>
struct partial_adaptor_pack_base
{
    typedef basic_first_of_adaptor
    <
        F,
        partial_adaptor_pack<Derived, F> 
    > type;
};

}

template<class F, class Pack>
struct partial_adaptor : detail::partial_adaptor_base<F, Pack>::type, F, Pack
{
    typedef typename detail::partial_adaptor_base<F, Pack>::type base;

    typedef partial_adaptor fit_rewritable1_tag;
    
    template<class... Ts>
    constexpr const F& base_function(Ts&&...) const noexcept
    {
        return *this;
    }

    constexpr const Pack& get_pack() const noexcept
    {
        return *this;
    }

    using base::operator();

    BOOST_HOF_INHERIT_DEFAULT(partial_adaptor, base, F, Pack);

    template<class X, class S>
    constexpr partial_adaptor(X&& x, S&& seq) 
    BOOST_HOF_NOEXCEPT(BOOST_HOF_IS_NOTHROW_CONSTRUCTIBLE(F, X&&) && BOOST_HOF_IS_NOTHROW_CONSTRUCTIBLE(Pack, S&&))
    : F(BOOST_HOF_FORWARD(X)(x)), Pack(BOOST_HOF_FORWARD(S)(seq))
    {}
};

template<class F>
struct partial_adaptor<F, void> : detail::partial_adaptor_pack_base<partial_adaptor<F, void>, detail::callable_base<F>>::type
{
    typedef typename detail::partial_adaptor_pack_base<partial_adaptor<F, void>, detail::callable_base<F>>::type base;

    typedef partial_adaptor fit_rewritable1_tag;
    
    template<class... Ts>
    constexpr const detail::callable_base<F>& base_function(Ts&&...) const noexcept
    {
        return *this;
    }

    using base::operator();

    BOOST_HOF_INHERIT_CONSTRUCTOR(partial_adaptor, base);

};

// Make partial_adaptor work with pipable_adaptor by removing its pipableness
template<class F>
struct partial_adaptor<pipable_adaptor<F>, void>
: partial_adaptor<F, void>
{
    typedef partial_adaptor<F, void> base;

    typedef partial_adaptor fit_rewritable1_tag;
    BOOST_HOF_INHERIT_CONSTRUCTOR(partial_adaptor, base);
};

template<class F>
struct partial_adaptor<static_<pipable_adaptor<F>>, void>
: partial_adaptor<F, void>
{
    typedef partial_adaptor<F, void> base;

    typedef partial_adaptor fit_rewritable1_tag;

    BOOST_HOF_INHERIT_CONSTRUCTOR(partial_adaptor, base);
};
}} // namespace boost::hof

#endif

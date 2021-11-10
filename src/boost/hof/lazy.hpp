/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    lazy.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_LAZY_H
#define BOOST_HOF_GUARD_FUNCTION_LAZY_H

/// lazy
/// ====
/// 
/// Description
/// -----------
/// 
/// The `lazy` function adaptor returns a function object call wrapper for a
/// function. Calling this wrapper is equivalent to invoking the function. It
/// is a simple form of lambda expressions, but is constexpr friendly. By
/// default, `lazy` captures all of its variables by value, just like `bind`.
/// `std::ref` can be used to capture references instead.
/// 
/// Ultimately, calling `lazy(f)(x)` is the equivalent to calling
/// `std::bind(f, x)` except the lazy version can be called in a constexpr
/// context, as well. The `lazy` adaptor is compatible with `std::bind`, so
/// most of the time `lazy` and `std::bind` can be used interchangeably.
/// 
/// Synopsis
/// --------
/// 
///     template<class F>
///     constexpr lazy_adaptor<F> lazy(F f);
/// 
/// Semantics
/// ---------
/// 
///     assert(lazy(f)(xs...) == std::bind(f, xs...))
///     assert(lazy(f)(xs...)() == f(xs...))
///     assert(lazy(f)(_1)(x) == f(x))
///     assert(lazy(f)(lazy(g)(_1))(x) == f(g(x)))
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
///     int main() {
///         auto add = [](auto x, auto y) { return x+y; };
///         auto increment = lazy(add)(_1, 1);
///         assert(increment(5) == 6);
///     }
/// 
/// References
/// ----------
/// 
/// * [std::bind](http://en.cppreference.com/w/cpp/utility/functional/bind)
/// 

#include <boost/hof/arg.hpp>
#include <boost/hof/first_of.hpp>
#include <boost/hof/always.hpp>
#include <boost/hof/static.hpp>
#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/compressed_pair.hpp>
#include <boost/hof/pack.hpp>
#include <boost/hof/detail/make.hpp>
#include <boost/hof/detail/static_const_var.hpp>
#include <functional>
#include <type_traits>

namespace boost { namespace hof {

namespace detail {

struct placeholder_transformer
{
    template<class T, typename std::enable_if<(std::is_placeholder<T>::value > 0), int>::type = 0>
    constexpr detail::make_args_f<std::size_t, std::is_placeholder<T>::value> operator()(const T&) const noexcept
    {
        return {};
    }
};

struct bind_transformer
{
    template<class T, typename std::enable_if<std::is_bind_expression<T>::value, int>::type = 0>
    constexpr const T& operator()(const T& x) const noexcept
    {
        return x;
    }
};

struct ref_transformer
{
    template<class T>
    constexpr auto operator()(std::reference_wrapper<T> x) const 
    BOOST_HOF_SFINAE_RETURNS(boost::hof::always_ref(x.get()));
};

struct id_transformer
{
    template<class T>
    constexpr auto operator()(T&& x) const 
    BOOST_HOF_SFINAE_RETURNS(always_detail::always_base<T>(BOOST_HOF_FORWARD(T)(x)));
};

BOOST_HOF_DECLARE_STATIC_VAR(pick_transformer, first_of_adaptor<placeholder_transformer, bind_transformer, ref_transformer, id_transformer>);

template<class T, class Pack>
constexpr auto lazy_transform(T&& x, const Pack& p) BOOST_HOF_RETURNS
(
    p(boost::hof::detail::pick_transformer(BOOST_HOF_FORWARD(T)(x)))
);

template<class F, class Pack>
struct lazy_unpack
{
    const F& f;
    const Pack& p;

    constexpr lazy_unpack(const F& fp, const Pack& pp) noexcept
    : f(fp), p(pp)
    {}

    template<class... Ts>
    constexpr auto operator()(Ts&&... xs) const BOOST_HOF_RETURNS
    (
        f(lazy_transform(BOOST_HOF_FORWARD(Ts)(xs), p)...)
    );
};

template<class F, class Pack>
constexpr lazy_unpack<F, Pack> make_lazy_unpack(const F& f, const Pack& p) noexcept
{
    return lazy_unpack<F, Pack>(f, p);
}

template<class F, class Pack>
struct lazy_invoker 
: detail::compressed_pair<F, Pack>
{
    typedef detail::compressed_pair<F, Pack> base_type;
    typedef lazy_invoker fit_rewritable1_tag;

#ifdef _MSC_VER
    BOOST_HOF_INHERIT_CONSTRUCTOR(lazy_invoker, base_type)
#else
    BOOST_HOF_INHERIT_DEFAULT_EMPTY(lazy_invoker, base_type)

    template<class X, class Y, 
        BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(base_type, X&&, Y&&)
    >
    constexpr lazy_invoker(X&& x, Y&& y) 
    BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(base_type, X&&, Y&&)
    : base_type(BOOST_HOF_FORWARD(X)(x), BOOST_HOF_FORWARD(Y)(y))
    {}
#endif

    template<class... Ts>
    constexpr const F& base_function(Ts&&... xs) const noexcept
    {
        return this->first(xs...);
    }

    template<class... Ts>
    constexpr const Pack& get_pack(Ts&&... xs) const noexcept
    {
        return this->second(xs...);
    }

    BOOST_HOF_RETURNS_CLASS(lazy_invoker);

    template<class... Ts>
    constexpr auto operator()(Ts&&... xs) const BOOST_HOF_RETURNS
    (
        BOOST_HOF_MANGLE_CAST(const Pack&)(BOOST_HOF_CONST_THIS->get_pack(xs...))(
            boost::hof::detail::make_lazy_unpack(
                BOOST_HOF_MANGLE_CAST(const F&)(BOOST_HOF_CONST_THIS->base_function(xs...)), 
                boost::hof::pack_forward(BOOST_HOF_FORWARD(Ts)(xs)...)
            )
        )
    );
};

template<class F, class Pack>
constexpr lazy_invoker<F, Pack> make_lazy_invoker(F f, Pack pack)
BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(lazy_invoker<F, Pack>, F&&, Pack&&)
{
    return lazy_invoker<F, Pack>(static_cast<F&&>(f), static_cast<Pack&&>(pack));
}

template<class F>
struct lazy_nullary_invoker : F
{
    BOOST_HOF_INHERIT_CONSTRUCTOR(lazy_nullary_invoker, F);

    template<class... Ts>
    constexpr const F& base_function(Ts&&... xs) const noexcept
    {
        return boost::hof::always_ref(*this)(xs...);
    }

    BOOST_HOF_RETURNS_CLASS(lazy_nullary_invoker);

    template<class... Ts>
    constexpr auto operator()(Ts&&... xs) const BOOST_HOF_RETURNS
    (
        BOOST_HOF_MANGLE_CAST(const F&)(BOOST_HOF_CONST_THIS->base_function(xs...))()
    );
};

template<class F>
constexpr lazy_nullary_invoker<F> make_lazy_nullary_invoker(F f)
BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(lazy_nullary_invoker<F>, F&&)
{
    return lazy_nullary_invoker<F>(static_cast<F&&>(f));
}
}


template<class F>
struct lazy_adaptor : detail::callable_base<F>
{
    BOOST_HOF_INHERIT_CONSTRUCTOR(lazy_adaptor, detail::callable_base<F>);

    template<class... Ts>
    constexpr const detail::callable_base<F>& base_function(Ts&&... xs) const noexcept
    {
        return boost::hof::always_ref(*this)(xs...);
    }

    BOOST_HOF_RETURNS_CLASS(lazy_adaptor);

    template<class T, class... Ts>
    constexpr auto operator()(T x, Ts... xs) const BOOST_HOF_RETURNS
    (
        boost::hof::detail::make_lazy_invoker(BOOST_HOF_RETURNS_C_CAST(detail::callable_base<F>&&)(BOOST_HOF_CONST_THIS->base_function(x, xs...)), 
            boost::hof::pack_basic(BOOST_HOF_RETURNS_STATIC_CAST(T&&)(x), BOOST_HOF_RETURNS_STATIC_CAST(Ts&&)(xs)...))
    );

    // Workaround for gcc 4.7
    template<class Unused=int>
    constexpr detail::lazy_nullary_invoker<F> operator()() const
    BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT(
        boost::hof::detail::make_lazy_nullary_invoker(BOOST_HOF_RETURNS_C_CAST(detail::callable_base<F>&&)(
            BOOST_HOF_CONST_THIS->base_function(BOOST_HOF_RETURNS_CONSTRUCT(Unused)())
        ))
    )
    {
        return boost::hof::detail::make_lazy_nullary_invoker((detail::callable_base<F>&&)(
            this->base_function(Unused())
        ));
    }

    // TODO: Overloads to use with ref qualifiers

    // template<class... Ts>
    // constexpr auto operator()(Ts... xs) const& BOOST_HOF_RETURNS
    // (
    //     boost::hof::detail::make_lazy_invoker(this->base_function(xs...), 
    //         pack(boost::hof::move(xs)...))
    // );

    // template<class... Ts>
    // constexpr auto operator()(Ts... xs) && BOOST_HOF_RETURNS
    // (
    //     boost::hof::detail::make_lazy_invoker((F&&)this->base_function(xs...), 
    //         pack(boost::hof::move(xs)...))
    // );
    
};

BOOST_HOF_DECLARE_STATIC_VAR(lazy, detail::make<lazy_adaptor>);

}} // namespace boost::hof

namespace std {
    template<class F, class Pack>
    struct is_bind_expression<boost::hof::detail::lazy_invoker<F, Pack>>
    : std::true_type
    {};

    template<class F>
    struct is_bind_expression<boost::hof::detail::lazy_nullary_invoker<F>>
    : std::true_type
    {};
}

#endif

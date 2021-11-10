/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    capture.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_CAPTURE_H
#define BOOST_HOF_GUARD_CAPTURE_H

#include <boost/hof/detail/callable_base.hpp>
#include <boost/hof/detail/compressed_pair.hpp>
#include <boost/hof/reveal.hpp>
#include <boost/hof/pack.hpp>
#include <boost/hof/always.hpp>
#include <boost/hof/detail/move.hpp>
#include <boost/hof/detail/result_type.hpp>

/// capture
/// =======
/// 
/// Description
/// -----------
/// 
/// The `capture` function decorator is used to capture values in a function.
/// It provides more flexibility in capturing than the lambda capture list in
/// C++. It provides a way to do move and perfect capturing. The values
/// captured are prepended to the argument list of the function that will be
/// called.
/// 
/// Synopsis
/// --------
/// 
///     // Capture by decaying each value
///     template<class... Ts>
///     constexpr auto capture(Ts&&... xs);
/// 
///     // Capture lvalues by reference and rvalue reference by reference
///     template<class... Ts>
///     constexpr auto capture_forward(Ts&&... xs);
/// 
///     // Capture lvalues by reference and rvalues by value.
///     template<class... Ts>
///     constexpr auto capture_basic(Ts&&... xs);
/// 
/// Semantics
/// ---------
/// 
///     assert(capture(xs...)(f)(ys...) == f(xs..., ys...));
/// 
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
///         auto add_one = boost::hof::capture(1)(sum_f());
///         assert(add_one(2) == 3);
///     }
/// 

namespace boost { namespace hof {

namespace detail {

template<class F, class Pack>
struct capture_invoke : detail::compressed_pair<detail::callable_base<F>, Pack>, detail::function_result_type<F>
{
    typedef capture_invoke fit_rewritable1_tag;
    typedef detail::compressed_pair<detail::callable_base<F>, Pack> base;
    BOOST_HOF_INHERIT_CONSTRUCTOR(capture_invoke, base)
    template<class... Ts>
    constexpr const detail::callable_base<F>& base_function(Ts&&... xs) const noexcept
    {
        return this->first(xs...);
    }

    template<class... Ts>
    constexpr const Pack& get_pack(Ts&&...xs) const noexcept
    {
        return this->second(xs...);
    }

    template<class Failure, class... Ts>
    struct unpack_capture_failure
    {
        template<class... Us>
        struct apply
        {
            typedef typename Failure::template of<Us..., Ts...> type;
        };
    };

    struct capture_failure
    {
        template<class Failure>
        struct apply
        {
            template<class... Ts>
            struct of
            : Pack::template apply<unpack_capture_failure<Failure, Ts...>>::type
            {};
        };
    };

    struct failure
    : failure_map<capture_failure, detail::callable_base<F>>
    {};

    BOOST_HOF_RETURNS_CLASS(capture_invoke);

    template<class... Ts>
    constexpr BOOST_HOF_SFINAE_RESULT
    (
        typename result_of<decltype(boost::hof::pack_join), 
            id_<const Pack&>, 
            result_of<decltype(boost::hof::pack_forward), id_<Ts>...> 
        >::type,
        id_<detail::callable_base<F>&&>
    ) 
    operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        boost::hof::pack_join
        (
            BOOST_HOF_MANGLE_CAST(const Pack&)(BOOST_HOF_CONST_THIS->get_pack(xs...)), 
            boost::hof::pack_forward(BOOST_HOF_FORWARD(Ts)(xs)...)
        )
        (BOOST_HOF_RETURNS_C_CAST(detail::callable_base<F>&&)(BOOST_HOF_CONST_THIS->base_function(xs...)))
    );
};

template<class Pack>
struct capture_pack : Pack
{
    BOOST_HOF_INHERIT_CONSTRUCTOR(capture_pack, Pack);

    BOOST_HOF_RETURNS_CLASS(capture_pack);

    // TODO: Should use rvalue ref qualifier
    template<class F>
    constexpr auto operator()(F f) const BOOST_HOF_SFINAE_RETURNS
    (
        capture_invoke<F, Pack>(BOOST_HOF_RETURNS_STATIC_CAST(F&&)(f), 
            BOOST_HOF_RETURNS_C_CAST(Pack&&)(
                BOOST_HOF_RETURNS_STATIC_CAST(const Pack&)(*boost::hof::always(BOOST_HOF_CONST_THIS)(f))
            )
        )
    );
};

struct make_capture_pack_f
{
    template<class Pack>
    constexpr capture_pack<Pack> operator()(Pack p) const
    BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(capture_pack<Pack>, Pack&&)
    {
        return capture_pack<Pack>(static_cast<Pack&&>(p));
    }
};

template<class F>
struct capture_f
{
    template<class... Ts>
    constexpr auto operator()(Ts&&... xs) const BOOST_HOF_RETURNS
    (
        BOOST_HOF_RETURNS_CONSTRUCT(make_capture_pack_f)()(BOOST_HOF_RETURNS_CONSTRUCT(F)()(BOOST_HOF_FORWARD(Ts)(xs)...))
    );
};
}

BOOST_HOF_DECLARE_STATIC_VAR(capture_basic, detail::capture_f<detail::pack_basic_f>);
BOOST_HOF_DECLARE_STATIC_VAR(capture_forward, detail::capture_f<detail::pack_forward_f>);
BOOST_HOF_DECLARE_STATIC_VAR(capture, detail::capture_f<detail::pack_f>);

}} // namespace boost::hof

#endif

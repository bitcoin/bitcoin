/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    callable_base.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_CALLABLE_BASE_H
#define BOOST_HOF_GUARD_CALLABLE_BASE_H

#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/result_of.hpp>
#include <boost/hof/apply.hpp>

#ifndef BOOST_HOF_CALLABLE_BASE_USE_TEMPLATE_ALIAS
#if (defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7)
#define BOOST_HOF_CALLABLE_BASE_USE_TEMPLATE_ALIAS 0
#else
#define BOOST_HOF_CALLABLE_BASE_USE_TEMPLATE_ALIAS 1
#endif
#endif

namespace boost { namespace hof { namespace detail {

template<class F>
struct non_class_function
{
    F f;
    BOOST_HOF_DELEGATE_CONSTRUCTOR(non_class_function, F, f)

    template<class... Ts>
    constexpr BOOST_HOF_SFINAE_RESULT(apply_f, id_<F>, id_<Ts>...) 
    operator()(Ts&&... xs) const BOOST_HOF_SFINAE_RETURNS
    (
        boost::hof::apply(f, BOOST_HOF_FORWARD(Ts)(xs)...)
    );
};

template<class F>
struct callable_base_type
: std::conditional<(BOOST_HOF_IS_CLASS(F) && !BOOST_HOF_IS_FINAL(F) && !BOOST_HOF_IS_POLYMORPHIC(F)), F, non_class_function<F>>
{};

#if BOOST_HOF_CALLABLE_BASE_USE_TEMPLATE_ALIAS
template<class F>
using callable_base = typename callable_base_type<F>::type;
#else
template<class F>
struct callable_base
: callable_base_type<F>::type
{
    typedef typename callable_base_type<F>::type base;
    BOOST_HOF_INHERIT_CONSTRUCTOR(callable_base, base)
};

template<class F>
struct callable_base_type<callable_base<F>>
: callable_base_type<F>
{};

#endif

}}} // namespace boost::hof

#endif

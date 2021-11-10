/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    always.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_ALWAYS_H
#define BOOST_HOF_GUARD_FUNCTION_ALWAYS_H

#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/unwrap.hpp>
#include <boost/hof/detail/static_const_var.hpp>

/// always
/// ======
/// 
/// Description
/// -----------
/// 
/// The `always` function returns a function object that will always return
/// the value given to it, no matter what parameters are passed to the
/// function object. The nullary version(i.e. `always(void)`) will return
/// `void`. On compilers, that don't support constexpr functions returning
/// `void`, a private empty type is returned instead. This return type is
/// specified as `BOOST_HOF_ALWAYS_VOID_RETURN`.
/// 
/// Synopsis
/// --------
/// 
///     template<class T>
///     constexpr auto always(T value);
/// 
///     template<class T>
///     constexpr auto always(void);
/// 
/// 
/// Semantics
/// ---------
/// 
///     assert(always(x)(xs...) == x);
/// 
/// Requirements
/// ------------
/// 
/// T must be:
/// 
/// * CopyConstructible
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <algorithm>
///     #include <cassert>
///     using namespace boost::hof;
/// 
///     int main() {
///         int ten = 10;
///         assert( always(ten)(1,2,3,4,5) == 10 );
///     }
/// 
///     // Count all
///     template<class Iterator, class T>
///     auto count(Iterator first, Iterator last)
///     {
///         return std::count_if(first, last, always(true));
///     }
/// 


#ifndef BOOST_HOF_NO_CONSTEXPR_VOID
#if defined(__clang__) && BOOST_HOF_HAS_RELAXED_CONSTEXPR
#define BOOST_HOF_NO_CONSTEXPR_VOID 0
#else
#define BOOST_HOF_NO_CONSTEXPR_VOID 1
#endif
#endif

namespace boost { namespace hof { namespace always_detail {

template<class T, class=void>
struct always_base
{
    T x;

    BOOST_HOF_DELEGATE_CONSTRUCTOR(always_base, T, x)

    typedef typename detail::unwrap_reference<T>::type result_type;

    template<class... As>
    constexpr result_type
    operator()(As&&...) const
    noexcept(std::is_reference<result_type>::value || BOOST_HOF_IS_NOTHROW_COPY_CONSTRUCTIBLE(result_type))
    {
        return this->x;
    }
};

template<class T>
struct always_base<T, typename std::enable_if<!BOOST_HOF_IS_EMPTY(T)>::type>
{
    T x;

    constexpr always_base(T xp) noexcept(BOOST_HOF_IS_NOTHROW_COPY_CONSTRUCTIBLE(T))
    : x(xp)
    {}

    typedef typename detail::unwrap_reference<T>::type result_type;

    template<class... As>
    constexpr result_type 
    operator()(As&&...) const 
    noexcept(std::is_reference<result_type>::value || BOOST_HOF_IS_NOTHROW_COPY_CONSTRUCTIBLE(result_type))
    {
        return this->x;
    }
};

#if BOOST_HOF_NO_CONSTEXPR_VOID
#define BOOST_HOF_ALWAYS_VOID_RETURN boost::hof::always_detail::always_base<void>::void_
#else
#define BOOST_HOF_ALWAYS_VOID_RETURN void
#endif

template<>
struct always_base<void>
{
    
    constexpr always_base() noexcept
    {}

    struct void_ {};

    template<class... As>
    constexpr BOOST_HOF_ALWAYS_VOID_RETURN 
    operator()(As&&...) const noexcept
    {
#if BOOST_HOF_NO_CONSTEXPR_VOID
        return void_();
#endif
    }
};

struct always_f
{
    template<class T>
    constexpr always_detail::always_base<T> operator()(T x) const noexcept(BOOST_HOF_IS_NOTHROW_COPY_CONSTRUCTIBLE(T))
    {
        return always_detail::always_base<T>(x);
    }

    constexpr always_detail::always_base<void> operator()() const noexcept
    {
        return always_detail::always_base<void>();
    }
};

struct always_ref_f
{
    template<class T>
    constexpr always_detail::always_base<T&> operator()(T& x) const noexcept
    {
        return always_detail::always_base<T&>(x);
    }
};

}
BOOST_HOF_DECLARE_STATIC_VAR(always, always_detail::always_f);
BOOST_HOF_DECLARE_STATIC_VAR(always_ref, always_detail::always_ref_f);

}} // namespace boost::hof

#endif

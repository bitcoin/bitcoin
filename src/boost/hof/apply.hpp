/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    apply.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_APPLY_H
#define BOOST_HOF_GUARD_APPLY_H

/// apply
/// =====
/// 
/// Description
/// -----------
/// 
/// The `apply` function calls the function given to it with its arguments.
/// 
/// Synopsis
/// --------
/// 
///     template<class F, class... Ts>
///     constexpr auto apply(F&& f, Ts&&... xs);
/// 
/// Semantics
/// ---------
/// 
///     assert(apply(f)(xs...) == f(xs...));
///     assert(fold(apply, f)(x, y, z) == f(x)(y)(z));
/// 
/// Requirements
/// ------------
/// 
/// F must be:
/// 
/// * [Invocable](Invocable)
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
///         assert(boost::hof::apply(sum_f(), 1, 2) == 3);
///     }
/// 

#include <boost/hof/detail/result_of.hpp>
#include <boost/hof/detail/forward.hpp>
#include <boost/hof/detail/static_const_var.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4003)
#endif

#define BOOST_HOF_DETAIL_FOREACH_QUAL(m, data) \
    m(, data) \
    m(const, data) \
    m(volatile, data) \
    m(const volatile, data)

namespace boost { namespace hof {

namespace detail {
#if BOOST_HOF_HAS_MANUAL_DEDUCTION || BOOST_HOF_NO_EXPRESSION_SFINAE
struct apply_mem_fn
{
    template<class...>
    struct convertible_args;

    template<class T, class U, class=void>
    struct is_convertible_args
    : std::false_type
    {};

    template<class... Ts, class... Us>
    struct is_convertible_args<
        convertible_args<Ts...>, 
        convertible_args<Us...>, 
        typename std::enable_if<(
            sizeof...(Ts) == sizeof...(Us)
        )>::type
    >
    : and_<std::is_convertible<Ts, Us>...>
    {};

    template<class From, class To>
    struct is_compatible
    : std::is_convertible<
        typename std::add_pointer<typename std::remove_reference<From>::type>::type,
        typename std::add_pointer<typename std::remove_reference<To>::type>::type
    >
    {};

#define BOOST_HOF_APPLY_MEM_FN_CALL(cv, data) \
    template <class R, class Base, class Derived, class... Ts, class... Us, class=typename std::enable_if<and_< \
        is_compatible<Derived, cv Base>, \
        is_convertible_args<convertible_args<Us...>, convertible_args<Ts...>> \
    >::value>::type> \
    constexpr R operator()(R (Base::*mf)(Ts...) cv, Derived&& ref, Us &&... xs) const \
    BOOST_HOF_RETURNS_DEDUCE_NOEXCEPT((BOOST_HOF_FORWARD(Derived)(ref).*mf)(BOOST_HOF_FORWARD(Us)(xs)...)) \
    { \
        return (BOOST_HOF_FORWARD(Derived)(ref).*mf)(BOOST_HOF_FORWARD(Us)(xs)...); \
    }
    BOOST_HOF_DETAIL_FOREACH_QUAL(BOOST_HOF_APPLY_MEM_FN_CALL, ~)
};

struct apply_mem_data
{
    template<class T, class R>
    struct match_qualifier
    { typedef R type; };

#define BOOST_HOF_APPLY_MEM_DATA_MATCH(cv, ref) \
    template<class T, class R> \
    struct match_qualifier<cv T ref, R> \
    : match_qualifier<T, cv R ref> \
    {};

    BOOST_HOF_DETAIL_FOREACH_QUAL(BOOST_HOF_APPLY_MEM_DATA_MATCH,&)
    BOOST_HOF_DETAIL_FOREACH_QUAL(BOOST_HOF_APPLY_MEM_DATA_MATCH,&&)

    template <class Base, class R, class Derived, class=typename std::enable_if<(
        std::is_base_of<Base, typename std::decay<Derived>::type>::value
    )>::type>
    constexpr typename match_qualifier<Derived, R>::type 
    operator()(R Base::*pmd, Derived&& ref) const noexcept
    {
        return BOOST_HOF_FORWARD(Derived)(ref).*pmd;
    }
};

template<class T, class U=decltype(*std::declval<T>())>
struct apply_deref
{ typedef U type; };

#endif

struct apply_f
{
#if BOOST_HOF_HAS_MANUAL_DEDUCTION || BOOST_HOF_NO_EXPRESSION_SFINAE
    template<class F, class T, class... Ts, class=typename std::enable_if<(
        std::is_member_function_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_fn, id_<F>, id_<T>, id_<Ts>...) 
    operator()(F&& f, T&& obj, Ts&&... xs) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_fn()(f, BOOST_HOF_FORWARD(T)(obj), BOOST_HOF_FORWARD(Ts)(xs)...)
    );

    template<class F, class T, class... Ts, class U=typename apply_deref<T>::type, class=typename std::enable_if<(
        std::is_member_function_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_fn, id_<F>, id_<U>, id_<Ts>...) 
    operator()(F&& f, T&& obj, Ts&&... xs) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_fn()(f, *BOOST_HOF_FORWARD(T)(obj), BOOST_HOF_FORWARD(Ts)(xs)...)
    );

    template<class F, class T, class... Ts, class=typename std::enable_if<(
        std::is_member_function_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_fn, id_<F>, id_<T&>, id_<Ts>...) 
    operator()(F&& f, const std::reference_wrapper<T>& ref, Ts&&... xs) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_fn()(f, ref.get(), BOOST_HOF_FORWARD(Ts)(xs)...)
    );

    template<class F, class T, class=typename std::enable_if<(
        std::is_member_object_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_data, id_<F>, id_<T>) 
    operator()(F&& f, T&& obj) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_data()(f, BOOST_HOF_FORWARD(T)(obj))
    );

    template<class F, class T, class U=typename apply_deref<T>::type, class=typename std::enable_if<(
        std::is_member_object_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_data, id_<F>, id_<U>) 
    operator()(F&& f, T&& obj) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_data()(f, *BOOST_HOF_FORWARD(T)(obj))
    );
    
    template<class F, class T, class=typename std::enable_if<(
        std::is_member_object_pointer<typename std::decay<F>::type>::value
    )>::type>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(apply_mem_data, id_<F>, id_<T&>) 
    operator()(F&& f, const std::reference_wrapper<T>& ref) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        apply_mem_data()(f, ref.get())
    );

#else

    template <class Base, class T, class Derived>
    constexpr auto operator()(T Base::*pmd, Derived&& ref) const
    BOOST_HOF_RETURNS(BOOST_HOF_FORWARD(Derived)(ref).*pmd);
     
    template <class PMD, class Pointer>
    constexpr auto operator()(PMD&& pmd, Pointer&& ptr) const
    BOOST_HOF_RETURNS((*BOOST_HOF_FORWARD(Pointer)(ptr)).*BOOST_HOF_FORWARD(PMD)(pmd));

    template <class Base, class T, class Derived>
    constexpr auto operator()(T Base::*pmd, const std::reference_wrapper<Derived>& ref) const
    BOOST_HOF_RETURNS(ref.get().*pmd);
     
    template <class Base, class T, class Derived, class... Args>
    constexpr auto operator()(T Base::*pmf, Derived&& ref, Args&&... args) const
    BOOST_HOF_RETURNS((BOOST_HOF_FORWARD(Derived)(ref).*pmf)(BOOST_HOF_FORWARD(Args)(args)...));
     
    template <class PMF, class Pointer, class... Args>
    constexpr auto operator()(PMF&& pmf, Pointer&& ptr, Args&&... args) const
    BOOST_HOF_RETURNS(((*BOOST_HOF_FORWARD(Pointer)(ptr)).*BOOST_HOF_FORWARD(PMF)(pmf))(BOOST_HOF_FORWARD(Args)(args)...));

    template <class Base, class T, class Derived, class... Args>
    constexpr auto operator()(T Base::*pmf, const std::reference_wrapper<Derived>& ref, Args&&... args) const
    BOOST_HOF_RETURNS((ref.get().*pmf)(BOOST_HOF_FORWARD(Args)(args)...));

#endif
    template<class F, class... Ts>
    constexpr BOOST_HOF_SFINAE_MANUAL_RESULT(F, id_<Ts>...) 
    operator()(F&& f, Ts&&... xs) const BOOST_HOF_SFINAE_MANUAL_RETURNS
    (
        f(BOOST_HOF_FORWARD(Ts)(xs)...)
    );
};

}

BOOST_HOF_DECLARE_STATIC_VAR(apply, detail::apply_f);

}} // namespace boost::hof

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif


//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, 
//  Aleksey Gurtovoy, Howard Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#if !defined(BOOST_PP_IS_ITERATING)

///// header body

#ifndef BOOST_TT_DETAIL_IS_MEM_FUN_POINTER_IMPL_HPP_INCLUDED
#define BOOST_TT_DETAIL_IS_MEM_FUN_POINTER_IMPL_HPP_INCLUDED

#include <boost/config.hpp>

#if defined(BOOST_TT_PREPROCESSING_MODE)
//
// Maintenance mode, hide include dependencies
// from trackers:
//
#define PPI <boost/preprocessor/iterate.hpp>
#include PPI
#undef PPI
#define PPI <boost/preprocessor/enum_params.hpp>
#include PPI
#undef PPI
#define PPI <boost/preprocessor/comma_if.hpp>
#include PPI
#undef PPI
#endif

namespace boost {
namespace type_traits {

template <typename T>
struct is_mem_fun_pointer_impl
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

#if !defined(BOOST_TT_PREPROCESSING_MODE)
// pre-processed code, don't edit, try GNU cpp with 
// cpp -I../../../ -DBOOST_TT_PREPROCESSING_MODE -x c++ -P filename

template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)()> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)()noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)() const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T >
struct is_mem_fun_pointer_impl<R(T::*)(...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0>
struct is_mem_fun_pointer_impl<R(T::*)(T0 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#if __cpp_noexcept_function_type
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
template <class R, class T, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_mem_fun_pointer_impl<R(T::*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#endif

#else

#undef BOOST_STATIC_CONSTANT
#define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, 25, "boost/type_traits/detail/is_mem_fun_pointer_impl.hpp"))
#include BOOST_PP_ITERATE()

#endif // BOOST_TT_PREPROCESSING_MODE

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_IS_MEM_FUN_POINTER_IMPL_HPP_INCLUDED

///// iteration

#else
#define BOOST_PP_COUNTER BOOST_PP_FRAME_ITERATION(1)

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T))> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif

@#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)) const> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };

@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...) const> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...) volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_mem_fun_pointer_impl<R (T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...) const volatile> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif
@#endif

@#if __cpp_noexcept_function_type

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T))noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T) ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif

@#if !defined(BOOST_TT_NO_CV_FUNC_TEST)
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T)) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T)) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T)) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };

@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T) ...) const noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T) ...) volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };

template <class R, class T BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_mem_fun_pointer_impl<R(T::*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T) ...) const volatile noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif
@#endif

@#endif

#undef BOOST_PP_COUNTER
#endif // BOOST_PP_IS_ITERATING


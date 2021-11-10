
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, 
//  Aleksey Gurtovoy, Howard Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#if !defined(BOOST_PP_IS_ITERATING)

///// header body

#ifndef BOOST_TT_DETAIL_IS_FUNCTION_PTR_TESTER_HPP_INCLUDED
#define BOOST_TT_DETAIL_IS_FUNCTION_PTR_TESTER_HPP_INCLUDED

#include <boost/type_traits/detail/yes_no_type.hpp>

#if defined(BOOST_TT_PREPROCESSING_MODE)
//
// Hide include dependencies from analysers since they're
// only require in maintenance mode:
//
#define PP1 <boost/preprocessor/iterate.hpp>
#define PP2 <boost/preprocessor/enum_params.hpp>
#define PP3 <boost/preprocessor/comma_if.hpp>
#include PP1
#include PP2
#include PP3
#undef PP1
#undef PP2
#undef PP3
#endif

namespace boost {
namespace type_traits {

// Note it is acceptable to use ellipsis here, since the argument will
// always be a pointer type of some sort (JM 2005/06/04):
no_type BOOST_TT_DECL is_function_ptr_tester(...);

#if !defined(BOOST_TT_PREPROCESSING_MODE)
// pre-processed code, don't edit, try GNU cpp with 
// cpp -I../../../ -DBOOST_TT_PREPROCESSING_MODE -x c++ -P filename

template <class R >
yes_type is_function_ptr_tester(R(*)());
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R >
yes_type is_function_ptr_tester(R(*)(...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R >
yes_type is_function_ptr_tester(R(__stdcall*)());
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R >
yes_type is_function_ptr_tester(R(__vectorcall*)());
#endif
#ifndef _MANAGED
template <class R >
yes_type is_function_ptr_tester(R(__fastcall*)());
#endif
template <class R >
yes_type is_function_ptr_tester(R(__cdecl*)());
#endif
template <class R, class T0 >
yes_type is_function_ptr_tester(R(*)(T0));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0 >
yes_type is_function_ptr_tester(R(*)(T0 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0));
#endif
#ifndef _MANAGED
template <class R, class T0 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0));
#endif
template <class R, class T0 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0));
#endif
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(*)(T0, T1));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(*)(T0, T1 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1));
#endif
template <class R, class T0, class T1 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1));
#endif
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2));
#endif
template <class R, class T0, class T1, class T2 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2));
#endif
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3));
#endif
template <class R, class T0, class T1, class T2, class T3 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...));
#endif
#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(__vectorcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
#endif
#ifndef _MANAGED
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
yes_type is_function_ptr_tester(R(__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
#endif
#else

#define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, 25, "boost/type_traits/detail/is_function_ptr_tester.hpp"))
#include BOOST_PP_ITERATE()

#endif // BOOST_TT_PREPROCESSING_MODE

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_IS_FUNCTION_PTR_TESTER_HPP_INCLUDED

///// iteration

#else
#define BOOST_PP_COUNTER BOOST_PP_FRAME_ITERATION(1)
#undef __stdcall
#undef __fastcall
#undef __cdecl

template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T) >
yes_type is_function_ptr_tester(R (*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)));
@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T) >
yes_type is_function_ptr_tester(R (*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...));
@#endif
@#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T) >
yes_type is_function_ptr_tester(R (__stdcall*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)));
@#if (_MSC_VER >= 1800) && !defined(_MANAGED) && (defined(_M_IX86_FP) && (_M_IX86_FP >= 2) || defined(_M_X64))
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T) >
yes_type is_function_ptr_tester(R(__vectorcall*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T)));
@#endif
@#ifndef _MANAGED
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T) >
yes_type is_function_ptr_tester(R (__fastcall*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)));
@#endif
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T) >
yes_type is_function_ptr_tester(R (__cdecl*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T)));
@#endif

#undef BOOST_PP_COUNTER
#endif // BOOST_PP_IS_ITERATING

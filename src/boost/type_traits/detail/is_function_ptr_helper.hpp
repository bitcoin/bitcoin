
//  Copyright 2000 John Maddock (john@johnmaddock.co.uk)
//  Copyright 2002 Aleksey Gurtovoy (agurtovoy@meta-comm.com)
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#if !defined(BOOST_PP_IS_ITERATING)

///// header body

#ifndef BOOST_TT_DETAIL_IS_FUNCTION_PTR_HELPER_HPP_INCLUDED
#define BOOST_TT_DETAIL_IS_FUNCTION_PTR_HELPER_HPP_INCLUDED

#if defined(BOOST_TT_PREPROCESSING_MODE)
//
// Hide these #include from dependency analysers as
// these are required in maintenance mode only:
//
#define PP1 <boost/preprocessor/iterate.hpp>
#include PP1
#undef PP1
#define PP1 <boost/preprocessor/enum_params.hpp>
#include PP1
#undef PP1
#define PP1 <boost/preprocessor/comma_if.hpp>
#include PP1
#undef PP1
#endif

namespace boost {
namespace type_traits {

template <class R>
struct is_function_ptr_helper
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

#if !defined(BOOST_TT_PREPROCESSING_MODE)
// preprocessor-generated part, don't edit by hand!

template <class R >
struct is_function_ptr_helper<R(*)()> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R >
struct is_function_ptr_helper<R(*)(...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R >
struct is_function_ptr_helper<R(*)()noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R >
struct is_function_ptr_helper<R(*)(...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0>
struct is_function_ptr_helper<R(*)(T0)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0>
struct is_function_ptr_helper<R(*)(T0 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0>
struct is_function_ptr_helper<R(*)(T0)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0>
struct is_function_ptr_helper<R(*)(T0 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1>
struct is_function_ptr_helper<R(*)(T0, T1)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1>
struct is_function_ptr_helper<R(*)(T0, T1 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1>
struct is_function_ptr_helper<R(*)(T0, T1)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1>
struct is_function_ptr_helper<R(*)(T0, T1 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2>
struct is_function_ptr_helper<R(*)(T0, T1, T2)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2>
struct is_function_ptr_helper<R(*)(T0, T1, T2 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2>
struct is_function_ptr_helper<R(*)(T0, T1, T2)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2>
struct is_function_ptr_helper<R(*)(T0, T1, T2 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#if __cpp_noexcept_function_type
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
struct is_function_ptr_helper<R(*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
#endif
#endif
#else

#undef BOOST_STATIC_CONSTANT
#define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, 25, "boost/type_traits/detail/is_function_ptr_helper.hpp"))
#include BOOST_PP_ITERATE()

#endif // BOOST_TT_PREPROCESSING_MODE

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_IS_FUNCTION_PTR_HELPER_HPP_INCLUDED

///// iteration

#else
#define BOOST_PP_COUNTER BOOST_PP_FRAME_ITERATION(1)

template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_function_ptr_helper<R (*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T))> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,class T)>
struct is_function_ptr_helper<R (*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER,T) ...)> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif
@#if __cpp_noexcept_function_type
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_function_ptr_helper<R(*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T))noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#ifndef BOOST_TT_NO_ELLIPSIS_IN_FUNC_TESTING
template <class R BOOST_PP_COMMA_IF(BOOST_PP_COUNTER) BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, class T)>
struct is_function_ptr_helper<R(*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_COUNTER, T) ...)noexcept> { BOOST_STATIC_CONSTANT(bool, value = true); };
@#endif
@#endif
#undef BOOST_PP_COUNTER
#endif // BOOST_PP_IS_ITERATING

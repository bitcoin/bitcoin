
// NO INCLUDE GUARDS, THE HEADER IS INTENDED FOR MULTIPLE INCLUSION

// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// $Source$
// $Date$
// $Revision$

//
// This header is deprecated and no longer used by type_traits:
//
#if defined(__GNUC__) || defined(_MSC_VER)
# pragma message("NOTE: Use of this header (bool_trait_def.hpp) is deprecated")
#endif

#include <boost/type_traits/detail/template_arity_spec.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/config.hpp>

//
// Unfortunately some libraries have started using this header without
// cleaning up afterwards: so we'd better undef the macros just in case 
// they've been defined already....
//
#ifdef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#undef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#undef BOOST_TT_AUX_BOOL_C_BASE
#undef BOOST_TT_AUX_BOOL_TRAIT_DEF1
#undef BOOST_TT_AUX_BOOL_TRAIT_DEF2
#undef BOOST_TT_AUX_BOOL_TRAIT_SPEC1
#undef BOOST_TT_AUX_BOOL_TRAIT_SPEC2
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC2
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_2
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_1
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_2
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_PARTIAL_SPEC2_1
#undef BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1
#endif

#ifndef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#   define BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) /**/
#endif

#ifndef BOOST_TT_AUX_BOOL_C_BASE
#   define BOOST_TT_AUX_BOOL_C_BASE(C) : public ::boost::integral_constant<bool,C>
#endif 


#define BOOST_TT_AUX_BOOL_TRAIT_DEF1(trait,T,C) \
template< typename T > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(1,trait) \
/**/


#define BOOST_TT_AUX_BOOL_TRAIT_DEF2(trait,T1,T2,C) \
template< typename T1, typename T2 > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(2,trait) \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_DEF3(trait,T1,T2,T3,C) \
template< typename T1, typename T2, typename T3 > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(3,trait) \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,C) \
template<> struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_SPEC2(trait,sp1,sp2,C) \
template<> struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(trait,sp,C) \
template<> struct trait##_impl< sp > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC2(trait,sp1,sp2,C) \
template<> struct trait##_impl< sp1,sp2 > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(param,trait,sp,C) \
template< param > struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_2(param1,param2,trait,sp,C) \
template< param1, param2 > struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_1(param,trait,sp1,sp2,C) \
template< param > struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_2(param1,param2,trait,sp1,sp2,C) \
template< param1, param2 > struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_PARTIAL_SPEC2_1(param,trait,sp1,sp2,C) \
template< param > struct trait##_impl< sp1,sp2 > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#ifndef BOOST_NO_CV_SPECIALIZATIONS
#   define BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp const,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp volatile,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp const volatile,value) \
    /**/
#else
#   define BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,value) \
    /**/
#endif

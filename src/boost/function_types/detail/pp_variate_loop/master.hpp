
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#ifdef __WAVE__
// this file has been generated from the master.hpp file in the same directory
#   pragma wave option(preserve: 0)
#endif

#if !defined(BOOST_FT_PREPROCESSING_MODE)
#   error "this file is only for two-pass preprocessing"
#endif

#if !defined(BOOST_PP_VALUE)
#   include <boost/preprocessor/slot/slot.hpp>
#   include <boost/preprocessor/facilities/empty.hpp>
#   include <boost/preprocessor/facilities/expand.hpp>
#   include <boost/function_types/detail/encoding/def.hpp>

BOOST_PP_EXPAND(#) define BOOST_FT_mfp 0
BOOST_PP_EXPAND(#) define BOOST_FT_syntax BOOST_FT_type_function

#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_non_variadic
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_variadic
#   include __FILE__

BOOST_PP_EXPAND(#) if !BOOST_FT_NO_CV_FUNC_SUPPORT
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_non_variadic|BOOST_FT_const
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_variadic|BOOST_FT_const
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_non_variadic|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_variadic|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_non_variadic|BOOST_FT_const|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_function|BOOST_FT_variadic|BOOST_FT_const|BOOST_FT_volatile
#   include __FILE__
BOOST_PP_EXPAND(#) endif


BOOST_PP_EXPAND(#) undef  BOOST_FT_syntax
BOOST_PP_EXPAND(#) define BOOST_FT_syntax BOOST_FT_type_function_pointer

#   define  BOOST_PP_VALUE \
      BOOST_FT_pointer|BOOST_FT_non_variadic
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_pointer|BOOST_FT_variadic
#   include __FILE__

BOOST_PP_EXPAND(#) undef  BOOST_FT_syntax
BOOST_PP_EXPAND(#) define BOOST_FT_syntax BOOST_FT_type_function_reference

#   define BOOST_PP_VALUE \
      BOOST_FT_reference|BOOST_FT_non_variadic
#   include __FILE__
#   define BOOST_PP_VALUE \
      BOOST_FT_reference|BOOST_FT_variadic
#   include __FILE__

BOOST_PP_EXPAND(#) undef  BOOST_FT_syntax
BOOST_PP_EXPAND(#) undef  BOOST_FT_mfp

BOOST_PP_EXPAND(#) define BOOST_FT_mfp 1
BOOST_PP_EXPAND(#) define BOOST_FT_syntax BOOST_FT_type_member_function_pointer

#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_non_variadic
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_variadic
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_non_variadic|BOOST_FT_const
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_variadic|BOOST_FT_const
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_non_variadic|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_variadic|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_non_variadic|BOOST_FT_const|BOOST_FT_volatile
#   include __FILE__
#   define  BOOST_PP_VALUE \
      BOOST_FT_member_function_pointer|BOOST_FT_variadic|BOOST_FT_const|BOOST_FT_volatile
#   include __FILE__

BOOST_PP_EXPAND(#) undef  BOOST_FT_syntax
BOOST_PP_EXPAND(#) undef  BOOST_FT_mfp

#   include <boost/function_types/detail/encoding/undef.hpp>
#else 

#   include BOOST_PP_ASSIGN_SLOT(1)

#   define  BOOST_PP_VALUE BOOST_PP_SLOT(1) & BOOST_FT_kind_mask
#   include BOOST_PP_ASSIGN_SLOT(2)

BOOST_PP_EXPAND(#) if !!(BOOST_PP_SLOT(2) & (BOOST_FT_variations))
BOOST_PP_EXPAND(#) if (BOOST_PP_SLOT(1) & (BOOST_FT_cond)) == (BOOST_FT_cond)

#   if ( BOOST_PP_SLOT(1) & (BOOST_FT_variadic) )
BOOST_PP_EXPAND(#)   define BOOST_FT_ell ...
BOOST_PP_EXPAND(#)   define BOOST_FT_nullary_param
#   else
BOOST_PP_EXPAND(#)   define BOOST_FT_ell
BOOST_PP_EXPAND(#)   define BOOST_FT_nullary_param BOOST_FT_NULLARY_PARAM
#   endif

#   if !( BOOST_PP_SLOT(1) & (BOOST_FT_volatile) )
#     if !( BOOST_PP_SLOT(1) & (BOOST_FT_const) )
BOOST_PP_EXPAND(#)   define BOOST_FT_cv 
#     else
BOOST_PP_EXPAND(#)   define BOOST_FT_cv const
#     endif
#   else
#     if !( BOOST_PP_SLOT(1) & (BOOST_FT_const) )
BOOST_PP_EXPAND(#)   define BOOST_FT_cv volatile
#     else
BOOST_PP_EXPAND(#)   define BOOST_FT_cv const volatile
#     endif
#   endif
BOOST_PP_EXPAND(#)   define BOOST_FT_flags BOOST_PP_SLOT(1)
BOOST_PP_EXPAND(#)   include BOOST_FT_variate_file

BOOST_PP_EXPAND(#)   undef BOOST_FT_cv
BOOST_PP_EXPAND(#)   undef BOOST_FT_ell
BOOST_PP_EXPAND(#)   undef BOOST_FT_nullary_param
BOOST_PP_EXPAND(#)   undef BOOST_FT_flags
BOOST_PP_EXPAND(#) endif
BOOST_PP_EXPAND(#) endif
#endif


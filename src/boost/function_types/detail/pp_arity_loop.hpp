
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is intended for multiple inclusion

#ifndef BOOST_FT_PREPROCESSING_MODE
// input:  BOOST_FT_mfp        0 or 1 <=> member function pointer?
// input:  BOOST_FT_type_name  BOOST_FT_type --> "R (* ..._type_name)()" (pass2)
#endif
// input:  BOOST_FT_syntax     type macro to use
// input:  BOOST_FT_cc         empty or cc specifier 
// input:  BOOST_FT_ell        empty or "..."
// input:  BOOST_FT_cv         empty or cv qualifiers
// input:  BOOST_FT_flags      single decimal integer encoding the flags
// output: BOOST_FT_n          number of component types (arity+1)
// output: BOOST_FT_arity      current arity
// output: BOOST_FT_type       macro that expands to the type
// output: BOOST_FT_tplargs(p) template arguments with given prefix
// output: BOOST_FT_params(p)  parameters with given prefix

#ifdef __WAVE__
#   pragma wave option(preserve: 0)
#endif

#ifndef BOOST_FT_ARITY_LOOP_IS_ITERATING

#   define BOOST_FT_AL_PREPROCESSED \
        BOOST_FT_AL_FILE(BOOST_FT_al_path,BOOST_FT_FROM_ARITY,BOOST_FT_mfp)

#   define BOOST_FT_AL_FILE(base_path,max_arity,mfp) \
        BOOST_FT_AL_FILE_I(base_path,max_arity,mfp)
#   define BOOST_FT_AL_FILE_I(base_path,max_arity,mfp) \
        <base_path/arity ## max_arity ## _ ## mfp.hpp>

#   if !defined(BOOST_FT_PREPROCESSING_MODE)

#     if BOOST_FT_MAX_ARITY < 10
#       define BOOST_FT_FROM_ARITY 0 
#     elif BOOST_FT_MAX_ARITY < 20
#       define BOOST_FT_FROM_ARITY 10
#     elif BOOST_FT_MAX_ARITY < 30
#       define BOOST_FT_FROM_ARITY 20
#     elif BOOST_FT_MAX_ARITY < 40
#       define BOOST_FT_FROM_ARITY 30
#     endif

#     if BOOST_FT_FROM_ARITY
#       include BOOST_FT_AL_PREPROCESSED
#     endif

#   elif !defined(BOOST_FT_FROM_ARITY) // single pass preprocessing
#     define BOOST_FT_FROM_ARITY 0

#   elif BOOST_FT_FROM_ARITY > 0       // arity20 includes arity10
BOOST_PP_EXPAND(#) include BOOST_FT_AL_PREPROCESSED
#   endif

#   undef BOOST_FT_AL_PREPROCESSED

#   undef BOOST_FT_AL_FILE
#   undef BOOST_FT_AL_FILE_I

#   if BOOST_FT_MAX_ARITY > BOOST_FT_FROM_ARITY

#     ifndef BOOST_FT_DETAIL_ARITY_LOOP_HPP_INCLUDED
#     define BOOST_FT_DETAIL_ARITY_LOOP_HPP_INCLUDED
#         include <boost/preprocessor/cat.hpp>
#         include <boost/preprocessor/tuple/eat.hpp>
#         include <boost/preprocessor/control/if.hpp>
#         include <boost/preprocessor/arithmetic/inc.hpp>
#         include <boost/preprocessor/facilities/empty.hpp>
#         include <boost/preprocessor/facilities/expand.hpp>
#         include <boost/preprocessor/iteration/iterate.hpp>
#         include <boost/preprocessor/repetition/enum_params.hpp>
#         include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#         include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#     endif

#     define BOOST_FT_AL_INCLUDE_FILE <BOOST_FT_al_path/master.hpp>

#     define BOOST_FT_ARITY_LOOP_PREFIX 1
#     include BOOST_FT_AL_INCLUDE_FILE
#     undef  BOOST_FT_ARITY_LOOP_PREFIX

#     if !BOOST_PP_IS_ITERATING
#       define BOOST_PP_FILENAME_1 BOOST_FT_AL_INCLUDE_FILE
#     elif BOOST_PP_ITERATION_DEPTH() == 1
#       define BOOST_PP_FILENAME_2 BOOST_FT_AL_INCLUDE_FILE
#     else
#       error "loops nested too deeply"
#     endif

#     define BOOST_FT_arity BOOST_PP_ITERATION()
#     define BOOST_FT_n     BOOST_PP_INC(BOOST_FT_arity)

#     define BOOST_FT_type \
          BOOST_FT_syntax(BOOST_FT_cc,BOOST_FT_type_name BOOST_PP_EMPTY)\
               (BOOST_FT_params(BOOST_PP_EMPTY) BOOST_FT_ell) BOOST_FT_cv

#     define BOOST_FT_tplargs(prefx) \
          prefx() R BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_FT_arity,prefx() T)

#     if !BOOST_FT_mfp

#       define BOOST_FT_params(prefx) \
            BOOST_PP_IF(BOOST_FT_arity,BOOST_PP_ENUM_PARAMS, \
                BOOST_FT_nullary_param BOOST_PP_TUPLE_EAT(2))( \
                    BOOST_FT_arity,prefx() T) 
#     else

#       define BOOST_FT_params(prefx) \
            BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_FT_arity,prefx() T)

#     endif

#     if !BOOST_FT_FROM_ARITY 
#       define BOOST_PP_ITERATION_LIMITS (BOOST_FT_mfp, BOOST_FT_MAX_ARITY)
#     else
#       define BOOST_PP_ITERATION_LIMITS \
            (BOOST_FT_FROM_ARITY+1, BOOST_FT_MAX_ARITY)
#     endif

#     define BOOST_FT_ARITY_LOOP_IS_ITERATING 1
#     include BOOST_PP_ITERATE()
#     undef  BOOST_FT_ARITY_LOOP_IS_ITERATING

#     undef BOOST_FT_arity
#     undef BOOST_FT_params
#     undef BOOST_FT_tplargs
#     undef BOOST_FT_type

#     define BOOST_FT_ARITY_LOOP_SUFFIX 1
#     include BOOST_FT_AL_INCLUDE_FILE
#     undef  BOOST_FT_ARITY_LOOP_SUFFIX

#     undef BOOST_FT_AL_INCLUDE_FILE
#   endif

#   undef BOOST_FT_FROM_ARITY

#else
#   error "attempt to nest arity loops"
#endif


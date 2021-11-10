/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_VARTEMPL_SUPPORT_HPP
#define BOOST_MULTI_INDEX_DETAIL_VARTEMPL_SUPPORT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

/* Utilities for emulation of variadic template functions. Variadic packs are
 * replaced by lists of BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS parameters:
 *
 *   - typename... Args            --> BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK
 *   - Args&&... args              --> BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK
 *   - std::forward<Args>(args)... --> BOOST_MULTI_INDEX_FORWARD_PARAM_PACK
 *
 * Forwarding emulated with Boost.Move. A template functions foo_imp
 * defined in such way accepts *exactly* BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS
 * arguments: variable number of arguments is emulated by providing a set of
 * overloads foo forwarding to foo_impl with
 * 
 *   BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL
 *   BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG (initial extra arg)
 *
 * which fill the extra args with boost::multi_index::detail::noarg's.
 * boost::multi_index::detail::vartempl_placement_new works the opposite
 * way: it acceps a full a pointer x to Value and a
 * BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK and forwards to
 * new(x) Value(args) where args is the argument pack after discarding
 * noarg's.
 *
 * Emulation decays to the real thing when the compiler supports variadic
 * templates and move semantics natively.
 */

#include <boost/config.hpp>

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)||\
    defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/logical/and.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/elem.hpp>

#if !defined(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS)
#define BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS 5
#endif

#define BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK                        \
BOOST_PP_ENUM_PARAMS(                                                \
  BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,typename T)

#define BOOST_MULTI_INDEX_VARTEMPL_ARG(z,n,_)                        \
BOOST_FWD_REF(BOOST_PP_CAT(T,n)) BOOST_PP_CAT(t,n)

#define BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK                        \
BOOST_PP_ENUM(                                                       \
  BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,                             \
  BOOST_MULTI_INDEX_VARTEMPL_ARG,~)

#define BOOST_MULTI_INDEX_VARTEMPL_FORWARD_ARG(z,n,_)                \
boost::forward<BOOST_PP_CAT(T,n)>(BOOST_PP_CAT(t,n))

#define BOOST_MULTI_INDEX_FORWARD_PARAM_PACK                         \
BOOST_PP_ENUM(                                                       \
  BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,                             \
  BOOST_MULTI_INDEX_VARTEMPL_FORWARD_ARG,~)

namespace boost{namespace multi_index{namespace detail{
struct noarg{};
}}}

/* call vartempl function without args */

#define BOOST_MULTI_INDEX_NULL_PARAM_PACK                            \
BOOST_PP_ENUM_PARAMS(                                                \
  BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,                             \
  boost::multi_index::detail::noarg() BOOST_PP_INTERCEPT)

#define BOOST_MULTI_INDEX_TEMPLATE_N(n)                              \
template<BOOST_PP_ENUM_PARAMS(n,typename T)>

#define BOOST_MULTI_INDEX_TEMPLATE_0(n)

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_AUX(z,n,data)        \
BOOST_PP_IF(n,                                                       \
  BOOST_MULTI_INDEX_TEMPLATE_N,                                      \
  BOOST_MULTI_INDEX_TEMPLATE_0)(n)                                   \
BOOST_PP_SEQ_ELEM(0,data) /* ret */                                  \
BOOST_PP_SEQ_ELEM(1,data) /* name_from */ (                          \
  BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_ARG,~))                 \
{                                                                    \
  return BOOST_PP_SEQ_ELEM(2,data) /* name_to */ (                   \
    BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_FORWARD_ARG,~)        \
    BOOST_PP_COMMA_IF(                                               \
      BOOST_PP_AND(                                                  \
        n,BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n)))    \
    BOOST_PP_ENUM_PARAMS(                                            \
      BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n),         \
      boost::multi_index::detail::noarg() BOOST_PP_INTERCEPT)        \
  );                                                                 \
}

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL(                     \
  ret,name_from,name_to)                                             \
BOOST_PP_REPEAT_FROM_TO(                                             \
  0,BOOST_PP_ADD(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,1),           \
  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_AUX,                       \
  (ret)(name_from)(name_to))

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG_AUX(       \
  z,n,data)                                                          \
BOOST_PP_IF(n,                                                       \
  BOOST_MULTI_INDEX_TEMPLATE_N,                                      \
  BOOST_MULTI_INDEX_TEMPLATE_0)(n)                                   \
BOOST_PP_SEQ_ELEM(0,data) /* ret */                                  \
BOOST_PP_SEQ_ELEM(1,data) /* name_from */ (                          \
  BOOST_PP_SEQ_ELEM(3,data) BOOST_PP_SEQ_ELEM(4,data) /* extra arg */\
  BOOST_PP_COMMA_IF(n)                                               \
  BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_ARG,~))                 \
{                                                                    \
  return BOOST_PP_SEQ_ELEM(2,data) /* name_to */ (                   \
    BOOST_PP_SEQ_ELEM(4,data) /* extra_arg_name */                   \
    BOOST_PP_COMMA_IF(n)                                             \
    BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_FORWARD_ARG,~)        \
    BOOST_PP_COMMA_IF(                                               \
      BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n))         \
    BOOST_PP_ENUM_PARAMS(                                            \
      BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n),         \
      boost::multi_index::detail::noarg() BOOST_PP_INTERCEPT)        \
  );                                                                 \
}

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(           \
  ret,name_from,name_to,extra_arg_type,extra_arg_name)               \
BOOST_PP_REPEAT_FROM_TO(                                             \
  0,BOOST_PP_ADD(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,1),           \
  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG_AUX,             \
  (ret)(name_from)(name_to)(extra_arg_type)(extra_arg_name))

namespace boost{
  
namespace multi_index{
  
namespace detail{

#define BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW_AUX(z,n,name)    \
template<                                                            \
  typename Value                                                     \
  BOOST_PP_COMMA_IF(n)                                               \
  BOOST_PP_ENUM_PARAMS(n,typename T)                                 \
>                                                                    \
Value* name(                                                         \
  Value* x                                                           \
  BOOST_PP_COMMA_IF(n)                                               \
  BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_ARG,~)                  \
  BOOST_PP_COMMA_IF(                                                 \
    BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n))           \
  BOOST_PP_ENUM_PARAMS(                                              \
    BOOST_PP_SUB(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,n),           \
    BOOST_FWD_REF(noarg) BOOST_PP_INTERCEPT))                        \
{                                                                    \
  return new(x) Value(                                               \
    BOOST_PP_ENUM(n,BOOST_MULTI_INDEX_VARTEMPL_FORWARD_ARG,~));      \
}

#define BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW(name)            \
BOOST_PP_REPEAT_FROM_TO(                                             \
  0,BOOST_PP_ADD(BOOST_MULTI_INDEX_LIMIT_VARTEMPL_ARGS,1),           \
  BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW_AUX,                   \
  name)

BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW(vartempl_placement_new)

#undef BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW_AUX
#undef BOOST_MULTI_INDEX_VARTEMPL_TO_PLACEMENT_NEW

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#else

/* native variadic templates support */

#include <utility>

#define BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK typename... Args
#define BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK Args&&... args
#define BOOST_MULTI_INDEX_FORWARD_PARAM_PACK  std::forward<Args>(args)...
#define BOOST_MULTI_INDEX_NULL_PARAM_PACK

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL(                     \
  ret,name_from,name_to)                                             \
template<typename... Args> ret name_from(Args&&... args)             \
{                                                                    \
  return name_to(std::forward<Args>(args)...);                       \
}

#define BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(           \
  ret,name_from,name_to,extra_arg_type,extra_arg_name)               \
template<typename... Args> ret name_from(                            \
  extra_arg_type extra_arg_name,Args&&... args)                      \
{                                                                    \
  return name_to(extra_arg_name,std::forward<Args>(args)...);        \
}

namespace boost{
  
namespace multi_index{
  
namespace detail{

template<typename Value,typename... Args>
Value* vartempl_placement_new(Value*x,Args&&... args)
{
  return new(x) Value(std::forward<Args>(args)...);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
#endif

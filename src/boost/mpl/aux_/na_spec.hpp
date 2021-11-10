
#ifndef BOOST_MPL_AUX_NA_SPEC_HPP_INCLUDED
#define BOOST_MPL_AUX_NA_SPEC_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#if !defined(BOOST_MPL_PREPROCESSING_MODE)
#   include <boost/mpl/lambda_fwd.hpp>
#   include <boost/mpl/int.hpp>
#   include <boost/mpl/bool.hpp>
#   include <boost/mpl/aux_/na.hpp>
#   include <boost/mpl/aux_/arity.hpp>
#   include <boost/mpl/aux_/template_arity_fwd.hpp>
#endif

#include <boost/mpl/aux_/preprocessor/params.hpp>
#include <boost/mpl/aux_/preprocessor/enum.hpp>
#include <boost/mpl/aux_/preprocessor/def_params_tail.hpp>
#include <boost/mpl/aux_/lambda_arity_param.hpp>
#include <boost/mpl/aux_/config/dtp.hpp>
#include <boost/mpl/aux_/config/eti.hpp>
#include <boost/mpl/aux_/nttp_decl.hpp>
#include <boost/mpl/aux_/config/ttp.hpp>
#include <boost/mpl/aux_/config/lambda.hpp>
#include <boost/mpl/aux_/config/overload_resolution.hpp>


#define BOOST_MPL_AUX_NA_PARAMS(i) \
    BOOST_MPL_PP_ENUM(i, na) \
/**/

#if defined(BOOST_MPL_CFG_BROKEN_DEFAULT_PARAMETERS_IN_NESTED_TEMPLATES)
#   define BOOST_MPL_AUX_NA_SPEC_ARITY(i, name) \
namespace aux { \
template< BOOST_MPL_AUX_NTTP_DECL(int, N) > \
struct arity< \
          name< BOOST_MPL_AUX_NA_PARAMS(i) > \
        , N \
        > \
    : int_< BOOST_MPL_LIMIT_METAFUNCTION_ARITY > \
{ \
}; \
} \
/**/
#else
#   define BOOST_MPL_AUX_NA_SPEC_ARITY(i, name) /**/
#endif

#define BOOST_MPL_AUX_NA_SPEC_MAIN(i, name) \
template<> \
struct name< BOOST_MPL_AUX_NA_PARAMS(i) > \
{ \
    template< \
          BOOST_MPL_PP_PARAMS(i, typename T) \
        BOOST_MPL_PP_NESTED_DEF_PARAMS_TAIL(i, typename T, na) \
        > \
    struct apply \
        : name< BOOST_MPL_PP_PARAMS(i, T) > \
    { \
    }; \
}; \
/**/

#if defined(BOOST_MPL_CFG_NO_FULL_LAMBDA_SUPPORT)
#   define BOOST_MPL_AUX_NA_SPEC_LAMBDA(i, name) \
template<> \
struct lambda< \
      name< BOOST_MPL_AUX_NA_PARAMS(i) > \
    , void_ \
    , true_ \
    > \
{ \
    typedef false_ is_le; \
    typedef name< BOOST_MPL_AUX_NA_PARAMS(i) > type; \
}; \
template<> \
struct lambda< \
      name< BOOST_MPL_AUX_NA_PARAMS(i) > \
    , void_ \
    , false_ \
    > \
{ \
    typedef false_ is_le; \
    typedef name< BOOST_MPL_AUX_NA_PARAMS(i) > type; \
}; \
/**/
#else
#   define BOOST_MPL_AUX_NA_SPEC_LAMBDA(i, name) \
template< typename Tag > \
struct lambda< \
      name< BOOST_MPL_AUX_NA_PARAMS(i) > \
    , Tag \
    BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(int_<-1>) \
    > \
{ \
    typedef false_ is_le; \
    typedef name< BOOST_MPL_AUX_NA_PARAMS(i) > result_; \
    typedef name< BOOST_MPL_AUX_NA_PARAMS(i) > type; \
}; \
/**/
#endif

#if defined(BOOST_MPL_CFG_EXTENDED_TEMPLATE_PARAMETERS_MATCHING) \
    || defined(BOOST_MPL_CFG_NO_FULL_LAMBDA_SUPPORT) \
        && defined(BOOST_MPL_CFG_BROKEN_OVERLOAD_RESOLUTION)
#   define BOOST_MPL_AUX_NA_SPEC_TEMPLATE_ARITY(i, j, name) \
namespace aux { \
template< BOOST_MPL_PP_PARAMS(j, typename T) > \
struct template_arity< \
          name< BOOST_MPL_PP_PARAMS(j, T) > \
        > \
    : int_<j> \
{ \
}; \
\
template<> \
struct template_arity< \
          name< BOOST_MPL_PP_ENUM(i, na) > \
        > \
    : int_<-1> \
{ \
}; \
} \
/**/
#else
#   define BOOST_MPL_AUX_NA_SPEC_TEMPLATE_ARITY(i, j, name) /**/
#endif

#if defined(BOOST_MPL_CFG_MSVC_ETI_BUG)
#   define BOOST_MPL_AUX_NA_SPEC_ETI(i, name) \
template<> \
struct name< BOOST_MPL_PP_ENUM(i, int) > \
{ \
    typedef int type; \
    enum { value = 0 }; \
}; \
/**/
#else
#   define BOOST_MPL_AUX_NA_SPEC_ETI(i, name) /**/
#endif

#define BOOST_MPL_AUX_NA_PARAM(param) param = na

#define BOOST_MPL_AUX_NA_SPEC_NO_ETI(i, name) \
BOOST_MPL_AUX_NA_SPEC_MAIN(i, name) \
BOOST_MPL_AUX_NA_SPEC_LAMBDA(i, name) \
BOOST_MPL_AUX_NA_SPEC_ARITY(i, name) \
BOOST_MPL_AUX_NA_SPEC_TEMPLATE_ARITY(i, i, name) \
/**/

#define BOOST_MPL_AUX_NA_SPEC(i, name) \
BOOST_MPL_AUX_NA_SPEC_NO_ETI(i, name) \
BOOST_MPL_AUX_NA_SPEC_ETI(i, name) \
/**/

#define BOOST_MPL_AUX_NA_SPEC2(i, j, name) \
BOOST_MPL_AUX_NA_SPEC_MAIN(i, name) \
BOOST_MPL_AUX_NA_SPEC_ETI(i, name) \
BOOST_MPL_AUX_NA_SPEC_LAMBDA(i, name) \
BOOST_MPL_AUX_NA_SPEC_ARITY(i, name) \
BOOST_MPL_AUX_NA_SPEC_TEMPLATE_ARITY(i, j, name) \
/**/


#endif // BOOST_MPL_AUX_NA_SPEC_HPP_INCLUDED

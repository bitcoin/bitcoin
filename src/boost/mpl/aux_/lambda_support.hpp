
#ifndef BOOST_MPL_AUX_LAMBDA_SUPPORT_HPP_INCLUDED
#define BOOST_MPL_AUX_LAMBDA_SUPPORT_HPP_INCLUDED

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

#include <boost/mpl/aux_/config/lambda.hpp>

#if !defined(BOOST_MPL_CFG_NO_FULL_LAMBDA_SUPPORT)

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) /**/
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT(i,name,params) /**/

#else

#   include <boost/mpl/int_fwd.hpp>
#   include <boost/mpl/aux_/yes_no.hpp>
#   include <boost/mpl/aux_/na_fwd.hpp>
#   include <boost/mpl/aux_/preprocessor/params.hpp>
#   include <boost/mpl/aux_/preprocessor/enum.hpp>
#   include <boost/mpl/aux_/config/msvc.hpp>
#   include <boost/mpl/aux_/config/workaround.hpp>

#   include <boost/preprocessor/tuple/to_list.hpp>
#   include <boost/preprocessor/list/for_each_i.hpp>
#   include <boost/preprocessor/inc.hpp>
#   include <boost/preprocessor/cat.hpp>

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_ARG_TYPEDEF_FUNC(R,typedef_,i,param) \
    typedef_ param BOOST_PP_CAT(arg,BOOST_PP_INC(i)); \
    /**/

// agurt, 07/mar/03: restore an old revision for the sake of SGI MIPSpro C++
#if BOOST_WORKAROUND(__EDG_VERSION__, <= 238) 

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT(i, name, params) \
    typedef BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE::int_<i> arity; \
    BOOST_PP_LIST_FOR_EACH_I_R( \
          1 \
        , BOOST_MPL_AUX_LAMBDA_SUPPORT_ARG_TYPEDEF_FUNC \
        , typedef \
        , BOOST_PP_TUPLE_TO_LIST(i,params) \
        ) \
    struct rebind \
    { \
        template< BOOST_MPL_PP_PARAMS(i,typename U) > struct apply \
            : name< BOOST_MPL_PP_PARAMS(i,U) > \
        { \
        }; \
    }; \
    /**/

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT(i, name, params) \
    /**/

#elif BOOST_WORKAROUND(__EDG_VERSION__, <= 244) && !defined(BOOST_INTEL_CXX_VERSION)
// agurt, 18/jan/03: old EDG-based compilers actually enforce 11.4 para 9
// (in strict mode), so we have to provide an alternative to the 
// MSVC-optimized implementation

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
    typedef BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE::int_<i> arity; \
    BOOST_PP_LIST_FOR_EACH_I_R( \
          1 \
        , BOOST_MPL_AUX_LAMBDA_SUPPORT_ARG_TYPEDEF_FUNC \
        , typedef \
        , BOOST_PP_TUPLE_TO_LIST(i,params) \
        ) \
    struct rebind; \
/**/

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT(i, name, params) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
}; \
template< BOOST_MPL_PP_PARAMS(i,typename T) > \
struct name<BOOST_MPL_PP_PARAMS(i,T)>::rebind \
{ \
    template< BOOST_MPL_PP_PARAMS(i,typename U) > struct apply \
        : name< BOOST_MPL_PP_PARAMS(i,U) > \
    { \
    }; \
/**/

#else // __EDG_VERSION__

namespace boost { namespace mpl { namespace aux {
template< typename T > struct has_rebind_tag;
}}}

#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
    typedef BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE::int_<i> arity; \
    BOOST_PP_LIST_FOR_EACH_I_R( \
          1 \
        , BOOST_MPL_AUX_LAMBDA_SUPPORT_ARG_TYPEDEF_FUNC \
        , typedef \
        , BOOST_PP_TUPLE_TO_LIST(i,params) \
        ) \
    friend class BOOST_PP_CAT(name,_rebind); \
    typedef BOOST_PP_CAT(name,_rebind) rebind; \
/**/

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610))
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_HAS_REBIND(i, name, params) \
template< BOOST_MPL_PP_PARAMS(i,typename T) > \
::boost::mpl::aux::yes_tag operator|( \
      ::boost::mpl::aux::has_rebind_tag<int> \
    , name<BOOST_MPL_PP_PARAMS(i,T)>* \
    ); \
::boost::mpl::aux::no_tag operator|( \
      ::boost::mpl::aux::has_rebind_tag<int> \
    , name< BOOST_MPL_PP_ENUM(i,::boost::mpl::na) >* \
    ); \
/**/
#elif !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_HAS_REBIND(i, name, params) \
template< BOOST_MPL_PP_PARAMS(i,typename T) > \
::boost::mpl::aux::yes_tag operator|( \
      ::boost::mpl::aux::has_rebind_tag<int> \
    , ::boost::mpl::aux::has_rebind_tag< name<BOOST_MPL_PP_PARAMS(i,T)> >* \
    ); \
/**/
#else
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT_HAS_REBIND(i, name, params) /**/
#endif

#   if !defined(BOOST_BORLANDC)
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT(i, name, params) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
}; \
BOOST_MPL_AUX_LAMBDA_SUPPORT_HAS_REBIND(i, name, params) \
class BOOST_PP_CAT(name,_rebind) \
{ \
 public: \
    template< BOOST_MPL_PP_PARAMS(i,typename U) > struct apply \
        : name< BOOST_MPL_PP_PARAMS(i,U) > \
    { \
    }; \
/**/
#   else
#   define BOOST_MPL_AUX_LAMBDA_SUPPORT(i, name, params) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(i, name, params) \
}; \
BOOST_MPL_AUX_LAMBDA_SUPPORT_HAS_REBIND(i, name, params) \
class BOOST_PP_CAT(name,_rebind) \
{ \
 public: \
    template< BOOST_MPL_PP_PARAMS(i,typename U) > struct apply \
    { \
        typedef typename name< BOOST_MPL_PP_PARAMS(i,U) >::type type; \
    }; \
/**/
#   endif // BOOST_BORLANDC

#endif // __EDG_VERSION__

#endif // BOOST_MPL_CFG_NO_FULL_LAMBDA_SUPPORT

#endif // BOOST_MPL_AUX_LAMBDA_SUPPORT_HPP_INCLUDED

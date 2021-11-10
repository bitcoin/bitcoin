
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_TEMPLATE_PARAMS_HPP)
#define BOOST_TTI_DETAIL_TEMPLATE_PARAMS_HPP

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/array/enum.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/tti/detail/denclosing_type.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)

#define BOOST_TTI_DETAIL_TEMPLATE_PARAMETERS(z,n,args) \
BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(4,n),args) \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_IMPLEMENTATION(args,introspect_macro) \
   template \
     < \
     typename BOOST_TTI_DETAIL_TP_T, \
     typename BOOST_TTI_DETAIL_TP_FALLBACK_ \
       = boost::mpl::bool_< BOOST_PP_ARRAY_ELEM(3, args) > \
     > \
   struct BOOST_PP_ARRAY_ELEM(0, args) \
     { \
     private: \
     introspect_macro(args) \
     public: \
       static const bool value \
         = BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args)< BOOST_TTI_DETAIL_TP_T >::value; \
       typedef typename BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) \
         < \
         BOOST_TTI_DETAIL_TP_T \
         >::type type; \
     }; \
/**/

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)

#define BOOST_TTI_DETAIL_HAS_MEMBER_MULTI_SUBSTITUTE(z,n,args) \
  template \
    < \
    template \
      < \
      BOOST_PP_ENUM_ ## z \
        ( \
        BOOST_PP_SUB \
          ( \
          BOOST_PP_ARRAY_SIZE(args), \
          4 \
          ), \
        BOOST_TTI_DETAIL_TEMPLATE_PARAMETERS, \
        args \
        ) \
      > \
    class BOOST_TTI_DETAIL_TM_V \
    > \
  struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_SUBSTITUTE(args) \
  BOOST_PP_REPEAT \
    ( \
    BOOST_PP_ARRAY_ELEM(2, args), \
    BOOST_TTI_DETAIL_HAS_MEMBER_MULTI_SUBSTITUTE, \
    args \
    ) \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_INTROSPECT(args) \
  template< typename U > \
  struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) \
    { \
    BOOST_TTI_DETAIL_HAS_MEMBER_SUBSTITUTE(args) \
    BOOST_MPL_HAS_MEMBER_REJECT(args, BOOST_PP_NIL) \
    BOOST_MPL_HAS_MEMBER_ACCEPT(args, BOOST_PP_NIL) \
    BOOST_STATIC_CONSTANT \
      ( \
      bool, value = BOOST_MPL_HAS_MEMBER_TEST(args) \
      ); \
    typedef boost::mpl::bool_< value > type; \
    }; \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_WITH_FUNCTION_SFINAE(args) \
  BOOST_TTI_DETAIL_HAS_MEMBER_IMPLEMENTATION \
    ( \
    args, \
    BOOST_TTI_DETAIL_HAS_MEMBER_INTROSPECT \
    ) \
/**/

#else // !!BOOST_WORKAROUND(BOOST_MSVC, <= 1400)

#define BOOST_TTI_DETAIL_HAS_MEMBER_MULTI_SUBSTITUTE_WITH_TEMPLATE_SFINAE(z,n,args) \
  template \
    < \
    template \
      < \
      BOOST_PP_ENUM_ ## z \
        ( \
        BOOST_PP_SUB \
          ( \
          BOOST_PP_ARRAY_SIZE(args), \
          4 \
          ), \
        BOOST_TTI_DETAIL_TEMPLATE_PARAMETERS, \
        args \
        ) \
      > \
    class BOOST_TTI_DETAIL_TM_U \
    > \
  struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE \
    ( \
    args, \
    n \
    ) \
    { \
    typedef \
      BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME(args) \
      type; \
    }; \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_SUBSTITUTE_WITH_TEMPLATE_SFINAE(args) \
  typedef void \
      BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME(args); \
  BOOST_PP_REPEAT \
    ( \
    BOOST_PP_ARRAY_ELEM(2, args), \
    BOOST_TTI_DETAIL_HAS_MEMBER_MULTI_SUBSTITUTE_WITH_TEMPLATE_SFINAE, \
    args \
    ) \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_INTROSPECT_WITH_TEMPLATE_SFINAE(args) \
  BOOST_MPL_HAS_MEMBER_REJECT_WITH_TEMPLATE_SFINAE(args,BOOST_PP_NIL) \
  BOOST_MPL_HAS_MEMBER_ACCEPT_WITH_TEMPLATE_SFINAE(args,BOOST_PP_NIL) \
  template< typename BOOST_TTI_DETAIL_TP_U > \
  struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) \
      : BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)< BOOST_TTI_DETAIL_TP_U > { \
  }; \
/**/

#define BOOST_TTI_DETAIL_HAS_MEMBER_WITH_TEMPLATE_SFINAE(args) \
  BOOST_TTI_DETAIL_HAS_MEMBER_SUBSTITUTE_WITH_TEMPLATE_SFINAE \
    ( \
    args \
    ) \
  BOOST_TTI_DETAIL_HAS_MEMBER_IMPLEMENTATION \
    ( \
    args, \
    BOOST_TTI_DETAIL_HAS_MEMBER_INTROSPECT_WITH_TEMPLATE_SFINAE \
    ) \
/**/

#endif // !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)

#else // defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)

#define BOOST_TTI_DETAIL_SAME(trait,name) \
  BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF \
    ( \
    trait, \
    name, \
    false \
    ) \
/**/

#define BOOST_TTI_DETAIL_TRAIT_CALL_HAS_TEMPLATE_CHECK_PARAMS(trait,name,tp) \
  BOOST_TTI_DETAIL_SAME(trait,name) \
/**/

#endif // !BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE

#define BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE_CHECK_PARAMS_OP(trait,name,tpArray) \
  BOOST_TTI_DETAIL_TRAIT_CALL_HAS_TEMPLATE_CHECK_PARAMS(BOOST_PP_CAT(trait,_detail),name,tpArray) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct BOOST_PP_CAT(trait,_detail_cp_op) : \
    BOOST_PP_CAT(trait,_detail)<BOOST_TTI_DETAIL_TP_T> \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE_CHECK_PARAMS(trait,name,tpArray) \
  BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE_CHECK_PARAMS_OP(trait,name,tpArray) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct trait \
    { \
    typedef typename \
      boost::mpl::eval_if \
        < \
        BOOST_TTI_NAMESPACE::detail::enclosing_type<BOOST_TTI_DETAIL_TP_T>, \
        BOOST_PP_CAT(trait,_detail_cp_op)<BOOST_TTI_DETAIL_TP_T>, \
        boost::mpl::false_ \
        >::type type; \
    BOOST_STATIC_CONSTANT(bool,value=type::value); \
    }; \
/**/

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)

#define BOOST_TTI_DETAIL_TRAIT_CALL_HAS_TEMPLATE_CHECK_PARAMS(trait,name,tpArray) \
  BOOST_TTI_DETAIL_HAS_MEMBER_WITH_FUNCTION_SFINAE \
    (  \
      ( BOOST_PP_ADD(BOOST_PP_ARRAY_SIZE(tpArray),4), ( trait, name, 1, false, BOOST_PP_ARRAY_ENUM(tpArray) ) )  \
    )  \
/**/

#else // BOOST_WORKAROUND(BOOST_MSVC, <= 1400)

#define BOOST_TTI_DETAIL_TRAIT_CALL_HAS_TEMPLATE_CHECK_PARAMS(trait,name,tpArray) \
  BOOST_TTI_DETAIL_HAS_MEMBER_WITH_TEMPLATE_SFINAE \
    ( \
      ( BOOST_PP_ADD(BOOST_PP_ARRAY_SIZE(tpArray),4), ( trait, name, 1, false, BOOST_PP_ARRAY_ENUM(tpArray) ) )  \
    ) \
/**/

#endif // !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
#endif // !defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)

#endif // BOOST_TTI_DETAIL_TEMPLATE_PARAMS_HPP


//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_TYPE_HPP)
#define BOOST_TTI_DETAIL_TYPE_HPP

#include <boost/mpl/apply.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/ddeftype.hpp>
#include <boost/tti/detail/dlambda.hpp>
#include <boost/tti/detail/denclosing_type.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

#define BOOST_TTI_DETAIL_TRAIT_INVOKE_HAS_TYPE(trait,name) \
template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_MFC> \
struct BOOST_PP_CAT(trait,_detail_type_invoke) : \
  boost::mpl::apply<BOOST_TTI_DETAIL_TP_MFC,typename BOOST_TTI_DETAIL_TP_T::name> \
  { \
  }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_OP_CHOOSE(trait,name) \
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(BOOST_PP_CAT(trait,_detail_type_mpl), name, false) \
BOOST_TTI_DETAIL_TRAIT_INVOKE_HAS_TYPE(trait,name) \
template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_U,class BOOST_TTI_DETAIL_TP_B> \
struct BOOST_PP_CAT(trait,_detail_type_op_choose) \
  { \
  BOOST_MPL_ASSERT((BOOST_TTI_NAMESPACE::detail::is_lambda_expression<BOOST_TTI_DETAIL_TP_U>)); \
  typedef typename BOOST_PP_CAT(trait,_detail_type_invoke)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_U>::type type; \
  }; \
\
template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_U> \
struct BOOST_PP_CAT(trait,_detail_type_op_choose)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_U,boost::mpl::false_::type> : \
  boost::mpl::false_ \
  { \
  }; \
\
template<class BOOST_TTI_DETAIL_TP_T> \
struct BOOST_PP_CAT(trait,_detail_type_op_choose)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_NAMESPACE::detail::deftype,boost::mpl::true_::type> : \
  boost::mpl::true_ \
  { \
  }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_OP(trait,name) \
BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_OP_CHOOSE(trait,name) \
template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_U> \
struct BOOST_PP_CAT(trait,_detail_type_op) : \
  BOOST_PP_CAT(trait,_detail_type_op_choose) \
    < \
    BOOST_TTI_DETAIL_TP_T, \
    BOOST_TTI_DETAIL_TP_U, \
    typename BOOST_PP_CAT(trait,_detail_type_mpl)<BOOST_TTI_DETAIL_TP_T>::type \
    > \
  { \
  }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TYPE(trait,name) \
BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_OP(trait,name) \
template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_U> \
struct BOOST_PP_CAT(trait,_detail_type) : \
    boost::mpl::eval_if \
        < \
        BOOST_TTI_NAMESPACE::detail::enclosing_type<BOOST_TTI_DETAIL_TP_T>, \
        BOOST_PP_CAT(trait,_detail_type_op)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_U>, \
        boost::mpl::false_ \
        > \
  { \
  }; \
/**/

#endif // BOOST_TTI_DETAIL_TYPE_HPP


//  (C) Copyright Edward Diener 2012,2013,2019
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_FUNCTION_HPP)
#define BOOST_TTI_DETAIL_FUNCTION_HPP

#include <boost/mpl/and.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/dmem_fun.hpp>
#include <boost/tti/detail/dstatic_mem_fun.hpp>
#include <boost/tti/detail/dtfunction.hpp>
#include <boost/tti/detail/denclosing_type.hpp>
#include <boost/tti/detail/dstatic_function_tags.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION_STATIC_CALL(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_IMPL_HAS_STATIC_MEMBER_FUNCTION(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_R,class BOOST_TTI_DETAIL_TP_FS,class BOOST_TTI_DETAIL_TP_TAG> \
  struct BOOST_PP_CAT(trait,_detail_hfsc) : \
    BOOST_PP_CAT(trait,_detail_ihsmf) \
        < \
        BOOST_TTI_DETAIL_TP_T, \
        typename BOOST_TTI_NAMESPACE::detail::tfunction_seq<BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG>::type \
        > \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION_STATIC(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION_STATIC_CALL(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_R,class BOOST_TTI_DETAIL_TP_FS,class BOOST_TTI_DETAIL_TP_TAG> \
  struct BOOST_PP_CAT(trait,_detail_hfs) : \
    boost::mpl::eval_if \
        < \
        boost::mpl::and_ \
            < \
            BOOST_TTI_NAMESPACE::detail::enclosing_type<BOOST_TTI_DETAIL_TP_T>, \
            BOOST_TTI_NAMESPACE::detail::static_function_tag<BOOST_TTI_DETAIL_TP_TAG> \
            >, \
        BOOST_PP_CAT(trait,_detail_hfsc)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG>, \
        boost::mpl::false_ \
        > \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_CALL_TYPES_MEMBER_FUNCTION(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION_STATIC(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_R,class BOOST_TTI_DETAIL_TP_FS,class BOOST_TTI_DETAIL_TP_TAG> \
  struct BOOST_PP_CAT(trait,_detail_hf) : \
    boost::mpl::or_ \
        < \
        BOOST_PP_CAT(trait,_detail_hmf_call_types)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG>, \
        BOOST_PP_CAT(trait,_detail_hfs)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG> \
        > \
    { \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_FUNCTION_HPP

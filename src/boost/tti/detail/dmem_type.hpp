
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_MEM_TYPE_HPP)
#define BOOST_TTI_DETAIL_MEM_TYPE_HPP

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/denclosing_type.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_MEMBER_TYPE_OP(trait,name) \
  BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(BOOST_PP_CAT(trait,_detail_member_type_mpl), name, false) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct BOOST_PP_CAT(trait,_detail_member_type_op) : \
    BOOST_PP_CAT(trait,_detail_member_type_mpl)<BOOST_TTI_DETAIL_TP_T> \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_MEMBER_TYPE(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_MEMBER_TYPE_OP(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct BOOST_PP_CAT(trait,_detail_has_type_member_type) \
    { \
    typedef typename \
    boost::mpl::eval_if \
        < \
        BOOST_TTI_NAMESPACE::detail::enclosing_type<BOOST_TTI_DETAIL_TP_T>, \
        BOOST_PP_CAT(trait,_detail_member_type_op)<BOOST_TTI_DETAIL_TP_T>, \
        boost::mpl::false_ \
        >::type type; \
    \
    BOOST_STATIC_CONSTANT(bool,value=type::value); \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_MEMBER_TYPE(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct BOOST_PP_CAT(trait,_detail_member_type) \
    { \
    typedef typename BOOST_TTI_DETAIL_TP_T::name type; \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_MEM_TYPE_HPP


//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_MEM_DATA_HPP)
#define BOOST_TTI_DETAIL_MEM_DATA_HPP

#include <boost/detail/workaround.hpp>
#include <boost/function_types/components.hpp>
#include <boost/function_types/is_member_object_pointer.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/ddeftype.hpp>
#include <boost/tti/detail/dftclass.hpp>
#include <boost/tti/detail/denclosing_type.hpp>
#include <boost/tti/gen/namespace_gen.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>

#if defined(BOOST_MSVC) || (BOOST_WORKAROUND(BOOST_GCC, >= 40400) && BOOST_WORKAROUND(BOOST_GCC, < 40600))

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_OP(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_C> \
  struct BOOST_PP_CAT(trait,_detail_hmd_op) \
    { \
    template<class> \
    struct return_of; \
    \
    template<class BOOST_TTI_DETAIL_TP_R,class BOOST_TTI_DETAIL_TP_IC> \
    struct return_of<BOOST_TTI_DETAIL_TP_R BOOST_TTI_DETAIL_TP_IC::*> \
      { \
      typedef BOOST_TTI_DETAIL_TP_R type; \
      }; \
    \
    template<bool,typename BOOST_TTI_DETAIL_TP_U> \
    struct menable_if; \
    \
    template<typename BOOST_TTI_DETAIL_TP_U> \
    struct menable_if<true,BOOST_TTI_DETAIL_TP_U> \
      { \
      typedef BOOST_TTI_DETAIL_TP_U type; \
      }; \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::yes_type check2(BOOST_TTI_DETAIL_TP_V BOOST_TTI_DETAIL_TP_U::*); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::no_type check2(BOOST_TTI_DETAIL_TP_U); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static typename \
      menable_if \
        < \
        sizeof(check2<BOOST_TTI_DETAIL_TP_U,BOOST_TTI_DETAIL_TP_V>(&BOOST_TTI_DETAIL_TP_U::name))==sizeof(::boost::type_traits::yes_type), \
        ::boost::type_traits::yes_type \
        > \
      ::type \
    has_matching_member(int); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::no_type has_matching_member(...); \
    \
    template<class BOOST_TTI_DETAIL_TP_U,class BOOST_TTI_DETAIL_TP_V> \
    struct ttc_md \
      { \
      typedef boost::mpl::bool_<sizeof(has_matching_member<BOOST_TTI_DETAIL_TP_V,typename return_of<BOOST_TTI_DETAIL_TP_U>::type>(0))==sizeof(::boost::type_traits::yes_type)> type; \
      }; \
    \
    typedef typename ttc_md<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_C>::type type; \
    \
    }; \
/**/

#else // !defined(BOOST_MSVC)

#include <boost/tti/detail/dmem_fun.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_OP(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_TYPES_MEMBER_FUNCTION(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_C> \
  struct BOOST_PP_CAT(trait,_detail_hmd_op) : \
    BOOST_PP_CAT(trait,_detail_hmf_types)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_C> \
    { \
    }; \
/**/

#endif // defined(BOOST_MSVC)

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_INVOKE_ENCLOSING_CLASS(trait) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hmd_invoke_enclosing_class) : \
    BOOST_PP_CAT(trait,_detail_hmd_op) \
        < \
        typename BOOST_TTI_NAMESPACE::detail::ptmd<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_TYPE>::type, \
        typename boost::remove_const<BOOST_TTI_DETAIL_TP_ET>::type \
        > \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_INVOKE_PT_MEMBER(trait) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hmd_invoke_pt_member) : \
    BOOST_PP_CAT(trait,_detail_hmd_op) \
        < \
        typename BOOST_TTI_NAMESPACE::detail::dmem_get_type<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_TYPE>::type, \
        typename boost::remove_const \
            < \
            typename BOOST_TTI_NAMESPACE::detail::dmem_get_enclosing<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_TYPE>::type \
            >::type \
        > \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_WITH_ENCLOSING_CLASS(trait) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_INVOKE_ENCLOSING_CLASS(trait) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hmd_with_enclosing_class) : \
    boost::mpl::eval_if \
        < \
        BOOST_TTI_NAMESPACE::detail::enclosing_type<BOOST_TTI_DETAIL_TP_ET>, \
        BOOST_PP_CAT(trait,_detail_hmd_invoke_enclosing_class) \
            < \
            BOOST_TTI_DETAIL_TP_ET, \
            BOOST_TTI_DETAIL_TP_TYPE \
            >, \
        boost::mpl::false_ \
        > \
    { \
    }; \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_OP(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_WITH_ENCLOSING_CLASS(trait) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA_INVOKE_PT_MEMBER(trait) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hmd) : \
    boost::mpl::eval_if \
        < \
        boost::is_same<BOOST_TTI_DETAIL_TP_TYPE,BOOST_TTI_NAMESPACE::detail::deftype>, \
        BOOST_PP_CAT(trait,_detail_hmd_invoke_pt_member) \
            < \
            BOOST_TTI_DETAIL_TP_ET, \
            BOOST_TTI_DETAIL_TP_TYPE \
            >, \
        BOOST_PP_CAT(trait,_detail_hmd_with_enclosing_class) \
            < \
            BOOST_TTI_DETAIL_TP_ET, \
            BOOST_TTI_DETAIL_TP_TYPE \
            > \
        > \
    { \
    }; \
/**/

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      
      template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_R>
      struct ptmd
        {
        typedef BOOST_TTI_DETAIL_TP_R BOOST_TTI_DETAIL_TP_T::* type;
        };
        
      template<class BOOST_TTI_DETAIL_TP_T>
      struct dmem_check_ptmd :
        boost::mpl::identity<BOOST_TTI_DETAIL_TP_T>
        {
        BOOST_MPL_ASSERT((boost::function_types::is_member_object_pointer<BOOST_TTI_DETAIL_TP_T>));
        };
        
      template<class BOOST_TTI_DETAIL_TP_T>
      struct dmem_check_ptec :
        BOOST_TTI_NAMESPACE::detail::class_type<BOOST_TTI_DETAIL_TP_T>
        {
        BOOST_MPL_ASSERT((boost::function_types::is_member_object_pointer<BOOST_TTI_DETAIL_TP_T>));
        };
        
      template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_T2>
      struct dmem_get_type :
        boost::mpl::eval_if
          <
          boost::is_same<BOOST_TTI_DETAIL_TP_T2,BOOST_TTI_NAMESPACE::detail::deftype>,
          BOOST_TTI_NAMESPACE::detail::dmem_check_ptmd<BOOST_TTI_DETAIL_TP_T>,
          BOOST_TTI_NAMESPACE::detail::ptmd<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_T2>
          >
        {
        };
        
      template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_T2>
      struct dmem_get_enclosing :
        boost::mpl::eval_if
          <
          boost::is_same<BOOST_TTI_DETAIL_TP_T2,BOOST_TTI_NAMESPACE::detail::deftype>,
          BOOST_TTI_NAMESPACE::detail::dmem_check_ptec<BOOST_TTI_DETAIL_TP_T>,
          boost::mpl::identity<BOOST_TTI_DETAIL_TP_T>
          >
        {
        };
        
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_MEM_DATA_HPP

//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
// 
#ifndef BOOST_NUMERIC_CONVERSION_DETAIL_META_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_DETAIL_META_FLC_12NOV2002_HPP

#include "boost/type_traits/remove_cv.hpp"

#include "boost/mpl/if.hpp"
#include "boost/mpl/eval_if.hpp"
#include "boost/mpl/equal_to.hpp"
#include "boost/mpl/not.hpp"
#include "boost/mpl/and.hpp"
#include "boost/mpl/bool.hpp"
#include "boost/mpl/identity.hpp"

namespace boost { namespace numeric { namespace convdetail
{
   template< class T1, class T2>
   struct equal_to
   {
   #if !defined(BOOST_BORLANDC)
   
       enum { x = ( BOOST_MPL_AUX_VALUE_WKND(T1)::value == BOOST_MPL_AUX_VALUE_WKND(T2)::value ) };
           
       BOOST_STATIC_CONSTANT(bool, value = x);
           
       typedef mpl::bool_<value> type;
       
   #else
   
       BOOST_STATIC_CONSTANT(bool, value = (
             BOOST_MPL_AUX_VALUE_WKND(T1)::value 
               == BOOST_MPL_AUX_VALUE_WKND(T2)::value
           ));
           
       typedef mpl::bool_<(
             BOOST_MPL_AUX_VALUE_WKND(T1)::value 
               == BOOST_MPL_AUX_VALUE_WKND(T2)::value
           )> type;
   #endif
   };
    
// Metafunction:
  //
  //   ct_switch4<Value,Case0Val,Case1Val,Case2Val,Case0Type,Case1Type,Case2Type,DefaultType>::type
  //
  // {Value,Case(X)Val} are Integral Constants (such as: mpl::int_<>)
  // {Case(X)Type,DefaultType} are arbitrary types. (not metafunctions)
  //
  // Returns Case(X)Type if Val==Case(X)Val; DefaultType otherwise.
  //
  template<class Value,
           class Case0Val,
           class Case1Val,
           class Case2Val,
           class Case0Type,
           class Case1Type,
           class Case2Type,
           class DefaultType
          >
  struct ct_switch4
  {
    typedef mpl::identity<Case0Type> Case0TypeQ ;
    typedef mpl::identity<Case1Type> Case1TypeQ ;

    typedef equal_to<Value,Case0Val> is_case0 ;
    typedef equal_to<Value,Case1Val> is_case1 ;
    typedef equal_to<Value,Case2Val> is_case2 ;

    typedef mpl::if_<is_case2,Case2Type,DefaultType> choose_2_3Q ;
    typedef mpl::eval_if<is_case1,Case1TypeQ,choose_2_3Q> choose_1_2_3Q ;

    typedef typename
      mpl::eval_if<is_case0,Case0TypeQ,choose_1_2_3Q>::type
        type ;
  } ;




  // Metafunction:
  //
  //   for_both<expr0,expr1,TT,TF,FT,FF>::type
  //
  // {exp0,expr1} are Boolean Integral Constants
  // {TT,TF,FT,FF} are aribtrary types. (not metafunctions)
  //
  // According to the combined boolean value of 'expr0 && expr1', selects the corresponding type.
  //
  template<class expr0, class expr1, class TT, class TF, class FT, class FF>
  struct for_both
  {
    typedef mpl::identity<TF> TF_Q ;
    typedef mpl::identity<TT> TT_Q ;

    typedef typename mpl::not_<expr0>::type not_expr0 ;
    typedef typename mpl::not_<expr1>::type not_expr1 ;

    typedef typename mpl::and_<expr0,expr1>::type     caseTT ;
    typedef typename mpl::and_<expr0,not_expr1>::type caseTF ;
    typedef typename mpl::and_<not_expr0,expr1>::type caseFT ;

    typedef mpl::if_<caseFT,FT,FF>                    choose_FT_FF_Q ;
    typedef mpl::eval_if<caseTF,TF_Q,choose_FT_FF_Q> choose_TF_FT_FF_Q ;

    typedef typename mpl::eval_if<caseTT,TT_Q,choose_TF_FT_FF_Q>::type type ;
  } ;

} } } // namespace boost::numeric::convdetail

#endif



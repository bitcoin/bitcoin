//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
// 
#ifndef BOOST_NUMERIC_CONVERSION_DETAIL_CONVERSION_TRAITS_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_DETAIL_CONVERSION_TRAITS_FLC_12NOV2002_HPP

#include "boost/type_traits/is_arithmetic.hpp"
#include "boost/type_traits/is_same.hpp"
#include "boost/type_traits/remove_cv.hpp"

#include "boost/numeric/conversion/detail/meta.hpp"
#include "boost/numeric/conversion/detail/int_float_mixture.hpp"
#include "boost/numeric/conversion/detail/sign_mixture.hpp"
#include "boost/numeric/conversion/detail/udt_builtin_mixture.hpp"
#include "boost/numeric/conversion/detail/is_subranged.hpp"

namespace boost { namespace numeric { namespace convdetail
{
  //-------------------------------------------------------------------
  // Implementation of the Conversion Traits for T != S
  //
  // This is a VISIBLE base class of the user-level conversion_traits<> class.
  //-------------------------------------------------------------------
  template<class T,class S>
  struct non_trivial_traits_impl
  {
    typedef typename get_int_float_mixture   <T,S>::type int_float_mixture ;
    typedef typename get_sign_mixture        <T,S>::type sign_mixture ;
    typedef typename get_udt_builtin_mixture <T,S>::type udt_builtin_mixture ;

    typedef typename get_is_subranged<T,S>::type subranged ;

    typedef mpl::false_ trivial ;

    typedef T target_type ;
    typedef S source_type ;
    typedef T result_type ;

    typedef typename mpl::if_< is_arithmetic<S>, S, S const&>::type argument_type ;

    typedef typename mpl::if_<subranged,S,T>::type supertype ;
    typedef typename mpl::if_<subranged,T,S>::type subtype   ;
  } ;

  //-------------------------------------------------------------------
  // Implementation of the Conversion Traits for T == S
  //
  // This is a VISIBLE base class of the user-level conversion_traits<> class.
  //-------------------------------------------------------------------
  template<class N>
  struct trivial_traits_impl
  {
    typedef typename get_int_float_mixture  <N,N>::type int_float_mixture ;
    typedef typename get_sign_mixture       <N,N>::type sign_mixture ;
    typedef typename get_udt_builtin_mixture<N,N>::type udt_builtin_mixture ;

    typedef mpl::false_ subranged ;
    typedef mpl::true_  trivial ;

    typedef N        target_type ;
    typedef N        source_type ;
    typedef N const& result_type ;
    typedef N const& argument_type ;

    typedef N supertype ;
    typedef N subtype  ;

  } ;

  //-------------------------------------------------------------------
  // Top level implementation selector.
  //-------------------------------------------------------------------
  template<class T, class S>
  struct get_conversion_traits
  {
    typedef typename remove_cv<T>::type target_type ;
    typedef typename remove_cv<S>::type source_type ;

    typedef typename is_same<target_type,source_type>::type is_trivial ;

    typedef trivial_traits_impl    <target_type>             trivial_imp ;
    typedef non_trivial_traits_impl<target_type,source_type> non_trivial_imp ;

    typedef typename mpl::if_<is_trivial,trivial_imp,non_trivial_imp>::type type ;
  } ;

} } } // namespace boost::numeric::convdetail

#endif



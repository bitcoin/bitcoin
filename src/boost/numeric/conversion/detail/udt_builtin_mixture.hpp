//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
// 
#ifndef BOOST_NUMERIC_CONVERSION_DETAIL_UDT_BUILTIN_MIXTURE_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_DETAIL_UDT_BUILTIN_MIXTURE_FLC_12NOV2002_HPP

#include "boost/type_traits/is_arithmetic.hpp"

#include "boost/numeric/conversion/udt_builtin_mixture_enum.hpp"
#include "boost/numeric/conversion/detail/meta.hpp"

#include "boost/mpl/integral_c.hpp"

namespace boost { namespace numeric { namespace convdetail
{
  // Integral Constants for 'UdtMixture'
  typedef mpl::integral_c<udt_builtin_mixture_enum, builtin_to_builtin> builtin2builtin_c ;
  typedef mpl::integral_c<udt_builtin_mixture_enum, builtin_to_udt>     builtin2udt_c ;
  typedef mpl::integral_c<udt_builtin_mixture_enum, udt_to_builtin>     udt2builtin_c ;
  typedef mpl::integral_c<udt_builtin_mixture_enum, udt_to_udt>         udt2udt_c ;

  // Metafunction:
  //
  //   for_udt_mixture<UdtMixture,BuiltIn2BuiltIn,BuiltIn2Udt,Udt2BuiltIn,Udt2Udt>::type
  //
  // {UdtMixture} is one of the Integral Constants for UdMixture, declared above.
  // {BuiltIn2BuiltIn,BuiltIn2Udt,Udt2BuiltIn,Udt2Udt} are aribtrary types. (not metafunctions)
  //
  // According to the value of 'UdtMixture', selects the corresponding type.
  //
  template<class UdtMixture, class BuiltIn2BuiltIn, class BuiltIn2Udt, class Udt2BuiltIn, class Udt2Udt>
  struct for_udt_builtin_mixture
  {
    typedef typename
      ct_switch4<UdtMixture
                 , builtin2builtin_c, builtin2udt_c, udt2builtin_c // default
                 , BuiltIn2BuiltIn  , BuiltIn2Udt  , Udt2BuiltIn  , Udt2Udt
                >::type
        type ;
  } ;

  // Metafunction:
  //
  //   get_udt_mixture<T,S>::type
  //
  // Selects the appropriate UdtMixture Integral Constant for the combination T,S.
  //
  template<class T,class S>
  struct get_udt_builtin_mixture
  {
    typedef is_arithmetic<S> S_builtin ;
    typedef is_arithmetic<T> T_builtin ;

    typedef typename
      for_both<S_builtin, T_builtin, builtin2builtin_c, builtin2udt_c, udt2builtin_c, udt2udt_c>::type
        type ;
  } ;

} } } // namespace boost::numeric::convdetail

#endif



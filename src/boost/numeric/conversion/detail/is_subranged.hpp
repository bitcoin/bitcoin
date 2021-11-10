//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
// 
#ifndef BOOST_NUMERIC_CONVERSION_DETAIL_IS_SUBRANGED_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_DETAIL_IS_SUBRANGED_FLC_12NOV2002_HPP

#include "boost/config.hpp"
#include "boost/limits.hpp"

#include "boost/mpl/int.hpp"
#include "boost/mpl/multiplies.hpp"
#include "boost/mpl/less.hpp"
#include "boost/mpl/equal_to.hpp"

#include "boost/type_traits/is_same.hpp"

#include "boost/numeric/conversion/detail/meta.hpp"
#include "boost/numeric/conversion/detail/int_float_mixture.hpp"
#include "boost/numeric/conversion/detail/sign_mixture.hpp"
#include "boost/numeric/conversion/detail/udt_builtin_mixture.hpp"

namespace boost { namespace numeric { namespace convdetail
{
  //---------------------------------------------------------------
  // Implementations of the compile time predicate "T is subranged"
  //---------------------------------------------------------------

    // for integral to integral conversions
    template<class T,class S>
    struct subranged_Sig2Unsig
    {
      // Signed to unsigned conversions are 'subranged' because of possible loose
      // of negative values.
      typedef mpl::true_ type ;
    } ;

    // for unsigned integral to signed integral conversions
    template<class T,class S>
    struct subranged_Unsig2Sig
    {
       // IMPORTANT NOTE:
       //
       // This code assumes that signed/unsigned integral values are represented
       // such that:
       //
       //  numeric_limits<signed T>::digits + 1 == numeric_limits<unsigned T>::digits
       //
       // The '+1' is required since numeric_limits<>::digits gives 1 bit less for signed integral types.
       //
       // This fact is used by the following logic:
       //
       //  if ( (numeric_limits<T>::digits+1) < (2*numeric_limits<S>::digits) )
       //    then the conversion is subranged.
       //

       typedef mpl::int_< ::std::numeric_limits<S>::digits > S_digits ;
       typedef mpl::int_< ::std::numeric_limits<T>::digits > T_digits ;

       // T is signed, so take digits+1
       typedef typename T_digits::next u_T_digits ;

       typedef mpl::int_<2> Two ;

       typedef typename mpl::multiplies<S_digits,Two>::type S_digits_times_2 ;

       typedef typename mpl::less<u_T_digits,S_digits_times_2>::type type ;
    } ;

    // for integral to integral conversions of the same sign.
    template<class T,class S>
    struct subranged_SameSign
    {
       // An integral conversion of the same sign is subranged if digits(T) < digits(S).

       typedef mpl::int_< ::std::numeric_limits<S>::digits > S_digits ;
       typedef mpl::int_< ::std::numeric_limits<T>::digits > T_digits ;

       typedef typename mpl::less<T_digits,S_digits>::type type ;
    } ;

    // for integral to float conversions
    template<class T,class S>
    struct subranged_Int2Float
    {
      typedef mpl::false_ type ;
    } ;

    // for float to integral conversions
    template<class T,class S>
    struct subranged_Float2Int
    {
      typedef mpl::true_ type ;
    } ;

    // for float to float conversions
    template<class T,class S>
    struct subranged_Float2Float
    {
      // If both T and S are floats,
      // compare exponent bits and if they match, mantisa bits.

      typedef mpl::int_< ::std::numeric_limits<S>::digits > S_mantisa ;
      typedef mpl::int_< ::std::numeric_limits<T>::digits > T_mantisa ;

      typedef mpl::int_< ::std::numeric_limits<S>::max_exponent > S_exponent ;
      typedef mpl::int_< ::std::numeric_limits<T>::max_exponent > T_exponent ;

      typedef typename mpl::less<T_exponent,S_exponent>::type T_smaller_exponent ;

      typedef typename mpl::equal_to<T_exponent,S_exponent>::type equal_exponents ;

      typedef mpl::less<T_mantisa,S_mantisa> T_smaller_mantisa ;

      typedef mpl::eval_if<equal_exponents,T_smaller_mantisa,mpl::false_> not_bigger_exponent_case ;

      typedef typename
        mpl::eval_if<T_smaller_exponent,mpl::true_,not_bigger_exponent_case>::type
          type ;
    } ;

    // for Udt to built-in conversions
    template<class T,class S>
    struct subranged_Udt2BuiltIn
    {
      typedef mpl::true_ type ;
    } ;

    // for built-in to Udt conversions
    template<class T,class S>
    struct subranged_BuiltIn2Udt
    {
      typedef mpl::false_ type ;
    } ;

    // for Udt to Udt conversions
    template<class T,class S>
    struct subranged_Udt2Udt
    {
      typedef mpl::false_ type ;
    } ;

  //-------------------------------------------------------------------
  // Selectors for the implementations of the subranged predicate
  //-------------------------------------------------------------------

    template<class T,class S>
    struct get_subranged_Int2Int
    {
      typedef subranged_SameSign<T,S>  Sig2Sig     ;
      typedef subranged_Sig2Unsig<T,S> Sig2Unsig   ;
      typedef subranged_Unsig2Sig<T,S> Unsig2Sig   ;
      typedef Sig2Sig                  Unsig2Unsig ;

      typedef typename get_sign_mixture<T,S>::type sign_mixture ;

      typedef typename
        for_sign_mixture<sign_mixture, Sig2Sig, Sig2Unsig, Unsig2Sig, Unsig2Unsig>::type
           type ;
    } ;

    template<class T,class S>
    struct get_subranged_BuiltIn2BuiltIn
    {
      typedef get_subranged_Int2Int<T,S> Int2IntQ ;

      typedef subranged_Int2Float  <T,S> Int2Float   ;
      typedef subranged_Float2Int  <T,S> Float2Int   ;
      typedef subranged_Float2Float<T,S> Float2Float ;

      typedef mpl::identity<Int2Float  > Int2FloatQ   ;
      typedef mpl::identity<Float2Int  > Float2IntQ   ;
      typedef mpl::identity<Float2Float> Float2FloatQ ;

      typedef typename get_int_float_mixture<T,S>::type int_float_mixture ;

      typedef for_int_float_mixture<int_float_mixture, Int2IntQ, Int2FloatQ, Float2IntQ, Float2FloatQ> for_ ;

      typedef typename for_::type selected ;

      typedef typename selected::type type ;
    } ;

    template<class T,class S>
    struct get_subranged
    {
      typedef get_subranged_BuiltIn2BuiltIn<T,S> BuiltIn2BuiltInQ ;

      typedef subranged_BuiltIn2Udt<T,S> BuiltIn2Udt ;
      typedef subranged_Udt2BuiltIn<T,S> Udt2BuiltIn ;
      typedef subranged_Udt2Udt<T,S>     Udt2Udt ;

      typedef mpl::identity<BuiltIn2Udt> BuiltIn2UdtQ ;
      typedef mpl::identity<Udt2BuiltIn> Udt2BuiltInQ ;
      typedef mpl::identity<Udt2Udt    > Udt2UdtQ     ;

      typedef typename get_udt_builtin_mixture<T,S>::type udt_builtin_mixture ;
      
      typedef typename
        for_udt_builtin_mixture<udt_builtin_mixture, BuiltIn2BuiltInQ, BuiltIn2UdtQ, Udt2BuiltInQ, Udt2UdtQ>::type
          selected ;

      typedef typename selected::type selected2 ;
 
      typedef typename selected2::type type ;
    } ;


  //-------------------------------------------------------------------
  // Top level implementation selector.
  //-------------------------------------------------------------------
  template<class T, class S>
  struct get_is_subranged
  {
    typedef get_subranged<T,S>         non_trivial_case ;
    typedef mpl::identity<mpl::false_> trivial_case ;

    typedef is_same<T,S> is_trivial ;
   
    typedef typename mpl::if_<is_trivial,trivial_case,non_trivial_case>::type selected ;
    
    typedef typename selected::type type ;
  } ;

} } } // namespace boost::numeric::convdetail

#endif



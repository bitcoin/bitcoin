//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
//
#ifndef BOOST_NUMERIC_CONVERSION_DETAIL_CONVERTER_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_DETAIL_CONVERTER_FLC_12NOV2002_HPP

#include <functional>

#include "boost/numeric/conversion/detail/meta.hpp"
#include "boost/numeric/conversion/detail/conversion_traits.hpp"
#include "boost/numeric/conversion/bounds.hpp"

#include "boost/type_traits/is_same.hpp"

#include "boost/mpl/integral_c.hpp"

namespace boost { namespace numeric { namespace convdetail
{
  // Integral Constants representing rounding modes
  typedef mpl::integral_c<std::float_round_style, std::round_toward_zero>         round2zero_c ;
  typedef mpl::integral_c<std::float_round_style, std::round_to_nearest>          round2nearest_c ;
  typedef mpl::integral_c<std::float_round_style, std::round_toward_infinity>     round2inf_c ;
  typedef mpl::integral_c<std::float_round_style, std::round_toward_neg_infinity> round2neg_inf_c ;

  // Metafunction:
  //
  //   for_round_style<RoundStyle,RoundToZero,RoundToNearest,RoundToInf,RoundToNegInf>::type
  //
  // {RoundStyle} Integral Constant specifying a round style as declared above.
  // {RoundToZero,RoundToNearest,RoundToInf,RoundToNegInf} arbitrary types.
  //
  // Selects one of the 4 types according to the value of RoundStyle.
  //
  template<class RoundStyle,class RoundToZero,class RoundToNearest,class RoundToInf,class RoundToNegInf>
  struct for_round_style
  {
    typedef ct_switch4<RoundStyle
                       , round2zero_c, round2nearest_c, round2inf_c // round2neg_inf_c
                       , RoundToZero , RoundToNearest , RoundToInf , RoundToNegInf
                      > selector ;

    typedef typename selector::type type ;
  } ;


















//--------------------------------------------------------------------------
//                             Range Checking Logic.
//
// The range checking logic is built up by combining 1 or 2 predicates.
// Each predicate is encapsulated in a template class and exposes
// the static member function 'apply'.
//
//--------------------------------------------------------------------------


  // Because a particular logic can combine either 1 or two predicates, the following
  // tags are used to allow the predicate applier to receive 2 preds, but optimize away
  // one of them if it is 'non-applicable'
  struct non_applicable { typedef mpl::false_ do_apply ; } ;
  struct applicable     { typedef mpl::true_  do_apply ; } ;


  //--------------------------------------------------------------------------
  //
  //                      Range Checking Logic implementations.
  //
  // The following classes, collectivelly named 'Predicates', are instantiated within
  // the corresponding range checkers.
  // Their static member function 'apply' is called to perform the actual range checking logic.
  //--------------------------------------------------------------------------

    // s < Lowest(T) ? cNegOverflow : cInRange
    //
    template<class Traits>
    struct LT_LoT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s < static_cast<S>(bounds<T>::lowest()) ? cNegOverflow : cInRange ;
      }
    } ;

    // s < 0 ? cNegOverflow : cInRange
    //
    template<class Traits>
    struct LT_Zero : applicable
    {
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s < static_cast<S>(0) ? cNegOverflow : cInRange ;
      }
    } ;

    // s <= Lowest(T)-1 ? cNegOverflow : cInRange
    //
    template<class Traits>
    struct LE_PrevLoT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s <= static_cast<S>(bounds<T>::lowest()) - static_cast<S>(1.0)
                 ? cNegOverflow : cInRange ;
      }
    } ;

    // s < Lowest(T)-0.5 ? cNegOverflow : cInRange
    //
    template<class Traits>
    struct LT_HalfPrevLoT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s < static_cast<S>(bounds<T>::lowest()) - static_cast<S>(0.5)
                 ? cNegOverflow : cInRange ;
      }
    } ;

    // s > Highest(T) ? cPosOverflow : cInRange
    //
    template<class Traits>
    struct GT_HiT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s > static_cast<S>(bounds<T>::highest())
                 ? cPosOverflow : cInRange ;
      }
    } ;

    // s >= Lowest(T) + 1 ? cPosOverflow : cInRange
    //
    template<class Traits>
    struct GE_SuccHiT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s >= static_cast<S>(bounds<T>::highest()) + static_cast<S>(1.0)
                 ? cPosOverflow : cInRange ;
      }
    } ;

    // s >= Lowest(T) + 0.5 ? cPosgOverflow : cInRange
    //
    template<class Traits>
    struct GT_HalfSuccHiT : applicable
    {
      typedef typename Traits::target_type T ;
      typedef typename Traits::source_type S ;
      typedef typename Traits::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        return s >= static_cast<S>(bounds<T>::highest()) + static_cast<S>(0.5)
                 ? cPosOverflow : cInRange ;
      }
    } ;


  //--------------------------------------------------------------------------
  //
  // Predicate Combiner.
  //
  // This helper classes are used to possibly combine the range checking logic
  // individually performed by the predicates
  //
  //--------------------------------------------------------------------------


    // Applies both predicates: first 'PredA', and if it equals 'cInRange', 'PredB'
    template<class PredA, class PredB>
    struct applyBoth
    {
      typedef typename PredA::argument_type argument_type ;

      static range_check_result apply ( argument_type s )
      {
        range_check_result r = PredA::apply(s) ;
        if ( r == cInRange )
          r = PredB::apply(s);
        return r ;
      }
    } ;

    template<class PredA, class PredB>
    struct combine
    {
      typedef applyBoth<PredA,PredB> Both ;
      typedef void                   NNone ; // 'None' is defined as a macro in (/usr/X11R6/include/X11/X.h)

      typedef typename PredA::do_apply do_applyA ;
      typedef typename PredB::do_apply do_applyB ;

      typedef typename for_both<do_applyA, do_applyB, Both, PredA, PredB, NNone>::type type ;
    } ;












//--------------------------------------------------------------------------
//                             Range Checker classes.
//
// The following classes are VISIBLE base classes of the user-level converter<> class.
// They supply the optimized 'out_of_range()' and 'validate_range()' static member functions
// visible in the user interface.
//
//--------------------------------------------------------------------------

  // Dummy range checker.
  template<class Traits>
  struct dummy_range_checker
  {
    typedef typename Traits::argument_type argument_type ;

    static range_check_result out_of_range ( argument_type ) { return cInRange ; }
    static void validate_range ( argument_type ) {}
  } ;

  // Generic range checker.
  //
  // All the range checking logic for all possible combinations of source and target
  // can be arranged in terms of one or two predicates, which test overflow on both neg/pos 'sides'
  // of the ranges.
  //
  // These predicates are given here as IsNegOverflow and IsPosOverflow.
  //
  template<class Traits, class IsNegOverflow, class IsPosOverflow, class OverflowHandler>
  struct generic_range_checker
  {
    typedef OverflowHandler overflow_handler ;

    typedef typename Traits::argument_type argument_type ;

    static range_check_result out_of_range ( argument_type s )
    {
      typedef typename combine<IsNegOverflow,IsPosOverflow>::type Predicate ;

      return Predicate::apply(s);
    }

    static void validate_range ( argument_type s )
      { OverflowHandler()( out_of_range(s) ) ; }
  } ;



//--------------------------------------------------------------------------
//
// Selectors for the optimized Range Checker class.
//
//--------------------------------------------------------------------------

  template<class Traits,class OverflowHandler>
  struct GetRC_Sig2Sig_or_Unsig2Unsig
  {
    typedef dummy_range_checker<Traits> Dummy ;

    typedef LT_LoT<Traits> Pred1 ;
    typedef GT_HiT<Traits> Pred2 ;

    typedef generic_range_checker<Traits,Pred1,Pred2,OverflowHandler> Normal ;

    typedef typename Traits::subranged subranged ;

    typedef typename mpl::if_<subranged,Normal,Dummy>::type type ;
  } ;

  template<class Traits, class OverflowHandler>
  struct GetRC_Sig2Unsig
  {
    typedef LT_Zero<Traits> Pred1 ;
    typedef GT_HiT <Traits> Pred2 ;

    typedef generic_range_checker<Traits,Pred1,Pred2,OverflowHandler> ChoiceA ;

    typedef generic_range_checker<Traits,Pred1,non_applicable,OverflowHandler> ChoiceB ;

    typedef typename Traits::target_type T ;
    typedef typename Traits::source_type S ;

    typedef typename subranged_Unsig2Sig<S,T>::type oposite_subranged ;

    typedef typename mpl::not_<oposite_subranged>::type positively_subranged ;

    typedef typename mpl::if_<positively_subranged,ChoiceA,ChoiceB>::type type ;
  } ;

  template<class Traits, class OverflowHandler>
  struct GetRC_Unsig2Sig
  {
    typedef GT_HiT<Traits> Pred1 ;

    typedef generic_range_checker<Traits,non_applicable,Pred1,OverflowHandler> type ;
  } ;

  template<class Traits,class OverflowHandler>
  struct GetRC_Int2Int
  {
    typedef GetRC_Sig2Sig_or_Unsig2Unsig<Traits,OverflowHandler> Sig2SigQ     ;
    typedef GetRC_Sig2Unsig             <Traits,OverflowHandler> Sig2UnsigQ   ;
    typedef GetRC_Unsig2Sig             <Traits,OverflowHandler> Unsig2SigQ   ;
    typedef Sig2SigQ                                             Unsig2UnsigQ ;

    typedef typename Traits::sign_mixture sign_mixture ;

    typedef typename
      for_sign_mixture<sign_mixture,Sig2SigQ,Sig2UnsigQ,Unsig2SigQ,Unsig2UnsigQ>::type
        selector ;

    typedef typename selector::type type ;
  } ;

  template<class Traits>
  struct GetRC_Int2Float
  {
    typedef dummy_range_checker<Traits> type ;
  } ;

  template<class Traits, class OverflowHandler, class Float2IntRounder>
  struct GetRC_Float2Int
  {
    typedef LE_PrevLoT    <Traits> Pred1 ;
    typedef GE_SuccHiT    <Traits> Pred2 ;
    typedef LT_HalfPrevLoT<Traits> Pred3 ;
    typedef GT_HalfSuccHiT<Traits> Pred4 ;
    typedef GT_HiT        <Traits> Pred5 ;
    typedef LT_LoT        <Traits> Pred6 ;

    typedef generic_range_checker<Traits,Pred1,Pred2,OverflowHandler> ToZero    ;
    typedef generic_range_checker<Traits,Pred3,Pred4,OverflowHandler> ToNearest ;
    typedef generic_range_checker<Traits,Pred1,Pred5,OverflowHandler> ToInf     ;
    typedef generic_range_checker<Traits,Pred6,Pred2,OverflowHandler> ToNegInf  ;

    typedef typename Float2IntRounder::round_style round_style ;

    typedef typename for_round_style<round_style,ToZero,ToNearest,ToInf,ToNegInf>::type type ;
  } ;

  template<class Traits, class OverflowHandler>
  struct GetRC_Float2Float
  {
    typedef dummy_range_checker<Traits> Dummy ;

    typedef LT_LoT<Traits> Pred1 ;
    typedef GT_HiT<Traits> Pred2 ;

    typedef generic_range_checker<Traits,Pred1,Pred2,OverflowHandler> Normal ;

    typedef typename Traits::subranged subranged ;

    typedef typename mpl::if_<subranged,Normal,Dummy>::type type ;
  } ;

  template<class Traits, class OverflowHandler, class Float2IntRounder>
  struct GetRC_BuiltIn2BuiltIn
  {
    typedef GetRC_Int2Int<Traits,OverflowHandler>                    Int2IntQ ;
    typedef GetRC_Int2Float<Traits>                                  Int2FloatQ ;
    typedef GetRC_Float2Int<Traits,OverflowHandler,Float2IntRounder> Float2IntQ ;
    typedef GetRC_Float2Float<Traits,OverflowHandler>                Float2FloatQ ;

    typedef typename Traits::int_float_mixture int_float_mixture ;

    typedef typename for_int_float_mixture<int_float_mixture, Int2IntQ, Int2FloatQ, Float2IntQ, Float2FloatQ>::type selector ;

    typedef typename selector::type type ;
  } ;

  template<class Traits, class OverflowHandler, class Float2IntRounder>
  struct GetRC
  {
    typedef GetRC_BuiltIn2BuiltIn<Traits,OverflowHandler,Float2IntRounder> BuiltIn2BuiltInQ ;

    typedef dummy_range_checker<Traits> Dummy ;

    typedef mpl::identity<Dummy> DummyQ ;

    typedef typename Traits::udt_builtin_mixture udt_builtin_mixture ;

    typedef typename for_udt_builtin_mixture<udt_builtin_mixture,BuiltIn2BuiltInQ,DummyQ,DummyQ,DummyQ>::type selector ;

    typedef typename selector::type type ;
  } ;




//--------------------------------------------------------------------------
//                             Converter classes.
//
// The following classes are VISIBLE base classes of the user-level converter<> class.
// They supply the optimized 'nearbyint()' and 'convert()' static member functions
// visible in the user interface.
//
//--------------------------------------------------------------------------

  //
  // Trivial Converter : used when (cv-unqualified) T == (cv-unqualified)  S
  //
  template<class Traits>
  struct trivial_converter_impl : public dummy_range_checker<Traits>
  {
    typedef Traits traits ;
    
    typedef typename Traits::source_type   source_type   ;
    typedef typename Traits::argument_type argument_type ;
    typedef typename Traits::result_type   result_type   ;

    static result_type low_level_convert ( argument_type s ) { return s ; }
    static source_type nearbyint         ( argument_type s ) { return s ; }
    static result_type convert           ( argument_type s ) { return s ; }
  } ;


  //
  // Rounding Converter : used for float to integral conversions.
  //
  template<class Traits,class RangeChecker,class RawConverter,class Float2IntRounder>
  struct rounding_converter : public RangeChecker
                             ,public Float2IntRounder
                             ,public RawConverter
  {
    typedef RangeChecker     RangeCheckerBase ;
    typedef Float2IntRounder Float2IntRounderBase ;
    typedef RawConverter     RawConverterBase ;

    typedef Traits traits ;

    typedef typename Traits::source_type   source_type   ;
    typedef typename Traits::argument_type argument_type ;
    typedef typename Traits::result_type   result_type   ;

    static result_type convert ( argument_type s )
    {
      RangeCheckerBase::validate_range(s);
      source_type s1 = Float2IntRounderBase::nearbyint(s);
      return RawConverterBase::low_level_convert(s1);
    }
  } ;


  //
  // Non-Rounding Converter : used for all other conversions.
  //
  template<class Traits,class RangeChecker,class RawConverter>
  struct non_rounding_converter : public RangeChecker
                                 ,public RawConverter
  {
    typedef RangeChecker RangeCheckerBase ;
    typedef RawConverter RawConverterBase ;

    typedef Traits traits ;

    typedef typename Traits::source_type   source_type   ;
    typedef typename Traits::argument_type argument_type ;
    typedef typename Traits::result_type   result_type   ;

    static source_type nearbyint ( argument_type s ) { return s ; }

    static result_type convert ( argument_type s )
    {
      RangeCheckerBase::validate_range(s);
      return RawConverterBase::low_level_convert(s);
    }
  } ;




//--------------------------------------------------------------------------
//
// Selectors for the optimized Converter class.
//
//--------------------------------------------------------------------------

  template<class Traits,class OverflowHandler,class Float2IntRounder,class RawConverter, class UserRangeChecker>
  struct get_non_trivial_converter
  {
    typedef GetRC<Traits,OverflowHandler,Float2IntRounder> InternalRangeCheckerQ ;

    typedef is_same<UserRangeChecker,UseInternalRangeChecker> use_internal_RC ;

    typedef mpl::identity<UserRangeChecker> UserRangeCheckerQ ;

    typedef typename
      mpl::eval_if<use_internal_RC,InternalRangeCheckerQ,UserRangeCheckerQ>::type
        RangeChecker ;

    typedef non_rounding_converter<Traits,RangeChecker,RawConverter>              NonRounding ;
    typedef rounding_converter<Traits,RangeChecker,RawConverter,Float2IntRounder> Rounding ;

    typedef mpl::identity<NonRounding> NonRoundingQ ;
    typedef mpl::identity<Rounding>    RoundingQ    ;

    typedef typename Traits::int_float_mixture int_float_mixture ;

    typedef typename
      for_int_float_mixture<int_float_mixture, NonRoundingQ, NonRoundingQ, RoundingQ, NonRoundingQ>::type
        selector ;

    typedef typename selector::type type ;
  } ;

  template< class Traits
           ,class OverflowHandler
           ,class Float2IntRounder
           ,class RawConverter
           ,class UserRangeChecker
          >
  struct get_converter_impl
  {
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT( 0x0561 ) )
    // bcc55 prefers sometimes template parameters to be explicit local types.
    // (notice that is is illegal to reuse the names like this)
    typedef Traits           Traits ;
    typedef OverflowHandler  OverflowHandler ;
    typedef Float2IntRounder Float2IntRounder ;
    typedef RawConverter     RawConverter ;
    typedef UserRangeChecker UserRangeChecker ;
#endif

    typedef trivial_converter_impl<Traits> Trivial ;
    typedef mpl::identity        <Trivial> TrivialQ ;

    typedef get_non_trivial_converter< Traits
                                      ,OverflowHandler
                                      ,Float2IntRounder
                                      ,RawConverter
                                      ,UserRangeChecker
                                     > NonTrivialQ ;

    typedef typename Traits::trivial trivial ;

    typedef typename mpl::eval_if<trivial,TrivialQ,NonTrivialQ>::type type ;
  } ;

} } } // namespace boost::numeric::convdetail

#endif



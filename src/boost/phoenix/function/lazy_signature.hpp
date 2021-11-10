////////////////////////////////////////////////////////////////////////////
// lazy_signature.hpp
//
// Build signature structs for Phoenix equivalents for FC++
// which are located in lazy_prelude.hpp
//
// These are not direct equivalents of the Boost FC++ structs.
// This file has to be included after lazy_list.hpp
//
// Implemented so far:
//
//   RTEFH    == ReturnTypeEnumFromHelper    (used in enum_from, enum_from_to)
//   RTFD     == ReturnTypeFunctionDelay     (used in repeat)
//   RTFFX    == ReturnTypeFunctoidFwithX     (used in thunk1)
//   RTFFXY   == ReturnTypeFunctoidFwithXandY (used in thunk2)
//   RTFFXYZ  == ReturnTypeFunctoidFwithXandYandZ (used in thunk3)
//   RTF      == ReturnTypeF                  (used in ptr_to_fun0)
//   RTFX     == ReturnTypeFwithX          (used in ptr_to_fun, ptr_to_mem_fun)
//   RTFXY    == ReturnTypeFwithXandY      (used in ptr_to_fun, ptr_to_mem_fun)
//   RTFXYZ   == ReturnTypeFwithXandYandZ  (used in ptr_to_fun, ptr_to_mem_fun)
//   RTFWXYZ  == ReturnTypeFwithWandXandYandZ (used in ptr_to_fun)
//   RTFGHX   == ReturnTypeFandGandHwithX    (used in compose)
//   RTFGHXY  == ReturnTypeFandGandHwithXY   (used in compose)
//   RTFGHXYZ == ReturnTypeFandGandHwithXYZ  (used in compose)
//   RTFGX    == ReturnTypeFandGwithX        (used in compose)
//   RTFGXY   == ReturnTypeFandGwithXY       (used in compose)
//   RTFGXYZ  == ReturnTypeFandGwithXYZ      (used in compose)
//   RTFL     == ReturnTypeFunctionList      (used in map)
//   RTAB     == ReturnTypeListAListB        (used in zip)
//   RTZAB    == ReturnTypeZipListAListB     (used in zip_with)
//
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_FUNCTION_LAZY_SIGNATURE
#define BOOST_PHOENIX_FUNCTION_LAZY_SIGNATURE

namespace boost {

  namespace phoenix {

    namespace impl {

      //template <class T> struct remove_RC; in lazy_operator.hpp

      // RTEFH == ReturnTypeEnumFromHelper
      template <class T>
      struct RTEFH
      {
          typedef typename UseList::template List<T>::type LType;
          typedef typename result_of::ListType<LType>::
                           delay_result_type type;
      };

      // RTFD == ReturnTypeFunctionDelay (used in repeat)
      template <class T>
      struct RTFD {
          typedef typename remove_RC<T>::type TTT;
          typedef typename UseList::template List<TTT>::type LType;
          typedef typename result_of::ListType<LType>::
                           delay_result_type type;
      };


      //   RTFFX   == ReturnTypeFunctoidFwithX   (used in thunk1)
      template <class F,class X>
      struct RTFFX {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename boost::result_of<FType(XType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFFXY   == ReturnTypeFunctoidFwithXandY   (used in thunk2)
      template <class F,class X,class Y>
      struct RTFFXY {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename boost::result_of<FType(XType,YType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFFXYZ  == ReturnTypeFunctoidFwithXandYandZ  (used in thunk3)
      template <class F,class X,class Y,class Z>
      struct RTFFXYZ {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename remove_RC<Z>::type ZType;
          typedef typename boost::result_of<FType(XType,YType,ZType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTF   == ReturnTypeF     (used in ptr_to_fun0)
      template <class F>
      struct RTF {
          typedef typename remove_RC<F>::type FType;
          typedef typename boost::result_of<FType()>::type FR;
          typedef typename remove_RC<FR>::type RType;
          typedef RType type;
      };

      //   RTFX   == ReturnTypeFwithX     (used in ptr_to_fun)
      template <class F,class X>
      struct RTFX {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename boost::result_of<FType(XType)>::type FR;
          typedef typename remove_RC<FR>::type RType;
          typedef RType type;
      };

      //   RTFXY  == ReturnTypeFwithXandY     (used in ptr_to_fun)
      template <class F,class X,class Y>
      struct RTFXY {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename boost::result_of<FType(XType,YType)>::type FR;
          typedef typename remove_RC<FR>::type RType;
          typedef RType type;
      };

      //   RTFXYZ  == ReturnTypeFwithXandYandZ  (used in ptr_to_fun)
      template <class F,class X,class Y,class Z>
      struct RTFXYZ {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename remove_RC<Z>::type ZType;
          typedef typename boost::result_of<FType(XType,YType,ZType)>::type FR;
          typedef typename remove_RC<FR>::type RType;
          typedef RType type;
      };

      //   RTFWXYZ  == ReturnTypeFwithWandXandYandZ  (used in ptr_to_fun)
      template <class F,class W,class X,class Y,class Z>
      struct RTFWXYZ {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<W>::type WType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename remove_RC<Z>::type ZType;
          typedef typename boost::result_of<FType(WType,XType,YType,ZType)>::
                  type FR;
          typedef typename remove_RC<FR>::type RType;
          typedef RType type;
      };

      //   RTFGHX  == ReturnTypeFandGandHwithX (used in compose)
      template <class F,class G,class H,class X>
      struct RTFGHX {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<H>::type HType;
          typedef typename remove_RC<X>::type XType;
          typedef typename boost::result_of<GType(XType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<HType(XType)>::type HR;
          typedef typename boost::result_of<HR()>::type HRR;
          typedef typename remove_RC<HRR>::type HRType;
          typedef typename boost::result_of<FType(GRType,HRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFGHXY == ReturnTypeFandGandHwithXY (used in compose)
      template <class F,class G,class H,class X,class Y>
      struct RTFGHXY {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<H>::type HType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename boost::result_of<GType(XType,YType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<HType(XType,YType)>::type HR;
          typedef typename boost::result_of<HR()>::type HRR;
          typedef typename remove_RC<HRR>::type HRType;
          typedef typename boost::result_of<FType(GRType,HRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFGHXYZ == ReturnTypeFandGandHwithXYZ (used in compose)
      template <class F,class G,class H,class X,class Y,class Z>
      struct RTFGHXYZ {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<H>::type HType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename remove_RC<Z>::type ZType;
          typedef typename boost::result_of<GType(XType,YType,ZType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<HType(XType,YType,ZType)>::type HR;
          typedef typename boost::result_of<HR()>::type HRR;
          typedef typename remove_RC<HRR>::type HRType;
          typedef typename boost::result_of<FType(GRType,HRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFGX   == ReturnTypeFandGwithX     (used in compose)
      template <class F,class G,class X>
      struct RTFGX {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<X>::type XType;
          typedef typename boost::result_of<GType(XType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<FType(GRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFGXY  == ReturnTypeFandGwithXY    (used in compose)
      template <class F,class G,class X,class Y>
      struct RTFGXY {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename boost::result_of<GType(XType,YType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<FType(GRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      //   RTFGXYZ == ReturnTypeFandGwithXYZ   (used in compose)
      template <class F,class G,class X,class Y,class Z>
      struct RTFGXYZ {
          typedef typename remove_RC<F>::type FType;
          typedef typename remove_RC<G>::type GType;
          typedef typename remove_RC<X>::type XType;
          typedef typename remove_RC<Y>::type YType;
          typedef typename remove_RC<Z>::type ZType;
          typedef typename boost::result_of<GType(XType,YType,ZType)>::type GR;
          typedef typename boost::result_of<GR()>::type GRR;
          typedef typename remove_RC<GRR>::type GRType;
          typedef typename boost::result_of<FType(GRType)>::type FR;
          typedef typename boost::result_of<FR()>::type RR;
          typedef typename remove_RC<RR>::type RType;
          typedef RType type;
      };

      // This is the way to make the return type for
      // map(f,l). It is used four times in the code
      // so I have made it a separate struct.
      // RTFL == ReturnTypeFunctionList
      template <class F,class L>
      struct RTFL {
           typedef typename remove_RC<F>::type Ftype;
           typedef typename result_of::ListType<L>::tail_result_type
                                   Ttype;
           typedef typename result_of::ListType<L>::value_type Vtype;
           // NOTE: FR is the type of the functor.
           typedef typename boost::result_of<Ftype(Vtype)>::type FR;
           // NOTE: RR is the type returned, which then needs
           // reference and const removal.
           typedef typename boost::result_of<FR()>::type RR;
           typedef typename remove_RC<RR>::type Rtype;
           typedef typename boost::remove_reference<L>::type LL;
           typedef typename LL::template cons_rebind<Rtype>::delay_type
                   type;
      };

      // RTAB == ReturnTypeListAListB
      template <typename LA, typename LB>
      struct RTAB {
                  typedef typename result_of::ListType<LA>::tail_result_type
                                   LAtype;
                  typedef typename result_of::ListType<LB>::tail_result_type
                                   LBtype;
                  typedef typename result_of::ListType<LA>::value_type VAA;
                  typedef typename boost::remove_const<VAA>::type VAtype;
                  typedef typename result_of::ListType<LB>::value_type VBB;
                  typedef typename boost::remove_const<VBB>::type VBtype;
                  typedef typename boost::result_of<Make_pair(VAtype,VBtype)>
                                  ::type FR;
                  typedef typename boost::result_of<FR()>::type RR;
                  typedef typename remove_RC<RR>::type Rtype;
                  typedef typename boost::remove_reference<LA>::type LLA;
                  typedef typename LLA::template cons_rebind<Rtype>::type type;
      };


      // RTZAB == ReturnTypeZipListAListB
      template <typename Z, typename LA, typename LB>
      struct RTZAB {
                  typedef typename remove_RC<Z>::type Ztype;
                  typedef typename result_of::ListType<LA>::tail_result_type
                                   LAtype;
                  typedef typename result_of::ListType<LB>::tail_result_type
                                   LBtype;
                  typedef typename result_of::ListType<LA>::value_type VAtype;
                  typedef typename result_of::ListType<LB>::value_type VBtype;
                  typedef typename boost::result_of<Ztype(VAtype,VBtype)>::type
                                   FR;
                  typedef typename boost::result_of<FR()>::type RR;
                  typedef typename remove_RC<RR>::type Rtype;
                  typedef typename boost::remove_reference<LA>::type LLA;
                  typedef typename LLA::template cons_rebind<Rtype>::type type;
      };

    }
  }
}

#endif

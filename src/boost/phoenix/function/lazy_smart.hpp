////////////////////////////////////////////////////////////////////////////
// lazy smart.hpp
//
// Build lazy functoid traits for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoid traits in smart.hpp
//
// I have copied the versions for zero, one, two and three arguments.
//
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_FUNCTION_LAZY_SMART
#define BOOST_PHOENIX_FUNCTION_LAZY_SMART

namespace boost {
    namespace phoenix {
      namespace fcpp {

//////////////////////////////////////////////////////////////////////
// Feature: Smartness
//////////////////////////////////////////////////////////////////////
// If F is smart, then we can refer to these entities:
//
//    functoid_traits<F>::template accepts<N>::args
//       A bool which says whether F can accept N arguments
//
//    functoid_traits<F>::max_args
//       An int which says what the most arguments F can accept is
//
//    functoid_traits<F>::template ensure_accepts<N>::args()
//       A no-op call that compiles only if F can accept N args
//
// We use traits so that if you happen to ask a non-smart functoid these
// questions, you will hopefully get a literate error message.

struct SmartFunctoid {};

// We add crazy identifiers to ensure that users don't accidentally talk
// to functoids directly; they should always be going through the traits
// class to ask for info.
struct smart_functoid0 : public SmartFunctoid {
   template <class Dummy, int i> struct crazy_accepts {
      static const bool args = false;
   };
   template <class Dummy> struct crazy_accepts<Dummy,0> {
      static const bool args = true;
   };
   static const int crazy_max_args = 0;
};

struct smart_functoid1 : public SmartFunctoid {
   template <class Dummy, int i> struct crazy_accepts {
      static const bool args = false;
   };
   template <class Dummy> struct crazy_accepts<Dummy,1> {
      static const bool args = true;
   };
   static const int crazy_max_args = 1;
};

struct smart_functoid2 : public SmartFunctoid {
   template <class Dummy, int i> struct crazy_accepts {
      static const bool args = false;
   };
   template <class Dummy> struct crazy_accepts<Dummy,1> {
      static const bool args = true;
   };
   template <class Dummy> struct crazy_accepts<Dummy,2> {
      static const bool args = true;
   };
   static const int crazy_max_args = 2;
};

struct smart_functoid3 : public SmartFunctoid {
   template <class Dummy, int i> struct crazy_accepts {
      static const bool args = false;
   };
   template <class Dummy> struct crazy_accepts<Dummy,1> {
      static const bool args = true;
   };
   template <class Dummy> struct crazy_accepts<Dummy,2> {
      static const bool args = true;
   };
   template <class Dummy> struct crazy_accepts<Dummy,3> {
      static const bool args = true;
   };
   static const int crazy_max_args = 3;
};


namespace impl {
   template <class F, bool b> struct NeededASmartFunctoidButInsteadGot {};
   template <class F> struct NeededASmartFunctoidButInsteadGot<F,true> {
      typedef F type;
   };
   template <bool b> struct Ensure;
   template <> struct Ensure<true> {};
} // end namespace impl

template <class MaybeASmartFunctoid>
struct functoid_traits {
  typedef typename boost::remove_reference<MaybeASmartFunctoid>::type MaybeASmartFunctoidT;
   typedef
      typename impl::NeededASmartFunctoidButInsteadGot<MaybeASmartFunctoidT,
         boost::is_base_and_derived<SmartFunctoid,
         MaybeASmartFunctoidT>::value>::type F;
      template <int i> struct accepts {
      static const bool args = F::template crazy_accepts<int,i>::args;
   };
   template <int i> struct ensure_accepts {
      static const bool ok = F::template crazy_accepts<int,i>::args;
      inline static void args() { (void) impl::Ensure<ok>(); }
   };
   static const int max_args = F::crazy_max_args;
};

// These can be used to make functoids smart without having to alter
// code elsewhere. These are used instead of boost::phoenix::function
// to declare the object.
template <typename F>
struct smart_function0 : public smart_functoid0,
                         public boost::phoenix::function<F>
{ };

template <typename F>
struct smart_function1 : public smart_functoid1,
                         public boost::phoenix::function<F>
{
  typedef F type;
};

template <typename F>
struct smart_function2 : public smart_functoid2,
                         public boost::phoenix::function<F>
{ };

template <typename F>
struct smart_function3 : public smart_functoid3,
                         public boost::phoenix::function<F>
{ };
      }
    }
}


#endif

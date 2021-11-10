////////////////////////////////////////////////////////////////////////////
// lazy prelude.hpp
//
// Build lazy operations for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoids in prelude.hpp
//
// Usage:  All of these are functors which need various numbers of arguments.
//         Those can be supplied as real arguments or as Phoenix arguments.
//         Execution will happen when all the arguments are supplied.
// e.g.
//         take(2,list)()  or  take(2,arg1)(list)
//
// Implemented so far:
//
// id (moved back to operators.hpp)
//
// A lot of what comes here uses the list type, so that will be needed first.
//
// Now that list<T> is available I can start to build things here.
//
//
// until(pred,f,start)         - if pred(start) is true, return start
//                               apply value = f(start)
//                               apply value = f(value)
//                               until pred(value) is true
//                               return value
//
// The predicate argument pred must be a lazy function taking one argument
// and returning bool.
// This can be a lazy function with an argument already.
// This has to be declared before the call to until.
// The type can be declated using Predicate as in this example:
//
//    Predicate<int>::type f(greater(arg1,10));
//    std::cout << until(f, inc, 1)() << std::endl;
//
// until2(pred,f,start,value2) - if pred(start,value2) is true, return start
//                               apply value1 = f(start)
//                               apply value1 = f(value1)
//                               until pred(value1,value2) is true
//                               return value1
//
// NOTE: until2 has been defined because this code does not support
//       FC++ currying, so that a partial function cannot be passed
//       as an argument. This provides a way of passing a second parameter.
//       There is now the option to use Predicate<T> as shown above.
//
// odd(n)     true if n is odd
// even(n)    true if n is even
//
// last(list)
// all_but_last(list)
// at(list,n)
// length(list)
// filter(pred,list)
// iterate(function,value)
// repeat(value)
// take(n,list)
// drop(n,list)
// enum_from(x)
// enum_from_to(x,y)
//
////////////////////////////////////////////////////////////////////////////
// Interdependence:
// The old Boost FC++ has a set of headers which interelate and call each
// other in a complicated way. I am going to document the interdependence
// of the files here. I will then make sure that they are called correctly
// starting from this file. John Fletcher. February 2015.
////////////////////////////////////////////////////////////////////////////
// BoostFC++ header sequence:
// 
// prelude.hpp   -> list.hpp (optinally monad.hpp at end)
// list.hpp      -> reuse.hpp
// reuse.hpp     -> function.hpp
// function.hpp  -> ref_count.hpp operator.hpp
// ref_count.hpp -> config.hpp boost headers and RefCountType definition
// operator.hpp  -> lambda.hpp
// lambda.hpp    -> full.hpp (use of lambda internals is optional)
// full.hpp      -> smart.hpp curry.hpp pre_lambda.hpp (optionally full4.hpp)
// smart.hpp     -> signature.hpp
// curry.hpp     -> signature.hpp
// signature.hpp -> config.hpp
//
////////////////////////////////////////////////////////////////////////////
// Proposed order in lazy_prelude.hpp
// on the basis that files need what they call.
//
// lazy_config.hpp    (If needed)* probably not needed.
// lazy_signature.hpp (If needed)*
// lazy_smart.hpp     (If needed)*
// lazy_curry.hpp     (If needed)*
// lazy_full.hpp      (If needed)*
// lazy_operator.hpp  (absorb definition of RefCountType)
// lazy_function.hpp  (may not now be needed)
// lazy_reuse.hpp     (implemented without use of FC++ functions)
// lazy_list.hpp
//
// * file does not yet exist.
////////////////////////////////////////////////////////////////////////////
// This is implemented such that no other lazy_ file calls other lazy_ files.
// They do call their own external files, which may well be duplicates.
// That can be sorted out later.
////////////////////////////////////////////////////////////////////////////
// Notes: full and curry operations should be covered by Phoenix.
// The lambda operations are quite different from Phoenix lambda
// and will be omitted.
// The implementation monad can be postponed.
// Some of function and reuse are needed for the list type.
// I will review later whether they are part of the external interface.
//
// John Fletcher  February 2015.
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/


#ifndef BOOST_PHOENIX_FUNCTION_LAZY_PRELUDE
#define BOOST_PHOENIX_FUNCTION_LAZY_PRELUDE

#include <exception>
#include <vector>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/scope.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/function/lazy_operator.hpp>
#include <boost/phoenix/function/lazy_reuse.hpp>
#include <boost/phoenix/function/lazy_list.hpp>

////////////////////////////////////////////////////////////////////////////
// To come here, the Haskell Prelude things which need list<T>.
// Things which do not need list<T> are in lazy_operator.hpp.
////////////////////////////////////////////////////////////////////////////

namespace boost {

  namespace phoenix {

    // These are in fcpp namespace as they introduce an FC++ style.
    namespace fcpp {

    template <typename T>
    struct Predicate {
        typedef typename boost::function1<bool,T> fun1_bool_T;
        typedef typename boost::phoenix::function<fun1_bool_T> bool_F_T;
        typedef bool_F_T type;
    };

    template <typename R>
    struct Function0 {
        typedef typename boost::function0<R> fun0_R;
        typedef typename boost::phoenix::function<fun0_R> R_F;
        typedef R_F type;
    };

    template <typename R,typename A0>
    struct Function1 {
        typedef typename boost::function1<R,A0> fun1_R_A0;
        typedef typename boost::phoenix::function<fun1_R_A0> R_F_A0;
        typedef R_F_A0 type;
    };

    template <typename R, typename A0, typename A1>
    struct Function2 {
      typedef typename boost::function2<R,A0,A1> fun2_R_A0_A1;
        typedef typename boost::phoenix::function<fun2_R_A0_A1> R_F_A0_A1;
        typedef R_F_A0_A1 type;
    };

    }

    namespace impl {
      using fcpp::INV;
      using fcpp::VAR;
      using fcpp::reuser1;
      using fcpp::reuser2;
      using fcpp::reuser3;
      using boost::phoenix::arg_names::arg1;

         struct Pow {

            template <typename Sig>
            struct result;

            template <typename This, typename N, typename A0>
            struct result<This(N,A0)>
               : boost::remove_reference<A0>
            {};

            template <typename N, typename A0>
            A0 operator()(N n, const A0 & a0,
            reuser2<INV,VAR,INV,Pow,N,A0> r = NIL ) const {
              if ( n <= 0 )
                 return A0(1);
              else if ( n==1 )
                 return a0;
              else {
                A0 a1 = r( Pow(), n-1, a0)();
                return a0*a1;
              }
            }

         };

         struct Apply {

            template <typename Sig>
            struct result;

            template <typename This, typename N, typename F,typename A0>
            struct result<This(N,F,A0)>
               : boost::remove_reference<A0>
            {};

            template <typename N, typename F, typename A0>
            A0 operator()(N n, const F &f, const A0 & a0,
            reuser3<INV,VAR,INV,INV,Apply,N,F,A0> r = NIL ) const {
              if ( n <= 0 )
                 return a0;
              else if ( n==1 )
                 return f(arg1)(a0);
              else {
                A0 a1 = r( Apply(), n-1, f, a0)();
                return f(a1)();
              }
            }

         };

         struct Odd {
            template <typename Sig>
            struct result;

            template <typename This, typename T>
            struct result<This(T)>
            {
              typedef bool type;
            };

            template <class T>
            typename result<Odd(T)>::type operator()( const T& x ) const {
               return x%2==1;
            }
         };

         struct Even {
            template <typename Sig>
            struct result;

            template <typename This, typename T>
            struct result<This(T)>
            {
              typedef bool type;
            };

            template <class T>
            typename result<Even(T)>::type operator()( const T& x ) const {
               return x%2==0;
            }
         };

    }
    typedef boost::phoenix::function<impl::Pow>   Pow;
    typedef boost::phoenix::function<impl::Apply> Apply;
    typedef boost::phoenix::function<impl::Odd>   Odd;
    typedef boost::phoenix::function<impl::Even>  Even;
    Pow   pow;
    Apply apply;
    Odd   odd;
    Even  even;

    namespace impl {
      using fcpp::INV;
      using fcpp::VAR;
      using fcpp::reuser1;
      using fcpp::reuser2;
      using fcpp::reuser3;
      using boost::phoenix::arg_names::arg1;

      // I cannot yet do currying to pass e.g. greater(9,arg1)
      // as a function. This can be done using Predicate<T>::type.
         struct Until {

             template <typename Sig> struct result;

             template <typename This, typename Pred, typename Unary, typename T>
             struct result<This(Pred,Unary,T)>
                : boost::remove_reference<T> {};

             template <class Pred, class Unary, class T>
             T operator()( const Pred& p,const Unary& op,const T &start) const
             {
               T tmp = start;
               while( !p(tmp)() ) {
                 tmp = apply(1,op,tmp)();
               }
                return tmp;
             }

          };

          struct Until2 {

             template <typename Sig> struct result;

             template <typename This, typename Binary, typename Unary,
                       typename T, typename X>
             struct result<This(Binary,Unary,T,X)>
                : boost::remove_reference<T> {};

             template <class Binary, class Unary, class T, class X>
             typename result<Until2(Binary,Unary,T,X)>::type
             operator()( const Binary& p, const Unary& op, const T & start,
                        const X & check ) const
             {
               T tmp1 = start;
               T tmp2;
               while( !p(tmp1,check)() ) {
                 tmp2 = apply(1,op,tmp1)();
                 tmp1 = tmp2;
                 
               }
               return tmp1;
             }
          };

          struct Last {
             template <typename Sig> struct result;

             template <typename This, typename L>
             struct result<This(L)>
             {
               typedef typename result_of::ListType<L>::value_type type;
             };

             template <class L>
             typename result<Last(L)>::type
             operator()( const L& ll ) const {
               size_t x = 0;
               typename result_of::ListType<L>::delay_result_type l = delay(ll);
               while( !null( tail(l)() )() ) {
                 l = tail(l)();
                 ++x;
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
                 if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                   break;
#endif
               }
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
                 if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                     throw lazy_exception("Your list is too long!!");
#endif
                 return head(l)();
             }
          };

          struct Init {

             template <typename Sig> struct result;

             template <typename This, typename L>
             struct result<This(L)>
             {
               typedef typename result_of::ListType<L>::force_result_type type;
             };

             template <class L>
             typename result<Init(L)>::type
             operator()( const L& l,
                         reuser1<INV,VAR,Init,
                         typename result_of::ListType<L>::delay_result_type>
                         r = NIL ) const {
               if( null( tail( l )() )() )
                   return NIL;
               else
                   return cons( head(l)(), r( Init(), tail(l)() )() )();
               }
          };

          struct Length {
            template <typename Sig> struct result;

            template <typename This, typename L>
            struct result<This(L)>
            {
               typedef size_t type;
            };

            template <class L>
            size_t operator()( const L& ll ) const {
              typename L::delay_result_type l = delay(ll);
              size_t x = 0;
              while( !null(l)() ) {
                  l = tail(l);
                  ++x;
                  if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                     break;
              }
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
              if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                   throw lazy_exception("Your list is too long!!");
#endif
              return x;
            }
          };

          // at is Haskell's operator (!!)
          // This is zero indexed.  at(l,0)() returns the first element.
          struct At {
            template <typename Sig> struct result;

            template <typename This, typename L, typename N>
            struct result<This(L,N)>
            {
               typedef typename result_of::ListType<L>::value_type type;
            };

              template <class L>
              typename result<At(L,size_t)>::type
              operator()( L l, size_t n ) const {
                  while( n!=0 ) {
                      l = tail(l);
                      --n;
                  }
                  return head(l)();
              }
          };

         template <class P,class L>
         struct FilterH
          {
              P p;
              L l;
              FilterH( const P& pp, const L& ll) : p(pp), l(ll) {}
              template <typename Sig> struct result;

              template <typename This, class PP, class LL>
              struct result<This(PP,LL)>
              {
                typedef typename boost::phoenix::result_of::
                        ListType<LL>::delay_result_type type;
              };
                typename result<FilterH(P,L)>::type operator()() const {
                typedef typename result_of::ListType<L>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> Fun2_R_P_L;
                typedef boost::phoenix::function<Fun2_R_P_L> FilterH_R_P_L;
                if (null(l)() )
                   return NIL;
                Fun2_R_P_L fun2_R_P_L = FilterH<P,L>(p,tail(l));
                FilterH_R_P_L filterh_R_P_L(fun2_R_P_L);
                if( p(head(l))() )
                   return cons( head(l)(), filterh_R_P_L() );
                else
                   return filterh_R_P_L();
              }
          };

          struct Filter {
            template <typename Sig> struct result;

                template <typename This, typename P, typename L>
                struct result<This(P,L)>
                {
                  typedef typename result_of::ListType<L>::delay_result_type
                          type;
                };

                template <class P, class L>
                typename result<Filter(P,L)>::type
                operator()( const P& p, const L& ll) const
                {
                     typename  result_of::ListType<L>::delay_result_type
                     l = delay(ll);
                     typedef typename result_of::ListType<L>::
                           delay_result_type result_type;
                     typedef boost::function0<result_type> Fun2_R_P_L;
                     typedef boost::phoenix::function<Fun2_R_P_L> FilterH_R_P_L;
                     Fun2_R_P_L fun2_R_P_L = FilterH<P,L>(p,l);
                     FilterH_R_P_L filterh_R_P_L(fun2_R_P_L);
                     return filterh_R_P_L();
                }
          };

         template <class F,class T>
         struct IterateH
          {
              F f;
              T t;
              IterateH( const F& ff, const T& tt) : f(ff), t(tt) {}
              template <typename Sig> struct result;

              template <typename This,class F2,class T2>
              struct result<This(F2,T2)>
              {
                typedef typename boost::remove_reference<T2>::type TT;
                typedef typename boost::remove_const<TT>::type TTT;
                typedef typename UseList::template List<TTT>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type type;
              };

                typename result<IterateH(F,T)>::type operator()() const {
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> Fun2_R_F_T;
                typedef boost::phoenix::function<Fun2_R_F_T> IterateH_R_F_T;
                Fun2_R_F_T fun2_R_F_T = IterateH<F,T>(f,f(t)());
                IterateH_R_F_T iterateh_R_F_T(fun2_R_F_T);
                   return cons( t, iterateh_R_F_T() );
              }
          };


          struct Iterate {
   // Note: this does always return an odd_list; iterate() takes no ListLike
   // parameter, and it requires that its result be lazy.
              template <typename Sig> struct result;

              template <typename This, typename F, typename T>
              struct result<This(F,T)>
              {
                typedef typename boost::remove_reference<T>::type TT;
                typedef typename boost::remove_const<TT>::type TTT;
                typedef typename UseList::template List<TTT>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type type;
              };

              template <class F, class T>
                typename result<Iterate(F,T)>::type operator()
                (const F& f, const T& t) const {
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> Fun2_R_F_T;
                typedef boost::phoenix::function<Fun2_R_F_T> IterateH_R_F_T;
                Fun2_R_F_T fun2_R_F_T = IterateH<F,T>(f,f(t)());
                IterateH_R_F_T iterateh_R_F_T(fun2_R_F_T);
                   return iterateh_R_F_T();
              }
          };

    }

    typedef boost::phoenix::function<impl::Until> Until;
    typedef boost::phoenix::function<impl::Until2> Until2;
    typedef boost::phoenix::function<impl::Last>  Last;
    typedef boost::phoenix::function<impl::Init>  Init;
    typedef boost::phoenix::function<impl::Length> Length;
    typedef boost::phoenix::function<impl::At>    At;
    typedef boost::phoenix::function<impl::Filter> Filter;
    typedef boost::phoenix::function<impl::Iterate> Iterate;
    Until until;
    Until2 until2;
    Last  last;
    Init  all_but_last;  // renamed from init which is not available.
    Length length;
    At at_;  //Renamed from at.
    Filter filter;
    Iterate iterate;

    namespace impl {

          struct Repeat {
         // See note for iterate()
              template <typename Sig> struct result;

              template <typename This, typename T>
              struct result<This(T)>
              {
                typedef typename boost::remove_reference<T>::type TT;
                typedef typename boost::remove_const<TT>::type TTT;
                typedef typename UseList::template List<TTT>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type type;
              };

              template <class T>
              typename result<Repeat(T)>::type operator()( const T& x) const
              {
                return iterate(id,x);
              }
          };

          struct Take {

             template <typename Sig> struct result;

             template <typename This, typename N, typename L>
             struct result<This(N,L)>
             {
               typedef typename result_of::ListType<L>::force_result_type type;
             };

             template <class N,class L>
             typename result<Take(N,L)>::type
             operator()( N n, const L& l,
               reuser2<INV,VAR,VAR,Take,N,
               typename result_of::ListType<L>::force_result_type>
               r = NIL
             ) const {
               if( n <= 0 || null(l)() )
                 return NIL;
               else {
                 return cons( head(l)(), r( Take(), n-1, tail(l)() )() )();
               }
             }
          };

          struct Drop {
             template <typename Sig> struct result;

             template <typename This, typename Dummy, typename L>
             struct result<This(Dummy,L)>
             {
               typedef typename result_of::ListType<L>::delay_result_type type;
             };
   
             template <class L>
             typename result<Drop(size_t,L)>::type
             operator()( size_t n, const L& ll ) const {
               typename L::delay_result_type l = delay(ll);
               while( n!=0 && !null(l)() ) {
                 --n;
                 l = tail(l)();
               }
               return l;
             }
          };
 
          template <class T>
          struct EFH
          {
              mutable T x;
              EFH( const T& xx) : x(xx) {}
              template <typename Sig> struct result;

              template <typename This, class TT>
              struct result<This(TT)>
              {
                typedef typename boost::phoenix::UseList::template
                        List<TT>::type LType;
                typedef typename boost::phoenix::result_of::
                        ListType<LType>::delay_result_type type;
              };
              typename result<EFH(T)>::type operator()() const {
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> fun1_R_TTT;
                //std::cout << "EFH (" << x << ")" << std::endl;
                ++x;
                fun1_R_TTT efh_R_TTT = EFH<T>(x);
                typedef boost::phoenix::function<fun1_R_TTT> EFH_R_T;
                EFH_R_T efh_R_T(efh_R_TTT);
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
                if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                     throw lazy_exception("Running away in EFH!!");
#endif
                return cons( x-1, efh_R_T() );
              }
          };

          struct Enum_from {
             template <typename Sig> struct result;

             template <typename This, typename T>
             struct result<This(T)>
             {
               typedef typename boost::remove_reference<T>::type TT;
               typedef typename boost::remove_const<TT>::type TTT;
               typedef typename UseList::template List<TTT>::type LType;
               typedef typename result_of::ListType<LType>::
                       delay_result_type type;
             };

             template <class T>
             typename result<Enum_from(T)>::type operator()
                (const T & x) const
              {
                typedef typename boost::remove_reference<T>::type TT;
                typedef typename boost::remove_const<TT>::type TTT;
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> fun1_R_TTT;
                fun1_R_TTT efh_R_TTT = EFH<TTT>(x);
                typedef boost::phoenix::function<fun1_R_TTT> EFH_R_T;
                EFH_R_T efh_R_T(efh_R_TTT);
                //std::cout << "enum_from (" << x << ")" << std::endl;
                return efh_R_T();
              }
          };

       template <class T>
         struct EFTH
          {
              mutable T x;
              T y;
              EFTH( const T& xx, const T& yy) : x(xx), y(yy) {}
              template <typename Sig> struct result;

              template <typename This, class TT>
              struct result<This(TT)>
              {
                typedef typename boost::phoenix::UseList::template
                        List<TT>::type LType;
                typedef typename boost::phoenix::result_of::
                        ListType<LType>::delay_result_type type;
              };
              typename result<EFTH(T)>::type operator()() const {
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> fun1_R_TTT;
                //std::cout << "EFTH (" << x << ")" << std::endl;
                if (x > y ) return NIL;
                ++x;
                fun1_R_TTT efth_R_TTT = EFTH<T>(x,y);
                typedef boost::phoenix::function<fun1_R_TTT> EFTH_R_T;
                EFTH_R_T efth_R_T(efth_R_TTT);
#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
                if (x > BOOST_PHOENIX_FUNCTION_MAX_LAZY_LIST_LENGTH)
                     throw lazy_exception("Running away in EFTH!!");
#endif
                return cons( x-1, efth_R_T() );
              }
          };

          struct Enum_from_to {
             template <typename Sig> struct result;

             template <typename This, typename T>
             struct result<This(T,T)>
             {
               typedef typename boost::remove_reference<T>::type TT;
               typedef typename boost::remove_const<TT>::type TTT;
               typedef typename UseList::template List<TTT>::type LType;
               typedef typename result_of::ListType<LType>::
                       delay_result_type type;
             };

             template <class T>
             typename result<Enum_from(T,T)>::type operator()
             (const T & x, const T & y) const
              {
                typedef typename boost::remove_reference<T>::type TT;
                typedef typename boost::remove_const<TT>::type TTT;
                typedef typename UseList::template List<T>::type LType;
                typedef typename result_of::ListType<LType>::
                        delay_result_type result_type;
                typedef boost::function0<result_type> fun1_R_TTT;
                fun1_R_TTT efth_R_TTT = EFTH<TTT>(x,y);
                typedef boost::phoenix::function<fun1_R_TTT> EFTH_R_T;
                EFTH_R_T efth_R_T(efth_R_TTT);
                //std::cout << "enum_from (" << x << ")" << std::endl;
                return efth_R_T();
              }
          };

    }


    //BOOST_PHOENIX_ADAPT_CALLABLE(apply, impl::apply, 3)
    // Functors to be used in reuser will have to be defined
    // using boost::phoenix::function directly
    // in order to be able to be used as arguments.
    typedef boost::phoenix::function<impl::Repeat> Repeat;
    typedef boost::phoenix::function<impl::Take>  Take;
    typedef boost::phoenix::function<impl::Drop>  Drop;
    typedef boost::phoenix::function<impl::Enum_from>     Enum_from;
    typedef boost::phoenix::function<impl::Enum_from_to>  Enum_from_to;
    Repeat repeat;
    Take  take;
    Drop  drop;
    Enum_from enum_from;
    Enum_from_to enum_from_to;

    namespace fcpp {


    }

  }

}


#endif

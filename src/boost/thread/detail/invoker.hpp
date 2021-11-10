// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2013/04 Vicente J. Botet Escriba
//    Provide implementation up to 9 parameters when BOOST_NO_CXX11_VARIADIC_TEMPLATES is defined.
//    Make use of Boost.Move
//    Make use of Boost.Tuple (movable)
// 2012/11 Vicente J. Botet Escriba
//    Adapt to boost libc++ implementation

//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
// The invoker code is based on the one from libcxx.
//===----------------------------------------------------------------------===//

#ifndef BOOST_THREAD_DETAIL_INVOKER_HPP
#define BOOST_THREAD_DETAIL_INVOKER_HPP

#include <boost/config.hpp>

#include <boost/utility/result_of.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoke.hpp>
#include <boost/thread/detail/make_tuple_indices.hpp>
#include <boost/thread/csbl/tuple.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/thread/detail/variadic_header.hpp>

namespace boost
{
  namespace detail
  {

#if defined(BOOST_THREAD_PROVIDES_INVOKE) && ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && ! defined(BOOST_NO_CXX11_HDR_TUPLE)

    template <class Fp, class ... Args>
    class invoker
    {
      //typedef typename decay<Fp>::type Fpd;
      //typedef tuple<typename decay<Args>::type...> Argsd;

      //csbl::tuple<Fpd, Argsd...> f_;
      csbl::tuple<Fp, Args...> f_;

    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE( invoker)
      //typedef typename invoke_of<_Fp, _Args...>::type Rp;
      typedef typename result_of<Fp(Args...)>::type result_type;

      template <class F, class ... As>
      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(As)... args)
      : f_(boost::forward<F>(f), boost::forward<As>(args)...)
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f) : f_(boost::move(BOOST_THREAD_RV(f).f_))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker( const invoker& f) : f_(f.f_)
      {}

      BOOST_SYMBOL_VISIBLE
      invoker& operator=(BOOST_THREAD_RV_REF(invoker) f)
      {
        if (this != &f)
        {
          f_ = boost::move(BOOST_THREAD_RV(f).f_);
        }
        return *this;
      }

      BOOST_SYMBOL_VISIBLE
      invoker& operator=( BOOST_THREAD_COPY_ASSIGN_REF(invoker) f)
      {
        if (this != &f)
        {
          f_ = f.f_;
        }
        return *this;
      }

      result_type operator()()
      {
        typedef typename make_tuple_indices<1+sizeof...(Args), 1>::type Index;
        return execute(Index());
      }
    private:
      template <size_t ...Indices>
      result_type
      execute(tuple_indices<Indices...>)
      {
        return detail::invoke(boost::move(csbl::get<0>(f_)), boost::move(csbl::get<Indices>(f_))...);
      }
    };

    template <class R, class Fp, class ... Args>
    class invoker_ret
    {
      //typedef typename decay<Fp>::type Fpd;
      //typedef tuple<typename decay<Args>::type...> Argsd;

      //csbl::tuple<Fpd, Argsd...> f_;
      csbl::tuple<Fp, Args...> f_;

    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE( invoker_ret)
      typedef R result_type;

      template <class F, class ... As>
      BOOST_SYMBOL_VISIBLE
      explicit invoker_ret(BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(As)... args)
      : f_(boost::forward<F>(f), boost::forward<As>(args)...)
      {}

      BOOST_SYMBOL_VISIBLE
      invoker_ret(BOOST_THREAD_RV_REF(invoker_ret) f) : f_(boost::move(BOOST_THREAD_RV(f).f_))
      {}

      result_type operator()()
      {
        typedef typename make_tuple_indices<1+sizeof...(Args), 1>::type Index;
        return execute(Index());
      }
    private:
      template <size_t ...Indices>
      result_type
      execute(tuple_indices<Indices...>)
      {
        return detail::invoke<R>(boost::move(csbl::get<0>(f_)), boost::move(csbl::get<Indices>(f_))...);
      }
    };
  //BOOST_THREAD_DCL_MOVABLE_BEG(X) invoker<Fp> BOOST_THREAD_DCL_MOVABLE_END
#else

#if ! defined BOOST_MSVC && defined(BOOST_THREAD_PROVIDES_INVOKE)

#define BOOST_THREAD_RV_REF_ARG_T(z, n, unused) BOOST_PP_COMMA_IF(n) BOOST_THREAD_RV_REF(Arg##n)
#define BOOST_THREAD_RV_REF_A_T(z, n, unused) BOOST_PP_COMMA_IF(n) BOOST_THREAD_RV_REF(A##n)
#define BOOST_THREAD_RV_REF_ARG(z, n, unused) , BOOST_THREAD_RV_REF(Arg##n) arg##n
#define BOOST_THREAD_FWD_REF_A(z, n, unused)   , BOOST_THREAD_FWD_REF(A##n) arg##n
#define BOOST_THREAD_FWD_REF_ARG(z, n, unused) , BOOST_THREAD_FWD_REF(Arg##n) arg##n
#define BOOST_THREAD_FWD_PARAM(z, n, unused) , boost::forward<Arg##n>(arg##n)
#define BOOST_THREAD_FWD_PARAM_A(z, n, unused) , boost::forward<A##n>(arg##n)
#define BOOST_THREAD_DCL(z, n, unused) Arg##n v##n;
#define BOOST_THREAD_MOVE_PARAM(z, n, unused) , v##n(boost::move(arg##n))
#define BOOST_THREAD_FORWARD_PARAM_A(z, n, unused) , v##n(boost::forward<A##n>(arg##n))
#define BOOST_THREAD_MOVE_RHS_PARAM(z, n, unused) , v##n(boost::move(x.v##n))
#define BOOST_THREAD_MOVE_DCL(z, n, unused) , boost::move(v##n)
#define BOOST_THREAD_MOVE_DCL_T(z, n, unused) BOOST_PP_COMMA_IF(n) boost::move(v##n)
#define BOOST_THREAD_ARG_DEF(z, n, unused) , class Arg##n = tuples::null_type

  template  <class Fp, class Arg = tuples::null_type
    BOOST_PP_REPEAT(BOOST_THREAD_MAX_ARGS, BOOST_THREAD_ARG_DEF, ~)
    >
    class invoker;

#define BOOST_THREAD_ASYNC_FUNCT(z, n, unused) \
    template <class Fp BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class Arg) > \
    class invoker<Fp BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, Arg)> \
    { \
      Fp fp_; \
      BOOST_PP_REPEAT(n, BOOST_THREAD_DCL, ~) \
    public: \
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker) \
      typedef typename result_of<Fp(BOOST_PP_ENUM_PARAMS(n, Arg))>::type result_type; \
      \
      template <class F BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class A) > \
      BOOST_SYMBOL_VISIBLE \
      explicit invoker(BOOST_THREAD_FWD_REF(F) f \
          BOOST_PP_REPEAT(n, BOOST_THREAD_FWD_REF_A, ~) \
      ) \
      : fp_(boost::forward<F>(f)) \
      BOOST_PP_REPEAT(n, BOOST_THREAD_FORWARD_PARAM_A, ~) \
      {} \
      \
      BOOST_SYMBOL_VISIBLE \
      invoker(BOOST_THREAD_RV_REF(invoker) x) \
      : fp_(boost::move(x.fp_)) \
      BOOST_PP_REPEAT(n, BOOST_THREAD_MOVE_RHS_PARAM, ~) \
      {} \
      \
      result_type operator()() { \
        return detail::invoke(boost::move(fp_) \
            BOOST_PP_REPEAT(n, BOOST_THREAD_MOVE_DCL, ~) \
        ); \
      } \
    }; \
    \
    template <class R BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class Arg) > \
    class invoker<R(*)(BOOST_PP_REPEAT(n, BOOST_THREAD_RV_REF_ARG_T, ~)) BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, Arg)> \
    { \
      typedef R(*Fp)(BOOST_PP_REPEAT(n, BOOST_THREAD_RV_REF_ARG_T, ~)); \
      Fp fp_; \
      BOOST_PP_REPEAT(n, BOOST_THREAD_DCL, ~) \
    public: \
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker) \
      typedef typename result_of<Fp(BOOST_PP_ENUM_PARAMS(n, Arg))>::type result_type; \
      \
      template <class R2 BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class A) > \
      BOOST_SYMBOL_VISIBLE \
      explicit invoker(R2(*f)(BOOST_PP_REPEAT(n, BOOST_THREAD_RV_REF_A_T, ~))  \
          BOOST_PP_REPEAT(n, BOOST_THREAD_FWD_REF_A, ~) \
      ) \
      : fp_(f) \
      BOOST_PP_REPEAT(n, BOOST_THREAD_FORWARD_PARAM_A, ~) \
      {} \
      \
      BOOST_SYMBOL_VISIBLE \
      invoker(BOOST_THREAD_RV_REF(invoker) x) \
      : fp_(x.fp_) \
      BOOST_PP_REPEAT(n, BOOST_THREAD_MOVE_RHS_PARAM, ~) \
      {} \
      \
      result_type operator()() { \
        return fp_( \
            BOOST_PP_REPEAT(n, BOOST_THREAD_MOVE_DCL_T, ~) \
        ); \
      } \
    };

    BOOST_PP_REPEAT(BOOST_THREAD_MAX_ARGS, BOOST_THREAD_ASYNC_FUNCT, ~)

    #undef BOOST_THREAD_RV_REF_ARG_T
    #undef BOOST_THREAD_RV_REF_ARG
    #undef BOOST_THREAD_FWD_REF_ARG
    #undef BOOST_THREAD_FWD_REF_A
    #undef BOOST_THREAD_FWD_PARAM
    #undef BOOST_THREAD_FWD_PARAM_A
    #undef BOOST_THREAD_DCL
    #undef BOOST_THREAD_MOVE_PARAM
    #undef BOOST_THREAD_MOVE_RHS_PARAM
    #undef BOOST_THREAD_MOVE_DCL
    #undef BOOST_THREAD_ARG_DEF
    #undef BOOST_THREAD_ASYNC_FUNCT

#else

    template <class Fp,
    class T0 = tuples::null_type, class T1 = tuples::null_type, class T2 = tuples::null_type,
    class T3 = tuples::null_type, class T4 = tuples::null_type, class T5 = tuples::null_type,
    class T6 = tuples::null_type, class T7 = tuples::null_type, class T8 = tuples::null_type
    , class T9 = tuples::null_type
    >
    class invoker;

    template <class Fp,
    class T0 , class T1 , class T2 ,
    class T3 , class T4 , class T5 ,
    class T6 , class T7 , class T8 >
    class invoker<Fp, T0, T1, T2, T3, T4, T5, T6, T7, T8>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
      T4 v4_;
      T5 v5_;
      T6 v6_;
      T7 v7_;
      T8 v8_;
      //::boost::tuple<Fp, T0, T1, T2, T3, T4, T5, T6, T7, T8> f_;

    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6, T7, T8)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
          , BOOST_THREAD_RV_REF(T4) a4
          , BOOST_THREAD_RV_REF(T5) a5
          , BOOST_THREAD_RV_REF(T6) a6
          , BOOST_THREAD_RV_REF(T7) a7
          , BOOST_THREAD_RV_REF(T8) a8
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      , v4_(boost::move(a4))
      , v5_(boost::move(a5))
      , v6_(boost::move(a6))
      , v7_(boost::move(a7))
      , v8_(boost::move(a8))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      , v4_(boost::move(BOOST_THREAD_RV(f).v4_))
      , v5_(boost::move(BOOST_THREAD_RV(f).v5_))
      , v6_(boost::move(BOOST_THREAD_RV(f).v6_))
      , v7_(boost::move(BOOST_THREAD_RV(f).v7_))
      , v8_(boost::move(BOOST_THREAD_RV(f).v8_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
            , boost::move(v4_)
            , boost::move(v5_)
            , boost::move(v6_)
            , boost::move(v7_)
            , boost::move(v8_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
    class invoker<Fp, T0, T1, T2, T3, T4, T5, T6, T7>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
      T4 v4_;
      T5 v5_;
      T6 v6_;
      T7 v7_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6, T7)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
          , BOOST_THREAD_RV_REF(T4) a4
          , BOOST_THREAD_RV_REF(T5) a5
          , BOOST_THREAD_RV_REF(T6) a6
          , BOOST_THREAD_RV_REF(T7) a7
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      , v4_(boost::move(a4))
      , v5_(boost::move(a5))
      , v6_(boost::move(a6))
      , v7_(boost::move(a7))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      , v4_(boost::move(BOOST_THREAD_RV(f).v4_))
      , v5_(boost::move(BOOST_THREAD_RV(f).v5_))
      , v6_(boost::move(BOOST_THREAD_RV(f).v6_))
      , v7_(boost::move(BOOST_THREAD_RV(f).v7_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
            , boost::move(v4_)
            , boost::move(v5_)
            , boost::move(v6_)
            , boost::move(v7_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    class invoker<Fp, T0, T1, T2, T3, T4, T5, T6>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
      T4 v4_;
      T5 v5_;
      T6 v6_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
          , BOOST_THREAD_RV_REF(T4) a4
          , BOOST_THREAD_RV_REF(T5) a5
          , BOOST_THREAD_RV_REF(T6) a6
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      , v4_(boost::move(a4))
      , v5_(boost::move(a5))
      , v6_(boost::move(a6))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      , v4_(boost::move(BOOST_THREAD_RV(f).v4_))
      , v5_(boost::move(BOOST_THREAD_RV(f).v5_))
      , v6_(boost::move(BOOST_THREAD_RV(f).v6_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
            , boost::move(v4_)
            , boost::move(v5_)
            , boost::move(v6_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5>
    class invoker<Fp, T0, T1, T2, T3, T4, T5>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
      T4 v4_;
      T5 v5_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
          , BOOST_THREAD_RV_REF(T4) a4
          , BOOST_THREAD_RV_REF(T5) a5
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      , v4_(boost::move(a4))
      , v5_(boost::move(a5))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      , v4_(boost::move(BOOST_THREAD_RV(f).v4_))
      , v5_(boost::move(BOOST_THREAD_RV(f).v5_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
            , boost::move(v4_)
            , boost::move(v5_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4>
    class invoker<Fp, T0, T1, T2, T3, T4>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
      T4 v4_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3, T4)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
          , BOOST_THREAD_RV_REF(T4) a4
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      , v4_(boost::move(a4))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      , v4_(boost::move(BOOST_THREAD_RV(f).v4_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
            , boost::move(v4_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2, class T3>
    class invoker<Fp, T0, T1, T2, T3>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
      T3 v3_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2, T3)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
          , BOOST_THREAD_RV_REF(T3) a3
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      , v3_(boost::move(a3))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      , v3_(boost::move(BOOST_THREAD_RV(f).v3_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
            , boost::move(v3_)
        );
      }
    };
    template <class Fp, class T0, class T1, class T2>
    class invoker<Fp, T0, T1, T2>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
      T2 v2_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1, T2)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
          , BOOST_THREAD_RV_REF(T2) a2
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      , v2_(boost::move(a2))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      , v2_(boost::move(BOOST_THREAD_RV(f).v2_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
            , boost::move(v2_)
        );
      }
    };
    template <class Fp, class T0, class T1>
    class invoker<Fp, T0, T1>
    {
      Fp fp_;
      T0 v0_;
      T1 v1_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0, T1)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
          , BOOST_THREAD_RV_REF(T1) a1
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      , v1_(boost::move(a1))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      , v1_(boost::move(BOOST_THREAD_RV(f).v1_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
            , boost::move(v1_)
        );
      }
    };
    template <class Fp, class T0>
    class invoker<Fp, T0>
    {
      Fp fp_;
      T0 v0_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp(T0)>::type result_type;

      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f
          , BOOST_THREAD_RV_REF(T0) a0
      )
      : fp_(boost::move(f))
      , v0_(boost::move(a0))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(BOOST_THREAD_RV(f).fp))
      , v0_(boost::move(BOOST_THREAD_RV(f).v0_))
      {}

      result_type operator()()
      {
        return detail::invoke(boost::move(fp_)
            , boost::move(v0_)
        );
      }
    };
    template <class Fp>
    class invoker<Fp>
    {
      Fp fp_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp()>::type result_type;
      BOOST_SYMBOL_VISIBLE
      explicit invoker(BOOST_THREAD_FWD_REF(Fp) f)
      : fp_(boost::move(f))
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(boost::move(f.fp_))
      {}
      result_type operator()()
      {
        return fp_();
      }
    };
    template <class R>
    class invoker<R(*)()>
    {
      typedef R(*Fp)();
      Fp fp_;
    public:
      BOOST_THREAD_COPYABLE_AND_MOVABLE(invoker)
      typedef typename result_of<Fp()>::type result_type;
      BOOST_SYMBOL_VISIBLE
      explicit invoker(Fp f)
      : fp_(f)
      {}

      BOOST_SYMBOL_VISIBLE
      invoker(BOOST_THREAD_RV_REF(invoker) f)
      : fp_(f.fp_)
      {}
      result_type operator()()
      {
        return fp_();
      }
    };
#endif
#endif

  }
}

#include <boost/thread/detail/variadic_footer.hpp>

#endif // header

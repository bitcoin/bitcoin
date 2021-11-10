// Copyright (C) 2012-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2013/04 Vicente J. Botet Escriba
//   Provide implementation up to 10 parameters when BOOST_NO_CXX11_VARIADIC_TEMPLATES is defined.
// 2012/11 Vicente J. Botet Escriba
//   Adapt to boost libc++ implementation

//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
// The make_tuple_indices C++11 code is based on the one from libcxx.
//===----------------------------------------------------------------------===//

#ifndef BOOST_THREAD_DETAIL_MAKE_TUPLE_INDICES_HPP
#define BOOST_THREAD_DETAIL_MAKE_TUPLE_INDICES_HPP

#include <boost/config.hpp>
#include <boost/static_assert.hpp>

namespace boost
{
  namespace detail
  {

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    // make_tuple_indices

    template <std::size_t...> struct tuple_indices
    {};

    template <std::size_t Sp, class IntTuple, std::size_t Ep>
    struct make_indices_imp;

    template <std::size_t Sp, std::size_t ...Indices, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<Indices...>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<Indices..., Sp>, Ep>::type type;
    };

    template <std::size_t Ep, std::size_t ...Indices>
    struct make_indices_imp<Ep, tuple_indices<Indices...>, Ep>
    {
      typedef tuple_indices<Indices...> type;
    };

    template <std::size_t Ep, std::size_t Sp = 0>
    struct make_tuple_indices
    {
      BOOST_STATIC_ASSERT_MSG(Sp <= Ep, "make_tuple_indices input error");
      typedef typename make_indices_imp<Sp, tuple_indices<>, Ep>::type type;
    };
#else

    // - tuple forward declaration -----------------------------------------------
    template <
      std::size_t T0 = 0, std::size_t T1 = 0, std::size_t T2 = 0,
      std::size_t T3 = 0, std::size_t T4 = 0, std::size_t T5 = 0,
      std::size_t T6 = 0, std::size_t T7 = 0, std::size_t T8 = 0,
      std::size_t T9 = 0>
    class tuple_indices {};

    template <std::size_t Sp, class IntTuple, std::size_t Ep>
    struct make_indices_imp;

    template <std::size_t Sp, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4, I5>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, I5, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4, I5, I6>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, I5, I6, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t I7
    , std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4, I5, I6, I7>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, Sp>, Ep>::type type;
    };
    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t I7
    , std::size_t I8
    , std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4, I5, I6, I7, I8>, Ep>
    {
      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8, Sp>, Ep>::type type;
    };
//    template <std::size_t Sp, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
//    , std::size_t I6
//    , std::size_t I7
//    , std::size_t I8
//    , std::size_t I9
//    , std::size_t Ep>
//    struct make_indices_imp<Sp, tuple_indices<I0, I1 , I2, I3, I4, I5, I6, I7, I8, I9>, Ep>
//    {
//      typedef typename make_indices_imp<Sp+1, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, Sp>, Ep>::type type;
//    };

    template <std::size_t Ep>
    struct make_indices_imp<Ep, tuple_indices<>, Ep>
    {
      typedef tuple_indices<> type;
    };
    template <std::size_t Ep, std::size_t I0>
    struct make_indices_imp<Ep, tuple_indices<I0>, Ep>
    {
      typedef tuple_indices<I0> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1>
    struct make_indices_imp<Ep, tuple_indices<I0, I1>, Ep>
    {
      typedef tuple_indices<I0, I1> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2>
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2>, Ep>
    {
      typedef tuple_indices<I0, I1, I2> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3>
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4>
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5>
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4, I5>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4, I5> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    >
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4, I5, I6>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4, I5, I6> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t I7
    >
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7> type;
    };
    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t I7
    , std::size_t I8
    >
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8> type;
    };

    template <std::size_t Ep, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
    , std::size_t I6
    , std::size_t I7
    , std::size_t I8
    , std::size_t I9
    >
    struct make_indices_imp<Ep, tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9>, Ep>
    {
      typedef tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9> type;
    };

    template <std::size_t Ep, std::size_t Sp = 0>
    struct make_tuple_indices
    {
      BOOST_STATIC_ASSERT_MSG(Sp <= Ep, "make_tuple_indices input error");
      typedef typename make_indices_imp<Sp, tuple_indices<>, Ep>::type type;
    };

#endif
  }
}

#endif // header

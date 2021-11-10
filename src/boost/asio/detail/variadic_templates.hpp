//
// detail/variadic_templates.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_VARIADIC_TEMPLATES_HPP
#define BOOST_ASIO_DETAIL_VARIADIC_TEMPLATES_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

# define BOOST_ASIO_VARIADIC_TPARAMS(n) BOOST_ASIO_VARIADIC_TPARAMS_##n

# define BOOST_ASIO_VARIADIC_TPARAMS_1 \
  typename T1
# define BOOST_ASIO_VARIADIC_TPARAMS_2 \
  typename T1, typename T2
# define BOOST_ASIO_VARIADIC_TPARAMS_3 \
  typename T1, typename T2, typename T3
# define BOOST_ASIO_VARIADIC_TPARAMS_4 \
  typename T1, typename T2, typename T3, typename T4
# define BOOST_ASIO_VARIADIC_TPARAMS_5 \
  typename T1, typename T2, typename T3, typename T4, typename T5
# define BOOST_ASIO_VARIADIC_TPARAMS_6 \
  typename T1, typename T2, typename T3, typename T4, typename T5, \
  typename T6
# define BOOST_ASIO_VARIADIC_TPARAMS_7 \
  typename T1, typename T2, typename T3, typename T4, typename T5, \
  typename T6, typename T7
# define BOOST_ASIO_VARIADIC_TPARAMS_8 \
  typename T1, typename T2, typename T3, typename T4, typename T5, \
  typename T6, typename T7, typename T8

# define BOOST_ASIO_VARIADIC_TARGS(n) BOOST_ASIO_VARIADIC_TARGS_##n

# define BOOST_ASIO_VARIADIC_TARGS_1 T1
# define BOOST_ASIO_VARIADIC_TARGS_2 T1, T2
# define BOOST_ASIO_VARIADIC_TARGS_3 T1, T2, T3
# define BOOST_ASIO_VARIADIC_TARGS_4 T1, T2, T3, T4
# define BOOST_ASIO_VARIADIC_TARGS_5 T1, T2, T3, T4, T5
# define BOOST_ASIO_VARIADIC_TARGS_6 T1, T2, T3, T4, T5, T6
# define BOOST_ASIO_VARIADIC_TARGS_7 T1, T2, T3, T4, T5, T6, T7
# define BOOST_ASIO_VARIADIC_TARGS_8 T1, T2, T3, T4, T5, T6, T7, T8

# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS(n) \
  BOOST_ASIO_VARIADIC_BYVAL_PARAMS_##n

# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_1 T1 x1
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_2 T1 x1, T2 x2
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_3 T1 x1, T2 x2, T3 x3
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_4 T1 x1, T2 x2, T3 x3, T4 x4
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_5 T1 x1, T2 x2, T3 x3, T4 x4, T5 x5
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_6 T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, \
  T6 x6
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_7 T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, \
  T6 x6, T7 x7
# define BOOST_ASIO_VARIADIC_BYVAL_PARAMS_8 T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, \
  T6 x6, T7 x7, T8 x8

# define BOOST_ASIO_VARIADIC_BYVAL_ARGS(n) \
  BOOST_ASIO_VARIADIC_BYVAL_ARGS_##n

# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_1 x1
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_2 x1, x2
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_3 x1, x2, x3
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_4 x1, x2, x3, x4
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_5 x1, x2, x3, x4, x5
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_6 x1, x2, x3, x4, x5, x6
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_7 x1, x2, x3, x4, x5, x6, x7
# define BOOST_ASIO_VARIADIC_BYVAL_ARGS_8 x1, x2, x3, x4, x5, x6, x7, x8

# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS(n) \
  BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_##n

# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_1 \
  const T1& x1
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_2 \
  const T1& x1, const T2& x2
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_3 \
  const T1& x1, const T2& x2, const T3& x3
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_4 \
  const T1& x1, const T2& x2, const T3& x3, const T4& x4
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_5 \
  const T1& x1, const T2& x2, const T3& x3, const T4& x4, const T5& x5
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_6 \
  const T1& x1, const T2& x2, const T3& x3, const T4& x4, const T5& x5, \
  const T6& x6
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_7 \
  const T1& x1, const T2& x2, const T3& x3, const T4& x4, const T5& x5, \
  const T6& x6, const T7& x7
# define BOOST_ASIO_VARIADIC_CONSTREF_PARAMS_8 \
  const T1& x1, const T2& x2, const T3& x3, const T4& x4, const T5& x5, \
  const T6& x6, const T7& x7, const T8& x8

# define BOOST_ASIO_VARIADIC_MOVE_PARAMS(n) \
  BOOST_ASIO_VARIADIC_MOVE_PARAMS_##n

# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_1 \
  BOOST_ASIO_MOVE_ARG(T1) x1
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_2 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_3 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_4 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3, BOOST_ASIO_MOVE_ARG(T4) x4
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_5 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3, BOOST_ASIO_MOVE_ARG(T4) x4, \
  BOOST_ASIO_MOVE_ARG(T5) x5
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_6 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3, BOOST_ASIO_MOVE_ARG(T4) x4, \
  BOOST_ASIO_MOVE_ARG(T5) x5, BOOST_ASIO_MOVE_ARG(T6) x6
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_7 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3, BOOST_ASIO_MOVE_ARG(T4) x4, \
  BOOST_ASIO_MOVE_ARG(T5) x5, BOOST_ASIO_MOVE_ARG(T6) x6, \
  BOOST_ASIO_MOVE_ARG(T7) x7
# define BOOST_ASIO_VARIADIC_MOVE_PARAMS_8 \
  BOOST_ASIO_MOVE_ARG(T1) x1, BOOST_ASIO_MOVE_ARG(T2) x2, \
  BOOST_ASIO_MOVE_ARG(T3) x3, BOOST_ASIO_MOVE_ARG(T4) x4, \
  BOOST_ASIO_MOVE_ARG(T5) x5, BOOST_ASIO_MOVE_ARG(T6) x6, \
  BOOST_ASIO_MOVE_ARG(T7) x7, BOOST_ASIO_MOVE_ARG(T8) x8

# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS(n) \
  BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_##n

# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_1 \
  BOOST_ASIO_MOVE_ARG(T1)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_2 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_3 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_4 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3), BOOST_ASIO_MOVE_ARG(T4)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_5 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3), BOOST_ASIO_MOVE_ARG(T4), \
  BOOST_ASIO_MOVE_ARG(T5)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_6 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3), BOOST_ASIO_MOVE_ARG(T4), \
  BOOST_ASIO_MOVE_ARG(T5), BOOST_ASIO_MOVE_ARG(T6)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_7 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3), BOOST_ASIO_MOVE_ARG(T4), \
  BOOST_ASIO_MOVE_ARG(T5), BOOST_ASIO_MOVE_ARG(T6), \
  BOOST_ASIO_MOVE_ARG(T7)
# define BOOST_ASIO_VARIADIC_UNNAMED_MOVE_PARAMS_8 \
  BOOST_ASIO_MOVE_ARG(T1), BOOST_ASIO_MOVE_ARG(T2), \
  BOOST_ASIO_MOVE_ARG(T3), BOOST_ASIO_MOVE_ARG(T4), \
  BOOST_ASIO_MOVE_ARG(T5), BOOST_ASIO_MOVE_ARG(T6), \
  BOOST_ASIO_MOVE_ARG(T7), BOOST_ASIO_MOVE_ARG(T8)

# define BOOST_ASIO_VARIADIC_MOVE_ARGS(n) \
  BOOST_ASIO_VARIADIC_MOVE_ARGS_##n

# define BOOST_ASIO_VARIADIC_MOVE_ARGS_1 \
  BOOST_ASIO_MOVE_CAST(T1)(x1)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_2 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_3 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_4 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3), BOOST_ASIO_MOVE_CAST(T4)(x4)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_5 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3), BOOST_ASIO_MOVE_CAST(T4)(x4), \
  BOOST_ASIO_MOVE_CAST(T5)(x5)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_6 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3), BOOST_ASIO_MOVE_CAST(T4)(x4), \
  BOOST_ASIO_MOVE_CAST(T5)(x5), BOOST_ASIO_MOVE_CAST(T6)(x6)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_7 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3), BOOST_ASIO_MOVE_CAST(T4)(x4), \
  BOOST_ASIO_MOVE_CAST(T5)(x5), BOOST_ASIO_MOVE_CAST(T6)(x6), \
  BOOST_ASIO_MOVE_CAST(T7)(x7)
# define BOOST_ASIO_VARIADIC_MOVE_ARGS_8 \
  BOOST_ASIO_MOVE_CAST(T1)(x1), BOOST_ASIO_MOVE_CAST(T2)(x2), \
  BOOST_ASIO_MOVE_CAST(T3)(x3), BOOST_ASIO_MOVE_CAST(T4)(x4), \
  BOOST_ASIO_MOVE_CAST(T5)(x5), BOOST_ASIO_MOVE_CAST(T6)(x6), \
  BOOST_ASIO_MOVE_CAST(T7)(x7), BOOST_ASIO_MOVE_CAST(T8)(x8)

# define BOOST_ASIO_VARIADIC_DECLVAL(n) \
  BOOST_ASIO_VARIADIC_DECLVAL_##n

# define BOOST_ASIO_VARIADIC_DECLVAL_1 \
  declval<T1>()
# define BOOST_ASIO_VARIADIC_DECLVAL_2 \
  declval<T1>(), declval<T2>()
# define BOOST_ASIO_VARIADIC_DECLVAL_3 \
  declval<T1>(), declval<T2>(), declval<T3>()
# define BOOST_ASIO_VARIADIC_DECLVAL_4 \
  declval<T1>(), declval<T2>(), declval<T3>(), declval<T4>()
# define BOOST_ASIO_VARIADIC_DECLVAL_5 \
  declval<T1>(), declval<T2>(), declval<T3>(), declval<T4>(), \
  declval<T5>()
# define BOOST_ASIO_VARIADIC_DECLVAL_6 \
  declval<T1>(), declval<T2>(), declval<T3>(), declval<T4>(), \
  declval<T5>(), declval<T6>()
# define BOOST_ASIO_VARIADIC_DECLVAL_7 \
  declval<T1>(), declval<T2>(), declval<T3>(), declval<T4>(), \
  declval<T5>(), declval<T6>(), declval<T7>()
# define BOOST_ASIO_VARIADIC_DECLVAL_8 \
  declval<T1>(), declval<T2>(), declval<T3>(), declval<T4>(), \
  declval<T5>(), declval<T6>(), declval<T7>(), declval<T8>()

# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL(n) \
  BOOST_ASIO_VARIADIC_MOVE_DECLVAL_##n

# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_1 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_2 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_3 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_4 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>(), declval<BOOST_ASIO_MOVE_ARG(T4)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_5 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>(), declval<BOOST_ASIO_MOVE_ARG(T4)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T5)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_6 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>(), declval<BOOST_ASIO_MOVE_ARG(T4)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T5)>(), declval<BOOST_ASIO_MOVE_ARG(T6)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_7 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>(), declval<BOOST_ASIO_MOVE_ARG(T4)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T5)>(), declval<BOOST_ASIO_MOVE_ARG(T6)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T7)>()
# define BOOST_ASIO_VARIADIC_MOVE_DECLVAL_8 \
  declval<BOOST_ASIO_MOVE_ARG(T1)>(), declval<BOOST_ASIO_MOVE_ARG(T2)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T3)>(), declval<BOOST_ASIO_MOVE_ARG(T4)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T5)>(), declval<BOOST_ASIO_MOVE_ARG(T6)>(), \
  declval<BOOST_ASIO_MOVE_ARG(T7)>(), declval<BOOST_ASIO_MOVE_ARG(T8)>()

# define BOOST_ASIO_VARIADIC_DECAY(n) \
  BOOST_ASIO_VARIADIC_DECAY_##n

# define BOOST_ASIO_VARIADIC_DECAY_1 \
  typename decay<T1>::type
# define BOOST_ASIO_VARIADIC_DECAY_2 \
  typename decay<T1>::type, typename decay<T2>::type
# define BOOST_ASIO_VARIADIC_DECAY_3 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type
# define BOOST_ASIO_VARIADIC_DECAY_4 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type, typename decay<T4>::type
# define BOOST_ASIO_VARIADIC_DECAY_5 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type, typename decay<T4>::type, \
  typename decay<T5>::type
# define BOOST_ASIO_VARIADIC_DECAY_6 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type, typename decay<T4>::type, \
  typename decay<T5>::type, typename decay<T6>::type
# define BOOST_ASIO_VARIADIC_DECAY_7 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type, typename decay<T4>::type, \
  typename decay<T5>::type, typename decay<T6>::type, \
  typename decay<T7>::type
# define BOOST_ASIO_VARIADIC_DECAY_8 \
  typename decay<T1>::type, typename decay<T2>::type, \
  typename decay<T3>::type, typename decay<T4>::type, \
  typename decay<T5>::type, typename decay<T6>::type, \
  typename decay<T7>::type, typename decay<T8>::type

# define BOOST_ASIO_VARIADIC_GENERATE(m) m(1) m(2) m(3) m(4) m(5) m(6) m(7) m(8)
# define BOOST_ASIO_VARIADIC_GENERATE_5(m) m(1) m(2) m(3) m(4) m(5)

#endif // !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#endif // BOOST_ASIO_DETAIL_VARIADIC_TEMPLATES_HPP

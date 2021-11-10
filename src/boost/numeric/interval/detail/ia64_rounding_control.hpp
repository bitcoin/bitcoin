/* Boost interval/detail/ia64_rounding_control.hpp file
 *
 * Copyright 2006-2007 Boris Gubenko
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_DETAIL_IA64_ROUNDING_CONTROL_HPP
#define BOOST_NUMERIC_INTERVAL_DETAIL_IA64_ROUNDING_CONTROL_HPP

#if !defined(ia64) && !defined(__ia64) && !defined(__ia64__)
#error This header only works on ia64 CPUs.
#endif

#if defined(__hpux)

#   include <fenv.h>

namespace boost {
namespace numeric {
namespace interval_lib {
namespace detail {


    struct ia64_rounding_control
    {
    typedef unsigned int rounding_mode;

    static void set_rounding_mode(const rounding_mode& mode)  { 
fesetround(mode); }
    static void get_rounding_mode(rounding_mode& mode)  { mode = fegetround(); }

    static void downward()    { set_rounding_mode(FE_DOWNWARD); }
    static void upward()      { set_rounding_mode(FE_UPWARD); }
    static void to_nearest()  { set_rounding_mode(FE_TONEAREST); }
    static void toward_zero() { set_rounding_mode(FE_TOWARDZERO); }
    };

} // namespace detail

extern "C" {
  float rintf(float);
  double rint(double);
  long double rintl(long double);
}

template<>
struct rounding_control<float>:
  detail::ia64_rounding_control
{
  static float force_rounding(const float r)
  { volatile float _r = r; return _r; }
  static float to_int(const float& x) { return rintf(x); }
};

template<>
struct rounding_control<double>:
  detail::ia64_rounding_control
{
  static const double & force_rounding(const double& r) { return r; }
  static double to_int(const double& r) { return rint(r); }
};

template<>
struct rounding_control<long double>:
  detail::ia64_rounding_control
{
  static const long double & force_rounding(const long double& r) { return r; }
  static long double to_int(const long double& r) { return rintl(r); }
};

} // namespace interval_lib
} // namespace numeric
} // namespace boost

#undef BOOST_NUMERIC_INTERVAL_NO_HARDWARE

#endif /* __hpux */

#endif /* BOOST_NUMERIC_INTERVAL_DETAIL_IA64_ROUNDING_CONTROL_HPP */


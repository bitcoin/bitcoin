/* Boost interval/detail/alpha_rounding_control.hpp file
 *
 * Copyright 2005 Felix Höfling, Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_DETAIL_ALPHA_ROUNDING_CONTROL_HPP
#define BOOST_NUMERIC_INTERVAL_DETAIL_ALPHA_ROUNDING_CONTROL_HPP

#if !defined(alpha) && !defined(__alpha__)
#error This header only works on Alpha CPUs.
#endif

#if defined(__GNUC__) || defined(__digital__) || defined(__DECCXX)

#include <float.h> // write_rnd() and read_rnd()

namespace boost {
namespace numeric {
namespace interval_lib {

namespace detail {
#if defined(__GNUC__ )
    typedef union {
    ::boost::long_long_type imode;
    double dmode;
    } rounding_mode_struct;

    // set bits 59-58 (DYN),
    // clear all exception bits and disable overflow (51) and inexact exceptions (62)
    static const rounding_mode_struct mode_upward      = { 0x4C08000000000000LL };
    static const rounding_mode_struct mode_downward    = { 0x4408000000000000LL };
    static const rounding_mode_struct mode_to_nearest  = { 0x4808000000000000LL };
    static const rounding_mode_struct mode_toward_zero = { 0x4008000000000000LL };

    struct alpha_rounding_control
    {
    typedef double rounding_mode;

    static void set_rounding_mode(const rounding_mode mode)
    { __asm__ __volatile__ ("mt_fpcr %0" : : "f"(mode)); }

    static void get_rounding_mode(rounding_mode& mode)
    { __asm__ __volatile__ ("mf_fpcr %0" : "=f"(mode)); }

    static void downward()    { set_rounding_mode(mode_downward.dmode);    }
    static void upward()      { set_rounding_mode(mode_upward.dmode);      }
    static void to_nearest()  { set_rounding_mode(mode_to_nearest.dmode);  }
    static void toward_zero() { set_rounding_mode(mode_toward_zero.dmode); }
    };
#elif defined(__digital__) || defined(__DECCXX)

#if defined(__DECCXX) && !(defined(__FLT_ROUNDS) && __FLT_ROUNDS == -1)
#error Dynamic rounding mode not enabled. See cxx man page for details.
#endif

    struct alpha_rounding_control
    {
    typedef unsigned int rounding_mode;

    static void set_rounding_mode(const rounding_mode& mode)  { write_rnd(mode); }
    static void get_rounding_mode(rounding_mode& mode)  { mode = read_rnd(); }

    static void downward()    { set_rounding_mode(FP_RND_RM); }
    static void upward()      { set_rounding_mode(FP_RND_RP); }
    static void to_nearest()  { set_rounding_mode(FP_RND_RN); }
    static void toward_zero() { set_rounding_mode(FP_RND_RZ); }
    };
#endif
} // namespace detail

extern "C" {
  float rintf(float);
  double rint(double);
  long double rintl(long double);
}

template<>
struct rounding_control<float>:
  detail::alpha_rounding_control
{
  static float force_rounding(const float r)
  { volatile float _r = r; return _r; }
  static float to_int(const float& x) { return rintf(x); }
};

template<>
struct rounding_control<double>:
  detail::alpha_rounding_control
{
  static const double & force_rounding(const double& r) { return r; }
  static double to_int(const double& r) { return rint(r); }
};

template<>
struct rounding_control<long double>:
  detail::alpha_rounding_control
{
  static const long double & force_rounding(const long double& r) { return r; }
  static long double to_int(const long double& r) { return rintl(r); }
};

} // namespace interval_lib
} // namespace numeric
} // namespace boost

#undef BOOST_NUMERIC_INTERVAL_NO_HARDWARE
#endif

#endif /* BOOST_NUMERIC_INTERVAL_DETAIL_ALPHA_ROUNDING_CONTROL_HPP */

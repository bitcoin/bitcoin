/* Boost interval/detail/x86_rounding_control.hpp file
 *
 * Copyright 2000 Jens Maurer
 * Copyright 2002 Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_DETAIL_X86_ROUNDING_CONTROL_HPP
#define BOOST_NUMERIC_INTERVAL_DETAIL_X86_ROUNDING_CONTROL_HPP

#if defined(__GNUC__) || defined(__BORLANDC__) && defined(__clang__)
#  include <boost/numeric/interval/detail/x86gcc_rounding_control.hpp>
#elif defined(__BORLANDC__)
#  include <boost/numeric/interval/detail/bcc_rounding_control.hpp>
#elif defined(_MSC_VER)
#  include <boost/numeric/interval/detail/msvc_rounding_control.hpp>
#elif defined(__MWERKS__) || defined(__ICC) || defined (__SUNPRO_CC)
#  define BOOST_NUMERIC_INTERVAL_USE_C99_SUBSYSTEM
#  include <boost/numeric/interval/detail/c99sub_rounding_control.hpp>
#else
#  error Unsupported C++ compiler.
#endif

namespace boost {
namespace numeric {
namespace interval_lib {

namespace detail {

#ifdef BOOST_NUMERIC_INTERVAL_USE_C99_SUBSYSTEM
typedef c99_rounding_control x86_rounding_control;
#undef BOOST_NUMERIC_INTERVAL_USE_C99_SUBSYSTEM
#else
struct fpu_rounding_modes
{
  unsigned short to_nearest;
  unsigned short downward;
  unsigned short upward;
  unsigned short toward_zero;
};

// exceptions masked, extended precision
// hardware default is 0x037f (0x1000 only has a meaning on 287)
static const fpu_rounding_modes rnd_mode = { 0x137f, 0x177f, 0x1b7f, 0x1f7f };

struct x86_rounding_control: x86_rounding
{
  static void to_nearest()  { set_rounding_mode(rnd_mode.to_nearest);  }
  static void downward()    { set_rounding_mode(rnd_mode.downward);    }
  static void upward()      { set_rounding_mode(rnd_mode.upward);      }
  static void toward_zero() { set_rounding_mode(rnd_mode.toward_zero); }
};
#endif // BOOST_NUMERIC_INTERVAL_USE_C99_SUBSYSTEM

} // namespace detail

template<>
struct rounding_control<float>: detail::x86_rounding_control
{
  static float force_rounding(const float& r) 
  { volatile float r_ = r; return r_; }
};

template<>
struct rounding_control<double>: detail::x86_rounding_control
{
  /*static double force_rounding(double r) 
  { asm volatile ("" : "+m"(r) : ); return r; }*/
  static double force_rounding(const double& r) 
  { volatile double r_ = r; return r_; }
};

namespace detail {

template<bool>
struct x86_rounding_control_long_double;

template<>
struct x86_rounding_control_long_double<false>: x86_rounding_control
{
  static long double force_rounding(long double const &r)
  { volatile long double r_ = r; return r_; }
};

template<>
struct x86_rounding_control_long_double<true>: x86_rounding_control
{
  static long double const &force_rounding(long double const &r)
  { return r; }
};

} // namespace detail

template<>
struct rounding_control<long double>:
  detail::x86_rounding_control_long_double< (sizeof(long double) >= 10) >
{};

} // namespace interval_lib
} // namespace numeric
} // namespace boost

#undef BOOST_NUMERIC_INTERVAL_NO_HARDWARE

#endif /* BOOST_NUMERIC_INTERVAL_DETAIL_X86_ROUNDING_CONTROL_HPP */

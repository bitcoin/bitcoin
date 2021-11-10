///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MP_FLOAT128_HPP
#define BOOST_MP_FLOAT128_HPP

#include <tuple>
#include <boost/config.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/detail/hash.hpp>

#if defined(BOOST_INTEL) && !defined(BOOST_MP_USE_FLOAT128) && !defined(BOOST_MP_USE_QUAD)
#if defined(BOOST_INTEL_CXX_VERSION) && (BOOST_INTEL_CXX_VERSION >= 1310) && defined(__GNUC__)
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6))
#define BOOST_MP_USE_FLOAT128
#endif
#endif

#ifndef BOOST_MP_USE_FLOAT128
#define BOOST_MP_USE_QUAD
#endif
#endif

#if defined(__GNUC__) && !defined(BOOST_MP_USE_FLOAT128) && !defined(BOOST_MP_USE_QUAD)
#define BOOST_MP_USE_FLOAT128
#endif

#if !defined(BOOST_MP_USE_FLOAT128) && !defined(BOOST_MP_USE_QUAD)
#error "Sorry compiler is neither GCC, not Intel, don't know how to configure this header."
#endif
#if defined(BOOST_MP_USE_FLOAT128) && defined(BOOST_MP_USE_QUAD)
#error "Oh dear, both BOOST_MP_USE_FLOAT128 and BOOST_MP_USE_QUAD are defined, which one should I be using?"
#endif

#if defined(BOOST_MP_USE_FLOAT128)

extern "C" {
#include <quadmath.h>
}

using float128_type = __float128;

#elif defined(BOOST_MP_USE_QUAD)

#include <boost/multiprecision/detail/float_string_cvt.hpp>

using float128_type = _Quad;

extern "C" {
_Quad __ldexpq(_Quad, int);
_Quad __frexpq(_Quad, int*);
_Quad __fabsq(_Quad);
_Quad __floorq(_Quad);
_Quad __ceilq(_Quad);
_Quad __sqrtq(_Quad);
_Quad __truncq(_Quad);
_Quad __expq(_Quad);
_Quad __powq(_Quad, _Quad);
_Quad __logq(_Quad);
_Quad __log10q(_Quad);
_Quad __sinq(_Quad);
_Quad __cosq(_Quad);
_Quad __tanq(_Quad);
_Quad __asinq(_Quad);
_Quad __acosq(_Quad);
_Quad __atanq(_Quad);
_Quad __sinhq(_Quad);
_Quad __coshq(_Quad);
_Quad __tanhq(_Quad);
_Quad __fmodq(_Quad, _Quad);
_Quad __atan2q(_Quad, _Quad);

#define ldexpq __ldexpq
#define frexpq __frexpq
#define fabsq __fabsq
#define floorq __floorq
#define ceilq __ceilq
#define sqrtq __sqrtq
#define truncq __truncq
#define expq __expq
#define powq __powq
#define logq __logq
#define log10q __log10q
#define sinq __sinq
#define cosq __cosq
#define tanq __tanq
#define asinq __asinq
#define acosq __acosq
#define atanq __atanq
#define sinhq __sinhq
#define coshq __coshq
#define tanhq __tanhq
#define fmodq __fmodq
#define atan2q __atan2q
}

inline _Quad isnanq(_Quad v)
{
   return v != v;
}
inline _Quad isinfq(_Quad v)
{
   return __fabsq(v) > 1.18973149535723176508575932662800702e4932Q;
}

#endif

namespace boost {
namespace multiprecision {

#ifndef BOOST_MP_BITS_OF_FLOAT128_DEFINED

namespace detail {

template <>
struct bits_of<float128_type>
{
   static constexpr const unsigned value = 113;
};

}

#endif

namespace backends {

struct float128_backend;

}

using backends::float128_backend;

template <>
struct number_category<backends::float128_backend> : public std::integral_constant<int, number_kind_floating_point>
{};
#if defined(BOOST_MP_USE_QUAD)
template <>
struct number_category<float128_type> : public std::integral_constant<int, number_kind_floating_point>
{};
#endif

using float128 = number<float128_backend, et_off>;

namespace quad_constants {
constexpr __float128 quad_min = static_cast<__float128>(1) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) / 1073741824;

constexpr __float128 quad_denorm_min = static_cast<__float128>(1) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) * static_cast<__float128>(DBL_MIN) / 5.5751862996326557854e+42;

constexpr double     dbl_mult = 8.9884656743115795386e+307;                                              // This has one bit set only.
constexpr __float128 quad_max = (static_cast<__float128>(1) - 9.62964972193617926527988971292463659e-35) // This now has all bits sets to 1
                                * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * static_cast<__float128>(dbl_mult) * 65536;
} // namespace quad_constants

#define BOOST_MP_QUAD_MIN boost::multiprecision::quad_constants::quad_min
#define BOOST_MP_QUAD_DENORM_MIN boost::multiprecision::quad_constants::quad_denorm_min
#define BOOST_MP_QUAD_MAX boost::multiprecision::quad_constants::quad_max


namespace backends {

struct float128_backend
{
   using signed_types = std::tuple<signed char, short, int, long, boost::long_long_type>;
   using unsigned_types = std::tuple<unsigned char, unsigned short, unsigned int, unsigned long, boost::ulong_long_type>;
   using float_types = std::tuple<float, double, long double>;
   using exponent_type = int                                  ;

 private:
   float128_type m_value;

 public:
   constexpr   float128_backend() noexcept : m_value(0) {}
   constexpr   float128_backend(const float128_backend& o) noexcept : m_value(o.m_value) {}
   BOOST_MP_CXX14_CONSTEXPR float128_backend& operator=(const float128_backend& o) noexcept
   {
      m_value = o.m_value;
      return *this;
   }
   template <class T>
   constexpr float128_backend(const T& i, const typename std::enable_if<std::is_convertible<T, float128_type>::value>::type* = 0) noexcept(noexcept(std::declval<float128_type&>() = std::declval<const T&>()))
       : m_value(i) {}
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value || std::is_convertible<T, float128_type>::value, float128_backend&>::type operator=(const T& i) noexcept(noexcept(std::declval<float128_type&>() = std::declval<const T&>()))
   {
      m_value = i;
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR float128_backend(long double const& f) : m_value(f)
   {
      if (::fabsl(f) > LDBL_MAX)
         m_value = (f < 0) ? -static_cast<__float128>(HUGE_VAL) : static_cast<__float128>(HUGE_VAL);
   }
   BOOST_MP_CXX14_CONSTEXPR float128_backend& operator=(long double const& f)
   {
      if (f > LDBL_MAX)
         m_value = static_cast<__float128>(HUGE_VAL);
      else if (-f > LDBL_MAX)
         m_value = -static_cast<__float128>(HUGE_VAL);
      else
         m_value = f;
      return *this;
   }
   float128_backend& operator=(const char* s)
   {
#ifndef BOOST_MP_USE_QUAD
      char* p_end;
      m_value = strtoflt128(s, &p_end);
      if (p_end - s != (std::ptrdiff_t)std::strlen(s))
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Unable to interpret input string as a floating point value"));
      }
#else
      boost::multiprecision::detail::convert_from_string(*this, s);
#endif
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR void swap(float128_backend& o) noexcept
   {
      // We don't call std::swap here because it's no constexpr (yet):
      float128_type t(o.value());
      o.value() = m_value;
      m_value = t;
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
#ifndef BOOST_MP_USE_QUAD
      char        buf[128];
      std::string format = "%";
      if (f & std::ios_base::showpos)
         format += "+";
      if (f & std::ios_base::showpoint)
         format += "#";
      format += ".*";
      if (digits == 0)
         digits = 36;
      format += "Q";

      if (f & std::ios_base::scientific)
         format += "e";
      else if (f & std::ios_base::fixed)
         format += "f";
      else
         format += "g";

      int v;
      if ((f & std::ios_base::scientific) && (f & std::ios_base::fixed))
      {
         v = quadmath_snprintf(buf, sizeof buf, "%Qa", m_value);
      }
      else
      {
         v = quadmath_snprintf(buf, sizeof buf, format.c_str(), digits, m_value);
      }

      if ((v < 0) || (v >= 127))
      {
         int                       v_max = v;
         std::unique_ptr<char[]>   buf2;
         buf2.reset(new char[v + 3]);
         v = quadmath_snprintf(&buf2[0], v_max + 3, format.c_str(), digits, m_value);
         if (v >= v_max + 3)
         {
            BOOST_THROW_EXCEPTION(std::runtime_error("Formatting of float128_type failed."));
         }
         return &buf2[0];
      }
      return buf;
#else
      return boost::multiprecision::detail::convert_to_string(*this, digits ? digits : 37, f);
#endif
   }
   BOOST_MP_CXX14_CONSTEXPR void negate() noexcept
   {
      m_value = -m_value;
   }
   BOOST_MP_CXX14_CONSTEXPR int compare(const float128_backend& o) const
   {
      return m_value == o.m_value ? 0 : m_value < o.m_value ? -1 : 1;
   }
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR int compare(const T& i) const
   {
      return m_value == i ? 0 : m_value < i ? -1 : 1;
   }
   BOOST_MP_CXX14_CONSTEXPR float128_type& value()
   {
      return m_value;
   }
   BOOST_MP_CXX14_CONSTEXPR const float128_type& value() const
   {
      return m_value;
   }
};

inline BOOST_MP_CXX14_CONSTEXPR void eval_add(float128_backend& result, const float128_backend& a)
{
   result.value() += a.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_add(float128_backend& result, const A& a)
{
   result.value() += a;
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(float128_backend& result, const float128_backend& a)
{
   result.value() -= a.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(float128_backend& result, const A& a)
{
   result.value() -= a;
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply(float128_backend& result, const float128_backend& a)
{
   result.value() *= a.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply(float128_backend& result, const A& a)
{
   result.value() *= a;
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide(float128_backend& result, const float128_backend& a)
{
   result.value() /= a.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide(float128_backend& result, const A& a)
{
   result.value() /= a;
}

inline BOOST_MP_CXX14_CONSTEXPR void eval_add(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = a.value() + b.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_add(float128_backend& result, const float128_backend& a, const A& b)
{
   result.value() = a.value() + b;
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = a.value() - b.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(float128_backend& result, const float128_backend& a, const A& b)
{
   result.value() = a.value() - b;
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_subtract(float128_backend& result, const A& a, const float128_backend& b)
{
   result.value() = a - b.value();
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = a.value() * b.value();
}
template <class A>
inline BOOST_MP_CXX14_CONSTEXPR void eval_multiply(float128_backend& result, const float128_backend& a, const A& b)
{
   result.value() = a.value() * b;
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_divide(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = a.value() / b.value();
}

template <class R>
inline BOOST_MP_CXX14_CONSTEXPR void eval_convert_to(R* result, const float128_backend& val)
{
   *result = static_cast<R>(val.value());
}

inline void eval_frexp(float128_backend& result, const float128_backend& arg, int* exp)
{
   result.value() = frexpq(arg.value(), exp);
}

inline void eval_ldexp(float128_backend& result, const float128_backend& arg, int exp)
{
   result.value() = ldexpq(arg.value(), exp);
}

inline void eval_floor(float128_backend& result, const float128_backend& arg)
{
   result.value() = floorq(arg.value());
}
inline void eval_ceil(float128_backend& result, const float128_backend& arg)
{
   result.value() = ceilq(arg.value());
}
inline void eval_sqrt(float128_backend& result, const float128_backend& arg)
{
   result.value() = sqrtq(arg.value());
}

inline void eval_rsqrt(float128_backend& result, const float128_backend& arg)
{
#if (LDBL_MANT_DIG > 100)
   // GCC can't mix and match __float128 and quad precision long double
   // error: __float128 and long double cannot be used in the same expression
   result.value() = 1 / sqrtq(arg.value());
#else
   using std::sqrt;
   if (arg.value() < std::numeric_limits<long double>::denorm_min() || arg.value() > (std::numeric_limits<long double>::max)()) {
      result.value() = 1/sqrtq(arg.value());
      return;
   }
   float128_backend xk = 1/sqrt(static_cast<long double>(arg.value()));

   // Newton iteration for f(x) = arg.value() - 1/x^2.
   BOOST_IF_CONSTEXPR (sizeof(long double) == sizeof(double)) {
       // If the long double is the same as a double, then we need two Newton iterations:
       xk.value() = xk.value() + xk.value()*(1-arg.value()*xk.value()*xk.value())/2;
       result.value() = xk.value() + xk.value()*(1-arg.value()*xk.value()*xk.value())/2;
   }
   else
   {
      // 80 bit long double only needs a single iteration to produce ~2ULPs.
      result.value() = xk.value() + xk.value() * (1 - arg.value() * xk.value() * xk.value()) / 2;
   }
#endif
}
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
inline BOOST_MP_CXX14_CONSTEXPR 
#else
inline
#endif
int eval_fpclassify(const float128_backend& arg)
{
   float128_type v = arg.value();
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   if (BOOST_MP_IS_CONST_EVALUATED(v))
   {
      if (v != v)
         return FP_NAN;
      if (v == 0)
         return FP_ZERO;
      float128_type t(v);
      if (t < 0)
         t = -t;
      if (t > BOOST_MP_QUAD_MAX)
         return FP_INFINITE;
      if (t < BOOST_MP_QUAD_MIN)
         return FP_SUBNORMAL;
      return FP_NORMAL;
   }
   else
#endif
   {
      if (isnanq(v))
         return FP_NAN;
      else if (isinfq(v))
         return FP_INFINITE;
      else if (v == 0)
         return FP_ZERO;

      float128_backend t(arg);
      if (t.value() < 0)
         t.negate();
      if (t.value() < BOOST_MP_QUAD_MIN)
         return FP_SUBNORMAL;
      return FP_NORMAL;
   }
}
#if defined(BOOST_GCC) && (__GNUC__ == 9)
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91705
inline BOOST_MP_CXX14_CONSTEXPR void eval_increment(float128_backend& arg)
{
   arg.value() = 1 + arg.value();
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_decrement(float128_backend& arg)
{
   arg.value() = arg.value() - 1;
}
#else
inline BOOST_MP_CXX14_CONSTEXPR void eval_increment(float128_backend& arg)
{
   ++arg.value();
}
inline BOOST_MP_CXX14_CONSTEXPR void eval_decrement(float128_backend& arg)
{
   --arg.value();
}
#endif

/*********************************************************************
*
* abs/fabs:
*
*********************************************************************/

#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
inline BOOST_MP_CXX14_CONSTEXPR void eval_abs(float128_backend& result, const float128_backend& arg)
#else
inline void eval_abs(float128_backend& result, const float128_backend& arg)
#endif
{
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   float128_type v(arg.value());
   if (BOOST_MP_IS_CONST_EVALUATED(v))
   {
      result.value() = v < 0 ? -v : v;
   }
   else
#endif
   {
      result.value() = fabsq(arg.value());
   }
}
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
inline BOOST_MP_CXX14_CONSTEXPR void eval_fabs(float128_backend& result, const float128_backend& arg)
#else
inline void eval_fabs(float128_backend& result, const float128_backend& arg)
#endif
{
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   float128_type v(arg.value());
   if (BOOST_MP_IS_CONST_EVALUATED(v))
   {
      result.value() = v < 0 ? -v : v;
   }
   else
#endif
   {
      result.value() = fabsq(arg.value());
   }
}

/*********************************************************************
*
* Floating point functions:
*
*********************************************************************/

inline void eval_trunc(float128_backend& result, const float128_backend& arg)
{
   result.value() = truncq(arg.value());
}
/*
// 
// This doesn't actually work... rely on our own default version instead.
//
inline void eval_round(float128_backend& result, const float128_backend& arg)
{
   if(isnanq(arg.value()) || isinf(arg.value()))
   {
      result = boost::math::policies::raise_rounding_error(
            "boost::multiprecision::trunc<%1%>(%1%)", 0, 
            number<float128_backend, et_off>(arg), 
            number<float128_backend, et_off>(arg), 
            boost::math::policies::policy<>()).backend();
      return;
   }
   result.value() = roundq(arg.value());
}
*/

inline void eval_exp(float128_backend& result, const float128_backend& arg)
{
   result.value() = expq(arg.value());
}
inline void eval_log(float128_backend& result, const float128_backend& arg)
{
   result.value() = logq(arg.value());
}
inline void eval_log10(float128_backend& result, const float128_backend& arg)
{
   result.value() = log10q(arg.value());
}
inline void eval_sin(float128_backend& result, const float128_backend& arg)
{
   result.value() = sinq(arg.value());
}
inline void eval_cos(float128_backend& result, const float128_backend& arg)
{
   result.value() = cosq(arg.value());
}
inline void eval_tan(float128_backend& result, const float128_backend& arg)
{
   result.value() = tanq(arg.value());
}
inline void eval_asin(float128_backend& result, const float128_backend& arg)
{
   result.value() = asinq(arg.value());
}
inline void eval_acos(float128_backend& result, const float128_backend& arg)
{
   result.value() = acosq(arg.value());
}
inline void eval_atan(float128_backend& result, const float128_backend& arg)
{
   result.value() = atanq(arg.value());
}
inline void eval_sinh(float128_backend& result, const float128_backend& arg)
{
   result.value() = sinhq(arg.value());
}
inline void eval_cosh(float128_backend& result, const float128_backend& arg)
{
   result.value() = coshq(arg.value());
}
inline void eval_tanh(float128_backend& result, const float128_backend& arg)
{
   result.value() = tanhq(arg.value());
}
inline void eval_fmod(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = fmodq(a.value(), b.value());
}
inline void eval_pow(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = powq(a.value(), b.value());
}
inline void eval_atan2(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = atan2q(a.value(), b.value());
}
#ifndef BOOST_MP_USE_QUAD
inline void eval_multiply_add(float128_backend& result, const float128_backend& a, const float128_backend& b, const float128_backend& c)
{
   result.value() = fmaq(a.value(), b.value(), c.value());
}
inline int eval_signbit BOOST_PREVENT_MACRO_SUBSTITUTION(const float128_backend& arg)
{
   return ::signbitq(arg.value());
}
#endif

inline std::size_t hash_value(const float128_backend& val)
{
   return boost::multiprecision::detail::hash_value(static_cast<double>(val.value()));
}

} // namespace backends

template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return asinhq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return acoshq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return atanhq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return cbrtq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return erfq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return erfcq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return expm1q(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return lgammaq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   if(eval_signbit(arg.backend()) != 0)
   {
      const bool result_is_neg = ((static_cast<unsigned long long>(floorq(-arg.backend().value())) % 2U) == 0U);

      const boost::multiprecision::number<float128_backend, ExpressionTemplates> result_of_tgammaq = fabsq(tgammaq(arg.backend().value()));

      return ((result_is_neg == false) ? result_of_tgammaq : -result_of_tgammaq);
   }
   else
   {
      return tgammaq(arg.backend().value());
   }
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   return log1pq(arg.backend().value());
}
template <boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<float128_backend, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<float128_backend, ExpressionTemplates>& arg)
{
   boost::multiprecision::number<float128_backend, ExpressionTemplates> res;
   eval_rsqrt(res.backend(), arg.backend());
   return res;
}

#ifndef BOOST_MP_USE_QUAD
template <multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates>& a, const boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates>& b)
{
   return ::copysignq(a.backend().value(), b.backend().value());
}

inline void eval_remainder(float128_backend& result, const float128_backend& a, const float128_backend& b)
{
   result.value() = remainderq(a.value(), b.value());
}
inline void eval_remainder(float128_backend& result, const float128_backend& a, const float128_backend& b, int* pi)
{
   result.value() = remquoq(a.value(), b.value(), pi);
}
#endif

} // namespace multiprecision

namespace math {

using boost::multiprecision::copysign;
using boost::multiprecision::signbit;

} // namespace math

} // namespace boost

namespace boost {
namespace archive {

class binary_oarchive;
class binary_iarchive;

} // namespace archive

namespace serialization {
namespace float128_detail {

template <class Archive>
void do_serialize(Archive& ar, boost::multiprecision::backends::float128_backend& val, const std::integral_constant<bool, false>&, const std::integral_constant<bool, false>&)
{
   // saving
   // non-binary
   std::string s(val.str(0, std::ios_base::scientific));
   ar&         boost::make_nvp("value", s);
}
template <class Archive>
void do_serialize(Archive& ar, boost::multiprecision::backends::float128_backend& val, const std::integral_constant<bool, true>&, const std::integral_constant<bool, false>&)
{
   // loading
   // non-binary
   std::string s;
   ar&         boost::make_nvp("value", s);
   val = s.c_str();
}

template <class Archive>
void do_serialize(Archive& ar, boost::multiprecision::backends::float128_backend& val, const std::integral_constant<bool, false>&, const std::integral_constant<bool, true>&)
{
   // saving
   // binary
   ar.save_binary(&val, sizeof(val));
}
template <class Archive>
void do_serialize(Archive& ar, boost::multiprecision::backends::float128_backend& val, const std::integral_constant<bool, true>&, const std::integral_constant<bool, true>&)
{
   // loading
   // binary
   ar.load_binary(&val, sizeof(val));
}

} // namespace float128_detail

template <class Archive>
void serialize(Archive& ar, boost::multiprecision::backends::float128_backend& val, unsigned int /*version*/)
{
   using load_tag = typename Archive::is_loading                                                                                                                                         ;
   using loading = std::integral_constant<bool, load_tag::value>                                                                                                                        ;
   using binary_tag = typename std::integral_constant<bool, std::is_same<Archive, boost::archive::binary_oarchive>::value || std::is_same<Archive, boost::archive::binary_iarchive>::value>;

   float128_detail::do_serialize(ar, val, loading(), binary_tag());
}

} // namespace serialization

} // namespace boost

namespace std {

template <boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >
{
   using number_type = boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates>;

 public:
   static constexpr bool is_specialized = true;
   static BOOST_MP_CXX14_CONSTEXPR number_type(min)() noexcept { return BOOST_MP_QUAD_MIN; }
   static BOOST_MP_CXX14_CONSTEXPR number_type(max)() noexcept { return BOOST_MP_QUAD_MAX; }
   static BOOST_MP_CXX14_CONSTEXPR number_type          lowest() noexcept { return -(max)(); }
   static constexpr int  digits       = 113;
   static constexpr int  digits10     = 33;
   static constexpr int  max_digits10 = 36;
   static constexpr bool is_signed    = true;
   static constexpr bool is_integer   = false;
   static constexpr bool is_exact     = false;
   static constexpr int  radix        = 2;
   static BOOST_MP_CXX14_CONSTEXPR number_type          epsilon() { return 1.92592994438723585305597794258492732e-34; /* this double value has only one bit set and so is exact */ }
   static BOOST_MP_CXX14_CONSTEXPR number_type          round_error() { return 0.5; }
   static constexpr int  min_exponent                  = -16381;
   static constexpr int  min_exponent10                = min_exponent * 301L / 1000L;
   static constexpr int  max_exponent                  = 16384;
   static constexpr int  max_exponent10                = max_exponent * 301L / 1000L;
   static constexpr bool has_infinity                  = true;
   static constexpr bool has_quiet_NaN                 = true;
   static constexpr bool has_signaling_NaN             = false;
   static constexpr float_denorm_style has_denorm      = denorm_present;
   static constexpr bool               has_denorm_loss = true;
   static BOOST_MP_CXX14_CONSTEXPR number_type                        infinity() { return HUGE_VAL; /* conversion from double infinity OK */ }
   static BOOST_MP_CXX14_CONSTEXPR number_type                        quiet_NaN() { return number_type("nan"); }
   static BOOST_MP_CXX14_CONSTEXPR number_type                        signaling_NaN() { return 0; }
   static BOOST_MP_CXX14_CONSTEXPR number_type                        denorm_min() { return BOOST_MP_QUAD_DENORM_MIN; }
   static constexpr bool               is_iec559       = true;
   static constexpr bool               is_bounded      = true;
   static constexpr bool               is_modulo       = false;
   static constexpr bool               traps           = false;
   static constexpr bool               tinyness_before = false;
   static constexpr float_round_style round_style      = round_to_nearest;
};

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_specialized;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::digits;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::digits10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::max_digits10;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_signed;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_integer;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_exact;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::radix;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::min_exponent;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::max_exponent;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::min_exponent10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::max_exponent10;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::has_infinity;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::has_quiet_NaN;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::has_signaling_NaN;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::has_denorm_loss;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_iec559;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_bounded;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::is_modulo;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::traps;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::tinyness_before;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_round_style numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::round_style;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_denorm_style numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::float128_backend, ExpressionTemplates> >::has_denorm;

} // namespace std

#endif

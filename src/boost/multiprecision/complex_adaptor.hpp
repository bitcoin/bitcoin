///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MULTIPRECISION_COMPLEX_ADAPTOR_HPP
#define BOOST_MULTIPRECISION_COMPLEX_ADAPTOR_HPP

#include <boost/multiprecision/number.hpp>
#include <cstdint>
#include <boost/multiprecision/detail/digits.hpp>
#include <boost/multiprecision/detail/hash.hpp>
#include <cmath>
#include <algorithm>
#include <complex>

namespace boost {
namespace multiprecision {
namespace backends {

template <class Backend>
struct debug_adaptor;

template <class Backend>
struct logged_adaptor;

template <class Backend>
struct complex_adaptor
{
 protected:
   Backend m_real, m_imag;

 public:
   Backend& real_data()
   {
      return m_real;
   }
   const Backend& real_data() const
   {
      return m_real;
   }
   Backend& imag_data()
   {
      return m_imag;
   }
   const Backend& imag_data() const
   {
      return m_imag;
   }

   using signed_types = typename Backend::signed_types  ;
   using unsigned_types = typename Backend::unsigned_types;
   using float_types = typename Backend::float_types   ;
   using exponent_type = typename Backend::exponent_type ;

   complex_adaptor() {}
   complex_adaptor(const complex_adaptor& o) : m_real(o.real_data()), m_imag(o.imag_data()) {}
   // Rvalue construct:
   complex_adaptor(complex_adaptor&& o) : m_real(std::move(o.real_data())), m_imag(std::move(o.imag_data()))
   {}
   complex_adaptor(const Backend& val)
       : m_real(val)
   {}

   complex_adaptor(const std::complex<float>& val)
   {
      m_real = (long double)val.real();
      m_imag = (long double)val.imag();
   }
   complex_adaptor(const std::complex<double>& val)
   {
      m_real = (long double)val.real();
      m_imag = (long double)val.imag();
   }
   complex_adaptor(const std::complex<long double>& val)
   {
      m_real = val.real();
      m_imag = val.imag();
   }

   complex_adaptor& operator=(const complex_adaptor& o)
   {
      m_real = o.real_data();
      m_imag = o.imag_data();
      return *this;
   }
   // rvalue assign:
   complex_adaptor& operator=(complex_adaptor&& o) noexcept
   {
      m_real = std::move(o.real_data());
      m_imag = std::move(o.imag_data());
      return *this;
   }
   template <class V>
   complex_adaptor& operator=(const V& v)
   {
      using ui_type = typename std::tuple_element<0, unsigned_types>::type;
      m_real = v;
      m_imag = ui_type(0u);
      return *this;
   }
   template <class T>
   complex_adaptor& operator=(const std::complex<T>& val)
   {
      m_real = (long double)val.real();
      m_imag = (long double)val.imag();
      return *this;
   }
   complex_adaptor& operator=(const char* s)
   {
      using ui_type = typename std::tuple_element<0, unsigned_types>::type;
      ui_type                                           zero = 0u;

      using default_ops::eval_fpclassify;

      if (s && (*s == '('))
      {
         std::string part;
         const char* p = ++s;
         while (*p && (*p != ',') && (*p != ')'))
            ++p;
         part.assign(s, p);
         if (part.size())
            real_data() = part.c_str();
         else
            real_data() = zero;
         s = p;
         if (*p && (*p != ')'))
         {
            ++p;
            while (*p && (*p != ')'))
               ++p;
            part.assign(s + 1, p);
         }
         else
            part.erase();
         if (part.size())
            imag_data() = part.c_str();
         else
            imag_data() = zero;

         if (eval_fpclassify(imag_data()) == (int)FP_NAN)
         {
            real_data() = imag_data();
         }
      }
      else
      {
         real_data() = s;
         imag_data() = zero;
      }
      return *this;
   }

   int compare(const complex_adaptor& o) const
   {
      // They are either equal or not:
      return (m_real.compare(o.real_data()) == 0) && (m_imag.compare(o.imag_data()) == 0) ? 0 : 1;
   }
   template <class T>
   int compare(const T& val) const
   {
      using default_ops::eval_is_zero;
      return (m_real.compare(val) == 0) && eval_is_zero(m_imag) ? 0 : 1;
   }
   void swap(complex_adaptor& o)
   {
      real_data().swap(o.real_data());
      imag_data().swap(o.imag_data());
   }
   std::string str(std::streamsize dig, std::ios_base::fmtflags f) const
   {
      using default_ops::eval_is_zero;
      if (eval_is_zero(imag_data()))
         return m_real.str(dig, f);
      return "(" + m_real.str(dig, f) + "," + m_imag.str(dig, f) + ")";
   }
   void negate()
   {
      m_real.negate();
      m_imag.negate();
   }
};

template <class Backend, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_eq(const complex_adaptor<Backend>& a, const T& b) noexcept
{
   return a.compare(b) == 0;
}

template <class Backend>
inline void eval_add(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& o)
{
   eval_add(result.real_data(), o.real_data());
   eval_add(result.imag_data(), o.imag_data());
}
template <class Backend>
inline void eval_subtract(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& o)
{
   eval_subtract(result.real_data(), o.real_data());
   eval_subtract(result.imag_data(), o.imag_data());
}
template <class Backend>
inline void eval_multiply(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& o)
{
   Backend t1, t2, t3;
   eval_multiply(t1, result.real_data(), o.real_data());
   eval_multiply(t2, result.imag_data(), o.imag_data());
   eval_subtract(t3, t1, t2);
   eval_multiply(t1, result.real_data(), o.imag_data());
   eval_multiply(t2, result.imag_data(), o.real_data());
   eval_add(t1, t2);
   result.real_data() = std::move(t3);
   result.imag_data() = std::move(t1);
}
template <class Backend>
inline void eval_divide(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& z)
{
   // (a+bi) / (c + di)
   using default_ops::eval_add;
   using default_ops::eval_divide;
   using default_ops::eval_fabs;
   using default_ops::eval_is_zero;
   using default_ops::eval_multiply;
   using default_ops::eval_subtract;
   Backend t1, t2;

   if (eval_is_zero(z.imag_data()))
   {
      eval_divide(result.real_data(), z.real_data());
      eval_divide(result.imag_data(), z.real_data());
      return;
   }

   eval_fabs(t1, z.real_data());
   eval_fabs(t2, z.imag_data());
   if (t1.compare(t2) < 0)
   {
      eval_divide(t1, z.real_data(), z.imag_data()); // t1 = c/d
      eval_multiply(t2, z.real_data(), t1);
      eval_add(t2, z.imag_data()); // denom = c * (c/d) + d
      Backend t_real(result.real_data());
      // real = (a * (c/d) + b) / (denom)
      eval_multiply(result.real_data(), t1);
      eval_add(result.real_data(), result.imag_data());
      eval_divide(result.real_data(), t2);
      // imag = (b * c/d - a) / denom
      eval_multiply(result.imag_data(), t1);
      eval_subtract(result.imag_data(), t_real);
      eval_divide(result.imag_data(), t2);
   }
   else
   {
      eval_divide(t1, z.imag_data(), z.real_data()); // t1 = d/c
      eval_multiply(t2, z.imag_data(), t1);
      eval_add(t2, z.real_data()); // denom = d * d/c + c

      Backend r_t(result.real_data());
      Backend i_t(result.imag_data());

      // real = (b * d/c + a) / denom
      eval_multiply(result.real_data(), result.imag_data(), t1);
      eval_add(result.real_data(), r_t);
      eval_divide(result.real_data(), t2);
      // imag = (-a * d/c + b) / denom
      eval_multiply(result.imag_data(), r_t, t1);
      result.imag_data().negate();
      eval_add(result.imag_data(), i_t);
      eval_divide(result.imag_data(), t2);
   }
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_add(complex_adaptor<Backend>& result, const T& scalar)
{
   using default_ops::eval_add;
   eval_add(result.real_data(), scalar);
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_subtract(complex_adaptor<Backend>& result, const T& scalar)
{
   using default_ops::eval_subtract;
   eval_subtract(result.real_data(), scalar);
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_multiply(complex_adaptor<Backend>& result, const T& scalar)
{
   using default_ops::eval_multiply;
   eval_multiply(result.real_data(), scalar);
   eval_multiply(result.imag_data(), scalar);
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_divide(complex_adaptor<Backend>& result, const T& scalar)
{
   using default_ops::eval_divide;
   eval_divide(result.real_data(), scalar);
   eval_divide(result.imag_data(), scalar);
}
// Optimised 3 arg versions:
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_add(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& a, const T& scalar)
{
   using default_ops::eval_add;
   eval_add(result.real_data(), a.real_data(), scalar);
   result.imag_data() = a.imag_data();
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_subtract(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& a, const T& scalar)
{
   using default_ops::eval_subtract;
   eval_subtract(result.real_data(), a.real_data(), scalar);
   result.imag_data() = a.imag_data();
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_multiply(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& a, const T& scalar)
{
   using default_ops::eval_multiply;
   eval_multiply(result.real_data(), a.real_data(), scalar);
   eval_multiply(result.imag_data(), a.imag_data(), scalar);
}
template <class Backend, class T>
inline typename std::enable_if< !std::is_same<complex_adaptor<Backend>, T>::value>::type eval_divide(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& a, const T& scalar)
{
   using default_ops::eval_divide;
   eval_divide(result.real_data(), a.real_data(), scalar);
   eval_divide(result.imag_data(), a.imag_data(), scalar);
}

template <class Backend>
inline bool eval_is_zero(const complex_adaptor<Backend>& val) noexcept
{
   using default_ops::eval_is_zero;
   return eval_is_zero(val.real_data()) && eval_is_zero(val.imag_data());
}
template <class Backend>
inline int eval_get_sign(const complex_adaptor<Backend>&)
{
   static_assert(sizeof(Backend) == UINT_MAX, "Complex numbers have no sign bit."); // designed to always fail
   return 0;
}

template <class Result, class Backend>
inline typename std::enable_if< !boost::multiprecision::detail::is_complex<Result>::value>::type eval_convert_to(Result* result, const complex_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   using default_ops::eval_is_zero;
   if (!eval_is_zero(val.imag_data()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   eval_convert_to(result, val.real_data());
}

template <class Backend, class T>
inline void assign_components(complex_adaptor<Backend>& result, const T& a, const T& b)
{
   result.real_data() = a;
   result.imag_data() = b;
}

//
// Native non-member operations:
//
template <class Backend>
inline void eval_sqrt(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& val)
{
   // Use the following:
   // sqrt(z) = (s, zi / 2s)       for zr >= 0
   //           (|zi| / 2s, +-s)   for zr <  0
   // where s = sqrt{ [ |zr| + sqrt(zr^2 + zi^2) ] / 2 },
   // and the +- sign is the same as the sign of zi.
   using default_ops::eval_abs;
   using default_ops::eval_add;
   using default_ops::eval_divide;
   using default_ops::eval_get_sign;
   using default_ops::eval_is_zero;

   if (eval_is_zero(val.imag_data()) && (eval_get_sign(val.real_data()) >= 0))
   {
      constexpr const typename std::tuple_element<0, typename Backend::unsigned_types>::type zero = 0u;
      eval_sqrt(result.real_data(), val.real_data());
      result.imag_data() = zero;
      return;
   }

   const bool __my_real_part_is_neg(eval_get_sign(val.real_data()) < 0);

   Backend __my_real_part_fabs(val.real_data());
   if (__my_real_part_is_neg)
      __my_real_part_fabs.negate();

   Backend t, __my_sqrt_part;
   eval_abs(__my_sqrt_part, val);
   eval_add(__my_sqrt_part, __my_real_part_fabs);
   eval_ldexp(t, __my_sqrt_part, -1);
   eval_sqrt(__my_sqrt_part, t);

   if (__my_real_part_is_neg == false)
   {
      eval_ldexp(t, __my_sqrt_part, 1);
      eval_divide(result.imag_data(), val.imag_data(), t);
      result.real_data() = __my_sqrt_part;
   }
   else
   {
      const bool __my_imag_part_is_neg(eval_get_sign(val.imag_data()) < 0);

      Backend __my_imag_part_fabs(val.imag_data());
      if (__my_imag_part_is_neg)
         __my_imag_part_fabs.negate();

      eval_ldexp(t, __my_sqrt_part, 1);
      eval_divide(result.real_data(), __my_imag_part_fabs, t);
      if (__my_imag_part_is_neg)
         __my_sqrt_part.negate();
      result.imag_data() = __my_sqrt_part;
   }
}

template <class Backend>
inline void eval_abs(Backend& result, const complex_adaptor<Backend>& val)
{
   Backend t1, t2;
   eval_multiply(t1, val.real_data(), val.real_data());
   eval_multiply(t2, val.imag_data(), val.imag_data());
   eval_add(t1, t2);
   eval_sqrt(result, t1);
}

template <class Backend>
inline void eval_pow(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& b, const complex_adaptor<Backend>& e)
{
   using default_ops::eval_acos;
   using default_ops::eval_cos;
   using default_ops::eval_exp;
   using default_ops::eval_get_sign;
   using default_ops::eval_is_zero;
   using default_ops::eval_multiply;
   using default_ops::eval_sin;

   if (eval_is_zero(e))
   {
      typename std::tuple_element<0, typename Backend::unsigned_types>::type one(1);
      result = one;
      return;
   }
   else if (eval_is_zero(b))
   {
      if (eval_is_zero(e.real_data()))
      {
         Backend n          = std::numeric_limits<number<Backend> >::quiet_NaN().backend();
         result.real_data() = n;
         result.imag_data() = n;
      }
      else if (eval_get_sign(e.real_data()) < 0)
      {
         Backend n          = std::numeric_limits<number<Backend> >::infinity().backend();
         result.real_data() = n;
         typename std::tuple_element<0, typename Backend::unsigned_types>::type zero(0);
         if (eval_is_zero(e.imag_data()))
            result.imag_data() = zero;
         else
            result.imag_data() = n;
      }
      else
      {
         typename std::tuple_element<0, typename Backend::unsigned_types>::type zero(0);
         result = zero;
      }
      return;
   }
   complex_adaptor<Backend> t;
   eval_log(t, b);
   eval_multiply(t, e);
   eval_exp(result, t);
}

template <class Backend>
inline void eval_exp(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_cos;
   using default_ops::eval_exp;
   using default_ops::eval_is_zero;
   using default_ops::eval_multiply;
   using default_ops::eval_sin;

   if (eval_is_zero(arg.imag_data()))
   {
      eval_exp(result.real_data(), arg.real_data());
      typename std::tuple_element<0, typename Backend::unsigned_types>::type zero(0);
      result.imag_data() = zero;
      return;
   }
   eval_cos(result.real_data(), arg.imag_data());
   eval_sin(result.imag_data(), arg.imag_data());
   Backend e;
   eval_exp(e, arg.real_data());
   if (eval_is_zero(result.real_data()))
      eval_multiply(result.imag_data(), e);
   else if (eval_is_zero(result.imag_data()))
      eval_multiply(result.real_data(), e);
   else
      eval_multiply(result, e);
}

template <class Backend>
inline void eval_log(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_add;
   using default_ops::eval_atan2;
   using default_ops::eval_get_sign;
   using default_ops::eval_is_zero;
   using default_ops::eval_log;
   using default_ops::eval_multiply;

   if (eval_is_zero(arg.imag_data()) && (eval_get_sign(arg.real_data()) >= 0))
   {
      eval_log(result.real_data(), arg.real_data());
      typename std::tuple_element<0, typename Backend::unsigned_types>::type zero(0);
      result.imag_data() = zero;
      return;
   }

   Backend t1, t2;
   eval_multiply(t1, arg.real_data(), arg.real_data());
   eval_multiply(t2, arg.imag_data(), arg.imag_data());
   eval_add(t1, t2);
   eval_log(t2, t1);
   eval_ldexp(result.real_data(), t2, -1);
   eval_atan2(result.imag_data(), arg.imag_data(), arg.real_data());
}

template <class Backend>
inline void eval_log10(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_divide;
   using default_ops::eval_log;

   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;

   Backend ten;
   ten = ui_type(10);
   Backend l_ten;
   eval_log(l_ten, ten);
   eval_log(result, arg);
   eval_divide(result, l_ten);
}

template <class Backend>
inline void eval_sin(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_cos;
   using default_ops::eval_cosh;
   using default_ops::eval_sin;
   using default_ops::eval_sinh;

   Backend t1, t2, t3;
   eval_sin(t1, arg.real_data());
   eval_cosh(t2, arg.imag_data());
   eval_multiply(t3, t1, t2);

   eval_cos(t1, arg.real_data());
   eval_sinh(t2, arg.imag_data());
   eval_multiply(result.imag_data(), t1, t2);
   result.real_data() = t3;
}

template <class Backend>
inline void eval_cos(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_cos;
   using default_ops::eval_cosh;
   using default_ops::eval_sin;
   using default_ops::eval_sinh;

   Backend t1, t2, t3;
   eval_cos(t1, arg.real_data());
   eval_cosh(t2, arg.imag_data());
   eval_multiply(t3, t1, t2);

   eval_sin(t1, arg.real_data());
   eval_sinh(t2, arg.imag_data());
   eval_multiply(result.imag_data(), t1, t2);
   result.imag_data().negate();
   result.real_data() = t3;
}

template <class T>
void tanh_imp(const T& r, const T& i, T& r_result, T& i_result)
{
   using default_ops::eval_tan;
   using default_ops::eval_sinh;
   using default_ops::eval_add;
   using default_ops::eval_fpclassify;
   using default_ops::eval_get_sign;

   using ui_type = typename std::tuple_element<0, typename T::unsigned_types>::type;
   ui_type one(1);
   //
   // Set:
   // t = tan(i);
   // s = sinh(r);
   // b = s * (1 + t^2);
   // d = 1 + b * s;
   //
   T t, s, b, d;
   eval_tan(t, i);
   eval_sinh(s, r);
   eval_multiply(d, t, t);
   eval_add(d, one);
   eval_multiply(b, d, s);
   eval_multiply(d, b, s);
   eval_add(d, one);

   if (eval_fpclassify(d) == FP_INFINITE)
   {
      r_result = one;
      if (eval_get_sign(s) < 0)
         r_result.negate();
      //
      // Imaginary part is a signed zero:
      //
      ui_type zero(0);
      i_result = zero;
      if (eval_get_sign(t) < 0)
         i_result.negate();
   }
   //
   // Real part is sqrt(1 + s^2) * b / d;
   // Imaginary part is t / d;
   //
   eval_divide(i_result, t, d);
   //
   // variable t is now spare, as is r_result.
   //
   eval_multiply(t, s, s);
   eval_add(t, one);
   eval_sqrt(r_result, t);
   eval_multiply(t, r_result, b);
   eval_divide(r_result, t, d);
}

template <class Backend>
inline void eval_tanh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   tanh_imp(arg.real_data(), arg.imag_data(), result.real_data(), result.imag_data());
}
template <class Backend>
inline void eval_tan(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   Backend t(arg.imag_data());
   t.negate();
   tanh_imp(t, arg.real_data(), result.imag_data(), result.real_data());
   result.imag_data().negate();
}

template <class Backend>
inline void eval_asin(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_add;
   using default_ops::eval_multiply;

   if (eval_is_zero(arg))
   {
      result = arg;
      return;
   }

   complex_adaptor<Backend> t1, t2;
   assign_components(t1, arg.imag_data(), arg.real_data());
   t1.real_data().negate();
   eval_asinh(t2, t1);

   assign_components(result, t2.imag_data(), t2.real_data());
   result.imag_data().negate();
}

template <class Backend>
inline void eval_acos(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;

   using default_ops::eval_asin;

   Backend half_pi, t1;
   t1 = static_cast<ui_type>(1u);
   eval_asin(half_pi, t1);
   eval_asin(result, arg);
   result.negate();
   eval_add(result.real_data(), half_pi);
}

template <class Backend>
inline void eval_atan(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   ui_type                                                             one = (ui_type)1u;

   using default_ops::eval_add;
   using default_ops::eval_is_zero;
   using default_ops::eval_log;
   using default_ops::eval_subtract;

   complex_adaptor<Backend> __my_z_times_i, t1, t2, t3;
   assign_components(__my_z_times_i, arg.imag_data(), arg.real_data());
   __my_z_times_i.real_data().negate();

   eval_add(t1, __my_z_times_i, one);
   eval_log(t2, t1);
   eval_subtract(t1, one, __my_z_times_i);
   eval_log(t3, t1);
   eval_subtract(t1, t3, t2);

   eval_ldexp(result.real_data(), t1.imag_data(), -1);
   eval_ldexp(result.imag_data(), t1.real_data(), -1);
   if (!eval_is_zero(result.real_data()))
      result.real_data().negate();
}

template <class Backend>
inline void eval_sinh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_cos;
   using default_ops::eval_cosh;
   using default_ops::eval_sin;
   using default_ops::eval_sinh;

   Backend t1, t2, t3;
   eval_cos(t1, arg.imag_data());
   eval_sinh(t2, arg.real_data());
   eval_multiply(t3, t1, t2);

   eval_cosh(t1, arg.real_data());
   eval_sin(t2, arg.imag_data());
   eval_multiply(result.imag_data(), t1, t2);
   result.real_data() = t3;
}

template <class Backend>
inline void eval_cosh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_cos;
   using default_ops::eval_cosh;
   using default_ops::eval_sin;
   using default_ops::eval_sinh;

   Backend t1, t2, t3;
   eval_cos(t1, arg.imag_data());
   eval_cosh(t2, arg.real_data());
   eval_multiply(t3, t1, t2);

   eval_sin(t1, arg.imag_data());
   eval_sinh(t2, arg.real_data());
   eval_multiply(result.imag_data(), t1, t2);
   result.real_data() = t3;
}

template <class Backend>
inline void eval_asinh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   ui_type                                                             one = (ui_type)1u;

   using default_ops::eval_add;
   using default_ops::eval_log;
   using default_ops::eval_multiply;

   complex_adaptor<Backend> t1, t2;
   eval_multiply(t1, arg, arg);
   eval_add(t1, one);
   eval_sqrt(t2, t1);
   eval_add(t2, arg);
   eval_log(result, t2);
}

template <class Backend>
inline void eval_acosh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   ui_type                                                             one = (ui_type)1u;

   using default_ops::eval_add;
   using default_ops::eval_divide;
   using default_ops::eval_log;
   using default_ops::eval_multiply;
   using default_ops::eval_subtract;

   complex_adaptor<Backend> __my_zp(arg);
   eval_add(__my_zp.real_data(), one);
   complex_adaptor<Backend> __my_zm(arg);
   eval_subtract(__my_zm.real_data(), one);

   complex_adaptor<Backend> t1, t2;
   eval_divide(t1, __my_zm, __my_zp);
   eval_sqrt(t2, t1);
   eval_multiply(t2, __my_zp);
   eval_add(t2, arg);
   eval_log(result, t2);
}

template <class Backend>
inline void eval_atanh(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   ui_type                                                             one = (ui_type)1u;

   using default_ops::eval_add;
   using default_ops::eval_divide;
   using default_ops::eval_log;
   using default_ops::eval_multiply;
   using default_ops::eval_subtract;

   complex_adaptor<Backend> t1, t2, t3;
   eval_add(t1, arg, one);
   eval_log(t2, t1);
   eval_subtract(t1, one, arg);
   eval_log(t3, t1);
   eval_subtract(t2, t3);

   eval_ldexp(result.real_data(), t2.real_data(), -1);
   eval_ldexp(result.imag_data(), t2.imag_data(), -1);
}

template <class Backend>
inline void eval_conj(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   result = arg;
   result.imag_data().negate();
}

template <class Backend>
inline void eval_proj(complex_adaptor<Backend>& result, const complex_adaptor<Backend>& arg)
{
   using default_ops::eval_get_sign;

   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   ui_type                                                             zero = (ui_type)0u;

   int c1 = eval_fpclassify(arg.real_data());
   int c2 = eval_fpclassify(arg.imag_data());
   if (c1 == FP_INFINITE)
   {
      result.real_data() = arg.real_data();
      if (eval_get_sign(result.real_data()) < 0)
         result.real_data().negate();
      result.imag_data() = zero;
      if (eval_get_sign(arg.imag_data()) < 0)
         result.imag_data().negate();
   }
   else if (c2 == FP_INFINITE)
   {
      result.real_data() = arg.imag_data();
      if (eval_get_sign(result.real_data()) < 0)
         result.real_data().negate();
      result.imag_data() = zero;
      if (eval_get_sign(arg.imag_data()) < 0)
         result.imag_data().negate();
   }
   else
      result = arg;
}

template <class Backend>
inline void eval_real(Backend& result, const complex_adaptor<Backend>& arg)
{
   result = arg.real_data();
}
template <class Backend>
inline void eval_imag(Backend& result, const complex_adaptor<Backend>& arg)
{
   result = arg.imag_data();
}

template <class Backend, class T>
inline void eval_set_imag(complex_adaptor<Backend>& result, const T& arg)
{
   result.imag_data() = arg;
}

template <class Backend, class T>
inline void eval_set_real(complex_adaptor<Backend>& result, const T& arg)
{
   result.real_data() = arg;
}

template <class Backend>
inline std::size_t hash_value(const complex_adaptor<Backend>& val)
{
   std::size_t result  = hash_value(val.real_data());
   std::size_t result2 = hash_value(val.imag_data());
   boost::multiprecision::detail::hash_combine(result, result2);
   return result;
}

} // namespace backends

using boost::multiprecision::backends::complex_adaptor;

template <class Backend>
struct number_category<complex_adaptor<Backend> > : public std::integral_constant<int, boost::multiprecision::number_kind_complex>
{};

template <class Backend, expression_template_option ExpressionTemplates>
struct component_type<number<complex_adaptor<Backend>, ExpressionTemplates> >
{
   using type = number<Backend, ExpressionTemplates>;
};

template <class Backend, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<Backend, ExpressionTemplates> >
{
   using type = number<complex_adaptor<Backend>, ExpressionTemplates>;
};

namespace detail {
   template <class Backend>
   struct is_variable_precision<complex_adaptor<Backend> > : public is_variable_precision<Backend>
   {};
} // namespace detail



template <class Backend, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<backends::debug_adaptor<Backend>, ExpressionTemplates> >
{
   using type = number<backends::debug_adaptor<complex_adaptor<Backend> >, ExpressionTemplates>;
};

template <class Backend, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<backends::logged_adaptor<Backend>, ExpressionTemplates> >
{
   using type = number<backends::logged_adaptor<complex_adaptor<Backend> >, ExpressionTemplates>;
};

}

} // namespace boost::multiprecision

#endif

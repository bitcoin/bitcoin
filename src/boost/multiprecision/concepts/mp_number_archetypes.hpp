///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MATH_CONCEPTS_ER_HPP
#define BOOST_MATH_CONCEPTS_ER_HPP

#include <iostream>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <functional>
#include <cmath>
#include <cstdint>
#include <boost/multiprecision/number.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost {
namespace multiprecision {
namespace concepts {

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

struct number_backend_float_architype
{
   using signed_types = std::tuple<boost::long_long_type> ;
   using unsigned_types = std::tuple<boost::ulong_long_type>;
   using float_types = std::tuple<long double>           ;
   using exponent_type = int                              ;

   number_backend_float_architype()
   {
      m_value = 0;
      std::cout << "Default construct" << std::endl;
   }
   number_backend_float_architype(const number_backend_float_architype& o)
   {
      std::cout << "Copy construct" << std::endl;
      m_value = o.m_value;
   }
   number_backend_float_architype& operator=(const number_backend_float_architype& o)
   {
      m_value = o.m_value;
      std::cout << "Assignment (" << m_value << ")" << std::endl;
      return *this;
   }
   number_backend_float_architype& operator=(boost::ulong_long_type i)
   {
      m_value = i;
      std::cout << "UInt Assignment (" << i << ")" << std::endl;
      return *this;
   }
   number_backend_float_architype& operator=(boost::long_long_type i)
   {
      m_value = i;
      std::cout << "Int Assignment (" << i << ")" << std::endl;
      return *this;
   }
   number_backend_float_architype& operator=(long double d)
   {
      m_value = d;
      std::cout << "long double Assignment (" << d << ")" << std::endl;
      return *this;
   }
   number_backend_float_architype& operator=(const char* s)
   {
#ifndef BOOST_NO_EXCEPTIONS
      try
      {
#endif
         m_value = boost::lexical_cast<long double>(s);
#ifndef BOOST_NO_EXCEPTIONS
      }
      catch (const std::exception&)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Unable to parse input string: \"") + s + std::string("\" as a valid floating point number.")));
      }
#endif
      std::cout << "const char* Assignment (" << s << ")" << std::endl;
      return *this;
   }
   void swap(number_backend_float_architype& o)
   {
      std::cout << "Swapping (" << m_value << " with " << o.m_value << ")" << std::endl;
      std::swap(m_value, o.m_value);
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      std::stringstream ss;
      ss.flags(f);
      if (digits)
         ss.precision(digits);
      else
         ss.precision(std::numeric_limits<long double>::digits10 + 3);
      std::intmax_t  i = m_value;
      std::uintmax_t u = m_value;
      if (!(f & std::ios_base::scientific) && m_value == i)
         ss << i;
      else if (!(f & std::ios_base::scientific) && m_value == u)
         ss << u;
      else
         ss << m_value;
      std::string s = ss.str();
      std::cout << "Converting to string (" << s << ")" << std::endl;
      return s;
   }
   void negate()
   {
      std::cout << "Negating (" << m_value << ")" << std::endl;
      m_value = -m_value;
   }
   int compare(const number_backend_float_architype& o) const
   {
      std::cout << "Comparison" << std::endl;
      return m_value > o.m_value ? 1 : (m_value < o.m_value ? -1 : 0);
   }
   int compare(boost::long_long_type i) const
   {
      std::cout << "Comparison with int" << std::endl;
      return m_value > i ? 1 : (m_value < i ? -1 : 0);
   }
   int compare(boost::ulong_long_type i) const
   {
      std::cout << "Comparison with unsigned" << std::endl;
      return m_value > i ? 1 : (m_value < i ? -1 : 0);
   }
   int compare(long double d) const
   {
      std::cout << "Comparison with long double" << std::endl;
      return m_value > d ? 1 : (m_value < d ? -1 : 0);
   }
   long double m_value;
};

inline void eval_add(number_backend_float_architype& result, const number_backend_float_architype& o)
{
   std::cout << "Addition (" << result.m_value << " += " << o.m_value << ")" << std::endl;
   result.m_value += o.m_value;
}
inline void eval_subtract(number_backend_float_architype& result, const number_backend_float_architype& o)
{
   std::cout << "Subtraction (" << result.m_value << " -= " << o.m_value << ")" << std::endl;
   result.m_value -= o.m_value;
}
inline void eval_multiply(number_backend_float_architype& result, const number_backend_float_architype& o)
{
   std::cout << "Multiplication (" << result.m_value << " *= " << o.m_value << ")" << std::endl;
   result.m_value *= o.m_value;
}
inline void eval_divide(number_backend_float_architype& result, const number_backend_float_architype& o)
{
   std::cout << "Division (" << result.m_value << " /= " << o.m_value << ")" << std::endl;
   result.m_value /= o.m_value;
}

inline void eval_convert_to(boost::ulong_long_type* result, const number_backend_float_architype& val)
{
   *result = static_cast<boost::ulong_long_type>(val.m_value);
}
inline void eval_convert_to(boost::long_long_type* result, const number_backend_float_architype& val)
{
   *result = static_cast<boost::long_long_type>(val.m_value);
}
inline void eval_convert_to(long double* result, number_backend_float_architype& val)
{
   *result = val.m_value;
}

inline void eval_frexp(number_backend_float_architype& result, const number_backend_float_architype& arg, int* exp)
{
   result = std::frexp(arg.m_value, exp);
}

inline void eval_ldexp(number_backend_float_architype& result, const number_backend_float_architype& arg, int exp)
{
   result = std::ldexp(arg.m_value, exp);
}

inline void eval_floor(number_backend_float_architype& result, const number_backend_float_architype& arg)
{
   result = std::floor(arg.m_value);
}

inline void eval_ceil(number_backend_float_architype& result, const number_backend_float_architype& arg)
{
   result = std::ceil(arg.m_value);
}

inline void eval_sqrt(number_backend_float_architype& result, const number_backend_float_architype& arg)
{
   result = std::sqrt(arg.m_value);
}

inline int eval_fpclassify(const number_backend_float_architype& arg)
{
   return (boost::math::fpclassify)(arg.m_value);
}

inline std::size_t hash_value(const number_backend_float_architype& v)
{
   std::hash<long double> hasher;
   return hasher(v.m_value);
}

using mp_number_float_architype = boost::multiprecision::number<number_backend_float_architype>;

} // namespace concepts

template <>
struct number_category<concepts::number_backend_float_architype> : public std::integral_constant<int, number_kind_floating_point>
{};

}} // namespace boost::multiprecision

namespace std {

template <boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::concepts::number_backend_float_architype, ExpressionTemplates> > : public std::numeric_limits<long double>
{
   using base_type = std::numeric_limits<long double>                                                                                   ;
   using number_type = boost::multiprecision::number<boost::multiprecision::concepts::number_backend_float_architype, ExpressionTemplates>;

 public:
   static number_type(min)() noexcept { return (base_type::min)(); }
   static number_type(max)() noexcept { return (base_type::max)(); }
   static number_type lowest() noexcept { return -(max)(); }
   static number_type epsilon() noexcept { return base_type::epsilon(); }
   static number_type round_error() noexcept { return base_type::round_error(); }
   static number_type infinity() noexcept { return base_type::infinity(); }
   static number_type quiet_NaN() noexcept { return base_type::quiet_NaN(); }
   static number_type signaling_NaN() noexcept { return base_type::signaling_NaN(); }
   static number_type denorm_min() noexcept { return base_type::denorm_min(); }
};

} // namespace std

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MATH_DEBUG_ADAPTER_HPP
#define BOOST_MATH_DEBUG_ADAPTER_HPP

#include <boost/multiprecision/traits/extract_exponent_type.hpp>
#include <boost/multiprecision/detail/integer_ops.hpp>

namespace boost {
namespace multiprecision {
namespace backends {

template <class Backend>
struct debug_adaptor
{
   using signed_types = typename Backend::signed_types  ;
   using unsigned_types = typename Backend::unsigned_types;
   using float_types = typename Backend::float_types   ;
   using exponent_type = typename extract_exponent_type<Backend, number_category<Backend>::value>::type;

 private:
   std::string debug_value;
   Backend     m_value;

 public:
   void update_view()
   {
#ifndef BOOST_NO_EXCEPTIONS
      try
      {
#endif
         debug_value = m_value.str(0, static_cast<std::ios_base::fmtflags>(0));
#ifndef BOOST_NO_EXCEPTIONS
      }
      catch (const std::exception& e)
      {
         debug_value = "String conversion failed with message: \"";
         debug_value += e.what();
         debug_value += "\"";
      }
#endif
   }
   debug_adaptor()
   {
      update_view();
   }
   debug_adaptor(const debug_adaptor& o) : debug_value(o.debug_value), m_value(o.m_value)
   {
   }
   debug_adaptor& operator=(const debug_adaptor& o)
   {
      debug_value = o.debug_value;
      m_value     = o.m_value;
      return *this;
   }
   template <class T>
   debug_adaptor(const T& i, const typename std::enable_if<std::is_convertible<T, Backend>::value>::type* = 0)
       : m_value(i)
   {
      update_view();
   }
   template <class T>
   debug_adaptor(const debug_adaptor<T>& i, const typename std::enable_if<std::is_convertible<T, Backend>::value>::type* = 0)
       : m_value(i.value())
   {
      update_view();
   }
   template <class T>
   debug_adaptor(const T& i, const T& j)
       : m_value(i, j)
   {
      update_view();
   }
   debug_adaptor(const Backend& i, unsigned digits10)
       : m_value(i, digits10)
   {
      update_view();
   }
   template <class T>
   typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value || std::is_convertible<T, Backend>::value, debug_adaptor&>::type operator=(const T& i)
   {
      m_value = i;
      update_view();
      return *this;
   }
   debug_adaptor& operator=(const char* s)
   {
      m_value = s;
      update_view();
      return *this;
   }
   void swap(debug_adaptor& o)
   {
      std::swap(m_value, o.value());
      std::swap(debug_value, o.debug_value);
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      return m_value.str(digits, f);
   }
   void negate()
   {
      m_value.negate();
      update_view();
   }
   int compare(const debug_adaptor& o) const
   {
      return m_value.compare(o.value());
   }
   template <class T>
   int compare(const T& i) const
   {
      return m_value.compare(i);
   }
   Backend& value()
   {
      return m_value;
   }
   const Backend& value() const
   {
      return m_value;
   }
   template <class Archive>
   void serialize(Archive& ar, const unsigned int /*version*/)
   {
      ar & boost::make_nvp("value", m_value);
      using tag = typename Archive::is_loading;
      if (tag::value)
         update_view();
   }
   static unsigned default_precision() noexcept
   {
      return Backend::default_precision();
   }
   static void default_precision(unsigned v) noexcept
   {
      Backend::default_precision(v);
   }
   static unsigned thread_default_precision() noexcept
   {
      return Backend::thread_default_precision();
   }
   static void thread_default_precision(unsigned v) noexcept
   {
      Backend::thread_default_precision(v);
   }
   unsigned precision() const noexcept
   {
      return value().precision();
   }
   void precision(unsigned digits10) noexcept
   {
      value().precision(digits10);
   }
   //
   // Variable precision options:
   // 
   static constexpr variable_precision_options default_variable_precision_options()noexcept
   {
      return Backend::default_variable_precision_options();
   }
   static constexpr variable_precision_options thread_default_variable_precision_options()noexcept
   {
      return Backend::thread_default_variable_precision_options();
   }
   static BOOST_MP_CXX14_CONSTEXPR void default_variable_precision_options(variable_precision_options opts)
   {
      Backend::default_variable_precision_options(opts);
   }
   static BOOST_MP_CXX14_CONSTEXPR void thread_default_variable_precision_options(variable_precision_options opts)
   {
      Backend::thread_default_variable_precision_options(opts);
   }
};

template <class Backend>
inline Backend const& unwrap_debug_type(debug_adaptor<Backend> const& val)
{
   return val.value();
}
template <class T>
inline const T& unwrap_debug_type(const T& val)
{
   return val;
}

template <class Backend, class V, class U>
inline BOOST_MP_CXX14_CONSTEXPR void assign_components(debug_adaptor<Backend>& result, const V& v1, const U& v2)
{
   using default_ops::assign_components;
   assign_components(result.value(), unwrap_debug_type(v1), unwrap_debug_type(v2));
   result.update_view();
}

#define NON_MEMBER_OP1(name, str)                                       \
   template <class Backend>                                             \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result) \
   {                                                                    \
      using default_ops::BOOST_JOIN(eval_, name);                       \
      BOOST_JOIN(eval_, name)                                           \
      (result.value());                                                 \
      result.update_view();                                             \
   }

#define NON_MEMBER_OP2(name, str)                                                                        \
   template <class Backend, class T>                                                                     \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const T& a)                      \
   {                                                                                                     \
      using default_ops::BOOST_JOIN(eval_, name);                                                        \
      BOOST_JOIN(eval_, name)                                                                            \
      (result.value(), unwrap_debug_type(a));                                                            \
      result.update_view();                                                                              \
   }                                                                                                     \
   template <class Backend>                                                                              \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a) \
   {                                                                                                     \
      using default_ops::BOOST_JOIN(eval_, name);                                                        \
      BOOST_JOIN(eval_, name)                                                                            \
      (result.value(), unwrap_debug_type(a));                                                            \
      result.update_view();                                                                              \
   }

#define NON_MEMBER_OP3(name, str)                                                                                                         \
   template <class Backend, class T, class U>                                                                                             \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const T& a, const U& b)                                           \
   {                                                                                                                                      \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                         \
      BOOST_JOIN(eval_, name)                                                                                                             \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b));                                                                       \
      result.update_view();                                                                                                               \
   }                                                                                                                                      \
   template <class Backend, class T>                                                                                                      \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const T& b)                      \
   {                                                                                                                                      \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                         \
      BOOST_JOIN(eval_, name)                                                                                                             \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b));                                                                       \
      result.update_view();                                                                                                               \
   }                                                                                                                                      \
   template <class Backend, class T>                                                                                                      \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const T& a, const debug_adaptor<Backend>& b)                      \
   {                                                                                                                                      \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                         \
      BOOST_JOIN(eval_, name)                                                                                                             \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b));                                                                       \
      result.update_view();                                                                                                               \
   }                                                                                                                                      \
   template <class Backend>                                                                                                               \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const debug_adaptor<Backend>& b) \
   {                                                                                                                                      \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                         \
      BOOST_JOIN(eval_, name)                                                                                                             \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b));                                                                       \
      result.update_view();                                                                                                               \
   }

#define NON_MEMBER_OP4(name, str)                                                                                                                                          \
   template <class Backend, class T, class U, class V>                                                                                                                     \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const T& a, const U& b, const V& c)                                                                \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }                                                                                                                                                                       \
   template <class Backend, class T>                                                                                                                                       \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const debug_adaptor<Backend>& b, const T& c)                      \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }                                                                                                                                                                       \
   template <class Backend, class T>                                                                                                                                       \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const T& b, const debug_adaptor<Backend>& c)                      \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }                                                                                                                                                                       \
   template <class Backend, class T>                                                                                                                                       \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const T& a, const debug_adaptor<Backend>& b, const debug_adaptor<Backend>& c)                      \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }                                                                                                                                                                       \
   template <class Backend>                                                                                                                                                \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const debug_adaptor<Backend>& b, const debug_adaptor<Backend>& c) \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }                                                                                                                                                                       \
   template <class Backend, class T, class U>                                                                                                                              \
   inline void BOOST_JOIN(eval_, name)(debug_adaptor<Backend> & result, const debug_adaptor<Backend>& a, const T& b, const U& c)                                           \
   {                                                                                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                          \
      BOOST_JOIN(eval_, name)                                                                                                                                              \
      (result.value(), unwrap_debug_type(a), unwrap_debug_type(b), unwrap_debug_type(c));                                                                                  \
      result.update_view();                                                                                                                                                \
   }

NON_MEMBER_OP2(add, "+=")
NON_MEMBER_OP2(subtract, "-=")
NON_MEMBER_OP2(multiply, "*=")
NON_MEMBER_OP2(divide, "/=")

template <class Backend, class R>
inline void eval_convert_to(R* result, const debug_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.value());
}
template <class Backend, class R>
inline void eval_convert_to(debug_adaptor<R>* result, const debug_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(&result->value(), val.value());
}
template <class Backend, class R>
inline void eval_convert_to(debug_adaptor<R>* result, const Backend& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(&result->value(), val);
}

template <class Backend>
inline void eval_convert_to(std::complex<float>* result, const debug_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.value());
}
template <class Backend>
inline void eval_convert_to(std::complex<double>* result, const debug_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.value());
}
template <class Backend>
inline void eval_convert_to(std::complex<long double>* result, const debug_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.value());
}



template <class Backend, class Exp>
inline void eval_frexp(debug_adaptor<Backend>& result, const debug_adaptor<Backend>& arg, Exp* exp)
{
   eval_frexp(result.value(), arg.value(), exp);
   result.update_view();
}

template <class Backend, class Exp>
inline void eval_ldexp(debug_adaptor<Backend>& result, const debug_adaptor<Backend>& arg, Exp exp)
{
   eval_ldexp(result.value(), arg.value(), exp);
   result.update_view();
}

template <class Backend, class Exp>
inline void eval_scalbn(debug_adaptor<Backend>& result, const debug_adaptor<Backend>& arg, Exp exp)
{
   using default_ops::eval_scalbn;
   eval_scalbn(result.value(), arg.value(), exp);
   result.update_view();
}

template <class Backend>
inline typename Backend::exponent_type eval_ilogb(const debug_adaptor<Backend>& arg)
{
   using default_ops::eval_ilogb;
   return eval_ilogb(arg.value());
}

NON_MEMBER_OP2(floor, "floor")
NON_MEMBER_OP2(ceil, "ceil")
NON_MEMBER_OP2(sqrt, "sqrt")
NON_MEMBER_OP2(logb, "logb")

template <class Backend>
inline int eval_fpclassify(const debug_adaptor<Backend>& arg)
{
   using default_ops::eval_fpclassify;
   return eval_fpclassify(arg.value());
}

/*********************************************************************
*
* Optional arithmetic operations come next:
*
*********************************************************************/

NON_MEMBER_OP3(add, "+")
NON_MEMBER_OP3(subtract, "-")
NON_MEMBER_OP3(multiply, "*")
NON_MEMBER_OP3(divide, "/")
NON_MEMBER_OP3(multiply_add, "fused-multiply-add")
NON_MEMBER_OP3(multiply_subtract, "fused-multiply-subtract")
NON_MEMBER_OP4(multiply_add, "fused-multiply-add")
NON_MEMBER_OP4(multiply_subtract, "fused-multiply-subtract")

NON_MEMBER_OP1(increment, "increment")
NON_MEMBER_OP1(decrement, "decrement")

/*********************************************************************
*
* Optional integer operations come next:
*
*********************************************************************/

NON_MEMBER_OP2(modulus, "%=")
NON_MEMBER_OP3(modulus, "%")
NON_MEMBER_OP2(bitwise_or, "|=")
NON_MEMBER_OP3(bitwise_or, "|")
NON_MEMBER_OP2(bitwise_and, "&=")
NON_MEMBER_OP3(bitwise_and, "&")
NON_MEMBER_OP2(bitwise_xor, "^=")
NON_MEMBER_OP3(bitwise_xor, "^")
NON_MEMBER_OP4(qr, "quotient-and-remainder")
NON_MEMBER_OP2(complement, "~")

template <class Backend>
inline void eval_left_shift(debug_adaptor<Backend>& arg, std::size_t a)
{
   using default_ops::eval_left_shift;
   eval_left_shift(arg.value(), a);
   arg.update_view();
}
template <class Backend>
inline void eval_left_shift(debug_adaptor<Backend>& arg, const debug_adaptor<Backend>& a, std::size_t b)
{
   using default_ops::eval_left_shift;
   eval_left_shift(arg.value(), a.value(), b);
   arg.update_view();
}
template <class Backend>
inline void eval_right_shift(debug_adaptor<Backend>& arg, std::size_t a)
{
   using default_ops::eval_right_shift;
   eval_right_shift(arg.value(), a);
   arg.update_view();
}
template <class Backend>
inline void eval_right_shift(debug_adaptor<Backend>& arg, const debug_adaptor<Backend>& a, std::size_t b)
{
   using default_ops::eval_right_shift;
   eval_right_shift(arg.value(), a.value(), b);
   arg.update_view();
}

template <class Backend, class T>
inline unsigned eval_integer_modulus(const debug_adaptor<Backend>& arg, const T& a)
{
   using default_ops::eval_integer_modulus;
   return eval_integer_modulus(arg.value(), a);
}

template <class Backend>
inline unsigned eval_lsb(const debug_adaptor<Backend>& arg)
{
   using default_ops::eval_lsb;
   return eval_lsb(arg.value());
}

template <class Backend>
inline unsigned eval_msb(const debug_adaptor<Backend>& arg)
{
   using default_ops::eval_msb;
   return eval_msb(arg.value());
}

template <class Backend>
inline bool eval_bit_test(const debug_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_test;
   return eval_bit_test(arg.value(), a);
}

template <class Backend>
inline void eval_bit_set(const debug_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_set;
   eval_bit_set(arg.value(), a);
   arg.update_view();
}
template <class Backend>
inline void eval_bit_unset(const debug_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_unset;
   eval_bit_unset(arg.value(), a);
   arg.update_view();
}
template <class Backend>
inline void eval_bit_flip(const debug_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_flip;
   eval_bit_flip(arg.value(), a);
   arg.update_view();
}

NON_MEMBER_OP3(gcd, "gcd")
NON_MEMBER_OP3(lcm, "lcm")
NON_MEMBER_OP4(powm, "powm")

/*********************************************************************
*
* abs/fabs:
*
*********************************************************************/

NON_MEMBER_OP2(abs, "abs")
NON_MEMBER_OP2(fabs, "fabs")

/*********************************************************************
*
* Floating point functions:
*
*********************************************************************/

NON_MEMBER_OP2(trunc, "trunc")
NON_MEMBER_OP2(round, "round")
NON_MEMBER_OP2(exp, "exp")
NON_MEMBER_OP2(log, "log")
NON_MEMBER_OP2(log10, "log10")
NON_MEMBER_OP2(sin, "sin")
NON_MEMBER_OP2(cos, "cos")
NON_MEMBER_OP2(tan, "tan")
NON_MEMBER_OP2(asin, "asin")
NON_MEMBER_OP2(acos, "acos")
NON_MEMBER_OP2(atan, "atan")
NON_MEMBER_OP2(sinh, "sinh")
NON_MEMBER_OP2(cosh, "cosh")
NON_MEMBER_OP2(tanh, "tanh")
NON_MEMBER_OP2(asinh, "asinh")
NON_MEMBER_OP2(acosh, "acosh")
NON_MEMBER_OP2(atanh, "atanh")
NON_MEMBER_OP3(fmod, "fmod")
NON_MEMBER_OP3(pow, "pow")
NON_MEMBER_OP3(atan2, "atan2")
NON_MEMBER_OP2(conj, "conj")

template <class Backend>
int eval_signbit(const debug_adaptor<Backend>& val)
{
   using default_ops::eval_signbit;
   return eval_signbit(val.value());
}

template <class Backend>
std::size_t hash_value(const debug_adaptor<Backend>& val)
{
   return hash_value(val.value());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline typename std::enable_if<number_category<Backend>::value == number_kind_rational, typename number<debug_adaptor<Backend>, ExpressionTemplates>::value_type>::type
   numerator(const number<debug_adaptor<Backend>, ExpressionTemplates>& arg)
{
   number<Backend, ExpressionTemplates> t(arg.backend().value());
   return numerator(t).backend();
}
template <class Backend, expression_template_option ExpressionTemplates>
inline typename std::enable_if<number_category<Backend>::value == number_kind_rational, typename number<debug_adaptor<Backend>, ExpressionTemplates>::value_type>::type
   denominator(const number<debug_adaptor<Backend>, ExpressionTemplates>& arg)
{
   number<Backend, ExpressionTemplates> t(arg.backend().value());
   return denominator(t).backend();
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_real(To& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_real;
   eval_real(to, from.value());
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_real(debug_adaptor<To>& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_real;
   eval_real(to.value(), from.value());
   to.update_view();
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_real(debug_adaptor<To>& to, const From& from)
{
   using default_ops::eval_real;
   eval_real(to.value(), from);
   to.update_view();
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_imag(To& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_imag;
   eval_imag(to, from.value());
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_imag(debug_adaptor<To>& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_imag;
   eval_imag(to.value(), from.value());
   to.update_view();
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_imag(debug_adaptor<To>& to, const From& from)
{
   using default_ops::eval_imag;
   eval_imag(to.value(), from);
   to.update_view();
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(To& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_set_real;
   eval_set_real(to, from.value());
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(debug_adaptor<To>& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_set_real;
   eval_set_real(to.value(), from.value());
   to.update_view();
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(debug_adaptor<To>& to, const From& from)
{
   using default_ops::eval_set_real;
   eval_set_real(to.value(), from);
   to.update_view();
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(To& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_set_imag;
   eval_set_imag(to, from.value());
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(debug_adaptor<To>& to, const debug_adaptor<From>& from)
{
   using default_ops::eval_set_imag;
   eval_set_imag(to.value(), from.value());
   to.update_view();
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(debug_adaptor<To>& to, const From& from)
{
   using default_ops::eval_set_imag;
   eval_set_imag(to.value(), from);
   to.update_view();
}


} // namespace backends

using backends::debug_adaptor;

namespace detail {
   template <class Backend>
   struct is_variable_precision<debug_adaptor<Backend> > : public is_variable_precision<Backend>
   {};
} // namespace detail

template <class Backend>
struct number_category<backends::debug_adaptor<Backend> > : public number_category<Backend>
{};

template <class Number>
using debug_adaptor_t = number<debug_adaptor<typename Number::backend_type>, Number::et>;


template <class Backend, expression_template_option ExpressionTemplates>
struct component_type<number<debug_adaptor<Backend>, ExpressionTemplates>>
{
   //
   // We'll make the component_type also a debug_adaptor:
   //
   using base_component_type = typename component_type<number<Backend, ExpressionTemplates>>::type;
   using base_component_backend = typename base_component_type::backend_type;
   using type = number<debug_adaptor<base_component_backend>, ExpressionTemplates>;
};

template <class Backend>
struct is_interval_number<backends::debug_adaptor<Backend> > : public is_interval_number<Backend> {};

}} // namespace boost::multiprecision

namespace std {

template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::debug_adaptor<Backend>, ExpressionTemplates> >
    : public std::numeric_limits<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
   using base_type = std::numeric_limits<boost::multiprecision::number<Backend, ExpressionTemplates> >                          ;
   using number_type = boost::multiprecision::number<boost::multiprecision::backends::debug_adaptor<Backend>, ExpressionTemplates>;

 public:
   static number_type(min)() noexcept { return (base_type::min)(); }
   static number_type(max)() noexcept { return (base_type::max)(); }
   static number_type lowest() noexcept { return -(max)(); }
   static number_type epsilon() noexcept { return base_type::epsilon(); }
   static number_type round_error() noexcept { return epsilon() / 2; }
   static number_type infinity() noexcept { return base_type::infinity(); }
   static number_type quiet_NaN() noexcept { return base_type::quiet_NaN(); }
   static number_type signaling_NaN() noexcept { return base_type::signaling_NaN(); }
   static number_type denorm_min() noexcept { return base_type::denorm_min(); }
};

} // namespace std

namespace boost {
namespace math {

namespace policies {

template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
struct precision<boost::multiprecision::number<boost::multiprecision::debug_adaptor<Backend>, ExpressionTemplates>, Policy>
    : public precision<boost::multiprecision::number<Backend, ExpressionTemplates>, Policy>
{};

#undef NON_MEMBER_OP1
#undef NON_MEMBER_OP2
#undef NON_MEMBER_OP3
#undef NON_MEMBER_OP4

}

}} // namespace boost::math::policies

#endif

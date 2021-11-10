///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MATH_LOGGED_ADAPTER_HPP
#define BOOST_MATH_LOGGED_ADAPTER_HPP

#include <boost/multiprecision/traits/extract_exponent_type.hpp>
#include <boost/multiprecision/detail/integer_ops.hpp>

namespace boost {
namespace multiprecision {

template <class Backend>
inline void log_postfix_event(const Backend&, const char* /*event_description*/)
{
}
template <class Backend, class T>
inline void log_postfix_event(const Backend&, const T&, const char* /*event_description*/)
{
}
template <class Backend>
inline void log_prefix_event(const Backend&, const char* /*event_description*/)
{
}
template <class Backend, class T>
inline void log_prefix_event(const Backend&, const T&, const char* /*event_description*/)
{
}
template <class Backend, class T, class U>
inline void log_prefix_event(const Backend&, const T&, const U&, const char* /*event_description*/)
{
}
template <class Backend, class T, class U, class V>
inline void log_prefix_event(const Backend&, const T&, const U&, const V&, const char* /*event_description*/)
{
}

namespace backends {

template <class Backend>
struct logged_adaptor
{
   using signed_types = typename Backend::signed_types  ;
   using unsigned_types = typename Backend::unsigned_types;
   using float_types = typename Backend::float_types   ;
   using exponent_type = typename extract_exponent_type<Backend, number_category<Backend>::value>::type;

 private:
   Backend m_value;

 public:
   logged_adaptor()
   {
      log_postfix_event(m_value, "Default construct");
   }
   logged_adaptor(const logged_adaptor& o)
   {
      log_prefix_event(m_value, o.value(), "Copy construct");
      m_value = o.m_value;
      log_postfix_event(m_value, "Copy construct");
   }
   // rvalue copy
   logged_adaptor(logged_adaptor&& o)
   {
      log_prefix_event(m_value, o.value(), "Move construct");
      m_value = static_cast<Backend&&>(o.m_value);
      log_postfix_event(m_value, "Move construct");
   }
   logged_adaptor& operator=(logged_adaptor&& o)
   {
      log_prefix_event(m_value, o.value(), "Move Assignment");
      m_value = static_cast<Backend&&>(o.m_value);
      log_postfix_event(m_value, "Move construct");
      return *this;
   }
   logged_adaptor& operator=(const logged_adaptor& o)
   {
      log_prefix_event(m_value, o.value(), "Assignment");
      m_value = o.m_value;
      log_postfix_event(m_value, "Copy construct");
      return *this;
   }
   template <class T>
   logged_adaptor(const T& i, const typename std::enable_if<std::is_convertible<T, Backend>::value>::type* = 0)
       : m_value(i)
   {
      log_postfix_event(m_value, "construct from arithmetic type");
   }
   template <class T>
   logged_adaptor(const logged_adaptor<T>& i, const typename std::enable_if<std::is_convertible<T, Backend>::value>::type* = 0)
       : m_value(i.value())
   {
      log_postfix_event(m_value, "construct from arithmetic type");
   }
   template <class T>
   logged_adaptor(const T& i, const T& j)
      : m_value(i, j)
   {
      log_postfix_event(m_value, "construct from a pair of arithmetic types");
   }
   logged_adaptor(const Backend& i, unsigned digits10)
      : m_value(i, digits10)
   {
      log_postfix_event(m_value, "construct from arithmetic type and precision");
   }
   logged_adaptor(const logged_adaptor<Backend>& i, unsigned digits10)
      : m_value(i, digits10)
   {
      log_postfix_event(m_value, "construct from arithmetic type and precision");
   }
   template <class T>
   typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value || std::is_convertible<T, Backend>::value, logged_adaptor&>::type operator=(const T& i)
   {
      log_prefix_event(m_value, i, "Assignment from arithmetic type");
      m_value = i;
      log_postfix_event(m_value, "Assignment from arithmetic type");
      return *this;
   }
   logged_adaptor& operator=(const char* s)
   {
      log_prefix_event(m_value, s, "Assignment from string type");
      m_value = s;
      log_postfix_event(m_value, "Assignment from string type");
      return *this;
   }
   void swap(logged_adaptor& o)
   {
      log_prefix_event(m_value, o.value(), "swap");
      std::swap(m_value, o.value());
      log_postfix_event(m_value, "swap");
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      log_prefix_event(m_value, "Conversion to string");
      std::string s = m_value.str(digits, f);
      log_postfix_event(m_value, s, "Conversion to string");
      return s;
   }
   void negate()
   {
      log_prefix_event(m_value, "negate");
      m_value.negate();
      log_postfix_event(m_value, "negate");
   }
   int compare(const logged_adaptor& o) const
   {
      log_prefix_event(m_value, o.value(), "compare");
      int r = m_value.compare(o.value());
      log_postfix_event(m_value, r, "compare");
      return r;
   }
   template <class T>
   int compare(const T& i) const
   {
      log_prefix_event(m_value, i, "compare");
      int r = m_value.compare(i);
      log_postfix_event(m_value, r, "compare");
      return r;
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
      log_prefix_event(m_value, "serialize");
      ar& boost::make_nvp("value", m_value);
      log_postfix_event(m_value, "serialize");
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

template <class T>
inline const T& unwrap_logged_type(const T& a) { return a; }
template <class Backend>
inline const Backend& unwrap_logged_type(const logged_adaptor<Backend>& a) { return a.value(); }

#define NON_MEMBER_OP1(name, str)                                        \
   template <class Backend>                                              \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result) \
   {                                                                     \
      using default_ops::BOOST_JOIN(eval_, name);                        \
      log_prefix_event(result.value(), str);                             \
      BOOST_JOIN(eval_, name)                                            \
      (result.value());                                                  \
      log_postfix_event(result.value(), str);                            \
   }

#define NON_MEMBER_OP2(name, str)                                                                          \
   template <class Backend, class T>                                                                       \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const T& a)                       \
   {                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                          \
      log_prefix_event(result.value(), unwrap_logged_type(a), str);                                        \
      BOOST_JOIN(eval_, name)                                                                              \
      (result.value(), unwrap_logged_type(a));                                                             \
      log_postfix_event(result.value(), str);                                                              \
   }                                                                                                       \
   template <class Backend>                                                                                \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a) \
   {                                                                                                       \
      using default_ops::BOOST_JOIN(eval_, name);                                                          \
      log_prefix_event(result.value(), unwrap_logged_type(a), str);                                        \
      BOOST_JOIN(eval_, name)                                                                              \
      (result.value(), unwrap_logged_type(a));                                                             \
      log_postfix_event(result.value(), str);                                                              \
   }

#define NON_MEMBER_OP3(name, str)                                                                                                            \
   template <class Backend, class T, class U>                                                                                                \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const T& a, const U& b)                                             \
   {                                                                                                                                         \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                            \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), str);                                                   \
      BOOST_JOIN(eval_, name)                                                                                                                \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b));                                                                        \
      log_postfix_event(result.value(), str);                                                                                                \
   }                                                                                                                                         \
   template <class Backend, class T>                                                                                                         \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const T& b)                       \
   {                                                                                                                                         \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                            \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), str);                                                   \
      BOOST_JOIN(eval_, name)                                                                                                                \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b));                                                                        \
      log_postfix_event(result.value(), str);                                                                                                \
   }                                                                                                                                         \
   template <class Backend, class T>                                                                                                         \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const T& a, const logged_adaptor<Backend>& b)                       \
   {                                                                                                                                         \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                            \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), str);                                                   \
      BOOST_JOIN(eval_, name)                                                                                                                \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b));                                                                        \
      log_postfix_event(result.value(), str);                                                                                                \
   }                                                                                                                                         \
   template <class Backend>                                                                                                                  \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const logged_adaptor<Backend>& b) \
   {                                                                                                                                         \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                            \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), str);                                                   \
      BOOST_JOIN(eval_, name)                                                                                                                \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b));                                                                        \
      log_postfix_event(result.value(), str);                                                                                                \
   }

#define NON_MEMBER_OP4(name, str)                                                                                                                                              \
   template <class Backend, class T, class U, class V>                                                                                                                         \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const T& a, const U& b, const V& c)                                                                   \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }                                                                                                                                                                           \
   template <class Backend, class T>                                                                                                                                           \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const logged_adaptor<Backend>& b, const T& c)                       \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }                                                                                                                                                                           \
   template <class Backend, class T>                                                                                                                                           \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const T& b, const logged_adaptor<Backend>& c)                       \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }                                                                                                                                                                           \
   template <class Backend, class T>                                                                                                                                           \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const T& a, const logged_adaptor<Backend>& b, const logged_adaptor<Backend>& c)                       \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }                                                                                                                                                                           \
   template <class Backend>                                                                                                                                                    \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const logged_adaptor<Backend>& b, const logged_adaptor<Backend>& c) \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }                                                                                                                                                                           \
   template <class Backend, class T, class U>                                                                                                                                  \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<Backend> & result, const logged_adaptor<Backend>& a, const T& b, const U& c)                                             \
   {                                                                                                                                                                           \
      using default_ops::BOOST_JOIN(eval_, name);                                                                                                                              \
      log_prefix_event(result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c), str);                                                              \
      BOOST_JOIN(eval_, name)                                                                                                                                                  \
      (result.value(), unwrap_logged_type(a), unwrap_logged_type(b), unwrap_logged_type(c));                                                                                   \
      log_postfix_event(result.value(), str);                                                                                                                                  \
   }

NON_MEMBER_OP2(add, "+=")
NON_MEMBER_OP2(subtract, "-=")
NON_MEMBER_OP2(multiply, "*=")
NON_MEMBER_OP2(divide, "/=")

template <class Backend, class R>
inline void eval_convert_to(R* result, const logged_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val.value(), "convert_to");
   eval_convert_to(result, val.value());
   log_postfix_event(val.value(), *result, "convert_to");
}

template <class Backend, class R>
inline void eval_convert_to(logged_adaptor<R>* result, const logged_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val.value(), "convert_to");
   eval_convert_to(&result->value(), val.value());
   log_postfix_event(val.value(), &result->value(), "convert_to");
}
template <class Backend, class R>
inline void eval_convert_to(logged_adaptor<R>* result, const Backend& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val, "convert_to");
   eval_convert_to(&result->value(), val);
   log_postfix_event(val, &result->value(), "convert_to");
}

template <class Backend>
inline void eval_convert_to(std::complex<float>* result, const logged_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val.value(), "convert_to");
   eval_convert_to(result, val.value());
   log_postfix_event(val.value(), *result, "convert_to");
}
template <class Backend>
inline void eval_convert_to(std::complex<double>* result, const logged_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val.value(), "convert_to");
   eval_convert_to(result, val.value());
   log_postfix_event(val.value(), *result, "convert_to");
}
template <class Backend>
inline void eval_convert_to(std::complex<long double>* result, const logged_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   log_prefix_event(val.value(), "convert_to");
   eval_convert_to(result, val.value());
   log_postfix_event(val.value(), *result, "convert_to");
}


template <class Backend, class Exp>
inline void eval_frexp(logged_adaptor<Backend>& result, const logged_adaptor<Backend>& arg, Exp* exp)
{
   log_prefix_event(arg.value(), "frexp");
   eval_frexp(result.value(), arg.value(), exp);
   log_postfix_event(result.value(), *exp, "frexp");
}

template <class Backend, class Exp>
inline void eval_ldexp(logged_adaptor<Backend>& result, const logged_adaptor<Backend>& arg, Exp exp)
{
   log_prefix_event(arg.value(), "ldexp");
   eval_ldexp(result.value(), arg.value(), exp);
   log_postfix_event(result.value(), exp, "ldexp");
}

template <class Backend, class Exp>
inline void eval_scalbn(logged_adaptor<Backend>& result, const logged_adaptor<Backend>& arg, Exp exp)
{
   using default_ops::eval_scalbn;
   log_prefix_event(arg.value(), "scalbn");
   eval_scalbn(result.value(), arg.value(), exp);
   log_postfix_event(result.value(), exp, "scalbn");
}

template <class Backend>
inline typename Backend::exponent_type eval_ilogb(const logged_adaptor<Backend>& arg)
{
   using default_ops::eval_ilogb;
   log_prefix_event(arg.value(), "ilogb");
   typename Backend::exponent_type r = eval_ilogb(arg.value());
   log_postfix_event(arg.value(), "ilogb");
   return r;
}

NON_MEMBER_OP2(floor, "floor")
NON_MEMBER_OP2(ceil, "ceil")
NON_MEMBER_OP2(sqrt, "sqrt")

template <class Backend>
inline int eval_fpclassify(const logged_adaptor<Backend>& arg)
{
   using default_ops::eval_fpclassify;
   log_prefix_event(arg.value(), "fpclassify");
   int r = eval_fpclassify(arg.value());
   log_postfix_event(arg.value(), r, "fpclassify");
   return r;
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
inline void eval_left_shift(logged_adaptor<Backend>& arg, std::size_t a)
{
   using default_ops::eval_left_shift;
   log_prefix_event(arg.value(), a, "<<=");
   eval_left_shift(arg.value(), a);
   log_postfix_event(arg.value(), "<<=");
}
template <class Backend>
inline void eval_left_shift(logged_adaptor<Backend>& arg, const logged_adaptor<Backend>& a, std::size_t b)
{
   using default_ops::eval_left_shift;
   log_prefix_event(arg.value(), a, b, "<<");
   eval_left_shift(arg.value(), a.value(), b);
   log_postfix_event(arg.value(), "<<");
}
template <class Backend>
inline void eval_right_shift(logged_adaptor<Backend>& arg, std::size_t a)
{
   using default_ops::eval_right_shift;
   log_prefix_event(arg.value(), a, ">>=");
   eval_right_shift(arg.value(), a);
   log_postfix_event(arg.value(), ">>=");
}
template <class Backend>
inline void eval_right_shift(logged_adaptor<Backend>& arg, const logged_adaptor<Backend>& a, std::size_t b)
{
   using default_ops::eval_right_shift;
   log_prefix_event(arg.value(), a, b, ">>");
   eval_right_shift(arg.value(), a.value(), b);
   log_postfix_event(arg.value(), ">>");
}

template <class Backend, class T>
inline unsigned eval_integer_modulus(const logged_adaptor<Backend>& arg, const T& a)
{
   using default_ops::eval_integer_modulus;
   log_prefix_event(arg.value(), a, "integer-modulus");
   unsigned r = eval_integer_modulus(arg.value(), a);
   log_postfix_event(arg.value(), r, "integer-modulus");
   return r;
}

template <class Backend>
inline unsigned eval_lsb(const logged_adaptor<Backend>& arg)
{
   using default_ops::eval_lsb;
   log_prefix_event(arg.value(), "least-significant-bit");
   unsigned r = eval_lsb(arg.value());
   log_postfix_event(arg.value(), r, "least-significant-bit");
   return r;
}

template <class Backend>
inline unsigned eval_msb(const logged_adaptor<Backend>& arg)
{
   using default_ops::eval_msb;
   log_prefix_event(arg.value(), "most-significant-bit");
   unsigned r = eval_msb(arg.value());
   log_postfix_event(arg.value(), r, "most-significant-bit");
   return r;
}

template <class Backend>
inline bool eval_bit_test(const logged_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_test;
   log_prefix_event(arg.value(), a, "bit-test");
   bool r = eval_bit_test(arg.value(), a);
   log_postfix_event(arg.value(), r, "bit-test");
   return r;
}

template <class Backend>
inline void eval_bit_set(const logged_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_set;
   log_prefix_event(arg.value(), a, "bit-set");
   eval_bit_set(arg.value(), a);
   log_postfix_event(arg.value(), arg, "bit-set");
}
template <class Backend>
inline void eval_bit_unset(const logged_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_unset;
   log_prefix_event(arg.value(), a, "bit-unset");
   eval_bit_unset(arg.value(), a);
   log_postfix_event(arg.value(), arg, "bit-unset");
}
template <class Backend>
inline void eval_bit_flip(const logged_adaptor<Backend>& arg, unsigned a)
{
   using default_ops::eval_bit_flip;
   log_prefix_event(arg.value(), a, "bit-flip");
   eval_bit_flip(arg.value(), a);
   log_postfix_event(arg.value(), arg, "bit-flip");
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
NON_MEMBER_OP2(logb, "logb")
NON_MEMBER_OP3(fmod, "fmod")
NON_MEMBER_OP3(pow, "pow")
NON_MEMBER_OP3(atan2, "atan2")
NON_MEMBER_OP2(asinh, "asinh")
NON_MEMBER_OP2(acosh, "acosh")
NON_MEMBER_OP2(atanh, "atanh")
NON_MEMBER_OP2(conj, "conj")

template <class Backend>
int eval_signbit(const logged_adaptor<Backend>& val)
{
   using default_ops::eval_signbit;
   return eval_signbit(val.value());
}

template <class Backend>
std::size_t hash_value(const logged_adaptor<Backend>& val)
{
   return hash_value(val.value());
}

template <class Backend, expression_template_option ExpressionTemplates>
inline typename std::enable_if<number_category<Backend>::value == number_kind_rational, typename number<logged_adaptor<Backend>, ExpressionTemplates>::value_type>::type
numerator(const number<logged_adaptor<Backend>, ExpressionTemplates>& arg)
{
   number<Backend, ExpressionTemplates> t(arg.backend().value());
   return numerator(t).backend();
}
template <class Backend, expression_template_option ExpressionTemplates>
inline typename std::enable_if<number_category<Backend>::value == number_kind_rational, typename number<logged_adaptor<Backend>, ExpressionTemplates>::value_type>::type
denominator(const number<logged_adaptor<Backend>, ExpressionTemplates>& arg)
{
   number<Backend, ExpressionTemplates> t(arg.backend().value());
   return denominator(t).backend();
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(To& to, const logged_adaptor<From>& from)
{
   using default_ops::eval_set_real;
   log_prefix_event(to, from.value(), "Set real part");
   eval_set_real(to, from.value());
   log_postfix_event(to, from.value(), "Set real part");
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(logged_adaptor<To>& to, const logged_adaptor<From>& from)
{
   using default_ops::eval_set_real;
   log_prefix_event(to.value(), from.value(), "Set real part");
   eval_set_real(to.value(), from.value());
   log_postfix_event(to.value(), from.value(), "Set real part");
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_real(logged_adaptor<To>& to, const From& from)
{
   using default_ops::eval_set_real;
   log_prefix_event(to.value(), from, "Set real part");
   eval_set_real(to.value(), from);
   log_postfix_event(to.value(), from, "Set real part");
}

template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(To& to, const logged_adaptor<From>& from)
{
   using default_ops::eval_set_imag;
   log_prefix_event(to, from.value(), "Set imag part");
   eval_set_imag(to, from.value());
   log_postfix_event(to, from.value(), "Set imag part");
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(logged_adaptor<To>& to, const logged_adaptor<From>& from)
{
   using default_ops::eval_set_imag;
   log_prefix_event(to.value(), from.value(), "Set imag part");
   eval_set_imag(to.value(), from.value());
   log_postfix_event(to.value(), from.value(), "Set imag part");
}
template <class To, class From>
inline BOOST_MP_CXX14_CONSTEXPR void eval_set_imag(logged_adaptor<To>& to, const From& from)
{
   using default_ops::eval_set_imag;
   log_prefix_event(to.value(), from, "Set imag part");
   eval_set_imag(to.value(), from);
   log_postfix_event(to.value(), from, "Set imag part");
}


#define NON_MEMBER_COMPLEX_TO_REAL(name, str)                                                    \
   template <class B1, class B2>                                                                 \
   inline void BOOST_JOIN(eval_, name)(logged_adaptor<B1> & result, const logged_adaptor<B2>& a) \
   {                                                                                             \
      using default_ops::BOOST_JOIN(eval_, name);                                                \
      log_prefix_event(a.value(), a.value(), str);                                               \
      BOOST_JOIN(eval_, name)                                                                    \
      (result.value(), a.value());                                                               \
      log_postfix_event(result.value(), str);                                                    \
   }                                                                                             \
   template <class B1, class B2>                                                                 \
   inline void BOOST_JOIN(eval_, name)(B1 & result, const logged_adaptor<B2>& a)                 \
   {                                                                                             \
      using default_ops::BOOST_JOIN(eval_, name);                                                \
      log_prefix_event(a.value(), a.value(), str);                                               \
      BOOST_JOIN(eval_, name)                                                                    \
      (result, a.value());                                                                       \
      log_postfix_event(result, str);                                                            \
   }

NON_MEMBER_COMPLEX_TO_REAL(real, "real")
NON_MEMBER_COMPLEX_TO_REAL(imag, "imag")

template <class T, class V, class U>
inline void assign_components(logged_adaptor<T>& result, const V& v1, const U& v2)
{
   using default_ops::assign_components;
   assign_components(result.value(), unwrap_logged_type(v1), unwrap_logged_type(v2));
}

} // namespace backends

using backends::logged_adaptor;

namespace detail {
   template <class Backend>
   struct is_variable_precision<logged_adaptor<Backend> > : public is_variable_precision<Backend>
   {};
} // namespace detail

template <class Backend>
struct number_category<backends::logged_adaptor<Backend> > : public number_category<Backend>
{};

template <class Number>
using logged_adaptor_t = number<logged_adaptor<typename Number::backend_type>, Number::et>;

template <class Backend, expression_template_option ExpressionTemplates>
struct component_type<number<logged_adaptor<Backend>, ExpressionTemplates>>
{
   //
   // We'll make the component_type also a logged_adaptor:
   //
   using base_component_type = typename component_type<number<Backend, ExpressionTemplates>>::type;
   using base_component_backend = typename base_component_type::backend_type;
   using type = number<logged_adaptor<base_component_backend>, ExpressionTemplates>;
};

template <class Backend>
struct is_interval_number<backends::logged_adaptor<Backend> > : public is_interval_number<Backend> {};

}} // namespace boost::multiprecision

namespace std {

template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<Backend>, ExpressionTemplates> >
    : public std::numeric_limits<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
   using base_type = std::numeric_limits<boost::multiprecision::number<Backend, ExpressionTemplates> >                           ;
   using number_type = boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<Backend>, ExpressionTemplates>;

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
struct precision<boost::multiprecision::number<boost::multiprecision::logged_adaptor<Backend>, ExpressionTemplates>, Policy>
    : public precision<boost::multiprecision::number<Backend, ExpressionTemplates>, Policy>
{};

}

}} // namespace boost::math::policies

#undef NON_MEMBER_OP1
#undef NON_MEMBER_OP2
#undef NON_MEMBER_OP3
#undef NON_MEMBER_OP4

#endif

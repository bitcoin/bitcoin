///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_EXTENDED_REAL_HPP
#define BOOST_MATH_EXTENDED_REAL_HPP

#include <boost/config.hpp>
#include <cstdint>
#include <boost/multiprecision/detail/precision.hpp>
#include <boost/multiprecision/detail/generic_interconvert.hpp>
#include <boost/multiprecision/detail/number_compare.hpp>
#include <boost/multiprecision/traits/is_restricted_conversion.hpp>
#include <boost/multiprecision/traits/is_complex.hpp>
#include <boost/multiprecision/detail/hash.hpp>
#include <istream> // stream operators
#include <cstdio>  // EOF
#include <cctype>  // isspace
#include <functional>  // std::hash
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
#include <string_view>
#endif

namespace boost {
namespace multiprecision {

#ifdef BOOST_MSVC
// warning C4127: conditional expression is constant
// warning C4714: function marked as __forceinline not inlined
#pragma warning(push)
#pragma warning(disable : 4127 4714 6326)
#endif

template <class Backend, expression_template_option ExpressionTemplates>
class number
{
   using self_type = number<Backend, ExpressionTemplates>;

 public:
   using backend_type = Backend                                 ;
   using value_type = typename component_type<self_type>::type;

   static constexpr expression_template_option et = ExpressionTemplates;

   BOOST_MP_FORCEINLINE constexpr number() noexcept(noexcept(Backend())) {}
   BOOST_MP_FORCEINLINE constexpr number(const number& e) noexcept(noexcept(Backend(std::declval<Backend const&>()))) : m_backend(e.m_backend) {}
   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const V& v, typename std::enable_if<
                                                                        (boost::multiprecision::detail::is_arithmetic<V>::value || std::is_same<std::string, V>::value || std::is_convertible<V, const char*>::value) && !std::is_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && !detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value
#ifdef BOOST_HAS_FLOAT128
                                                                        && !std::is_same<V, __float128>::value
#endif
                                               >::type* = 0)
   {
      m_backend = canonical_value(v);
   }
   template <class V>
   BOOST_MP_FORCEINLINE constexpr number(const V& v, typename std::enable_if<
                                                         std::is_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && !detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value>::type* = 0)
#ifndef BOOST_INTEL
       noexcept(noexcept(Backend(std::declval<typename detail::canonical<V, Backend>::type const&>())))
#endif
       : m_backend(canonical_value(v))
   {}
   template <class V>
   BOOST_MP_FORCEINLINE constexpr number(const V& v, unsigned digits10, typename std::enable_if<(boost::multiprecision::detail::is_arithmetic<V>::value || std::is_same<std::string, V>::value || std::is_convertible<V, const char*>::value) && !detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value && (boost::multiprecision::number_category<Backend>::value != boost::multiprecision::number_kind_complex) && (boost::multiprecision::number_category<Backend>::value != boost::multiprecision::number_kind_rational && std::is_same<self_type, value_type>::value)
#ifdef BOOST_HAS_FLOAT128
                                                                                                && !std::is_same<V, __float128>::value
#endif
                                                                                                          >::type* = 0)
       : m_backend(canonical_value(v), digits10)
   {}
   //
   // Conversions from unscoped enum's are implicit:
   //
   template <class V>
   BOOST_MP_FORCEINLINE 
#if !(defined(BOOST_MSVC) && (BOOST_MSVC <= 1900))
      constexpr 
#endif
      number(const V& v, typename std::enable_if<
      std::is_enum<V>::value && std::is_convertible<V, int>::value && !std::is_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && !detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value>::type* = 0)
      : number(static_cast<typename std::underlying_type<V>::type>(v))
   {}
   //
   // Conversions from scoped enum's are explicit:
   //
   template <class V>
   BOOST_MP_FORCEINLINE explicit 
#if !(defined(BOOST_MSVC) && (BOOST_MSVC <= 1900))
       constexpr
#endif
       number(const V& v, typename std::enable_if<
      std::is_enum<V>::value && !std::is_convertible<V, int>::value && !std::is_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && !detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value>::type* = 0)
      : number(static_cast<typename std::underlying_type<V>::type>(v))
   {}

   BOOST_MP_FORCEINLINE constexpr number(const number& e, unsigned digits10)
       noexcept(noexcept(Backend(std::declval<Backend const&>(), std::declval<unsigned>())))
       : m_backend(e.m_backend, digits10) {}
   template <class V>
   explicit BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const V& v, typename std::enable_if<
                                                                                 (boost::multiprecision::detail::is_arithmetic<V>::value || std::is_same<std::string, V>::value || std::is_convertible<V, const char*>::value) && !detail::is_explicitly_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value>::type* = 0)
       noexcept(noexcept(std::declval<Backend&>() = std::declval<typename detail::canonical<V, Backend>::type const&>()))
   {
      m_backend = canonical_value(v);
   }
   template <class V>
   explicit BOOST_MP_FORCEINLINE constexpr number(const V& v, typename std::enable_if<
                                                                  detail::is_explicitly_convertible<typename detail::canonical<V, Backend>::type, Backend>::value && (detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value || !std::is_convertible<typename detail::canonical<V, Backend>::type, Backend>::value)>::type* = 0)
       noexcept(noexcept(Backend(std::declval<typename detail::canonical<V, Backend>::type const&>())))
       : m_backend(canonical_value(v)) {}
   template <class V>
   explicit BOOST_MP_FORCEINLINE constexpr number(const V& v, unsigned digits10, typename std::enable_if<(boost::multiprecision::detail::is_arithmetic<V>::value || std::is_same<std::string, V>::value || std::is_convertible<V, const char*>::value) && detail::is_restricted_conversion<typename detail::canonical<V, Backend>::type, Backend>::value && (boost::multiprecision::number_category<Backend>::value != boost::multiprecision::number_kind_complex) && (boost::multiprecision::number_category<Backend>::value != boost::multiprecision::number_kind_rational)>::type* = 0)
       : m_backend(canonical_value(v), digits10) {}

   template <expression_template_option ET>
   BOOST_MP_FORCEINLINE constexpr number(const number<Backend, ET>& val)
       noexcept(noexcept(Backend(std::declval<Backend const&>()))) : m_backend(val.backend()) {}

   template <class Other, expression_template_option ET>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const number<Other, ET>& val,
                                                        typename std::enable_if<(std::is_convertible<Other, Backend>::value && !detail::is_restricted_conversion<Other, Backend>::value)>::type* = 0)
       noexcept(noexcept(Backend(std::declval<Other const&>())))
       : m_backend(val.backend()) {}

   template <class Other, expression_template_option ET>
   explicit BOOST_MP_CXX14_CONSTEXPR number(const number<Other, ET>& val, typename std::enable_if<
                                                     (!detail::is_explicitly_convertible<Other, Backend>::value)>::type* = 0)
   {
      //
      // Attempt a generic interconvertion:
      //
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard_1(val);
      detail::scoped_default_precision<number<Other, ET> >                    precision_guard_2(val);
      using detail::generic_interconvert;
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      {
         if (precision_guard_1.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
         {
            self_type t;
            generic_interconvert(t.backend(), val.backend(), number_category<Backend>(), number_category<Other>());
            *this = std::move(t);
            return;
         }
      }
      generic_interconvert(backend(), val.backend(), number_category<Backend>(), number_category<Other>());
   }
   template <class Other, expression_template_option ET>
   explicit BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const number<Other, ET>& val, typename std::enable_if<
                                                                                                   (detail::is_explicitly_convertible<Other, Backend>::value && (detail::is_restricted_conversion<Other, Backend>::value || !std::is_convertible<Other, Backend>::value))>::type* = 0) noexcept(noexcept(Backend(std::declval<Other const&>())))
       : m_backend(val.backend()) {}

   template <class V, class U>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const V& v1, const U& v2,
                                                        typename std::enable_if<(std::is_convertible<V, value_type>::value && std::is_convertible<U, value_type>::value && !std::is_same<value_type, self_type>::value)>::type* = 0)
   {
      using default_ops::assign_components;
      // Copy precision options from this type to component_type:
      boost::multiprecision::detail::scoped_precision_options<value_type> scoped_opts(*this);
      // precision guards:
      detail::scoped_default_precision<self_type>  precision_guard(v1, v2, *this);
      detail::scoped_default_precision<value_type> component_precision_guard(v1, v2, *this);
      assign_components(m_backend, canonical_value(detail::evaluate_if_expression(v1)), canonical_value(detail::evaluate_if_expression(v2)));
   }
   template <class V, class U>
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR number(const V& v1, const U& v2,
                                        typename std::enable_if<
                                                                     (std::is_constructible<value_type, V>::value || std::is_convertible<V, std::string>::value) && (std::is_constructible<value_type, U>::value || std::is_convertible<U, std::string>::value) && !std::is_same<value_type, self_type>::value && !std::is_same<V, self_type>::value && !(std::is_convertible<V, value_type>::value && std::is_convertible<U, value_type>::value)>::type* = 0)
   {
      using default_ops::assign_components;
      // Copy precision options from this type to component_type:
      boost::multiprecision::detail::scoped_precision_options<value_type> scoped_opts(*this);
      // precision guards:
      detail::scoped_default_precision<self_type>  precision_guard(v1, v2, *this);
      detail::scoped_default_precision<value_type> component_precision_guard(v1, v2, *this);
      assign_components(m_backend, canonical_value(detail::evaluate_if_expression(v1)), canonical_value(detail::evaluate_if_expression(v2)));
   }
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
   //
   // Support for new types in C++17
   //
   template <class Traits>
   explicit inline BOOST_MP_CXX14_CONSTEXPR number(const std::basic_string_view<char, Traits>& view)
   {
      using default_ops::assign_from_string_view;
      assign_from_string_view(this->backend(), view);
   }
   template <class Traits>
   explicit inline BOOST_MP_CXX14_CONSTEXPR number(const std::basic_string_view<char, Traits>& view_x, const std::basic_string_view<char, Traits>& view_y)
   {
      using default_ops::assign_from_string_view;
      assign_from_string_view(this->backend(), view_x, view_y);
   }
   template <class Traits>
   explicit BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const std::basic_string_view<char, Traits>& v, unsigned digits10)
       : m_backend(canonical_value(v), digits10) {}
   template <class Traits>
   BOOST_MP_CXX14_CONSTEXPR number& assign(const std::basic_string_view<char, Traits>& view)
   {
      using default_ops::assign_from_string_view;
      assign_from_string_view(this->backend(), view);
      return *this;
   }
#endif

   template <class V, class U>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const V& v1, const U& v2, unsigned digits10,
                                                        typename std::enable_if<(std::is_convertible<V, value_type>::value && std::is_convertible<U, value_type>::value && !std::is_same<value_type, self_type>::value)>::type* = 0)
       : m_backend(canonical_value(detail::evaluate_if_expression(v1)), canonical_value(detail::evaluate_if_expression(v2)), digits10)
   {}
   template <class V, class U>
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR number(const V& v1, const U& v2, unsigned digits10,
                                                                 typename std::enable_if<((std::is_constructible<value_type, V>::value || std::is_convertible<V, std::string>::value) && (std::is_constructible<value_type, U>::value || std::is_convertible<U, std::string>::value) && !std::is_same<value_type, self_type>::value) && !(is_convertible<V, value_type>::value && is_convertible<U, value_type>::value)>::type* = 0)
       : m_backend(detail::evaluate_if_expression(v1), detail::evaluate_if_expression(v2), digits10) {}

   template <class Other, expression_template_option ET>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(const number<Other, ET>& v1, const number<Other, ET>& v2, typename std::enable_if<std::is_convertible<Other, Backend>::value>::type* = 0)
   {
      using default_ops::assign_components;
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(v1, v2);
      assign_components(m_backend, v1.backend(), v2.backend());
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      using tag_type = std::integral_constant<bool, is_equivalent_number_type<number, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value>;
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> >                                       precision_guard(e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_IF_CONSTEXPR (std::is_same<self_type, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value)
      {
         BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
            if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
            {
               number t(e);
               return *this = std::move(t);
            }
      }
      do_assign(e, tag_type());
      return *this;
   }
   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR number& assign(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      using tag_type = std::integral_constant<bool, is_equivalent_number_type<number, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value>;
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> >                                       precision_guard(e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_IF_CONSTEXPR(std::is_same<self_type, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value)
      {
         BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
         if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
            {
               number t;
               t.assign(e);
               return *this = std::move(t);
            }
      }
      do_assign(e, tag_type());
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR number& assign(const value_type& a, const value_type& b)
   {
      assign_components(backend(), a.backend(), b.backend());
      return *this;
   }
   template <class V, class U>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(std::is_convertible<V, value_type>::value&& std::is_convertible<U, value_type>::value && !std::is_same<value_type, self_type>::value), number&>::type 
      assign(const V& v1, const U& v2, unsigned Digits)
   {
      self_type r(v1, v2, Digits);
      boost::multiprecision::detail::scoped_source_precision<self_type> scope;
      return *this = r;
   }
   BOOST_MP_CXX14_CONSTEXPR number& assign(const value_type & a, const value_type & b, unsigned Digits)
   {
      this->precision(Digits);
      boost::multiprecision::detail::scoped_target_precision<self_type> scoped;
      assign_components(backend(), canonical_value(detail::evaluate_if_expression(a)), canonical_value(detail::evaluate_if_expression(b)));
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator=(const number& e)
       noexcept(noexcept(std::declval<Backend&>() = std::declval<Backend const&>()))
   {
      m_backend = e.m_backend;
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator=(const V& v)
       noexcept(noexcept(std::declval<Backend&>() = std::declval<const typename detail::canonical<V, Backend>::type&>()))
   {
      m_backend = canonical_value(v);
      return *this;
   }
   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number<Backend, ExpressionTemplates>& assign(const V& v)
       noexcept(noexcept(std::declval<Backend&>() = std::declval<const typename detail::canonical<V, Backend>::type&>()))
   {
      m_backend = canonical_value(v);
      return *this;
   }
   template <class V, class U>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number<Backend, ExpressionTemplates>& assign(const V& v, const U& digits10_or_component)
       noexcept(noexcept(std::declval<Backend&>() = std::declval<const typename detail::canonical<V, Backend>::type&>()))
   {
      number t(v, digits10_or_component);
      boost::multiprecision::detail::scoped_source_precision<self_type> scope;
      return *this = t;
   }
   template <class Other, expression_template_option ET>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!boost::multiprecision::detail::is_explicitly_convertible<Other, Backend>::value, number<Backend, ExpressionTemplates>&>::type
   assign(const number<Other, ET>& v)
   {
      //
      // Attempt a generic interconvertion:
      //
      using detail::generic_interconvert;
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, v);
      detail::scoped_default_precision<number<Other, ET> >                    precision_guard2(*this, v);
      //
      // If the current precision of *this differs from that of value v, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(v);
         return *this = std::move(t);
      }
      generic_interconvert(backend(), v.backend(), number_category<Backend>(), number_category<Other>());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR number(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e, typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value>::type* = 0)
   {
      //
      // No preicsion guard here, we already have one in operator=
      //
      *this = e;
   }
   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   explicit BOOST_MP_CXX14_CONSTEXPR number(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e,
                                            typename std::enable_if<!std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value && boost::multiprecision::detail::is_explicitly_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value>::type* = 0)
   {
      //
      // No precision guard as assign has one already:
      //
      assign(e);
   }

   // rvalues:
   BOOST_MP_FORCEINLINE constexpr number(number&& r)
       noexcept(noexcept(Backend(std::declval<Backend>())))
       : m_backend(static_cast<Backend&&>(r.m_backend))
   {}
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator=(number&& r) noexcept(noexcept(std::declval<Backend&>() = std::declval<Backend>()))
   {
      m_backend = static_cast<Backend&&>(r.m_backend);
      return *this;
   }
   template <class Other, expression_template_option ET>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number(number<Other, ET>&& val,
                                                        typename std::enable_if<(std::is_convertible<Other, Backend>::value && !detail::is_restricted_conversion<Other, Backend>::value)>::type* = 0)
       noexcept(noexcept(Backend(std::declval<Other const&>())))
       : m_backend(static_cast<number<Other, ET>&&>(val).backend()) {}
   template <class Other, expression_template_option ET>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<(std::is_convertible<Other, Backend>::value && !detail::is_restricted_conversion<Other, Backend>::value), number&>::type 
         operator=(number<Other, ET>&& val)
            noexcept(noexcept(Backend(std::declval<Other const&>())))
   {
      m_backend = std::move(val).backend();
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR number& operator+=(const self_type& val)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, val);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(*this + val);
         return *this = std::move(t);
      }
      do_add(detail::expression<detail::terminal, self_type>(val), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator+=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      // Create a copy if e contains this, but not if we're just doing a
      //    x += x
      if ((contains_self(e) && !is_self(e)))
      {
         self_type temp(e);
         do_add(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_add(e, tag());
      }
      return *this;
   }

   template <class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR number& operator+=(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_IF_CONSTEXPR(std::is_same<self_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value)
      {
         BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
         if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
            {
               number t(*this + e);
               return *this = std::move(t);
            }
      }
      //
      // Fused multiply-add:
      //
      using default_ops::eval_multiply_add;
      eval_multiply_add(m_backend, canonical_value(e.left_ref()), canonical_value(e.right_ref()));
      return *this;
   }

   template <class V>
   typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
      BOOST_MP_CXX14_CONSTEXPR operator+=(const V& v)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, v);
      //
      // If the current precision of *this differs from that of value v, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
         {
            number t(*this + v);
            return *this = std::move(t);
         }

      using default_ops::eval_add;
      eval_add(m_backend, canonical_value(v));
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR number& operator-=(const self_type& val)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, val);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(*this - val);
         return *this = std::move(t);
      }
      do_subtract(detail::expression<detail::terminal, self_type>(val), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator-=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      // Create a copy if e contains this:
      if (contains_self(e))
      {
         self_type temp(e);
         do_subtract(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_subtract(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator-=(const V& v)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, v);
      //
      // If the current precision of *this differs from that of value v, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
         {
            number t(*this - v);
            return *this = std::move(t);
         }

      using default_ops::eval_subtract;
      eval_subtract(m_backend, canonical_value(v));
      return *this;
   }

   template <class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR number& operator-=(const detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_IF_CONSTEXPR(std::is_same<self_type, typename detail::expression<detail::multiply_immediates, Arg1, Arg2, Arg3, Arg4>::result_type>::value)
      {
         BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
         if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
            {
               number t(*this - e);
               return *this = std::move(t);
            }
      }
      //
      // Fused multiply-subtract:
      //
      using default_ops::eval_multiply_subtract;
      eval_multiply_subtract(m_backend, canonical_value(e.left_ref()), canonical_value(e.right_ref()));
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR number& operator*=(const self_type& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(*this * e);
         return *this = std::move(t);
      }
      do_multiplies(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator*=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      // Create a temporary if the RHS references *this, but not
      // if we're just doing an   x *= x;
      if ((contains_self(e) && !is_self(e)))
      {
         self_type temp(e);
         do_multiplies(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_multiplies(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator*=(const V& v)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, v);
      //
      // If the current precision of *this differs from that of value v, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
         {
            number t(*this + v);
            return *this = std::move(t);
         }

      using default_ops::eval_multiply;
      eval_multiply(m_backend, canonical_value(v));
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR number& operator%=(const self_type& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(*this % e);
         return *this = std::move(t);
      }
      do_modulus(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }
   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator%=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      // Create a temporary if the RHS references *this:
      if (contains_self(e))
      {
         self_type temp(e);
         do_modulus(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_modulus(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }
   template <class V>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator%=(const V& v)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      using default_ops::eval_modulus;
      eval_modulus(m_backend, canonical_value(v));
      return *this;
   }

   //
   // These operators are *not* proto-ized.
   // The issue is that the increment/decrement must happen
   // even if the result of the operator *is never used*.
   // Possibly we could modify our expression wrapper to
   // execute the increment/decrement on destruction, but
   // correct implementation will be tricky, so defered for now...
   //
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator++()
   {
      using default_ops::eval_increment;
      eval_increment(m_backend);
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator--()
   {
      using default_ops::eval_decrement;
      eval_decrement(m_backend);
      return *this;
   }

   inline BOOST_MP_CXX14_CONSTEXPR number operator++(int)
   {
      using default_ops::eval_increment;
      self_type temp(*this);
      eval_increment(m_backend);
      return temp;
   }

   inline BOOST_MP_CXX14_CONSTEXPR number operator--(int)
   {
      using default_ops::eval_decrement;
      self_type temp(*this);
      eval_decrement(m_backend);
      return temp;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<V>::value, number&>::type operator<<=(V val)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The left-shift operation is only valid for integer types");
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(V) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<V>::value && boost::multiprecision::detail::is_integral<V>::value > ());
      eval_left_shift(m_backend, static_cast<std::size_t>(canonical_value(val)));
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_integral<V>::value, number&>::type operator>>=(V val)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The right-shift operation is only valid for integer types");
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(V) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<V>::value && boost::multiprecision::detail::is_integral<V>::value>());
      eval_right_shift(m_backend, static_cast<std::size_t>(canonical_value(val)));
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator/=(const self_type& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      //
      // If the current precision of *this differs from that of expression e, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
      {
         number t(*this / e);
         return *this = std::move(t);
      }
      do_divide(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator/=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, e);
      // Create a temporary if the RHS references *this:
      if (contains_self(e))
      {
         self_type temp(e);
         do_divide(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_divide(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator/=(const V& v)
   {
      detail::scoped_default_precision<number<Backend, ExpressionTemplates> > precision_guard(*this, v);
      //
      // If the current precision of *this differs from that of value v, then we
      // create a temporary (which will have the correct precision thanks to precision_guard)
      // and then move the result into *this.  In C++17 we add a leading "if constexpr"
      // which causes this code to be eliminated in the common case that this type is
      // not actually variable precision.  Pre C++17 this code should still be mostly
      // optimised away, but we can't prevent instantiation of the dead code leading
      // to longer build and possibly link times.
      //
      BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(number)
      if (precision_guard.precision() != boost::multiprecision::detail::current_precision_of<self_type>(*this))
         {
            number t(*this + v);
            return *this = std::move(t);
         }

      using default_ops::eval_divide;
      eval_divide(m_backend, canonical_value(v));
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator&=(const self_type& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      do_bitwise_and(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator&=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      // Create a temporary if the RHS references *this, but not
      // if we're just doing an   x &= x;
      if (contains_self(e) && !is_self(e))
      {
         self_type temp(e);
         do_bitwise_and(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_bitwise_and(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator&=(const V& v)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      using default_ops::eval_bitwise_and;
      eval_bitwise_and(m_backend, canonical_value(v));
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator|=(const self_type& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      do_bitwise_or(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator|=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      // Create a temporary if the RHS references *this, but not
      // if we're just doing an   x |= x;
      if (contains_self(e) && !is_self(e))
      {
         self_type temp(e);
         do_bitwise_or(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_bitwise_or(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator|=(const V& v)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      using default_ops::eval_bitwise_or;
      eval_bitwise_or(m_backend, canonical_value(v));
      return *this;
   }

   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR number& operator^=(const self_type& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      do_bitwise_xor(detail::expression<detail::terminal, self_type>(e), detail::terminal());
      return *this;
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type, self_type>::value, number&>::type operator^=(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      if (contains_self(e))
      {
         self_type temp(e);
         do_bitwise_xor(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
      }
      else
      {
         do_bitwise_xor(e, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::tag_type());
      }
      return *this;
   }

   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<V, self_type>::value, number<Backend, ExpressionTemplates>&>::type
   operator^=(const V& v)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      using default_ops::eval_bitwise_xor;
      eval_bitwise_xor(m_backend, canonical_value(v));
      return *this;
   }
   //
   // swap:
   //
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR void swap(self_type& other) noexcept(noexcept(std::declval<Backend>().swap(std::declval<Backend&>())))
   {
      m_backend.swap(other.backend());
   }
   //
   // Zero and sign:
   //
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool is_zero() const
   {
      using default_ops::eval_is_zero;
      return eval_is_zero(m_backend);
   }
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR int sign() const
   {
      using default_ops::eval_get_sign;
      return eval_get_sign(m_backend);
   }
   //
   // String conversion functions:
   //
   std::string str(std::streamsize digits = 0, std::ios_base::fmtflags f = std::ios_base::fmtflags(0)) const
   {
      return m_backend.str(digits, f);
   }
   template <class Archive>
   void serialize(Archive& ar, const unsigned int /*version*/)
   {
      ar& boost::make_nvp("backend", m_backend);
   }

 private:
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR void convert_to_imp(T* result) const
   {
      using default_ops::eval_convert_to;
      eval_convert_to(result, m_backend);
   }
   template <class B2, expression_template_option ET>
   BOOST_MP_CXX14_CONSTEXPR void convert_to_imp(number<B2, ET>* result) const
   {
      result->assign(*this);
   }
   BOOST_MP_CXX14_CONSTEXPR void convert_to_imp(std::string* result) const
   {
      *result = this->str();
   }

 public:
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to() const
   {
      T result = T();
      convert_to_imp(&result);
      return result;
   }
   //
   // Use in boolean context, and explicit conversion operators:
   //
#if BOOST_WORKAROUND(BOOST_MSVC, < 1900) || (defined(__apple_build_version__) && BOOST_WORKAROUND(__clang_major__, < 9))
   template <class T>
#else
   template <class T, class = typename std::enable_if<std::is_enum<T>::value || !(std::is_constructible<T, self_type const&>::value || !std::is_default_constructible<T>::value || (!boost::multiprecision::detail::is_arithmetic<T>::value && !boost::multiprecision::detail::is_complex<T>::value)), T>::type>
#endif
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return this->template convert_to<T>();
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      return !is_zero();
   }
   //
   // Default precision:
   //
   static BOOST_MP_CXX14_CONSTEXPR unsigned default_precision() noexcept
   {
      return Backend::default_precision();
   }
   static BOOST_MP_CXX14_CONSTEXPR void default_precision(unsigned digits10)
   {
      Backend::default_precision(digits10);
      Backend::thread_default_precision(digits10);
   }
   static BOOST_MP_CXX14_CONSTEXPR unsigned thread_default_precision() noexcept
   {
      return Backend::thread_default_precision();
   }
   static BOOST_MP_CXX14_CONSTEXPR void thread_default_precision(unsigned digits10)
   {
      Backend::thread_default_precision(digits10);
   }
   BOOST_MP_CXX14_CONSTEXPR unsigned precision() const noexcept
   {
      return m_backend.precision();
   }
   BOOST_MP_CXX14_CONSTEXPR void precision(unsigned digits10)
   {
      m_backend.precision(digits10);
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
   //
   // Comparison:
   //
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR int compare(const number<Backend, ExpressionTemplates>& o) const
       noexcept(noexcept(std::declval<Backend>().compare(std::declval<Backend>())))
   {
      return m_backend.compare(o.m_backend);
   }
   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<V>::value && (number_category<Backend>::value != number_kind_complex), int>::type compare(const V& o) const
   {
      using default_ops::eval_get_sign;
      if (o == 0)
         return eval_get_sign(m_backend);
      return m_backend.compare(canonical_value(o));
   }
   template <class V>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<boost::multiprecision::detail::is_arithmetic<V>::value && (number_category<Backend>::value == number_kind_complex), int>::type compare(const V& o) const
   {
      using default_ops::eval_get_sign;
      return m_backend.compare(canonical_value(o));
   }
   //
   // Direct access to the underlying backend:
   //
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR Backend& backend() & noexcept
   {
      return m_backend;
   }
   BOOST_MP_FORCEINLINE constexpr const Backend& backend() const& noexcept { return m_backend; }
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR Backend&& backend() && noexcept { return static_cast<Backend&&>(m_backend); }
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR Backend const&& backend() const&& noexcept { return static_cast<Backend const&&>(m_backend); }
   //
   // Complex number real and imag:
   //
   BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<number<Backend, ExpressionTemplates> >::type
   real() const
   {
      using default_ops::eval_real;
      detail::scoped_default_precision<typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type> precision_guard(*this);
      typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type                                   result;
      eval_real(result.backend(), backend());
      return result;
   }
   BOOST_MP_CXX14_CONSTEXPR typename scalar_result_from_possible_complex<number<Backend, ExpressionTemplates> >::type
   imag() const
   {
      using default_ops::eval_imag;
      detail::scoped_default_precision<typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type> precision_guard(*this);
      typename scalar_result_from_possible_complex<multiprecision::number<Backend, ExpressionTemplates> >::type                                   result;
      eval_imag(result.backend(), backend());
      return result;
   }
   template <class T>
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T, self_type>::value, self_type&>::type real(const T& val)
   {
      using default_ops::eval_set_real;
      eval_set_real(backend(), canonical_value(val));
      return *this;
   }
   template <class T>
   inline BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T, self_type>::value && number_category<self_type>::value == number_kind_complex, self_type&>::type imag(const T& val)
   {
      using default_ops::eval_set_imag;
      eval_set_imag(backend(), canonical_value(val));
      return *this;
   }

 private:
   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<std::is_assignable<number, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value>::type 
      do_assign(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e, const std::integral_constant<bool, false>&)
   {
      // The result of the expression isn't the same type as this -
      // create a temporary result and assign it to *this:
      using temp_type = typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type;
      temp_type                                                                     t(e);
      *this = std::move(t);
   }
   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!std::is_assignable<number, typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type>::value>::type 
      do_assign(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e, const std::integral_constant<bool, false>&)
   {
      // The result of the expression isn't the same type as this -
      // create a temporary result and assign it to *this:
      using temp_type = typename detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type;
      temp_type                                                                     t(e);
      this->assign(t);
   }

   template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const detail::expression<tag, Arg1, Arg2, Arg3, Arg4>& e, const std::integral_constant<bool, true>&)
   {
      do_assign(e, tag());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::add_immediates&)
   {
      using default_ops::eval_add;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_add(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::subtract_immediates&)
   {
      using default_ops::eval_subtract;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_subtract(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::multiply_immediates&)
   {
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::multiply_add&)
   {
      using default_ops::eval_multiply_add;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply_add(m_backend, canonical_value(e.left().value()), canonical_value(e.middle().value()), canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::multiply_subtract&)
   {
      using default_ops::eval_multiply_subtract;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply_subtract(m_backend, canonical_value(e.left().value()), canonical_value(e.middle().value()), canonical_value(e.right().value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::divide_immediates&)
   {
      using default_ops::eval_divide;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_divide(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::negate&)
   {
      using left_type = typename Exp::left_type;
      do_assign(e.left(), typename left_type::tag_type());
      m_backend.negate();
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::plus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && br)
      {
         self_type temp(e);
         temp.m_backend.swap(this->m_backend);
      }
      else if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_add(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         // Ignore the right node, it's *this, just add the left:
         do_add(e.left(), typename left_type::tag_type());
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      { // br is always false, but if bl is true we must take the this branch:
         do_assign(e.left(), typename left_type::tag_type());
         do_add(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_add(e.left(), typename left_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::minus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && br)
      {
         self_type temp(e);
         temp.m_backend.swap(this->m_backend);
      }
      else if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just subtract the right:
         do_subtract(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         // Ignore the right node, it's *this, just subtract the left and negate the result:
         do_subtract(e.left(), typename left_type::tag_type());
         m_backend.negate();
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      { // br is always false, but if bl is true we must take the this branch:
         do_assign(e.left(), typename left_type::tag_type());
         do_subtract(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_subtract(e.left(), typename left_type::tag_type());
         m_backend.negate();
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::multiplies&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && br)
      {
         self_type temp(e);
         temp.m_backend.swap(this->m_backend);
      }
      else if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_multiplies(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         // Ignore the right node, it's *this, just add the left:
         do_multiplies(e.left(), typename left_type::tag_type());
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      { // br is always false, but if bl is true we must take the this branch:
         do_assign(e.left(), typename left_type::tag_type());
         do_multiplies(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_multiplies(e.left(), typename left_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::divides&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_divide(e.right(), typename right_type::tag_type());
      }
      else if (br)
      {
         self_type temp(e);
         temp.m_backend.swap(this->m_backend);
      }
      else
      {
         do_assign(e.left(), typename left_type::tag_type());
         do_divide(e.right(), typename right_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::modulus&)
   {
      //
      // This operation is only valid for integer backends:
      //
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");

      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_modulus(e.right(), typename right_type::tag_type());
      }
      else if (br)
      {
         self_type temp(e);
         temp.m_backend.swap(this->m_backend);
      }
      else
      {
         do_assign(e.left(), typename left_type::tag_type());
         do_modulus(e.right(), typename right_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::modulus_immediates&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      using default_ops::eval_modulus;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_modulus(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_and&)
   {
      //
      // This operation is only valid for integer backends:
      //
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");

      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_bitwise_and(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         do_bitwise_and(e.left(), typename left_type::tag_type());
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      {
         do_assign(e.left(), typename left_type::tag_type());
         do_bitwise_and(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_bitwise_and(e.left(), typename left_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_and_immediates&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");
      using default_ops::eval_bitwise_and;
      eval_bitwise_and(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_or&)
   {
      //
      // This operation is only valid for integer backends:
      //
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");

      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_bitwise_or(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         do_bitwise_or(e.left(), typename left_type::tag_type());
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      {
         do_assign(e.left(), typename left_type::tag_type());
         do_bitwise_or(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_bitwise_or(e.left(), typename left_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_or_immediates&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");
      using default_ops::eval_bitwise_or;
      eval_bitwise_or(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_xor&)
   {
      //
      // This operation is only valid for integer backends:
      //
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");

      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;

      constexpr int const left_depth  = left_type::depth;
      constexpr int const right_depth = right_type::depth;

      bool bl = contains_self(e.left());
      bool br = contains_self(e.right());

      if (bl && is_self(e.left()))
      {
         // Ignore the left node, it's *this, just add the right:
         do_bitwise_xor(e.right(), typename right_type::tag_type());
      }
      else if (br && is_self(e.right()))
      {
         do_bitwise_xor(e.left(), typename left_type::tag_type());
      }
      else if (!br && (bl || (left_depth >= right_depth)))
      {
         do_assign(e.left(), typename left_type::tag_type());
         do_bitwise_xor(e.right(), typename right_type::tag_type());
      }
      else
      {
         do_assign(e.right(), typename right_type::tag_type());
         do_bitwise_xor(e.left(), typename left_type::tag_type());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_xor_immediates&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "Bitwise operations are only valid for integer types");
      using default_ops::eval_bitwise_xor;
      eval_bitwise_xor(m_backend, canonical_value(e.left().value()), canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::terminal&)
   {
      if (!is_self(e))
      {
         m_backend = canonical_value(e.value());
      }
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::function&)
   {
      using tag_type = typename Exp::arity;
      boost::multiprecision::detail::maybe_promote_precision(this);
      do_assign_function(e, tag_type());
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::shift_left&)
   {
      // We can only shift by an integer value, not an arbitrary expression:
      using left_type = typename Exp::left_type   ;
      using right_type = typename Exp::right_type  ;
      using right_arity = typename right_type::arity;
      static_assert(right_arity::value == 0, "The left shift operator requires an integer value for the shift operand.");
      using right_value_type = typename right_type::result_type;
      static_assert(boost::multiprecision::detail::is_integral<right_value_type>::value, "The left shift operator requires an integer value for the shift operand.");
      using tag_type = typename left_type::tag_type;
      do_assign_left_shift(e.left(), canonical_value(e.right().value()), tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::shift_right&)
   {
      // We can only shift by an integer value, not an arbitrary expression:
      using left_type = typename Exp::left_type   ;
      using right_type = typename Exp::right_type  ;
      using right_arity = typename right_type::arity;
      static_assert(right_arity::value == 0, "The left shift operator requires an integer value for the shift operand.");
      using right_value_type = typename right_type::result_type;
      static_assert(boost::multiprecision::detail::is_integral<right_value_type>::value, "The left shift operator requires an integer value for the shift operand.");
      using tag_type = typename left_type::tag_type;
      do_assign_right_shift(e.left(), canonical_value(e.right().value()), tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::bitwise_complement&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ~ operation is only valid for integer types");
      using default_ops::eval_complement;
      self_type temp(e.left());
      eval_complement(m_backend, temp.backend());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign(const Exp& e, const detail::complement_immediates&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ~ operation is only valid for integer types");
      using default_ops::eval_complement;
      eval_complement(m_backend, canonical_value(e.left().value()));
   }

   template <class Exp, class Val>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_right_shift(const Exp& e, const Val& val, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The right shift operation is only valid for integer types");
      using default_ops::eval_right_shift;
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(Val) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<Val>::value&& boost::multiprecision::detail::is_integral<Val>::value>());
      eval_right_shift(m_backend, canonical_value(e.value()), static_cast<std::size_t>(val));
   }

   template <class Exp, class Val>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_left_shift(const Exp& e, const Val& val, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The left shift operation is only valid for integer types");
      using default_ops::eval_left_shift;
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(Val) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<Val>::value&& boost::multiprecision::detail::is_integral<Val>::value>());
      eval_left_shift(m_backend, canonical_value(e.value()), static_cast<std::size_t>(val));
   }

   template <class Exp, class Val, class Tag>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_right_shift(const Exp& e, const Val& val, const Tag&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The right shift operation is only valid for integer types");
      using default_ops::eval_right_shift;
      self_type temp(e);
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(Val) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<Val>::value&& boost::multiprecision::detail::is_integral<Val>::value>());
      eval_right_shift(m_backend, temp.backend(), static_cast<std::size_t>(val));
   }

   template <class Exp, class Val, class Tag>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_left_shift(const Exp& e, const Val& val, const Tag&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The left shift operation is only valid for integer types");
      using default_ops::eval_left_shift;
      self_type temp(e);
      detail::check_shift_range(val, std::integral_constant<bool, (sizeof(Val) > sizeof(std::size_t))>(), std::integral_constant<bool, boost::multiprecision::detail::is_signed<Val>::value&& boost::multiprecision::detail::is_integral<Val>::value>());
      eval_left_shift(m_backend, temp.backend(), static_cast<std::size_t>(val));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function(const Exp& e, const std::integral_constant<int, 1>&)
   {
      e.left().value()(&m_backend);
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function(const Exp& e, const std::integral_constant<int, 2>&)
   {
      using right_type = typename Exp::right_type     ;
      using tag_type = typename right_type::tag_type;
      do_assign_function_1(e.left().value(), e.right_ref(), tag_type());
   }
   template <class F, class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_1(const F& f, const Exp& val, const detail::terminal&)
   {
      f(m_backend, function_arg_value(val));
   }
   template <class F, class Exp, class Tag>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_1(const F& f, const Exp& val, const Tag&)
   {
      typename Exp::result_type t(val);
      f(m_backend, t.backend());
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function(const Exp& e, const std::integral_constant<int, 3>&)
   {
      using middle_type = typename Exp::middle_type     ;
      using tag_type = typename middle_type::tag_type;
      using end_type = typename Exp::right_type      ;
      using end_tag = typename end_type::tag_type   ;
      do_assign_function_2(e.left().value(), e.middle_ref(), e.right_ref(), tag_type(), end_tag());
   }
   template <class F, class Exp1, class Exp2>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_2(const F& f, const Exp1& val1, const Exp2& val2, const detail::terminal&, const detail::terminal&)
   {
      f(m_backend, function_arg_value(val1), function_arg_value(val2));
   }
   template <class F, class Exp1, class Exp2, class Tag1>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_2(const F& f, const Exp1& val1, const Exp2& val2, const Tag1&, const detail::terminal&)
   {
      typename Exp1::result_type temp1(val1);
      f(m_backend, std::move(temp1.backend()), function_arg_value(val2));
   }
   template <class F, class Exp1, class Exp2, class Tag2>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_2(const F& f, const Exp1& val1, const Exp2& val2, const detail::terminal&, const Tag2&)
   {
      typename Exp2::result_type temp2(val2);
      f(m_backend, function_arg_value(val1), std::move(temp2.backend()));
   }
   template <class F, class Exp1, class Exp2, class Tag1, class Tag2>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_2(const F& f, const Exp1& val1, const Exp2& val2, const Tag1&, const Tag2&)
   {
      typename Exp1::result_type temp1(val1);
      typename Exp2::result_type temp2(val2);
      f(m_backend, std::move(temp1.backend()), std::move(temp2.backend()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function(const Exp& e, const std::integral_constant<int, 4>&)
   {
      using left_type = typename Exp::left_middle_type ;
      using left_tag_type = typename left_type::tag_type   ;
      using middle_type = typename Exp::right_middle_type;
      using middle_tag_type = typename middle_type::tag_type ;
      using right_type = typename Exp::right_type       ;
      using right_tag_type = typename right_type::tag_type  ;
      do_assign_function_3a(e.left().value(), e.left_middle_ref(), e.right_middle_ref(), e.right_ref(), left_tag_type(), middle_tag_type(), right_tag_type());
   }

   template <class F, class Exp1, class Exp2, class Exp3, class Tag2, class Tag3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3a(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const detail::terminal&, const Tag2& t2, const Tag3& t3)
   {
      do_assign_function_3b(f, val1, val2, val3, t2, t3);
   }
   template <class F, class Exp1, class Exp2, class Exp3, class Tag1, class Tag2, class Tag3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3a(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const Tag1&, const Tag2& t2, const Tag3& t3)
   {
      typename Exp1::result_type t(val1);
      do_assign_function_3b(f, std::move(t), val2, val3, t2, t3);
   }
   template <class F, class Exp1, class Exp2, class Exp3, class Tag3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3b(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const detail::terminal&, const Tag3& t3)
   {
      do_assign_function_3c(f, val1, val2, val3, t3);
   }
   template <class F, class Exp1, class Exp2, class Exp3, class Tag2, class Tag3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3b(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const Tag2& /*t2*/, const Tag3& t3)
   {
      typename Exp2::result_type t(val2);
      do_assign_function_3c(f, val1, std::move(t), val3, t3);
   }
   template <class F, class Exp1, class Exp2, class Exp3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3c(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const detail::terminal&)
   {
      f(m_backend, function_arg_value(val1), function_arg_value(val2), function_arg_value(val3));
   }
   template <class F, class Exp1, class Exp2, class Exp3, class Tag3>
   BOOST_MP_CXX14_CONSTEXPR void do_assign_function_3c(const F& f, const Exp1& val1, const Exp2& val2, const Exp3& val3, const Tag3& /*t3*/)
   {
      typename Exp3::result_type t(val3);
      do_assign_function_3c(f, val1, val2, std::move(t), detail::terminal());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::terminal&)
   {
      using default_ops::eval_add;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_add(m_backend, canonical_value(e.value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::negate&)
   {
      using left_type = typename Exp::left_type;
      boost::multiprecision::detail::maybe_promote_precision(this);
      do_subtract(e.left(), typename left_type::tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::plus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_add(e.left(), typename left_type::tag_type());
      do_add(e.right(), typename right_type::tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::minus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_add(e.left(), typename left_type::tag_type());
      do_subtract(e.right(), typename right_type::tag_type());
   }

   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const unknown&)
   {
      self_type temp(e);
      do_add(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::add_immediates&)
   {
      using default_ops::eval_add;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_add(m_backend, canonical_value(e.left().value()));
      eval_add(m_backend, canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_add(const Exp& e, const detail::subtract_immediates&)
   {
      using default_ops::eval_add;
      using default_ops::eval_subtract;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_add(m_backend, canonical_value(e.left().value()));
      eval_subtract(m_backend, canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::terminal&)
   {
      using default_ops::eval_subtract;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_subtract(m_backend, canonical_value(e.value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::negate&)
   {
      using left_type = typename Exp::left_type;
      do_add(e.left(), typename left_type::tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::plus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_subtract(e.left(), typename left_type::tag_type());
      do_subtract(e.right(), typename right_type::tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::minus&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_subtract(e.left(), typename left_type::tag_type());
      do_add(e.right(), typename right_type::tag_type());
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::add_immediates&)
   {
      using default_ops::eval_subtract;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_subtract(m_backend, canonical_value(e.left().value()));
      eval_subtract(m_backend, canonical_value(e.right().value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const detail::subtract_immediates&)
   {
      using default_ops::eval_add;
      using default_ops::eval_subtract;
      eval_subtract(m_backend, canonical_value(e.left().value()));
      eval_add(m_backend, canonical_value(e.right().value()));
   }
   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_subtract(const Exp& e, const unknown&)
   {
      self_type temp(e);
      do_subtract(detail::expression<detail::terminal, self_type>(temp), detail::terminal());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_multiplies(const Exp& e, const detail::terminal&)
   {
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply(m_backend, canonical_value(e.value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_multiplies(const Exp& e, const detail::negate&)
   {
      using left_type = typename Exp::left_type;
      do_multiplies(e.left(), typename left_type::tag_type());
      m_backend.negate();
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_multiplies(const Exp& e, const detail::multiplies&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_multiplies(e.left(), typename left_type::tag_type());
      do_multiplies(e.right(), typename right_type::tag_type());
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_multiplies(const Exp& e, const detail::divides&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_multiplies(e.left(), typename left_type::tag_type());
      do_divide(e.right(), typename right_type::tag_type());
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_multiplies(const Exp& e, const detail::multiply_immediates&)
   {
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply(m_backend, canonical_value(e.left().value()));
      eval_multiply(m_backend, canonical_value(e.right().value()));
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_multiplies(const Exp& e, const detail::divide_immediates&)
   {
      using default_ops::eval_divide;
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_multiply(m_backend, canonical_value(e.left().value()));
      eval_divide(m_backend, canonical_value(e.right().value()));
   }
   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_multiplies(const Exp& e, const unknown&)
   {
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      self_type temp(e);
      eval_multiply(m_backend, temp.m_backend);
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_divide(const Exp& e, const detail::terminal&)
   {
      using default_ops::eval_divide;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_divide(m_backend, canonical_value(e.value()));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_divide(const Exp& e, const detail::negate&)
   {
      using left_type = typename Exp::left_type;
      do_divide(e.left(), typename left_type::tag_type());
      m_backend.negate();
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_divide(const Exp& e, const detail::multiplies&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_divide(e.left(), typename left_type::tag_type());
      do_divide(e.right(), typename right_type::tag_type());
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_divide(const Exp& e, const detail::divides&)
   {
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_divide(e.left(), typename left_type::tag_type());
      do_multiplies(e.right(), typename right_type::tag_type());
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_divides(const Exp& e, const detail::multiply_immediates&)
   {
      using default_ops::eval_divide;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_divide(m_backend, canonical_value(e.left().value()));
      eval_divide(m_backend, canonical_value(e.right().value()));
   }
   //
   // This rearrangement is disabled for integer types, the test on sizeof(Exp) is simply to make
   // the disable_if dependent on the template argument (the size of 1 can never occur in practice).
   //
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR typename std::enable_if<!(boost::multiprecision::number_category<self_type>::value == boost::multiprecision::number_kind_integer || sizeof(Exp) == 1)>::type
   do_divides(const Exp& e, const detail::divide_immediates&)
   {
      using default_ops::eval_divide;
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_divide(m_backend, canonical_value(e.left().value()));
      mutiply(m_backend, canonical_value(e.right().value()));
   }

   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_divide(const Exp& e, const unknown&)
   {
      using default_ops::eval_multiply;
      boost::multiprecision::detail::maybe_promote_precision(this);
      self_type temp(e);
      eval_divide(m_backend, temp.m_backend);
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_modulus(const Exp& e, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      using default_ops::eval_modulus;
      boost::multiprecision::detail::maybe_promote_precision(this);
      eval_modulus(m_backend, canonical_value(e.value()));
   }

   template <class Exp, class Unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_modulus(const Exp& e, const Unknown&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The modulus operation is only valid for integer types");
      using default_ops::eval_modulus;
      boost::multiprecision::detail::maybe_promote_precision(this);
      self_type temp(e);
      eval_modulus(m_backend, canonical_value(temp));
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_and(const Exp& e, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      using default_ops::eval_bitwise_and;
      eval_bitwise_and(m_backend, canonical_value(e.value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_and(const Exp& e, const detail::bitwise_and&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_bitwise_and(e.left(), typename left_type::tag_type());
      do_bitwise_and(e.right(), typename right_type::tag_type());
   }
   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_and(const Exp& e, const unknown&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise & operation is only valid for integer types");
      using default_ops::eval_bitwise_and;
      self_type temp(e);
      eval_bitwise_and(m_backend, temp.m_backend);
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_or(const Exp& e, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      using default_ops::eval_bitwise_or;
      eval_bitwise_or(m_backend, canonical_value(e.value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_or(const Exp& e, const detail::bitwise_or&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_bitwise_or(e.left(), typename left_type::tag_type());
      do_bitwise_or(e.right(), typename right_type::tag_type());
   }
   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_or(const Exp& e, const unknown&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise | operation is only valid for integer types");
      using default_ops::eval_bitwise_or;
      self_type temp(e);
      eval_bitwise_or(m_backend, temp.m_backend);
   }

   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_xor(const Exp& e, const detail::terminal&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      using default_ops::eval_bitwise_xor;
      eval_bitwise_xor(m_backend, canonical_value(e.value()));
   }
   template <class Exp>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_xor(const Exp& e, const detail::bitwise_xor&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      using left_type = typename Exp::left_type ;
      using right_type = typename Exp::right_type;
      do_bitwise_xor(e.left(), typename left_type::tag_type());
      do_bitwise_xor(e.right(), typename right_type::tag_type());
   }
   template <class Exp, class unknown>
   BOOST_MP_CXX14_CONSTEXPR void do_bitwise_xor(const Exp& e, const unknown&)
   {
      static_assert(number_category<Backend>::value == number_kind_integer, "The bitwise ^ operation is only valid for integer types");
      using default_ops::eval_bitwise_xor;
      self_type temp(e);
      eval_bitwise_xor(m_backend, temp.m_backend);
   }

   // Tests if the expression contains a reference to *this:
   template <class Exp>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool contains_self(const Exp& e) const noexcept
   {
      return contains_self(e, typename Exp::arity());
   }
   template <class Exp>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool contains_self(const Exp& e, std::integral_constant<int, 0> const&) const noexcept
   {
      return is_realy_self(e.value());
   }
   template <class Exp>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool contains_self(const Exp& e, std::integral_constant<int, 1> const&) const noexcept
   {
      using child_type = typename Exp::left_type;
      return contains_self(e.left(), typename child_type::arity());
   }
   template <class Exp>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool contains_self(const Exp& e, std::integral_constant<int, 2> const&) const noexcept
   {
      using child0_type = typename Exp::left_type ;
      using child1_type = typename Exp::right_type;
      return contains_self(e.left(), typename child0_type::arity()) || contains_self(e.right(), typename child1_type::arity());
   }
   template <class Exp>
   BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR bool contains_self(const Exp& e, std::integral_constant<int, 3> const&) const noexcept
   {
      using child0_type = typename Exp::left_type  ;
      using child1_type = typename Exp::middle_type;
      using child2_type = typename Exp::right_type ;
      return contains_self(e.left(), typename child0_type::arity()) || contains_self(e.middle(), typename child1_type::arity()) || contains_self(e.right(), typename child2_type::arity());
   }

   // Test if the expression is a reference to *this:
   template <class Exp>
   BOOST_MP_FORCEINLINE constexpr bool is_self(const Exp& e) const noexcept
   {
      return is_self(e, typename Exp::arity());
   }
   template <class Exp>
   BOOST_MP_FORCEINLINE constexpr bool is_self(const Exp& e, std::integral_constant<int, 0> const&) const noexcept
   {
      return is_realy_self(e.value());
   }
   template <class Exp, int v>
   BOOST_MP_FORCEINLINE constexpr bool is_self(const Exp&, std::integral_constant<int, v> const&) const noexcept
   {
      return false;
   }

   template <class Val>
   BOOST_MP_FORCEINLINE constexpr bool is_realy_self(const Val&) const noexcept { return false; }
   BOOST_MP_FORCEINLINE constexpr bool is_realy_self(const self_type& v) const noexcept { return &v == this; }

   static BOOST_MP_FORCEINLINE constexpr const Backend& function_arg_value(const self_type& v) noexcept { return v.backend(); }
   template <class Other, expression_template_option ET2>
   static BOOST_MP_FORCEINLINE constexpr const Other& function_arg_value(const number<Other, ET2>& v) noexcept { return v.backend(); }
   template <class V>
   static BOOST_MP_FORCEINLINE constexpr const V& function_arg_value(const V& v) noexcept { return v; }
   template <class A1, class A2, class A3, class A4>
   static BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR const A1& function_arg_value(const detail::expression<detail::terminal, A1, A2, A3, A4>& exp) noexcept { return exp.value(); }
   template <class A2, class A3, class A4>
   static BOOST_MP_FORCEINLINE constexpr const Backend& function_arg_value(const detail::expression<detail::terminal, number<Backend>, A2, A3, A4>& exp) noexcept { return exp.value().backend(); }
   Backend                                                    m_backend;

 public:
   //
   // These shouldn't really need to be public, or even member functions, but it makes implementing
   // the non-member operators way easier if they are:
   //
   static BOOST_MP_FORCEINLINE constexpr const Backend& canonical_value(const self_type& v) noexcept { return v.m_backend; }
   template <class B2, expression_template_option ET>
   static BOOST_MP_FORCEINLINE constexpr const B2& canonical_value(const number<B2, ET>& v) noexcept { return v.backend(); }
   template <class B2, expression_template_option ET>
   static BOOST_MP_FORCEINLINE constexpr B2&& canonical_value(number<B2, ET>&& v) noexcept { return static_cast<number<B2, ET>&&>(v).backend(); }
   template <class V>
   static BOOST_MP_FORCEINLINE constexpr typename std::enable_if<!std::is_same<typename detail::canonical<V, Backend>::type, V>::value, typename detail::canonical<V, Backend>::type>::type
   canonical_value(const V& v) noexcept { return static_cast<typename detail::canonical<V, Backend>::type>(v); }
   template <class V>
   static BOOST_MP_FORCEINLINE constexpr typename std::enable_if<std::is_same<typename detail::canonical<V, Backend>::type, V>::value, const V&>::type
   canonical_value(const V& v) noexcept { return v; }
   static BOOST_MP_FORCEINLINE typename detail::canonical<std::string, Backend>::type canonical_value(const std::string& v) noexcept { return v.c_str(); }
};

template <class Backend, expression_template_option ExpressionTemplates>
inline std::ostream& operator<<(std::ostream& os, const number<Backend, ExpressionTemplates>& r)
{
   std::streamsize d  = os.precision();
   std::string     s  = r.str(d, os.flags());
   std::streamsize ss = os.width();
   if (ss > static_cast<std::streamsize>(s.size()))
   {
      char fill = os.fill();
      if ((os.flags() & std::ios_base::left) == std::ios_base::left)
         s.append(static_cast<std::string::size_type>(ss - s.size()), fill);
      else
         s.insert(static_cast<std::string::size_type>(0), static_cast<std::string::size_type>(ss - s.size()), fill);
   }
   return os << s;
}

template <class Backend, expression_template_option ExpressionTemplates>
std::string to_string(const number<Backend, ExpressionTemplates>& val)
{
  return val.str(6, std::ios_base::fixed|std::ios_base::showpoint);
}

namespace detail {

template <class tag, class A1, class A2, class A3, class A4>
inline std::ostream& operator<<(std::ostream& os, const expression<tag, A1, A2, A3, A4>& r)
{
   using value_type = typename expression<tag, A1, A2, A3, A4>::result_type;
   value_type                                                    temp(r);
   return os << temp;
}
//
// What follows is the input streaming code: this is not "proper" iostream code at all
// but that's fiendishly hard to write when dealing with multiple backends all
// with different requirements... yes we could deligate this to the backend author...
// but we really want backends to be EASY to write!
// For now just pull in all the characters that could possibly form the number
// and let the backend's string parser make use of it.  This fixes most use cases
// including CSV type formats such as those used by the Random lib.
//
inline std::string read_string_while(std::istream& is, std::string const& permitted_chars)
{
   std::ios_base::iostate     state = std::ios_base::goodbit;
   const std::istream::sentry sentry_check(is);
   std::string                result;

   if (sentry_check)
   {
      int c = is.rdbuf()->sgetc();

      for (;; c = is.rdbuf()->snextc())
         if (std::istream::traits_type::eq_int_type(std::istream::traits_type::eof(), c))
         { // end of file:
            state |= std::ios_base::eofbit;
            break;
         }
         else if (permitted_chars.find_first_of(std::istream::traits_type::to_char_type(c)) == std::string::npos)
         {
            // Invalid numeric character, stop reading:
            //is.rdbuf()->sputbackc(static_cast<char>(c));
            break;
         }
         else
         {
            result.append(1, std::istream::traits_type::to_char_type(c));
         }
   }

   if (!result.size())
      state |= std::ios_base::failbit;
   is.setstate(state);
   return result;
}

} // namespace detail

template <class Backend, expression_template_option ExpressionTemplates>
inline std::istream& operator>>(std::istream& is, number<Backend, ExpressionTemplates>& r)
{
   bool        hex_format = (is.flags() & std::ios_base::hex) == std::ios_base::hex;
   bool        oct_format = (is.flags() & std::ios_base::oct) == std::ios_base::oct;
   std::string s;
   switch (boost::multiprecision::number_category<number<Backend, ExpressionTemplates> >::value)
   {
   case boost::multiprecision::number_kind_integer:
      if (oct_format)
         s = detail::read_string_while(is, "+-01234567");
      else if (hex_format)
         s = detail::read_string_while(is, "+-xXabcdefABCDEF0123456789");
      else
         s = detail::read_string_while(is, "+-0123456789");
      break;
   case boost::multiprecision::number_kind_floating_point:
      s = detail::read_string_while(is, "+-eE.0123456789infINFnanNANinfinityINFINITY");
      break;
   default:
      is >> s;
   }
   if (s.size())
   {
      if (hex_format && (number_category<Backend>::value == number_kind_integer) && ((s[0] != '0') || (s[1] != 'x')))
         s.insert(s.find_first_not_of("+-"), "0x");
      if (oct_format && (number_category<Backend>::value == number_kind_integer) && (s[0] != '0'))
         s.insert(s.find_first_not_of("+-"), "0");
      r.assign(s);
   }
   else if (!is.fail())
      is.setstate(std::istream::failbit);
   return is;
}

template <class Backend, expression_template_option ExpressionTemplates>
BOOST_MP_FORCEINLINE BOOST_MP_CXX14_CONSTEXPR void swap(number<Backend, ExpressionTemplates>& a, number<Backend, ExpressionTemplates>& b)
    noexcept(noexcept(std::declval<number<Backend, ExpressionTemplates>&>() = std::declval<number<Backend, ExpressionTemplates>&>()))
{
   a.swap(b);
}
//
// Boost.Hash support, just call hash_value for the backend, which may or may not be supported:
//
template <class Backend, expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR std::size_t hash_value(const number<Backend, ExpressionTemplates>& val)
{
   return hash_value(val.backend());
}

} // namespace multiprecision

template <class T>
class rational;

template <class Backend, multiprecision::expression_template_option ExpressionTemplates>
inline std::istream& operator>>(std::istream& is, rational<multiprecision::number<Backend, ExpressionTemplates> >& r)
{
   std::string                                          s1;
   multiprecision::number<Backend, ExpressionTemplates> v1, v2;
   char                                                 c;
   bool                                                 have_hex   = false;
   bool                                                 hex_format = (is.flags() & std::ios_base::hex) == std::ios_base::hex;
   bool                                                 oct_format = (is.flags() & std::ios_base::oct) == std::ios_base::oct;

   while ((EOF != (c = static_cast<char>(is.peek()))) && (c == 'x' || c == 'X' || c == '-' || c == '+' || (c >= '0' && c <= '9') || (have_hex && (c >= 'a' && c <= 'f')) || (have_hex && (c >= 'A' && c <= 'F'))))
   {
      if (c == 'x' || c == 'X')
         have_hex = true;
      s1.append(1, c);
      is.get();
   }
   if (hex_format && ((s1[0] != '0') || (s1[1] != 'x')))
      s1.insert(static_cast<std::string::size_type>(0), "0x");
   if (oct_format && (s1[0] != '0'))
      s1.insert(static_cast<std::string::size_type>(0), "0");
   v1.assign(s1);
   s1.erase();
   if (c == '/')
   {
      is.get();
      while ((EOF != (c = static_cast<char>(is.peek()))) && (c == 'x' || c == 'X' || c == '-' || c == '+' || (c >= '0' && c <= '9') || (have_hex && (c >= 'a' && c <= 'f')) || (have_hex && (c >= 'A' && c <= 'F'))))
      {
         if (c == 'x' || c == 'X')
            have_hex = true;
         s1.append(1, c);
         is.get();
      }
      if (hex_format && ((s1[0] != '0') || (s1[1] != 'x')))
         s1.insert(static_cast<std::string::size_type>(0), "0x");
      if (oct_format && (s1[0] != '0'))
         s1.insert(static_cast<std::string::size_type>(0), "0");
      v2.assign(s1);
   }
   else
      v2 = 1;
   r.assign(v1, v2);
   return is;
}

template <class T, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<T, ExpressionTemplates> numerator(const rational<multiprecision::number<T, ExpressionTemplates> >& a)
{
   return a.numerator();
}

template <class T, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR multiprecision::number<T, ExpressionTemplates> denominator(const rational<multiprecision::number<T, ExpressionTemplates> >& a)
{
   return a.denominator();
}

template <class T, multiprecision::expression_template_option ExpressionTemplates>
inline BOOST_MP_CXX14_CONSTEXPR std::size_t hash_value(const rational<multiprecision::number<T, ExpressionTemplates> >& val)
{
   std::size_t result = hash_value(val.numerator());
   boost::multiprecision::detail::hash_combine(result, hash_value(val.denominator()));
   return result;
}

namespace multiprecision {

template <class I>
struct component_type<boost::rational<I> >
{
   using type = I;
};

} // namespace multiprecision

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // namespace boost

namespace std {

template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct hash<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
   BOOST_MP_CXX14_CONSTEXPR std::size_t operator()(const boost::multiprecision::number<Backend, ExpressionTemplates>& val) const { return hash_value(val); }
};
template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct hash<boost::rational<boost::multiprecision::number<Backend, ExpressionTemplates> > >
{
   BOOST_MP_CXX14_CONSTEXPR std::size_t operator()(const boost::rational<boost::multiprecision::number<Backend, ExpressionTemplates> >& val) const
   {
      std::size_t result = hash_value(val.numerator());
      boost::multiprecision::detail::hash_combine(result, hash_value(val.denominator()));
      return result;
   }
};

} // namespace std

#include <boost/multiprecision/detail/ublas_interop.hpp>

#endif

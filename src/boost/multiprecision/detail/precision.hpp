///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_PRECISION_HPP
#define BOOST_MP_PRECISION_HPP

#include <boost/multiprecision/traits/is_variable_precision.hpp>
#include <boost/multiprecision/detail/number_base.hpp>
#include <boost/multiprecision/detail/digits.hpp>

namespace boost { namespace multiprecision { namespace detail {

template <class B, boost::multiprecision::expression_template_option ET>
inline constexpr unsigned current_precision_of_last_chance_imp(const boost::multiprecision::number<B, ET>&, const std::integral_constant<int, 0>&)
{
   return std::numeric_limits<boost::multiprecision::number<B, ET> >::digits10;
}
template <class B, boost::multiprecision::expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR unsigned current_precision_of_last_chance_imp(const boost::multiprecision::number<B, ET>& val, const std::integral_constant<int, 1>&)
{
   //
   // We have an arbitrary precision integer, take it's "precision" as the
   // location of the most-significant-bit less the location of the
   // least-significant-bit, ie the number of bits required to represent the
   // the value assuming we will have an exponent to shift things by:
   //
   return val.is_zero() ? 1 : 1 + digits2_2_10(msb(abs(val)) - lsb(abs(val)) + 1);
}
template <class B, boost::multiprecision::expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR unsigned current_precision_of_last_chance_imp(const boost::multiprecision::number<B, ET>& val, const std::integral_constant<int, 2>&)
{
   //
   // We have an arbitrary precision rational, take it's "precision" as the
   // the larger of the "precision" of numerator and denominator:
   //
   return (std::max)(current_precision_of_last_chance_imp(numerator(val), std::integral_constant<int, 1>()), current_precision_of_last_chance_imp(denominator(val), std::integral_constant<int, 1>()));
}

template <class B, boost::multiprecision::expression_template_option ET>
inline BOOST_MP_CXX14_CONSTEXPR unsigned current_precision_of_imp(const boost::multiprecision::number<B, ET>& n, const std::integral_constant<bool, true>&)
{
   return n.precision();
}
template <class B, boost::multiprecision::expression_template_option ET>
inline constexpr unsigned current_precision_of_imp(const boost::multiprecision::number<B, ET>& val, const std::integral_constant<bool, false>&)
{
   using tag = std::integral_constant<int,
                                      std::numeric_limits<boost::multiprecision::number<B, ET> >::is_specialized &&
                                              std::numeric_limits<boost::multiprecision::number<B, ET> >::is_integer &&
                                              std::numeric_limits<boost::multiprecision::number<B, ET> >::is_exact &&
                                              !std::numeric_limits<boost::multiprecision::number<B, ET> >::is_modulo
                                          ? 1
                                      : boost::multiprecision::number_category<boost::multiprecision::number<B, ET> >::value == boost::multiprecision::number_kind_rational ? 2
                                                                                                                                                                            : 0>;
   return current_precision_of_last_chance_imp(val, tag());
}

template <class R, class Terminal>
inline constexpr unsigned current_precision_of_terminal(const Terminal&)
{
   return (R::thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision) 
      ? (std::numeric_limits<Terminal>::min_exponent ? std::numeric_limits<Terminal>::digits10 : 1 + std::numeric_limits<Terminal>::digits10) : 0;
}
template <class R, class Terminal>
inline constexpr unsigned current_precision_of(const Terminal& r)
{
   return current_precision_of_terminal<R>(R::canonical_value(r));
}
template <class R>
inline constexpr unsigned current_precision_of(const float&)
{
   using list = typename R::backend_type::float_types;
   using first_float = typename std::tuple_element<0, list>::type;

   return (R::thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision) ? std::numeric_limits<first_float>::digits10 : 0;
}

template <class R, class Terminal, std::size_t N>
inline constexpr unsigned current_precision_of(const Terminal (&)[N])
{ // For string literals:
   return 0;
}

template <class R, class B, boost::multiprecision::expression_template_option ET>
inline constexpr unsigned current_precision_of_imp(const boost::multiprecision::number<B, ET>& n, const std::true_type&)
{
   return std::is_same<R, boost::multiprecision::number<B, ET> >::value 
      || (std::is_same<typename R::value_type, boost::multiprecision::number<B, ET> >::value && (R::thread_default_variable_precision_options() >= variable_precision_options::preserve_component_precision))
      || (R::thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision) 
      ? current_precision_of_imp(n, boost::multiprecision::detail::is_variable_precision<boost::multiprecision::number<B, ET> >()) : 0;
}
template <class R, class B, boost::multiprecision::expression_template_option ET>
inline constexpr unsigned current_precision_of_imp(const boost::multiprecision::number<B, ET>& n, const std::false_type&)
{
   return std::is_same<R, boost::multiprecision::number<B, ET> >::value 
      || std::is_same<typename R::value_type, boost::multiprecision::number<B, ET> >::value
      ? current_precision_of_imp(n, boost::multiprecision::detail::is_variable_precision<boost::multiprecision::number<B, ET> >()) : 0;
}

template <class R, class B, boost::multiprecision::expression_template_option ET>
inline constexpr unsigned current_precision_of(const boost::multiprecision::number<B, ET>& n)
{
   return current_precision_of_imp<R>(n, boost::multiprecision::detail::is_variable_precision<R>());
}

template <class R, class tag, class Arg1>
inline constexpr unsigned current_precision_of(const expression<tag, Arg1, void, void, void>& expr)
{
   return current_precision_of<R>(expr.left_ref());
}

template <class R, class Arg1>
inline constexpr unsigned current_precision_of(const expression<terminal, Arg1, void, void, void>& expr)
{
   return current_precision_of<R>(expr.value());
}

template <class R, class tag, class Arg1, class Arg2>
inline constexpr unsigned current_precision_of(const expression<tag, Arg1, Arg2, void, void>& expr)
{
   return (std::max)(current_precision_of<R>(expr.left_ref()), current_precision_of<R>(expr.right_ref()));
}

template <class R, class tag, class Arg1, class Arg2, class Arg3>
inline constexpr unsigned current_precision_of(const expression<tag, Arg1, Arg2, Arg3, void>& expr)
{
   return (std::max)((std::max)(current_precision_of<R>(expr.left_ref()), current_precision_of<R>(expr.right_ref())), current_precision_of<R>(expr.middle_ref()));
}

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4130)
#endif

template <class R, bool = boost::multiprecision::detail::is_variable_precision<R>::value>
struct scoped_default_precision
{
   template <class T>
   constexpr scoped_default_precision(const T&) {}
   template <class T, class U>
   constexpr scoped_default_precision(const T&, const U&) {}
   template <class T, class U, class V>
   constexpr scoped_default_precision(const T&, const U&, const V&) {}

   //
   // This function is never called: in C++17 it won't be compiled either:
   //
   unsigned precision() const
   {
      BOOST_ASSERT("This function should never be called!!" == 0);
      return 0;
   }
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template <class R>
struct scoped_default_precision<R, true>
{
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR scoped_default_precision(const T& a)
   {
      init(has_uniform_precision() ? R::thread_default_precision() : (std::max)(R::thread_default_precision(), current_precision_of<R>(a)));
   }
   template <class T, class U>
   BOOST_MP_CXX14_CONSTEXPR scoped_default_precision(const T& a, const U& b)
   {
      init(has_uniform_precision() ? R::thread_default_precision() : (std::max)(R::thread_default_precision(), (std::max)(current_precision_of<R>(a), current_precision_of<R>(b))));
   }
   template <class T, class U, class V>
   BOOST_MP_CXX14_CONSTEXPR scoped_default_precision(const T& a, const U& b, const V& c)
   {
      init(has_uniform_precision() ? R::thread_default_precision() : (std::max)((std::max)(current_precision_of<R>(a), current_precision_of<R>(b)), (std::max)(R::thread_default_precision(), current_precision_of<R>(c))));
   }
   ~scoped_default_precision()
   {
      if(m_new_prec != m_old_prec)
         R::thread_default_precision(m_old_prec);
   }
   BOOST_MP_CXX14_CONSTEXPR unsigned precision() const
   {
      return m_new_prec;
   }

   static constexpr bool has_uniform_precision()
   {
      return R::thread_default_variable_precision_options() <= boost::multiprecision::variable_precision_options::assume_uniform_precision;
   }

 private:
   BOOST_MP_CXX14_CONSTEXPR void init(unsigned p)
   {
      m_old_prec = R::thread_default_precision();
      if (p && (p != m_old_prec))
      {
         R::thread_default_precision(p);
         m_new_prec = p;
      }
      else
         m_new_prec = m_old_prec;
   }
   unsigned m_old_prec, m_new_prec;
};

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void maybe_promote_precision(T*, const std::integral_constant<bool, false>&) {}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void maybe_promote_precision(T* obj, const std::integral_constant<bool, true>&)
{
   if (obj->precision() != T::thread_default_precision())
   {
      obj->precision(T::thread_default_precision());
   }
}

template <class T>
inline BOOST_MP_CXX14_CONSTEXPR void maybe_promote_precision(T* obj)
{
   maybe_promote_precision(obj, std::integral_constant<bool, boost::multiprecision::detail::is_variable_precision<T>::value>());
}

#ifndef BOOST_NO_CXX17_IF_CONSTEXPR
#define BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(T) \
   if                                               \
   constexpr(boost::multiprecision::detail::is_variable_precision<T>::value)
#else
#define BOOST_MP_CONSTEXPR_IF_VARIABLE_PRECISION(T) if (boost::multiprecision::detail::is_variable_precision<T>::value)
#endif

template <class T, bool = boost::multiprecision::detail::is_variable_precision<T>::value>
struct scoped_target_precision
{
   variable_precision_options opts;
   scoped_target_precision() : opts(T::thread_default_variable_precision_options())
   {
      T::thread_default_variable_precision_options(variable_precision_options::preserve_target_precision);
   }
   ~scoped_target_precision()
   {
      T::thread_default_variable_precision_options(opts);
   }
};
template <class T>
struct scoped_target_precision<T, false> {};

template <class T, bool = boost::multiprecision::detail::is_variable_precision<T>::value>
struct scoped_source_precision
{
   variable_precision_options opts;
   scoped_source_precision() : opts(T::thread_default_variable_precision_options())
   {
      T::thread_default_variable_precision_options(variable_precision_options::preserve_source_precision);
   }
   ~scoped_source_precision()
   {
      T::thread_default_variable_precision_options(opts);
   }
};
template <class T>
struct scoped_source_precision<T, false> {};

template <class T, bool = boost::multiprecision::detail::is_variable_precision<T>::value>
struct scoped_precision_options
{
   unsigned saved_digits;
   boost::multiprecision::variable_precision_options saved_options;

   scoped_precision_options(unsigned digits) 
      : saved_digits(T::thread_default_precision()), saved_options(T::thread_default_variable_precision_options())
   {
      T::thread_default_precision(digits);
   }
   scoped_precision_options(unsigned digits, variable_precision_options opts)
      : saved_digits(T::thread_default_precision()), saved_options(T::thread_default_variable_precision_options())
   {
      T::thread_default_precision(digits);
      T::thread_default_variable_precision_options(opts);
   }
   template <class U>
   scoped_precision_options(const U& u)
      : saved_digits(T::thread_default_precision()), saved_options(T::thread_default_variable_precision_options())
   {
      T::thread_default_precision(u.precision());
      T::thread_default_variable_precision_options(U::thread_default_variable_precision_options());
   }
   ~scoped_precision_options()
   {
      T::thread_default_variable_precision_options(saved_options);
      T::thread_default_precision(saved_digits);
   }
};

template <class T>
struct scoped_precision_options<T, false>
{
   scoped_precision_options(unsigned) {}
   scoped_precision_options(unsigned, variable_precision_options) {}
   template <class U>
   scoped_precision_options(const U&) {}
   ~scoped_precision_options() {}
};

}
}
} // namespace boost::multiprecision::detail

#endif // BOOST_MP_IS_BACKEND_HPP

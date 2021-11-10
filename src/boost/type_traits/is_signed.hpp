
//  (C) Copyright John Maddock 2005.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_IS_SIGNED_HPP_INCLUDED
#define BOOST_TT_IS_SIGNED_HPP_INCLUDED

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <climits>

namespace boost {

#if !defined( BOOST_CODEGEARC )

#if !(defined(BOOST_MSVC) && BOOST_MSVC <= 1310) && \
    !(defined(__EDG_VERSION__) && __EDG_VERSION__ <= 238) &&\
    !defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)

namespace detail{

template <class T>
struct is_signed_values
{
   //
   // Note that we cannot use BOOST_STATIC_CONSTANT here, using enum's
   // rather than "real" static constants simply doesn't work or give
   // the correct answer.
   //
   typedef typename remove_cv<T>::type no_cv_t;
   static const no_cv_t minus_one = (static_cast<no_cv_t>(-1));
   static const no_cv_t zero = (static_cast<no_cv_t>(0));
};

template <class T>
struct is_signed_helper
{
   typedef typename remove_cv<T>::type no_cv_t;
   BOOST_STATIC_CONSTANT(bool, value = (!(::boost::detail::is_signed_values<T>::minus_one  > boost::detail::is_signed_values<T>::zero)));
};

template <bool integral_type>
struct is_signed_select_helper
{
   template <class T>
   struct rebind
   {
      typedef is_signed_helper<T> type;
   };
};

template <>
struct is_signed_select_helper<false>
{
   template <class T>
   struct rebind
   {
      typedef false_type type;
   };
};

template <class T>
struct is_signed_impl
{
   typedef ::boost::detail::is_signed_select_helper< ::boost::is_integral<T>::value || ::boost::is_enum<T>::value> selector;
   typedef typename selector::template rebind<T> binder;
   typedef typename binder::type type;
   BOOST_STATIC_CONSTANT(bool, value = type::value);
};

}

template <class T> struct is_signed : public integral_constant<bool, boost::detail::is_signed_impl<T>::value> {};

#else

template <class T> struct is_signed : public false_type{};

#endif

#else //defined( BOOST_CODEGEARC )
   template <class T> struct is_signed : public integral_constant<bool, __is_signed(T)>{};
#endif

template <> struct is_signed<signed char> : public true_type{};
template <> struct is_signed<const signed char> : public true_type{};
template <> struct is_signed<volatile signed char> : public true_type{};
template <> struct is_signed<const volatile signed char> : public true_type{};
template <> struct is_signed<short> : public true_type{};
template <> struct is_signed<const short> : public true_type{};
template <> struct is_signed<volatile short> : public true_type{};
template <> struct is_signed<const volatile short> : public true_type{};
template <> struct is_signed<int> : public true_type{};
template <> struct is_signed<const int> : public true_type{};
template <> struct is_signed<volatile int> : public true_type{};
template <> struct is_signed<const volatile int> : public true_type{};
template <> struct is_signed<long> : public true_type{};
template <> struct is_signed<const long> : public true_type{};
template <> struct is_signed<volatile long> : public true_type{};
template <> struct is_signed<const volatile long> : public true_type{};

template <> struct is_signed<unsigned char> : public false_type{};
template <> struct is_signed<const unsigned char> : public false_type{};
template <> struct is_signed<volatile unsigned char> : public false_type{};
template <> struct is_signed<const volatile unsigned char> : public false_type{};
template <> struct is_signed<unsigned short> : public false_type{};
template <> struct is_signed<const unsigned short> : public false_type{};
template <> struct is_signed<volatile unsigned short> : public false_type{};
template <> struct is_signed<const volatile unsigned short> : public false_type{};
template <> struct is_signed<unsigned int> : public false_type{};
template <> struct is_signed<const unsigned int> : public false_type{};
template <> struct is_signed<volatile unsigned int> : public false_type{};
template <> struct is_signed<const volatile unsigned int> : public false_type{};
template <> struct is_signed<unsigned long> : public false_type{};
template <> struct is_signed<const unsigned long> : public false_type{};
template <> struct is_signed<volatile unsigned long> : public false_type{};
template <> struct is_signed<const volatile unsigned long> : public false_type{};
#ifdef BOOST_HAS_LONG_LONG
template <> struct is_signed< ::boost::long_long_type> : public true_type{};
template <> struct is_signed<const ::boost::long_long_type> : public true_type{};
template <> struct is_signed<volatile ::boost::long_long_type> : public true_type{};
template <> struct is_signed<const volatile ::boost::long_long_type> : public true_type{};

template <> struct is_signed< ::boost::ulong_long_type> : public false_type{};
template <> struct is_signed<const ::boost::ulong_long_type> : public false_type{};
template <> struct is_signed<volatile ::boost::ulong_long_type> : public false_type{};
template <> struct is_signed<const volatile ::boost::ulong_long_type> : public false_type{};
#endif
#if defined(CHAR_MIN) 
#if CHAR_MIN != 0
template <> struct is_signed<char> : public true_type{};
template <> struct is_signed<const char> : public true_type{};
template <> struct is_signed<volatile char> : public true_type{};
template <> struct is_signed<const volatile char> : public true_type{};
#else
template <> struct is_signed<char> : public false_type{};
template <> struct is_signed<const char> : public false_type{};
template <> struct is_signed<volatile char> : public false_type{};
template <> struct is_signed<const volatile char> : public false_type{};
#endif
#endif
#if defined(WCHAR_MIN) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
#if WCHAR_MIN != 0
template <> struct is_signed<wchar_t> : public true_type{};
template <> struct is_signed<const wchar_t> : public true_type{};
template <> struct is_signed<volatile wchar_t> : public true_type{};
template <> struct is_signed<const volatile wchar_t> : public true_type{};
#else
template <> struct is_signed<wchar_t> : public false_type{};
template <> struct is_signed<const wchar_t> : public false_type{};
template <> struct is_signed<volatile wchar_t> : public false_type{};
template <> struct is_signed<const volatile wchar_t> : public false_type{};
#endif
#endif
} // namespace boost

#endif // BOOST_TT_IS_MEMBER_FUNCTION_POINTER_HPP_INCLUDED

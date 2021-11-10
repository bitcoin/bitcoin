
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_INTEGRAL_HPP_INCLUDED
#define BOOST_TT_IS_INTEGRAL_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/integral_constant.hpp>

namespace boost {

#if defined( BOOST_CODEGEARC )
   template <class T>
   struct is_integral : public integral_constant<bool, __is_integral(T)> {};
#else

template <class T> struct is_integral : public false_type {};
template <class T> struct is_integral<const T> : public is_integral<T> {};
template <class T> struct is_integral<volatile const T> : public is_integral<T>{};
template <class T> struct is_integral<volatile T> : public is_integral<T>{};

//* is a type T an [cv-qualified-] integral type described in the standard (3.9.1p3)
// as an extension we include long long, as this is likely to be added to the
// standard at a later date
template<> struct is_integral<unsigned char> : public true_type {};
template<> struct is_integral<unsigned short> : public true_type{};
template<> struct is_integral<unsigned int> : public true_type{};
template<> struct is_integral<unsigned long> : public true_type{};

template<> struct is_integral<signed char> : public true_type{};
template<> struct is_integral<short> : public true_type{};
template<> struct is_integral<int> : public true_type{};
template<> struct is_integral<long> : public true_type{};

template<> struct is_integral<char> : public true_type{};
template<> struct is_integral<bool> : public true_type{};

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
// If the following line fails to compile and you're using the Intel
// compiler, see http://lists.boost.org/MailArchives/boost-users/msg06567.php,
// and define BOOST_NO_INTRINSIC_WCHAR_T on the command line.
template<> struct is_integral<wchar_t> : public true_type{};
#endif

// Same set of integral types as in boost/type_traits/integral_promotion.hpp.
// Please, keep in sync. -- Alexander Nasonov
#if (defined(BOOST_INTEL_CXX_VERSION) && defined(_MSC_VER) && (BOOST_INTEL_CXX_VERSION <= 600)) \
    || (defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x600) && (_MSC_VER < 1300))
template<> struct is_integral<unsigned __int8> : public true_type{};
template<> struct is_integral<unsigned __int16> : public true_type{};
template<> struct is_integral<unsigned __int32> : public true_type{};
template<> struct is_integral<__int8> : public true_type{};
template<> struct is_integral<__int16> : public true_type{};
template<> struct is_integral<__int32> : public true_type{};
#ifdef BOOST_BORLANDC
template<> struct is_integral<unsigned __int64> : public true_type{};
template<> struct is_integral<__int64> : public true_type{};
#endif
#endif

# if defined(BOOST_HAS_LONG_LONG)
template<> struct is_integral< ::boost::ulong_long_type> : public true_type{};
template<> struct is_integral< ::boost::long_long_type> : public true_type{};
#elif defined(BOOST_HAS_MS_INT64)
template<> struct is_integral<unsigned __int64> : public true_type{};
template<> struct is_integral<__int64> : public true_type{};
#endif
        
#ifdef BOOST_HAS_INT128
template<> struct is_integral<boost::int128_type> : public true_type{};
template<> struct is_integral<boost::uint128_type> : public true_type{};
#endif
#ifndef BOOST_NO_CXX11_CHAR16_T
template<> struct is_integral<char16_t> : public true_type{};
#endif
#ifndef BOOST_NO_CXX11_CHAR32_T
template<> struct is_integral<char32_t> : public true_type{};
#endif

#endif  // non-CodeGear implementation

} // namespace boost

#endif // BOOST_TT_IS_INTEGRAL_HPP_INCLUDED

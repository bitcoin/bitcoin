// Copyright Paul A. Bristow 2017.
// Copyright John Maddock 2017.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// test_value.hpp

#ifndef TEST_VALUE_HPP
#define TEST_VALUE_HPP

// BOOST_MATH_TEST_VALUE is used to create a test value of suitable type from a decimal digit string.
// Two parameters, both a floating-point literal double like 1.23 (not long double so no suffix L)
// and a decimal digit string const char* like "1.23" must be provided.
// The decimal value represented must be the same of course, with at least enough precision for long double.
//   Note there are two gotchas to this approach:
// * You need all values to be real floating-point values
// * and *MUST* include a decimal point (to avoid confusion with an integer literal).
// * It's slow to compile compared to a simple literal.

// Speed is not an issue for a few test values,
// but it's not generally usable in large tables
// where you really need everything to be statically initialized.

// Macro BOOST_MATH_INSTRUMENT_CREATE_TEST_VALUE provides a global diagnostic value for create_type.

#include <boost/cstdfloat.hpp> // For float_64_t, float128_t. Must be first include!
#include <boost/math/tools/lexical_cast.hpp>
#include <limits>
#include <type_traits>

#ifdef BOOST_MATH_INSTRUMENT_CREATE_TEST_VALUE
// global int create_type(0); must be defined before including this file.
#endif

#ifdef BOOST_HAS_FLOAT128
typedef __float128 largest_float;
#define BOOST_MATH_TEST_LARGEST_FLOAT_SUFFIX(x) x##Q
#define BOOST_MATH_TEST_LARGEST_FLOAT_DIGITS 113
#else
typedef long double largest_float;
#define BOOST_MATH_TEST_LARGEST_FLOAT_SUFFIX(x) x##L
#define BOOST_MATH_TEST_LARGEST_FLOAT_DIGITS std::numeric_limits<long double>::digits
#endif

template <class T, class T2>
inline T create_test_value(largest_float val, const char*, const std::true_type&, const T2&)
{ // Construct from long double or quad parameter val (ignoring string/const char* str).
  // (This is case for MPL parameters = true_ and T2 == false_,
  // and  MPL parameters = true_ and T2 == true_  cpp_bin_float)
  // All built-in/fundamental floating-point types,
  // and other User-Defined Types that can be constructed without loss of precision
  // from long double suffix L (or quad suffix Q),
  //
  // Choose this method, even if can be constructed from a string,
  // because it will be faster, and more likely to be the closest representation.
  // (This is case for MPL parameters = true_type and T2 == true_type).
  #ifdef BOOST_MATH_INSTRUMENT_CREATE_TEST_VALUE
  create_type = 1;
  #endif
  return static_cast<T>(val);
}

template <class T>
inline T create_test_value(largest_float, const char* str, const std::false_type&, const std::true_type&)
{ // Construct from decimal digit string const char* @c str (ignoring long double parameter).
  // For example, extended precision or other User-Defined types which ARE constructible from a string
  // (but not from double, or long double without loss of precision).
  // (This is case for MPL parameters = false_type and T2 == true_type).
  #ifdef BOOST_MATH_INSTRUMENT_CREATE_TEST_VALUE
  create_type = 2;
  #endif
  return T(str);
}

template <class T>
inline T create_test_value(largest_float, const char* str, const std::false_type&, const std::false_type&)
{ // Create test value using from lexical cast of decimal digit string const char* str.
  // For example, extended precision or other User-Defined types which are NOT constructible from a string
  // (NOR constructible from a long double).
    // (This is case T1 = false_type and T2 == false_type).
  #ifdef BOOST_MATH_INSTRUMENT_CREATE_TEST_VALUE
  create_type = 3;
  #elif defined(BOOST_MATH_STANDALONE)
  static_assert(sizeof(T) == 0, "Can not create a test value using lexical cast of string in standalone mode");
  #endif
  return boost::lexical_cast<T>(str);
}

// T real type, x a decimal digits representation of a floating-point, for example: 12.34.
// It must include a decimal point (or it would be interpreted as an integer).

//  x is converted to a long double by appending the letter L (to suit long double fundamental type), 12.34L.
//  x is also passed as a const char* or string representation "12.34"
//  (to suit most other types that cannot be constructed from long double without possible loss).

// BOOST_MATH_TEST_LARGEST_FLOAT_SUFFIX(x) makes a long double or quad version, with
// suffix a letter L (or Q) to suit long double (or quad) fundamental type, 12.34L or 12.34Q.
// #x makes a decimal digit string version to suit multiprecision and fixed_point constructors, "12.34".
// (Constructing from double or long double (or quad) could lose precision for multiprecision or fixed-point).

// The matching create_test_value function above is chosen depending on the T1 and T2 mpl bool truths.
// The string version from #x is used if the precision of T is greater than long double.

// Example: long double test_value = BOOST_MATH_TEST_VALUE(double, 1.23456789);

#define BOOST_MATH_TEST_VALUE(T, x) create_test_value<T>(\
  BOOST_MATH_TEST_LARGEST_FLOAT_SUFFIX(x),\
  #x,\
  std::integral_constant<bool, \
    std::numeric_limits<T>::is_specialized &&\
      (std::numeric_limits<T>::radix == 2)\
        && (std::numeric_limits<T>::digits <= BOOST_MATH_TEST_LARGEST_FLOAT_DIGITS)\
        && std::is_convertible<largest_float, T>::value>(),\
  std::integral_constant<bool, \
    std::is_constructible<T, const char*>::value>()\
)
#endif // TEST_VALUE_HPP

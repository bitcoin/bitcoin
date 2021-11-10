// boost\math\tools\promotion.hpp

// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2006.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Promote arguments functions to allow math functions to have arguments
// provided as integer OR real (floating-point, built-in or UDT)
// (called ArithmeticType in functions that use promotion)
// that help to reduce the risk of creating multiple instantiations.
// Allows creation of an inline wrapper that forwards to a foo(RT, RT) function,
// so you never get to instantiate any mixed foo(RT, IT) functions.

#ifndef BOOST_MATH_PROMOTION_HPP
#define BOOST_MATH_PROMOTION_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <type_traits>

namespace boost
{
  namespace math
  {
    namespace tools
    {
      // If either T1 or T2 is an integer type,
      // pretend it was a double (for the purposes of further analysis).
      // Then pick the wider of the two floating-point types
      // as the actual signature to forward to.
      // For example:
      // foo(int, short) -> double foo(double, double);
      // foo(int, float) -> double foo(double, double);
      // Note: NOT float foo(float, float)
      // foo(int, double) -> foo(double, double);
      // foo(double, float) -> double foo(double, double);
      // foo(double, float) -> double foo(double, double);
      // foo(any-int-or-float-type, long double) -> foo(long double, long double);
      // but ONLY float foo(float, float) is unchanged.
      // So the only way to get an entirely float version is to call foo(1.F, 2.F),
      // But since most (all?) the math functions convert to double internally,
      // probably there would not be the hoped-for gain by using float here.

      // This follows the C-compatible conversion rules of pow, etc
      // where pow(int, float) is converted to pow(double, double).

      template <class T>
      struct promote_arg
      { // If T is integral type, then promote to double.
        using type = typename std::conditional<std::is_integral<T>::value, double, T>::type;
      };
      // These full specialisations reduce std::conditional usage and speed up
      // compilation:
      template <> struct promote_arg<float> { using type = float; };
      template <> struct promote_arg<double>{ using type = double; };
      template <> struct promote_arg<long double> { using type = long double; };
      template <> struct promote_arg<int> {  using type = double; };

      template <class T1, class T2>
      struct promote_args_2
      { // Promote, if necessary, & pick the wider of the two floating-point types.
        // for both parameter types, if integral promote to double.
        using T1P = typename promote_arg<T1>::type; // T1 perhaps promoted.
        using T2P = typename promote_arg<T2>::type; // T2 perhaps promoted.

        using type = typename std::conditional<
          std::is_floating_point<T1P>::value && std::is_floating_point<T2P>::value, // both T1P and T2P are floating-point?
#ifdef BOOST_MATH_USE_FLOAT128
           typename std::conditional<std::is_same<__float128, T1P>::value || std::is_same<__float128, T2P>::value, // either long double?
            __float128,
#endif
             typename std::conditional<std::is_same<long double, T1P>::value || std::is_same<long double, T2P>::value, // either long double?
               long double, // then result type is long double.
               typename std::conditional<std::is_same<double, T1P>::value || std::is_same<double, T2P>::value, // either double?
                  double, // result type is double.
                  float // else result type is float.
             >::type
#ifdef BOOST_MATH_USE_FLOAT128
             >::type
#endif
             >::type,
          // else one or the other is a user-defined type:
          typename std::conditional<!std::is_floating_point<T2P>::value && std::is_convertible<T1P, T2P>::value, T2P, T1P>::type>::type;
      }; // promote_arg2
      // These full specialisations reduce std::conditional usage and speed up
      // compilation:
      template <> struct promote_args_2<float, float> { using type = float; };
      template <> struct promote_args_2<double, double>{ using type = double; };
      template <> struct promote_args_2<long double, long double> { using type = long double; };
      template <> struct promote_args_2<int, int> {  using type = double; };
      template <> struct promote_args_2<int, float> {  using type = double; };
      template <> struct promote_args_2<float, int> {  using type = double; };
      template <> struct promote_args_2<int, double> {  using type = double; };
      template <> struct promote_args_2<double, int> {  using type = double; };
      template <> struct promote_args_2<int, long double> {  using type = long double; };
      template <> struct promote_args_2<long double, int> {  using type = long double; };
      template <> struct promote_args_2<float, double> {  using type = double; };
      template <> struct promote_args_2<double, float> {  using type = double; };
      template <> struct promote_args_2<float, long double> {  using type = long double; };
      template <> struct promote_args_2<long double, float> {  using type = long double; };
      template <> struct promote_args_2<double, long double> {  using type = long double; };
      template <> struct promote_args_2<long double, double> {  using type = long double; };

      template <class T1, class T2=float, class T3=float, class T4=float, class T5=float, class T6=float>
      struct promote_args
      {
         using type = typename promote_args_2<
            typename std::remove_cv<T1>::type,
            typename promote_args_2<
               typename std::remove_cv<T2>::type,
               typename promote_args_2<
                  typename std::remove_cv<T3>::type,
                  typename promote_args_2<
                     typename std::remove_cv<T4>::type,
                     typename promote_args_2<
                        typename std::remove_cv<T5>::type, typename std::remove_cv<T6>::type
                     >::type
                  >::type
               >::type
            >::type
         >::type;

#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
         //
         // Guard against use of long double if it's not supported:
         //
         static_assert((0 == std::is_same<type, long double>::value), "Sorry, but this platform does not have sufficient long double support for the special functions to be reliably implemented.");
#endif
      };

      //
      // This struct is the same as above, but has no static assert on long double usage,
      // it should be used only on functions that can be implemented for long double
      // even when std lib support is missing or broken for that type.
      //
      template <class T1, class T2=float, class T3=float, class T4=float, class T5=float, class T6=float>
      struct promote_args_permissive
      {
         using type = typename promote_args_2<
            typename std::remove_cv<T1>::type,
            typename promote_args_2<
               typename std::remove_cv<T2>::type,
               typename promote_args_2<
                  typename std::remove_cv<T3>::type,
                  typename promote_args_2<
                     typename std::remove_cv<T4>::type,
                     typename promote_args_2<
                        typename std::remove_cv<T5>::type, typename std::remove_cv<T6>::type
                     >::type
                  >::type
               >::type
            >::type
         >::type;
      };

    } // namespace tools
  } // namespace math
} // namespace boost

#endif // BOOST_MATH_PROMOTION_HPP


// Copyright John Maddock 2017.
// Copyright Paul A. Bristow 2016, 2017, 2018.
// Copyright Nicholas Thompson 2018

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
//  copy at http ://www.boost.org/LICENSE_1_0.txt).

#ifndef BOOST_MATH_SF_LAMBERT_W_HPP
#define BOOST_MATH_SF_LAMBERT_W_HPP

#ifdef _MSC_VER
#pragma warning(disable : 4127)
#endif

/*
Implementation of an algorithm for the Lambert W0 and W-1 real-only functions.

This code is based in part on the algorithm by
Toshio Fukushima,
"Precise and fast computation of Lambert W-functions without transcendental function evaluations",
J.Comp.Appl.Math. 244 (2013) 77-89,
and on a C/C++ version by Darko Veberic, darko.veberic@ijs.si
based on the Fukushima algorithm and Toshio Fukushima's FORTRAN version of his algorithm.

First derivative of Lambert_w is derived from
Princeton Companion to Applied Mathematics, 'The Lambert-W function', Section 1.3: Series and Generating Functions.

*/

/*
TODO revise this list of macros.
Some macros that will show some (or much) diagnostic values if #defined.
//[boost_math_instrument_lambert_w_macros

// #define-able macros
BOOST_MATH_INSTRUMENT_LAMBERT_W_HALLEY                     // Halley refinement diagnostics.
BOOST_MATH_INSTRUMENT_LAMBERT_W_PRECISION                  // Precision.
BOOST_MATH_INSTRUMENT_LAMBERT_WM1                          // W1 branch diagnostics.
BOOST_MATH_INSTRUMENT_LAMBERT_WM1_HALLEY                   // Halley refinement diagnostics only for W-1 branch.
BOOST_MATH_INSTRUMENT_LAMBERT_WM1_TINY                     // K > 64, z > -1.0264389699511303e-26
BOOST_MATH_INSTRUMENT_LAMBERT_WM1_LOOKUP                   // Show results from W-1 lookup table.
BOOST_MATH_INSTRUMENT_LAMBERT_W_SCHROEDER                  // Schroeder refinement diagnostics.
BOOST_MATH_INSTRUMENT_LAMBERT_W_TERMS                      // Number of terms used for near-singularity series.
BOOST_MATH_INSTRUMENT_LAMBERT_W_SINGULARITY_SERIES         // Show evaluation of series near branch singularity.
BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES_ITERATIONS  // Show evaluation of series for small z.
//] [/boost_math_instrument_lambert_w_macros]
*/

#include <boost/math/policies/error_handling.hpp>
#include <boost/math/policies/policy.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/log1p.hpp> // for log (1 + x)
#include <boost/math/constants/constants.hpp> // For exp_minus_one == 3.67879441171442321595523770161460867e-01.
#include <boost/math/special_functions/pow.hpp> // powers with compile time exponent, used in arbitrary precision code.
#include <boost/math/tools/series.hpp> // series functor.
//#include <boost/math/tools/polynomial.hpp>  // polynomial.
#include <boost/math/tools/rational.hpp>  // evaluate_polynomial.
#include <boost/math/tools/precision.hpp> // boost::math::tools::max_value().
#include <boost/math/tools/big_constant.hpp>
#include <boost/math/tools/cxx03_warn.hpp>

#include <limits>
#include <cmath>
#include <limits>
#include <exception>
#include <type_traits>
#include <cstdint>

// Needed for testing and diagnostics only.
#include <iostream>
#include <typeinfo>
#include <boost/math/special_functions/next.hpp>  // For float_distance.

using lookup_t = double; // Type for lookup table (double or float, or even long double?)

//#include "J:\Cpp\Misc\lambert_w_lookup_table_generator\lambert_w_lookup_table.ipp"
// #include "lambert_w_lookup_table.ipp" // Boost.Math version.
#include <boost/math/special_functions/detail/lambert_w_lookup_table.ipp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

namespace boost {
namespace math {
namespace lambert_w_detail {

//! \brief Applies a single Halley step to make a better estimate of Lambert W.
//! \details Used the simplified formulae obtained from
//! http://www.wolframalpha.com/input/?i=%5B2(z+exp(z)-w)+d%2Fdx+(z+exp(z)-w)%5D+%2F+%5B2+(d%2Fdx+(z+exp(z)-w))%5E2+-+(z+exp(z)-w)+d%5E2%2Fdx%5E2+(z+exp(z)-w)%5D
//! [2(z exp(z)-w) d/dx (z exp(z)-w)] / [2 (d/dx (z exp(z)-w))^2 - (z exp(z)-w) d^2/dx^2 (z exp(z)-w)]

//! \tparam T floating-point (or fixed-point) type.
//! \param w_est Lambert W estimate.
//! \param z Argument z for Lambert_w function.
//! \returns New estimate of Lambert W, hopefully improved.
//!
template <typename T>
inline T lambert_w_halley_step(T w_est, const T z)
{
  BOOST_MATH_STD_USING
  T e = exp(w_est);
  w_est -= 2 * (w_est + 1) * (e * w_est - z) / (z * (w_est + 2) + e * (w_est * (w_est + 2) + 2));
  return w_est;
} // template <typename T> lambert_w_halley_step(T w_est, T z)

//! \brief Halley iterate to refine Lambert_w estimate,
//! taking at least one Halley_step.
//! Repeat Halley steps until the *last step* had fewer than half the digits wrong,
//! the step we've just taken should have been sufficient to have completed the iteration.

//! \tparam T floating-point (or fixed-point) type.
//! \param z Argument z for Lambert_w function.
//! \param w_est Lambert w estimate.
template <typename T>
inline T lambert_w_halley_iterate(T w_est, const T z)
{
  BOOST_MATH_STD_USING
  static const T max_diff = boost::math::tools::root_epsilon<T>() * fabs(w_est);

  T w_new = lambert_w_halley_step(w_est, z);
  T diff = fabs(w_est - w_new);
  while (diff > max_diff)
  {
    w_est = w_new;
    w_new = lambert_w_halley_step(w_est, z);
    diff = fabs(w_est - w_new);
  }
  return w_new;
} // template <typename T> lambert_w_halley_iterate(T w_est, T z)

// Two Halley function versions that either
// single step (if std::false_type) or iterate (if std::true_type).
// Selected at compile-time using parameter 3.
template <typename T>
inline T lambert_w_maybe_halley_iterate(T z, T w, std::false_type const&)
{
   return lambert_w_halley_step(z, w); // Single step.
}

template <typename T>
inline T lambert_w_maybe_halley_iterate(T z, T w, std::true_type const&)
{
   return lambert_w_halley_iterate(z, w); // Iterate steps.
}

//! maybe_reduce_to_double function,
//! Two versions that have a compile-time option to
//! reduce argument z to double precision (if true_type).
//! Version is selected at compile-time using parameter 2.

template <typename T>
inline double maybe_reduce_to_double(const T& z, const std::true_type&)
{
  return static_cast<double>(z); // Reduce to double precision.
}

template <typename T>
inline T maybe_reduce_to_double(const T& z, const std::false_type&)
{ // Don't reduce to double.
  return z;
}

template <typename T>
inline double must_reduce_to_double(const T& z, const std::true_type&)
{
   return static_cast<double>(z); // Reduce to double precision.
}

template <typename T>
inline double must_reduce_to_double(const T& z, const std::false_type&)
{ // try a lexical_cast and hope for the best:
   return boost::lexical_cast<double>(z);
}

//! \brief Schroeder method, fifth-order update formula,
//! \details See T. Fukushima page 80-81, and
//! A. Householder, The Numerical Treatment of a Single Nonlinear Equation,
//! McGraw-Hill, New York, 1970, section 4.4.
//! Fukushima algorithm switches to @c schroeder_update after pre-computed bisections,
//! chosen to ensure that the result will be achieve the +/- 10 epsilon target.
//! \param w Lambert w estimate from bisection or series.
//! \param y bracketing value from bisection.
//! \returns Refined estimate of Lambert w.

// Schroeder refinement, called unless NOT required by precision policy.
template<typename T>
inline T schroeder_update(const T w, const T y)
{
  // Compute derivatives using 5th order Schroeder refinement.
  // Since this is the final step, it will always use the highest precision type T.
  // Example of Call:
  //   result = schroeder_update(w, y);
  //where
  // w is estimate of Lambert W (from bisection or series).
  // y is z * e^-w.

  BOOST_MATH_STD_USING // Aid argument dependent lookup of abs.
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SCHROEDER
    std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
  using boost::math::float_distance;
  T fd = float_distance<T>(w, y);
  std::cout << "Schroder ";
  if (abs(fd) < 214748000.)
  {
    std::cout << " Distance = "<< static_cast<int>(fd);
  }
  else
  {
    std::cout << "Difference w - y = " << (w - y) << ".";
  }
  std::cout << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SCHROEDER
  //  Fukushima equation 18, page 6.
  const T f0 = w - y; // f0 = w - y.
  const T f1 = 1 + y; // f1 = df/dW
  const T f00 = f0 * f0;
  const T f11 = f1 * f1;
  const T f0y = f0 * y;
  const T result =
    w - 4 * f0 * (6 * f1 * (f11 + f0y)  +  f00 * y) /
    (f11 * (24 * f11 + 36 * f0y) +
      f00 * (6 * y * y  +  8 * f1 * y  +  f0y)); // Fukushima Page 81, equation 21 from equation 20.

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SCHROEDER
  std::cout << "Schroeder refined " << w << "  " << y << ", difference  " << w-y  << ", change " << w - result << ", to result " << result << std::endl;
  std::cout.precision(saved_precision); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SCHROEDER

  return result;
} // template<typename T = double> T schroeder_update(const T w, const T y)

  //! \brief Series expansion used near the singularity/branch point z = -exp(-1) = -3.6787944.
  //! Wolfram InverseSeries[Series[sqrt[2(p Exp[1 + p] + 1)], {p,-1, 20}]]
  //! Wolfram command used to obtain 40 series terms at 50 decimal digit precision was
  //! N[InverseSeries[Series[Sqrt[2(p Exp[1 + p] + 1)], { p,-1,40 }]], 50]
  //! -1+p-p^2/3+(11 p^3)/72-(43 p^4)/540+(769 p^5)/17280-(221 p^6)/8505+(680863 p^7)/43545600 ...
  //! Decimal values of specifications for built-in floating-point types below
  //! are at least 21 digits precision == max_digits10 for long double.
  //! Longer decimal digits strings are rationals evaluated using Wolfram.

template<typename T>
T lambert_w_singularity_series(const T p)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SINGULARITY_SERIES
  std::size_t saved_precision = std::cout.precision(3);
  std::cout << "Singularity_series Lambert_w p argument = " << p << std::endl;
  std::cout
    //<< "Argument Type = " << typeid(T).name()
    //<< ", max_digits10 = " << std::numeric_limits<T>::max_digits10
    //<< ", epsilon = " << std::numeric_limits<T>::epsilon()
    << std::endl;
  std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SINGULARITY_SERIES

  static const T q[] =
  {
    -static_cast<T>(1), // j0
    +T(1), // j1
    -T(1) / 3, // 1/3  j2
    +T(11) / 72, // 0.152777777777777778, // 11/72 j3
    -T(43) / 540, // 0.0796296296296296296, // 43/540 j4
    +T(769) / 17280, // 0.0445023148148148148,  j5
    -T(221) / 8505, // 0.0259847148736037625,  j6
    //+T(0.0156356325323339212L), // j7
    //+T(0.015635632532333921222810111699000587889476778365667L), // j7 from Wolfram N[680863/43545600, 50]
    +T(680863uLL) / 43545600uLL, // +0.0156356325323339212, j7
    //-T(0.00961689202429943171L), // j8
    -T(1963uLL) / 204120uLL, // 0.00961689202429943171, j8
    //-T(0.0096168920242994317068391142465216539290613364687439L), // j8 from Wolfram N[1963/204120, 50]
    +T(226287557uLL) / 37623398400uLL, // 0.00601454325295611786, j9
    -T(5776369uLL) / 1515591000uLL, // 0.00381129803489199923, j10
    //+T(0.00244087799114398267L), j11 0.0024408779911439826658968585286437530215699919795550
    +T(169709463197uLL) / 69528040243200uLL, // j11
    // -T(0.00157693034468678425L), // j12  -0.0015769303446867842539234095399314115973161850314723
    -T(1118511313uLL) / 709296588000uLL, // j12
    +T(667874164916771uLL) / 650782456676352000uLL, // j13
    //+T(0.00102626332050760715L), // j13 0.0010262633205076071544375481533906861056468041465973
    -T(500525573uLL) / 744761417400uLL, // j14
    // -T(0.000672061631156136204L), j14
    //+T(1003663334225097487uLL) / 234281684403486720000uLL, // j15 0.00044247306181462090993020760858473726479232802068800 error C2177: constant too big
    //+T(0.000442473061814620910L, // j15
    BOOST_MATH_BIG_CONSTANT(T, 64, +0.000442473061814620910), // j15
    // -T(0.000292677224729627445L), // j16
    BOOST_MATH_BIG_CONSTANT(T, 64, -0.000292677224729627445), // j16
    //+T(0.000194387276054539318L), // j17
    BOOST_MATH_BIG_CONSTANT(T, 64, 0.000194387276054539318), // j17
    //-T(0.000129574266852748819L), // j18
    BOOST_MATH_BIG_CONSTANT(T, 64, -0.000129574266852748819), // j18
    //+T(0.0000866503580520812717L), // j19 N[+1150497127780071399782389/13277465363600276402995200000, 50] 0.000086650358052081271660451590462390293190597827783288
    BOOST_MATH_BIG_CONSTANT(T, 64, +0.0000866503580520812717), // j19
    //-T(0.0000581136075044138168L) // j20  N[2853534237182741069/49102686267859224000000, 50] 0.000058113607504413816772205464778828177256611844221913
    // -T(2853534237182741069uLL) / 49102686267859224000000uLL  // j20 // error C2177: constant too big,
    // so must use BOOST_MATH_BIG_CONSTANT(T, ) format in hope of using suffix Q for quad or decimal digits string for others.
    //-T(0.000058113607504413816772205464778828177256611844221913L), // j20  N[2853534237182741069/49102686267859224000000, 50] 0.000058113607504413816772205464778828177256611844221913
    BOOST_MATH_BIG_CONSTANT(T, 113, -0.000058113607504413816772205464778828177256611844221913) // j20  - last used by Fukushima
    // More terms don't seem to give any improvement (worse in fact) and are not use for many z values.
    //BOOST_MATH_BIG_CONSTANT(T, +0.000039076684867439051635395583044527492132109160553593), // j21
    //BOOST_MATH_BIG_CONSTANT(T, -0.000026338064747231098738584082718649443078703982217219), // j22
    //BOOST_MATH_BIG_CONSTANT(T, +0.000017790345805079585400736282075184540383274460464169), // j23
    //BOOST_MATH_BIG_CONSTANT(T, -0.000012040352739559976942274116578992585158113153190354), // j24
    //BOOST_MATH_BIG_CONSTANT(T, +8.1635319824966121713827512573558687050675701559448E-6), // j25
    //BOOST_MATH_BIG_CONSTANT(T, -5.5442032085673591366657251660804575198155559225316E-6) // j26
    // -T(5.5442032085673591366657251660804575198155559225316E-6L) // j26
    // 21 to 26 Added for long double.
  }; // static const T q[]

     /*
     // Temporary copy of original double values for comparison; these are reproduced well.
     static const T q[] =
     {
     -1L,  // j0
     +1L,  // j1
     -0.333333333333333333L, // 1/3 j2
     +0.152777777777777778L, // 11/72 j3
     -0.0796296296296296296L, // 43/540
     +0.0445023148148148148L,
     -0.0259847148736037625L,
     +0.0156356325323339212L,
     -0.00961689202429943171L,
     +0.00601454325295611786L,
     -0.00381129803489199923L,
     +0.00244087799114398267L,
     -0.00157693034468678425L,
     +0.00102626332050760715L,
     -0.000672061631156136204L,
     +0.000442473061814620910L,
     -0.000292677224729627445L,
     +0.000194387276054539318L,
     -0.000129574266852748819L,
     +0.0000866503580520812717L,
     -0.0000581136075044138168L // j20
     };
     */

     // Decide how many series terms to use, increasing as z approaches the singularity,
     // balancing run-time versus computational noise from round-off.
     // In practice, we truncate the series expansion at a certain order.
     // If the order is too large, not only does the amount of computation increase,
     // but also the round-off errors accumulate.
     // See Fukushima equation 35, page 85 for logic of choice of number of series terms.

  BOOST_MATH_STD_USING // Aid argument dependent lookup (ADL) of abs.

    const T absp = abs(p);

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_TERMS
  {
    int terms = 20; // Default to using all terms.
    if (absp < 0.01159)
    { // Very near singularity.
      terms = 6;
    }
    else if (absp < 0.0766)
    { // Near singularity.
      terms = 10;
    }
    std::streamsize saved_precision = std::cout.precision(3);
    std::cout << "abs(p) = " << absp << ", terms = " << terms << std::endl;
    std::cout.precision(saved_precision);
  }
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_TERMS

  if (absp < 0.01159)
  { // Only 6 near-singularity series terms are useful.
    return
      -1 +
      p * (1 +
        p * (q[2] +
          p * (q[3] +
            p * (q[4] +
              p * (q[5] +
                p * q[6]
                )))));
  }
  else if (absp < 0.0766) // Use 10 near-singularity series terms.
  { // Use 10 near-singularity series terms.
    return
      -1 +
      p * (1 +
        p * (q[2] +
          p * (q[3] +
            p * (q[4] +
              p * (q[5] +
                p * (q[6] +
                  p * (q[7] +
                    p * (q[8] +
                      p * (q[9] +
                        p * q[10]
                        )))))))));
  }
  else
  { // Use all 20 near-singularity series terms.
    return
      -1 +
      p * (1 +
        p * (q[2] +
          p * (q[3] +
            p * (q[4] +
              p * (q[5] +
                p * (q[6] +
                  p * (q[7] +
                    p * (q[8] +
                      p * (q[9] +
                        p * (q[10] +
                          p * (q[11] +
                            p * (q[12] +
                              p * (q[13] +
                                p * (q[14] +
                                  p * (q[15] +
                                    p * (q[16] +
                                      p * (q[17] +
                                        p * (q[18] +
                                          p * (q[19] +
                                            p * q[20] // Last Fukushima term.
                                            )))))))))))))))))));
    //                                                + // more terms for more precise T: long double ...
    //// but makes almost no difference, so don't use more terms?
    //                                          p*q[21] +
    //                                            p*q[22] +
    //                                              p*q[23] +
    //                                                p*q[24] +
    //                                                 p*q[25]
    //                                         )))))))))))))))))));
  }
} // template<typename T = double> T lambert_w_singularity_series(const T p)


 /////////////////////////////////////////////////////////////////////////////////////////////

  //! \brief Series expansion used near zero (abs(z) < 0.05).
  //! \details
  //! Coefficients of the inverted series expansion of the Lambert W function around z = 0.
  //! Tosio Fukushima always uses all 17 terms of a Taylor series computed using Wolfram with
  //!   InverseSeries[Series[z Exp[z],{z,0,17}]]
  //! Tosio Fukushima / Journal of Computational and Applied Mathematics 244 (2013) page 86.

  //! Decimal values of specifications for built-in floating-point types below
  //! are 21 digits precision == max_digits10 for long double.
  //! Care! Some coefficients might overflow some fixed_point types.

  //! This version is intended to allow use by user-defined types
  //! like Boost.Multiprecision quad and cpp_dec_float types.
  //! The three specializations below for built-in float, double
  //! (and perhaps long double) will be chosen in preference for these types.

  //! This version uses rationals computed by Wolfram as far as possible,
  //! limited by maximum size of uLL integers.
  //! For higher term, uses decimal digit strings computed by Wolfram up to the maximum possible using uLL rationals,
  //! and then higher coefficients are computed as necessary using function lambert_w0_small_z_series_term
  //! until the precision required by the policy is achieved.
  //! InverseSeries[Series[z Exp[z],{z,0,34}]] also computed.

  // Series evaluation for LambertW(z) as z -> 0.
  // See http://functions.wolfram.com/ElementaryFunctions/ProductLog/06/01/01/0003/
  //  http://functions.wolfram.com/ElementaryFunctions/ProductLog/06/01/01/0003/MainEq1.L.gif

  //! \brief  lambert_w0_small_z uses a tag_type to select a variant depending on the size of the type.
  //! The Lambert W is computed by lambert_w0_small_z for small z.
  //! The cutoff for z smallness determined by Tosio Fukushima by trial and error is (abs(z) < 0.05),
  //! but the optimum might be a function of the size of the type of z.

  //! \details
  //! The tag_type selection is based on the value @c std::numeric_limits<T>::max_digits10.
  //! This allows distinguishing between long double types that commonly vary between 64 and 80-bits,
  //! and also compilers that have a float type using 64 bits and/or long double using 128-bits.
  //! It assumes that max_digits10 is defined correctly or this might fail to make the correct selection.
  //! causing very small differences in computing lambert_w that would be very difficult to detect and diagnose.
  //! Cannot switch on @c std::numeric_limits<>::max() because comparison values may overflow the compiler limit.
  //! Cannot switch on @c std::numeric_limits<long double>::max_exponent10()
  //! because both 80 and 128 bit floating-point implementations use 11 bits for the exponent.
  //! So must rely on @c std::numeric_limits<long double>::max_digits10.

  //! Specialization of float zero series expansion used for small z (abs(z) < 0.05).
  //! Specializations of lambert_w0_small_z for built-in types.
  //! These specializations should be chosen in preference to T version.
  //! For example: lambert_w0_small_z(0.001F) should use the float version.
  //! (Parameter Policy is not used by built-in types when all terms are used during an inline computation,
  //! but for the tag_type selection to work, they all must include Policy in their signature.

  // Forward declaration of variants of lambert_w0_small_z.
template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 0> const&);   //  for float (32-bit) type.

template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 1> const&);   //  for double (64-bit) type.

template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 2> const&);   //  for long double (double extended 80-bit) type.

template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 3> const&);   //  for long double (128-bit) type.

template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 4> const&);   //  for float128 quadmath Q type.

template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy&, std::integral_constant<int, 5> const&);   //  Generic multiprecision T.
                                                                        // Set tag_type depending on max_digits10.
template <typename T, typename Policy>
T lambert_w0_small_z(T x, const Policy& pol)
{ //std::numeric_limits<T>::max_digits10 == 36 ? 3 : // 128-bit long double.
  using tag_type = std::integral_constant<int,
     std::numeric_limits<T>::is_specialized == 0 ? 5 :
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
    std::numeric_limits<T>::max_digits10 <=  9 ? 0 : // for float 32-bit.
    std::numeric_limits<T>::max_digits10 <= 17 ? 1 : // for double 64-bit.
    std::numeric_limits<T>::max_digits10 <= 22 ? 2 : // for 80-bit double extended.
    std::numeric_limits<T>::max_digits10 <  37 ? 4  // for both 128-bit long double (3) and 128-bit quad suffix Q type (4).
#else
     std::numeric_limits<T>::radix != 2 ? 5 :
     std::numeric_limits<T>::digits <= 24 ? 0 : // for float 32-bit.
     std::numeric_limits<T>::digits <= 53 ? 1 : // for double 64-bit.
     std::numeric_limits<T>::digits <= 64 ? 2 : // for 80-bit double extended.
     std::numeric_limits<T>::digits <= 113 ? 4  // for both 128-bit long double (3) and 128-bit quad suffix Q type (4).
#endif
      :  5>;                                           // All Generic multiprecision types.
  // std::cout << "\ntag type = " << tag_type << std::endl; // error C2275: 'tag_type': illegal use of this type as an expression.
  return lambert_w0_small_z(x, pol, tag_type());
} // template <typename T> T lambert_w0_small_z(T x)

  //! Specialization of float (32-bit) series expansion used for small z (abs(z) < 0.05).
  // Only 9 Coefficients are computed to 21 decimal digits precision, ample for 32-bit float used by most platforms.
  // Taylor series coefficients used are computed by Wolfram to 50 decimal digits using instruction
  // N[InverseSeries[Series[z Exp[z],{z,0,34}]],50],
  // as proposed by Tosio Fukushima and implemented by Darko Veberic.

template <typename T, typename Policy>
T lambert_w0_small_z(T z, const Policy&, std::integral_constant<int, 0> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize prec = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "\ntag_type 0 float lambert_w0_small_z called with z = " << z << " using " << 9 << " terms of precision "
    << std::numeric_limits<float>::max_digits10 << " decimal digits. " << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  T result =
    z * (1 - // j1 z^1 term = 1
      z * (1 -  // j2 z^2 term = -1
        z * (static_cast<float>(3uLL) / 2uLL - // 3/2 // j3 z^3 term = 1.5.
          z * (2.6666666666666666667F -  // 8/3 // j4
            z * (5.2083333333333333333F - // -125/24 // j5
              z * (10.8F - // j6
                z * (23.343055555555555556F - // j7
                  z * (52.012698412698412698F - // j8
                    z * 118.62522321428571429F)))))))); // j9

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "return w = " << result << std::endl;
  std::cout.precision(prec); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES

  return result;
} // template <typename T>   T lambert_w0_small_z(T x, std::integral_constant<int, 0> const&)

  //! Specialization of double (64-bit double) series expansion used for small z (abs(z) < 0.05).
  // 17 Coefficients are computed to 21 decimal digits precision suitable for 64-bit double used by most platforms.
  // Taylor series coefficients used are computed by Wolfram to 50 decimal digits using instruction
  // N[InverseSeries[Series[z Exp[z],{z,0,34}]],50], as proposed by Tosio Fukushima and implemented by Veberic.

template <typename T, typename Policy>
T lambert_w0_small_z(const T z, const Policy&, std::integral_constant<int, 1> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize prec = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "\ntag_type 1 double lambert_w0_small_z called with z = " << z << " using " << 17 << " terms of precision, "
    << std::numeric_limits<double>::max_digits10 << " decimal digits. " << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  T result =
    z * (1. - // j1 z^1
      z * (1. -  // j2 z^2
        z * (1.5 - // 3/2 // j3 z^3
          z * (2.6666666666666666667 -  // 8/3 // j4
            z * (5.2083333333333333333 - // -125/24 // j5
              z * (10.8 - // j6
                z * (23.343055555555555556 - // j7
                  z * (52.012698412698412698 - // j8
                    z * (118.62522321428571429 - // j9
                      z * (275.57319223985890653 - // j10
                        z * (649.78717234347442681 - // j11
                          z * (1551.1605194805194805 - // j12
                            z * (3741.4497029592385495 - // j13
                              z * (9104.5002411580189358 - // j14
                                z * (22324.308512706601434 - // j15
                                  z * (55103.621972903835338 - // j16
                                    z * 136808.86090394293563)))))))))))))))); // j17 z^17

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "return w = " << result << std::endl;
  std::cout.precision(prec); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES

  return result;
} // T lambert_w0_small_z(const T z, std::integral_constant<int, 1> const&)

  //! Specialization of long double (80-bit double extended) series expansion used for small z (abs(z) < 0.05).
  // 21 Coefficients are computed to 21 decimal digits precision suitable for 80-bit long double used by some
  // platforms including GCC and Clang when generating for Intel X86 floating-point processors with 80-bit operations enabled (the default).
  // (This is NOT used by Microsoft Visual Studio where double and long always both use only 64-bit type.
  // Nor used for 128-bit float128.)
template <typename T, typename Policy>
T lambert_w0_small_z(const T z, const Policy&, std::integral_constant<int, 2> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize precision = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "\ntag_type 2 long double (80-bit double extended) lambert_w0_small_z called with z = " << z << " using " << 21 << " terms of precision, "
    << std::numeric_limits<long double>::max_digits10 << " decimal digits. " << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
//  T  result =
//    z * (1.L - // j1 z^1
//      z * (1.L -  // j2 z^2
//        z * (1.5L - // 3/2 // j3
//          z * (2.6666666666666666667L -  // 8/3 // j4
//            z * (5.2083333333333333333L - // -125/24 // j5
//              z * (10.800000000000000000L - // j6
//                z * (23.343055555555555556L - // j7
//                  z * (52.012698412698412698L - // j8
//                    z * (118.62522321428571429L - // j9
//                      z * (275.57319223985890653L - // j10
//                        z * (649.78717234347442681L - // j11
//                          z * (1551.1605194805194805L - // j12
//                            z * (3741.4497029592385495L - // j13
//                              z * (9104.5002411580189358L - // j14
//                                z * (22324.308512706601434L - // j15
//                                  z * (55103.621972903835338L - // j16
//                                    z * (136808.86090394293563L - // j17 z^17  last term used by Fukushima double.
//                                      z * (341422.050665838363317L - // z^18
//                                        z * (855992.9659966075514633L - // z^19
//                                          z * (2.154990206091088289321e6L - // z^20
//                                            z * 5.4455529223144624316423e6L   // z^21
//                                              ))))))))))))))))))));
//

  T result =
z * (1.L - // z j1
z * (1.L - // z^2
z * (1.500000000000000000000000000000000L - // z^3
z * (2.666666666666666666666666666666666L - // z ^ 4
z * (5.208333333333333333333333333333333L - // z ^ 5
z * (10.80000000000000000000000000000000L - // z ^ 6
z * (23.34305555555555555555555555555555L - //  z ^ 7
z * (52.01269841269841269841269841269841L - // z ^ 8
z * (118.6252232142857142857142857142857L - // z ^ 9
z * (275.5731922398589065255731922398589L - // z ^ 10
z * (649.7871723434744268077601410934744L - // z ^ 11
z * (1551.160519480519480519480519480519L - // z ^ 12
z * (3741.449702959238549516327294105071L - //z ^ 13
z * (9104.500241158018935796713574491352L - //  z ^ 14
z * (22324.308512706601434280005708577137L - //  z ^ 15
z * (55103.621972903835337697771560205422L - //  z ^ 16
z * (136808.86090394293563342215789305736L - // z ^ 17
z * (341422.05066583836331735491399356945L - //  z^18
z * (855992.9659966075514633630250633224L - // z^19
z * (2.154990206091088289321708745358647e6L // z^20 distance -5 without term 20
))))))))))))))))))));

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "return w = " << result << std::endl;
  std::cout.precision(precision); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  return result;
}  // long double lambert_w0_small_z(const T z, std::integral_constant<int, 1> const&)

//! Specialization of 128-bit long double series expansion used for small z (abs(z) < 0.05).
// 34 Taylor series coefficients used are computed by Wolfram to 50 decimal digits using instruction
// N[InverseSeries[Series[z Exp[z],{z,0,34}]],50],
// and are suffixed by L as they are assumed of type long double.
// (This is NOT used for 128-bit quad boost::multiprecision::float128 type which required a suffix Q
// nor multiprecision type cpp_bin_float_quad that can only be initialised at full precision of the type
// constructed with a decimal digit string like "2.6666666666666666666666666666666666666666666666667".)

template <typename T, typename Policy>
T lambert_w0_small_z(const T z, const Policy&, std::integral_constant<int, 3> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize precision = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "\ntag_type 3 long double (128-bit) lambert_w0_small_z called with z = " << z << " using " << 17 << " terms of precision,  "
    << std::numeric_limits<double>::max_digits10 << " decimal digits. " << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  T  result =
    z * (1.L - // j1
      z * (1.L -  // j2
        z * (1.5L - // 3/2 // j3
          z * (2.6666666666666666666666666666666666L -  // 8/3 // j4
            z * (5.2052083333333333333333333333333333L - // -125/24 // j5
              z * (10.800000000000000000000000000000000L - // j6
                z * (23.343055555555555555555555555555555L - // j7
                  z * (52.0126984126984126984126984126984126L - // j8
                    z * (118.625223214285714285714285714285714L - // j9
                      z * (275.57319223985890652557319223985890L - // * z ^ 10 - // j10
                        z * (649.78717234347442680776014109347442680776014109347L - // j11
                          z * (1551.1605194805194805194805194805194805194805194805L - // j12
                            z * (3741.4497029592385495163272941050718828496606274384L - // j13
                              z * (9104.5002411580189357967135744913522691300469078247L - // j14
                                z * (22324.308512706601434280005708577137148565719994291L - // j15
                                  z * (55103.621972903835337697771560205422639285073147507L - // j16
                                    z * 136808.86090394293563342215789305736395683485630576L    // j17
                                      ))))))))))))))));

#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "return w = " << result << std::endl;
  std::cout.precision(precision); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  return result;
}  // T lambert_w0_small_z(const T z, std::integral_constant<int, 3> const&)

//! Specialization of 128-bit quad series expansion used for small z (abs(z) < 0.05).
// 34 Taylor series coefficients used were computed by Wolfram to 50 decimal digits using instruction
//   N[InverseSeries[Series[z Exp[z],{z,0,34}]],50],
// and are suffixed by Q as they are assumed of type quad.
// This could be used for 128-bit quad (which requires a suffix Q for full precision).
// But experiments with GCC 7.2.0 show that while this gives full 128-bit precision
// when the -f-ext-numeric-literals option is in force and the libquadmath library available,
// over the range -0.049 to +0.049, 
// it is slightly slower than getting a double approximation followed by a single Halley step.

#ifdef BOOST_HAS_FLOAT128
template <typename T, typename Policy>
T lambert_w0_small_z(const T z, const Policy&, std::integral_constant<int, 4> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize precision = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "\ntag_type 4 128-bit quad float128 lambert_w0_small_z called with z = " << z << " using " << 34 << " terms of precision, "
    << std::numeric_limits<float128>::max_digits10 << " max decimal digits." << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  T  result =
    z * (1.Q - // z j1
      z * (1.Q - // z^2
        z * (1.500000000000000000000000000000000Q - // z^3
          z * (2.666666666666666666666666666666666Q - // z ^ 4
            z * (5.208333333333333333333333333333333Q - // z ^ 5
              z * (10.80000000000000000000000000000000Q - // z ^ 6
                z * (23.34305555555555555555555555555555Q - //  z ^ 7
                  z * (52.01269841269841269841269841269841Q - // z ^ 8
                    z * (118.6252232142857142857142857142857Q - // z ^ 9
                      z * (275.5731922398589065255731922398589Q - // z ^ 10
                        z * (649.7871723434744268077601410934744Q - // z ^ 11
                          z * (1551.160519480519480519480519480519Q - // z ^ 12
                            z * (3741.449702959238549516327294105071Q - //z ^ 13
                              z * (9104.500241158018935796713574491352Q - //  z ^ 14
                                z * (22324.308512706601434280005708577137Q - //  z ^ 15
                                  z * (55103.621972903835337697771560205422Q - //  z ^ 16
                                    z * (136808.86090394293563342215789305736Q - // z ^ 17
                                      z * (341422.05066583836331735491399356945Q - //  z^18
                                        z * (855992.9659966075514633630250633224Q - // z^19
                                          z * (2.154990206091088289321708745358647e6Q - //  20
                                            z * (5.445552922314462431642316420035073e6Q - // 21
                                              z * (1.380733000216662949061923813184508e7Q - // 22
                                                z * (3.511704498513923292853869855945334e7Q - // 23
                                                  z * (8.956800256102797693072819557780090e7Q - // 24
                                                    z * (2.290416846187949813964782641734774e8Q - // 25
                                                      z * (5.871035041171798492020292225245235e8Q - // 26
                                                        z * (1.508256053857792919641317138812957e9Q - // 27
                                                          z * (3.882630161293188940385873468413841e9Q - // 28
                                                            z * (1.001394313665482968013913601565723e10Q - // 29
                                                              z * (2.587356736265760638992878359024929e10Q - // 30
                                                                z * (6.696209709358073856946120522333454e10Q - // 31
                                                                  z * (1.735711659599198077777078238043644e11Q - // 32
                                                                    z * (4.505680465642353886756098108484670e11Q - // 33
                                                                      z * (1.171223178256487391904047636564823e12Q  //z^34
                                                                        ))))))))))))))))))))))))))))))))));


 #ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "return w = " << result << std::endl;
  std::cout.precision(precision); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES

  return result;
}  // T lambert_w0_small_z(const T z, std::integral_constant<int, 4> const&) float128

#else

template <typename T, typename Policy>
inline T lambert_w0_small_z(const T z, const Policy& pol, std::integral_constant<int, 4> const&)
{
   return lambert_w0_small_z(z, pol, std::integral_constant<int, 5>());
}

#endif // BOOST_HAS_FLOAT128

//! Series functor to compute series term using pow and factorial.
//! \details Functor is called after evaluating polynomial with the coefficients as rationals below.
template <typename T>
struct lambert_w0_small_z_series_term
{
  using result_type = T;
  //! \param _z Lambert W argument z.
  //! \param -term  -pow<18>(z) / 6402373705728000uLL
  //! \param _k number of terms == initially 18

  //  Note *after* evaluating N terms, its internal state has k = N and term = (-1)^N z^N.

  lambert_w0_small_z_series_term(T _z, T _term, int _k)
    : k(_k), z(_z), term(_term) { }

  T operator()()
  { // Called by sum_series until needs precision set by factor (policy::get_epsilon).
    using std::pow;
    ++k;
    term *= -z / k;
    //T t = pow(z, k) * pow(T(k), -1 + k) / factorial<T>(k); // (z^k * k(k-1)^k) / k!
    T result = term * pow(T(k), -1 + k); // term * k^(k-1)
                                         // std::cout << " k = " << k << ", term = " << term << ", result = " << result << std::endl;
    return result; //
  }
private:
  int k;
  T z;
  T term;
}; // template <typename T> struct lambert_w0_small_z_series_term

   //! Generic variant for T a User-defined types like Boost.Multiprecision.
template <typename T, typename Policy>
inline T lambert_w0_small_z(T z, const Policy& pol, std::integral_constant<int, 5> const&)
{
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::streamsize precision = std::cout.precision(std::numeric_limits<T>::max_digits10); // Save.
  std::cout << "Generic lambert_w0_small_z called with z = " << z << " using as many terms needed for precision." << std::endl;
  std::cout << "Argument z is of type " << typeid(T).name() << std::endl;
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES

  // First several terms of the series are tabulated and evaluated as a polynomial:
  // this will save us a bunch of expensive calls to pow.
  // Then our series functor is initialized "as if" it had already reached term 18,
  // enough evaluation of built-in 64-bit double and float (and 80-bit long double?) types.

  // Coefficients should be stored such that the coefficients for the x^i terms are in poly[i].
  static const T coeff[] =
  {
    0, // z^0  Care: zeroth term needed by tools::evaluate_polynomial, but not in the Wolfram equation, so indexes are one different!
    1, // z^1 term.
    -1, // z^2 term
    static_cast<T>(3uLL) / 2uLL, // z^3 term.
    -static_cast<T>(8uLL) / 3uLL, // z^4
    static_cast<T>(125uLL) / 24uLL, // z^5
    -static_cast<T>(54uLL) / 5uLL, // z^6
    static_cast<T>(16807uLL) / 720uLL, // z^7
    -static_cast<T>(16384uLL) / 315uLL, // z^8
    static_cast<T>(531441uLL) / 4480uLL, // z^9
    -static_cast<T>(156250uLL) / 567uLL, // z^10
    static_cast<T>(2357947691uLL) / 3628800uLL, // z^11
    -static_cast<T>(2985984uLL) / 1925uLL, // z^12
    static_cast<T>(1792160394037uLL) / 479001600uLL, // z^13
    -static_cast<T>(7909306972uLL) / 868725uLL, // z^14
    static_cast<T>(320361328125uLL) / 14350336uLL, // z^15
    -static_cast<T>(35184372088832uLL) / 638512875uLL, // z^16
    static_cast<T>(2862423051509815793uLL) / 20922789888000uLL, // z^17 term
    -static_cast<T>(5083731656658uLL) / 14889875uLL,
    // z^18 term. = 136808.86090394293563342215789305735851647769682393

    // z^18 is biggest that can be computed as rational using the largest possible uLL integers,
    // so higher terms cannot be potentially compiler-computed as uLL rationals.
    // Wolfram (5083731656658 z ^ 18) / 14889875 or
    // -341422.05066583836331735491399356945575432970390954 z^18

    // See note below calling the functor to compute another term,
    // sufficient for 80-bit long double precision.
    // Wolfram -341422.05066583836331735491399356945575432970390954 z^19 term.
    // (5480386857784802185939 z^19)/6402373705728000
    // But now this variant is not used to compute long double
    // as specializations are provided above.
  }; // static const T coeff[]

     /*
     Table of 19 computed coefficients:

     #0 0
     #1 1
     #2 -1
     #3 1.5
     #4 -2.6666666666666666666666666666666665382713370408509
     #5 5.2083333333333333333333333333333330765426740817019
     #6 -10.800000000000000000000000000000000616297582203915
     #7 23.343055555555555555555555555555555076212991619177
     #8 -52.012698412698412698412698412698412659282693193402
     #9 118.62522321428571428571428571428571146835390992496
     #10 -275.57319223985890652557319223985891400375196748314
     #11 649.7871723434744268077601410934743969785223845882
     #12 -1551.1605194805194805194805194805194947599566007429
     #13 3741.4497029592385495163272941050719510009019331763
     #14 -9104.5002411580189357967135744913524243896052869184
     #15 22324.308512706601434280005708577137322392070452582
     #16 -55103.621972903835337697771560205423203318720697224
     #17 136808.86090394293563342215789305735851647769682393
         136808.86090394293563342215789305735851647769682393   == Exactly same as Wolfram computed value.
     #18 -341422.05066583836331735491399356947486381600607416
          341422.05066583836331735491399356945575432970390954  z^19  Wolfram value differs at 36 decimal digit, as expected.
     */

  using boost::math::policies::get_epsilon; // for type T.
  using boost::math::tools::sum_series;
  using boost::math::tools::evaluate_polynomial;
  // http://www.boost.org/doc/libs/release/libs/math/doc/html/math_toolkit/roots/rational.html

  // std::streamsize prec = std::cout.precision(std::numeric_limits <T>::max_digits10);

  T result = evaluate_polynomial(coeff, z);
  //  template <std::size_t N, typename T, typename V>
  //  V evaluate_polynomial(const T(&poly)[N], const V& val);
  // Size of coeff found from N
  //std::cout << "evaluate_polynomial(coeff, z); == " << result << std::endl;
  //std::cout << "result = " << result << std::endl;
  // It's an artefact of the way I wrote the functor: *after* evaluating N
  // terms, its internal state has k = N and term = (-1)^N z^N.  So after
  // evaluating 18 terms, we initialize the functor to the term we've just
  // evaluated, and then when it's called, it increments itself to the next term.
  // So 18!is 6402373705728000, which is where that comes from.

  // The 19th coefficient of the polynomial is actually, 19 ^ 18 / 19!=
  // 104127350297911241532841 / 121645100408832000 which after removing GCDs
  // reduces down to Wolfram rational 5480386857784802185939 / 6402373705728000.
  // Wolfram z^19 term +(5480386857784802185939 z^19) /6402373705728000
  // +855992.96599660755146336302506332246623424823099755 z^19

  //! Evaluate Functor.
  lambert_w0_small_z_series_term<T> s(z, -pow<18>(z) / 6402373705728000uLL, 18);

  // Temporary to list the coefficients.
  //std::cout << " Table of coefficients" << std::endl;
  //std::streamsize saved_precision = std::cout.precision(50);
  //for (size_t i = 0; i != 19; i++)
  //{
  //  std::cout << "#" << i << " " << coeff[i] << std::endl;
  //}
  //std::cout.precision(saved_precision);

  std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>(); // Max iterations from policy.
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES
  std::cout << "max iter from policy = " << max_iter << std::endl;
  // //   max iter from policy = 1000000 is default.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES

  result = sum_series(s, get_epsilon<T, Policy>(), max_iter, result);
  // result == evaluate_polynomial.
  //sum_series(Functor& func, int bits, std::uintmax_t& max_terms, const U& init_value)
  // std::cout << "sum_series(s, get_epsilon<T, Policy>(), max_iter, result); = " << result << std::endl;

  //T epsilon = get_epsilon<T, Policy>();
  //std::cout << "epsilon from policy = " << epsilon << std::endl;
  // epsilon from policy = 1.93e-34 for T == quad
  //  5.35e-51 for t = cpp_bin_float_50

  // std::cout << " get eps = " << get_epsilon<T, Policy>() << std::endl; // quad eps = 1.93e-34, bin_float_50 eps = 5.35e-51
  policies::check_series_iterations<T>("boost::math::lambert_w0_small_z<%1%>(%1%)", max_iter, pol);
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES_ITERATIONS
  std::cout << "z = " << z << " needed  " << max_iter << " iterations." << std::endl;
  std::cout.precision(prec); // Restore.
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W_SMALL_Z_SERIES_ITERATIONS
  return result;
} // template <typename T, typename Policy> inline T lambert_w0_small_z_series(T z, const Policy& pol)

// Approximate lambert_w0 (used for z values that are outside range of lookup table or rational functions)
// Corless equation 4.19, page 349, and Chapeau-Blondeau equation 20, page 2162.
template <typename T>
inline T lambert_w0_approx(T z)
{
  BOOST_MATH_STD_USING
  T lz = log(z);
  T llz = log(lz);
  T w = lz - llz + (llz / lz); // Corless equation 4.19, page 349, and Chapeau-Blondeau equation 20, page 2162.
  return w;
  // std::cout << "w max " << max_w << std::endl; // double 703.227
}

  //////////////////////////////////////////////////////////////////////////////////////////

//! \brief Lambert_w0 implementations for float, double and higher precisions.
//! 3rd parameter used to select which version is used.

//! /details Rational polynomials are provided for several range of argument z.
//! For very small values of z, and for z very near the branch singularity at -e^-1 (~= -0.367879),
//! two other series functions are used.

//! float precision polynomials are used for 32-bit (usually float) precision (for speed)
//! double precision polynomials are used for 64-bit (usually double) precision.
//! For higher precisions, a 64-bit double approximation is computed first,
//! and then refined using Halley iterations.

template <typename T>
inline T do_get_near_singularity_param(T z)
{
   BOOST_MATH_STD_USING
   const T p2 = 2 * (boost::math::constants::e<T>() * z + 1);
   const T p = sqrt(p2);
   return p;
}
template <typename T, typename Policy>
inline T get_near_singularity_param(T z, const Policy)
{
   using value_type = typename policies::evaluation<T, Policy>::type;
   return static_cast<T>(do_get_near_singularity_param(static_cast<value_type>(z)));
}

// Forward declarations:

//template <typename T, typename Policy> T lambert_w0_small_z(T z, const Policy& pol);
//template <typename T, typename Policy>
//T lambert_w0_imp(T w, const Policy& pol, const std::integral_constant<int, 0>&); // 32 bit usually float.
//template <typename T, typename Policy>
//T lambert_w0_imp(T w, const Policy& pol, const std::integral_constant<int, 1>&); //  64 bit usually double.
//template <typename T, typename Policy>
//T lambert_w0_imp(T w, const Policy& pol, const std::integral_constant<int, 2>&); // 80-bit long double.

template <typename T>
T lambert_w_positive_rational_float(T z)
{
   BOOST_MATH_STD_USING
   if (z < 2)
   {
      if (z < 0.5)
      { // 0.05 < z < 0.5
        // Maximum Deviation Found:                     2.993e-08
        // Expected Error Term : 2.993e-08
        // Maximum Relative Change in Control Points : 7.555e-04 Y offset : -8.196592331e-01
         static const T Y = 8.196592331e-01f;
         static const T P[] = {
            1.803388345e-01f,
            -4.820256838e-01f,
            -1.068349741e+00f,
            -3.506624319e-02f,
         };
         static const T Q[] = {
            1.000000000e+00f,
            2.871703469e+00f,
            1.690949264e+00f,
         };
         return z * (Y + boost::math::tools::evaluate_polynomial(P, z) / boost::math::tools::evaluate_polynomial(Q, z));
      }
      else
      { // 0.5 < z < 2
        // Max error in interpolated form: 1.018e-08
         static const T Y = 5.503368378e-01f;
         static const T P[] = {
            4.493332766e-01f,
            2.543432707e-01f,
            -4.808788799e-01f,
            -1.244425316e-01f,
         };
         static const T Q[] = {
            1.000000000e+00f,
            2.780661241e+00f,
            1.830840318e+00f,
            2.407221031e-01f,
         };
         return z * (Y + boost::math::tools::evaluate_rational(P, Q, z));
      }
   }
   else if (z < 6)
   {
      // 2 < z < 6
      // Max error in interpolated form: 2.944e-08
      static const T Y = 1.162393570e+00f;
      static const T P[] = {
         -1.144183394e+00f,
         -4.712732855e-01f,
         1.563162512e-01f,
         1.434010911e-02f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         1.192626340e+00f,
         2.295580708e-01f,
         5.477869455e-03f,
      };
      return Y + boost::math::tools::evaluate_rational(P, Q, z);
   }
   else if (z < 18)
   {
      // 6 < z < 18
      // Max error in interpolated form: 5.893e-08
      static const T Y = 1.809371948e+00f;
      static const T P[] = {
         -1.689291769e+00f,
         -3.337812742e-01f,
         3.151434873e-02f,
         1.134178734e-03f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         5.716915685e-01f,
         4.489521292e-02f,
         4.076716763e-04f,
      };
      return Y + boost::math::tools::evaluate_rational(P, Q, z);
   }
   else if (z < 9897.12905874)  // 2.8 < log(z) < 9.2
   {
      // Max error in interpolated form: 1.771e-08
      static const T Y = -1.402973175e+00f;
      static const T P[] = {
         1.966174312e+00f,
         2.350864728e-01f,
         -5.098074353e-02f,
         -1.054818339e-02f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         4.388208264e-01f,
         8.316639634e-02f,
         3.397187918e-03f,
         -1.321489743e-05f,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_polynomial(P, log_w) / boost::math::tools::evaluate_polynomial(Q, log_w);
   }
   else if (z < 7.896296e+13)  // 9.2 < log(z) <= 32
   {
      // Max error in interpolated form: 5.821e-08
      static const T Y = -2.735729218e+00f;
      static const T P[] = {
         3.424903470e+00f,
         7.525631787e-02f,
         -1.427309584e-02f,
         -1.435974178e-05f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         2.514005579e-01f,
         6.118994652e-03f,
         -1.357889535e-05f,
         7.312865624e-08f,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_polynomial(P, log_w) / boost::math::tools::evaluate_polynomial(Q, log_w);
   }
   else // 32 < log(z) < 100
   {
      // Max error in interpolated form: 1.491e-08
      static const T Y = -4.012863159e+00f;
      static const T P[] = {
         4.431629226e+00f,
         2.756690487e-01f,
         -2.992956930e-03f,
         -4.912259384e-05f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         2.015434591e-01f,
         4.949426142e-03f,
         1.609659944e-05f,
         -5.111523436e-09f,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_polynomial(P, log_w) / boost::math::tools::evaluate_polynomial(Q, log_w);
   }
}

template <typename T, typename Policy>
T lambert_w_negative_rational_float(T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   if (z > -0.27)
   {
      if (z < -0.051)
      {
         // -0.27 < z < -0.051
         // Max error in interpolated form: 5.080e-08
         static const T Y = 1.255809784e+00f;
         static const T P[] = {
            -2.558083412e-01f,
            -2.306524098e+00f,
            -5.630887033e+00f,
            -3.803974556e+00f,
         };
         static const T Q[] = {
            1.000000000e+00f,
            5.107680783e+00f,
            7.914062868e+00f,
            3.501498501e+00f,
         };
         return z * (Y + boost::math::tools::evaluate_rational(P, Q, z));
      }
      else
      {
         // Very small z so use a series function.
         return lambert_w0_small_z(z, pol);
      }
   }
   else if (z > -0.3578794411714423215955237701)
   { // Very close to branch singularity.
     // Max error in interpolated form: 5.269e-08
      static const T Y = 1.220928431e-01f;
      static const T P[] = {
         -1.221787446e-01f,
         -6.816155875e+00f,
         7.144582035e+01f,
         1.128444390e+03f,
      };
      static const T Q[] = {
         1.000000000e+00f,
         6.480326790e+01f,
         1.869145243e+02f,
         -1.361804274e+03f,
         1.117826726e+03f,
      };
      T d = z + 0.367879441171442321595523770161460867445811f;
      return -d / (Y + boost::math::tools::evaluate_polynomial(P, d) / boost::math::tools::evaluate_polynomial(Q, d));
   }
   else
   {
      // z is very close (within 0.01) of the singularity at e^-1.
      return lambert_w_singularity_series(get_near_singularity_param(z, pol));
   }
}

//! Lambert_w0 @b 'float' implementation, selected when T is 32-bit precision.
template <typename T, typename Policy>
inline T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 1>&)
{
  static const char* function = "boost::math::lambert_w0<%1%>"; // For error messages.
  BOOST_MATH_STD_USING // Aid ADL of std functions.

  if ((boost::math::isnan)(z))
  {
    return boost::math::policies::raise_domain_error<T>(function, "Expected a value > -e^-1 (-0.367879...) but got %1%.", z, pol);
  }
  if ((boost::math::isinf)(z))
  {
    return boost::math::policies::raise_overflow_error<T>(function, "Expected a finite value but got %1%.", z, pol);
  }

   if (z >= 0.05) // Fukushima switch point.
   // if (z >= 0.045) // 34 terms makes 128-bit 'exact' below 0.045.
   { // Normal ranges using several rational polynomials.
      return lambert_w_positive_rational_float(z);
   }
   else if (z <= -0.3678794411714423215955237701614608674458111310f)
   {
      if (z < -0.3678794411714423215955237701614608674458111310f)
         return boost::math::policies::raise_domain_error<T>(function, "Expected z >= -e^-1 (-0.367879...) but got %1%.", z, pol);
      return -1;
   }
   else // z < 0.05
   {
      return lambert_w_negative_rational_float(z, pol);
   }
} // T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 1>&) for 32-bit usually float.

template <typename T>
T lambert_w_positive_rational_double(T z)
{
   BOOST_MATH_STD_USING
   if (z < 2)
   {
      if (z < 0.5)
      {
         // Max error in interpolated form: 2.255e-17
         static const T offset = 8.19659233093261719e-01;
         static const T P[] = {
            1.80340766906685177e-01,
            3.28178241493119307e-01,
            -2.19153620687139706e+00,
            -7.24750929074563990e+00,
            -7.28395876262524204e+00,
            -2.57417169492512916e+00,
            -2.31606948888704503e-01
         };
         static const T Q[] = {
            1.00000000000000000e+00,
            7.36482529307436604e+00,
            2.03686007856430677e+01,
            2.62864592096657307e+01,
            1.59742041380858333e+01,
            4.03760534788374589e+00,
            2.91327346750475362e-01
         };
         return z * (offset + boost::math::tools::evaluate_polynomial(P, z) / boost::math::tools::evaluate_polynomial(Q, z));
      }
      else
      {
         // Max error in interpolated form: 3.806e-18
         static const T offset = 5.50335884094238281e-01;
         static const T P[] = {
            4.49664083944098322e-01,
            1.90417666196776909e+00,
            1.99951368798255994e+00,
            -6.91217310299270265e-01,
            -1.88533935998617058e+00,
            -7.96743968047750836e-01,
            -1.02891726031055254e-01,
            -3.09156013592636568e-03
         };
         static const T Q[] = {
            1.00000000000000000e+00,
            6.45854489419584014e+00,
            1.54739232422116048e+01,
            1.72606164253337843e+01,
            9.29427055609544096e+00,
            2.29040824649748117e+00,
            2.21610620995418981e-01,
            5.70597669908194213e-03
         };
         return z * (offset + boost::math::tools::evaluate_rational(P, Q, z));
      }
   }
   else if (z < 6)
   {
      // 2 < z < 6
      // Max error in interpolated form: 1.216e-17
      static const T Y = 1.16239356994628906e+00;
      static const T P[] = {
         -1.16230494982099475e+00,
         -3.38528144432561136e+00,
         -2.55653717293161565e+00,
         -3.06755172989214189e-01,
         1.73149743765268289e-01,
         3.76906042860014206e-02,
         1.84552217624706666e-03,
         1.69434126904822116e-05,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         3.77187616711220819e+00,
         4.58799960260143701e+00,
         2.24101228462292447e+00,
         4.54794195426212385e-01,
         3.60761772095963982e-02,
         9.25176499518388571e-04,
         4.43611344705509378e-06,
      };
      return Y + boost::math::tools::evaluate_rational(P, Q, z);
   }
   else if (z < 18)
   {
      // 6 < z < 18
      // Max error in interpolated form: 1.985e-19
      static const T offset = 1.80937194824218750e+00;
      static const T P[] =
      {
         -1.80690935424793635e+00,
         -3.66995929380314602e+00,
         -1.93842957940149781e+00,
         -2.94269984375794040e-01,
         1.81224710627677778e-03,
         2.48166798603547447e-03,
         1.15806592415397245e-04,
         1.43105573216815533e-06,
         3.47281483428369604e-09
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         2.57319080723908597e+00,
         1.96724528442680658e+00,
         5.84501352882650722e-01,
         7.37152837939206240e-02,
         3.97368430940416778e-03,
         8.54941838187085088e-05,
         6.05713225608426678e-07,
         8.17517283816615732e-10
      };
      return offset + boost::math::tools::evaluate_rational(P, Q, z);
   }
   else if (z < 9897.12905874)  // 2.8 < log(z) < 9.2
   {
      // Max error in interpolated form: 1.195e-18
      static const T Y = -1.40297317504882812e+00;
      static const T P[] = {
         1.97011826279311924e+00,
         1.05639945701546704e+00,
         3.33434529073196304e-01,
         3.34619153200386816e-02,
         -5.36238353781326675e-03,
         -2.43901294871308604e-03,
         -2.13762095619085404e-04,
         -4.85531936495542274e-06,
         -2.02473518491905386e-08,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         8.60107275833921618e-01,
         4.10420467985504373e-01,
         1.18444884081994841e-01,
         2.16966505556021046e-02,
         2.24529766630769097e-03,
         9.82045090226437614e-05,
         1.36363515125489502e-06,
         3.44200749053237945e-09,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_rational(P, Q, log_w);
   }
   else if (z < 7.896296e+13)  // 9.2 < log(z) <= 32
   {
      // Max error in interpolated form: 6.529e-18
      static const T Y = -2.73572921752929688e+00;
      static const T P[] = {
         3.30547638424076217e+00,
         1.64050071277550167e+00,
         4.57149576470736039e-01,
         4.03821227745424840e-02,
         -4.99664976882514362e-04,
         -1.28527893803052956e-04,
         -2.95470325373338738e-06,
         -1.76662025550202762e-08,
         -1.98721972463709290e-11,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         6.91472559412458759e-01,
         2.48154578891676774e-01,
         4.60893578284335263e-02,
         3.60207838982301946e-03,
         1.13001153242430471e-04,
         1.33690948263488455e-06,
         4.97253225968548872e-09,
         3.39460723731970550e-12,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_rational(P, Q, log_w);
   }
   else if (z < 2.6881171e+43) // 32 < log(z) < 100
   {
      // Max error in interpolated form: 2.015e-18
      static const T Y = -4.01286315917968750e+00;
      static const T P[] = {
         5.07714858354309672e+00,
         -3.32994414518701458e+00,
         -8.61170416909864451e-01,
         -4.01139705309486142e-02,
         -1.85374201771834585e-04,
         1.08824145844270666e-05,
         1.17216905810452396e-07,
         2.97998248101385990e-10,
         1.42294856434176682e-13,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         -4.85840770639861485e-01,
         -3.18714850604827580e-01,
         -3.20966129264610534e-02,
         -1.06276178044267895e-03,
         -1.33597828642644955e-05,
         -6.27900905346219472e-08,
         -9.35271498075378319e-11,
         -2.60648331090076845e-14,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_rational(P, Q, log_w);
   }
   else // 100 < log(z) < 710
   {
      // Max error in interpolated form: 5.277e-18
      static const T Y = -5.70115661621093750e+00;
      static const T P[] = {
         6.42275660145116698e+00,
         1.33047964073367945e+00,
         6.72008923401652816e-02,
         1.16444069958125895e-03,
         7.06966760237470501e-06,
         5.48974896149039165e-09,
         -7.00379652018853621e-11,
         -1.89247635913659556e-13,
         -1.55898770790170598e-16,
         -4.06109208815303157e-20,
         -2.21552699006496737e-24,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         3.34498588416632854e-01,
         2.51519862456384983e-02,
         6.81223810622416254e-04,
         7.94450897106903537e-06,
         4.30675039872881342e-08,
         1.10667669458467617e-10,
         1.31012240694192289e-13,
         6.53282047177727125e-17,
         1.11775518708172009e-20,
         3.78250395617836059e-25,
      };
      T log_w = log(z);
      return log_w + Y + boost::math::tools::evaluate_rational(P, Q, log_w);
   }
}

template <typename T, typename Policy>
T lambert_w_negative_rational_double(T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   if (z > -0.1)
   {
      if (z < -0.051)
      {
         // -0.1 < z < -0.051
         // Maximum Deviation Found:                     4.402e-22
         // Expected Error Term : 4.240e-22
         // Maximum Relative Change in Control Points : 4.115e-03
         static const T Y = 1.08633995056152344e+00;
         static const T P[] = {
            -8.63399505615014331e-02,
            -1.64303871814816464e+00,
            -7.71247913918273738e+00,
            -1.41014495545382454e+01,
            -1.02269079949257616e+01,
            -2.17236002836306691e+00,
         };
         static const T Q[] = {
            1.00000000000000000e+00,
            7.44775406945739243e+00,
            2.04392643087266541e+01,
            2.51001961077774193e+01,
            1.31256080849023319e+01,
            2.11640324843601588e+00,
         };
         return z * (Y + boost::math::tools::evaluate_rational(P, Q, z));
      }
      else
      {
         // Very small z > 0.051:
         return lambert_w0_small_z(z, pol);
      }
   }
   else if (z > -0.2)
   {
      // -0.2 < z < -0.1
      // Maximum Deviation Found:                     2.898e-20
      // Expected Error Term : 2.873e-20
      // Maximum Relative Change in Control Points : 3.779e-04
      static const T Y = 1.20359611511230469e+00;
      static const T P[] = {
         -2.03596115108465635e-01,
         -2.95029082937201859e+00,
         -1.54287922188671648e+01,
         -3.81185809571116965e+01,
         -4.66384358235575985e+01,
         -2.59282069989642468e+01,
         -4.70140451266553279e+00,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         9.57921436074599929e+00,
         3.60988119290234377e+01,
         6.73977699505546007e+01,
         6.41104992068148823e+01,
         2.82060127225153607e+01,
         4.10677610657724330e+00,
      };
      return z * (Y + boost::math::tools::evaluate_rational(P, Q, z));
   }
   else if (z > -0.3178794411714423215955237)
   {
      // Max error in interpolated form: 6.996e-18
      static const T Y = 3.49680423736572266e-01;
      static const T P[] = {
         -3.49729841718749014e-01,
         -6.28207407760709028e+01,
         -2.57226178029669171e+03,
         -2.50271008623093747e+04,
         1.11949239154711388e+05,
         1.85684566607844318e+06,
         4.80802490427638643e+06,
         2.76624752134636406e+06,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         1.82717661215113000e+02,
         8.00121119810280100e+03,
         1.06073266717010129e+05,
         3.22848993926057721e+05,
         -8.05684814514171256e+05,
         -2.59223192927265737e+06,
         -5.61719645211570871e+05,
         6.27765369292636844e+04,
      };
      T d = z + 0.367879441171442321595523770161460867445811;
      return -d / (Y + boost::math::tools::evaluate_polynomial(P, d) / boost::math::tools::evaluate_polynomial(Q, d));
   }
   else if (z > -0.3578794411714423215955237701)
   {
      // Max error in interpolated form: 1.404e-17
      static const T Y = 5.00126481056213379e-02;
      static const T  P[] = {
         -5.00173570682372162e-02,
         -4.44242461870072044e+01,
         -9.51185533619946042e+03,
         -5.88605699015429386e+05,
         -1.90760843597427751e+06,
         5.79797663818311404e+08,
         1.11383352508459134e+10,
         5.67791253678716467e+10,
         6.32694500716584572e+10,
      };
      static const T Q[] = {
         1.00000000000000000e+00,
         9.08910517489981551e+02,
         2.10170163753340133e+05,
         1.67858612416470327e+07,
         4.90435561733227953e+08,
         4.54978142622939917e+09,
         2.87716585708739168e+09,
         -4.59414247951143131e+10,
         -1.72845216404874299e+10,
      };
      T d = z + 0.36787944117144232159552377016146086744581113103176804;
      return -d / (Y + boost::math::tools::evaluate_polynomial(P, d) / boost::math::tools::evaluate_polynomial(Q, d));
   }
   else
   {  // z is very close (within 0.01) of the singularity at -e^-1,
      // so use a series expansion from R. M. Corless et al.
      const T p2 = 2 * (boost::math::constants::e<T>() * z + 1);
      const T p = sqrt(p2);
      return lambert_w_detail::lambert_w_singularity_series(p);
   }
}

//! Lambert_w0 @b 'double' implementation, selected when T is 64-bit precision.
template <typename T, typename Policy>
inline T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 2>&)
{
   static const char* function = "boost::math::lambert_w0<%1%>";
   BOOST_MATH_STD_USING // Aid ADL of std functions.

   // Detect unusual case of 32-bit double with a wider/64-bit long double
   static_assert(std::numeric_limits<double>::digits >= 53,
   "Our double precision coefficients will be truncated, "
   "please file a bug report with details of your platform's floating point types "
   "- or possibly edit the coefficients to have "
   "an appropriate size-suffix for 64-bit floats on your platform - L?");

    if ((boost::math::isnan)(z))
    {
      return boost::math::policies::raise_domain_error<T>(function, "Expected a value > -e^-1 (-0.367879...) but got %1%.", z, pol);
    }
    if ((boost::math::isinf)(z))
    {
      return boost::math::policies::raise_overflow_error<T>(function, "Expected a finite value but got %1%.", z, pol);
    }

   if (z >= 0.05)
   {
      return lambert_w_positive_rational_double(z);
   }
   else if (z <= -0.36787944117144232159552377016146086744581113103176804) // Precision is max_digits10(cpp_bin_float_50).
   {
      if (z < -0.36787944117144232159552377016146086744581113103176804)
      {
         return boost::math::policies::raise_domain_error<T>(function, "Expected z >= -e^-1 (-0.367879...) but got %1%.", z, pol);
      }
      return -1;
   }
   else
   {
      return lambert_w_negative_rational_double(z, pol);
   }
} // T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 2>&) 64-bit precision, usually double.

//! lambert_W0 implementation for extended precision types including
//! long double (80-bit and 128-bit), ???
//! quad float128, Boost.Multiprecision types like cpp_bin_float_quad, cpp_bin_float_50...

template <typename T, typename Policy>
inline T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 0>&)
{
   static const char* function = "boost::math::lambert_w0<%1%>";
   BOOST_MATH_STD_USING // Aid ADL of std functions.

   // Filter out special cases first:
   if ((boost::math::isnan)(z))
   {
      return boost::math::policies::raise_domain_error<T>(function, "Expected z >= -e^-1 (-0.367879...) but got %1%.", z, pol);
   }
   if (fabs(z) <= 0.05f)
   {
      // Very small z:
      return lambert_w0_small_z(z, pol);
   }
   if (z > (std::numeric_limits<double>::max)())
   {
      if ((boost::math::isinf)(z))
      {
         return policies::raise_overflow_error<T>(function, 0, pol);
         // Or might return infinity if available else max_value,
         // but other Boost.Math special functions raise overflow.
      }
      // z is larger than the largest double, so cannot use the polynomial to get an approximation,
      // so use the asymptotic approximation and Halley iterate:

     T w = lambert_w0_approx(z);  // Make an inline function as also used elsewhere.
      //T lz = log(z);
      //T llz = log(lz);
      //T w = lz - llz + (llz / lz); // Corless equation 4.19, page 349, and Chapeau-Blondeau equation 20, page 2162.
      return lambert_w_halley_iterate(w, z);
   }
   if (z < -0.3578794411714423215955237701)
   { // Very close to branch point so rational polynomials are not usable.
      if (z <= -boost::math::constants::exp_minus_one<T>())
      {
         if (z == -boost::math::constants::exp_minus_one<T>())
         { // Exactly at the branch point singularity.
            return -1;
         }
         return boost::math::policies::raise_domain_error<T>(function, "Expected z >= -e^-1 (-0.367879...) but got %1%.", z, pol);
      }
      // z is very close (within 0.01) of the branch singularity at -e^-1
      // so use a series approximation proposed by Corless et al.
      const T p2 = 2 * (boost::math::constants::e<T>() * z + 1);
      const T p = sqrt(p2);
      T w = lambert_w_detail::lambert_w_singularity_series(p);
      return lambert_w_halley_iterate(w, z);
   }

   // Phew!  If we get here we are in the normal range of the function,
   // so get a double precision approximation first, then iterate to full precision of T.
   // We define a tag_type that is:
   // true_type if there are so many digits precision wanted that iteration is necessary.
   // false_type if a single Halley step is sufficient.

   using precision_type = typename policies::precision<T, Policy>::type;
   using tag_type = std::integral_constant<bool,
      (precision_type::value == 0) || (precision_type::value > 113) ?
      true // Unknown at compile-time, variable/arbitrary, or more than float128 or cpp_bin_quad 128-bit precision.
      : false // float, double, float128, cpp_bin_quad 128-bit, so single Halley step.
   >;

   // For speed, we also cast z to type double when that is possible
   //   if (std::is_constructible<double, T>() == true).
   T w = lambert_w0_imp(maybe_reduce_to_double(z, std::is_constructible<double, T>()), pol, std::integral_constant<int, 2>());

   return lambert_w_maybe_halley_iterate(w, z, tag_type());

} // T lambert_w0_imp(T z, const Policy& pol, const std::integral_constant<int, 0>&)  all extended precision types.

  // Lambert w-1 implementation
// ==============================================================================================

  //! Lambert W for W-1 branch, -max(z) < z <= -1/e.
  // TODO is -max(z) allowed?
template<typename T, typename Policy>
T lambert_wm1_imp(const T z, const Policy&  pol)
{
  // Catch providing an integer value as parameter x to lambert_w, for example, lambert_w(1).
  // Need to ensure it is a floating-point type (of the desired type, float 1.F, double 1., or long double 1.L),
  // or static_casted integer, for example:  static_cast<float>(1) or static_cast<cpp_dec_float_50>(1).
  // Want to allow fixed_point types too, so do not just test for floating-point.
  // Integral types should be promoted to double by user Lambert w functions.
  // If integral type provided to user function lambert_w0 or lambert_wm1,
  // then should already have been promoted to double.
  static_assert(!std::is_integral<T>::value,
    "Must be floating-point or fixed type (not integer type), for example: lambert_wm1(1.), not lambert_wm1(1)!");

  BOOST_MATH_STD_USING // Aid argument dependent lookup (ADL) of abs.

  const char* function = "boost::math::lambert_wm1<RealType>(<RealType>)"; // Used for error messages.

  // Check for edge and corner cases first:
  if ((boost::math::isnan)(z))
  {
    return policies::raise_domain_error(function,
      "Argument z is NaN!",
      z, pol);
  } // isnan

  if ((boost::math::isinf)(z))
  {
    return policies::raise_domain_error(function,
      "Argument z is infinite!",
      z, pol);
  } // isinf

  if (z == static_cast<T>(0))
  { // z is exactly zero so return -std::numeric_limits<T>::infinity();
    if (std::numeric_limits<T>::has_infinity)
    {
      return -std::numeric_limits<T>::infinity();
    }
    else
    {
      return -tools::max_value<T>();
    }
  }
  if (std::numeric_limits<T>::has_denorm)
  { // All real types except arbitrary precision.
    if (!(boost::math::isnormal)(z))
    { // Almost zero - might also just return infinity like z == 0 or max_value?
      return policies::raise_overflow_error(function,
        "Argument z =  %1% is denormalized! (must be z > (std::numeric_limits<RealType>::min)() or z == 0)",
        z, pol);
    }
  }

  if (z > static_cast<T>(0))
  { //
    return policies::raise_domain_error(function,
      "Argument z = %1% is out of range (z <= 0) for Lambert W-1 branch! (Try Lambert W0 branch?)",
      z, pol);
  }
  if (z > -boost::math::tools::min_value<T>())
  { // z is denormalized, so cannot be computed.
    // -std::numeric_limits<T>::min() is smallest for type T,
    // for example, for double: lambert_wm1(-2.2250738585072014e-308) = -714.96865723796634
    return policies::raise_overflow_error(function,
      "Argument z = %1% is too small (z < -std::numeric_limits<T>::min so denormalized) for Lambert W-1 branch!",
      z, pol);
  }
  if (z == -boost::math::constants::exp_minus_one<T>()) // == singularity/branch point z = -exp(-1) = -3.6787944.
  { // At singularity, so return exactly -1.
    return -static_cast<T>(1);
  }
  // z is too negative for the W-1 (or W0) branch.
  if (z < -boost::math::constants::exp_minus_one<T>()) // > singularity/branch point z = -exp(-1) = -3.6787944.
  {
    return policies::raise_domain_error(function,
      "Argument z = %1% is out of range (z < -exp(-1) = -3.6787944... <= 0) for Lambert W-1 (or W0) branch!",
      z, pol);
  }
  if (z < static_cast<T>(-0.35))
  { // Close to singularity/branch point z = -0.3678794411714423215955237701614608727 but on W-1 branch.
    const T p2 = 2 * (boost::math::constants::e<T>() * z + 1);
    if (p2 == 0)
    { // At the singularity at branch point.
      return -1;
    }
    if (p2 > 0)
    {
      T w_series = lambert_w_singularity_series(T(-sqrt(p2)));
      if (boost::math::tools::digits<T>() > 53)
      { // Multiprecision, so try a Halley refinement.
        w_series = lambert_w_detail::lambert_w_halley_iterate(w_series, z);
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1_NOT_BUILTIN
        std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
        std::cout << "Lambert W-1 Halley updated to " << w_series << std::endl;
        std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_WM1_NOT_BUILTIN
      }
      return w_series;
    }
    // Should not get here.
    return policies::raise_domain_error(function,
      "Argument z = %1% is out of range for Lambert W-1 branch. (Should not get here - please report!)",
      z, pol);
  } // if (z < -0.35)

  using lambert_w_lookup::wm1es;
  using lambert_w_lookup::wm1zs;
  using lambert_w_lookup::noof_wm1zs; // size == 64

  // std::cout <<" Wm1zs[63] (== G[64]) = " << " " << wm1zs[63] << std::endl; // Wm1zs[63] (== G[64]) =  -1.0264389699511283e-26
  // Check that z argument value is not smaller than lookup_table G[64]
  // std::cout << "(z > wm1zs[63]) = " << std::boolalpha << (z > wm1zs[63]) << std::endl;

  if (z >= wm1zs[63]) // wm1zs[63]  = -1.0264389699511282259046957018510946438e-26L  W = 64.00000000000000000
  {  // z >= -1.0264389699511303e-26 (but z != 0 and z >= std::numeric_limits<T>::min() and so NOT denormalized).

    // Some info on Lambert W-1 values for extreme values of z.
    // std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
    // std::cout << "-std::numeric_limits<float>::min() = " << -(std::numeric_limits<float>::min)() << std::endl;
    // std::cout << "-std::numeric_limits<double>::min() = " << -(std::numeric_limits<double>::min)() << std::endl;
    // -std::numeric_limits<float>::min() = -1.1754943508222875e-38
    // -std::numeric_limits<double>::min() = -2.2250738585072014e-308
    // N[productlog(-1, -1.1754943508222875 * 10^-38 ), 50] = -91.856775324595479509567756730093823993834155027858
    // N[productlog(-1, -2.2250738585072014e-308 * 10^-308 ), 50] = -1424.8544521230553853558132180518404363617968042942
    // N[productlog(-1, -1.4325445274604020119111357113179868158* 10^-27), 37] = -65.99999999999999999999999999999999955

    // R.M.Corless, G.H.Gonnet, D.E.G.Hare, D.J.Jeffrey, and D.E.Knuth,
    // On the Lambert W function, Adv.Comput.Math., vol. 5, pp. 329, 1996.
    // Francois Chapeau-Blondeau and Abdelilah Monir
    // Numerical Evaluation of the Lambert W Function
    // IEEE Transactions On Signal Processing, VOL. 50, NO. 9, Sep 2002
    // https://pdfs.semanticscholar.org/7a5a/76a9369586dd0dd34dda156d8f2779d1fd59.pdf
    // Estimate Lambert W using ln(-z)  ...
    // This is roughly the power of ten * ln(10) ~= 2.3.   n ~= 10^n
    //  and improve by adding a second term -ln(ln(-z))
    T guess; // bisect lowest possible Gk[=64] (for lookup_t type)
    T lz = log(-z);
    T llz = log(-lz);
    guess = lz - llz + (llz / lz); // Chapeau-Blondeau equation 20, page 2162.
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1_TINY
    std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
    std::cout << "z = " << z << ", guess = " << guess << ", ln(-z) = " << lz << ", ln(-ln(-z) = " << llz << ", llz/lz = " << (llz / lz) << std::endl;
    // z = -1.0000000000000001e-30, guess = -73.312782616731482, ln(-z) = -69.077552789821368, ln(-ln(-z) = 4.2352298269101114, llz/lz = -0.061311231447304194
    // z = -9.9999999999999999e-91, guess = -212.56650048504233, ln(-z) = -207.23265836946410, ln(-ln(-z) = 5.3338421155782205, llz/lz = -0.025738424423764311
    // >z = -2.2250738585072014e-308, guess = -714.95942238244606, ln(-z) = -708.39641853226408, ln(-ln(-z) = 6.5630038501819854, llz/lz = -0.0092645920821846622
    int d10 = policies::digits_base10<T, Policy>(); // policy template parameter digits10
    int d2 = policies::digits<T, Policy>(); // digits base 2 from policy.
    std::cout << "digits10 = " << d10 << ", digits2 = " << d2 // For example: digits10 = 1, digits2 = 5
      << std::endl;
    std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_WM1_TINY
    if (policies::digits<T, Policy>() < 12)
    { // For the worst case near w = 64, the error in the 'guess' is ~0.008, ratio ~ 0.0001 or 1 in 10,000 digits 10 ~= 4, or digits2 ~= 12.
      return guess;
    }
    T result = lambert_w_detail::lambert_w_halley_iterate(guess, z);
    return result;

    // Was Fukushima
    // G[k=64] == g[63] == -1.02643897e-26
    //return policies::raise_domain_error(function,
    //  "Argument z = %1% is too small (< -1.02643897e-26) ! (Should not occur, please report.",
    //  z, pol);
  } // Z too small so use approximation and Halley.
    // Else Use a lookup table to find the nearest integer part of Lambert W-1 as starting point for Bisection.

  if (boost::math::tools::digits<T>() > 53)
  { // T is more precise than 64-bit double (or long double, or ?),
    // so compute an approximate value using only one Schroeder refinement,
    // (avoiding any double-precision Halley refinement from policy double_digits2<50> 53 - 3 = 50
    // because are next going to use Halley refinement at full/high precision using this as an approximation).
    using boost::math::policies::precision;
    using boost::math::policies::digits10;
    using boost::math::policies::digits2;
    using boost::math::policies::policy;
    // Compute a 50-bit precision approximate W0 in a double (no Halley refinement).
    T double_approx(static_cast<T>(lambert_wm1_imp(must_reduce_to_double(z, std::is_constructible<double, T>()), policy<digits2<50>>())));
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1_NOT_BUILTIN
    std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
    std::cout << "Lambert_wm1 Argument Type " << typeid(T).name() << " approximation double = " << double_approx << std::endl;
    std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_WM1
    // Perform additional Halley refinement(s) to ensure that
    // get a near as possible to correct result (usually +/- one epsilon).
    T result = lambert_w_halley_iterate(double_approx, z);
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1
    std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
    std::cout << "Result " << typeid(T).name() << " precision Halley refinement =    " << result << std::endl;
    std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_WM1
    return result;
  } // digits > 53  - higher precision than double.
  else // T is double or less precision.
  { // Use a lookup table to find the nearest integer part of Lambert W as starting point for Bisection.
    using namespace boost::math::lambert_w_detail::lambert_w_lookup;
    // Bracketing sequence  n = (2, 4, 8, 16, 32, 64) for W-1 branch. (0 is -infinity)
    // Since z is probably quite small, start with lowest n (=2).
    int n = 2;
    if (wm1zs[n - 1] > z)
    {
      goto bisect;
    }
    for (int j = 1; j <= 5; ++j)
    {
      n *= 2;
      if (wm1zs[n - 1] > z)
      {
        goto overshot;
      }
    }
    // else z < g[63] == -1.0264389699511303e-26, so Lambert W-1 integer part > 64.
    // This should not now occur (should be caught by test and code above) so should be a logic_error?
    return policies::raise_domain_error(function,
      "Argument z = %1% is too small (< -1.026439e-26) (logic error - please report!)",
      z, pol);
  overshot:
    {
      int nh = n / 2;
      for (int j = 1; j <= 5; ++j)
      {
        nh /= 2; // halve step size.
        if (nh <= 0)
        {
          break; // goto bisect;
        }
        if (wm1zs[n - nh - 1] > z)
        {
          n -= nh;
        }
      }
    }
  bisect:
    --n;  
    // g[n] now holds lambert W of floor integer n and g[n+1] the ceil part;
    // these are used as initial values for bisection.
#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1_LOOKUP
    std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
    std::cout << "Result lookup W-1(" << z << ") bisection between wm1zs[" << n - 1 << "] = " << wm1zs[n - 1] << " and wm1zs[" << n << "] = " << wm1zs[n]
      << ", bisect mean = " << (wm1zs[n - 1] + wm1zs[n]) / 2 << std::endl;
    std::cout.precision(saved_precision);
#endif // BOOST_MATH_INSTRUMENT_LAMBERT_WM1_LOOKUP

    // Compute bisections is the number of bisections computed from n,
    // such that a single application of the fifth-order Schroeder update formula
    // after the bisections is enough to evaluate Lambert W-1 with (near?) 53-bit accuracy.
    // Fukushima established these by trial and error?
    int bisections = 11; //  Assume maximum number of bisections will be needed (most common case).
    if (n >= 8)
    {
      bisections = 8;
    }
    else if (n >= 3)
    {
      bisections = 9;
    }
    else if (n >= 2)
    {
      bisections = 10;
    }
    // Bracketing, Fukushima section 2.3, page 82:
    // (Avoiding using exponential function for speed).
    // Only use @c lookup_t precision, default double, for bisection (again for speed),
    // and use later Halley refinement for higher precisions.
    using lambert_w_lookup::halves;
    using lambert_w_lookup::sqrtwm1s;

    using calc_type = typename std::conditional<std::is_constructible<lookup_t, T>::value, lookup_t, T>::type;

    calc_type w = -static_cast<calc_type>(n); // Equation 25,
    calc_type y = static_cast<calc_type>(z * wm1es[n - 1]); // Equation 26,
                                                          // Perform the bisections fractional bisections for necessary precision.
    for (int j = 0; j < bisections; ++j)
    { // Equation 27.
      calc_type wj = w - halves[j]; // Subtract 1/2, 1/4, 1/8 ...
      calc_type yj = y * sqrtwm1s[j]; // Multiply by sqrt(1/e), ...
      if (wj < yj)
      {
        w = wj;
        y = yj;
      }
    } // for j
    return static_cast<T>(schroeder_update(w, y)); // Schroeder 5th order method refinement.

//      else // Perform additional Halley refinement(s) to ensure that
//           // get a near as possible to correct result (usually +/- epsilon).
//      {
//       // result = lambert_w_halley_iterate(result, z);
//        result = lambert_w_halley_step(result, z);  // Just one Halley step should be enough.
//#ifdef BOOST_MATH_INSTRUMENT_LAMBERT_WM1_HALLEY
//        std::streamsize saved_precision = std::cout.precision(std::numeric_limits<T>::max_digits10);
//        std::cout << "Halley refinement estimate =    " << result << std::endl;
//        std::cout.precision(saved_precision);
//#endif // BOOST_MATH_INSTRUMENT_LAMBERT_W1_HALLEY
//        return result; // Halley
//      } // Schroeder or Schroeder and Halley.
    }
  } // template<typename T = double> T lambert_wm1_imp(const T z)
} // namespace lambert_w_detail

/////////////////////////////  User Lambert w functions. //////////////////////////////

//! Lambert W0 using User-defined policy.
  template <typename T, typename Policy>
  inline
    typename boost::math::tools::promote_args<T>::type
    lambert_w0(T z, const Policy& pol)
  {
     // Promote integer or expression template arguments to double,
     // without doing any other internal promotion like float to double.
    using result_type = typename tools::promote_args<T>::type;

    // Work out what precision has been selected,
    // based on the Policy and the number type.
    using precision_type = typename policies::precision<result_type, Policy>::type;
    // and then select the correct implementation based on that precision (not the type T):
    using tag_type = std::integral_constant<int,
      (precision_type::value == 0) || (precision_type::value > 53) ?
        0  // either variable precision (0), or greater than 64-bit precision.
      : (precision_type::value <= 24) ? 1 // 32-bit (probably float) precision.
      : 2  // 64-bit (probably double) precision.
      >;

    return lambert_w_detail::lambert_w0_imp(result_type(z), pol, tag_type()); // 
  } // lambert_w0(T z, const Policy& pol)

  //! Lambert W0 using default policy.
  template <typename T>
  inline
    typename tools::promote_args<T>::type
    lambert_w0(T z)
  {
    // Promote integer or expression template arguments to double,
    // without doing any other internal promotion like float to double.
    using result_type = typename tools::promote_args<T>::type;

    // Work out what precision has been selected, based on the Policy and the number type.
    // For the default policy version, we want the *default policy* precision for T.
    using precision_type = typename policies::precision<result_type, policies::policy<>>::type;
    // and then select the correct implementation based on that (not the type T):
    using tag_type = std::integral_constant<int,
      (precision_type::value == 0) || (precision_type::value > 53) ?
      0  // either variable precision (0), or greater than 64-bit precision.
      : (precision_type::value <= 24) ? 1 // 32-bit (probably float) precision.
      : 2  // 64-bit (probably double) precision.
    >;
    return lambert_w_detail::lambert_w0_imp(result_type(z),  policies::policy<>(), tag_type());
  } // lambert_w0(T z) using default policy.

    //! W-1 branch (-max(z) < z <= -1/e).

    //! Lambert W-1 using User-defined policy.
  template <typename T, typename Policy>
  inline
    typename tools::promote_args<T>::type
    lambert_wm1(T z, const Policy& pol)
  {
    // Promote integer or expression template arguments to double,
    // without doing any other internal promotion like float to double.
    using result_type = typename tools::promote_args<T>::type;
    return lambert_w_detail::lambert_wm1_imp(result_type(z), pol); //
  }

  //! Lambert W-1 using default policy.
  template <typename T>
  inline
    typename tools::promote_args<T>::type
    lambert_wm1(T z)
  {
    using result_type = typename tools::promote_args<T>::type;
    return lambert_w_detail::lambert_wm1_imp(result_type(z), policies::policy<>());
  } // lambert_wm1(T z)

  // First derivative of Lambert W0 and W-1.
  template <typename T, typename Policy>
  inline typename tools::promote_args<T>::type
  lambert_w0_prime(T z, const Policy& pol)
  {
    using result_type = typename tools::promote_args<T>::type;
    using std::numeric_limits;
    if (z == 0)
    {
        return static_cast<result_type>(1);
    }
    // This is the sensible choice if we regard the Lambert-W function as complex analytic.
    // Of course on the real line, it's just undefined.
    if (z == - boost::math::constants::exp_minus_one<result_type>())
    {
        return numeric_limits<result_type>::has_infinity ? numeric_limits<result_type>::infinity() : boost::math::tools::max_value<result_type>();
    }
    // if z < -1/e, we'll let lambert_w0 do the error handling:
    result_type w = lambert_w0(result_type(z), pol);
    // If w ~ -1, then presumably this can get inaccurate.
    // Is there an accurate way to evaluate 1 + W(-1/e + eps)?
    //  Yes: This is discussed in the Princeton Companion to Applied Mathematics,
    // 'The Lambert-W function', Section 1.3: Series and Generating Functions.
    // 1 + W(-1/e + x) ~ sqrt(2ex).
    // Nick is not convinced this formula is more accurate than the naive one.
    // However, for z != -1/e, we never get rounded to w = -1 in any precision I've tested (up to cpp_bin_float_100).
    return w / (z * (1 + w));
  } // lambert_w0_prime(T z)

  template <typename T>
  inline typename tools::promote_args<T>::type
     lambert_w0_prime(T z)
  {
     return lambert_w0_prime(z, policies::policy<>());
  }
  
  template <typename T, typename Policy>
  inline typename tools::promote_args<T>::type
  lambert_wm1_prime(T z, const Policy& pol)
  {
    using std::numeric_limits;
    using result_type = typename tools::promote_args<T>::type;
    //if (z == 0)
    //{
    //      return static_cast<result_type>(1);
    //}
    //if (z == - boost::math::constants::exp_minus_one<result_type>())
    if (z == 0 || z == - boost::math::constants::exp_minus_one<result_type>())
    {
        return numeric_limits<result_type>::has_infinity ? -numeric_limits<result_type>::infinity() : -boost::math::tools::max_value<result_type>();
    }

    result_type w = lambert_wm1(z, pol);
    return w/(z*(1+w));
  } // lambert_wm1_prime(T z)

  template <typename T>
  inline typename tools::promote_args<T>::type
     lambert_wm1_prime(T z)
  {
     return lambert_wm1_prime(z, policies::policy<>());
  }

}} //boost::math namespaces

#endif // #ifdef BOOST_MATH_SF_LAMBERT_W_HPP


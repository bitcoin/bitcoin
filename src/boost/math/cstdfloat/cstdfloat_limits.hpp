///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2014.
// Copyright John Maddock 2014.
// Copyright Paul Bristow 2014.
// Distributed under the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Implement quadruple-precision std::numeric_limits<> support.

#ifndef BOOST_MATH_CSTDFLOAT_LIMITS_2014_01_09_HPP_
  #define BOOST_MATH_CSTDFLOAT_LIMITS_2014_01_09_HPP_

  #include <boost/math/cstdfloat/cstdfloat_types.hpp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

  #if defined(BOOST_CSTDFLOAT_HAS_INTERNAL_FLOAT128_T) && defined(BOOST_MATH_USE_FLOAT128) && !defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT)

    #include <limits>

    // Define the name of the global quadruple-precision function to be used for
    // calculating quiet_NaN() in the specialization of std::numeric_limits<>.
    #if defined(__INTEL_COMPILER)
      #define BOOST_CSTDFLOAT_FLOAT128_SQRT   __sqrtq
    #elif defined(__GNUC__)
      #define BOOST_CSTDFLOAT_FLOAT128_SQRT   sqrtq
    #endif

    // Forward declaration of the quadruple-precision square root function.
    extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_SQRT(boost::math::cstdfloat::detail::float_internal128_t) throw();

    namespace std
    {
      template<>
      class numeric_limits<boost::math::cstdfloat::detail::float_internal128_t>
      {
      public:
        static constexpr bool                                                 is_specialized           = true;
        static                 boost::math::cstdfloat::detail::float_internal128_t  (min) () noexcept  { return BOOST_CSTDFLOAT_FLOAT128_MIN; }
        static                 boost::math::cstdfloat::detail::float_internal128_t  (max) () noexcept  { return BOOST_CSTDFLOAT_FLOAT128_MAX; }
        static                 boost::math::cstdfloat::detail::float_internal128_t  lowest() noexcept  { return -(max)(); }
        static constexpr int                                                  digits                   = 113;
        static constexpr int                                                  digits10                 = 33;
        static constexpr int                                                  max_digits10             = 36;
        static constexpr bool                                                 is_signed                = true;
        static constexpr bool                                                 is_integer               = false;
        static constexpr bool                                                 is_exact                 = false;
        static constexpr int                                                  radix                    = 2;
        static                 boost::math::cstdfloat::detail::float_internal128_t  epsilon    ()            { return BOOST_CSTDFLOAT_FLOAT128_EPS; }
        static                 boost::math::cstdfloat::detail::float_internal128_t  round_error()            { return BOOST_FLOAT128_C(0.5); }
        static constexpr int                                                  min_exponent             = -16381;
        static constexpr int                                                  min_exponent10           = static_cast<int>((min_exponent * 301L) / 1000L);
        static constexpr int                                                  max_exponent             = +16384;
        static constexpr int                                                  max_exponent10           = static_cast<int>((max_exponent * 301L) / 1000L);
        static constexpr bool                                                 has_infinity             = true;
        static constexpr bool                                                 has_quiet_NaN            = true;
        static constexpr bool                                                 has_signaling_NaN        = false;
        static constexpr float_denorm_style                                   has_denorm               = denorm_present;
        static constexpr bool                                                 has_denorm_loss          = false;
        static                 boost::math::cstdfloat::detail::float_internal128_t  infinity     ()          { return BOOST_FLOAT128_C(1.0) / BOOST_FLOAT128_C(0.0); }
        static                 boost::math::cstdfloat::detail::float_internal128_t  quiet_NaN    ()          { return -(::BOOST_CSTDFLOAT_FLOAT128_SQRT(BOOST_FLOAT128_C(-1.0))); }
        static                 boost::math::cstdfloat::detail::float_internal128_t  signaling_NaN()          { return BOOST_FLOAT128_C(0.0); }
        static                 boost::math::cstdfloat::detail::float_internal128_t  denorm_min   ()          { return BOOST_CSTDFLOAT_FLOAT128_DENORM_MIN; }
        static constexpr bool                                                 is_iec559                = true;
        static constexpr bool                                                 is_bounded               = true;
        static constexpr bool                                                 is_modulo                = false;
        static constexpr bool                                                 traps                    = false;
        static constexpr bool                                                 tinyness_before          = false;
        static constexpr float_round_style                                    round_style              = round_to_nearest;
      };
    } // namespace std

  #endif // Not BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT (i.e., the user would like to have libquadmath support)

#endif // BOOST_MATH_CSTDFLOAT_LIMITS_2014_01_09_HPP_


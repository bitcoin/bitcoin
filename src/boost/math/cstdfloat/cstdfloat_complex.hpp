///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2014.
// Copyright John Maddock 2014.
// Copyright Paul Bristow 2014.
// Distributed under the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Implement quadruple-precision (and extended) support for <complex>.

#ifndef BOOST_MATH_CSTDFLOAT_COMPLEX_2014_02_15_HPP_
  #define BOOST_MATH_CSTDFLOAT_COMPLEX_2014_02_15_HPP_

  #include <boost/math/cstdfloat/cstdfloat_types.hpp>
  #include <boost/math/cstdfloat/cstdfloat_limits.hpp>
  #include <boost/math/cstdfloat/cstdfloat_cmath.hpp>
  #include <boost/math/cstdfloat/cstdfloat_iostream.hpp>

  #if defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_LIMITS)
  #error You can not use <boost/math/cstdfloat/cstdfloat_complex.hpp> with BOOST_CSTDFLOAT_NO_LIBQUADMATH_LIMITS defined.
  #endif
  #if defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_CMATH)
  #error You can not use <boost/math/cstdfloat/cstdfloat_complex.hpp> with BOOST_CSTDFLOAT_NO_LIBQUADMATH_CMATH defined.
  #endif
  #if defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_IOSTREAM)
  #error You can not use <boost/math/cstdfloat/cstdfloat_complex.hpp> with BOOST_CSTDFLOAT_NO_LIBQUADMATH_IOSTREAM defined.
  #endif

  #if defined(BOOST_CSTDFLOAT_HAS_INTERNAL_FLOAT128_T) && defined(BOOST_MATH_USE_FLOAT128) && !defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT)

  #define BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE boost::math::cstdfloat::detail::float_internal128_t
  #include <boost/math/cstdfloat/cstdfloat_complex_std.hpp>
  #undef BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE

  #endif // Not BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT (i.e., the user would like to have libquadmath support)

#endif // BOOST_MATH_CSTDFLOAT_COMPLEX_2014_02_15_HPP_

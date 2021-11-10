
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2014 Anton Bikineev
//  Copyright 2014 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2014 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_HYPERGEOMETRIC_PADE_HPP
#define BOOST_MATH_HYPERGEOMETRIC_PADE_HPP

  namespace boost{ namespace math{ namespace detail{

  // Luke: C ---------- SUBROUTINE R1F1P(CP, Z, A, B, N) ----------
  // Luke: C ----- PADE APPROXIMATION OF 1F1( 1 ; CP ; -Z ) -------
  template <class T, class Policy>
  inline T hypergeometric_1F1_pade(const T& cp, const T& zp, const Policy& )
  {
    BOOST_MATH_STD_USING

    static const T one = T(1);

    // Luke: C ------------- INITIALIZATION -------------
    const T z = -zp;
    const T zz = z * z;
    T b0 = one;
    T a0 = one;
    T xi1 = one;
    T ct1 = cp + one;
    T cp1 = cp - one;

    T b1 = one + (z / ct1);
    T a1 = b1 - (z / cp);

    const unsigned max_iterations = boost::math::policies::get_max_series_iterations<Policy>();

    T b2 = T(0), a2 = T(0);
    T result = T(0), prev_result;

    for (unsigned k = 1; k < max_iterations; ++k)
    {
      // Luke: C ----- CALCULATION OF THE MULTIPLIERS -----
      // Luke: C ----------- FOR THE RECURSION ------------
      const T ct2 = ct1 * ct1;
      const T g1 = one + ((cp1 / (ct2 + ct1 + ct1)) * z);
      const T g2 = ((xi1 / (ct2 - one)) * ((xi1 + cp1) / ct2)) * zz;

      // Luke: C ------- THE RECURRENCE RELATIONS ---------
      // Luke: C ------------ ARE AS FOLLOWS --------------
      b2 = (g1 * b1) + (g2 * b0);
      a2 = (g1 * a1) + (g2 * a0);

      prev_result = result;
      result = a2 / b2;

      // condition for interruption
      if ((fabs(result) * boost::math::tools::epsilon<T>()) > fabs(result - prev_result))
        break;

      b0 = b1; b1 = b2;
      a0 = a1; a1 = a2;

      ct1 += 2;
      xi1 += 1;
    }

    return a2 / b2;
  }

  // Luke: C -------- SUBROUTINE R2F1P(BP, CP, Z, A, B, N) --------
  // Luke: C ---- PADE APPROXIMATION OF 2F1( 1 , BP; CP ; -Z ) ----
  template <class T, class Policy>
  inline T hypergeometric_2F1_pade(const T& bp, const T& cp, const T& zp, const Policy&)
  {
    BOOST_MATH_STD_USING

    static const T one = T(1);

    // Luke: C ---------- INITIALIZATION -----------
    const T z = -zp;
    const T zz = z * z;
    T b0 = one;
    T a0 = one;
    T xi1 = one;
    T ct1 = cp;
    const T b1c1 = (cp - one) * (bp - one);

    T b1 = one + ((z / (cp + one)) * (bp + one));
    T a1 = b1 - ((bp / cp) * z);

    const unsigned max_iterations = boost::math::policies::get_max_series_iterations<Policy>();

    T b2 = T(0), a2 = T(0);
    T result = T(0), prev_result = a1 / b1;

    for (unsigned k = 1; k < max_iterations; ++k)
    {
      // Luke: C ----- CALCULATION OF THE MULTIPLIERS -----
      // Luke: C ----------- FOR THE RECURSION ------------
      const T ct2 = ct1 + xi1;
      const T ct3 = ct2 * ct2;
      const T g2 = (((((ct1 / ct3) * (bp - ct1)) / (ct3 - one)) * xi1) * (bp + xi1)) * zz;
      ++xi1;
      const T g1 = one + (((((xi1 + xi1) * ct1) + b1c1) / (ct3 + ct2 + ct2)) * z);

      // Luke: C ------- THE RECURRENCE RELATIONS ---------
      // Luke: C ------------ ARE AS FOLLOWS --------------
      b2 = (g1 * b1) + (g2 * b0);
      a2 = (g1 * a1) + (g2 * a0);

      prev_result = result;
      result = a2 / b2;

      // condition for interruption
      if ((fabs(result) * boost::math::tools::epsilon<T>()) > fabs(result - prev_result))
        break;

      b0 = b1; b1 = b2;
      a0 = a1; a1 = a2;

      ++ct1;
    }

    return a2 / b2;
  }

  } } } // namespaces

#endif // BOOST_MATH_HYPERGEOMETRIC_PADE_HPP

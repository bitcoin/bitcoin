//  Copyright (c) 2013 Anton Bikineev
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This is a partial header, do not include on it's own!!!
//
// Contains asymptotic expansions for derivatives of Bessel J(v,x) and Y(v,x)
// functions, as x -> INF.
#ifndef BOOST_MATH_SF_DETAIL_BESSEL_JY_DERIVATIVES_ASYM_HPP
#define BOOST_MATH_SF_DETAIL_BESSEL_JY_DERIVATIVES_ASYM_HPP

#ifdef _MSC_VER
#pragma once
#endif

namespace boost{ namespace math{ namespace detail{

template <class T>
inline T asymptotic_bessel_derivative_amplitude(T v, T x)
{
   // Calculate the amplitude for J'(v,x) and I'(v,x)
   // for large x: see A&S 9.2.30.
   BOOST_MATH_STD_USING
   T s = 1;
   const T mu = 4 * v * v;
   T txq = 2 * x;
   txq *= txq;

   s -= (mu - 3) / (2 * txq);
   s -= ((mu - 1) * (mu - 45)) / (txq * txq * 8);

   return sqrt(s * 2 / (boost::math::constants::pi<T>() * x));
}

template <class T>
inline T asymptotic_bessel_derivative_phase_mx(T v, T x)
{
   // Calculate the phase of J'(v, x) and Y'(v, x) for large x.
   // See A&S 9.2.31.
   // Note that the result returned is the phase less (x - PI(v/2 - 1/4))
   // which we'll factor in later when we calculate the sines/cosines of the result:
   const T mu = 4 * v * v;
   const T mu2 = mu * mu;
   const T mu3 = mu2 * mu;
   T denom = 4 * x;
   T denom_mult = denom * denom;

   T s = 0;
   s += (mu + 3) / (2 * denom);
   denom *= denom_mult;
   s += (mu2 + (46 * mu) - 63) / (6 * denom);
   denom *= denom_mult;
   s += (mu3 + (185 * mu2) - (2053 * mu) + 1899) / (5 * denom);
   return s;
}

template <class T, class Policy>
inline T asymptotic_bessel_y_derivative_large_x_2(T v, T x, const Policy& pol)
{
   // See A&S 9.2.20.
   BOOST_MATH_STD_USING
   // Get the phase and amplitude:
   const T ampl = asymptotic_bessel_derivative_amplitude(v, x);
   const T phase = asymptotic_bessel_derivative_phase_mx(v, x);
   BOOST_MATH_INSTRUMENT_VARIABLE(ampl);
   BOOST_MATH_INSTRUMENT_VARIABLE(phase);
   //
   // Calculate the sine of the phase, using
   // sine/cosine addition rules to factor in
   // the x - PI(v/2 - 1/4) term not added to the
   // phase when we calculated it.
   //
   const T cx = cos(x);
   const T sx = sin(x);
   const T vd2shifted = (v / 2) - 0.25f;
   const T ci = cos_pi(vd2shifted, pol);
   const T si = sin_pi(vd2shifted, pol);
   const T sin_phase = sin(phase) * (cx * ci + sx * si) + cos(phase) * (sx * ci - cx * si);
   BOOST_MATH_INSTRUMENT_CODE(sin(phase));
   BOOST_MATH_INSTRUMENT_CODE(cos(x));
   BOOST_MATH_INSTRUMENT_CODE(cos(phase));
   BOOST_MATH_INSTRUMENT_CODE(sin(x));
   return sin_phase * ampl;
}

template <class T, class Policy>
inline T asymptotic_bessel_j_derivative_large_x_2(T v, T x, const Policy& pol)
{
   // See A&S 9.2.20.
   BOOST_MATH_STD_USING
   // Get the phase and amplitude:
   const T ampl = asymptotic_bessel_derivative_amplitude(v, x);
   const T phase = asymptotic_bessel_derivative_phase_mx(v, x);
   BOOST_MATH_INSTRUMENT_VARIABLE(ampl);
   BOOST_MATH_INSTRUMENT_VARIABLE(phase);
   //
   // Calculate the sine of the phase, using
   // sine/cosine addition rules to factor in
   // the x - PI(v/2 - 1/4) term not added to the
   // phase when we calculated it.
   //
   BOOST_MATH_INSTRUMENT_CODE(cos(phase));
   BOOST_MATH_INSTRUMENT_CODE(cos(x));
   BOOST_MATH_INSTRUMENT_CODE(sin(phase));
   BOOST_MATH_INSTRUMENT_CODE(sin(x));
   const T cx = cos(x);
   const T sx = sin(x);
   const T vd2shifted = (v / 2) - 0.25f;
   const T ci = cos_pi(vd2shifted, pol);
   const T si = sin_pi(vd2shifted, pol);
   const T sin_phase = cos(phase) * (cx * ci + sx * si) - sin(phase) * (sx * ci - cx * si);
   BOOST_MATH_INSTRUMENT_VARIABLE(sin_phase);
   return sin_phase * ampl;
}

template <class T>
inline bool asymptotic_bessel_derivative_large_x_limit(const T& v, const T& x)
{
   BOOST_MATH_STD_USING
   //
   // This function is the copy of math::asymptotic_bessel_large_x_limit
   // It means that we use the same rules for determining how x is large
   // compared to v.
   //
   // Determines if x is large enough compared to v to take the asymptotic
   // forms above.  From A&S 9.2.28 we require:
   //    v < x * eps^1/8
   // and from A&S 9.2.29 we require:
   //    v^12/10 < 1.5 * x * eps^1/10
   // using the former seems to work OK in practice with broadly similar
   // error rates either side of the divide for v < 10000.
   // At double precision eps^1/8 ~= 0.01.
   //
   return (std::max)(T(fabs(v)), T(1)) < x * sqrt(boost::math::tools::forth_root_epsilon<T>());
}

}}} // namespaces

#endif // BOOST_MATH_SF_DETAIL_BESSEL_JY_DERIVATIVES_ASYM_HPP

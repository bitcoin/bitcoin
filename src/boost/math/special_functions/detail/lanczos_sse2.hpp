//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS_SSE2
#define BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS_SSE2

#ifdef _MSC_VER
#pragma once
#endif

#include <emmintrin.h>

#if defined(__GNUC__) || defined(__PGI) || defined(__SUNPRO_CC)
#define ALIGN16 __attribute__((__aligned__(16)))
#else
#define ALIGN16 __declspec(align(16))
#endif

namespace boost{ namespace math{ namespace lanczos{

template <>
inline double lanczos13m53::lanczos_sum<double>(const double& x)
{
   static const ALIGN16 double coeff[26] = {
      static_cast<double>(2.506628274631000270164908177133837338626L),
      static_cast<double>(1u),
      static_cast<double>(210.8242777515793458725097339207133627117L),
      static_cast<double>(66u),
      static_cast<double>(8071.672002365816210638002902272250613822L),
      static_cast<double>(1925u),
      static_cast<double>(186056.2653952234950402949897160456992822L),
      static_cast<double>(32670u),
      static_cast<double>(2876370.628935372441225409051620849613599L),
      static_cast<double>(357423u),
      static_cast<double>(31426415.58540019438061423162831820536287L),
      static_cast<double>(2637558u),
      static_cast<double>(248874557.8620541565114603864132294232163L),
      static_cast<double>(13339535u),
      static_cast<double>(1439720407.311721673663223072794912393972L),
      static_cast<double>(45995730u),
      static_cast<double>(6039542586.35202800506429164430729792107L),
      static_cast<double>(105258076u),
      static_cast<double>(17921034426.03720969991975575445893111267L),
      static_cast<double>(150917976u),
      static_cast<double>(35711959237.35566804944018545154716670596L),
      static_cast<double>(120543840u),
      static_cast<double>(42919803642.64909876895789904700198885093L),
      static_cast<double>(39916800u),
      static_cast<double>(23531376880.41075968857200767445163675473L),
      static_cast<double>(0u)
   };

   static const double lim = 4.31965e+25; // By experiment, the largest x for which the SSE2 code does not go bad.

   if (x > lim)
   {
      double z = 1 / x;
      return ((((((((((((coeff[24] * z + coeff[22]) * z + coeff[20]) * z + coeff[18]) * z + coeff[16]) * z + coeff[14]) * z + coeff[12]) * z + coeff[10]) * z + coeff[8]) * z + coeff[6]) * z + coeff[4]) * z + coeff[2]) * z + coeff[0]) / ((((((((((((coeff[25] * z + coeff[23]) * z + coeff[21]) * z + coeff[19]) * z + coeff[17]) * z + coeff[15]) * z + coeff[13]) * z + coeff[11]) * z + coeff[9]) * z + coeff[7]) * z + coeff[5]) * z + coeff[3]) * z + coeff[1]);
   }

   __m128d vx = _mm_load1_pd(&x);
   __m128d sum_even = _mm_load_pd(coeff);
   __m128d sum_odd = _mm_load_pd(coeff+2);
   __m128d nc_odd, nc_even;
   __m128d vx2 = _mm_mul_pd(vx, vx);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 4);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 6);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 8);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 10);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 12);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 14);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 16);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 18);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 20);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 22);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 24);
   sum_odd = _mm_mul_pd(sum_odd, vx);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_even = _mm_add_pd(sum_even, sum_odd);


   double ALIGN16 t[2];
   _mm_store_pd(t, sum_even);
   
   return t[0] / t[1];
}

template <>
inline double lanczos13m53::lanczos_sum_expG_scaled<double>(const double& x)
{
   static const ALIGN16 double coeff[26] = {
         static_cast<double>(0.006061842346248906525783753964555936883222L),
         static_cast<double>(1u),
         static_cast<double>(0.5098416655656676188125178644804694509993L),
         static_cast<double>(66u),
         static_cast<double>(19.51992788247617482847860966235652136208L),
         static_cast<double>(1925u),
         static_cast<double>(449.9445569063168119446858607650988409623L),
         static_cast<double>(32670u),
         static_cast<double>(6955.999602515376140356310115515198987526L),
         static_cast<double>(357423u),
         static_cast<double>(75999.29304014542649875303443598909137092L),
         static_cast<double>(2637558u),
         static_cast<double>(601859.6171681098786670226533699352302507L),
         static_cast<double>(13339535u),
         static_cast<double>(3481712.15498064590882071018964774556468L),
         static_cast<double>(45995730u),
         static_cast<double>(14605578.08768506808414169982791359218571L),
         static_cast<double>(105258076u),
         static_cast<double>(43338889.32467613834773723740590533316085L),
         static_cast<double>(150917976u),
         static_cast<double>(86363131.28813859145546927288977868422342L),
         static_cast<double>(120543840u),
         static_cast<double>(103794043.1163445451906271053616070238554L),
         static_cast<double>(39916800u),
         static_cast<double>(56906521.91347156388090791033559122686859L),
         static_cast<double>(0u)
   };

   static const double lim = 4.76886e+25; // By experiment, the largest x for which the SSE2 code does not go bad.

   if (x > lim)
   {
      double z = 1 / x;
      return ((((((((((((coeff[24] * z + coeff[22]) * z + coeff[20]) * z + coeff[18]) * z + coeff[16]) * z + coeff[14]) * z + coeff[12]) * z + coeff[10]) * z + coeff[8]) * z + coeff[6]) * z + coeff[4]) * z + coeff[2]) * z + coeff[0]) / ((((((((((((coeff[25] * z + coeff[23]) * z + coeff[21]) * z + coeff[19]) * z + coeff[17]) * z + coeff[15]) * z + coeff[13]) * z + coeff[11]) * z + coeff[9]) * z + coeff[7]) * z + coeff[5]) * z + coeff[3]) * z + coeff[1]);
   }

   __m128d vx = _mm_load1_pd(&x);
   __m128d sum_even = _mm_load_pd(coeff);
   __m128d sum_odd = _mm_load_pd(coeff+2);
   __m128d nc_odd, nc_even;
   __m128d vx2 = _mm_mul_pd(vx, vx);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 4);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 6);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 8);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 10);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 12);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 14);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 16);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 18);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 20);
   sum_odd = _mm_mul_pd(sum_odd, vx2);
   nc_odd = _mm_load_pd(coeff + 22);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_odd = _mm_add_pd(sum_odd, nc_odd);

   sum_even = _mm_mul_pd(sum_even, vx2);
   nc_even = _mm_load_pd(coeff + 24);
   sum_odd = _mm_mul_pd(sum_odd, vx);
   sum_even = _mm_add_pd(sum_even, nc_even);
   sum_even = _mm_add_pd(sum_even, sum_odd);


   double ALIGN16 t[2];
   _mm_store_pd(t, sum_even);
   
   return t[0] / t[1];
}

#ifdef _MSC_VER

static_assert(sizeof(double) == sizeof(long double), "sizeof(long double) != sizeof(double) is not supported");

template <>
inline long double lanczos13m53::lanczos_sum<long double>(const long double& x)
{
   return lanczos_sum<double>(static_cast<double>(x));
}
template <>
inline long double lanczos13m53::lanczos_sum_expG_scaled<long double>(const long double& x)
{
   return lanczos_sum_expG_scaled<double>(static_cast<double>(x));
}
#endif

} // namespace lanczos
} // namespace math
} // namespace boost

#undef ALIGN16

#endif // BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS






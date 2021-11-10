//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_EXPINT_HPP
#define BOOST_MATH_EXPINT_HPP

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4702) // Unreachable code (release mode only warning)
#endif

#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/tools/fraction.hpp>
#include <boost/math/tools/series.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/pow.hpp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

namespace boost{ namespace math{

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint(unsigned n, T z, const Policy& /*pol*/);
   
namespace detail{

template <class T>
inline T expint_1_rational(const T& z, const std::integral_constant<int, 0>&)
{
   // this function is never actually called
   BOOST_MATH_ASSERT(0);
   return z;
}

template <class T>
T expint_1_rational(const T& z, const std::integral_constant<int, 53>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     2.006e-18
      // Expected Error Term:                         2.006e-18
      // Max error found at double precision:         2.760e-17
      static const T Y = 0.66373538970947265625F;
      static const T P[6] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0865197248079397976498),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0320913665303559189999),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.245088216639761496153),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0368031736257943745142),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00399167106081113256961),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.000111507792921197858394)
      };
      static const T Q[6] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.37091387659397013215),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.056770677104207528384),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00427347600017103698101),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.000131049900798434683324),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.528611029520217142048e-6)
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Maximum Deviation Found (interpolated):      1.444e-17
      // Max error found at double precision:         3.119e-17
      static const T P[11] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.121013190657725568138e-18),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.999999999999998811143),
         BOOST_MATH_BIG_CONSTANT(T, 53, -43.3058660811817946037),
         BOOST_MATH_BIG_CONSTANT(T, 53, -724.581482791462469795),
         BOOST_MATH_BIG_CONSTANT(T, 53, -6046.8250112711035463),
         BOOST_MATH_BIG_CONSTANT(T, 53, -27182.6254466733970467),
         BOOST_MATH_BIG_CONSTANT(T, 53, -66598.2652345418633509),
         BOOST_MATH_BIG_CONSTANT(T, 53, -86273.1567711649528784),
         BOOST_MATH_BIG_CONSTANT(T, 53, -54844.4587226402067411),
         BOOST_MATH_BIG_CONSTANT(T, 53, -14751.4895786128450662),
         BOOST_MATH_BIG_CONSTANT(T, 53, -1185.45720315201027667)
      };
      static const T Q[12] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 45.3058660811801465927),
         BOOST_MATH_BIG_CONSTANT(T, 53, 809.193214954550328455),
         BOOST_MATH_BIG_CONSTANT(T, 53, 7417.37624454689546708),
         BOOST_MATH_BIG_CONSTANT(T, 53, 38129.5594484818471461),
         BOOST_MATH_BIG_CONSTANT(T, 53, 113057.05869159631492),
         BOOST_MATH_BIG_CONSTANT(T, 53, 192104.047790227984431),
         BOOST_MATH_BIG_CONSTANT(T, 53, 180329.498380501819718),
         BOOST_MATH_BIG_CONSTANT(T, 53, 86722.3403467334749201),
         BOOST_MATH_BIG_CONSTANT(T, 53, 18455.4124737722049515),
         BOOST_MATH_BIG_CONSTANT(T, 53, 1229.20784182403048905),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.776491285282330997549)
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
T expint_1_rational(const T& z, const std::integral_constant<int, 64>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     3.807e-20
      // Expected Error Term:                         3.807e-20
      // Max error found at long double precision:    6.249e-20

      static const T Y = 0.66373538970947265625F;
      static const T P[6] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0865197248079397956816),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0275114007037026844633),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.246594388074877139824),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0237624819878732642231),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00259113319641673986276),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.30853660894346057053e-4)
      };
      static const T Q[7] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.317978365797784100273),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0393622602554758722511),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00204062029115966323229),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.732512107100088047854e-5),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.202872781770207871975e-5),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.52779248094603709945e-7)
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Maximum Deviation Found (interpolated):     2.220e-20
      // Max error found at long double precision:   1.346e-19
      static const T P[14] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.534401189080684443046e-23),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.999999999999999999905),
         BOOST_MATH_BIG_CONSTANT(T, 64, -62.1517806091379402505),
         BOOST_MATH_BIG_CONSTANT(T, 64, -1568.45688271895145277),
         BOOST_MATH_BIG_CONSTANT(T, 64, -21015.3431990874009619),
         BOOST_MATH_BIG_CONSTANT(T, 64, -164333.011755931661949),
         BOOST_MATH_BIG_CONSTANT(T, 64, -777917.270775426696103),
         BOOST_MATH_BIG_CONSTANT(T, 64, -2244188.56195255112937),
         BOOST_MATH_BIG_CONSTANT(T, 64, -3888702.98145335643429),
         BOOST_MATH_BIG_CONSTANT(T, 64, -3909822.65621952648353),
         BOOST_MATH_BIG_CONSTANT(T, 64, -2149033.9538897398457),
         BOOST_MATH_BIG_CONSTANT(T, 64, -584705.537139793925189),
         BOOST_MATH_BIG_CONSTANT(T, 64, -65815.2605361889477244),
         BOOST_MATH_BIG_CONSTANT(T, 64, -2038.82870680427258038)
      };
      static const T Q[14] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 64.1517806091379399478),
         BOOST_MATH_BIG_CONSTANT(T, 64, 1690.76044393722763785),
         BOOST_MATH_BIG_CONSTANT(T, 64, 24035.9534033068949426),
         BOOST_MATH_BIG_CONSTANT(T, 64, 203679.998633572361706),
         BOOST_MATH_BIG_CONSTANT(T, 64, 1074661.58459976978285),
         BOOST_MATH_BIG_CONSTANT(T, 64, 3586552.65020899358773),
         BOOST_MATH_BIG_CONSTANT(T, 64, 7552186.84989547621411),
         BOOST_MATH_BIG_CONSTANT(T, 64, 9853333.79353054111434),
         BOOST_MATH_BIG_CONSTANT(T, 64, 7689642.74550683631258),
         BOOST_MATH_BIG_CONSTANT(T, 64, 3385553.35146759180739),
         BOOST_MATH_BIG_CONSTANT(T, 64, 763218.072732396428725),
         BOOST_MATH_BIG_CONSTANT(T, 64, 73930.2995984054930821),
         BOOST_MATH_BIG_CONSTANT(T, 64, 2063.86994219629165937)
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
T expint_1_rational(const T& z, const std::integral_constant<int, 113>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     2.477e-35
      // Expected Error Term:                         2.477e-35
      // Max error found at long double precision:    6.810e-35

      static const T Y = 0.66373538970947265625F;
      static const T P[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0865197248079397956434879099175975937),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0369066175910795772830865304506087759),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.24272036838415474665971599314725545),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.0502166331248948515282379137550178307),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.00768384138547489410285101483730424919),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.000612574337702109683505224915484717162),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.380207107950635046971492617061708534e-4),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.136528159460768830763009294683628406e-5),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.346839106212658259681029388908658618e-7),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.340500302777838063940402160594523429e-9)
      };
      static const T Q[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.426568827778942588160423015589537302),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0841384046470893490592450881447510148),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0100557215850668029618957359471132995),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.000799334870474627021737357294799839363),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.434452090903862735242423068552687688e-4),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.15829674748799079874182885081231252e-5),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.354406206738023762100882270033082198e-7),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.369373328141051577845488477377890236e-9),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.274149801370933606409282434677600112e-12)
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z <= 4)
   {
      // Max error in interpolated form:             5.614e-35
      // Max error found at long double precision:   7.979e-35

      static const T Y = 0.70190334320068359375F;

      static const T P[16] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.298096656795020369955077350585959794),
         BOOST_MATH_BIG_CONSTANT(T, 113, 12.9314045995266142913135497455971247),
         BOOST_MATH_BIG_CONSTANT(T, 113, 226.144334921582637462526628217345501),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2070.83670924261732722117682067381405),
         BOOST_MATH_BIG_CONSTANT(T, 113, 10715.1115684330959908244769731347186),
         BOOST_MATH_BIG_CONSTANT(T, 113, 30728.7876355542048019664777316053311),
         BOOST_MATH_BIG_CONSTANT(T, 113, 38520.6078609349855436936232610875297),
         BOOST_MATH_BIG_CONSTANT(T, 113, -27606.0780981527583168728339620565165),
         BOOST_MATH_BIG_CONSTANT(T, 113, -169026.485055785605958655247592604835),
         BOOST_MATH_BIG_CONSTANT(T, 113, -254361.919204983608659069868035092282),
         BOOST_MATH_BIG_CONSTANT(T, 113, -195765.706874132267953259272028679935),
         BOOST_MATH_BIG_CONSTANT(T, 113, -83352.6826013533205474990119962408675),
         BOOST_MATH_BIG_CONSTANT(T, 113, -19251.6828496869586415162597993050194),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2226.64251774578542836725386936102339),
         BOOST_MATH_BIG_CONSTANT(T, 113, -109.009437301400845902228611986479816),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.51492042209561411434644938098833499)
      };
      static const T Q[16] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 113, 46.734521442032505570517810766704587),
         BOOST_MATH_BIG_CONSTANT(T, 113, 908.694714348462269000247450058595655),
         BOOST_MATH_BIG_CONSTANT(T, 113, 9701.76053033673927362784882748513195),
         BOOST_MATH_BIG_CONSTANT(T, 113, 63254.2815292641314236625196594947774),
         BOOST_MATH_BIG_CONSTANT(T, 113, 265115.641285880437335106541757711092),
         BOOST_MATH_BIG_CONSTANT(T, 113, 732707.841188071900498536533086567735),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1348514.02492635723327306628712057794),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1649986.81455283047769673308781585991),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1326000.828522976970116271208812099),
         BOOST_MATH_BIG_CONSTANT(T, 113, 683643.09490612171772350481773951341),
         BOOST_MATH_BIG_CONSTANT(T, 113, 217640.505137263607952365685653352229),
         BOOST_MATH_BIG_CONSTANT(T, 113, 40288.3467237411710881822569476155485),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3932.89353979531632559232883283175754),
         BOOST_MATH_BIG_CONSTANT(T, 113, 169.845369689596739824177412096477219),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.17607292280092201170768401876895354)
      };
      T recip = 1 / z;
      result = Y + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Max error in interpolated form:             4.413e-35
      // Max error found at long double precision:   8.928e-35

      static const T P[19] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.559148411832951463689610809550083986e-40),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.999999999999999999999999999999999997),
         BOOST_MATH_BIG_CONSTANT(T, 113, -166.542326331163836642960118190147367),
         BOOST_MATH_BIG_CONSTANT(T, 113, -12204.639128796330005065904675153652),
         BOOST_MATH_BIG_CONSTANT(T, 113, -520807.069767086071806275022036146855),
         BOOST_MATH_BIG_CONSTANT(T, 113, -14435981.5242137970691490903863125326),
         BOOST_MATH_BIG_CONSTANT(T, 113, -274574945.737064301247496460758654196),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3691611582.99810039356254671781473079),
         BOOST_MATH_BIG_CONSTANT(T, 113, -35622515944.8255047299363690814678763),
         BOOST_MATH_BIG_CONSTANT(T, 113, -248040014774.502043161750715548451142),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1243190389769.53458416330946622607913),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4441730126135.54739052731990368425339),
         BOOST_MATH_BIG_CONSTANT(T, 113, -11117043181899.7388524310281751971366),
         BOOST_MATH_BIG_CONSTANT(T, 113, -18976497615396.9717776601813519498961),
         BOOST_MATH_BIG_CONSTANT(T, 113, -21237496819711.1011661104761906067131),
         BOOST_MATH_BIG_CONSTANT(T, 113, -14695899122092.5161620333466757812848),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5737221535080.30569711574295785864903),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1077042281708.42654526404581272546244),
         BOOST_MATH_BIG_CONSTANT(T, 113, -68028222642.1941480871395695677675137)
      };
      static const T Q[20] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 113, 168.542326331163836642960118190147311),
         BOOST_MATH_BIG_CONSTANT(T, 113, 12535.7237814586576783518249115343619),
         BOOST_MATH_BIG_CONSTANT(T, 113, 544891.263372016404143120911148640627),
         BOOST_MATH_BIG_CONSTANT(T, 113, 15454474.7241010258634446523045237762),
         BOOST_MATH_BIG_CONSTANT(T, 113, 302495899.896629522673410325891717381),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4215565948.38886507646911672693270307),
         BOOST_MATH_BIG_CONSTANT(T, 113, 42552409471.7951815668506556705733344),
         BOOST_MATH_BIG_CONSTANT(T, 113, 313592377066.753173979584098301610186),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1688763640223.4541980740597514904542),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6610992294901.59589748057620192145704),
         BOOST_MATH_BIG_CONSTANT(T, 113, 18601637235659.6059890851321772682606),
         BOOST_MATH_BIG_CONSTANT(T, 113, 36944278231087.2571020964163402941583),
         BOOST_MATH_BIG_CONSTANT(T, 113, 50425858518481.7497071917028793820058),
         BOOST_MATH_BIG_CONSTANT(T, 113, 45508060902865.0899967797848815980644),
         BOOST_MATH_BIG_CONSTANT(T, 113, 25649955002765.3817331501988304758142),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8259575619094.6518520988612711292331),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1299981487496.12607474362723586264515),
         BOOST_MATH_BIG_CONSTANT(T, 113, 70242279152.8241187845178443118302693),
         BOOST_MATH_BIG_CONSTANT(T, 113, -37633302.9409263839042721539363416685)
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
struct expint_fraction
{
   typedef std::pair<T,T> result_type;
   expint_fraction(unsigned n_, T z_) : b(n_ + z_), i(-1), n(n_){}
   std::pair<T,T> operator()()
   {
      std::pair<T,T> result = std::make_pair(-static_cast<T>((i+1) * (n+i)), b);
      b += 2;
      ++i;
      return result;
   }
private:
   T b;
   int i;
   unsigned n;
};

template <class T, class Policy>
inline T expint_as_fraction(unsigned n, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   BOOST_MATH_INSTRUMENT_VARIABLE(z)
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   expint_fraction<T> f(n, z);
   T result = tools::continued_fraction_b(
      f, 
      boost::math::policies::get_epsilon<T, Policy>(),
      max_iter);
   policies::check_series_iterations<T>("boost::math::expint_continued_fraction<%1%>(unsigned,%1%)", max_iter, pol);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   BOOST_MATH_INSTRUMENT_VARIABLE(max_iter)
   result = exp(-z) / result;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   return result;
}

template <class T>
struct expint_series
{
   typedef T result_type;
   expint_series(unsigned k_, T z_, T x_k_, T denom_, T fact_) 
      : k(k_), z(z_), x_k(x_k_), denom(denom_), fact(fact_){}
   T operator()()
   {
      x_k *= -z;
      denom += 1;
      fact *= ++k;
      return x_k / (denom * fact);
   }
private:
   unsigned k;
   T z;
   T x_k;
   T denom;
   T fact;
};

template <class T, class Policy>
inline T expint_as_series(unsigned n, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();

   BOOST_MATH_INSTRUMENT_VARIABLE(z)

   T result = 0;
   T x_k = -1;
   T denom = T(1) - n;
   T fact = 1;
   unsigned k = 0;
   for(; k < n - 1;)
   {
      result += x_k / (denom * fact);
      denom += 1;
      x_k *= -z;
      fact *= ++k;
   }
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += pow(-z, static_cast<T>(n - 1)) 
      * (boost::math::digamma(static_cast<T>(n), pol) - log(z)) / fact;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)

   expint_series<T> s(k, z, x_k, denom, fact);
   result = tools::sum_series(s, policies::get_epsilon<T, Policy>(), max_iter, result);
   policies::check_series_iterations<T>("boost::math::expint_series<%1%>(unsigned,%1%)", max_iter, pol);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   BOOST_MATH_INSTRUMENT_VARIABLE(max_iter)
   return result;
}

template <class T, class Policy, class Tag>
T expint_imp(unsigned n, T z, const Policy& pol, const Tag& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(unsigned, %1%)";
   if(z < 0)
      return policies::raise_domain_error<T>(function, "Function requires z >= 0 but got %1%.", z, pol);
   if(z == 0)
      return n == 1 ? policies::raise_overflow_error<T>(function, 0, pol) : T(1 / (static_cast<T>(n - 1)));

   T result;

   bool f;
   if(n < 3)
   {
      f = z < 0.5;
   }
   else
   {
      f = z < (static_cast<T>(n - 2) / static_cast<T>(n - 1));
   }
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4127) // conditional expression is constant
#endif
   if(n == 0)
      result = exp(-z) / z;
   else if((n == 1) && (Tag::value))
   {
      result = expint_1_rational(z, tag);
   }
   else if(f)
      result = expint_as_series(n, z, pol);
   else
      result = expint_as_fraction(n, z, pol);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

   return result;
}

template <class T>
struct expint_i_series
{
   typedef T result_type;
   expint_i_series(T z_) : k(0), z_k(1), z(z_){}
   T operator()()
   {
      z_k *= z / ++k;
      return z_k / k;
   }
private:
   unsigned k;
   T z_k;
   T z;
};

template <class T, class Policy>
T expint_i_as_series(T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   T result = log(z); // (log(z) - log(1 / z)) / 2;
   result += constants::euler<T>();
   expint_i_series<T> s(z);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   result = tools::sum_series(s, policies::get_epsilon<T, Policy>(), max_iter, result);
   policies::check_series_iterations<T>("boost::math::expint_i_series<%1%>(%1%)", max_iter, pol);
   return result;
}

template <class T, class Policy, class Tag>
T expint_i_imp(T z, const Policy& pol, const Tag& tag)
{
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, T(-z), pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);
   return expint_i_as_series(z, pol);
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const std::integral_constant<int, 53>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, T(-z), pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      // Maximum Deviation Found:                     2.852e-18
      // Expected Error Term:                         2.852e-18
      // Max Error found at double precision =        Poly: 2.636335e-16   Cheb: 4.187027e-16
      static const T P[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 2.98677224343598593013),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.356343618769377415068),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.780836076283730801839),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.114670926327032002811),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0499434773576515260534),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00726224593341228159561),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00115478237227804306827),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.000116419523609765200999),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.798296365679269702435e-5),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.2777056254402008721e-6)
      };
      static const T Q[8] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, -1.17090412365413911947),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.62215109846016746276),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.195114782069495403315),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0391523431392967238166),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00504800158663705747345),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.000389034007436065401822),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.138972589601781706598e-4)
      };

      static const T c1 = BOOST_MATH_BIG_CONSTANT(T, 53, 1677624236387711.0);
      static const T c2 = BOOST_MATH_BIG_CONSTANT(T, 53, 4503599627370496.0);
      static const T r1 = static_cast<T>(c1 / c2);
      static const T r2 = BOOST_MATH_BIG_CONSTANT(T, 53, 0.131401834143860282009280387409357165515556574352422001206362e-16);
      static const T r = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 53, 0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392));
      T t = (z / 3) - 1;
      result = tools::evaluate_polynomial(P, t) 
         / tools::evaluate_polynomial(Q, t);
      t = (z - r1) - r2;
      result *= t;
      if(fabs(t) < 0.1)
      {
         result += boost::math::log1p(t / r, pol);
      }
      else
      {
         result += log(z / r);
      }
   }
   else if (z <= 10)
   {
      // Maximum Deviation Found:                     6.546e-17
      // Expected Error Term:                         6.546e-17
      // Max Error found at double precision =        Poly: 6.890169e-17   Cheb: 6.772128e-17
      static const T Y = 1.158985137939453125F;
      static const T P[8] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00139324086199402804173),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0349921221823888744966),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0264095520754134848538),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00761224003005476438412),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00247496209592143627977),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.000374885917942100256775),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.554086272024881826253e-4),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.396487648924804510056e-5)
      };
      static const T Q[8] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.744625566823272107711),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.329061095011767059236),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.100128624977313872323),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0223851099128506347278),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00365334190742316650106),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.000402453408512476836472),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.263649630720255691787e-4)
      };
      T t = z / 2 - 4;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 20)
   {
      // Maximum Deviation Found:                     1.843e-17
      // Expected Error Term:                         -1.842e-17
      // Max Error found at double precision =        Poly: 4.375868e-17   Cheb: 5.860967e-17

      static const T Y = 1.0869731903076171875F;
      static const T P[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00893891094356945667451),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0484607730127134045806),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0652810444222236895772),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0478447572647309671455),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0226059218923777094596),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00720603636917482065907),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00155941947035972031334),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.000209750022660200888349),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.138652200349182596186e-4)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.97017214039061194971),
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.86232465043073157508),
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.09601437090337519977),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.438873285773088870812),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.122537731979686102756),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0233458478275769288159),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00278170769163303669021),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.000159150281166108755531)
      };
      T t = z / 5 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 40)
   {
      // Maximum Deviation Found:                     5.102e-18
      // Expected Error Term:                         5.101e-18
      // Max Error found at double precision =        Poly: 1.441088e-16   Cheb: 1.864792e-16


      static const T Y = 1.03937530517578125F;
      static const T P[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00356165148914447597995),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0229930320357982333406),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0449814350482277917716),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0453759383048193402336),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0272050837209380717069),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00994403059883350813295),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.00207592267812291726961),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.000192178045857733706044),
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.113161784705911400295e-9)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 2.84354408840148561131),
         BOOST_MATH_BIG_CONSTANT(T, 53, 3.6599610090072393012),
         BOOST_MATH_BIG_CONSTANT(T, 53, 2.75088464344293083595),
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.2985244073998398643),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.383213198510794507409),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.0651165455496281337831),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.00488071077519227853585)
      };
      T t = z / 10 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else
   {
      // Max Error found at double precision =        3.381886e-17
      static const T exp40 = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 53, 2.35385266837019985407899910749034804508871617254555467236651e17));
      static const T Y= 1.013065338134765625F;
      static const T P[6] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, -0.0130653381347656243849),
         BOOST_MATH_BIG_CONSTANT(T, 53, 0.19029710559486576682),
         BOOST_MATH_BIG_CONSTANT(T, 53, 94.7365094537197236011),
         BOOST_MATH_BIG_CONSTANT(T, 53, -2516.35323679844256203),
         BOOST_MATH_BIG_CONSTANT(T, 53, 18932.0850014925993025),
         BOOST_MATH_BIG_CONSTANT(T, 53, -38703.1431362056714134)
      };
      static const T Q[7] = {    
         BOOST_MATH_BIG_CONSTANT(T, 53, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 53, 61.9733592849439884145),
         BOOST_MATH_BIG_CONSTANT(T, 53, -2354.56211323420194283),
         BOOST_MATH_BIG_CONSTANT(T, 53, 22329.1459489893079041),
         BOOST_MATH_BIG_CONSTANT(T, 53, -70126.245140396567133),
         BOOST_MATH_BIG_CONSTANT(T, 53, 54738.2833147775537106),
         BOOST_MATH_BIG_CONSTANT(T, 53, 8297.16296356518409347)
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const std::integral_constant<int, 64>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, T(-z), pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      // Maximum Deviation Found:                     3.883e-21
      // Expected Error Term:                         3.883e-21
      // Max Error found at long double precision =   Poly: 3.344801e-19   Cheb: 4.989937e-19

      static const T P[11] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 2.98677224343598593764),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.25891613550886736592),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.789323584998672832285),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.092432587824602399339),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0514236978728625906656),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00658477469745132977921),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00124914538197086254233),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.000131429679565472408551),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.11293331317982763165e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.629499283139417444244e-6),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.177833045143692498221e-7)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, -1.20352377969742325748),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.66707904942606479811),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.223014531629140771914),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0493340022262908008636),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00741934273050807310677),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00074353567782087939294),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.455861727069603367656e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.131515429329812837701e-5)
      };

      static const T c1 = BOOST_MATH_BIG_CONSTANT(T, 64, 1677624236387711.0);
      static const T c2 = BOOST_MATH_BIG_CONSTANT(T, 64, 4503599627370496.0);
      static const T r1 = c1 / c2;
      static const T r2 = BOOST_MATH_BIG_CONSTANT(T, 64, 0.131401834143860282009280387409357165515556574352422001206362e-16);
      static const T r = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392));
      T t = (z / 3) - 1;
      result = tools::evaluate_polynomial(P, t) 
         / tools::evaluate_polynomial(Q, t);
      t = (z - r1) - r2;
      result *= t;
      if(fabs(t) < 0.1)
      {
         result += boost::math::log1p(t / r, pol);
      }
      else
      {
         result += log(z / r);
      }
   }
   else if (z <= 10)
   {
      // Maximum Deviation Found:                     2.622e-21
      // Expected Error Term:                         -2.622e-21
      // Max Error found at long double precision =   Poly: 1.208328e-20   Cheb: 1.073723e-20

      static const T Y = 1.158985137939453125F;
      static const T P[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00139324086199409049399),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0345238388952337563247),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0382065278072592940767),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0156117003070560727392),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00383276012430495387102),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.000697070540945496497992),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.877310384591205930343e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.623067256376494930067e-5),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.377246883283337141444e-6)
      };
      static const T Q[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.08073635708902053767),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.553681133533942532909),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.176763647137553797451),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0387891748253869928121),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0060603004848394727017),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.000670519492939992806051),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.4947357050100855646e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.204339282037446434827e-5),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.146951181174930425744e-7)
      };
      T t = z / 2 - 4;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 20)
   {
      // Maximum Deviation Found:                     3.220e-20
      // Expected Error Term:                         3.220e-20
      // Max Error found at long double precision =   Poly: 7.696841e-20   Cheb: 6.205163e-20


      static const T Y = 1.0869731903076171875F;
      static const T P[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00893891094356946995368),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0487562980088748775943),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0670568657950041926085),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0509577352851442932713),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.02551800927409034206),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00892913759760086687083),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00224469630207344379888),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.000392477245911296982776),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.44424044184395578775e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.252788029251437017959e-5)
      };
      static const T Q[10] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 2.00323265503572414261),
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.94688958187256383178),
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.19733638134417472296),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.513137726038353385661),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.159135395578007264547),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0358233587351620919881),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0056716655597009417875),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.000577048986213535829925),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.290976943033493216793e-4)
      };
      T t = z / 5 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 40)
   {
      // Maximum Deviation Found:                     2.940e-21
      // Expected Error Term:                         -2.938e-21
      // Max Error found at long double precision =   Poly: 3.419893e-19   Cheb: 3.359874e-19

      static const T Y = 1.03937530517578125F;
      static const T P[12] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00356165148914447278177),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0240235006148610849678),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0516699967278057976119),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0586603078706856245674),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0409960120868776180825),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0185485073689590665153),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.00537842101034123222417),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.000920988084778273760609),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.716742618812210980263e-4),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.504623302166487346677e-9),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.712662196671896837736e-10),
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.533769629702262072175e-11)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 3.13286733695729715455),
         BOOST_MATH_BIG_CONSTANT(T, 64, 4.49281223045653491929),
         BOOST_MATH_BIG_CONSTANT(T, 64, 3.84900294427622911374),
         BOOST_MATH_BIG_CONSTANT(T, 64, 2.15205199043580378211),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.802912186540269232424),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.194793170017818925388),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.0280128013584653182994),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.00182034930799902922549)
      };
      T t = z / 10 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else
   {
      // Maximum Deviation Found:                     3.536e-20
      // Max Error found at long double precision =   Poly: 1.310671e-19   Cheb: 8.630943e-11

      static const T exp40 = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.35385266837019985407899910749034804508871617254555467236651e17));
      static const T Y= 1.013065338134765625F;
      static const T P[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, -0.0130653381347656250004),
         BOOST_MATH_BIG_CONSTANT(T, 64, 0.644487780349757303739),
         BOOST_MATH_BIG_CONSTANT(T, 64, 143.995670348227433964),
         BOOST_MATH_BIG_CONSTANT(T, 64, -13918.9322758014173709),
         BOOST_MATH_BIG_CONSTANT(T, 64, 476260.975133624194484),
         BOOST_MATH_BIG_CONSTANT(T, 64, -7437102.15135982802122),
         BOOST_MATH_BIG_CONSTANT(T, 64, 53732298.8764767916542),
         BOOST_MATH_BIG_CONSTANT(T, 64, -160695051.957997452509),
         BOOST_MATH_BIG_CONSTANT(T, 64, 137839271.592778020028)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 64, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 64, 27.2103343964943718802),
         BOOST_MATH_BIG_CONSTANT(T, 64, -8785.48528692879413676),
         BOOST_MATH_BIG_CONSTANT(T, 64, 397530.290000322626766),
         BOOST_MATH_BIG_CONSTANT(T, 64, -7356441.34957799368252),
         BOOST_MATH_BIG_CONSTANT(T, 64, 63050914.5343400957524),
         BOOST_MATH_BIG_CONSTANT(T, 64, -246143779.638307701369),
         BOOST_MATH_BIG_CONSTANT(T, 64, 384647824.678554961174),
         BOOST_MATH_BIG_CONSTANT(T, 64, -166288297.874583961493)
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy>
void expint_i_imp_113a(T& result, const T& z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     1.230e-36
   // Expected Error Term:                         -1.230e-36
   // Max Error found at long double precision =   Poly: 4.355299e-34   Cheb: 7.512581e-34


   static const T P[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.98677224343598593765287235997328555),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.333256034674702967028780537349334037),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.851831522798101228384971644036708463),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0657854833494646206186773614110374948),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0630065662557284456000060708977935073),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00311759191425309373327784154659649232),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00176213568201493949664478471656026771),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.491548660404172089488535218163952295e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.207764227621061706075562107748176592e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.225445398156913584846374273379402765e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.996939977231410319761273881672601592e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.212546902052178643330520878928100847e-9),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.154646053060262871360159325115980023e-9),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.143971277122049197323415503594302307e-11),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.306243138978114692252817805327426657e-13)
   };
   static const T Q[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, -1.40178870313943798705491944989231793),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.943810968269701047641218856758605284),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.405026631534345064600850391026113165),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.123924153524614086482627660399122762),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0286364505373369439591132549624317707),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00516148845910606985396596845494015963),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000738330799456364820380739850924783649),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.843737760991856114061953265870882637e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.767957673431982543213661388914587589e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.549136847313854595809952100614840031e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.299801381513743676764008325949325404e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.118419479055346106118129130945423483e-8),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.30372295663095470359211949045344607e-10),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.382742953753485333207877784720070523e-12)
   };

   static const T c1 = BOOST_MATH_BIG_CONSTANT(T, 113, 1677624236387711.0);
   static const T c2 = BOOST_MATH_BIG_CONSTANT(T, 113, 4503599627370496.0);
   static const T c3 = BOOST_MATH_BIG_CONSTANT(T, 113, 266514582277687.0);
   static const T c4 = BOOST_MATH_BIG_CONSTANT(T, 113, 4503599627370496.0);
   static const T c5 = BOOST_MATH_BIG_CONSTANT(T, 113, 4503599627370496.0);
   static const T r1 = c1 / c2;
   static const T r2 = c3 / c4 / c5;
   static const T r3 = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.283806480836357377069325311780969887585024578164571984232357e-31));
   static const T r = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392));
   T t = (z / 3) - 1;
   result = tools::evaluate_polynomial(P, t) 
      / tools::evaluate_polynomial(Q, t);
   t = ((z - r1) - r2) - r3;
   result *= t;
   if(fabs(t) < 0.1)
   {
      result += boost::math::log1p(t / r, pol);
   }
   else
   {
      result += log(z / r);
   }
}

template <class T>
void expint_i_113b(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     7.779e-36
   // Expected Error Term:                         -7.779e-36
   // Max Error found at long double precision =   Poly: 2.576723e-35   Cheb: 1.236001e-34

   static const T Y = 1.158985137939453125F;
   static const T P[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00139324086199409049282472239613554817),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0338173111691991289178779840307998955),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0555972290794371306259684845277620556),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0378677976003456171563136909186202177),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0152221583517528358782902783914356667),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00428283334203873035104248217403126905),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000922782631491644846511553601323435286),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000155513428088853161562660696055496696),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.205756580255359882813545261519317096e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.220327406578552089820753181821115181e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.189483157545587592043421445645377439e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.122426571518570587750898968123803867e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.635187358949437991465353268374523944e-9),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.203015132965870311935118337194860863e-10),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.384276705503357655108096065452950822e-12)
   };
   static const T Q[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.58784732785354597996617046880946257),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.18550755302279446339364262338114098),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.55598993549661368604527040349702836),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.184290888380564236919107835030984453),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0459658051803613282360464632326866113),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0089505064268613225167835599456014705),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00139042673882987693424772855926289077),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.000174210708041584097450805790176479012),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.176324034009707558089086875136647376e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.142935845999505649273084545313710581e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.907502324487057260675816233312747784e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.431044337808893270797934621235918418e-8),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.139007266881450521776529705677086902e-9),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.234715286125516430792452741830364672e-11)
   };
   T t = z / 2 - 4;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   result *= exp(z) / z;
   result += z;
}

template <class T>
void expint_i_113c(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     1.082e-34
   // Expected Error Term:                         1.080e-34
   // Max Error found at long double precision =   Poly: 1.958294e-34   Cheb: 2.472261e-34


   static const T Y = 1.091579437255859375F;
   static const T P[17] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00685089599550151282724924894258520532),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0443313550253580053324487059748497467),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.071538561252424027443296958795814874),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0622923153354102682285444067843300583),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0361631270264607478205393775461208794),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0153192826839624850298106509601033261),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00496967904961260031539602977748408242),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00126989079663425780800919171538920589),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000258933143097125199914724875206326698),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.422110326689204794443002330541441956e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.546004547590412661451073996127115221e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.546775260262202177131068692199272241e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.404157632825805803833379568956559215e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.200612596196561323832327013027419284e-8),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.502538501472133913417609379765434153e-10),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.326283053716799774936661568391296584e-13),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.869226483473172853557775877908693647e-15)
   };
   static const T Q[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.23227220874479061894038229141871087),
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.40221000361027971895657505660959863),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.65476320985936174728238416007084214),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.816828602963895720369875535001248227),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.306337922909446903672123418670921066),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0902400121654409267774593230720600752),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0212708882169429206498765100993228086),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00404442626252467471957713495828165491),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0006195601618842253612635241404054589),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.755930932686543009521454653994321843e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.716004532773778954193609582677482803e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.500881663076471627699290821742924233e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.233593219218823384508105943657387644e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.554900353169148897444104962034267682e-9)
   };
   T t = z / 4 - 3.5;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   result *= exp(z) / z;
   result += z;
}

template <class T>
void expint_i_113d(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     3.163e-35
   // Expected Error Term:                         3.163e-35
   // Max Error found at long double precision =   Poly: 4.158110e-35   Cheb: 5.385532e-35

   static const T Y = 1.051731109619140625F;
   static const T P[14] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00144552494420652573815404828020593565),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0126747451594545338365684731262912741),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.01757394877502366717526779263438073),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0126838952395506921945756139424722588),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0060045057928894974954756789352443522),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00205349237147226126653803455793107903),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000532606040579654887676082220195624207),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000107344687098019891474772069139014662),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.169536802705805811859089949943435152e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.20863311729206543881826553010120078e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.195670358542116256713560296776654385e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.133291168587253145439184028259772437e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.595500337089495614285777067722823397e-9),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.133141358866324100955927979606981328e-10)
   };
   static const T Q[14] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.72490783907582654629537013560044682),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.44524329516800613088375685659759765),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.778241785539308257585068744978050181),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.300520486589206605184097270225725584),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0879346899691339661394537806057953957),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0200802415843802892793583043470125006),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00362842049172586254520256100538273214),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.000519731362862955132062751246769469957),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.584092147914050999895178697392282665e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.501851497707855358002773398333542337e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.313085677467921096644895738538865537e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.127552010539733113371132321521204458e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.25737310826983451144405899970774587e-9)
   };
   T t = z / 4 - 5.5;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result *= exp(z) / z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
}

template <class T>
void expint_i_113e(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     7.972e-36
   // Expected Error Term:                         7.962e-36
   // Max Error found at long double precision =   Poly: 1.711721e-34   Cheb: 3.100018e-34

   static const T Y = 1.032726287841796875F;
   static const T P[15] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00141056919297307534690895009969373233),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0123384175302540291339020257071411437),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0298127270706864057791526083667396115),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0390686759471630584626293670260768098),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0338226792912607409822059922949035589),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0211659736179834946452561197559654582),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0100428887460879377373158821400070313),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00370717396015165148484022792801682932),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0010768667551001624764329000496561659),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000246127328761027039347584096573123531),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.437318110527818613580613051861991198e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.587532682329299591501065482317771497e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.565697065670893984610852937110819467e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.350233957364028523971768887437839573e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.105428907085424234504608142258423505e-8)
   };
   static const T Q[16] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 3.17261315255467581204685605414005525),
      BOOST_MATH_BIG_CONSTANT(T, 113, 4.85267952971640525245338392887217426),
      BOOST_MATH_BIG_CONSTANT(T, 113, 4.74341914912439861451492872946725151),
      BOOST_MATH_BIG_CONSTANT(T, 113, 3.31108463283559911602405970817931801),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.74657006336994649386607925179848899),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.718255607416072737965933040353653244),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.234037553177354542791975767960643864),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0607470145906491602476833515412605389),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0125048143774226921434854172947548724),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00201034366420433762935768458656609163),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.000244823338417452367656368849303165721),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.213511655166983177960471085462540807e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.119323998465870686327170541547982932e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.322153582559488797803027773591727565e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.161635525318683508633792845159942312e-16)
   };
   T t = z / 8 - 4.25;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result *= exp(z) / z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
}

template <class T>
void expint_i_113f(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     4.469e-36
   // Expected Error Term:                         4.468e-36
   // Max Error found at long double precision =   Poly: 1.288958e-35   Cheb: 2.304586e-35

   static const T Y = 1.0216197967529296875F;
   static const T P[12] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000322999116096627043476023926572650045),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00385606067447365187909164609294113346),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00686514524727568176735949971985244415),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00606260649593050194602676772589601799),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00334382362017147544335054575436194357),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00126108534260253075708625583630318043),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000337881489347846058951220431209276776),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.648480902304640018785370650254018022e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.87652644082970492211455290209092766e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.794712243338068631557849449519994144e-6),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.434084023639508143975983454830954835e-7),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.107839681938752337160494412638656696e-8)
   };
   static const T Q[12] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.09913805456661084097134805151524958),
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.07041755535439919593503171320431849),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.26406517226052371320416108604874734),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.529689923703770353961553223973435569),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.159578150879536711042269658656115746),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0351720877642000691155202082629857131),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.00565313621289648752407123620997063122),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.000646920278540515480093843570291218295),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.499904084850091676776993523323213591e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.233740058688179614344680531486267142e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.498800627828842754845418576305379469e-7)
   };
   T t = z / 7 - 7;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result *= exp(z) / z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
}

template <class T>
void expint_i_113g(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     5.588e-35
   // Expected Error Term:                         -5.566e-35
   // Max Error found at long double precision =   Poly: 9.976345e-35   Cheb: 8.358865e-35

   static const T Y = 1.015148162841796875F;
   static const T P[11] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000435714784725086961464589957142615216),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00432114324353830636009453048419094314),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0100740363285526177522819204820582424),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0116744115827059174392383504427640362),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00816145387784261141360062395898644652),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00371380272673500791322744465394211508),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.00112958263488611536502153195005736563),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.000228316462389404645183269923754256664),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.29462181955852860250359064291292577e-4),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.21972450610957417963227028788460299e-5),
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.720558173805289167524715527536874694e-7)
   };
   static const T Q[11] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, 2.95918362458402597039366979529287095),
      BOOST_MATH_BIG_CONSTANT(T, 113, 3.96472247520659077944638411856748924),
      BOOST_MATH_BIG_CONSTANT(T, 113, 3.15563251550528513747923714884142131),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.64674612007093983894215359287448334),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.58695020129846594405856226787156424),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.144358385319329396231755457772362793),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.024146911506411684815134916238348063),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.0026257132337460784266874572001650153),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.000167479843750859222348869769094711093),
      BOOST_MATH_BIG_CONSTANT(T, 113, 0.475673638665358075556452220192497036e-5)
   };
   T t = z / 14 - 5;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result *= exp(z) / z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += z;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
}

template <class T>
void expint_i_113h(T& result, const T& z)
{
   BOOST_MATH_STD_USING
   // Maximum Deviation Found:                     4.448e-36
   // Expected Error Term:                         4.445e-36
   // Max Error found at long double precision =   Poly: 2.058532e-35   Cheb: 2.165465e-27

   static const T Y= 1.00849151611328125F;
   static const T P[9] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, -0.0084915161132812500000001440233607358),
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.84479378737716028341394223076147872),
      BOOST_MATH_BIG_CONSTANT(T, 113, -130.431146923726715674081563022115568),
      BOOST_MATH_BIG_CONSTANT(T, 113, 4336.26945491571504885214176203512015),
      BOOST_MATH_BIG_CONSTANT(T, 113, -76279.0031974974730095170437591004177),
      BOOST_MATH_BIG_CONSTANT(T, 113, 729577.956271997673695191455111727774),
      BOOST_MATH_BIG_CONSTANT(T, 113, -3661928.69330208734947103004900349266),
      BOOST_MATH_BIG_CONSTANT(T, 113, 8570600.041606912735872059184527855),
      BOOST_MATH_BIG_CONSTANT(T, 113, -6758379.93672362080947905580906028645)
   };
   static const T Q[10] = {    
      BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
      BOOST_MATH_BIG_CONSTANT(T, 113, -99.4868026047611434569541483506091713),
      BOOST_MATH_BIG_CONSTANT(T, 113, 3879.67753690517114249705089803055473),
      BOOST_MATH_BIG_CONSTANT(T, 113, -76495.82413252517165830203774900806),
      BOOST_MATH_BIG_CONSTANT(T, 113, 820773.726408311894342553758526282667),
      BOOST_MATH_BIG_CONSTANT(T, 113, -4803087.64956923577571031564909646579),
      BOOST_MATH_BIG_CONSTANT(T, 113, 14521246.227703545012713173740895477),
      BOOST_MATH_BIG_CONSTANT(T, 113, -19762752.0196769712258527849159393044),
      BOOST_MATH_BIG_CONSTANT(T, 113, 8354144.67882768405803322344185185517),
      BOOST_MATH_BIG_CONSTANT(T, 113, 355076.853106511136734454134915432571)
   };
   T t = 1 / z;
   result = Y + tools::evaluate_polynomial(P, t)
      / tools::evaluate_polynomial(Q, t);
   result *= exp(z) / z;
   result += z;
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const std::integral_constant<int, 113>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, T(-z), pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      expint_i_imp_113a(result, z, pol);
   }
   else if (z <= 10)
   {
      expint_i_113b(result, z);
   }
   else if(z <= 18)
   {
      expint_i_113c(result, z);
   }
   else if(z <= 26)
   {
      expint_i_113d(result, z);
   }
   else if(z <= 42)
   {
      expint_i_113e(result, z);
   }
   else if(z <= 56)
   {
      expint_i_113f(result, z);
   }
   else if(z <= 84)
   {
      expint_i_113g(result, z);
   }
   else if(z <= 210)
   {
      expint_i_113h(result, z);
   }
   else // z > 210
   {
      // Maximum Deviation Found:                     3.963e-37
      // Expected Error Term:                         3.963e-37
      // Max Error found at long double precision =   Poly: 1.248049e-36   Cheb: 2.843486e-29

      static const T exp40 = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2.35385266837019985407899910749034804508871617254555467236651e17));
      static const T Y= 1.00252532958984375F;
      static const T P[8] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.00252532958984375000000000000000000085),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.16591386866059087390621952073890359),
         BOOST_MATH_BIG_CONSTANT(T, 113, -67.8483431314018462417456828499277579),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1567.68688154683822956359536287575892),
         BOOST_MATH_BIG_CONSTANT(T, 113, -17335.4683325819116482498725687644986),
         BOOST_MATH_BIG_CONSTANT(T, 113, 93632.6567462673524739954389166550069),
         BOOST_MATH_BIG_CONSTANT(T, 113, -225025.189335919133214440347510936787),
         BOOST_MATH_BIG_CONSTANT(T, 113, 175864.614717440010942804684741336853)
      };
      static const T Q[9] = {    
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 113, -65.6998869881600212224652719706425129),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1642.73850032324014781607859416890077),
         BOOST_MATH_BIG_CONSTANT(T, 113, -19937.2610222467322481947237312818575),
         BOOST_MATH_BIG_CONSTANT(T, 113, 124136.267326632742667972126625064538),
         BOOST_MATH_BIG_CONSTANT(T, 113, -384614.251466704550678760562965502293),
         BOOST_MATH_BIG_CONSTANT(T, 113, 523355.035910385688578278384032026998),
         BOOST_MATH_BIG_CONSTANT(T, 113, -217809.552260834025885677791936351294),
         BOOST_MATH_BIG_CONSTANT(T, 113, -8555.81719551123640677261226549550872)
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy, class tag>
struct expint_i_initializer
{
   struct init
   {
      init()
      {
         do_init(tag());
      }
      static void do_init(const std::integral_constant<int, 0>&){}
      static void do_init(const std::integral_constant<int, 53>&)
      {
         boost::math::expint(T(5), Policy());
         boost::math::expint(T(7), Policy());
         boost::math::expint(T(18), Policy());
         boost::math::expint(T(38), Policy());
         boost::math::expint(T(45), Policy());
      }
      static void do_init(const std::integral_constant<int, 64>&)
      {
         boost::math::expint(T(5), Policy());
         boost::math::expint(T(7), Policy());
         boost::math::expint(T(18), Policy());
         boost::math::expint(T(38), Policy());
         boost::math::expint(T(45), Policy());
      }
      static void do_init(const std::integral_constant<int, 113>&)
      {
         boost::math::expint(T(5), Policy());
         boost::math::expint(T(7), Policy());
         boost::math::expint(T(17), Policy());
         boost::math::expint(T(25), Policy());
         boost::math::expint(T(40), Policy());
         boost::math::expint(T(50), Policy());
         boost::math::expint(T(80), Policy());
         boost::math::expint(T(200), Policy());
         boost::math::expint(T(220), Policy());
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class Policy, class tag>
const typename expint_i_initializer<T, Policy, tag>::init expint_i_initializer<T, Policy, tag>::initializer;

template <class T, class Policy, class tag>
struct expint_1_initializer
{
   struct init
   {
      init()
      {
         do_init(tag());
      }
      static void do_init(const std::integral_constant<int, 0>&){}
      static void do_init(const std::integral_constant<int, 53>&)
      {
         boost::math::expint(1, T(0.5), Policy());
         boost::math::expint(1, T(2), Policy());
      }
      static void do_init(const std::integral_constant<int, 64>&)
      {
         boost::math::expint(1, T(0.5), Policy());
         boost::math::expint(1, T(2), Policy());
      }
      static void do_init(const std::integral_constant<int, 113>&)
      {
         boost::math::expint(1, T(0.5), Policy());
         boost::math::expint(1, T(2), Policy());
         boost::math::expint(1, T(6), Policy());
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class Policy, class tag>
const typename expint_1_initializer<T, Policy, tag>::init expint_1_initializer<T, Policy, tag>::initializer;

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint_forwarder(T z, const Policy& /*pol*/, std::true_type const&)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   typedef std::integral_constant<int,
      precision_type::value <= 0 ? 0 :
      precision_type::value <= 53 ? 53 :
      precision_type::value <= 64 ? 64 :
      precision_type::value <= 113 ? 113 : 0
   > tag_type;

   expint_i_initializer<value_type, forwarding_policy, tag_type>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::expint_i_imp(
      static_cast<value_type>(z),
      forwarding_policy(),
      tag_type()), "boost::math::expint<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type
expint_forwarder(unsigned n, T z, const std::false_type&)
{
   return boost::math::expint(n, z, policies::policy<>());
}

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint(unsigned n, T z, const Policy& /*pol*/)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   typedef std::integral_constant<int,
      precision_type::value <= 0 ? 0 :
      precision_type::value <= 53 ? 53 :
      precision_type::value <= 64 ? 64 :
      precision_type::value <= 113 ? 113 : 0
   > tag_type;

   detail::expint_1_initializer<value_type, forwarding_policy, tag_type>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::expint_imp(
      n,
      static_cast<value_type>(z),
      forwarding_policy(),
      tag_type()), "boost::math::expint<%1%>(unsigned, %1%)");
}

template <class T, class U>
inline typename detail::expint_result<T, U>::type
   expint(T const z, U const u)
{
   typedef typename policies::is_policy<U>::type tag_type;
   return detail::expint_forwarder(z, u, tag_type());
}

template <class T>
inline typename tools::promote_args<T>::type
   expint(T z)
{
   return expint(z, policies::policy<>());
}

}} // namespaces

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // BOOST_MATH_EXPINT_HPP



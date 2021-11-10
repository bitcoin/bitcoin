//  Copyright (c) 2017 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Modified Bessel function of the first kind of order zero
// we use the approximating forms derived in:
// "Rational Approximations for the Modified Bessel Function of the First Kind - I1(x) for Computations with Double Precision"
// by Pavel Holoborodko, 
// see http://www.advanpix.com/2015/11/12/rational-approximations-for-the-modified-bessel-function-of-the-first-kind-i1-for-computations-with-double-precision/
// The actual coefficients used are our own, and extend Pavel's work to precision's other than double.

#ifndef BOOST_MATH_BESSEL_I1_HPP
#define BOOST_MATH_BESSEL_I1_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/rational.hpp>
#include <boost/math/tools/big_constant.hpp>
#include <boost/math/tools/assert.hpp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

// Modified Bessel function of the first kind of order one
// minimax rational approximations on intervals, see
// Blair and Edwards, Chalk River Report AECL-4928, 1974

namespace boost { namespace math { namespace detail{

template <typename T>
T bessel_i1(const T& x);

template <class T, class tag>
struct bessel_i1_initializer
{
   struct init
   {
      init()
      {
         do_init(tag());
      }
      static void do_init(const std::integral_constant<int, 64>&)
      {
         bessel_i1(T(1));
         bessel_i1(T(15));
         bessel_i1(T(80));
         bessel_i1(T(101));
      }
      static void do_init(const std::integral_constant<int, 113>&)
      {
         bessel_i1(T(1));
         bessel_i1(T(10));
         bessel_i1(T(14));
         bessel_i1(T(19));
         bessel_i1(T(34));
         bessel_i1(T(99));
         bessel_i1(T(101));
      }
      template <class U>
      static void do_init(const U&) {}
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class tag>
const typename bessel_i1_initializer<T, tag>::init bessel_i1_initializer<T, tag>::initializer;

template <typename T, int N>
T bessel_i1_imp(const T&, const std::integral_constant<int, N>&)
{
   BOOST_MATH_ASSERT(0);
   return 0;
}

template <typename T>
T bessel_i1_imp(const T& x, const std::integral_constant<int, 24>&)
{
   BOOST_MATH_STD_USING
      if(x < 7.75)
      {
         //Max error in interpolated form : 1.348e-08
         // Max Error found at float precision = Poly : 1.469121e-07
         static const float P[] = {
            8.333333221e-02f,
            6.944453712e-03f,
            3.472097211e-04f,
            1.158047174e-05f,
            2.739745142e-07f,
            5.135884609e-09f,
            5.262251502e-11f,
            1.331933703e-12f
         };
         T a = x * x / 4;
         T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
         return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
      }
      else
      {
         // Max error in interpolated form: 9.000e-08
         // Max Error found at float precision = Poly: 1.044345e-07

         static const float P[] = {
            3.98942115977513013e-01f,
            -1.49581264836620262e-01f,
            -4.76475741878486795e-02f,
            -2.65157315524784407e-02f,
            -1.47148600683672014e-01f
         };
         T ex = exp(x / 2);
         T result = ex * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
         result *= ex;
         return result;
      }
}

template <typename T>
T bessel_i1_imp(const T& x, const std::integral_constant<int, 53>&)
{
   BOOST_MATH_STD_USING
   if(x < 7.75)
   {
      // Bessel I0 over[10 ^ -16, 7.75]
      // Max error in interpolated form: 5.639e-17
      // Max Error found at double precision = Poly: 1.795559e-16

      static const double P[] = {
         8.333333333333333803e-02,
         6.944444444444341983e-03,
         3.472222222225921045e-04,
         1.157407407354987232e-05,
         2.755731926254790268e-07,
         4.920949692800671435e-09,
         6.834657311305621830e-11,
         7.593969849687574339e-13,
         6.904822652741917551e-15,
         5.220157095351373194e-17,
         3.410720494727771276e-19,
         1.625212890947171108e-21,
         1.332898928162290861e-23
      };
      T a = x * x / 4;
      T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
      return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
   }
   else if(x < 500)
   {
      // Max error in interpolated form: 1.796e-16
      // Max Error found at double precision = Poly: 2.898731e-16

      static const double P[] = {
         3.989422804014406054e-01,
         -1.496033551613111533e-01,
         -4.675104253598537322e-02,
         -4.090895951581637791e-02,
         -5.719036414430205390e-02,
         -1.528189554374492735e-01,
         3.458284470977172076e+00,
         -2.426181371595021021e+02,
         1.178785865993440669e+04,
         -4.404655582443487334e+05,
         1.277677779341446497e+07,
         -2.903390398236656519e+08,
         5.192386898222206474e+09,
         -7.313784438967834057e+10,
         8.087824484994859552e+11,
         -6.967602516005787001e+12,
         4.614040809616582764e+13,
         -2.298849639457172489e+14,
         8.325554073334618015e+14,
         -2.067285045778906105e+15,
         3.146401654361325073e+15,
         -2.213318202179221945e+15
      };
      return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
   }
   else
   {
      // Max error in interpolated form: 1.320e-19
      // Max Error found at double precision = Poly: 7.065357e-17
      static const double P[] = {
         3.989422804014314820e-01,
         -1.496033551467584157e-01,
         -4.675105322571775911e-02,
         -4.090421597376992892e-02,
         -5.843630344778927582e-02
      };
      T ex = exp(x / 2);
      T result = ex * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
      result *= ex;
      return result;
   }
}

template <typename T>
T bessel_i1_imp(const T& x, const std::integral_constant<int, 64>&)
{
   BOOST_MATH_STD_USING
      if(x < 7.75)
      {
         // Bessel I0 over[10 ^ -16, 7.75]
         // Max error in interpolated form: 8.086e-21
         // Max Error found at float80 precision = Poly: 7.225090e-20
         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 64, 8.33333333333333333340071817e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, 6.94444444444444442462728070e-03),
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.47222222222222318886683883e-04),
            BOOST_MATH_BIG_CONSTANT(T, 64, 1.15740740740738880709555060e-05),
            BOOST_MATH_BIG_CONSTANT(T, 64, 2.75573192240046222242685145e-07),
            BOOST_MATH_BIG_CONSTANT(T, 64, 4.92094986131253986838697503e-09),
            BOOST_MATH_BIG_CONSTANT(T, 64, 6.83465258979924922633502182e-11),
            BOOST_MATH_BIG_CONSTANT(T, 64, 7.59405830675154933645967137e-13),
            BOOST_MATH_BIG_CONSTANT(T, 64, 6.90369179710633344508897178e-15),
            BOOST_MATH_BIG_CONSTANT(T, 64, 5.23003610041709452814262671e-17),
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.35291901027762552549170038e-19),
            BOOST_MATH_BIG_CONSTANT(T, 64, 1.83991379419781823063672109e-21),
            BOOST_MATH_BIG_CONSTANT(T, 64, 8.87732714140192556332037815e-24),
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.32120654663773147206454247e-26),
            BOOST_MATH_BIG_CONSTANT(T, 64, 1.95294659305369207813486871e-28) 
         };
         T a = x * x / 4;
         T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
         return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
      }
      else if(x < 20)
      {
         // Max error in interpolated form: 4.258e-20
         // Max Error found at float80 precision = Poly: 2.851105e-19
         // Maximum Deviation Found : 3.887e-20
         // Expected Error Term : 3.887e-20
         // Maximum Relative Change in Control Points : 1.681e-04
         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.98942260530218897338680e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.49599542849073670179540e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.70492865454119188276875e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -3.12389893307392002405869e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, 1.49696126385202602071197e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -3.84206507612717711565967e+01),
            BOOST_MATH_BIG_CONSTANT(T, 64, 2.14748094784412558689584e+03),
            BOOST_MATH_BIG_CONSTANT(T, 64, -7.70652726663596993005669e+04),
            BOOST_MATH_BIG_CONSTANT(T, 64, 2.01659736164815617174439e+06),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.04740659606466305607544e+07),
            BOOST_MATH_BIG_CONSTANT(T, 64, 6.38383394696382837263656e+08),
            BOOST_MATH_BIG_CONSTANT(T, 64, -8.00779638649147623107378e+09),
            BOOST_MATH_BIG_CONSTANT(T, 64, 8.02338237858684714480491e+10),
            BOOST_MATH_BIG_CONSTANT(T, 64, -6.41198553664947312995879e+11),
            BOOST_MATH_BIG_CONSTANT(T, 64, 4.05915186909564986897554e+12),
            BOOST_MATH_BIG_CONSTANT(T, 64, -2.00907636964168581116181e+13),
            BOOST_MATH_BIG_CONSTANT(T, 64, 7.60855263982359981275199e+13),
            BOOST_MATH_BIG_CONSTANT(T, 64, -2.12901817219239205393806e+14),
            BOOST_MATH_BIG_CONSTANT(T, 64, 4.14861794397709807823575e+14),
            BOOST_MATH_BIG_CONSTANT(T, 64, -5.02808138522587680348583e+14),
            BOOST_MATH_BIG_CONSTANT(T, 64, 2.85505477056514919387171e+14)
         };
         return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
      }
      else if(x < 100)
      {
         // Bessel I0 over [15, 50]
         // Maximum Deviation Found:                     2.444e-20
         // Expected Error Term : 2.438e-20
         // Maximum Relative Change in Control Points : 2.101e-03
         // Max Error found at float80 precision = Poly : 6.029974e-20

         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.98942280401431675205845e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.49603355149968887210170e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.67510486284376330257260e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.09071458907089270559464e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -5.75278280327696940044714e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.10591299500956620739254e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -2.77061766699949309115618e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -5.42683771801837596371638e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -9.17021412070404158464316e+00),
            BOOST_MATH_BIG_CONSTANT(T, 64, 1.04154379346763380543310e+02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.43462345357478348323006e+03),
            BOOST_MATH_BIG_CONSTANT(T, 64, 9.98109660274422449523837e+03),
            BOOST_MATH_BIG_CONSTANT(T, 64, -3.74438822767781410362757e+04)
         };
         return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
      }
      else
      {
         // Bessel I0 over[100, INF]
         // Max error in interpolated form: 2.456e-20
         // Max Error found at float80 precision = Poly: 5.446356e-20
         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 64, 3.98942280401432677958445e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.49603355150537411254359e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.67510484842456251368526e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -4.09071676503922479645155e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -5.75256179814881566010606e-02),
            BOOST_MATH_BIG_CONSTANT(T, 64, -1.10754910257965227825040e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -2.67858639515616079840294e-01),
            BOOST_MATH_BIG_CONSTANT(T, 64, -9.17266479586791298924367e-01)
         };
         T ex = exp(x / 2);
         T result = ex * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
         result *= ex;
         return result;
      }
}

template <typename T>
T bessel_i1_imp(const T& x, const std::integral_constant<int, 113>&)
{
   BOOST_MATH_STD_USING
   if(x < 7.75)
   {
      // Bessel I0 over[10 ^ -34, 7.75]
      // Max error in interpolated form: 1.835e-35
      // Max Error found at float128 precision = Poly: 1.645036e-34

      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.3333333333333333333333333333333331804098e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.9444444444444444444444444444445418303082e-03),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.4722222222222222222222222222119082346591e-04),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.1574074074074074074074074078415867655987e-05),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.7557319223985890652557318255143448192453e-07),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.9209498614260519022423916850415000626427e-09),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.8346525853139609753354247043900442393686e-11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 7.5940584281266233060080535940234144302217e-13),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.9036894801151120925605467963949641957095e-15),
         BOOST_MATH_BIG_CONSTANT(T, 113, 5.2300677879659941472662086395055636394839e-17),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.3526075563884539394691458717439115962233e-19),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.8420920639497841692288943167036233338434e-21),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.7718669711748690065381181691546032291365e-24),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.6549445715236427401845636880769861424730e-26),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.3437296196812697924703896979250126739676e-28),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.3912734588619073883015937023564978854893e-31),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.2839967682792395867255384448052781306897e-33),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.3790094235693528861015312806394354114982e-36),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.0423861671932104308662362292359563970482e-39),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.7493858979396446292135661268130281652945e-41),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.2786079392547776769387921361408303035537e-44),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.2335693685833531118863552173880047183822e-47)
      };
      T a = x * x / 4;
      T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
      return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
   }
   else if(x < 11)
   {
      // Max error in interpolated form: 8.574e-36
      // Maximum Deviation Found : 4.689e-36
      // Expected Error Term : 3.760e-36
      // Maximum Relative Change in Control Points : 5.204e-03
      // Max Error found at float128 precision = Poly : 2.882561e-34

      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.333333333333333326889717360850080939e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.944444444444444511272790848815114507e-03),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.472222222222221892451965054394153443e-04),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.157407407407408437378868534321538798e-05),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.755731922398566216824909767320161880e-07),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.920949861426434829568192525456800388e-09),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.834652585308926245465686943255486934e-11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 7.594058428179852047689599244015979196e-13),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.903689479655006062822949671528763738e-15),
         BOOST_MATH_BIG_CONSTANT(T, 113, 5.230067791254403974475987777406992984e-17),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.352607536815161679702105115200693346e-19),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.842092161364672561828681848278567885e-21),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.771862912600611801856514076709932773e-24),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.654958704184380914803366733193713605e-26),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.343688672071130980471207297730607625e-28),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.392252844664709532905868749753463950e-31),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.282086786672692641959912811902298600e-33),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.408812012322547015191398229942864809e-36),
         BOOST_MATH_BIG_CONSTANT(T, 113, 7.681220437734066258673404589233009892e-39),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.072417451640733785626701738789290055e-41),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.352218520142636864158849446833681038e-44),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.407918492276267527897751358794783640e-46)
      };
      T a = x * x / 4;
      T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
      return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
   }
   else if(x < 15)
   {
      //Max error in interpolated form: 7.599e-36
      // Maximum Deviation Found : 1.766e-35
      // Expected Error Term : 1.021e-35
      // Maximum Relative Change in Control Points : 6.228e-03
      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.333333333333255774414858563409941233e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.944444444444897867884955912228700291e-03),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.472222222220954970397343617150959467e-04),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.157407407409660682751155024932538578e-05),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.755731922369973706427272809014190998e-07),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.920949861702265600960449699129258153e-09),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.834652583208361401197752793379677147e-11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 7.594058441128280500819776168239988143e-13),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.903689413939268702265479276217647209e-15),
         BOOST_MATH_BIG_CONSTANT(T, 113, 5.230068069012898202890718644753625569e-17),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.352606552027491657204243201021677257e-19),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.842095100698532984651921750204843362e-21),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.771789051329870174925649852681844169e-24),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.655114381199979536997025497438385062e-26),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.343415732516712339472538688374589373e-28),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.396177019032432392793591204647901390e-31),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.277563309255167951005939802771456315e-33),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.449201419305514579791370198046544736e-36),
         BOOST_MATH_BIG_CONSTANT(T, 113, 7.415430703400740634202379012388035255e-39),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.195458831864936225409005027914934499e-41),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.829726762743879793396637797534668039e-45),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.698302711685624490806751012380215488e-46),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.062520475425422618494185821587228317e-49),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.732372906742845717148185173723304360e-52)
      };
      T a = x * x / 4;
      T Q[3] = { 1, 0.5f, boost::math::tools::evaluate_polynomial(P, a) };
      return x * boost::math::tools::evaluate_polynomial(Q, a) / 2;
   }
   else if(x < 20)
   {
      // Max error in interpolated form: 8.864e-36
      // Max Error found at float128 precision = Poly: 8.522841e-35
      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.989422793693152031514179994954750043e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.496029423752889591425633234009799670e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.682975926820553021482820043377990241e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3.138871171577224532369979905856458929e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -8.765350219426341341990447005798111212e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, 5.321389275507714530941178258122955540e+01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.727748393898888756515271847678850411e+03),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.123040820686242586086564998713862335e+05),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3.784112378374753535335272752884808068e+06),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.054920416060932189433079126269416563e+08),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.450129415468060676827180524327749553e+09),
         BOOST_MATH_BIG_CONSTANT(T, 113, 4.758831882046487398739784498047935515e+10),
         BOOST_MATH_BIG_CONSTANT(T, 113, -7.736936520262204842199620784338052937e+11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.051128683324042629513978256179115439e+13),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.188008285959794869092624343537262342e+14),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.108530004906954627420484180793165669e+15),
         BOOST_MATH_BIG_CONSTANT(T, 113, -8.441516828490144766650287123765318484e+15),
         BOOST_MATH_BIG_CONSTANT(T, 113, 5.158251664797753450664499268756393535e+16),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.467314522709016832128790443932896401e+17),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.896222045367960462945885220710294075e+17),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.273382139594876997203657902425653079e+18),
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.669871448568623680543943144842394531e+18),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.813923031370708069940575240509912588e+18)
      };
      return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
   }
   else if(x < 35)
   {
      // Max error in interpolated form: 6.028e-35
      // Max Error found at float128 precision = Poly: 1.368313e-34

      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.989422804012941975429616956496046931e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.496033550576049830976679315420681402e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.675107835141866009896710750800622147e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.090104965125365961928716504473692957e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5.842241652296980863361375208605487570e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.063604828033747303936724279018650633e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -9.113375972811586130949401996332817152e+00),
         BOOST_MATH_BIG_CONSTANT(T, 113, 6.334748570425075872639817839399823709e+02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3.759150758768733692594821032784124765e+04),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.863672813448915255286274382558526321e+06),
         BOOST_MATH_BIG_CONSTANT(T, 113, -7.798248643371718775489178767529282534e+07),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.769963173932801026451013022000669267e+09),
         BOOST_MATH_BIG_CONSTANT(T, 113, -8.381780137198278741566746511015220011e+10),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.163891337116820832871382141011952931e+12),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.764325864671438675151635117936912390e+13),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.925668307403332887856809510525154955e+14),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.416692606589060039334938090985713641e+16),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.892398600219306424294729851605944429e+17),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.107232903741874160308537145391245060e+18),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.930223393531877588898224144054112045e+19),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.427759576167665663373350433236061007e+20),
         BOOST_MATH_BIG_CONSTANT(T, 113, 8.306019279465532835530812122374386654e+20),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3.653753000392125229440044977239174472e+21),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.140760686989511568435076842569804906e+22),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.249149337812510200795436107962504749e+22),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.101619088427348382058085685849420866e+22)
      };
      return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
   }
   else if(x < 100)
   {
      // Max error in interpolated form: 5.494e-35
      // Max Error found at float128 precision = Poly: 1.214651e-34

      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.989422804014326779399307367861631577e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.496033551505372542086590873271571919e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.675104848454290286276466276677172664e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.090716742397105403027549796269213215e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5.752570419098513588311026680089351230e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.107369803696534592906420980901195808e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.699214194000085622941721628134575121e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -7.953006169077813678478720427604462133e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.746618809476524091493444128605380593e+00),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.084446249943196826652788161656973391e+01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5.020325182518980633783194648285500554e+01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.510195971266257573425196228564489134e+02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5.241661863814900938075696173192225056e+03),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.323374362891993686413568398575539777e+05),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.112838452096066633754042734723911040e+06),
         BOOST_MATH_BIG_CONSTANT(T, 113, 9.369270194978310081563767560113534023e+07),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.704295412488936504389347368131134993e+09),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.320829576277038198439987439508754886e+10),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.258818139077875493434420764260185306e+11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.396791306321498426110315039064592443e+12),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.217617301585849875301440316301068439e+12)
      };
      return exp(x) * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
   }
   else
   {
      // Bessel I0 over[100, INF]
      // Max error in interpolated form: 6.081e-35
      // Max Error found at float128 precision = Poly: 1.407151e-34
      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 3.9894228040143267793994605993438200208417e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.4960335515053725422747977247811372936584e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.6751048484542891946087411826356811991039e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.0907167423975030452875828826630006305665e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -5.7525704189964886494791082898669060345483e-02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.1073698056568248642163476807108190176386e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.6992139012879749064623499618582631684228e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -7.9530409594026597988098934027440110587905e-01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.7462844478733532517044536719240098183686e+00),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.0870711340681926669381449306654104739256e+01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -4.8510175413216969245241059608553222505228e+01),
         BOOST_MATH_BIG_CONSTANT(T, 113, -2.4094682286011573747064907919522894740063e+02),
         BOOST_MATH_BIG_CONSTANT(T, 113, -1.3128845936764406865199641778959502795443e+03),
         BOOST_MATH_BIG_CONSTANT(T, 113, -8.1655901321962541203257516341266838487359e+03),
         BOOST_MATH_BIG_CONSTANT(T, 113, -3.8019591025686295090160445920753823994556e+04),
         BOOST_MATH_BIG_CONSTANT(T, 113, -6.7008089049178178697338128837158732831105e+05)
      };
      T ex = exp(x / 2);
      T result = ex * boost::math::tools::evaluate_polynomial(P, T(1 / x)) / sqrt(x);
      result *= ex;
      return result;
   }
}

template <typename T>
T bessel_i1_imp(const T& x, const std::integral_constant<int, 0>&)
{
   if(boost::math::tools::digits<T>() <= 24)
      return bessel_i1_imp(x, std::integral_constant<int, 24>());
   else if(boost::math::tools::digits<T>() <= 53)
      return bessel_i1_imp(x, std::integral_constant<int, 53>());
   else if(boost::math::tools::digits<T>() <= 64)
      return bessel_i1_imp(x, std::integral_constant<int, 64>());
   else if(boost::math::tools::digits<T>() <= 113)
      return bessel_i1_imp(x, std::integral_constant<int, 113>());
   BOOST_MATH_ASSERT(0);
   return 0;
}

template <typename T>
inline T bessel_i1(const T& x)
{
   typedef std::integral_constant<int,
      ((std::numeric_limits<T>::digits == 0) || (std::numeric_limits<T>::radix != 2)) ?
      0 :
      std::numeric_limits<T>::digits <= 24 ?
      24 :
      std::numeric_limits<T>::digits <= 53 ?
      53 :
      std::numeric_limits<T>::digits <= 64 ?
      64 :
      std::numeric_limits<T>::digits <= 113 ?
      113 : -1
   > tag_type;

   bessel_i1_initializer<T, tag_type>::force_instantiate();
   return bessel_i1_imp(x, tag_type());
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_I1_HPP


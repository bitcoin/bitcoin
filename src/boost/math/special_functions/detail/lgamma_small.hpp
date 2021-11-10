//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_FUNCTIONS_DETAIL_LGAMMA_SMALL
#define BOOST_MATH_SPECIAL_FUNCTIONS_DETAIL_LGAMMA_SMALL

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/big_constant.hpp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

namespace boost{ namespace math{ namespace detail{

//
// These need forward declaring to keep GCC happy:
//
template <class T, class Policy, class Lanczos>
T gamma_imp(T z, const Policy& pol, const Lanczos& l);
template <class T, class Policy>
T gamma_imp(T z, const Policy& pol, const lanczos::undefined_lanczos& l);

//
// lgamma for small arguments:
//
template <class T, class Policy, class Lanczos>
T lgamma_small_imp(T z, T zm1, T zm2, const std::integral_constant<int, 64>&, const Policy& /* l */, const Lanczos&)
{
   // This version uses rational approximations for small
   // values of z accurate enough for 64-bit mantissas
   // (80-bit long doubles), works well for 53-bit doubles as well.
   // Lanczos is only used to select the Lanczos function.

   BOOST_MATH_STD_USING  // for ADL of std names
   T result = 0;
   if(z < tools::epsilon<T>())
   {
      result = -log(z);
   }
   else if((zm1 == 0) || (zm2 == 0))
   {
      // nothing to do, result is zero....
   }
   else if(z > 2)
   {
      //
      // Begin by performing argument reduction until
      // z is in [2,3):
      //
      if(z >= 3)
      {
         do
         {
            z -= 1;
            zm2 -= 1;
            result += log(z);
         }while(z >= 3);
         // Update zm2, we need it below:
         zm2 = z - 2;
      }

      //
      // Use the following form:
      //
      // lgamma(z) = (z-2)(z+1)(Y + R(z-2))
      //
      // where R(z-2) is a rational approximation optimised for
      // low absolute error - as long as it's absolute error
      // is small compared to the constant Y - then any rounding
      // error in it's computation will get wiped out.
      //
      // R(z-2) has the following properties:
      //
      // At double: Max error found:                    4.231e-18
      // At long double: Max error found:               1.987e-21
      // Maximum Deviation Found (approximation error): 5.900e-24
      //
      static const T P[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.180355685678449379109e-1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.25126649619989678683e-1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.494103151567532234274e-1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.172491608709613993966e-1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.259453563205438108893e-3)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.541009869215204396339e-3)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.324588649825948492091e-4))
      };
      static const T Q[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1e1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.196202987197795200688e1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.148019669424231326694e1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.541391432071720958364e0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.988504251128010129477e-1)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.82130967464889339326e-2)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.224936291922115757597e-3)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.223352763208617092964e-6))
      };

      static const float Y = 0.158963680267333984375e0f;

      T r = zm2 * (z + 1);
      T R = tools::evaluate_polynomial(P, zm2);
      R /= tools::evaluate_polynomial(Q, zm2);

      result +=  r * Y + r * R;
   }
   else
   {
      //
      // If z is less than 1 use recurrence to shift to
      // z in the interval [1,2]:
      //
      if(z < 1)
      {
         result += -log(z);
         zm2 = zm1;
         zm1 = z;
         z += 1;
      }
      //
      // Two approximations, on for z in [1,1.5] and
      // one for z in [1.5,2]:
      //
      if(z <= 1.5)
      {
         //
         // Use the following form:
         //
         // lgamma(z) = (z-1)(z-2)(Y + R(z-1))
         //
         // where R(z-1) is a rational approximation optimised for
         // low absolute error - as long as it's absolute error
         // is small compared to the constant Y - then any rounding
         // error in it's computation will get wiped out.
         //
         // R(z-1) has the following properties:
         //
         // At double precision: Max error found:                1.230011e-17
         // At 80-bit long double precision:   Max error found:  5.631355e-21
         // Maximum Deviation Found:                             3.139e-021
         // Expected Error Term:                                 3.139e-021

         //
         static const float Y = 0.52815341949462890625f;

         static const T P[] = {
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.490622454069039543534e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.969117530159521214579e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.414983358359495381969e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.406567124211938417342e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.158413586390692192217e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.240149820648571559892e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.100346687696279557415e-2))
         };
         static const T Q[] = {
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.302349829846463038743e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.348739585360723852576e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.191415588274426679201e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.507137738614363510846e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.577039722690451849648e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.195768102601107189171e-2))
         };

         T r = tools::evaluate_polynomial(P, zm1) / tools::evaluate_polynomial(Q, zm1);
         T prefix = zm1 * zm2;

         result += prefix * Y + prefix * r;
      }
      else
      {
         //
         // Use the following form:
         //
         // lgamma(z) = (2-z)(1-z)(Y + R(2-z))
         //
         // where R(2-z) is a rational approximation optimised for
         // low absolute error - as long as it's absolute error
         // is small compared to the constant Y - then any rounding
         // error in it's computation will get wiped out.
         //
         // R(2-z) has the following properties:
         //
         // At double precision, max error found:              1.797565e-17
         // At 80-bit long double precision, max error found:  9.306419e-21
         // Maximum Deviation Found:                           2.151e-021
         // Expected Error Term:                               2.150e-021
         //
         static const float Y = 0.452017307281494140625f;

         static const T P[] = {
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.292329721830270012337e-1)), 
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.144216267757192309184e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.142440390738631274135e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.542809694055053558157e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.850535976868336437746e-2)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.431171342679297331241e-3))
         };
         static const T Q[] = {
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.150169356054485044494e1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.846973248876495016101e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.220095151814995745555e0)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.25582797155975869989e-1)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.100666795539143372762e-2)),
            static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.827193521891290553639e-6))
         };
         T r = zm2 * zm1;
         T R = tools::evaluate_polynomial(P, T(-zm2)) / tools::evaluate_polynomial(Q, T(-zm2));

         result += r * Y + r * R;
      }
   }
   return result;
}
template <class T, class Policy, class Lanczos>
T lgamma_small_imp(T z, T zm1, T zm2, const std::integral_constant<int, 113>&, const Policy& /* l */, const Lanczos&)
{
   //
   // This version uses rational approximations for small
   // values of z accurate enough for 113-bit mantissas
   // (128-bit long doubles).
   //
   BOOST_MATH_STD_USING  // for ADL of std names
   T result = 0;
   if(z < tools::epsilon<T>())
   {
      result = -log(z);
      BOOST_MATH_INSTRUMENT_CODE(result);
   }
   else if((zm1 == 0) || (zm2 == 0))
   {
      // nothing to do, result is zero....
   }
   else if(z > 2)
   {
      //
      // Begin by performing argument reduction until
      // z is in [2,3):
      //
      if(z >= 3)
      {
         do
         {
            z -= 1;
            result += log(z);
         }while(z >= 3);
         zm2 = z - 2;
      }
      BOOST_MATH_INSTRUMENT_CODE(zm2);
      BOOST_MATH_INSTRUMENT_CODE(z);
      BOOST_MATH_INSTRUMENT_CODE(result);

      //
      // Use the following form:
      //
      // lgamma(z) = (z-2)(z+1)(Y + R(z-2))
      //
      // where R(z-2) is a rational approximation optimised for
      // low absolute error - as long as it's absolute error
      // is small compared to the constant Y - then any rounding
      // error in it's computation will get wiped out.
      //
      // Maximum Deviation Found (approximation error)      3.73e-37

      static const T P[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.018035568567844937910504030027467476655),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.013841458273109517271750705401202404195),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.062031842739486600078866923383017722399),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.052518418329052161202007865149435256093),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.01881718142472784129191838493267755758),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0025104830367021839316463675028524702846),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.00021043176101831873281848891452678568311),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.00010249622350908722793327719494037981166),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.11381479670982006841716879074288176994e-4),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.49999811718089980992888533630523892389e-6),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.70529798686542184668416911331718963364e-8)
      };
      static const T Q[] = {
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.5877485070422317542808137697939233685),
         BOOST_MATH_BIG_CONSTANT(T, 113, 2.8797959228352591788629602533153837126),
         BOOST_MATH_BIG_CONSTANT(T, 113, 1.8030885955284082026405495275461180977),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.69774331297747390169238306148355428436),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.17261566063277623942044077039756583802),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.02729301254544230229429621192443000121),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.0026776425891195270663133581960016620433),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.00015244249160486584591370355730402168106),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.43997034032479866020546814475414346627e-5),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.46295080708455613044541885534408170934e-7),
         BOOST_MATH_BIG_CONSTANT(T, 113, -0.93326638207459533682980757982834180952e-11),
         BOOST_MATH_BIG_CONSTANT(T, 113, 0.42316456553164995177177407325292867513e-13)
      };

      T R = tools::evaluate_polynomial(P, zm2);
      R /= tools::evaluate_polynomial(Q, zm2);

      static const float Y = 0.158963680267333984375F;

      T r = zm2 * (z + 1);

      result +=  r * Y + r * R;
      BOOST_MATH_INSTRUMENT_CODE(result);
   }
   else
   {
      //
      // If z is less than 1 use recurrence to shift to
      // z in the interval [1,2]:
      //
      if(z < 1)
      {
         result += -log(z);
         zm2 = zm1;
         zm1 = z;
         z += 1;
      }
      BOOST_MATH_INSTRUMENT_CODE(result);
      BOOST_MATH_INSTRUMENT_CODE(z);
      BOOST_MATH_INSTRUMENT_CODE(zm2);
      //
      // Three approximations, on for z in [1,1.35], [1.35,1.625] and [1.625,1]
      //
      if(z <= 1.35)
      {
         //
         // Use the following form:
         //
         // lgamma(z) = (z-1)(z-2)(Y + R(z-1))
         //
         // where R(z-1) is a rational approximation optimised for
         // low absolute error - as long as it's absolute error
         // is small compared to the constant Y - then any rounding
         // error in it's computation will get wiped out.
         //
         // R(z-1) has the following properties:
         //
         // Maximum Deviation Found (approximation error)            1.659e-36
         // Expected Error Term (theoretical error)                  1.343e-36
         // Max error found at 128-bit long double precision         1.007e-35
         //
         static const float Y = 0.54076099395751953125f;

         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.036454670944013329356512090082402429697),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.066235835556476033710068679907798799959),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.67492399795577182387312206593595565371),
            BOOST_MATH_BIG_CONSTANT(T, 113, -1.4345555263962411429855341651960000166),
            BOOST_MATH_BIG_CONSTANT(T, 113, -1.4894319559821365820516771951249649563),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.87210277668067964629483299712322411566),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.29602090537771744401524080430529369136),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.0561832587517836908929331992218879676),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.0053236785487328044334381502530383140443),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.00018629360291358130461736386077971890789),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.10164985672213178500790406939467614498e-6),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.13680157145361387405588201461036338274e-8)
         };
         static const T Q[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
            BOOST_MATH_BIG_CONSTANT(T, 113, 4.9106336261005990534095838574132225599),
            BOOST_MATH_BIG_CONSTANT(T, 113, 10.258804800866438510889341082793078432),
            BOOST_MATH_BIG_CONSTANT(T, 113, 11.88588976846826108836629960537466889),
            BOOST_MATH_BIG_CONSTANT(T, 113, 8.3455000546999704314454891036700998428),
            BOOST_MATH_BIG_CONSTANT(T, 113, 3.6428823682421746343233362007194282703),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.97465989807254572142266753052776132252),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.15121052897097822172763084966793352524),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.012017363555383555123769849654484594893),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.0003583032812720649835431669893011257277)
         };

         T r = tools::evaluate_polynomial(P, zm1) / tools::evaluate_polynomial(Q, zm1);
         T prefix = zm1 * zm2;

         result += prefix * Y + prefix * r;
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
      else if(z <= 1.625)
      {
         //
         // Use the following form:
         //
         // lgamma(z) = (2-z)(1-z)(Y + R(2-z))
         //
         // where R(2-z) is a rational approximation optimised for
         // low absolute error - as long as it's absolute error
         // is small compared to the constant Y - then any rounding
         // error in it's computation will get wiped out.
         //
         // R(2-z) has the following properties:
         //
         // Max error found at 128-bit long double precision  9.634e-36
         // Maximum Deviation Found (approximation error)     1.538e-37
         // Expected Error Term (theoretical error)           2.350e-38
         //
         static const float Y = 0.483787059783935546875f;

         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.017977422421608624353488126610933005432),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.18484528905298309555089509029244135703),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.40401251514859546989565001431430884082),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.40277179799147356461954182877921388182),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.21993421441282936476709677700477598816),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.069595742223850248095697771331107571011),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.012681481427699686635516772923547347328),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.0012489322866834830413292771335113136034),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.57058739515423112045108068834668269608e-4),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.8207548771933585614380644961342925976e-6)
         };
         static const T Q[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
            BOOST_MATH_BIG_CONSTANT(T, 113, -2.9629552288944259229543137757200262073),
            BOOST_MATH_BIG_CONSTANT(T, 113, 3.7118380799042118987185957298964772755),
            BOOST_MATH_BIG_CONSTANT(T, 113, -2.5569815272165399297600586376727357187),
            BOOST_MATH_BIG_CONSTANT(T, 113, 1.0546764918220835097855665680632153367),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.26574021300894401276478730940980810831),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.03996289731752081380552901986471233462),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.0033398680924544836817826046380586480873),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.00013288854760548251757651556792598235735),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.17194794958274081373243161848194745111e-5)
         };
         T r = zm2 * zm1;
         T R = tools::evaluate_polynomial(P, T(0.625 - zm1)) / tools::evaluate_polynomial(Q, T(0.625 - zm1));

         result += r * Y + r * R;
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
      else
      {
         //
         // Same form as above.
         //
         // Max error found (at 128-bit long double precision) 1.831e-35
         // Maximum Deviation Found (approximation error)      8.588e-36
         // Expected Error Term (theoretical error)            1.458e-36
         //
         static const float Y = 0.443811893463134765625f;

         static const T P[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.021027558364667626231512090082402429494),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.15128811104498736604523586803722368377),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.26249631480066246699388544451126410278),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.21148748610533489823742352180628489742),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.093964130697489071999873506148104370633),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.024292059227009051652542804957550866827),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.0036284453226534839926304745756906117066),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.0002939230129315195346843036254392485984),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.11088589183158123733132268042570710338e-4),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.13240510580220763969511741896361984162e-6)
         };
         static const T Q[] = {
            BOOST_MATH_BIG_CONSTANT(T, 113, 1.0),
            BOOST_MATH_BIG_CONSTANT(T, 113, -2.4240003754444040525462170802796471996),
            BOOST_MATH_BIG_CONSTANT(T, 113, 2.4868383476933178722203278602342786002),
            BOOST_MATH_BIG_CONSTANT(T, 113, -1.4047068395206343375520721509193698547),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.47583809087867443858344765659065773369),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.09865724264554556400463655444270700132),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.012238223514176587501074150988445109735),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.00084625068418239194670614419707491797097),
            BOOST_MATH_BIG_CONSTANT(T, 113, 0.2796574430456237061420839429225710602e-4),
            BOOST_MATH_BIG_CONSTANT(T, 113, -0.30202973883316730694433702165188835331e-6)
         };
         // (2 - x) * (1 - x) * (c + R(2 - x))
         T r = zm2 * zm1;
         T R = tools::evaluate_polynomial(P, T(-zm2)) / tools::evaluate_polynomial(Q, T(-zm2));

         result += r * Y + r * R;
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
   }
   BOOST_MATH_INSTRUMENT_CODE(result);
   return result;
}
template <class T, class Policy, class Lanczos>
T lgamma_small_imp(T z, T zm1, T zm2, const std::integral_constant<int, 0>&, const Policy& pol, const Lanczos& l)
{
   //
   // No rational approximations are available because either
   // T has no numeric_limits support (so we can't tell how
   // many digits it has), or T has more digits than we know
   // what to do with.... we do have a Lanczos approximation
   // though, and that can be used to keep errors under control.
   //
   BOOST_MATH_STD_USING  // for ADL of std names
   T result = 0;
   if(z < tools::epsilon<T>())
   {
      result = -log(z);
   }
   else if(z < 0.5)
   {
      // taking the log of tgamma reduces the error, no danger of overflow here:
      result = log(gamma_imp(z, pol, Lanczos()));
   }
   else if(z >= 3)
   {
      // taking the log of tgamma reduces the error, no danger of overflow here:
      result = log(gamma_imp(z, pol, Lanczos()));
   }
   else if(z >= 1.5)
   {
      // special case near 2:
      T dz = zm2;
      result = dz * log((z + lanczos_g_near_1_and_2(l) - T(0.5)) / boost::math::constants::e<T>());
      result += boost::math::log1p(dz / (lanczos_g_near_1_and_2(l) + T(1.5)), pol) * T(1.5);
      result += boost::math::log1p(Lanczos::lanczos_sum_near_2(dz), pol);
   }
   else
   {
      // special case near 1:
      T dz = zm1;
      result = dz * log((z + lanczos_g_near_1_and_2(l) - T(0.5)) / boost::math::constants::e<T>());
      result += boost::math::log1p(dz / (lanczos_g_near_1_and_2(l) + T(0.5)), pol) / 2;
      result += boost::math::log1p(Lanczos::lanczos_sum_near_1(dz), pol);
   }
   return result;
}

}}} // namespaces

#endif // BOOST_MATH_SPECIAL_FUNCTIONS_DETAIL_LGAMMA_SMALL


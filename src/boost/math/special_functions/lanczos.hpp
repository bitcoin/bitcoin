//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS
#define BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/big_constant.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/policies/policy.hpp>
#include <limits>
#include <type_traits>
#include <cstdint>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

namespace boost{ namespace math{ namespace lanczos{

//
// Individual lanczos approximations start here.
//
// Optimal values for G for each N are taken from
// http://web.mala.bc.ca/pughg/phdThesis/phdThesis.pdf,
// as are the theoretical error bounds.
//
// Constants calculated using the method described by Godfrey
// http://my.fit.edu/~gabdo/gamma.txt and elaborated by Toth at
// http://www.rskey.org/gamma.htm using NTL::RR at 1000 bit precision.
//
// Begin with a small helper to force initialization of constants prior
// to main.  This makes the constant initialization thread safe, even
// when called with a user-defined number type.
//
template <class Lanczos, class T>
struct lanczos_initializer
{
   struct init
   {
      init()
      {
         T t(1);
         Lanczos::lanczos_sum(t);
         Lanczos::lanczos_sum_expG_scaled(t);
         Lanczos::lanczos_sum_near_1(t);
         Lanczos::lanczos_sum_near_2(t);
         Lanczos::g();
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};
template <class Lanczos, class T>
typename lanczos_initializer<Lanczos, T>::init const lanczos_initializer<Lanczos, T>::initializer;

//
// Non-member helper which allows us to have a different g() value for the
// near_1 and near_2 approximations.  This is a big help in reducing error
// rates for multiprecision types at large digit counts.
// Default version assumes all g() values are the same.
//
template <class L>
inline double lanczos_g_near_1_and_2(const L&)
{
   return L::g();
}


//
// Lanczos Coefficients for N=6 G=5.581
// Max experimental error (with arbitrary precision arithmetic) 9.516e-12
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos6 : public std::integral_constant<int, 35>
{
   //
   // Produces slightly better than float precision when evaluated at
   // double precision:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      lanczos_initializer<lanczos6, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[6] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 8706.349592549009182288174442774377925882)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 8523.650341121874633477483696775067709735)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 3338.029219476423550899999750161289306564)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 653.6424994294008795995653541449610986791)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 63.99951844938187085666201263218840287667)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 2.506628274631006311133031631822390264407))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint16_t) denom[6] = {
         static_cast<std::uint16_t>(0u),
         static_cast<std::uint16_t>(24u),
         static_cast<std::uint16_t>(50u),
         static_cast<std::uint16_t>(35u),
         static_cast<std::uint16_t>(10u),
         static_cast<std::uint16_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      lanczos_initializer<lanczos6, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[6] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 32.81244541029783471623665933780748627823)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 32.12388941444332003446077108933558534361)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 12.58034729455216106950851080138931470954)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 2.463444478353241423633780693218408889251)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 0.2412010548258800231126240760264822486599)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 0.009446967704539249494420221613134244048319))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint16_t) denom[6] = {
         static_cast<std::uint16_t>(0u),
         static_cast<std::uint16_t>(24u),
         static_cast<std::uint16_t>(50u),
         static_cast<std::uint16_t>(35u),
         static_cast<std::uint16_t>(10u),
         static_cast<std::uint16_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      lanczos_initializer<lanczos6, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[5] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 2.044879010930422922760429926121241330235)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, -2.751366405578505366591317846728753993668)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 1.02282965224225004296750609604264824677)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, -0.09786124911582813985028889636665335893627)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 0.0009829742267506615183144364420540766510112)),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      lanczos_initializer<lanczos6, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[5] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 5.748142489536043490764289256167080091892)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, -7.734074268282457156081021756682138251825)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 2.875167944990511006997713242805893543947)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, -0.2750873773533504542306766137703788781776)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 35, 0.002763134585812698552178368447708846850353)),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 5.581000000000000405009359383257105946541; }
};

//
// Lanczos Coefficients for N=11 G=10.900511
// Max experimental error (with arbitrary precision arithmetic) 2.16676e-19
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos11 : public std::integral_constant<int, 60>
{
   //
   // Produces slightly better than double precision when evaluated at
   // extended-double precision:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      lanczos_initializer<lanczos11, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[11] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 38474670393.31776828316099004518914832218)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 36857665043.51950660081971227404959150474)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 15889202453.72942008945006665994637853242)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 4059208354.298834770194507810788393801607)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 680547661.1834733286087695557084801366446)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 78239755.00312005289816041245285376206263)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 6246580.776401795264013335510453568106366)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 341986.3488721347032223777872763188768288)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 12287.19451182455120096222044424100527629)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 261.6140441641668190791708576058805625502)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 2.506628274631000502415573855452633787834))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[11] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(362880u),
         static_cast<std::uint32_t>(1026576u),
         static_cast<std::uint32_t>(1172700u),
         static_cast<std::uint32_t>(723680u),
         static_cast<std::uint32_t>(269325u),
         static_cast<std::uint32_t>(63273u),
         static_cast<std::uint32_t>(9450u),
         static_cast<std::uint32_t>(870u),
         static_cast<std::uint32_t>(45u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      lanczos_initializer<lanczos11, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[11] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 709811.662581657956893540610814842699825)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 679979.847415722640161734319823103390728)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 293136.785721159725251629480984140341656)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 74887.5403291467179935942448101441897121)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 12555.29058241386295096255111537516768137)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 1443.42992444170669746078056942194198252)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 115.2419459613734722083208906727972935065)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 6.30923920573262762719523981992008976989)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.2266840463022436475495508977579735223818)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.004826466289237661857584712046231435101741)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.4624429436045378766270459638520555557321e-4))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[11] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(362880u),
         static_cast<std::uint32_t>(1026576u),
         static_cast<std::uint32_t>(1172700u),
         static_cast<std::uint32_t>(723680u),
         static_cast<std::uint32_t>(269325u),
         static_cast<std::uint32_t>(63273u),
         static_cast<std::uint32_t>(9450u),
         static_cast<std::uint32_t>(870u),
         static_cast<std::uint32_t>(45u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      lanczos_initializer<lanczos11, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[10] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 4.005853070677940377969080796551266387954)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -13.17044315127646469834125159673527183164)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 17.19146865350790353683895137079288129318)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -11.36446409067666626185701599196274701126)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 4.024801119349323770107694133829772634737)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -0.7445703262078094128346501724255463005006)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.06513861351917497265045550019547857713172)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -0.00217899958561830354633560009312512312758)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.17655204574495137651670832229571934738e-4)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -0.1036282091079938047775645941885460820853e-7)),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      lanczos_initializer<lanczos11, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[10] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 19.05889633808148715159575716844556056056)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -62.66183664701721716960978577959655644762)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 81.7929198065004751699057192860287512027)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -54.06941772964234828416072865069196553015)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 19.14904664790693019642068229478769661515)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -3.542488556926667589704590409095331790317)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.3099140334815639910894627700232804503017)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -0.01036716187296241640634252431913030440825)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, 0.8399926504443119927673843789048514017761e-4)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 60, -0.493038376656195010308610694048822561263e-7)),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 10.90051099999999983936049829935654997826; }
};

//
// Lanczos Coefficients for N=13 G=13.144565
// Max experimental error (with arbitrary precision arithmetic) 9.2213e-23
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos13 : public std::integral_constant<int, 72>
{
   //
   // Produces slightly better than extended-double precision when evaluated at
   // higher precision:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      lanczos_initializer<lanczos13, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[13] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 44012138428004.60895436261759919070125699)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 41590453358593.20051581730723108131357995)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 18013842787117.99677796276038389462742949)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 4728736263475.388896889723995205703970787)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 837910083628.4046470415724300225777912264)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 105583707273.4299344907359855510105321192)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 9701363618.494999493386608345339104922694)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 654914397.5482052641016767125048538245644)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 32238322.94213356530668889463945849409184)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 1128514.219497091438040721811544858643121)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 26665.79378459858944762533958798805525125)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 381.8801248632926870394389468349331394196)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 2.506628274631000502415763426076722427007))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[13] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(39916800u),
         static_cast<std::uint32_t>(120543840u),
         static_cast<std::uint32_t>(150917976u),
         static_cast<std::uint32_t>(105258076u),
         static_cast<std::uint32_t>(45995730u),
         static_cast<std::uint32_t>(13339535u),
         static_cast<std::uint32_t>(2637558u),
         static_cast<std::uint32_t>(357423u),
         static_cast<std::uint32_t>(32670u),
         static_cast<std::uint32_t>(1925u),
         static_cast<std::uint32_t>(66u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      lanczos_initializer<lanczos13, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[13] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 86091529.53418537217994842267760536134841)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 81354505.17858011242874285785316135398567)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 35236626.38815461910817650960734605416521)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 9249814.988024471294683815872977672237195)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 1639024.216687146960253839656643518985826)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 206530.8157641225032631778026076868855623)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 18976.70193530288915698282139308582105936)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 1281.068909912559479885759622791374106059)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 63.06093343420234536146194868906771599354)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 2.207470909792527638222674678171050209691)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.05216058694613505427476207805814960742102)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.0007469903808915448316510079585999893674101)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.4903180573459871862552197089738373164184e-5))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[13] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(39916800u),
         static_cast<std::uint32_t>(120543840u),
         static_cast<std::uint32_t>(150917976u),
         static_cast<std::uint32_t>(105258076u),
         static_cast<std::uint32_t>(45995730u),
         static_cast<std::uint32_t>(13339535u),
         static_cast<std::uint32_t>(2637558u),
         static_cast<std::uint32_t>(357423u),
         static_cast<std::uint32_t>(32670u),
         static_cast<std::uint32_t>(1925u),
         static_cast<std::uint32_t>(66u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      lanczos_initializer<lanczos13, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[12] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 4.832115561461656947793029596285626840312)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -19.86441536140337740383120735104359034688)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 33.9927422807443239927197864963170585331)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -31.41520692249765980987427413991250886138)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 17.0270866009599345679868972409543597821)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -5.5077216950865501362506920516723682167)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 1.037811741948214855286817963800439373362)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.106640468537356182313660880481398642811)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.005276450526660653288757565778182586742831)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.0001000935625597121545867453746252064770029)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.462590910138598083940803704521211569234e-6)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.1735307814426389420248044907765671743012e-9)),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      lanczos_initializer<lanczos13, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[12] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 26.96979819614830698367887026728396466395)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -110.8705424709385114023884328797900204863)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 189.7258846119231466417015694690434770085)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -175.3397202971107486383321670769397356553)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 95.03437648691551457087250340903980824948)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -30.7406022781665264273675797983497141978)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 5.792405601630517993355102578874590410552)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.5951993240669148697377539518639997795831)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.02944979359164017509944724739946255067671)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.0005586586555377030921194246330399163602684)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, 0.2581888478270733025288922038673392636029e-5)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 72, -0.9685385411006641478305219367315965391289e-9)),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 13.1445650000000000545696821063756942749; }
};

//
// Lanczos Coefficients for N=6 G=1.428456135094165802001953125
// Max experimental error (with arbitrary precision arithmetic) 8.111667e-8
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos6m24 : public std::integral_constant<int, 24>
{
   //
   // Use for float precision, when evaluated as a float:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[6] = {
         static_cast<T>(58.52061591769095910314047740215847630266L),
         static_cast<T>(182.5248962595894264831189414768236280862L),
         static_cast<T>(211.0971093028510041839168287718170827259L),
         static_cast<T>(112.2526547883668146736465390902227161763L),
         static_cast<T>(27.5192015197455403062503721613097825345L),
         static_cast<T>(2.50662858515256974113978724717473206342L)
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint16_t) denom[6] = {
         static_cast<std::uint16_t>(0u),
         static_cast<std::uint16_t>(24u),
         static_cast<std::uint16_t>(50u),
         static_cast<std::uint16_t>(35u),
         static_cast<std::uint16_t>(10u),
         static_cast<std::uint16_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[6] = {
         static_cast<T>(14.0261432874996476619570577285003839357L),
         static_cast<T>(43.74732405540314316089531289293124360129L),
         static_cast<T>(50.59547402616588964511581430025589038612L),
         static_cast<T>(26.90456680562548195593733429204228910299L),
         static_cast<T>(6.595765571169314946316366571954421695196L),
         static_cast<T>(0.6007854010515290065101128585795542383721L)
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint16_t) denom[6] = {
         static_cast<std::uint16_t>(0u),
         static_cast<std::uint16_t>(24u),
         static_cast<std::uint16_t>(50u),
         static_cast<std::uint16_t>(35u),
         static_cast<std::uint16_t>(10u),
         static_cast<std::uint16_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[5] = {
         static_cast<T>(0.4922488055204602807654354732674868442106L),
         static_cast<T>(0.004954497451132152436631238060933905650346L),
         static_cast<T>(-0.003374784572167105840686977985330859371848L),
         static_cast<T>(0.001924276018962061937026396537786414831385L),
         static_cast<T>(-0.00056533046336427583708166383712907694434L),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[5] = {
         static_cast<T>(0.6534966888520080645505805298901130485464L),
         static_cast<T>(0.006577461728560758362509168026049182707101L),
         static_cast<T>(-0.004480276069269967207178373559014835978161L),
         static_cast<T>(0.00255461870648818292376982818026706528842L),
         static_cast<T>(-0.000750517993690428370380996157470900204524L),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 1.428456135094165802001953125; }
};

//
// Lanczos Coefficients for N=13 G=6.024680040776729583740234375
// Max experimental error (with arbitrary precision arithmetic) 1.196214e-17
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos13m53 : public std::integral_constant<int, 53>
{
   //
   // Use for double precision, when evaluated as a double:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[13] = {
         static_cast<T>(23531376880.41075968857200767445163675473L),
         static_cast<T>(42919803642.64909876895789904700198885093L),
         static_cast<T>(35711959237.35566804944018545154716670596L),
         static_cast<T>(17921034426.03720969991975575445893111267L),
         static_cast<T>(6039542586.35202800506429164430729792107L),
         static_cast<T>(1439720407.311721673663223072794912393972L),
         static_cast<T>(248874557.8620541565114603864132294232163L),
         static_cast<T>(31426415.58540019438061423162831820536287L),
         static_cast<T>(2876370.628935372441225409051620849613599L),
         static_cast<T>(186056.2653952234950402949897160456992822L),
         static_cast<T>(8071.672002365816210638002902272250613822L),
         static_cast<T>(210.8242777515793458725097339207133627117L),
         static_cast<T>(2.506628274631000270164908177133837338626L)
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[13] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(39916800u),
         static_cast<std::uint32_t>(120543840u),
         static_cast<std::uint32_t>(150917976u),
         static_cast<std::uint32_t>(105258076u),
         static_cast<std::uint32_t>(45995730u),
         static_cast<std::uint32_t>(13339535u),
         static_cast<std::uint32_t>(2637558u),
         static_cast<std::uint32_t>(357423u),
         static_cast<std::uint32_t>(32670u),
         static_cast<std::uint32_t>(1925u),
         static_cast<std::uint32_t>(66u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[13] = {
         static_cast<T>(56906521.91347156388090791033559122686859L),
         static_cast<T>(103794043.1163445451906271053616070238554L),
         static_cast<T>(86363131.28813859145546927288977868422342L),
         static_cast<T>(43338889.32467613834773723740590533316085L),
         static_cast<T>(14605578.08768506808414169982791359218571L),
         static_cast<T>(3481712.15498064590882071018964774556468L),
         static_cast<T>(601859.6171681098786670226533699352302507L),
         static_cast<T>(75999.29304014542649875303443598909137092L),
         static_cast<T>(6955.999602515376140356310115515198987526L),
         static_cast<T>(449.9445569063168119446858607650988409623L),
         static_cast<T>(19.51992788247617482847860966235652136208L),
         static_cast<T>(0.5098416655656676188125178644804694509993L),
         static_cast<T>(0.006061842346248906525783753964555936883222L)
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint32_t) denom[13] = {
         static_cast<std::uint32_t>(0u),
         static_cast<std::uint32_t>(39916800u),
         static_cast<std::uint32_t>(120543840u),
         static_cast<std::uint32_t>(150917976u),
         static_cast<std::uint32_t>(105258076u),
         static_cast<std::uint32_t>(45995730u),
         static_cast<std::uint32_t>(13339535u),
         static_cast<std::uint32_t>(2637558u),
         static_cast<std::uint32_t>(357423u),
         static_cast<std::uint32_t>(32670u),
         static_cast<std::uint32_t>(1925u),
         static_cast<std::uint32_t>(66u),
         static_cast<std::uint32_t>(1u)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[12] = {
         static_cast<T>(2.208709979316623790862569924861841433016L),
         static_cast<T>(-3.327150580651624233553677113928873034916L),
         static_cast<T>(1.483082862367253753040442933770164111678L),
         static_cast<T>(-0.1993758927614728757314233026257810172008L),
         static_cast<T>(0.004785200610085071473880915854204301886437L),
         static_cast<T>(-0.1515973019871092388943437623825208095123e-5L),
         static_cast<T>(-0.2752907702903126466004207345038327818713e-7L),
         static_cast<T>(0.3075580174791348492737947340039992829546e-7L),
         static_cast<T>(-0.1933117898880828348692541394841204288047e-7L),
         static_cast<T>(0.8690926181038057039526127422002498960172e-8L),
         static_cast<T>(-0.2499505151487868335680273909354071938387e-8L),
         static_cast<T>(0.3394643171893132535170101292240837927725e-9L),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[12] = {
         static_cast<T>(6.565936202082889535528455955485877361223L),
         static_cast<T>(-9.8907772644920670589288081640128194231L),
         static_cast<T>(4.408830289125943377923077727900630927902L),
         static_cast<T>(-0.5926941084905061794445733628891024027949L),
         static_cast<T>(0.01422519127192419234315002746252160965831L),
         static_cast<T>(-0.4506604409707170077136555010018549819192e-5L),
         static_cast<T>(-0.8183698410724358930823737982119474130069e-7L),
         static_cast<T>(0.9142922068165324132060550591210267992072e-7L),
         static_cast<T>(-0.5746670642147041587497159649318454348117e-7L),
         static_cast<T>(0.2583592566524439230844378948704262291927e-7L),
         static_cast<T>(-0.7430396708998719707642735577238449585822e-8L),
         static_cast<T>(0.1009141566987569892221439918230042368112e-8L),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 6.024680040776729583740234375; }
};

//
// Lanczos Coefficients for N=17 G=12.2252227365970611572265625
// Max experimental error (with arbitrary precision arithmetic) 2.7699e-26
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos17m64 : public std::integral_constant<int, 64>
{
   //
   // Use for extended-double precision, when evaluated as an extended-double:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      lanczos_initializer<lanczos17m64, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[17] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 553681095419291969.2230556393350368550504)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 731918863887667017.2511276782146694632234)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 453393234285807339.4627124634539085143364)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 174701893724452790.3546219631779712198035)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 46866125995234723.82897281620357050883077)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9281280675933215.169109622777099699054272)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1403600894156674.551057997617468721789536)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 165345984157572.7305349809894046783973837)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 15333629842677.31531822808737907246817024)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1123152927963.956626161137169462874517318)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 64763127437.92329018717775593533620578237)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2908830362.657527782848828237106640944457)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 99764700.56999856729959383751710026787811)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2525791.604886139959837791244686290089331)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 44516.94034970167828580039370201346554872)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 488.0063567520005730476791712814838113252)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.50662827463100050241576877135758834683))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint64_t) denom[17] = {
         BOOST_MATH_INT_VALUE_SUFFIX(0, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1307674368000, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(4339163001600, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(6165817614720, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(5056995703824, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(2706813345600, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1009672107080, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(272803210680, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(54631129553, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(8207628000, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(928095740, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(78558480, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(4899622, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(218400, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(6580, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(120, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1, uLL)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      lanczos_initializer<lanczos17m64, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[17] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2715894658327.717377557655133124376674911)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3590179526097.912105038525528721129550434)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2223966599737.814969312127353235818710172)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 856940834518.9562481809925866825485883417)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 229885871668.749072933597446453399395469)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 45526171687.54610815813502794395753410032)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6884887713.165178784550917647709216424823)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 811048596.1407531864760282453852372777439)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 75213915.96540822314499613623119501704812)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5509245.417224265151697527957954952830126)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 317673.5368435419126714931842182369574221)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 14268.27989845035520147014373320337523596)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 489.3618720403263670213909083601787814792)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 12.38941330038454449295883217865458609584)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.2183627389504614963941574507281683147897)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.002393749522058449186690627996063983095463)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1229541408909435212800785616808830746135e-4))
      };
      static const BOOST_MATH_INT_TABLE_TYPE(T, std::uint64_t) denom[17] = {
         BOOST_MATH_INT_VALUE_SUFFIX(0, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1307674368000, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(4339163001600, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(6165817614720, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(5056995703824, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(2706813345600, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1009672107080, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(272803210680, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(54631129553, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(8207628000, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(928095740, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(78558480, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(4899622, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(218400, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(6580, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(120, uLL),
         BOOST_MATH_INT_VALUE_SUFFIX(1, uLL)
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      lanczos_initializer<lanczos17m64, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[16] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.493645054286536365763334986866616581265)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -16.95716370392468543800733966378143997694)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 26.19196892983737527836811770970479846644)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -21.3659076437988814488356323758179283908)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9.913992596774556590710751047594507535764)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -2.62888300018780199210536267080940382158)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.3807056693542503606384861890663080735588)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.02714647489697685807340312061034730486958)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0007815484715461206757220527133967191796747)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.6108630817371501052576880554048972272435e-5)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.5037380238864836824167713635482801545086e-8)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.1483232144262638814568926925964858237006e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1346609158752142460943888149156716841693e-14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.660492688923978805315914918995410340796e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.1472114697343266749193617793755763792681e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.1410901942033374651613542904678399264447e-16)),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      lanczos_initializer<lanczos17m64, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[16] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 23.56409085052261327114594781581930373708)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -88.92116338946308797946237246006238652361)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 137.3472822086847596961177383569603988797)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -112.0400438263562152489272966461114852861)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 51.98768915202973863076166956576777843805)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -13.78552090862799358221343319574970124948)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.996371068830872830250406773917646121742)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.1423525874909934506274738563671862576161)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.004098338646046865122459664947239111298524)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.3203286637326511000882086573060433529094e-4)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.2641536751640138646146395939004587594407e-7)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.7777876663062235617693516558976641009819e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.7061443477097101636871806229515157914789e-14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.3463537849537988455590834887691613484813e-14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.7719578215795234036320348283011129450595e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -0.7398586479708476329563577384044188912075e-16)),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 12.2252227365970611572265625; }
};

//
// Lanczos Coefficients for N=24 G=20.3209821879863739013671875
// Max experimental error (with arbitrary precision arithmetic) 1.0541e-38
// Generated with compiler: Microsoft Visual C++ version 8.0 on Win32 at Mar 23 2006
//
struct lanczos24m113 : public std::integral_constant<int, 113>
{
   //
   // Use for long-double precision, when evaluated as an long-double:
   //
   template <class T>
   static T lanczos_sum(const T& z)
   {
      lanczos_initializer<lanczos24m113, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[24] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2029889364934367661624137213253.22102954656825019111612712252027267955023987678816620961507)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2338599599286656537526273232565.2727349714338768161421882478417543004440597874814359063158)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1288527989493833400335117708406.3953711906175960449186720680201425446299360322830739180195)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 451779745834728745064649902914.550539158066332484594436145043388809847364393288132164411521)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 113141284461097964029239556815.291212318665536114012605167994061291631013303788706545334708)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 21533689802794625866812941616.7509064680880468667055339259146063256555368135236149614592432)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3235510315314840089932120340.71494940111731241353655381919722177496659303550321056514776757)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 393537392344185475704891959.081297108513472083749083165179784098220158201055270548272414314)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 39418265082950435024868801.5005452240816902251477336582325944930252142622315101857742955673)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3290158764187118871697791.05850632319194734270969161036889516414516566453884272345518372696)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 230677110449632078321772.618245845856640677845629174549731890660612368500786684333975350954)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 13652233645509183190158.5916189185218250859402806777406323001463296297553612462737044693697)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 683661466754325350495.216655026531202476397782296585200982429378069417193575896602446904762)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 28967871782219334117.0122379171041074970463982134039409352925258212207710168851968215545064)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1036104088560167006.2022834098572346459442601718514554488352117620272232373622553429728555)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 31128490785613152.8380102669349814751268126141105475287632676569913936040772990253369753962)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 779327504127342.536207878988196814811198475410572992436243686674896894543126229424358472541)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 16067543181294.643350688789124777020407337133926174150582333950666044399234540521336771876)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 268161795520.300916569439413185778557212729611517883948634711190170998896514639936969855484)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3533216359.10528191668842486732408440112703691790824611391987708562111396961696753452085068)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 35378979.5479656110614685178752543826919239614088343789329169535932709470588426584501652577)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 253034.881362204346444503097491737872930637147096453940375713745904094735506180552724766444)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1151.61895453463992438325318456328526085882924197763140514450975619271382783957699017875304)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2.50662827463100050241576528481104515966515623051532908941425544355490413900497467936202516))
      };
      static const T denom[24] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.112400072777760768e22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.414847677933545472e22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 6756146673770930688000.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 6548684852703068697600.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4280722865357147142912.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2021687376910682741568.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 720308216440924653696.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 199321978221066137360.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 43714229649594412832.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 7707401101297361068.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1103230881185949736.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 129006659818331295.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 12363045847086207.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 971250460939913.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 62382416421941.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3256091103430.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 136717357942.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4546047198.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 116896626.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2240315.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 30107.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 253.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1.0))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      lanczos_initializer<lanczos24m113, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T num[24] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3035162425359883494754.02878223286972654682199012688209026810841953293372712802258398358538)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3496756894406430103600.16057175075063458536101374170860226963245118484234495645518505519827)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1926652656689320888654.01954015145958293168365236755537645929361841917596501251362171653478)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 675517066488272766316.083023742440619929434602223726894748181327187670231286180156444871912)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 169172853104918752780.086262749564831660238912144573032141700464995906149421555926000038492)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 32197935167225605785.6444116302160245528783954573163541751756353183343357329404208062043808)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4837849542714083249.37587447454818124327561966323276633775195138872820542242539845253171632)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 588431038090493242.308438203986649553459461798968819276505178004064031201740043314534404158)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 58939585141634058.6206417889192563007809470547755357240808035714047014324843817783741669733)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4919561837722192.82991866530802080996138070630296720420704876654726991998309206256077395868)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 344916580244240.407442753122831512004021081677987651622305356145640394384006997569631719101)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 20413302960687.8250598845969238472629322716685686993835561234733641729957841485003560103066)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1022234822943.78400752460970689311934727763870970686747383486600540378889311406851534545789)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 43313787191.9821354846952908076307094286897439975815501673706144217246093900159173598852503)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1549219505.59667418528481770869280437577581951167003505825834192510436144666564648361001914)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 46544421.1998761919380541579358096705925369145324466147390364674998568485110045455014967149)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1165278.06807504975090675074910052763026564833951579556132777702952882101173607903881127542)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 24024.759267256769471083727721827405338569868270177779485912486668586611981795179894572115)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 400.965008113421955824358063769761286758463521789765880962939528760888853281920872064838918)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 5.28299015654478269617039029170846385138134929147421558771949982217659507918482272439717603)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.0528999024412510102409256676599360516359062802002483877724963720047531347449011629466149805)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.000378346710654740685454266569593414561162134092347356968516522170279688139165340746957511115)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.172194142179211139195966608011235161516824700287310869949928393345257114743230967204370963e-5)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.374799931707148855771381263542708435935402853962736029347951399323367765509988401336565436e-8))
      };
      static const T denom[24] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.112400072777760768e22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.414847677933545472e22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 6756146673770930688000.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 6548684852703068697600.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4280722865357147142912.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2021687376910682741568.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 720308216440924653696.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 199321978221066137360.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 43714229649594412832.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 7707401101297361068.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1103230881185949736.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 129006659818331295.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 12363045847086207.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 971250460939913.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 62382416421941.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 3256091103430.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 136717357942.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4546047198.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 116896626.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2240315.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 30107.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 253.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1.0))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      lanczos_initializer<lanczos24m113, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[23] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 7.4734083002469026177867421609938203388868806387315406134072298925733950040583068760685908)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -50.4225805042247530267317342133388132970816607563062253708655085754357843064134941138154171)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 152.288200621747008570784082624444625293884063492396162110698238568311211546361189979357019)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -271.894959539150384169327513139846971255640842175739337449692360299099322742181325023644769)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 319.240102980202312307047586791116902719088581839891008532114107693294261542869734803906793)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -259.493144143048088289689500935518073716201741349569864988870534417890269467336454358361499)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 149.747518319689708813209645403067832020714660918583227716408482877303972685262557460145835)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -61.9261301009341333289187201425188698128684426428003249782448828881580630606817104372760037)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 18.3077524177286961563937379403377462608113523887554047531153187277072451294845795496072365)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -3.82011322251948043097070160584761236869363471824695092089556195047949392738162970152230254)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.549382685505691522516705902336780999493262538301283190963770663549981309645795228539620711)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.0524814679715180697633723771076668718265358076235229045603747927518423453658004287459638024)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.00315392664003333528534120626687784812050217700942910879712808180705014754163256855643360698)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.000110098373127648510519799564665442121339511198561008748083409549601095293123407080388658329)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.19809382866681658224945717689377373458866950897791116315219376038432014207446832310901893e-5)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.152278977408600291408265615203504153130482270424202400677280558181047344681214058227949755e-7)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.364344768076106268872239259083188037615571711218395765792787047015406264051536972018235217e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.148897510480440424971521542520683536298361220674662555578951242811522959610991621951203526e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.261199241161582662426512749820666625442516059622425213340053324061794752786482115387573582e-18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.780072664167099103420998436901014795601783313858454665485256897090476089641613851903791529e-24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.303465867587106629530056603454807425512962762653755513440561256044986695349304176849392735e-24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.615420597971283870342083342286977366161772327800327789325710571275345878439656918541092056e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.499641233843540749369110053005439398774706583601830828776209650445427083113181961630763702e-26)),
      };
      T result = 0;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(k*dz + k*k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      lanczos_initializer<lanczos24m113, T>::force_instantiate(); // Ensure our constants get initialized before main()
      static const T d[23] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 61.4165001061101455341808888883960361969557848005400286332291451422461117307237198559485365)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -414.372973678657049667308134761613915623353625332248315105320470271523320700386200587519147)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1251.50505818554680171298972755376376836161706773644771875668053742215217922228357204561873)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -2234.43389421602399514176336175766511311493214354568097811220122848998413358085613880612158)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 2623.51647746991904821899989145639147785427273427135380151752779100215839537090464785708684)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -2132.51572435428751962745870184529534443305617818870214348386131243463614597272260797772423)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 1230.62572059218405766499842067263311220019173335523810725664442147670956427061920234820189)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -508.90919151163744999377586956023909888833335885805154492270846381061182696305011395981929)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 150.453184562246579758706538566480316921938628645961177699894388251635886834047343195475395)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -31.3937061525822497422230490071156186113405446381476081565548185848237169870395131828731397)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 4.51482916590287954234936829724231512565732528859217337795452389161322923867318809206313688)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.431292919341108177524462194102701868233551186625103849565527515201492276412231365776131952)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.0259189820815586225636729971503340447445001375909094681698918294680345547092233915092128323)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.000904788882557558697594884691337532557729219389814315972435534723829065673966567231504429712)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.162793589759218213439218473348810982422449144393340433592232065020562974405674317564164312e-4)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.125142926178202562426432039899709511761368233479483128438847484617555752948755923647214487e-6)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.299418680048132583204152682950097239197934281178261879500770485862852229898797687301941982e-9)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.122364035267809278675627784883078206654408225276233049012165202996967011873995261617995421e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.21465364366598631597052073538883430194257709353929022544344097235100199405814005393447785e-17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.641064035802907518396608051803921688237330857546406669209280666066685733941549058513986818e-23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.249388374622173329690271566855185869111237201309011956145463506483151054813346819490278951e-23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, -0.505752900177513489906064295001851463338022055787536494321532352380960774349054239257683149e-24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 113, 0.410605371184590959139968810080063542546949719163227555918846829816144878123034347778284006e-25)),
      };
      T result = 0;
      T z = dz + 2;
      for(unsigned k = 1; k <= sizeof(d)/sizeof(d[0]); ++k)
      {
         result += (-d[k-1]*dz)/(z + k*z + k*k - 1);
      }
      return result;
   }

   static double g(){ return 20.3209821879863739013671875; }
};

//
// Lanczos Coefficients for N=27 G=2.472513680905104038743047567550092935562134e+01
// Max experimental error (with MP precision arithmetic) 0.000000000000000000000000000000000000000000e+00
// Generated with compiler: Microsoft Visual C++ version 14.2 on Win32 at May 23 2021
// Type precision was 134 bits or 42 max_digits10
//
struct lanczos27MP : public std::integral_constant<int, 134>
{
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[27] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.532923291341302819860952064783714673718970e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.715272050979243637524956158081893927075092e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.399396313336459710065708403038293278484916e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.615805213483907585030394968151583590083805e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.094287593119694642121339924355455488336630e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.985179143643083871895846729884916046817583e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.864723387203319421361199873281888626383507e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.374651939493419385833371654981557918551584e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.304504350810987437240912594601486056121725e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.724892917231894382998818728699010291796660e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.909901039551708500588401626148435467434009e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.145381204249362220411918333792713760478856e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.902980366355225260615014098246446681081078e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.620997933261144559370948440813656891792187e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.003441440382636640319535096309665505136930e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.309721390821762354780404195884829522953769e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 9.381514076593540726655991152770953882150136e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.275266040978137565809877941293859174071955e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.690398430937632687996992361090819887063422e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 8.142411407304237744553849404860811146407986e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.174971623395676312463521417132401487856454e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.384092119107453943335286646923309490786229e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.296932429990667045419860753608558102709582e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 9.299378037650538629629318998114044963408825e+07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.792561328661952922209314899668849919321249e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.580741273679785112052701460119954412080073e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.506628274631000502415765284811045253005320e+00))
      };
      static const T denom[27] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 0.000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.551121004333098598400000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.919012881170120359936000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.004801715483511615488000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.023395306017446756725760000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.087414531983767267719680000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.577035564590760682636262400000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.374646821796792697868000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.144457803247115877036800000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.001369304512841374110000000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.969281004511108202428800000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.188201437529851278250000000000000000000000e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.284218746244111474800000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.805445587427335451250000000000000000000000e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.514594692699448186500000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.557372853474553750000000000000000000000000e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.349615694227860500000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.297275331854287500000000000000000000000000e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.956673043671350000000000000000000000000000e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.256393782500000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.968295763000000000000000000000000000000000e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.724710487500000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.336854950000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.858750000000000000000000000000000000000000e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.005000000000000000000000000000000000000000e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.250000000000000000000000000000000000000000e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[27] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.630539114451826442425094380936505531231478e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.963898228350662244301785145431331232866294e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.558292778812387748738731408569861630189290e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 8.438339470758124934572462000795083198080916e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.000511235267926346573212315280041509763731e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.629185970715063928416526096935558921044815e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.237116237146422484431753186953979152997281e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.169337167415775727114018906990954798102547e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.041097534463262894898495303906833076469281e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.981486521549315574859643064948741979243976e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.491567035847004398885838650781864506656075e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.093917524216073202169716871304960622121045e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.079147622499629876874169792116583887362096e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.791551915666662583520458128259897770660473e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.834431723470453391466841656396291574724498e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.050635015489291434258728317621551605496937e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.715072384266421431637543951156767586591045e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.159505514655385281007353699906486901798470e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 8.574706336771416438731056639147393961539411e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.488547033239016552342729952719496931402330e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.148012961586177396403312787979484589898276e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.530314564772178162122057449947469958774484e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.370974425637913452858480025228307253546963e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.700056764080375263450528442694493496437080e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 8.761474446005270789145652778771406388702068e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.889816806780013044430000551700375309307825e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.582468135039046226997146555551548992616343e-11))
      };
      static const T denom[27] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 0.000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.551121004333098598400000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.919012881170120359936000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.004801715483511615488000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.023395306017446756725760000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.087414531983767267719680000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.577035564590760682636262400000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.374646821796792697868000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.144457803247115877036800000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.001369304512841374110000000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.969281004511108202428800000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.188201437529851278250000000000000000000000e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.284218746244111474800000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.805445587427335451250000000000000000000000e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.514594692699448186500000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.557372853474553750000000000000000000000000e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.349615694227860500000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.297275331854287500000000000000000000000000e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.956673043671350000000000000000000000000000e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.256393782500000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.968295763000000000000000000000000000000000e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.724710487500000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.336854950000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.858750000000000000000000000000000000000000e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.005000000000000000000000000000000000000000e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.250000000000000000000000000000000000000000e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[34] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 6.264579889722939745225908247624593169040293e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -3.470545597111704235784909052092266897169254e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 8.398164226943527197542310295220360303173237e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.166490739555248669771075340695671987349622e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.028101937812836112448434230485371426845812e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -6.003050880354706854567842055875605768028585e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.355206767355338215012383892758889890708805e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -6.173166763225116428638036856999036700963277e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.055748115088123667349396984075505516234940e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.127784364612243323022358484127515048080935e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.013011055366411613813518259345336997226641e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.271137289000937686705998821090835222190159e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.195172534910278451113805217678979457290834e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.421890451863814077221239932785029648679973e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.066311611137421591999312557597869716741027e-11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.797948012646761974584234409950319937184538e-16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -5.274002995605577985657965320478056380380290e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.270091452696164640108774677242731307730848e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -6.933040546739252731034872986511694993372995e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.405071936614348906224568346156522897751303e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.105092450748689398417350156762592106638543e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.573335807137266819877752062372030042747590e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.690602407074901259448169161354115161602278e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.445091932555604281164557526008785529455861e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.932804556880430674197633802977544778784320e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.320001406610629373227596309759263536640140e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -7.699733918513786660891771237627803608806010e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.776870859236169815307382842451635095251495e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.526154769745297076196084765279504608995696e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.939458578626915680695594094484224178207306e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.229538969055131478930409285699348366508295e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.207569067702627873429089508800955397620386e-23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.542428477414786133402832964643707382175743e-24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.409458057545117569935733339065832415295665e-25))
      };
      T result = 0;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (k * dz + k * k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[34] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.391991857844535020743473289228849738381662e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -2.433141291692735004291785549611375831426138e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 5.887812040849956173864447000497922705559488e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -8.178070869177285054991117755136346786974125e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.207850198088647199855281811058606257270817e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -4.208638257131458956367681504789416772705762e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.651195950543217389263490876246883903526458e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -4.327903648523876358512872196882929451369963e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.401672908678997114468388150043974540095678e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -7.906706968342945744899907670199667000072243e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 4.916704391410548803397953511596928808893685e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.592256249729202493268939584019491192080080e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.240081857804364904696255913500139170039349e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -9.968635402954290441376528527568797927543768e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 7.475731807209447934074840206826861054997914e-11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.961594409606987475034042150632670295904917e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -3.697515016601028609216707527257479621172555e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.591523031442252914289458638424672100510104e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -4.860638409502590149748648713304503849363893e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 9.850723614235842081434077716825371111986246e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.475844999417373489569601576817086030522522e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.804122560714365990744061859839148408328067e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.886336206511766947905039498619940334834436e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.714212931833249115161397417081604581762608e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.355056847554880232469037060291577918972607e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 9.254308400931922182743462783124793743058980e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -5.398154269396277345367516583851274647578103e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 2.647900793652290520419156346839352858087685e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.069961504286664892352397126472100106281531e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 3.462971538614891132079878533424998572755101e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -8.620091428399885297009840750915836982112365e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 1.547689636281132331592940788973245529484744e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, -1.782453950387991004107321678322483537333246e-23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 134, 9.881473972208065873607436095608077625677024e-25)),
      };
      T result = 0;
      T z = dz + 2;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (z + k * z + k * k - 1);
      }
      return result;
   }

   static double g() { return 2.472513680905104038743047567550092935562134e+01; }
};

inline double lanczos_g_near_1_and_2(const lanczos27MP&)
{
   return 17.03623256087303;
}

//
// Lanczos Coefficients for N=35 G=2.96640371531248092651367187500000000000000000000000000e+01
// Max experimental error (with 50 digit precision arithmetic) 67eps
// Generated with compiler: Microsoft Visual C++ version 14.2 on Win32 at Oct 14 2019
// Type precision was 168 bits or 53 max_digits10
//
struct lanczos35MP : public std::integral_constant<int, 168>
{
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[35] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.17215050716253100021302249837728942659410271586236104e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.51055117651708470336913962553466820524801246971658127e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.40813458996718289733677017073036013655624930344397267e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.10569518324826607478187974291222641098997506635019681e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.34502197565331471178368569687788687058240547971732391e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.74311603169690571192608960963509140372217014888512918e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.50656021978234091874071935392175934984492682009447097e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.12703102551730381018400796362603958419580969330315139e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.02844698442195350077632196816248435420923619452768200e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.90106767379334717236568166816961185224083190775430842e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.86371531667026447746284883480888667804130713757839681e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.34808948517797782155274346690360992144536507118093783e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.83232124439938458545786668616393415008373341980153072e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.62895707563068512468013948922815298700909218398406635e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.30384063116420066671650072267242339695473078925159324e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.76258309689585811716178198120267186946262194080905971e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.51837231299916455171135124843484994848995300472356341e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.46324357690180919340289798257560253430931750807924001e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.75333853376321853646128997503611223620394342435525484e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.01719517877315910652307531002686423847077617217874485e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.27861878894319497853745513558138184450369083409359360e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.89640024726662067702004632718605032785787967237099607e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.81537701811791870172286588846619085013138846074815251e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.03090758312551459302562064161308518889144037164899961e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.60538569869661647274451913615710409703905629234367906e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.18176163448730621246454091850022844174919234685832508e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.56586635256765282348264053213197702964352373258511008e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.58289895656990946427745668670352144404744258615044371e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.19373478903102411154024309088124853938046967389531861e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.54192605870424877025476980158698548681325282029269310e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.73027427579217615249706012469272147499107562412573337e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.82675918536460865549992482360500962016208597062710654e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.21869956201943834772161655315196962519434419814106818e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.50897418653428667959996348205296461689142907811767371e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.50662827463100050241576528481104525300698674060984055e+00))
      };
      static const T denom[35] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 0.00000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.68331761881188649551819440128000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.55043336733310191803732770947072000000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.55728779174162547080350866368102400000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.37352350419052295388404251629977600000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.72117566475005542296335706764492800000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.28417720643003773414159612967554252800000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.45822739485943139719482682477713244160000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.16476527817201997988283152951021977600000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.49225481668254064104679479029764121600000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.57726463942545496998486904826347776000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.20859297660335343156864734965859840000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.23364307820330543590375511999050240000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.80750015058176473779293385245398400000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.28183125026789051815954180232544000000000000000000000e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.49437224233918151570015089338400000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.37000480501772121324931003824000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.96258640868140652967646352465000000000000000000000000e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.41894262447739018035536664650000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.96452376168568744680811480000000000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.94875410890088264440962800000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.38478815149246067334598000000000000000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.00124085806115519088380000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.65117470518809938644000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.15145312544238764840000000000000000000000000000000000e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.12192419709374919000000000000000000000000000000000000e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.22038661704031100000000000000000000000000000000000000e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.40979763670090400000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.29191290647440000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.04437176604000000000000000000000000000000000000000000e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.21763644400000000000000000000000000000000000000000000e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.60169360000000000000000000000000000000000000000000000e+07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.51096000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.61000000000000000000000000000000000000000000000000000e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.00000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[35] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.84421398435712762388902267099927585742388886580864424e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.28731583799033736725852757551292030085556435695468295e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.84381150359300352571680869181416248982215282642834936e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.68539753215772969226355064737523321566208288321687448e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.76117184320624276162478300964159399462275652881271996e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.59183627116994441494601110756468114877940946273012852e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.90089018057779871758440184258134151304912092733579104e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.02273473587728940068021671629793244969348874651645551e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 9.20304883823127369598764418881022021049206245678741573e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 9.03625836242722113759123056762610636251641913153595812e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.67794913334462808923359541498599600753842936204419932e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.69338859264140114791649895977363900871692586779302150e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.70864158121145435408364940074910197916145829346031858e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.13295647753179115743895667847873122731507276407230715e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.08730493440263847356723847541024859440843056640671533e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.92672649809905793239714364398097142490510744815940192e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.98815678372776973689475889094271298156568135487559824e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.15357141696015228406471054927723105303656292491717836e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.29582156512528703674984172534752222415664014582498353e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.56951562180494343732211791410530161839249714612303326e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.67422350715677024140556410421772283993277946880053914e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.79254663081905790190270601146772274854974105071798035e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.61465496276608608941993297108655885737613121720232292e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.34987044168298086318822469739196823360923972361455073e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.10209211537761991333937729340544738747931371426736883e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.85679879496413826670691454915567101976631415248412906e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.35974553231926272707704478737590721340254406209650188e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.38204802486455055334129565820015244464343854444712513e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.87247644413155087645140975008088533286977710080244249e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.01899805954981363917258740277358024893572331522514601e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.14314215799519834172753514406176454576793263619287700e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.01075867159821346256470334018168931185179114379271616e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.59576526838074751422330690168945437827562833198707558e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.28525092722679899458094768960179796663588010298597603e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.28217919006153582429216342066702743329957749672852350e-13))
      };
      static const T denom[35] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 0.00000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.68331761881188649551819440128000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.55043336733310191803732770947072000000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.55728779174162547080350866368102400000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.37352350419052295388404251629977600000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.72117566475005542296335706764492800000000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.28417720643003773414159612967554252800000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.45822739485943139719482682477713244160000000000000000e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.16476527817201997988283152951021977600000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.49225481668254064104679479029764121600000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.57726463942545496998486904826347776000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.20859297660335343156864734965859840000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.23364307820330543590375511999050240000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.80750015058176473779293385245398400000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.28183125026789051815954180232544000000000000000000000e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.49437224233918151570015089338400000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.37000480501772121324931003824000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.96258640868140652967646352465000000000000000000000000e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.41894262447739018035536664650000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.96452376168568744680811480000000000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.94875410890088264440962800000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.38478815149246067334598000000000000000000000000000000e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.00124085806115519088380000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.65117470518809938644000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.15145312544238764840000000000000000000000000000000000e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.12192419709374919000000000000000000000000000000000000e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.22038661704031100000000000000000000000000000000000000e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.40979763670090400000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.29191290647440000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.04437176604000000000000000000000000000000000000000000e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.21763644400000000000000000000000000000000000000000000e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.60169360000000000000000000000000000000000000000000000e+07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.51096000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.61000000000000000000000000000000000000000000000000000e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.00000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[42] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.2258008829795701933757823508857131818190413131511363e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -6.1680809698202901664719598422224259984110345848176138e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.0937956909159916126016144892534179459545368045658870e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -4.2570860117223597345299309707009980433696777143916823e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.7808407045434705509914139521956838552432057817709310e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -5.5355182201018147597112724614545263772722036922648575e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.8474340895549068665467127190441982794533803160633534e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.9687073432491586288948383529096081854867384409828362e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.4457539281218595159502905008069838638140685905208109e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.0724321926101376768201888687693227423632630755627070e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.1941554220476109189863208161993450668341832413951177e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -6.0469416499468520752326008902894754184436051369514739e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.0254471406496505041361077191383344271915106887055424e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.9743975328123868311047848806382369109187457702980947e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.6326975883294075748535457727960259872733702003969396e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.8276395425975110081829250599527615065306178329307764e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.4994926214942760944619799278085799215984014361562132e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.1685212562684580327244208091708941173130794374261284e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.0566129445336641178978472923139566421562362783155822e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -6.6744193557172228303189080097715371728193237070211608e-17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.3116377246238995291497495503598572469502355628188604e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.7791795131683583370183641939988202673347172514688534e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 9.6372242277604226411817535739257869758194674562641039e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.7502495488892655715569603094708394381657045801526069e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.0501577132014302973783965458067331883116843242885033e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.0246214059191840597181314245134333087378581123342727e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.4016071303078853730266134475467378117726380022343630e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.6214830666337247122639245651193515459936309025504988e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.6312853482448038567407561706085851388360060108080568e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.4458785355627609495060506977643541320437284829970271e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.1331287575394227733315016732552406681866623847709417e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -7.8351635033967037250982310034619565150687081453609992e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.7520885958378593874310858129100278585054737696926701e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.5058409122183022757924336573867978222207111500077203e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.1353898614924597482474648262273645405650282912119167e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -4.3531153377666279783383214654257629384565834244973196e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.3839135182642184911017974189326632232475070566724497e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.5479558181723745255902653783884759401621303982915322e-29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.0441825447107352322817077249008075090725287665933142e-30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.0157887327297754418593987114368959771100770274203800e-30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 9.4607280988529299025458955706898751267120992042268667e-32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -4.2702032336418528894772149178970767164510337389404370e-33))
      };
      T result = 0;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (k * dz + k * k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[42] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 7.3782193657165970743894979068466124765194827248379940e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -5.5325256602067816772285455933211570612342576586214891e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.8780522570799869937961476290263461833002660531646012e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.8184384596766268378888212415693303553880671796724735e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.1851863962133477520750252664910607723762372771833722e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -4.9651417912803026185477059393373316779106801664686922e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.4509968222802070571038728168526976259879110509473673e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.7658529366356277958293590921029620586497540226150778e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.6785479743985639684438881535624244315638292743993560e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.8588900406390499925005060563245955316983471925301184e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 3.7619921996662567540276653040387614527561121386327888e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -5.4238684621351227134322239416053684476498738936970880e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.4045887339956612618258661862314001628281850888893694e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.5648780296066214471224136948413423004282323597057130e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.4644654223878248996887367583334112564695286627087816e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.4332418933955508302078477926243914098802666261366678e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.0358676398914795323109452992079597262040795201720992e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.9450781456519433542572418782578042705818718277820822e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 2.7416614067965791526251519843473783727166050306987362e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -5.9866912469474631311384623900742191091588854047124831e-16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.7643298058164570865040068204832109970445542816595386e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.4928145473701388663847838847907869194628008362147191e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.6442105079571407791997926495585104881317603986245265e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.4668655090053572169091092679046557243825245275372519e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 5.4267531441890485644063141608998212380717408586417687e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -9.1904503977518246823477462773596622658428222241396033e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.2571863845333234910026102012663485977044671519503762e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.4544064381831932921238058900083969277495893492892409e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.4631986986621358205011740516995928067691739105999606e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -1.2968960911316058263225162731552440661464077730310616e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.0163718599154085420173689991270137222141149005433561e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -7.0278330240079354003556694314544898753730575016426382e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 4.2624362787531585897289168590458242934350512724487489e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -2.2476405895248242769628910185593849817270999749433590e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.0183999810930941402072961555627198647985838184285745e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.9045729824028673594421184017022743317479187113896743e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 1.2413159115073860802650598534057774896614268826143102e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.1823766097367740928881247634568036933183255409575449e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 6.3183542619623422719031481991659628332908631907371078e-29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -9.1112247985618590949970839428497941653776549519221927e-30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, 8.4859004327675283792859615082199609974336399587796249e-31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 168, -3.8302040910318742925508017945893539585506545571212821e-32)),
      };
      T result = 0;
      T z = dz + 2;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (z + k * z + k * k - 1);
      }
      return result;
   }

   static double g() { return 2.96640371531248092651367187500000000000000000000000000e+01; }
};

inline double lanczos_g_near_1_and_2(const lanczos35MP&)
{
   return 22.36563469469547;
}
//
// Lanczos Coefficients for N=48 G=2.880805098265409469604492187500000000000000000000000000000000000e+01
// Max experimental error (with 60-digit precision arithmetic) 51eps
// Generated with compiler: Microsoft Visual C++ version 14.2 on Win32 at Oct 14 2019
// Type precision was 201 bits or 63 max_digits10
//
struct lanczos48MP : public std::integral_constant<int, 201>
{
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.761757987425932419978923296640371540367427757167447418730589877e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.723233313564421930629677035555276136256253817229396631458438691e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.460052620548943146316510839385235752729444155384745952604400014e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.118620599704657143233902039524163888476114389296433891234019212e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.103553323924588863191816202847384353588419783622786374048756587e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.051624469576894078907076790635986076815810433950937821174281248e+69)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.865434054315747674202246332480484800778071304068935338977820344e+68)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.291785980379681713553231795767203835753576510251486784293089714e+68)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.073927196464385740270105346713079967925505577692095446860826790e+67)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.884317172328855613403642857232246924724496526520223674336243586e+66)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.515983058669346491005379681336434957516572863544374020968683717e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.791988252541273516986153564408477102509671668999707480365384945e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.645764905652320236264233988360776875326874810201273735655153182e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.144135487589921315512939394666974184673239886993573956770438389e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.444700846549614719681016920231266383188819427952261902403138865e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.721099093953481665535866508692670759355705777392277743203856663e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.100969797434901880312682514502493221610943693861105392844971160e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.418121506159806547634040503980950792234471035467217702752406105e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.417864259432558812733518752689742288284271989351444645566759428e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.665995533734965936996397899459612023184583125575089834552055942e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.444766925649844009950058690449625999301860892596426461258095232e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.053637791492838551734963920042182131006240650838206322215619662e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.150696853422753584935226676401667305978026730065639035499393518e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.985976091763077924792684854305586783380530313659602423780141188e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.269589095786672590317833654141210781129738119237951536741077115e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.718300118825405526804849893058410300716988331091767076237827497e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.001037055130874457401651655102738871459032839441218104652569066e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.475842513986568687160423191409256650108932454810648362428602348e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.620452049086499203878684285356863241396518483154492676811559133e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.169661026157169583693125067814111812572434991018171004040405784e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.227918466522161929152413190031319328201533237960827483146218740e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.876388843752351291646654793076860108915313255758699513365393870e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.145947758366681136606104191450792163942386660344907590963820717e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.853323303407534484800459250019301328433169196161471441696806506e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.154628006575221227908667538321556179086649067527404327882584768e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.357526820024103486396860374714568600536209103260198100884104997e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.431529899588725297356982438015035066854198997921929156832870645e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.345565129287503320724079046959642760096964859126850291147857935e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.118851309483567225684739040233675455708538654675741148330404763e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.153371780240325463304870847387326315142505274277395976930776452e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.146212685927632120682088036018035709941745020823689824280902727e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.771109638413640784841091904266004758198074452790973613270876444e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.247775743837944205683004431867637625466576857881195465700397478e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.570311375510395966207715903995528566489264305503840005145629111e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.307932649387240491969419239876926639445902586258953887216911993e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.743144608535924824275750439447323876880302369055576390115394778e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.749690888961891063146468955091435916957208840312184463551812828e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.506628274631000502415765284811045253006986740609938316629929233e+00))
      };
      static const T denom[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 0.000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.502622159812088949850305428800254892961651752960000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.430336111272256671478593169569751383305061494947840000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.920361290698585974808779016476219830728024276336640000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.149178946896205138947217427059336370288899808821248000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.374105269656119699331051574067858017333550280343552000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.521316226597066883749849655326023294027593332332429312000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.808864152650289891915479515152146571014320216782405632000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.514810409642252571378917003183814999063638859346214912000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.583350992233550434239775839017811699814141926043903590400000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.478403249251559174520099458337662519939088809134875607040000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.848344883280695333961708798743230793633983609036568330240000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.943873277267014936040757307088314776495222166971439104000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.331069721888505257142927693659482094449571844495257600000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.196124539826947758881834650235619760202156354268084224000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.723744838816127002822609734027860811982593574672547840000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.205691767196054136766333529400075228162139411801728000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.517213632743192166819003098472340901249838381523200000000000000e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.571722144655713179046526371841394014407124514352640000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.359512744028577584409389641902976782871564427046400000000000000e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.949188285585060392916084953872833077002135851920000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.452967188675463645529736303316005271151737332000000000000000000e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.790015208782962556675223159728484084908850744000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.970673071264242753610155919125826961862567840000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.299166890445957751586491053313346243255473500000000000000000000e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.652735578141047520337049888545244673386975000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.508428802270485256066710729742536448661900000000000000000000000e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.093294777021479729147119238554967297499000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.155176275192359061296447275633302204250000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.907505708457079284974986712721395225000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.195848283940498442888394846136646210000000000000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.305934675041764670409270520636101000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.238840089027488915014959267151000000000000000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.846167161648076059624793804150000000000000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.707826341119682695847826052600000000000000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.648183019818072129964867660000000000000000000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.059080011923383455919277000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.490144286132397218940500000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.838362455658776519186000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.970532718044669378600000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.814183952293757550000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.413370614847675000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.134958017031000000000000000000000000000000000000000000000000000e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.765795079100000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.928125650000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.675250000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.081000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.775732062655417998910881298714821053061055705608286949609421120e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.688437299644448784121592662352787426980194425446481703306505899e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.990941408817264621124181941423397180231807676408175000011574647e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.611362716446299768312931282360230566955098878347512701289885826e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.401071382066693821667231534775770086983519477562699643517826070e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.404885497858970433702192998314287586471872015950314081905843790e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.115877029354588030985670444733795075439494699793733843615128537e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.981190790128533233774351539949086864384527026303253658346042487e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.391693345003088328615594164751621620795026048184784616056424156e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.889256530644592752851605934648543064680013184446459552930302708e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.083600502252557317792851907104175947655615832167024966482957198e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.168663303100387254423547467716347840589509950430146037235024663e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.123598327107617380847613820395680616677588511868146055764672247e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.689997752127767317102012222013845618089045780981297513260591263e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.534390868711924145397558028431517797916157184545344400315049888e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.304302698603539256283286371502868034443493795813215278491516590e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.393109140624987047793401361048831961769792029208766436336102130e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.978018543190809154654104033779556195143800802618966016721119650e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.053360999285885098804414279382371819392475408561904784568215676e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.134477518753880004346650767299407142912151189519394755303948278e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.294423222517027804991661400849986263936601088969957809227734095e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.411090410120803602405769061472811786006792830932395177026805674e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.546364324011365762789375386661337991434000702963811196005801731e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.228448949533845774618310075362255075191314754073111861819975658e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.912781600174900095022672513908490962899309128877584272045832513e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.145953154225327686809754524860534768156895534588187817885425867e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.085123669861365984774838320924008647858451270384142925874188908e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.630367231261397170650842427640465271470437848007390468680241668e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.732182596346604787991836614669276692020582495778773122326853797e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.604810530255586389021528105443008249789929772232910820974558737e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.866283281281868197964883431828004811500103664332499479032936741e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.194674953754173153419535571352963617418336620849047024493757781e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.894136566262225941799684575793203365634052117390221232065529506e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.728530091896234109430773225830735206267902257956559214561779937e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.558479853180206010560597094150305393424259777860361999786422123e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.183799294403182487629551851184805610521945574359855930862189385e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.411871423439005125979602342436157376541872925894678545707600871e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.146934230284030660663814250662713645615827253848318877256260252e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.448218665084135299794121636822853382005896647323977605040284573e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.512810104228409918190743070957013357446861162954554120244345275e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.586025460907685522041021408846741988415862331430490056017676558e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.540359114012197595748944623835295064565126012703153392373623351e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.845554430040583564794301575257907183920519062724643766057340299e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.408536849955106342184570268692357634552350288861587703063273018e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.030953654039823541442226125506893371879437951634029024402619056e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.454172918244607114802676127860508419821673596398248024962237789e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.155627562127299657410444702080985966726894475302009989071093439e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.725246714864934496649491688787278190129598018071339049048385845e-13))
      };
      static const T denom[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 0.000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.502622159812088949850305428800254892961651752960000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.430336111272256671478593169569751383305061494947840000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.920361290698585974808779016476219830728024276336640000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.149178946896205138947217427059336370288899808821248000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.374105269656119699331051574067858017333550280343552000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.521316226597066883749849655326023294027593332332429312000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.808864152650289891915479515152146571014320216782405632000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.514810409642252571378917003183814999063638859346214912000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.583350992233550434239775839017811699814141926043903590400000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.478403249251559174520099458337662519939088809134875607040000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.848344883280695333961708798743230793633983609036568330240000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.943873277267014936040757307088314776495222166971439104000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.331069721888505257142927693659482094449571844495257600000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.196124539826947758881834650235619760202156354268084224000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.723744838816127002822609734027860811982593574672547840000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.205691767196054136766333529400075228162139411801728000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.517213632743192166819003098472340901249838381523200000000000000e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.571722144655713179046526371841394014407124514352640000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.359512744028577584409389641902976782871564427046400000000000000e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.949188285585060392916084953872833077002135851920000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.452967188675463645529736303316005271151737332000000000000000000e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.790015208782962556675223159728484084908850744000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.970673071264242753610155919125826961862567840000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.299166890445957751586491053313346243255473500000000000000000000e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.652735578141047520337049888545244673386975000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.508428802270485256066710729742536448661900000000000000000000000e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.093294777021479729147119238554967297499000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.155176275192359061296447275633302204250000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.907505708457079284974986712721395225000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.195848283940498442888394846136646210000000000000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.305934675041764670409270520636101000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.238840089027488915014959267151000000000000000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.846167161648076059624793804150000000000000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.707826341119682695847826052600000000000000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 6.648183019818072129964867660000000000000000000000000000000000000e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.059080011923383455919277000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.490144286132397218940500000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.838362455658776519186000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.970532718044669378600000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.814183952293757550000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.413370614847675000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 9.134958017031000000000000000000000000000000000000000000000000000e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.765795079100000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.928125650000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.675250000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.081000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[47] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.059629332377126683204423480567078764834299559082175332563440691e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.045539783916612448318159279915745234781500064405838259582295756e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.784116147862702971548198855631720823614071322755242269800139953e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.347627123899697763041970836639890836066182746484603984701614322e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.616287350264343765684251764154979472791739226517501453422663702e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -3.713882062539651653939339395399443747287004395732955159091898814e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.991169606573224259776909844091992693404451938778998047720606365e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -3.317302161605094814956529918647229867233820698992970037871348037e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.160243421312714521088457044577429625205805822189897013706603525e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.109943233027050100899811890306430189301581767622560123811853152e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.510589694767723034579229465791750718722450232983242500655372350e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.447631000703120050516586541372187152390222336990410786008441418e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.650513815713423478665128697883383003943391843803280033790640056e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -7.169833252147741984016531016457108860830636610643268300442548571e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.082891222574188256195988224106955541928146669677565424595939508e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.236107424816170540654753273736991964308279435358993150196240041e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.042295614972976540486053879488442847688158698802215145729595300e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -6.301008161384761854991230670333450694872613042265540662425668275e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.626174700692043436308812511757112824553679923076031241653340508e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -7.165638597797307942127436742547456896168876912136407736672893749e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.193760947891421842393017150194414897043594152709554867681454093e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.102566205604210639065160857917396944102487766555058309172771685e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.915816623470797626925445072607835810426224865943397673652473644e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -8.588275837841705058968991523347781566219989845111381889185487327e-16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.200550301285945062259329336559146630395284987411539369061121774e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -3.164333226683698411437894680594408940426530663957731548446585176e-23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.066415481671710192926882432742434212829003971627792457166443068e-28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.794259516500627365643093960688415401054083199354112116216326548e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -4.109766027021453750770079684473469373477285891593627979028234104e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 7.857040454431507009464118652247309465880198950544005451066913133e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.257636833252205356462338019252188768182918234805529456629813332e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.657386968948568677903872677704817552898314429680193647771915640e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.807368757318279512579151153998249666772948741065806312921477647e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.661046240741398691824399424582067048482718145278248186045239803e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.310274358393495831279259654715581878034928245769119610060724565e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.979289812994200254512860775692570111131240734486735844065571645e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -5.374132043246630393307108400571746261019561481928368054130159659e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.807680467889122570534300256450516518962725443297886143108832476e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.273791157694681089776609329544693948790210894828257493359951461e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.971177216154470328027539744763226999793762414262864963697237346e-36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.645869582759689501568146144102914403686604774258048281344406053e-36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.533836765077295478897031652308024155740827573708543095934776509e-37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.011071482407693628614243045457397049948479637840391111641112292e-37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.753334959707221495336088007359122169612976692723773645699626150e-38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -2.217604773736938924265403811396189809599754278055061061653740309e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.819104328189909539214493755590516594857915205552841395610714917e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -7.261124772729210946163851510369531392121538686694430629664292782e-42))
      };
      T result = 0;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (k * dz + k * k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[47] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.201442621036266842137537764128372139686555918574926377003612763e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.185467427150643969519910927764836582205108528009141221591420898e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.424388386017623557963301151646679462091516489317860889362683594e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.527983998220780910263892115033927387104053611029099941633323011e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.966432728352315714505545454293409301356907573727621630702827634e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -4.210921746972897898551337991192707389898034825880579655985363009e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.525319492963037163576188790739239848749059077112768508582824310e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -3.761266399512640929192286468240357629226481512485264527650043412e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.449355108314973517543246836489412427594992113516547680523282212e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.258490177973741431378782429416242097479994678322390199981700552e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.114255088752286384038861754183366335220682008583459292808501983e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.641371685961506906939690430062582517060728808639566257675679493e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.139072742579462987548668350779672609568514018384674745960251434e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -8.129392978890804438983060711164783076784089453197491087525720250e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.227817717944841986447189375517242505918979312023367060292099051e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.401539292067249253713639886818857395065226008969910929456090178e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.181789081601278618540976740818676551399023595924451938057596056e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -7.144290488450459735914078985115746320918090890348935029860425141e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.977643331768050273059868974450773270172308183228656321879824795e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -8.124636941696344229278652214634921673116603924841964381194849043e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.353525462444406600575359080915245707387262742058104197063680358e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.250125861423094782405286690199652039727315544398975014264972834e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.573714720964717327652547152474097356959063887913062262865877352e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -9.737669879005051560419153179757554889911318336987864449783329044e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 4.762722217636305077074994367900679148917691897585712642440813437e-18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -3.587825185218585020252537180920386716805319681061835516115435092e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.209136980512837161314713015292452549173388035330975386269996826e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.034390508507134900778125110328032318737425888723900242108805840e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -4.659788018772143666295222723749466460348336784193790467337277007e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 8.908571128342935499766722474863105091718059244706787068658556651e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.425950044120254934054607924023969978647876123112048584684333719e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.879199908120536747953526966437055347446296944118172532473563579e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -2.049254197314637745167349860869170443784687973315125511356920644e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.883348910945891785870183207161008885784794173754432580579430117e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.485632203001929498321635338807138918181560966989477820879657556e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.018101439657813295290872898460623215815148336073781084176896879e-33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -6.093367832078140478972419022586567008505333455627897676553352131e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 3.183440545955440848970303491445824299419388286256245840846211512e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.444266348988579122529259208173467560400718346248315966198898381e-34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.636484383471871096369253024129613184534143941833907586683970329e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.866141116496477515961611479835778926021343627571438400431425496e-35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 5.140613382384541819628458619521408963917801187880958447868987984e-36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -1.146386132171160390143187663792496413753249459594650450672610453e-36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 1.987988898740147227778865012441676866493607979490727350027458052e-37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -2.514393298082843730831623322496784440966181704206301582735570257e-38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, 2.062560373914843383483799612278119836498689222815662595453851079e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 201, -8.232902310328177520527925464546117674377821202617522000849431630e-41)),
      };
      T result = 0;
      T z = dz + 2;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (z + k * z + k * k - 1);
      }
      return result;
   }

   static double g() { return 2.880805098265409469604492187500000000000000000000000000000000000e+01; }
};
//
// Lanczos Coefficients for N=49 G=3.531905273437499914734871708787977695465087890625000000000000000000000000e+01
// Max experimental error (with MP precision arithmetic) 0.000000000000000000000000000000000000000000000000000000000000000000000000e+00
// Generated with compiler: Microsoft Visual C++ version 14.2 on Win32 at May 23 2021
// Type precision was 234 bits or 72 max_digits10
//
struct lanczos49MP : public std::integral_constant<int, 234>
{
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[49] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.019754080776483553135944314398390557182640085494778723336498544843678485e+75)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.676059842235360762770131859925648183945167646928679564649946220888559950e+75)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.735650057396761011129552305882284776566019938011364428733911563803428382e+75)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.344111322348095681337661934126816558843997557802467558098296633193647235e+74)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.279680216265226689865713732197298481387164441051031689408254603978998739e+74)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.534520884978570988754896701605114795240254179745381857143817149573644190e+73)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.094111428842887690996413081740290562974159836498138544655694952917058279e+73)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.810579629726532069935485995735078752851873354363515145898406449827612179e+72)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.558918434547693216059693184449337082460551934950816980580247364223027781e+71)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.135873934032978624782044873078145389831812962597897650459872577452106819e+70)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.371686637133152315568960647279607142944608875215381633194517073295482279e+69)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.210709791290211789451664416279176396010610028867877916229859938579263979e+68)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.728493971517374186383948573740973812742868532549695728659376828743835354e+67)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.082196640005602972025690170863702787506422900608580581233509996818990072e+66)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.434296904963427729210616126543061543796855074189151534322145897294331943e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.956561957668034323532429269801091280845027370525277092791205986418388937e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.088465031117387833989029481250197681372395074774197408003458965955199114e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.638006253255673292005995366039132327048285444095672469721651872147026836e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.251091350064240218784436940525650498454117574503185703454335896105827459e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.438965470269742797671507642697325276853822736866823134892262314489634493e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.171096428020164782492194583597894107448038723781192404665010946856416728e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.967964763783296071512049792468354332039332869323402055488486515880211228e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.691239511306362286583973595898917873397629545053239582674237238671075149e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.371351678345135454487045322888974392983710298168736348320865063481886470e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.399139456864000364822305296187724318277750756512114883045860699513780982e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.335644907596902422235465624341780552277299385700659659374735347476790554e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.541494306852766687136536628805359613169443442681232300932385167150904206e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.454056970723993494338669836718832149990196703371093557999871225274488103e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.122804036582476347394286398036300142213667443126058369432667730381406969e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.350437044906396962588019269622122085882249454809361766675217669398941902e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.350870345849974415762276588597695308720927596639430565914494820338490132e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.751385019528115548010259296564491721747559138159547937603014471132985302e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.476113159838159765344429146854859477076267913220368138623944281045558949e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.294398437121931983845715884547145127710330599817550100565429763115048575e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.839781945206891229035081139535266037057033378415390353746447012903101485e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.711123560991297649877080280435694393772679378388200862936651880878974513e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.629549637595714724789129505098155944312207076674498749184233965222505036e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.574490956477982663124058935512682291631619753616365947349506147258465402e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.813448512582564107136706674347542806763477703915847701777955330517207527e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.794883897023788035285405896000621807947409236640451176090748912226153397e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.960479107645842137800504870276268649357196518321705636965540139043447991e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.584873693858658935208259218348427715203386574579073396812006362138544628e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.245936834930690813978996898166861135901321349975150652784783993349872251e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.773017141345068472510554017030953209437181800565207451480397068178339458e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.641092177320087625773994531564401851985558319657745034892144486195046163e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.105900602857710093040451403979744395599050533242553243284994146810134883e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.764831079995208797424885873124263386851285031310243195313947355076198006e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.390800780998954208500039666019609185743083611214630479125238184115750385e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.506628274631000502415765284811045253006986740609938316629923576327386304e+00))
      };
      static const T denom[49] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 0.000000000000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.586232415111681806429643551536119799691976323891200000000000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.147760594457772724544789095126583405046340554378444800000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.336873167741057974874912069439520834275222024827699200000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.939317717948202275053279980882650292343063152909352960000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.587321266207338310075066414082486631849657629849681920000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.708759679197182632355739853743909528366304368999677296640000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.853793140116069180377738686747691213170064352110549401600000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.712847307796887697739638943011607706661342285570961571840000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.289323070446191229806483814370209648903283093834096836608000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.773184626371587855448424329320482554352785932897781894348800000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.435061276344423987072041299926950982073631843385358712832000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.038454928643566553335326814205831024316152779380233211904000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.839990097014298964461251746728788062040820983609914982400000000000000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.354892309375504992458915625473361082395092049509521612800000000000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.297725282262744672148100400166565576520346155229059072000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.209049614463758144562437732220821438434464881014066944000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.403659584108905732081564809222007746403637980496076800000000000000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.460430771262504410833767704612689276896332359898060800000000000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.366143204159002782577065768878538489390347732147072000000000000000000000e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.152069768627836143111498892510529224478160293107040000000000000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.778134072359739526905845579458057851415301312320000000000000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.054274336803456047167091188388392791058897181680000000000000000000000000e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.785217864372490349864295597961987080566291959200000000000000000000000000e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.147675745636024418606666386969855430516329329000000000000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.106702410770888109717062552947599620817425600000000000000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.181697115208175590688403931524236804258068000000000000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.204691425427143998305817115095088274690720000000000000000000000000000000e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.522623270425567317240421434031487657474000000000000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.991703958167186325234691030612357960000000000000000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.527992642977421966550442489563632412000000000000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.749637581210127837980751990835613680000000000000000000000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.178189516884684460466301376197071000000000000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.691582574877344639525149014665600000000000000000000000000000000000000000e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.588845541974326926673272048872000000000000000000000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.832472360434176596931313852800000000000000000000000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.162585907585797437278546956000000000000000000000000000000000000000000000e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.759447826405610148821312000000000000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.354174640292022182957920000000000000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.764512833139771127128000000000000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.823199175622735427100000000000000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.478468141272164800000000000000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.842713641648132000000000000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.137488170420800000000000000000000000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.672014134600000000000000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.194862400000000000000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.183320000000000000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.128000000000000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.000000000000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[49] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.256115936295239128792053510340342045264892843178101822334871337037830072e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.226382973449509462464247401218271019985727521806127065773488938845990367e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.954125855720840120393676022050001333138789037332565663424594891457273557e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.365654586155298475098646391183374531128854691159534781627889669107191405e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.044730374864121201936514442517987939299764008179567577221682561782183421e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.536356651078758500509516730725625323443004425012359430385110182685573948e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.014086778674585662580239648780048641118590040590185584314710995851825637e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.297512615100351568281310997196097430272736169985311846063847409602541101e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.172699484909110833455814713100736399837181820940085502574724938707290372e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.437106278000765568297205273500695563702563420274384149002742312586130286e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.545174371038470657228461270859888251519093095798232203286784662979129719e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.471402006255895070535216946049289412987253853074246902793428565040300946e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.250412452299054568666234182931337401773386777590706330586351790579683785e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.542277292811232416618979097150690892713986488440887554977845301225180167e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.573086578098175124839634461979173468237189761083032074786971884241523043e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.104607420271440962257749252277010146224322860594339170999893725175800398e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.331938462909285706991247107187321645123045527742414883815000495606636547e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.208943799307442534797422457740696336724404643783689597868534121046756796e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.733493346199244389057953770564978235470425412393741328222813802783023596e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.492565577437473684677047557246888601845840521959370888739670894941330993e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.949686666262532681847746981577965659951530620900061798663932175932503947e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.651553737747111429808666993553840522239787479567412806226997545642698201e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.233339502372167653077823100186702309252932859938068579827741608727620372e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.836417632015534931979173072268670137192644539820843175043472436022533106e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.099476078371695988128239701316213720767789442435824569996795481661867708e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.903495249944998411669955533495843613032560579446280190215596256375769138e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.064350138053898858268455142115215057417819864153422362951139202939062254e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.582922994233975143266417356893351025577967930478759984258796518122317112e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.264234026390622585348908134631186844315412961960282698027228844566912117e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.188774153889105680535092985608022230760508397240005354866271201170463092e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.077355341399798609786211120408446935756994848842131485074396331912972263e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.719182300108746375704686202821491852452619639378413670885143111968297622e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.509589596583338412898049035294868361167322854043341291672085090640104583e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.342872197271389972201661238004898097226547537612646678471852083509061055e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.051089551701679946626603411942133093477647785693757342688765248578133558e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.450407423742747934005020832364099630124534867513398798024857849571995697e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.413023778896356322547226273989314105826233010702360664846872922408122652e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.929512168994516762673517469057078217752079550440541815733308359698355209e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.122462776963280343259527700358220850883119067227711325233658329309622143e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.197396290684303702682694949348422316362810937601334045965871833844532390e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.356726450420886407112529306259506900505875228060535982041959265851381932e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.263148912218322814862431970673685825699419657740452507847522864799628960e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.320663245567314255422944457710191091603773917350053498982455870821336578e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.270816501767199044429825237923512258332267743288560304635568302760089360e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.960034133400021433681975737071902053498506976351095156717280432287769760e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.650907660437171004901238983264357806498757360812524606971708594836581635e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.725344352001816640635025885398718044955247687225228912342703408863775468e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.012213341659767638341287600182102653785253052492980766472349845276996656e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.148735984247176123115370642724455566337349193609892794757225210307646070e-15))
      };
      static const T denom[49] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 0.000000000000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.586232415111681806429643551536119799691976323891200000000000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.147760594457772724544789095126583405046340554378444800000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.336873167741057974874912069439520834275222024827699200000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.939317717948202275053279980882650292343063152909352960000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.587321266207338310075066414082486631849657629849681920000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.708759679197182632355739853743909528366304368999677296640000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.853793140116069180377738686747691213170064352110549401600000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.712847307796887697739638943011607706661342285570961571840000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.289323070446191229806483814370209648903283093834096836608000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.773184626371587855448424329320482554352785932897781894348800000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.435061276344423987072041299926950982073631843385358712832000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.038454928643566553335326814205831024316152779380233211904000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.839990097014298964461251746728788062040820983609914982400000000000000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.354892309375504992458915625473361082395092049509521612800000000000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.297725282262744672148100400166565576520346155229059072000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.209049614463758144562437732220821438434464881014066944000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.403659584108905732081564809222007746403637980496076800000000000000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.460430771262504410833767704612689276896332359898060800000000000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.366143204159002782577065768878538489390347732147072000000000000000000000e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.152069768627836143111498892510529224478160293107040000000000000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.778134072359739526905845579458057851415301312320000000000000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.054274336803456047167091188388392791058897181680000000000000000000000000e+47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.785217864372490349864295597961987080566291959200000000000000000000000000e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.147675745636024418606666386969855430516329329000000000000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.106702410770888109717062552947599620817425600000000000000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.181697115208175590688403931524236804258068000000000000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.204691425427143998305817115095088274690720000000000000000000000000000000e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.522623270425567317240421434031487657474000000000000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.991703958167186325234691030612357960000000000000000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.527992642977421966550442489563632412000000000000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.749637581210127837980751990835613680000000000000000000000000000000000000e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.178189516884684460466301376197071000000000000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.691582574877344639525149014665600000000000000000000000000000000000000000e+32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.588845541974326926673272048872000000000000000000000000000000000000000000e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.832472360434176596931313852800000000000000000000000000000000000000000000e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.162585907585797437278546956000000000000000000000000000000000000000000000e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.759447826405610148821312000000000000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.354174640292022182957920000000000000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.764512833139771127128000000000000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.823199175622735427100000000000000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.478468141272164800000000000000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.842713641648132000000000000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.137488170420800000000000000000000000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.672014134600000000000000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.194862400000000000000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.183320000000000000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.128000000000000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.000000000000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.233965513689195496302526816415068018137532804347903252026160914018410959e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.432567696701419045483804034990696504881298696037704685583731202573594084e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.800990151010204780591569831451389602736047219596430673280355834870101274e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.648373417629954217779547889047207255669324591553480603234009701221311635e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.284437059737535030909183878223579768026497336818714964176813046702885009e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.107670081863262975759677889098250504331506870772724719160419469778560968e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.504360049237893454280746661699303420195288960483588101185311510511921407e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.611963690317610801367925234041815344408602188860817148261094946859641055e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.384196825969374886217890731633708081987015968697189525652444057097652970e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -9.622790278065968351142661698209916105231451436587332112667311309898112907e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.449580263451402816852387882691399098166718792531955596134468390704873784e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.521891259305373384442177581056279631601936059906718384076120380236152660e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.540905879286304237452585078525021002948388724011947750659051968465604420e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.945234350579576646146368625320015500721771847656877764914364796999018932e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.387416831492605275144126841246803690906548552137287238128485938487779304e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.495798927786732788454640929952042349030889163403974404914516146280530443e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.422943482445615791699986810123527175351383953692494761445061153868299970e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.102903565532198274392276413606953369815588940855811878456066484772851432e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.094535028891646496546262084074169534843273855151767819779938179094208188e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.358095366769232988323350838247191988585424776577310286935330610243011743e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.340530976890392038064297300042596231921772020705486053763028541479656280e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -5.516126555541810552632615941497264105370152230590961486150754875983890898e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.581070770182358530034488215134552244420314579258249233845063889274308385e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.004041803560396287949218893554883846494476801309699018606217164405518118e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.544940513373792201443764129828570298376651506143103507977578771056330357e-11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.379886756316120706725450388823927894039691498002769785120646242635823255e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.045944766231461555774689284231476775257484866624431880423176393512207387e-16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.148008349481903710728852060629194581596537318553456161574664356163226596e-18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.275939213508899016428747078031525836021120921513924955762486505940940452e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -5.002074313199153828387282409848698339315167131865367803807278126600553143e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.312111841788085794021549817149806212210409562054999023338937587332395908e-31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -5.626943187621427535665396572257733498961554813667830477603994486170242725e-38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.301342661659868689654008011518619781951990528882947953585638647826710208e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.943206436729179344584590804980950978078020765463364229230497237642990035e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.427985516258891942239250982488681113868086410604389604401061682071089711e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.507315406936486999900039292673781706696418133992677762607195734544800586e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.128602212783277628616662270027724563712726585388232543952969868791288808e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.491772665541813784561971549986243397968726526748411110109050933398474269e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.703534948980953284999107504304646577995487754638088095508324603995172100e-41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -4.254734374358011114570777363031532176229997594050978074992481078060588238e-41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.744400882431682663010546719169103111178937911186079762496396144974231758e-41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -5.962681142344559707919656870822506290159603512929253279249408502104315114e-42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.676028721091677637759913537058723470799950761402802553076562435074866681e-42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.784145301171889911387402141919770031244273301985414474074193734901497838e-43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.609382025715763421062067113377305459236198785081684246167946524325632358e-44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -8.390549764136377795654766730143252213193890014751189315315457960953487009e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.892571860428412953244204670046307154753124542150699703190076405369134986e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.750996769906711001487027901108989269217518777400665423098451353536180397e-47))
      };
      T result = 0;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (k * dz + k * k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[48] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.614127734928823683399031924928203896697519780457812139739363243361356121e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.873915620620241270111954934939697069495813017577862172724257417200307532e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.020433263568799913803105156119729477192007677199414299858195073560627451e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.464288862550385816890047588667703388387049707546055857832236105882063068e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.220557255453323997837181773609238378313171107809994660932186398177260086e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.448922988893760252035044756495535237100474172953062930583429032976738764e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.967825884804576295373543809345485166700533400342174877153161242305801190e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.108580240999530194943835615053920860235079936071561387017856553284307741e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.810642568703399030220299393722324927217417866857948859237865608524037352e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.258739608434628125323282855631713548863857229929349389847042534864564944e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 7.128496339139347963949841917163532287235353252857530075762245657435637432e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.298839862995292241245807816785011319551976707839297599695370993581498944e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.248028459892634227909925496833936295152351078572924268478121601580013392e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.852607223132643180520259993802350485519940462836224552121831884016060957e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.663344935420648740647632834149137036410170363282848810726789852512040974e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.956627238308290890004850312580119602770863155061517303602115059073018915e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.169408084580850119987259593698335014334975914638255180200227109093248923e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -4.058851441448441752395908342271882717629330686791993331066549580482119207e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.047904711622988136321913253467026391205138759157051345098701122533901641e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.084581449711468658856874852797634685668605539676219540421511092086784257e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.753524069615947925605286028020036313127948537531901833841745089038345419e-03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -7.215544327537876774584851905154058374732203881621234998864896154317725107e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.068169776809028066872463505146223239383722607505095101927453621607143258e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.929532177536816530254915957100118239727512976366989010196601003141928983e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 4.637071893688824301043677755554700799944720607401583032240419889530137613e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.113086368090515818656083953394041201305628629939937324183681568207995992e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.052475329075586591944149534403412432330839345810529801140374324740860043e-14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.501688739492061913271178171928421774535209059448213231063121542726735922e-17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 6.901359655394917555561825304708227606591126162154019191593957327845941711e-21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -6.543121984804052902464306048576584437634232699686221158393201347605610557e-25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 5.640594686585611716813857202286477090235259523435190405734068397561049518e-30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -7.360501538535076761830231649645320930651740695652757112015168140991871811e-37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.702262550718545671102818966852852449667460777654069395734978498316238097e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -2.541872823365472358451692936741339351955915607098512711275660224596475692e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 3.176003476857354088823169817786505980377842772003301806663816129924483628e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.279773456918425628141724849602711155924011970103858226014129388572719403e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.784385649492108496180681575852127700091310578360977342260193352159187181e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.951360558254817806983487356566692526413661637802019026539779594905551108e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 1.138493498985334706374097781200574458692873800531006014767134080212447095e-39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -5.565540270144127561133019747139820473778629043608866511009741796439135919e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.281818911412862056602174736820614290615384940807745468773991962352596562e-40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -7.799674220733213250330301520376602557698625350979839820580611411813901622e-41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 2.192382536820850868749995412323169490150589703968805623276375892630104761e-41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -4.949971304595638285364920838869349221534754917093372730956379282259345856e-42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 8.645611826340683772044616248727903964535937439036743879074020086333544390e-43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -1.097552479007470044347986004697357054639788223386746160457893283010512693e-43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, 9.016047273189589762707582112298788030798897468010511171850691914431226857e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 234, -3.598528593988298984798384438686079221879557020145063999565131046963034260e-46)),
      };
      T result = 0;
      T z = dz + 2;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (z + k * z + k * k - 1);
      }
      return result;
   }

   static double g() { return 3.531905273437499914734871708787977695465087890625000000000000000000000000e+01; }
};

inline double lanczos_g_near_1_and_2(const lanczos49MP&)
{
   return 33.54638671875000;
}

//
// Lanczos Coefficients for N=52 G=4.9921416015624998863131622783839702606201171875000000000000000000000000000000000000e+01
// Max experimental error (with MP precision arithmetic) 0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000e+00
// Generated with compiler: Microsoft Visual C++ version 14.2 on Win32 at May 22 2021
// Type precision was 267 bits or 82 max_digits10
//
struct lanczos52MP : public std::integral_constant<int, 267>
{
   template <class T>
   static T lanczos_sum(const T& z)
   {
      static const T num[52] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.2155666558597192337239536765115831322604714024167432764126799013946738944179064162e+86)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.4127424062560995063147129656553600039438028633959646865531341376543275935920940510e+86)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2432219642804430367752303997394644425738553439619047355470691880100895245432999409e+86)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0716287209474721369403884015994122665163651602768597920624758793936677215462844844e+86)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6014675657079399574912415792629561012344641595734333223485162579517263855066064448e+85)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.9469676695440316675866392095745726625355531618465991865275205877617243118858829897e+84)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.6725271559955312030697432949232367888201769834554225137624859446813736913045818789e+83)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.9780497929717466762906957902715326245615108291247102827737801883575021795143342955e+82)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1101996681004003890634693624249367763382356608502270377869975360325542360496573703e+82)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0730494939748425861290791265310097852481749101609248058586423168275758843014930972e+81)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.1172053614337336389459663390209768009612090987295451850767107564157925998672033369e+79)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.8745706482859815560001340912167784132864948819270481387088631069414787418480398968e+78)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.6357176185157048005266104227745750047123126912102898052446617690907900639513363491e+77)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8133959284164525780065088214012765730887094925955713435258703832914376223639644932e+76)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5448240429477602353083070839059087571962149587434416045281607922549720081768160509e+75)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.7087163798498299832185901789792446192843546181020939291287863440328666852365937976e+73)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.5087881343681529988104227139117041222747015656224230641710513581568311157611360164e+72)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4613990031561986058105893353533330535471171288800176856715938119122276074192401252e+71)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.5842870550209699690118875938391296069620575640884684638841925128818533345720622707e+69)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.9620974233284411294099207169175001352700074635064713292874723705223947089062735699e+68)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.3508670578887212089913735806070046196943428333276779526487779234259460474649929457e+66)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.8965656059697746722951241391165378222796048954133067890004635351203115571283351365e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.2318932193708961851167812069245962898666936025147808964284039760031905048392189838e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3345087899695578368548637479429057252399026780996532026306572668368481753083446035e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1496377806602727082710361157715063723084394483433707624016834119253720339362659524e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.8813988021021593903128761372635593523664342047294885462297735355749747492567917846e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3920956168650489448209669451137968296707297452484593173253158710325555644182737609e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6075799130233642952967048487438119546115615639287311948474085370143960495152991553e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.5215062065032735365892761801093050964342794846560150713881139044884954246649618411e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.2544625196666558309461059934941389307783421507071758131936839820683873609174384275e+51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0762146929190267795931737389984769617036500590695127135611707890269647516827819385e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4748530311527356370488285932357594780386892716488623785717991900210848418789146683e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.8647515097879595082625933421641550420004206477055484917914396725134286781971556627e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1719649433541326415406560692060639805792580579388667358862336598215919709516402950e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.3261545166351967601378626266818200343311913249759151579277193749871910741821474852e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2856597199911108057679518458817252978491416960383076400280909448497251424577297828e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0550442252418806202988575468283015522717243147245311227478147425985867569431393246e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6853992677311201643273807677942992750803865734634293875367989072577697582186788546e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2561436609157539906689091056055983931948431067099841829117491622768568140084478954e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.4705206401662673954957473131998048608819946981033942322942114785407455940229233196e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.1407133281903210341367420024686210845937006762879607281383052013961468193494392782e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.7901285852211406723998154843259068960125192963338996339928284868353890653815257694e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3438998631585891046402379167404007265944767353624748363650936091693896699924614422e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.6902706401090767936392657986442523138728548699749092883267263725876049498265301615e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0929659841192947874271802260842934838560431084502779238527946412041140734245143884e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.5862712170406726646812829026907981494912831615106789457689818325123973212320279526e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7379005522986464990683100411983090503131358664627225315444053238030719367736285441e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.7401492854176051649551304067922101744785432102294852270917872876250420006402939364e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.3052097444388598287826452442622724171650622602495079541553039504582845036611195233e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.8093796662195533917872631136981728293723308532958487302137409818490410036072819019e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.3192906485096381210566149918556620595525679738152760526187454875638091923687554946e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.5066282746310005024157652848110452530069867406099383166299235763422936546004304390e+00))
      };
      static const T denom[52] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.0414093201713378043612608166064768844377641568960512000000000000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3683925049359750564345782687270252191318781054337155072000000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8312047394413543873001574618939688475496532684433218600960000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.6266290361540649084000356943724186480051615706407501824000000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2578261522689833134479268958172001145701798207577980403712000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2001844261052005486660334218376501837226733355004196185702400000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1681037008119350981332433342566749327534832358109654944841728000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.0293361153311185534392570196926631029364162024577328008396800000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7968291458361430782020246122299560311802074147902210076049408000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.4212998306869600977887871207212578754682594793002122395254784000000000000000000000e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4006322967557247180769968530346138316658911433773347563153653760000000000000000000e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1334912462852682149761710693821775975226278702191992823808000000000000000000000000e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.1263921790865468343839418571823409266633338824655665334886400000000000000000000000e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0548002159482240692664043366538929906734975613031337827840000000000000000000000000e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6095781466700764043324234378972985924892034584990590768742400000000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1887214284827766716471402753528692603931747042835394432000000000000000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6644926572075096083148238618984385847884240529010940198400000000000000000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9154102380883742873084802432628398856163736124576909120000000000000000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8768151045628896730547232493410634338494669305466040192000000000000000000000000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.5674503027583263140245049650089911892130421780961760000000000000000000000000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0774565729992714117801952876016228015049549176491224000000000000000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5271995659293168127377699172748774995796493494870600000000000000000000000000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0217297271367563021376459886512004721472442416486880000000000000000000000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.2295402510227377004563262212164474005108576587500000000000000000000000000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.4652078765044198452095090589463630638929867781650000000000000000000000000000000000e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7599751702378955591170076678443001141850220448750000000000000000000000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.1661954970720573655661780303655361431161958585000000000000000000000000000000000000e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.4624589782073664468902246801624082962588775000000000000000000000000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3415303241063823936930939721131813816093940000000000000000000000000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.7484118887814252101652801793318408875609500000000000000000000000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5345701242523770267594030980609724717749800000000000000000000000000000000000000000e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.5242293875075726230709587746676742825000000000000000000000000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2153799706792737162996155167868591485000000000000000000000000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9704836232659058554494106146940431250000000000000000000000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.5926306456751344865378122278650335000000000000000000000000000000000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3255133142885196993084362383550000000000000000000000000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.4074634262098477202456261501600000000000000000000000000000000000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9362537824702021303895557050000000000000000000000000000000000000000000000000000000e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.7695574975175958167624223800000000000000000000000000000000000000000000000000000000e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.5430949131153796097540000000000000000000000000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.7427444047045863749135000000000000000000000000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.9163115009072256171250000000000000000000000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.9274383168492884295000000000000000000000000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.0731790610548750000000000000000000000000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9491312919646000000000000000000000000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1366198225750000000000000000000000000000000000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.3570498490000000000000000000000000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1862250000000000000000000000000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.9135000000000000000000000000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2750000000000000000000000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0000000000000000000000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }

   template <class T>
   static T lanczos_sum_expG_scaled(const T& z)
   {
      static const T num[52] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2968364952374867351881152115042817894191583875220489481700563388077315440993668645e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3379758994539627857606593702434364057385206718035611620158459666404856221820703129e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.7667661507089657936560642518188013126674666141084536651063996312630940638352438169e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2358817974531517479015567958773172164495426366469934483861648449503257164430597676e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.4277884337482721045863559292777143585727342521289738221346249419373476553706960477e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0321517843552558179026592998573667333331808062275687745076218349815677074985963362e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6008215787076020416349896191435806440785263359504290408274535992043818825469757467e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0818534880683055883178218002277669005830432150690130666222387190067193078388546735e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.3163575041638782544481444410280012073460223217514438140658322833554580013590099428e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2388461455413602821779255933063678852868399289922539164611151211712717693487661387e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.9022440433706121764851738708000810548525373575286185895490944900856672029848122464e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4343332795539887118781806263726922877784774015269102474184340994778721415926159254e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.6721153873219058826376914786734034750310419590675042405183247501763477751707551977e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.8699628167986487178009534272364155924157279822557444996249799757685987843467799627e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2231722520846745745100754786781488061001048923809737738337722261187091800649653747e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6083722187098822924476598339806607838145759265459316214837904824198871273160389130e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.3208522386531907691637761481331075921814846045082743370064300808114976199945037122e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.0491114749931089548713463433723654679587396917777106103842119656518486449033314603e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1651242201716492163343440840407055970036142959703118121095709130441296636187948680e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.0937853081905503955154539906393844955264076759979726863680373891843152853217317289e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3250660210211249438634406634827812076838178133500698949898645633350310552370070845e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9570575453729198045577937136948643736522682229986841993417806493265120889607818521e+43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0915995985127530070498760979124015775616748288243587047367665303776535820627821806e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.7843635148151485646319955417265990806303645948936943738048724343049029864512194310e+40)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.5715089981189796608580098957107087883056675964697423692398096552244865201126563742e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4357579282713452366310121777478478833619311278663562587227576970753556113485917320e+37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9045145853415845950075255530781663873014618840017618405203106705648527948595759022e+35)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.4405414384364870662900944749199576151689786973539596080446881216688775114111401244e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.4338208995125086931437020109275839003754703899452696411395381222142064582727164193e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5135951828248789401437398422210292739440102976394266807495518051141687446206426902e+30)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2454501218684993019799803589484969919745689383924985827556442495293105857928357346e+28)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.0771824063818140076215034706980442595962662610399835096143334152833816531221628660e+26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.8906795572088352821870480514365587413988764084322141604846302060067104077934035979e+24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.5316598805398286987903918997966274192433403859480290402541541527289805994033568970e+22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.8533661333839969582529636915290426021792269810841946250308753325085094758818317464e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.7688764431225908123092120518561018783609436337842987641148647520393460757258932651e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.2877126063932361367573287637428749753108811218139980724742491890986894871059180965e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.5164730755154849065370365582371664006783859248354506763822538105452920154803662025e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6208599037402680475080806960664748732421517587082652352333314249266933718172853538e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7673175927530323929887314159232706831613273850912577130092484125942772196619237882e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0725755228231653548395606930605822556041123469753556193146323302002970112193491322e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.8214170582643892131251904381692606071847290625764371754232909841174413907293095377e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8039573621910762166001861467061356387294777080327106241940313632291985560896860294e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1872369877837204032074079418146123055615124618000942960799358353269625124323889098e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.3668338250989578832466315465536077672534713261875481920848252453271529995224516632e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3741815275584317532245789502116967720634823787379160467839166784058598703361284195e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.6260135014231198723283200126807995474827979567257597700738272144468899515236872878e-07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.8035718374820798318368374097127140964803349360705519156103446564446533105562174708e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3155399273220963343988533731471377905809634204429087235409329216539588313353641052e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6293749415061585604836546051163481302110136557722031417335367202114639560092787049e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3184778139696006596104645792244972612333458493576785210966728195969324996631733257e-18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.2299125832253333486600023635817464870204660970908989075481425992405717273229096642e-22))
      };
      static const T denom[52] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.0414093201713378043612608166064768844377641568960512000000000000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3683925049359750564345782687270252191318781054337155072000000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8312047394413543873001574618939688475496532684433218600960000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.6266290361540649084000356943724186480051615706407501824000000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2578261522689833134479268958172001145701798207577980403712000000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.2001844261052005486660334218376501837226733355004196185702400000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1681037008119350981332433342566749327534832358109654944841728000000000000000000000e+65)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.0293361153311185534392570196926631029364162024577328008396800000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7968291458361430782020246122299560311802074147902210076049408000000000000000000000e+64)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.4212998306869600977887871207212578754682594793002122395254784000000000000000000000e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4006322967557247180769968530346138316658911433773347563153653760000000000000000000e+63)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1334912462852682149761710693821775975226278702191992823808000000000000000000000000e+62)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.1263921790865468343839418571823409266633338824655665334886400000000000000000000000e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0548002159482240692664043366538929906734975613031337827840000000000000000000000000e+61)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6095781466700764043324234378972985924892034584990590768742400000000000000000000000e+60)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1887214284827766716471402753528692603931747042835394432000000000000000000000000000e+59)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6644926572075096083148238618984385847884240529010940198400000000000000000000000000e+58)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9154102380883742873084802432628398856163736124576909120000000000000000000000000000e+57)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.8768151045628896730547232493410634338494669305466040192000000000000000000000000000e+56)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.5674503027583263140245049650089911892130421780961760000000000000000000000000000000e+55)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0774565729992714117801952876016228015049549176491224000000000000000000000000000000e+54)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5271995659293168127377699172748774995796493494870600000000000000000000000000000000e+53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0217297271367563021376459886512004721472442416486880000000000000000000000000000000e+52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.2295402510227377004563262212164474005108576587500000000000000000000000000000000000e+50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.4652078765044198452095090589463630638929867781650000000000000000000000000000000000e+49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.7599751702378955591170076678443001141850220448750000000000000000000000000000000000e+48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.1661954970720573655661780303655361431161958585000000000000000000000000000000000000e+46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.4624589782073664468902246801624082962588775000000000000000000000000000000000000000e+45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3415303241063823936930939721131813816093940000000000000000000000000000000000000000e+44)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.7484118887814252101652801793318408875609500000000000000000000000000000000000000000e+42)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.5345701242523770267594030980609724717749800000000000000000000000000000000000000000e+41)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.5242293875075726230709587746676742825000000000000000000000000000000000000000000000e+39)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2153799706792737162996155167868591485000000000000000000000000000000000000000000000e+38)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9704836232659058554494106146940431250000000000000000000000000000000000000000000000e+36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.5926306456751344865378122278650335000000000000000000000000000000000000000000000000e+34)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3255133142885196993084362383550000000000000000000000000000000000000000000000000000e+33)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.4074634262098477202456261501600000000000000000000000000000000000000000000000000000e+31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9362537824702021303895557050000000000000000000000000000000000000000000000000000000e+29)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.7695574975175958167624223800000000000000000000000000000000000000000000000000000000e+27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.5430949131153796097540000000000000000000000000000000000000000000000000000000000000e+25)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.7427444047045863749135000000000000000000000000000000000000000000000000000000000000e+23)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.9163115009072256171250000000000000000000000000000000000000000000000000000000000000e+21)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.9274383168492884295000000000000000000000000000000000000000000000000000000000000000e+19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.0731790610548750000000000000000000000000000000000000000000000000000000000000000000e+17)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9491312919646000000000000000000000000000000000000000000000000000000000000000000000e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1366198225750000000000000000000000000000000000000000000000000000000000000000000000e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.3570498490000000000000000000000000000000000000000000000000000000000000000000000000e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1862250000000000000000000000000000000000000000000000000000000000000000000000000000e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.9135000000000000000000000000000000000000000000000000000000000000000000000000000000e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2750000000000000000000000000000000000000000000000000000000000000000000000000000000e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0000000000000000000000000000000000000000000000000000000000000000000000000000000000e+00))
      };
      return boost::math::tools::evaluate_rational(num, denom, z);
   }


   template<class T>
   static T lanczos_sum_near_1(const T& dz)
   {
      static const T d[56] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4249481633301349696310814410227012806541100102720500928500445853537331413655453290e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.9263209672927829270913652941762375058727326960303110137656951784697992824730035351e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2326134462101140657073655882621393643823409472993225649429843685598155061860815843e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.9662801801612054404095225935108977904002486830482176026791636595192650184999106786e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4138906470545941456294493142170199869989528110729651897652377168498087934667952997e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.0258375230969759913527502498295624381557778356817817750999982139142785355759733840e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.0558030423043855628646211274492485894483342086566824594162146024985516781169314244e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -6.7626679132766782666656123523498939281680490327213627146988312255304416262392894908e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.3671346711777066286093979449135095463576878628561846047759456811238250487006378990e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -6.6153175690992245402186127652781399642963298842199508872954793356226534605339323333e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.9373755602608411416894250529851681229919578866115774473369305562033628341735461195e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.0800169087178819510009898255169517991710412699732186488007608833012065028092686003e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.6113163429881357240014185384821233436360839107514932109343845514870210427965645190e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -7.0800055208994526950912019754052939899262033086446277779400918426435279835354194130e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.6124888869258097801249338962341633267998552553797818622228028001697157538310910762e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -8.0821345203062947277822243784585588745042841720677807798397954250617939305106506107e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0897234684613304316686535121178451999373954297009955842614973223259353042645941321e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.4950083481830885847356672064097545823284701899839135264776743195466249025042048810e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.9937323326320843218449977911798288651196634496462091472078054583251005505547883394e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.1659790383945267871585567047493689107693027444469426401770619301894048344984407423e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.3811750187329199929662456874823737031712476205965338135458988288637511665389967467e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.3126057597099554726738230571307233576246752870932752732642137542414937991213738675e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.8603278118738070120786476302797971799214428999935477462676132231556636610008100990e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -5.7497497499750147559650543128496209797619661773802614023013669364107360500260024694e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.5455669051693660429433444114100199274899990967149528799831986363426701813268631682e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -8.3264707731119706730021053355613906861401420453549875829176477978488700910763350933e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.9452716333766656109625805591981088427443890665155986807469770654997947109460588842e-09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.1107270143427404085533822649150071785436589803547989849458232547054059840123879700e-11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2247407103283988605835624937345007318815870047594787786390767998944461846883100894e-13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.0188429331847134597398824340892444962476368435762668313929678602451262836436731615e-15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.9431143198672109701045920614322844783143358661530851155605617982604226639477903946e-18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.1511473493622067561750500375000264321547190996454590396416983138367840485620637483e-20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.6282967029324804059233959451615215356643254329321661581764544172041721688288262211e-24)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.5217584051959451141711566663295724419583710296958056943098481375601920843969902641e-27)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.7686779496951341450129898316334506462294006086177935245787611395642697303462481134e-32)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.8183279898437068462121010262051285364082674967144808177521395570040237794774928143e-37)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.2301356317533360807190133199588525842189842444453020346114472115201441728418314131e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0538087755476779873724252308339637373681982420524344896439951953765025296864700289e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.7368861518094102628071448870890238982875711317443189777819779346097156832497387210e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.2127031296187719256650789211253726767816659542004125362087789737915250037588745613e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.2845218026163010098046208410081722573197086178079121337275774350592686815176989662e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9013371187879239559027439498823204927228357549709009148789220121477613502450061401e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.2089862298013055403993867290116144511841012233395353799752061257869549127164945780e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4527554216741432380141011585082087141319941270266854468232919279949735902352803053e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -8.2837166837027744593760237442711271028335526295415770894642075176684832265233618094e-47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.1059350216220761035803511769514044782618497241929104348673618588819476720900045601e-47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.7687314351211090978609979306272584841117647148823431923713385159635722296908466519e-47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.5969462796000208210556070606779102539560199436191964981433237450523523642860510077e-48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.1139078862715122066066654151520960142401928001906787354175105139384536625985592872e-48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.7479750622279993094858160487261517850737208803407757126835554161719741165140561935e-49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.3023257271883644155548861805986381464362072120660374852325331311980696768011868477e-49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.3944877765920457854470382189260552086188360983214813092431218719428924850775816044e-50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.4340311277195491754628728769381617236158442845382974748650525927232846582643581377e-51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.6046447704173152191387256911138439680611975514728905319266554977285611063458373730e-52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.4634008445755689570224291035627638546740260971523702032261365019321949141711275488e-53)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.2246731300220072906782081133065950352668949898418513030190006777980796985877588993e-55))
      };
      T result = 0;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (k * dz + k * k);
      }
      return result;
   }

   template<class T>
   static T lanczos_sum_near_2(const T& dz)
   {
      static const T d[56] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1359871474796665853092357455924330354587340093067807143261699873815704783987359772e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.8875414095359657817766255009397774415784763914903057809977502598124862632510767554e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.8476787764422274017528261804071971508619123082396685980448133660376964287516316704e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -7.4444186171772165373465718949072340109367841198406244729719893501352272283072463708e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1194120093410237139270201027497804988299179344758829377397770385486861046469631203e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.5357088952571626888790834841289410266972526362527135768453266364708511931775492144e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.5786127498932741985489606333645851990301122033327283854824446437684628190530769575e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.0137191034145345260388746496030657014468720426439135322862142167063250610782207818e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1043282398857092560432546685968118908647431787644471096079379813564778477126420342e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -9.9163139177426012658354764048015319057833754923450432555380434933570665536720208880e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 7.4010907978229491816684804764341724977655263945662331641124141340242633003517508734e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.6169234084041844689042685434707229435012900685910183054712320472809843835362299874e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.4153517213588660353769404140539796364318846647922938442928982516763682295297524025e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.0612877847699383856310204418767999963941175773527544927951233517414654065993562292e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.9161022337583429443319078749411063125143924304273472659235111671109967684138896371e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.2115062080034682817602956663792443265985654149672855657063890194977099012713284199e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.1324805949350810193947943179103430601921304099416931772402636138580376468344900971e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -6.7379854977281995958474241831424455939028898406040636148869222006236410315246590813e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1982547830365481876880107367272973441376153376561639433022240961322166151300388030e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.7477942737376629095840094280792553329439356352076603742418189295402785845496828618e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.0703715155075453775002558378654461695986020593253642834916629665157095201951753511e+00)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.9675866846243159020907742164534290169104130230063615572789631974066350817947062221e-01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.4780561158714357685758956261017689353220221023341472401836356861114284203430539079e-02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -8.6188339219787319686113112867608363101891398970645607178011869309209804235190901119e-04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.8157866597647120851117681906042345217930339391652255480834165995078338419417330285e-05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.2481320382678181695580703583602390565135442535050898393382403247198998298046344360e-06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.9159483230174733349570293758760922152980844047473628166544981799492351078480622954e-08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.6629576379996951454117430064764364032095890196838102664116218770332578145496215211e-10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.8338633562069325240602901391334379837235954960205324388722028354858275502279946625e-12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.0262311774099493927119217222064229499121659914970978765047236920420564019570051218e-14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.0407678912374899180410563256969325940215095079830235805022567411720384237429934979e-16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.7255616775185743686583950617043659309702235282144322436444914063380593981448767314e-19)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.1434762424301802086970046614089097993428600914262940192954247147222512140342020251e-22)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.2811050104947718753668138286684667456060595320836882460657390484001848305710926658e-26)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.6472071585409775242391978767682093973522869636260937895265262698161653967448170294e-31)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.7256607055318261608651300407042778032797661723154154620044497458638616772687293939e-36)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2336914691939376212057695014798607076719945066572302277992040650923577371403732374e-43)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.0786447260639215447891518190226750925343851277049431943886851989702400712288529676e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.1025728477854679597292503557220243198932574654689944030312971203381791545284070615e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.8158191084622128024881877808372136805034582816867095109911304441480739442791111504e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -4.9234747877490011456140079812669308237793558885580849786063897631770733025035207671e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 4.3490836759659078559291835803157452697088321139262167213210146255834532556301891370e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.3112546247213856964589957649093570444521154190936715486464158458810486032666803106e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 2.1776700296770363161098741319363506481285657595677761712371712460328658791045355723e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.2417232307174566390298057341943116719841715593695502126267674406446096534743228763e-45)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 6.1547673524314003744185487198857922344498908431160001720400958944740800214791613435e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -2.6513158232596442320093540701345954468693696224501434722041096230050093398371645003e-46)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 9.8887754856348531434071033903412118519453856283236928328635658056271487149248163047e-47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.1687328649763305634603605974128554276109745864508580830005398092245895954643537734e-47)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 8.6161736776863672083691176367303804395378775987877354282547095064819083181262752062e-48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -1.9521769890951289875720183789100014523337064918766622763963203875214061068994800866e-48)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 3.5893201221052508883479696840784788679013550155716308694609078334579681197829045250e-49)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -5.1475882011819285876938214420399142925361432312238692258191602240316479716676925358e-50)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 5.4033368363711826759446617519812384260206693934532562051784267615057052651438166096e-51)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, -3.6926203201715401183464726950807528731521709827951454941037337126228208878967951308e-52)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 267, 1.2328726440751392123787631395330686880390176572387043105330275032212649717981066795e-53)),
      };
      T result = 0;
      T z = dz + 2;
      for (unsigned k = 1; k <= sizeof(d) / sizeof(d[0]); ++k)
      {
         result += (-d[k - 1] * dz) / (z + k * z + k * k - 1);
      }
      return result;
   }

   static double g() { return 4.9921416015624998863131622783839702606201171875000000000000000000000000000000000000e+01; }
};

inline double lanczos_g_near_1_and_2(const lanczos52MP&)
{
   return 38.73733398437500;
}


//
// placeholder for no lanczos info available:
//
struct undefined_lanczos : public std::integral_constant<int, (std::numeric_limits<int>::max)() - 1> { };

template <class Real, class Policy>
struct lanczos
{
   static constexpr auto target_precision = policies::precision<Real, Policy>::type::value <= 0 ? (std::numeric_limits<int>::max)()-2 : 
                                                                                                   policies::precision<Real, Policy>::type::value;

   using type = typename std::conditional<(target_precision <= lanczos6m24::value), lanczos6m24, 
                typename std::conditional<(target_precision <= lanczos13m53::value), lanczos13m53,
                typename std::conditional<(target_precision <= lanczos11::value), lanczos11,
                typename std::conditional<(target_precision <= lanczos17m64::value), lanczos17m64,
                typename std::conditional<(target_precision <= lanczos24m113::value), lanczos24m113,
                typename std::conditional<(target_precision <= lanczos27MP::value), lanczos27MP,
                typename std::conditional<(target_precision <= lanczos35MP::value), lanczos35MP,
                typename std::conditional<(target_precision <= lanczos48MP::value), lanczos48MP,
                typename std::conditional<(target_precision <= lanczos49MP::value), lanczos49MP,
                typename std::conditional<(target_precision <= lanczos52MP::value), lanczos52MP, undefined_lanczos>::type
                >::type>::type>::type>::type>::type>::type>::type>::type
                >::type;
};

} // namespace lanczos
} // namespace math
} // namespace boost

#if !defined(_CRAYC) && !defined(__CUDACC__) && (!defined(__GNUC__) || (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ > 3)))
#if ((defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64)) && !defined(_MANAGED)
#include <boost/math/special_functions/detail/lanczos_sse2.hpp>
#endif
#endif

#endif // BOOST_MATH_SPECIAL_FUNCTIONS_LANCZOS

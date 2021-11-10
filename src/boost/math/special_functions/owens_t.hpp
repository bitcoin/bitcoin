// Copyright Benjamin Sobotta 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OWENS_T_HPP
#define BOOST_OWENS_T_HPP

// Reference:
// Mike Patefield, David Tandy
// FAST AND ACCURATE CALCULATION OF OWEN'S T-FUNCTION
// Journal of Statistical Software, 5 (5), 1-25

#ifdef _MSC_VER
#  pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/math/tools/throw_exception.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/big_constant.hpp>

#include <stdexcept>
#include <cmath>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

namespace boost
{
   namespace math
   {
      namespace detail
      {
         // owens_t_znorm1(x) = P(-oo<Z<=x)-0.5 with Z being normally distributed.
         template<typename RealType, class Policy>
         inline RealType owens_t_znorm1(const RealType x, const Policy& pol)
         {
            using namespace boost::math::constants;
            return boost::math::erf(x*one_div_root_two<RealType>(), pol)*half<RealType>();
         } // RealType owens_t_znorm1(const RealType x)

         // owens_t_znorm2(x) = P(x<=Z<oo) with Z being normally distributed.
         template<typename RealType, class Policy>
         inline RealType owens_t_znorm2(const RealType x, const Policy& pol)
         {
            using namespace boost::math::constants;
            return boost::math::erfc(x*one_div_root_two<RealType>(), pol)*half<RealType>();
         } // RealType owens_t_znorm2(const RealType x)

         // Auxiliary function, it computes an array key that is used to determine
         // the specific computation method for Owen's T and the order thereof
         // used in owens_t_dispatch.
         template<typename RealType>
         inline unsigned short owens_t_compute_code(const RealType h, const RealType a)
         {
            static const RealType hrange[] =
            { 0.02f, 0.06f, 0.09f, 0.125f, 0.26f, 0.4f,  0.6f,  1.6f,  1.7f,  2.33f,  2.4f,  3.36f, 3.4f,  4.8f };

            static const RealType arange[] = { 0.025f, 0.09f, 0.15f, 0.36f, 0.5f, 0.9f, 0.99999f };
            /*
            original select array from paper:
            1, 1, 2,13,13,13,13,13,13,13,13,16,16,16, 9
            1, 2, 2, 3, 3, 5, 5,14,14,15,15,16,16,16, 9
            2, 2, 3, 3, 3, 5, 5,15,15,15,15,16,16,16,10
            2, 2, 3, 5, 5, 5, 5, 7, 7,16,16,16,16,16,10
            2, 3, 3, 5, 5, 6, 6, 8, 8,17,17,17,12,12,11
            2, 3, 5, 5, 5, 6, 6, 8, 8,17,17,17,12,12,12
            2, 3, 4, 4, 6, 6, 8, 8,17,17,17,17,17,12,12
            2, 3, 4, 4, 6, 6,18,18,18,18,17,17,17,12,12
            */                  
            // subtract one because the array is written in FORTRAN in mind - in C arrays start @ zero
            static const unsigned short select[] =
            {
               0,    0 ,   1  , 12   ,12 ,  12  , 12  , 12 ,  12  , 12  , 12  , 15  , 15 ,  15  ,  8,
               0  ,  1  ,  1   , 2 ,   2   , 4  ,  4  , 13 ,  13  , 14  , 14 ,  15  , 15  , 15  ,  8,
               1  ,  1   , 2 ,   2  ,  2  ,  4   , 4  , 14  , 14 ,  14  , 14 ,  15  , 15 ,  15  ,  9,
               1  ,  1   , 2 ,   4  ,  4  ,  4   , 4  ,  6  ,  6 ,  15  , 15 ,  15 ,  15 ,  15  ,  9,
               1  ,  2   , 2  ,  4  ,  4  ,  5   , 5  ,  7  ,  7  , 16   ,16 ,  16 ,  11 ,  11 ,  10,
               1  ,  2   , 4  ,  4   , 4  ,  5   , 5  ,  7  ,  7  , 16  , 16 ,  16 ,  11  , 11 ,  11,
               1  ,  2   , 3  ,  3  ,  5  ,  5   , 7  ,  7  , 16 ,  16  , 16 ,  16 ,  16  , 11 ,  11,
               1  ,  2   , 3   , 3   , 5  ,  5 ,  17  , 17  , 17 ,  17  , 16 ,  16 ,  16 ,  11 ,  11
            };

            unsigned short ihint = 14, iaint = 7;
            for(unsigned short i = 0; i != 14; i++)
            {
               if( h <= hrange[i] )
               {
                  ihint = i;
                  break;
               }
            } // for(unsigned short i = 0; i != 14; i++)

            for(unsigned short i = 0; i != 7; i++)
            {
               if( a <= arange[i] )
               {
                  iaint = i;
                  break;
               }
            } // for(unsigned short i = 0; i != 7; i++)

            // interpret select array as 8x15 matrix
            return select[iaint*15 + ihint];

         } // unsigned short owens_t_compute_code(const RealType h, const RealType a)

         template<typename RealType>
         inline unsigned short owens_t_get_order_imp(const unsigned short icode, RealType, const std::integral_constant<int, 53>&)
         {
            static const unsigned short ord[] = {2, 3, 4, 5, 7, 10, 12, 18, 10, 20, 30, 0, 4, 7, 8, 20, 0, 0}; // 18 entries

            BOOST_MATH_ASSERT(icode<18);

            return ord[icode];
         } // unsigned short owens_t_get_order(const unsigned short icode, RealType, std::integral_constant<int, 53> const&)

         template<typename RealType>
         inline unsigned short owens_t_get_order_imp(const unsigned short icode, RealType, const std::integral_constant<int, 64>&)
        {
           // method ================>>>       {1, 1, 1, 1, 1,  1,  1,  1,  2,  2,  2,  3, 4,  4,  4,  4,  5, 6}
           static const unsigned short ord[] = {3, 4, 5, 6, 8, 11, 13, 19, 10, 20, 30,  0, 7, 10, 11, 23,  0, 0}; // 18 entries

          BOOST_MATH_ASSERT(icode<18);

          return ord[icode];
        } // unsigned short owens_t_get_order(const unsigned short icode, RealType, std::integral_constant<int, 64> const&)

         template<typename RealType, typename Policy>
         inline unsigned short owens_t_get_order(const unsigned short icode, RealType r, const Policy&)
         {
            typedef typename policies::precision<RealType, Policy>::type precision_type;
            typedef std::integral_constant<int,
               precision_type::value <= 0 ? 64 :
               precision_type::value <= 53 ? 53 : 64
            > tag_type;

            return owens_t_get_order_imp(icode, r, tag_type());
         }

         // compute the value of Owen's T function with method T1 from the reference paper
         template<typename RealType, typename Policy>
         inline RealType owens_t_T1(const RealType h, const RealType a, const unsigned short m, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const RealType hs = -h*h*half<RealType>();
            const RealType dhs = exp( hs );
            const RealType as = a*a;

            unsigned short j=1;
            RealType jj = 1;
            RealType aj = a * one_div_two_pi<RealType>();
            RealType dj = boost::math::expm1( hs, pol);
            RealType gj = hs*dhs;

            RealType val = atan( a ) * one_div_two_pi<RealType>();

            while( true )
            {
               val += dj*aj/jj;

               if( m <= j )
                  break;

               j++;
               jj += static_cast<RealType>(2);
               aj *= as;
               dj = gj - dj;
               gj *= hs / static_cast<RealType>(j);
            } // while( true )

            return val;
         } // RealType owens_t_T1(const RealType h, const RealType a, const unsigned short m)

         // compute the value of Owen's T function with method T2 from the reference paper
         template<typename RealType, class Policy>
         inline RealType owens_t_T2(const RealType h, const RealType a, const unsigned short m, const RealType ah, const Policy& pol, const std::false_type&)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const unsigned short maxii = m+m+1;
            const RealType hs = h*h;
            const RealType as = -a*a;
            const RealType y = static_cast<RealType>(1) / hs;

            unsigned short ii = 1;
            RealType val = 0;
            RealType vi = a * exp( -ah*ah*half<RealType>() ) * one_div_root_two_pi<RealType>();
            RealType z = owens_t_znorm1(ah, pol)/h;

            while( true )
            {
               val += z;
               if( maxii <= ii )
               {
                  val *= exp( -hs*half<RealType>() ) * one_div_root_two_pi<RealType>();
                  break;
               } // if( maxii <= ii )
               z = y * ( vi - static_cast<RealType>(ii) * z );
               vi *= as;
               ii += 2;
            } // while( true )

            return val;
         } // RealType owens_t_T2(const RealType h, const RealType a, const unsigned short m, const RealType ah)

         // compute the value of Owen's T function with method T3 from the reference paper
         template<typename RealType, class Policy>
         inline RealType owens_t_T3_imp(const RealType h, const RealType a, const RealType ah, const std::integral_constant<int, 53>&, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

      const unsigned short m = 20;

            static const RealType c2[] =
            {
               static_cast<RealType>(0.99999999999999987510),
               static_cast<RealType>(-0.99999999999988796462),      static_cast<RealType>(0.99999999998290743652),
               static_cast<RealType>(-0.99999999896282500134),      static_cast<RealType>(0.99999996660459362918),
               static_cast<RealType>(-0.99999933986272476760),      static_cast<RealType>(0.99999125611136965852),
               static_cast<RealType>(-0.99991777624463387686),      static_cast<RealType>(0.99942835555870132569),
               static_cast<RealType>(-0.99697311720723000295),      static_cast<RealType>(0.98751448037275303682),
               static_cast<RealType>(-0.95915857980572882813),      static_cast<RealType>(0.89246305511006708555),
               static_cast<RealType>(-0.76893425990463999675),      static_cast<RealType>(0.58893528468484693250),
               static_cast<RealType>(-0.38380345160440256652),      static_cast<RealType>(0.20317601701045299653),
               static_cast<RealType>(-0.82813631607004984866E-01),  static_cast<RealType>(0.24167984735759576523E-01),
               static_cast<RealType>(-0.44676566663971825242E-02),  static_cast<RealType>(0.39141169402373836468E-03)
            };

            const RealType as = a*a;
            const RealType hs = h*h;
            const RealType y = static_cast<RealType>(1)/hs;

            RealType ii = 1;
            unsigned short i = 0;
            RealType vi = a * exp( -ah*ah*half<RealType>() ) * one_div_root_two_pi<RealType>();
            RealType zi = owens_t_znorm1(ah, pol)/h;
            RealType val = 0;

            while( true )
            {
               BOOST_MATH_ASSERT(i < 21);
               val += zi*c2[i];
               if( m <= i ) // if( m < i+1 )
               {
                  val *= exp( -hs*half<RealType>() ) * one_div_root_two_pi<RealType>();
                  break;
               } // if( m < i )
               zi = y * (ii*zi - vi);
               vi *= as;
               ii += 2;
               i++;
            } // while( true )

            return val;
         } // RealType owens_t_T3(const RealType h, const RealType a, const RealType ah)

        // compute the value of Owen's T function with method T3 from the reference paper
        template<class RealType, class Policy>
        inline RealType owens_t_T3_imp(const RealType h, const RealType a, const RealType ah, const std::integral_constant<int, 64>&, const Policy& pol)
        {
          BOOST_MATH_STD_USING
          using namespace boost::math::constants;
          
          const unsigned short m = 30;

          static const RealType c2[] =
          {
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.99999999999999999999999729978162447266851932041876728736094298092917625009873),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.99999999999999999999467056379678391810626533251885323416799874878563998732905968),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.99999999999999999824849349313270659391127814689133077036298754586814091034842536),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.9999999999999997703859616213643405880166422891953033591551179153879839440241685),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.99999999999998394883415238173334565554173013941245103172035286759201504179038147),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.9999999999993063616095509371081203145247992197457263066869044528823599399470977),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.9999999999797336340409464429599229870590160411238245275855903767652432017766116267),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.999999999574958412069046680119051639753412378037565521359444170241346845522403274),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.9999999933226234193375324943920160947158239076786103108097456617750134812033362048),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.9999999188923242461073033481053037468263536806742737922476636768006622772762168467),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.9999992195143483674402853783549420883055129680082932629160081128947764415749728967),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.999993935137206712830997921913316971472227199741857386575097250553105958772041501),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.99996135597690552745362392866517133091672395614263398912807169603795088421057688716),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.99979556366513946026406788969630293820987757758641211293079784585126692672425362469),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.999092789629617100153486251423850590051366661947344315423226082520411961968929483),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.996593837411918202119308620432614600338157335862888580671450938858935084316004769854),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.98910017138386127038463510314625339359073956513420458166238478926511821146316469589567),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.970078558040693314521331982203762771512160168582494513347846407314584943870399016019),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.92911438683263187495758525500033707204091967947532160289872782771388170647150321633673),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.8542058695956156057286980736842905011429254735181323743367879525470479126968822863),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.73796526033030091233118357742803709382964420335559408722681794195743240930748630755),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.58523469882837394570128599003785154144164680587615878645171632791404210655891158),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.415997776145676306165661663581868460503874205343014196580122174949645271353372263),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.2588210875241943574388730510317252236407805082485246378222935376279663808416534365),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.1375535825163892648504646951500265585055789019410617565727090346559210218472356689),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.0607952766325955730493900985022020434830339794955745989150270485056436844239206648),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.0216337683299871528059836483840390514275488679530797294557060229266785853764115),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -0.00593405693455186729876995814181203900550014220428843483927218267309209471516256),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 0.0011743414818332946510474576182739210553333860106811865963485870668929503649964142),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, -1.489155613350368934073453260689881330166342484405529981510694514036264969925132e-4),
             BOOST_MATH_BIG_CONSTANT(RealType, 260, 9.072354320794357587710929507988814669454281514268844884841547607134260303118208e-6)
          };

          const RealType as = a*a;
          const RealType hs = h*h;
          const RealType y = 1 / hs;

          RealType ii = 1;
          unsigned short i = 0;
          RealType vi = a * exp( -ah*ah*half<RealType>() ) * one_div_root_two_pi<RealType>();
          RealType zi = owens_t_znorm1(ah, pol)/h;
          RealType val = 0;

          while( true )
          {
              BOOST_MATH_ASSERT(i < 31);
              val += zi*c2[i];
              if( m <= i ) // if( m < i+1 )
              {
                val *= exp( -hs*half<RealType>() ) * one_div_root_two_pi<RealType>();
                break;
              } // if( m < i )
              zi = y * (ii*zi - vi);
              vi *= as;
              ii += 2;
              i++;
          } // while( true )

          return val;
        } // RealType owens_t_T3(const RealType h, const RealType a, const RealType ah)

        template<class RealType, class Policy>
        inline RealType owens_t_T3(const RealType h, const RealType a, const RealType ah, const Policy& pol)
        {
            typedef typename policies::precision<RealType, Policy>::type precision_type;
            typedef std::integral_constant<int,
               precision_type::value <= 0 ? 64 :
               precision_type::value <= 53 ? 53 : 64
            > tag_type;

            return owens_t_T3_imp(h, a, ah, tag_type(), pol);
        }

         // compute the value of Owen's T function with method T4 from the reference paper
         template<typename RealType>
         inline RealType owens_t_T4(const RealType h, const RealType a, const unsigned short m)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const unsigned short maxii = m+m+1;
            const RealType hs = h*h;
            const RealType as = -a*a;

            unsigned short ii = 1;
            RealType ai = a * exp( -hs*(static_cast<RealType>(1)-as)*half<RealType>() ) * one_div_two_pi<RealType>();
            RealType yi = 1;
            RealType val = 0;

            while( true )
            {
               val += ai*yi;
               if( maxii <= ii )
                  break;
               ii += 2;
               yi = (static_cast<RealType>(1)-hs*yi) / static_cast<RealType>(ii);
               ai *= as;
            } // while( true )

            return val;
         } // RealType owens_t_T4(const RealType h, const RealType a, const unsigned short m)

         // compute the value of Owen's T function with method T5 from the reference paper
         template<typename RealType>
         inline RealType owens_t_T5_imp(const RealType h, const RealType a, const std::integral_constant<int, 53>&)
         {
            BOOST_MATH_STD_USING
            /*
               NOTICE:
               - The pts[] array contains the squares (!) of the abscissas, i.e. the roots of the Legendre
                 polynomial P_n(x), instead of the plain roots as required in Gauss-Legendre
                 quadrature, because T5(h,a,m) contains only x^2 terms.
               - The wts[] array contains the weights for Gauss-Legendre quadrature scaled with a factor
                 of 1/(2*pi) according to T5(h,a,m).
             */

            const unsigned short m = 13;
            static const RealType pts[] = {
               static_cast<RealType>(0.35082039676451715489E-02),
               static_cast<RealType>(0.31279042338030753740E-01),  static_cast<RealType>(0.85266826283219451090E-01),
               static_cast<RealType>(0.16245071730812277011),      static_cast<RealType>(0.25851196049125434828),
               static_cast<RealType>(0.36807553840697533536),      static_cast<RealType>(0.48501092905604697475),
               static_cast<RealType>(0.60277514152618576821),      static_cast<RealType>(0.71477884217753226516),
               static_cast<RealType>(0.81475510988760098605),      static_cast<RealType>(0.89711029755948965867),
               static_cast<RealType>(0.95723808085944261843),      static_cast<RealType>(0.99178832974629703586) };
            static const RealType wts[] = { 
               static_cast<RealType>(0.18831438115323502887E-01),
               static_cast<RealType>(0.18567086243977649478E-01),  static_cast<RealType>(0.18042093461223385584E-01),
               static_cast<RealType>(0.17263829606398753364E-01),  static_cast<RealType>(0.16243219975989856730E-01),
               static_cast<RealType>(0.14994592034116704829E-01),  static_cast<RealType>(0.13535474469662088392E-01),
               static_cast<RealType>(0.11886351605820165233E-01),  static_cast<RealType>(0.10070377242777431897E-01),
               static_cast<RealType>(0.81130545742299586629E-02),  static_cast<RealType>(0.60419009528470238773E-02),
               static_cast<RealType>(0.38862217010742057883E-02),  static_cast<RealType>(0.16793031084546090448E-02) };

            const RealType as = a*a;
            const RealType hs = -h*h*boost::math::constants::half<RealType>();

            RealType val = 0;
            for(unsigned short i = 0; i < m; ++i)
            {
               BOOST_MATH_ASSERT(i < 13);
               const RealType r = static_cast<RealType>(1) + as*pts[i];
               val += wts[i] * exp( hs*r ) / r;
            } // for(unsigned short i = 0; i < m; ++i)

            return val*a;
         } // RealType owens_t_T5(const RealType h, const RealType a)

        // compute the value of Owen's T function with method T5 from the reference paper
        template<typename RealType>
        inline RealType owens_t_T5_imp(const RealType h, const RealType a, const std::integral_constant<int, 64>&)
        {
          BOOST_MATH_STD_USING
            /*
              NOTICE:
              - The pts[] array contains the squares (!) of the abscissas, i.e. the roots of the Legendre
              polynomial P_n(x), instead of the plain roots as required in Gauss-Legendre
              quadrature, because T5(h,a,m) contains only x^2 terms.
              - The wts[] array contains the weights for Gauss-Legendre quadrature scaled with a factor
              of 1/(2*pi) according to T5(h,a,m).
            */

          const unsigned short m = 19;
          static const RealType pts[] = {
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0016634282895983227941),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.014904509242697054183),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.04103478879005817919),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.079359853513391511008),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.1288612130237615133),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.18822336642448518856),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.25586876186122962384),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.32999972011807857222),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.40864620815774761438),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.48971819306044782365),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.57106118513245543894),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.6505134942981533829),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.72596367859928091618),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.79540665919549865924),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.85699701386308739244),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.90909804422384697594),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.95032536436570154409),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.97958418733152273717),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.99610366384229088321)
          };
          static const RealType wts[] = {
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.012975111395684900835),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.012888764187499150078),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.012716644398857307844),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.012459897461364705691),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.012120231988292330388),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.011699908404856841158),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.011201723906897224448),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.010628993848522759853),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0099855296835573320047),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0092756136096132857933),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0085039700881139589055),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0076757344408814561254),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0067964187616556459109),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.005871875456524750363),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0049082589542498110071),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0039119870792519721409),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0028897090921170700834),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.0018483371329504443947),
               BOOST_MATH_BIG_CONSTANT(RealType, 64, 0.00079623320100438873578)
          };

          const RealType as = a*a;
          const RealType hs = -h*h*boost::math::constants::half<RealType>();

          RealType val = 0;
          for(unsigned short i = 0; i < m; ++i)
            {
              BOOST_MATH_ASSERT(i < 19);
              const RealType r = 1 + as*pts[i];
              val += wts[i] * exp( hs*r ) / r;
            } // for(unsigned short i = 0; i < m; ++i)

          return val*a;
        } // RealType owens_t_T5(const RealType h, const RealType a)

        template<class RealType, class Policy>
        inline RealType owens_t_T5(const RealType h, const RealType a, const Policy&)
        {
            typedef typename policies::precision<RealType, Policy>::type precision_type;
            typedef std::integral_constant<int,
               precision_type::value <= 0 ? 64 :
               precision_type::value <= 53 ? 53 : 64
            > tag_type;

            return owens_t_T5_imp(h, a, tag_type());
        }


         // compute the value of Owen's T function with method T6 from the reference paper
         template<typename RealType, class Policy>
         inline RealType owens_t_T6(const RealType h, const RealType a, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const RealType normh = owens_t_znorm2(h, pol);
            const RealType y = static_cast<RealType>(1) - a;
            const RealType r = atan2(y, static_cast<RealType>(1 + a) );

            RealType val = normh * ( static_cast<RealType>(1) - normh ) * half<RealType>();

            if( r != 0 )
               val -= r * exp( -y*h*h*half<RealType>()/r ) * one_div_two_pi<RealType>();

            return val;
         } // RealType owens_t_T6(const RealType h, const RealType a, const unsigned short m)

         template <class T, class Policy>
         std::pair<T, T> owens_t_T1_accelerated(T h, T a, const Policy& pol)
         {
            //
            // This is the same series as T1, but:
            // * The Taylor series for atan has been combined with that for T1, 
            //   reducing but not eliminating cancellation error.
            // * The resulting alternating series is then accelerated using method 1
            //   from H. Cohen, F. Rodriguez Villegas, D. Zagier, 
            //   "Convergence acceleration of alternating series", Bonn, (1991).
            //
            BOOST_MATH_STD_USING
            static const char* function = "boost::math::owens_t<%1%>(%1%, %1%)";
            T half_h_h = h * h / 2;
            T a_pow = a;
            T aa = a * a;
            T exp_term = exp(-h * h / 2);
            T one_minus_dj_sum = exp_term; 
            T sum = a_pow * exp_term;
            T dj_pow = exp_term;
            T term = sum;
            T abs_err;
            int j = 1;

            //
            // Normally with this form of series acceleration we can calculate
            // up front how many terms will be required - based on the assumption
            // that each term decreases in size by a factor of 3.  However,
            // that assumption does not apply here, as the underlying T1 series can 
            // go quite strongly divergent in the early terms, before strongly
            // converging later.  Various "guesstimates" have been tried to take account
            // of this, but they don't always work.... so instead set "n" to the 
            // largest value that won't cause overflow later, and abort iteration
            // when the last accelerated term was small enough...
            //
            int n;
#ifndef BOOST_NO_EXCEPTIONS
            try
            {
#endif
               n = itrunc(T(tools::log_max_value<T>() / 6));
#ifndef BOOST_NO_EXCEPTIONS
            }
            catch(...)
            {
               n = (std::numeric_limits<int>::max)();
            }
#endif
            n = (std::min)(n, 1500);
            T d = pow(3 + sqrt(T(8)), n);
            d = (d + 1 / d) / 2;
            T b = -1;
            T c = -d;
            c = b - c;
            sum *= c;
            b = -n * n * b * 2;
            abs_err = ldexp(fabs(sum), -tools::digits<T>());

            while(j < n)
            {
               a_pow *= aa;
               dj_pow *= half_h_h / j;
               one_minus_dj_sum += dj_pow;
               term = one_minus_dj_sum * a_pow / (2 * j + 1);
               c = b - c;
               sum += c * term;
               abs_err += ldexp((std::max)(T(fabs(sum)), T(fabs(c*term))), -tools::digits<T>());
               b = (j + n) * (j - n) * b / ((j + T(0.5)) * (j + 1));
               ++j;
               //
               // Include an escape route to prevent calculating too many terms:
               //
               if((j > 10) && (fabs(sum * tools::epsilon<T>()) > fabs(c * term)))
                  break;
            }
            abs_err += fabs(c * term);
            if(sum < 0)  // sum must always be positive, if it's negative something really bad has happened:
               policies::raise_evaluation_error(function, 0, T(0), pol);
            return std::pair<T, T>((sum / d) / boost::math::constants::two_pi<T>(), abs_err / sum);
         }

         template<typename RealType, class Policy>
         inline RealType owens_t_T2(const RealType h, const RealType a, const unsigned short m, const RealType ah, const Policy& pol, const std::true_type&)
         {
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const unsigned short maxii = m+m+1;
            const RealType hs = h*h;
            const RealType as = -a*a;
            const RealType y = static_cast<RealType>(1) / hs;

            unsigned short ii = 1;
            RealType val = 0;
            RealType vi = a * exp( -ah*ah*half<RealType>() ) / root_two_pi<RealType>();
            RealType z = owens_t_znorm1(ah, pol)/h;
            RealType last_z = fabs(z);
            RealType lim = policies::get_epsilon<RealType, Policy>();

            while( true )
            {
               val += z;
               //
               // This series stops converging after a while, so put a limit
               // on how far we go before returning our best guess:
               //
               if((fabs(lim * val) > fabs(z)) || ((ii > maxii) && (fabs(z) > last_z)) || (z == 0))
               {
                  val *= exp( -hs*half<RealType>() ) / root_two_pi<RealType>();
                  break;
               } // if( maxii <= ii )
               last_z = fabs(z);
               z = y * ( vi - static_cast<RealType>(ii) * z );
               vi *= as;
               ii += 2;
            } // while( true )

            return val;
         } // RealType owens_t_T2(const RealType h, const RealType a, const unsigned short m, const RealType ah)

         template<typename RealType, class Policy>
         inline std::pair<RealType, RealType> owens_t_T2_accelerated(const RealType h, const RealType a, const RealType ah, const Policy& pol)
         {
            //
            // This is the same series as T2, but with acceleration applied.
            // Note that we have to be *very* careful to check that nothing bad
            // has happened during evaluation - this series will go divergent
            // and/or fail to alternate at a drop of a hat! :-(
            //
            BOOST_MATH_STD_USING
            using namespace boost::math::constants;

            const RealType hs = h*h;
            const RealType as = -a*a;
            const RealType y = static_cast<RealType>(1) / hs;

            unsigned short ii = 1;
            RealType val = 0;
            RealType vi = a * exp( -ah*ah*half<RealType>() ) / root_two_pi<RealType>();
            RealType z = boost::math::detail::owens_t_znorm1(ah, pol)/h;
            RealType last_z = fabs(z);

            //
            // Normally with this form of series acceleration we can calculate
            // up front how many terms will be required - based on the assumption
            // that each term decreases in size by a factor of 3.  However,
            // that assumption does not apply here, as the underlying T1 series can 
            // go quite strongly divergent in the early terms, before strongly
            // converging later.  Various "guesstimates" have been tried to take account
            // of this, but they don't always work.... so instead set "n" to the 
            // largest value that won't cause overflow later, and abort iteration
            // when the last accelerated term was small enough...
            //
            int n;
#ifndef BOOST_NO_EXCEPTIONS
            try
            {
#endif
               n = itrunc(RealType(tools::log_max_value<RealType>() / 6));
#ifndef BOOST_NO_EXCEPTIONS
            }
            catch(...)
            {
               n = (std::numeric_limits<int>::max)();
            }
#endif
            n = (std::min)(n, 1500);
            RealType d = pow(3 + sqrt(RealType(8)), n);
            d = (d + 1 / d) / 2;
            RealType b = -1;
            RealType c = -d;
            int s = 1;

            for(int k = 0; k < n; ++k)
            {
               //
               // Check for both convergence and whether the series has gone bad:
               //
               if(
                  (fabs(z) > last_z)     // Series has gone divergent, abort
                  || (fabs(val) * tools::epsilon<RealType>() > fabs(c * s * z))  // Convergence!
                  || (z * s < 0)         // Series has stopped alternating - all bets are off - abort.
                  )
               {
                  break;
               }
               c = b - c;
               val += c * s * z;
               b = (k + n) * (k - n) * b / ((k + RealType(0.5)) * (k + 1));
               last_z = fabs(z);
               s = -s;
               z = y * ( vi - static_cast<RealType>(ii) * z );
               vi *= as;
               ii += 2;
            } // while( true )
            RealType err = fabs(c * z) / val;
            return std::pair<RealType, RealType>(val * exp( -hs*half<RealType>() ) / (d * root_two_pi<RealType>()), err);
         } // RealType owens_t_T2_accelerated(const RealType h, const RealType a, const RealType ah, const Policy&)

         template<typename RealType, typename Policy>
         inline RealType T4_mp(const RealType h, const RealType a, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            
            const RealType hs = h*h;
            const RealType as = -a*a;

            unsigned short ii = 1;
            RealType ai = constants::one_div_two_pi<RealType>() * a * exp( -0.5*hs*(1.0-as) );
            RealType yi = 1.0;
            RealType val = 0.0;

            RealType lim = boost::math::policies::get_epsilon<RealType, Policy>();

            while( true )
            {
               RealType term = ai*yi;
               val += term;
               if((yi != 0) && (fabs(val * lim) > fabs(term)))
                  break;
               ii += 2;
               yi = (1.0-hs*yi) / static_cast<RealType>(ii);
               ai *= as;
               if(ii > (std::min)(1500, (int)policies::get_max_series_iterations<Policy>()))
                  policies::raise_evaluation_error("boost::math::owens_t<%1%>", 0, val, pol);
            } // while( true )

            return val;
         } // arg_type owens_t_T4(const arg_type h, const arg_type a, const unsigned short m)


         // This routine dispatches the call to one of six subroutines, depending on the values
         // of h and a.
         // preconditions: h >= 0, 0<=a<=1, ah=a*h
         //
         // Note there are different versions for different precisions....
         template<typename RealType, typename Policy>
         inline RealType owens_t_dispatch(const RealType h, const RealType a, const RealType ah, const Policy& pol, std::integral_constant<int, 64> const&)
         {
            // Simple main case for 64-bit precision or less, this is as per the Patefield-Tandy paper:
            BOOST_MATH_STD_USING
            //
            // Handle some special cases first, these are from
            // page 1077 of Owen's original paper:
            //
            if(h == 0)
            {
               return atan(a) * constants::one_div_two_pi<RealType>();
            }
            if(a == 0)
            {
               return 0;
            }
            if(a == 1)
            {
               return owens_t_znorm2(RealType(-h), pol) * owens_t_znorm2(h, pol) / 2;
            }
            if(a >= tools::max_value<RealType>())
            {
               return owens_t_znorm2(RealType(fabs(h)), pol);
            }
            RealType val = 0; // avoid compiler warnings, 0 will be overwritten in any case
            const unsigned short icode = owens_t_compute_code(h, a);
            const unsigned short m = owens_t_get_order(icode, val /* just a dummy for the type */, pol);
            static const unsigned short meth[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 4, 4, 4, 4, 5, 6}; // 18 entries

            // determine the appropriate method, T1 ... T6
            switch( meth[icode] )
            {
            case 1: // T1
               val = owens_t_T1(h,a,m,pol);
               break;
            case 2: // T2
               typedef typename policies::precision<RealType, Policy>::type precision_type;
               typedef std::integral_constant<bool, (precision_type::value == 0) || (precision_type::value > 64)> tag_type;
               val = owens_t_T2(h, a, m, ah, pol, tag_type());
               break;
            case 3: // T3
               val = owens_t_T3(h,a,ah, pol);
               break;
            case 4: // T4
               val = owens_t_T4(h,a,m);
               break;
            case 5: // T5
               val = owens_t_T5(h,a, pol);
               break;
            case 6: // T6
               val = owens_t_T6(h,a, pol);
               break;
            default:
               BOOST_MATH_THROW_EXCEPTION(std::logic_error("selection routine in Owen's T function failed"));
            }
            return val;
         }

         template<typename RealType, typename Policy>
         inline RealType owens_t_dispatch(const RealType h, const RealType a, const RealType ah, const Policy& pol, const std::integral_constant<int, 65>&)
         {
            // Arbitrary precision version:
            BOOST_MATH_STD_USING
            //
            // Handle some special cases first, these are from
            // page 1077 of Owen's original paper:
            //
            if(h == 0)
            {
               return atan(a) * constants::one_div_two_pi<RealType>();
            }
            if(a == 0)
            {
               return 0;
            }
            if(a == 1)
            {
               return owens_t_znorm2(RealType(-h), pol) * owens_t_znorm2(h, pol) / 2;
            }
            if(a >= tools::max_value<RealType>())
            {
               return owens_t_znorm2(RealType(fabs(h)), pol);
            }
            // Attempt arbitrary precision code, this will throw if it goes wrong:
            typedef typename boost::math::policies::normalise<Policy, boost::math::policies::evaluation_error<> >::type forwarding_policy;
            std::pair<RealType, RealType> p1(0, tools::max_value<RealType>()), p2(0, tools::max_value<RealType>());
            RealType target_precision = policies::get_epsilon<RealType, Policy>() * 1000;
            bool have_t1(false), have_t2(false);
            if(ah < 3)
            {
#ifndef BOOST_NO_EXCEPTIONS
               try
               {
#endif
                  have_t1 = true;
                  p1 = owens_t_T1_accelerated(h, a, forwarding_policy());
                  if(p1.second < target_precision)
                     return p1.first;
#ifndef BOOST_NO_EXCEPTIONS
               }
               catch(const boost::math::evaluation_error&){}  // T1 may fail and throw, that's OK
#endif
            }
            if(ah > 1)
            {
#ifndef BOOST_NO_EXCEPTIONS
               try
               {
#endif
                  have_t2 = true;
                  p2 = owens_t_T2_accelerated(h, a, ah, forwarding_policy());
                  if(p2.second < target_precision)
                     return p2.first;
#ifndef BOOST_NO_EXCEPTIONS
               }
               catch(const boost::math::evaluation_error&){}  // T2 may fail and throw, that's OK
#endif
            }
            //
            // If we haven't tried T1 yet, do it now - sometimes it succeeds and the number of iterations
            // is fairly low compared to T4.
            //
            if(!have_t1)
            {
#ifndef BOOST_NO_EXCEPTIONS
               try
               {
#endif
                  have_t1 = true;
                  p1 = owens_t_T1_accelerated(h, a, forwarding_policy());
                  if(p1.second < target_precision)
                     return p1.first;
#ifndef BOOST_NO_EXCEPTIONS
               }
               catch(const boost::math::evaluation_error&){}  // T1 may fail and throw, that's OK
#endif
            }
            //
            // If we haven't tried T2 yet, do it now - sometimes it succeeds and the number of iterations
            // is fairly low compared to T4.
            //
            if(!have_t2)
            {
#ifndef BOOST_NO_EXCEPTIONS
               try
               {
#endif
                  have_t2 = true;
                  p2 = owens_t_T2_accelerated(h, a, ah, forwarding_policy());
                  if(p2.second < target_precision)
                     return p2.first;
#ifndef BOOST_NO_EXCEPTIONS
               }
               catch(const boost::math::evaluation_error&){}  // T2 may fail and throw, that's OK
#endif
            }
            //
            // OK, nothing left to do but try the most expensive option which is T4,
            // this is often slow to converge, but when it does converge it tends to
            // be accurate:
#ifndef BOOST_NO_EXCEPTIONS
            try
            {
#endif
               return T4_mp(h, a, pol);
#ifndef BOOST_NO_EXCEPTIONS
            }
            catch(const boost::math::evaluation_error&){}  // T4 may fail and throw, that's OK
#endif
            //
            // Now look back at the results from T1 and T2 and see if either gave better
            // results than we could get from the 64-bit precision versions.
            //
            if((std::min)(p1.second, p2.second) < 1e-20)
            {
               return p1.second < p2.second ? p1.first : p2.first;
            }
            //
            // We give up - no arbitrary precision versions succeeded!
            //
            return owens_t_dispatch(h, a, ah, pol, std::integral_constant<int, 64>());
         } // RealType owens_t_dispatch(RealType h, RealType a, RealType ah)
         template<typename RealType, typename Policy>
         inline RealType owens_t_dispatch(const RealType h, const RealType a, const RealType ah, const Policy& pol, const std::integral_constant<int, 0>&)
         {
            // We don't know what the precision is until runtime:
            if(tools::digits<RealType>() <= 64)
               return owens_t_dispatch(h, a, ah, pol, std::integral_constant<int, 64>());
            return owens_t_dispatch(h, a, ah, pol, std::integral_constant<int, 65>());
         }
         template<typename RealType, typename Policy>
         inline RealType owens_t_dispatch(const RealType h, const RealType a, const RealType ah, const Policy& pol)
         {
            // Figure out the precision and forward to the correct version:
            typedef typename policies::precision<RealType, Policy>::type precision_type;
            typedef std::integral_constant<int,
               precision_type::value <= 0 ? 0 :
               precision_type::value <= 64 ? 64 : 65
            > tag_type;

            return owens_t_dispatch(h, a, ah, pol, tag_type());
         }
         // compute Owen's T function, T(h,a), for arbitrary values of h and a
         template<typename RealType, class Policy>
         inline RealType owens_t(RealType h, RealType a, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            // exploit that T(-h,a) == T(h,a)
            h = fabs(h);

            // Use equation (2) in the paper to remap the arguments
            // such that h>=0 and 0<=a<=1 for the call of the actual
            // computation routine.

            const RealType fabs_a = fabs(a);
            const RealType fabs_ah = fabs_a*h;

            RealType val = 0.0; // avoid compiler warnings, 0.0 will be overwritten in any case

            if(fabs_a <= 1)
            {
               val = owens_t_dispatch(h, fabs_a, fabs_ah, pol);
            } // if(fabs_a <= 1.0)
            else 
            {
               if( h <= 0.67 )
               {
                  const RealType normh = owens_t_znorm1(h, pol);
                  const RealType normah = owens_t_znorm1(fabs_ah, pol);
                  val = static_cast<RealType>(1)/static_cast<RealType>(4) - normh*normah -
                     owens_t_dispatch(fabs_ah, static_cast<RealType>(1 / fabs_a), h, pol);
               } // if( h <= 0.67 )
               else
               {
                  const RealType normh = detail::owens_t_znorm2(h, pol);
                  const RealType normah = detail::owens_t_znorm2(fabs_ah, pol);
                  val = constants::half<RealType>()*(normh+normah) - normh*normah -
                     owens_t_dispatch(fabs_ah, static_cast<RealType>(1 / fabs_a), h, pol);
               } // else [if( h <= 0.67 )]
            } // else [if(fabs_a <= 1)]

            // exploit that T(h,-a) == -T(h,a)
            if(a < 0)
            {
               return -val;
            } // if(a < 0)

            return val;
         } // RealType owens_t(RealType h, RealType a)

         template <class T, class Policy, class tag>
         struct owens_t_initializer
         {
            struct init
            {
               init()
               {
                  do_init(tag());
               }
               template <int N>
               static void do_init(const std::integral_constant<int, N>&){}
               static void do_init(const std::integral_constant<int, 64>&)
               {
                  boost::math::owens_t(static_cast<T>(7), static_cast<T>(0.96875), Policy());
                  boost::math::owens_t(static_cast<T>(2), static_cast<T>(0.5), Policy());
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
         const typename owens_t_initializer<T, Policy, tag>::init owens_t_initializer<T, Policy, tag>::initializer;

      } // namespace detail

      template <class T1, class T2, class Policy>
      inline typename tools::promote_args<T1, T2>::type owens_t(T1 h, T2 a, const Policy& pol)
      {
         typedef typename tools::promote_args<T1, T2>::type result_type;
         typedef typename policies::evaluation<result_type, Policy>::type value_type;
         typedef typename policies::precision<value_type, Policy>::type precision_type;
         typedef std::integral_constant<int,
            precision_type::value <= 0 ? 0 :
            precision_type::value <= 64 ? 64 : 65
         > tag_type;

         detail::owens_t_initializer<result_type, Policy, tag_type>::force_instantiate();
            
         return policies::checked_narrowing_cast<result_type, Policy>(detail::owens_t(static_cast<value_type>(h), static_cast<value_type>(a), pol), "boost::math::owens_t<%1%>(%1%,%1%)");
      }

      template <class T1, class T2>
      inline typename tools::promote_args<T1, T2>::type owens_t(T1 h, T2 a)
      {
         return owens_t(h, a, policies::policy<>());
      }


   } // namespace math
} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
// EOF

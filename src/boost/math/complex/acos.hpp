//  (C) Copyright John Maddock 2005.
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_COMPLEX_ACOS_INCLUDED
#define BOOST_MATH_COMPLEX_ACOS_INCLUDED

#ifndef BOOST_MATH_COMPLEX_DETAILS_INCLUDED
#  include <boost/math/complex/details.hpp>
#endif
#ifndef BOOST_MATH_LOG1P_INCLUDED
#  include <boost/math/special_functions/log1p.hpp>
#endif
#include <boost/math/tools/assert.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::sqrt; using ::fabs; using ::acos; using ::asin; using ::atan; using ::atan2; }
#endif

namespace boost{ namespace math{

template<class T> 
std::complex<T> acos(const std::complex<T>& z)
{
   //
   // This implementation is a transcription of the pseudo-code in:
   //
   // "Implementing the Complex Arcsine and Arccosine Functions using Exception Handling."
   // T E Hull, Thomas F Fairgrieve and Ping Tak Peter Tang.
   // ACM Transactions on Mathematical Software, Vol 23, No 3, Sept 1997.
   //

   //
   // These static constants should really be in a maths constants library,
   // note that we have tweaked a_crossover as per: https://svn.boost.org/trac/boost/ticket/7290
   //
   static const T one = static_cast<T>(1);
   //static const T two = static_cast<T>(2);
   static const T half = static_cast<T>(0.5L);
   static const T a_crossover = static_cast<T>(10);
   static const T b_crossover = static_cast<T>(0.6417L);
   static const T s_pi = boost::math::constants::pi<T>();
   static const T half_pi = s_pi / 2;
   static const T log_two = boost::math::constants::ln_two<T>();
   static const T quarter_pi = s_pi / 4;
   
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   //
   // Get real and imaginary parts, discard the signs as we can 
   // figure out the sign of the result later:
   //
   T x = std::fabs(z.real());
   T y = std::fabs(z.imag());

   T real, imag; // these hold our result

   // 
   // Handle special cases specified by the C99 standard,
   // many of these special cases aren't really needed here,
   // but doing it this way prevents overflow/underflow arithmetic
   // in the main body of the logic, which may trip up some machines:
   //
   if((boost::math::isinf)(x))
   {
      if((boost::math::isinf)(y))
      {
         real = quarter_pi;
         imag = std::numeric_limits<T>::infinity();
      }
      else if((boost::math::isnan)(y))
      {
         return std::complex<T>(y, -std::numeric_limits<T>::infinity());
      }
      else
      {
         // y is not infinity or nan:
         real = 0;
         imag = std::numeric_limits<T>::infinity();
      }
   }
   else if((boost::math::isnan)(x))
   {
      if((boost::math::isinf)(y))
         return std::complex<T>(x, ((boost::math::signbit)(z.imag())) ? std::numeric_limits<T>::infinity() :  -std::numeric_limits<T>::infinity());
      return std::complex<T>(x, x);
   }
   else if((boost::math::isinf)(y))
   {
      real = half_pi;
      imag = std::numeric_limits<T>::infinity();
   }
   else if((boost::math::isnan)(y))
   {
      return std::complex<T>((x == 0) ? half_pi : y, y);
   }
   else
   {
      //
      // What follows is the regular Hull et al code,
      // begin with the special case for real numbers:
      //
      if((y == 0) && (x <= one))
         return std::complex<T>((x == 0) ? half_pi : std::acos(z.real()), (boost::math::changesign)(z.imag()));
      //
      // Figure out if our input is within the "safe area" identified by Hull et al.
      // This would be more efficient with portable floating point exception handling;
      // fortunately the quantities M and u identified by Hull et al (figure 3), 
      // match with the max and min methods of numeric_limits<T>.
      //
      T safe_max = detail::safe_max(static_cast<T>(8));
      T safe_min = detail::safe_min(static_cast<T>(4));

      T xp1 = one + x;
      T xm1 = x - one;

      if((x < safe_max) && (x > safe_min) && (y < safe_max) && (y > safe_min))
      {
         T yy = y * y;
         T r = std::sqrt(xp1*xp1 + yy);
         T s = std::sqrt(xm1*xm1 + yy);
         T a = half * (r + s);
         T b = x / a;

         if(b <= b_crossover)
         {
            real = std::acos(b);
         }
         else
         {
            T apx = a + x;
            if(x <= one)
            {
               real = std::atan(std::sqrt(half * apx * (yy /(r + xp1) + (s-xm1)))/x);
            }
            else
            {
               real = std::atan((y * std::sqrt(half * (apx/(r + xp1) + apx/(s+xm1))))/x);
            }
         }

         if(a <= a_crossover)
         {
            T am1;
            if(x < one)
            {
               am1 = half * (yy/(r + xp1) + yy/(s - xm1));
            }
            else
            {
               am1 = half * (yy/(r + xp1) + (s + xm1));
            }
            imag = boost::math::log1p(am1 + std::sqrt(am1 * (a + one)));
         }
         else
         {
            imag = std::log(a + std::sqrt(a*a - one));
         }
      }
      else
      {
         //
         // This is the Hull et al exception handling code from Fig 6 of their paper:
         //
         if(y <= (std::numeric_limits<T>::epsilon() * std::fabs(xm1)))
         {
            if(x < one)
            {
               real = std::acos(x);
               imag = y / std::sqrt(xp1*(one-x));
            }
            else
            {
               // This deviates from Hull et al's paper as per https://svn.boost.org/trac/boost/ticket/7290
               if(((std::numeric_limits<T>::max)() / xp1) > xm1)
               {
                  // xp1 * xm1 won't overflow:
                  real = y / std::sqrt(xm1*xp1);
                  imag = boost::math::log1p(xm1 + std::sqrt(xp1*xm1));
               }
               else
               {
                  real = y / x;
                  imag = log_two + std::log(x);
               }
            }
         }
         else if(y <= safe_min)
         {
            // There is an assumption in Hull et al's analysis that
            // if we get here then x == 1.  This is true for all "good"
            // machines where :
            // 
            // E^2 > 8*sqrt(u); with:
            //
            // E =  std::numeric_limits<T>::epsilon()
            // u = (std::numeric_limits<T>::min)()
            //
            // Hull et al provide alternative code for "bad" machines
            // but we have no way to test that here, so for now just assert
            // on the assumption:
            //
            BOOST_MATH_ASSERT(x == 1);
            real = std::sqrt(y);
            imag = std::sqrt(y);
         }
         else if(std::numeric_limits<T>::epsilon() * y - one >= x)
         {
            real = half_pi;
            imag = log_two + std::log(y);
         }
         else if(x > one)
         {
            real = std::atan(y/x);
            T xoy = x/y;
            imag = log_two + std::log(y) + half * boost::math::log1p(xoy*xoy);
         }
         else
         {
            real = half_pi;
            T a = std::sqrt(one + y*y);
            imag = half * boost::math::log1p(static_cast<T>(2)*y*(y+a));
         }
      }
   }

   //
   // Finish off by working out the sign of the result:
   //
   if((boost::math::signbit)(z.real()))
      real = s_pi - real;
   if(!(boost::math::signbit)(z.imag()))
      imag = (boost::math::changesign)(imag);

   return std::complex<T>(real, imag);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

} } // namespaces

#endif // BOOST_MATH_COMPLEX_ACOS_INCLUDED

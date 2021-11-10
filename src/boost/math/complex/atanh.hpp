//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_COMPLEX_ATANH_INCLUDED
#define BOOST_MATH_COMPLEX_ATANH_INCLUDED

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
std::complex<T> atanh(const std::complex<T>& z)
{
   //
   // References:
   //
   // Eric W. Weisstein. "Inverse Hyperbolic Tangent." 
   // From MathWorld--A Wolfram Web Resource. 
   // http://mathworld.wolfram.com/InverseHyperbolicTangent.html
   //
   // Also: The Wolfram Functions Site,
   // http://functions.wolfram.com/ElementaryFunctions/ArcTanh/
   //
   // Also "Abramowitz and Stegun. Handbook of Mathematical Functions."
   // at : http://jove.prohosting.com/~skripty/toc.htm
   //
   // See also: https://svn.boost.org/trac/boost/ticket/7291
   //
   
   static const T pi = boost::math::constants::pi<T>();
   static const T half_pi = pi / 2;
   static const T one = static_cast<T>(1.0L);
   static const T two = static_cast<T>(2.0L);
   static const T four = static_cast<T>(4.0L);
   static const T zero = static_cast<T>(0);
   static const T log_two = boost::math::constants::ln_two<T>();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

   T x = std::fabs(z.real());
   T y = std::fabs(z.imag());

   T real, imag;  // our results

   T safe_upper = detail::safe_max(two);
   T safe_lower = detail::safe_min(static_cast<T>(2));

   //
   // Begin by handling the special cases specified in C99:
   //
   if((boost::math::isnan)(x))
   {
      if((boost::math::isnan)(y))
         return std::complex<T>(x, x);
      else if((boost::math::isinf)(y))
         return std::complex<T>(0, ((boost::math::signbit)(z.imag()) ? -half_pi : half_pi));
      else
         return std::complex<T>(x, x);
   }
   else if((boost::math::isnan)(y))
   {
      if(x == 0)
         return std::complex<T>(x, y);
      if((boost::math::isinf)(x))
         return std::complex<T>(0, y);
      else
         return std::complex<T>(y, y);
   }
   else if((x > safe_lower) && (x < safe_upper) && (y > safe_lower) && (y < safe_upper))
   {

      T yy = y*y;
      T mxm1 = one - x;
      ///
      // The real part is given by:
      // 
      // real(atanh(z)) == log1p(4*x / ((x-1)*(x-1) + y^2))
      // 
      real = boost::math::log1p(four * x / (mxm1*mxm1 + yy));
      real /= four;
      if((boost::math::signbit)(z.real()))
         real = (boost::math::changesign)(real);

      imag = std::atan2((y * two), (mxm1*(one+x) - yy));
      imag /= two;
      if(z.imag() < 0)
         imag = (boost::math::changesign)(imag);
   }
   else
   {
      //
      // This section handles exception cases that would normally cause
      // underflow or overflow in the main formulas.
      //
      // Begin by working out the real part, we need to approximate
      //    real = boost::math::log1p(4x / ((x-1)^2 + y^2))
      // without either overflow or underflow in the squared terms.
      //
      T mxm1 = one - x;
      if(x >= safe_upper)
      {
         // x-1 = x to machine precision:
         if((boost::math::isinf)(x) || (boost::math::isinf)(y))
         {
            real = 0;
         }
         else if(y >= safe_upper)
         {
            // Big x and y: divide through by x*y:
            real = boost::math::log1p((four/y) / (x/y + y/x));
         }
         else if(y > one)
         {
            // Big x: divide through by x:
            real = boost::math::log1p(four / (x + y*y/x));
         }
         else
         {
            // Big x small y, as above but neglect y^2/x:
            real = boost::math::log1p(four/x);
         }
      }
      else if(y >= safe_upper)
      {
         if(x > one)
         {
            // Big y, medium x, divide through by y:
            real = boost::math::log1p((four*x/y) / (y + mxm1*mxm1/y));
         }
         else
         {
            // Small or medium x, large y:
            real = four*x/y/y;
         }
      }
      else if (x != one)
      {
         // y is small, calculate divisor carefully:
         T div = mxm1*mxm1;
         if(y > safe_lower)
            div += y*y;
         real = boost::math::log1p(four*x/div);
      }
      else
         real = boost::math::changesign(two * (std::log(y) - log_two));

      real /= four;
      if((boost::math::signbit)(z.real()))
         real = (boost::math::changesign)(real);

      //
      // Now handle imaginary part, this is much easier,
      // if x or y are large, then the formula:
      //    atan2(2y, (1-x)*(1+x) - y^2)
      // evaluates to +-(PI - theta) where theta is negligible compared to PI.
      //
      if((x >= safe_upper) || (y >= safe_upper))
      {
         imag = pi;
      }
      else if(x <= safe_lower)
      {
         //
         // If both x and y are small then atan(2y),
         // otherwise just x^2 is negligible in the divisor:
         //
         if(y <= safe_lower)
            imag = std::atan2(two*y, one);
         else
         {
            if((y == zero) && (x == zero))
               imag = 0;
            else
               imag = std::atan2(two*y, one - y*y);
         }
      }
      else
      {
         //
         // y^2 is negligible:
         //
         if((y == zero) && (x == one))
            imag = 0;
         else
            imag = std::atan2(two*y, mxm1*(one+x));
      }
      imag /= two;
      if((boost::math::signbit)(z.imag()))
         imag = (boost::math::changesign)(imag);
   }
   return std::complex<T>(real, imag);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

} } // namespaces

#endif // BOOST_MATH_COMPLEX_ATANH_INCLUDED

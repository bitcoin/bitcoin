// Copyright John Maddock 2012.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_HANKEL_HPP
#define BOOST_MATH_HANKEL_HPP

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/bessel.hpp>

namespace boost{ namespace math{

namespace detail{

template <class T, class Policy>
std::complex<T> hankel_imp(T v, T x, const bessel_no_int_tag&, const Policy& pol, int sign)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::cyl_hankel_1<%1%>(%1%,%1%)";

   if(x < 0)
   {
      bool isint_v = floor(v) == v;
      T j, y;
      bessel_jy(v, -x, &j, &y, need_j | need_y, pol);
      std::complex<T> cx(x), cv(v);
      std::complex<T> j_result, y_result;
      if(isint_v)
      {
         int s = (iround(v) & 1) ? -1 : 1;
         j_result = j * s;
         y_result = T(s) * (y - (2 / constants::pi<T>()) * (log(-x) - log(cx)) * j);
      }
      else
      {
         j_result = pow(cx, v) * pow(-cx, -v) * j;
         T p1 = pow(-x, v);
         std::complex<T> p2 = pow(cx, v);
         y_result = p1 * y / p2
            + (p2 / p1 - p1 / p2) * j / tan(constants::pi<T>() * v);
      }
      // multiply y_result by i:
      y_result = std::complex<T>(-sign * y_result.imag(), sign * y_result.real());
      return j_result + y_result;
   }

   if(x == 0)
   {
      if(v == 0)
      {
         // J is 1, Y is -INF
         return std::complex<T>(1, sign * -policies::raise_overflow_error<T>(function, 0, pol));
      }
      else
      {
         // At least one of J and Y is complex infinity:
         return std::complex<T>(policies::raise_overflow_error<T>(function, 0, pol), sign * policies::raise_overflow_error<T>(function, 0, pol));
      }
   }

   T j, y;
   bessel_jy(v, x, &j, &y, need_j | need_y, pol);
   return std::complex<T>(j, sign * y);
}

template <class T, class Policy>
std::complex<T> hankel_imp(int v, T x, const bessel_int_tag&, const Policy& pol, int sign);

template <class T, class Policy>
inline std::complex<T> hankel_imp(T v, T x, const bessel_maybe_int_tag&, const Policy& pol, int sign)
{
   BOOST_MATH_STD_USING  // ADL of std names.
   int ival = detail::iconv(v, pol);
   if(0 == v - ival)
   {
      return hankel_imp(ival, x, bessel_int_tag(), pol, sign);
   }
   return hankel_imp(v, x, bessel_no_int_tag(), pol, sign);
}

template <class T, class Policy>
inline std::complex<T> hankel_imp(int v, T x, const bessel_int_tag&, const Policy& pol, int sign)
{
   BOOST_MATH_STD_USING
   if((std::abs(v) < 200) && (x > 0))
      return std::complex<T>(bessel_jn(v, x, pol), sign * bessel_yn(v, x, pol));
   return hankel_imp(static_cast<T>(v), x, bessel_no_int_tag(), pol, sign);
}

template <class T, class Policy>
inline std::complex<T> sph_hankel_imp(T v, T x, const Policy& pol, int sign)
{
   BOOST_MATH_STD_USING
   return constants::root_half_pi<T>() * hankel_imp(v + 0.5f, x, bessel_no_int_tag(), pol, sign) / sqrt(std::complex<T>(x));
}

} // namespace detail

template <class T1, class T2, class Policy>
inline std::complex<typename detail::bessel_traits<T1, T2, Policy>::result_type> cyl_hankel_1(T1 v, T2 x, const Policy& pol)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename detail::bessel_traits<T1, T2, Policy>::optimisation_tag tag_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<std::complex<result_type>, Policy>(detail::hankel_imp<value_type>(v, static_cast<value_type>(x), tag_type(), pol, 1), "boost::math::cyl_hankel_1<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline std::complex<typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type> cyl_hankel_1(T1 v, T2 x)
{
   return cyl_hankel_1(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline std::complex<typename detail::bessel_traits<T1, T2, Policy>::result_type> cyl_hankel_2(T1 v, T2 x, const Policy& pol)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename detail::bessel_traits<T1, T2, Policy>::optimisation_tag tag_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<std::complex<result_type>, Policy>(detail::hankel_imp<value_type>(v, static_cast<value_type>(x), tag_type(), pol, -1), "boost::math::cyl_hankel_1<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline std::complex<typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type> cyl_hankel_2(T1 v, T2 x)
{
   return cyl_hankel_2(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline std::complex<typename detail::bessel_traits<T1, T2, Policy>::result_type> sph_hankel_1(T1 v, T2 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<std::complex<result_type>, Policy>(detail::sph_hankel_imp<value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy(), 1), "boost::math::sph_hankel_1<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline std::complex<typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type> sph_hankel_1(T1 v, T2 x)
{
   return sph_hankel_1(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline std::complex<typename detail::bessel_traits<T1, T2, Policy>::result_type> sph_hankel_2(T1 v, T2 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<std::complex<result_type>, Policy>(detail::sph_hankel_imp<value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy(), -1), "boost::math::sph_hankel_1<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline std::complex<typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type> sph_hankel_2(T1 v, T2 x)
{
   return sph_hankel_2(v, x, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_HANKEL_HPP


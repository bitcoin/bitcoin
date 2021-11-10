// Copyright John Maddock 2012.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_JACOBI_ELLIPTIC_HPP
#define BOOST_MATH_JACOBI_ELLIPTIC_HPP

#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>

namespace boost{ namespace math{

namespace detail{

template <class T, class Policy>
T jacobi_recurse(const T& x, const T& k, T anm1, T bnm1, unsigned N, T* pTn, const Policy& pol)
{
   BOOST_MATH_STD_USING
   ++N;
   T Tn;
   T cn = (anm1 - bnm1) / 2;
   T an = (anm1 + bnm1) / 2;
   if(cn < policies::get_epsilon<T, Policy>())
   {
      Tn = ldexp(T(1), (int)N) * x * an;
   }
   else
      Tn = jacobi_recurse<T>(x, k, an, sqrt(anm1 * bnm1), N, 0, pol);
   if(pTn)
      *pTn = Tn;
   return (Tn + asin((cn / an) * sin(Tn))) / 2;
}

template <class T, class Policy>
T jacobi_imp(const T& x, const T& k, T* cn, T* dn, const Policy& pol, const char* function)
{
   BOOST_MATH_STD_USING
   if(k < 0)
   {
      *cn = policies::raise_domain_error<T>(function, "Modulus k must be positive but got %1%.", k, pol);
      *dn = *cn;
      return *cn;
   }
   if(k > 1)
   {
      T xp = x * k;
      T kp = 1 / k;
      T snp, cnp, dnp;
      snp = jacobi_imp(xp, kp, &cnp, &dnp, pol, function);
      *cn = dnp;
      *dn = cnp;
      return snp * kp;
   }
   //
   // Special cases first:
   //
   if(x == 0)
   {
      *cn = *dn = 1;
      return 0;
   }
   if(k == 0)
   {
      *cn = cos(x);
      *dn = 1;
      return sin(x);
   }
   if(k == 1)
   {
      *cn = *dn = 1 / cosh(x);
      return tanh(x);
   }
   //
   // Asymptotic forms from A&S 16.13:
   //
   if(k < tools::forth_root_epsilon<T>())
   {
      T su = sin(x);
      T cu = cos(x);
      T m = k * k;
      *dn = 1 - m * su * su / 2;
      *cn = cu + m * (x - su * cu) * su / 4;
      return su - m * (x - su * cu) * cu / 4;
   }
   /*  Can't get this to work to adequate precision - disabled for now...
   //
   // Asymptotic forms from A&S 16.15:
   //
   if(k > 1 - tools::root_epsilon<T>())
   {
      T tu = tanh(x);
      T su = sinh(x);
      T cu = cosh(x);
      T sec = 1 / cu;
      T kp = 1 - k;
      T m1 = 2 * kp - kp * kp;
      *dn = sec + m1 * (su * cu + x) * tu * sec / 4;
      *cn = sec - m1 * (su * cu - x) * tu * sec / 4;
      T sn = tu;
      T sn2 = m1 * (x * sec * sec - tu) / 4;
      T sn3 = (72 * x * cu + 4 * (8 * x * x - 5) * su - 19 * sinh(3 * x) + sinh(5 * x)) * sec * sec * sec * m1 * m1 / 512;
      return sn + sn2 - sn3;
   }*/
   T T1;
   T kc = 1 - k;
   T k_prime = k < 0.5 ? T(sqrt(1 - k * k)) : T(sqrt(2 * kc - kc * kc));
   T T0 = jacobi_recurse(x, k, T(1), k_prime, 0, &T1, pol);
   *cn = cos(T0);
   *dn = cos(T0) / cos(T1 - T0);
   return sin(T0);
}

} // namespace detail

template <class T, class U, class V, class Policy>
inline typename tools::promote_args<T, U, V>::type jacobi_elliptic(T k, U theta, V* pcn, V* pdn, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   static const char* function = "boost::math::jacobi_elliptic<%1%>(%1%)";

   value_type sn, cn, dn;
   sn = detail::jacobi_imp<value_type>(static_cast<value_type>(theta), static_cast<value_type>(k), &cn, &dn, forwarding_policy(), function);
   if(pcn)
      *pcn = policies::checked_narrowing_cast<result_type, Policy>(cn, function);
   if(pdn)
      *pdn = policies::checked_narrowing_cast<result_type, Policy>(dn, function);
   return policies::checked_narrowing_cast<result_type, Policy>(sn, function);
}

template <class T, class U, class V>
inline typename tools::promote_args<T, U, V>::type jacobi_elliptic(T k, U theta, V* pcn, V* pdn)
{
   return jacobi_elliptic(k, theta, pcn, pdn, policies::policy<>());
}

template <class U, class T, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_sn(U k, T theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   return jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), static_cast<result_type*>(0), static_cast<result_type*>(0), pol);
}

template <class U, class T>
inline typename tools::promote_args<T, U>::type jacobi_sn(U k, T theta)
{
   return jacobi_sn(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_cn(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type cn;
   jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), &cn, static_cast<result_type*>(0), pol);
   return cn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_cn(T k, U theta)
{
   return jacobi_cn(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_dn(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type dn;
   jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), static_cast<result_type*>(0), &dn, pol);
   return dn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_dn(T k, U theta)
{
   return jacobi_dn(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_cd(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type cn, dn;
   jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), &cn, &dn, pol);
   return cn / dn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_cd(T k, U theta)
{
   return jacobi_cd(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_dc(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type cn, dn;
   jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), &cn, &dn, pol);
   return dn / cn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_dc(T k, U theta)
{
   return jacobi_dc(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_ns(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   return 1 / jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), static_cast<result_type*>(0), static_cast<result_type*>(0), pol);
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_ns(T k, U theta)
{
   return jacobi_ns(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_sd(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type sn, dn;
   sn = jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), static_cast<result_type*>(0), &dn, pol);
   return sn / dn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_sd(T k, U theta)
{
   return jacobi_sd(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_ds(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type sn, dn;
   sn = jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), static_cast<result_type*>(0), &dn, pol);
   return dn / sn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_ds(T k, U theta)
{
   return jacobi_ds(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_nc(T k, U theta, const Policy& pol)
{
   return 1 / jacobi_cn(k, theta, pol);
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_nc(T k, U theta)
{
   return jacobi_nc(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_nd(T k, U theta, const Policy& pol)
{
   return 1 / jacobi_dn(k, theta, pol);
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_nd(T k, U theta)
{
   return jacobi_nd(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_sc(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type sn, cn;
   sn = jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), &cn, static_cast<result_type*>(0), pol);
   return sn / cn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_sc(T k, U theta)
{
   return jacobi_sc(k, theta, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type jacobi_cs(T k, U theta, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   result_type sn, cn;
   sn = jacobi_elliptic(static_cast<result_type>(k), static_cast<result_type>(theta), &cn, static_cast<result_type*>(0), pol);
   return cn / sn;
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type jacobi_cs(T k, U theta)
{
   return jacobi_cs(k, theta, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_JACOBI_ELLIPTIC_HPP

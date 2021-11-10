//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_LEGENDRE_HPP
#define BOOST_MATH_SPECIAL_LEGENDRE_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <utility>
#include <vector>
#include <type_traits>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/tools/cxx03_warn.hpp>

namespace boost{
namespace math{

// Recurrence relation for legendre P and Q polynomials:
template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type
   legendre_next(unsigned l, T1 x, T2 Pl, T3 Plm1)
{
   typedef typename tools::promote_args<T1, T2, T3>::type result_type;
   return ((2 * l + 1) * result_type(x) * result_type(Pl) - l * result_type(Plm1)) / (l + 1);
}

namespace detail{

// Implement Legendre P and Q polynomials via recurrence:
template <class T, class Policy>
T legendre_imp(unsigned l, T x, const Policy& pol, bool second = false)
{
   static const char* function = "boost::math::legrendre_p<%1%>(unsigned, %1%)";
   // Error handling:
   if((x < -1) || (x > 1))
      return policies::raise_domain_error<T>(
         function,
         "The Legendre Polynomial is defined for"
         " -1 <= x <= 1, but got x = %1%.", x, pol);

   T p0, p1;
   if(second)
   {
      // A solution of the second kind (Q):
      p0 = (boost::math::log1p(x, pol) - boost::math::log1p(-x, pol)) / 2;
      p1 = x * p0 - 1;
   }
   else
   {
      // A solution of the first kind (P):
      p0 = 1;
      p1 = x;
   }
   if(l == 0)
      return p0;

   unsigned n = 1;

   while(n < l)
   {
      std::swap(p0, p1);
      p1 = boost::math::legendre_next(n, x, p0, p1);
      ++n;
   }
   return p1;
}

template <class T, class Policy>
T legendre_p_prime_imp(unsigned l, T x, const Policy& pol, T* Pn 
#ifdef BOOST_NO_CXX11_NULLPTR
   = 0
#else
   = nullptr
#endif
)
{
   static const char* function = "boost::math::legrendre_p_prime<%1%>(unsigned, %1%)";
   // Error handling:
   if ((x < -1) || (x > 1))
      return policies::raise_domain_error<T>(
         function,
         "The Legendre Polynomial is defined for"
         " -1 <= x <= 1, but got x = %1%.", x, pol);
   
   if (l == 0)
    {
        if (Pn)
        {
           *Pn = 1;
        }
        return 0;
    }
    T p0 = 1;
    T p1 = x;
    T p_prime;
    bool odd = l & 1;
    // If the order is odd, we sum all the even polynomials:
    if (odd)
    {
        p_prime = p0;
    }
    else // Otherwise we sum the odd polynomials * (2n+1)
    {
        p_prime = 3*p1;
    }

    unsigned n = 1;
    while(n < l - 1)
    {
       std::swap(p0, p1);
       p1 = boost::math::legendre_next(n, x, p0, p1);
       ++n;
       if (odd)
       {
          p_prime += (2*n+1)*p1;
          odd = false;
       }
       else
       {
           odd = true;
       }
    }
    // This allows us to evaluate the derivative and the function for the same cost.
    if (Pn)
    {
        std::swap(p0, p1);
        *Pn = boost::math::legendre_next(n, x, p0, p1);
    }
    return p_prime;
}

template <class T, class Policy>
struct legendre_p_zero_func
{
   int n;
   const Policy& pol;

   legendre_p_zero_func(int n_, const Policy& p) : n(n_), pol(p) {}

   std::pair<T, T> operator()(T x) const
   { 
      T Pn;
      T Pn_prime = detail::legendre_p_prime_imp(n, x, pol, &Pn);
      return std::pair<T, T>(Pn, Pn_prime); 
   }
};

template <class T, class Policy>
std::vector<T> legendre_p_zeros_imp(int n, const Policy& pol)
{
    using std::cos;
    using std::sin;
    using std::ceil;
    using std::sqrt;
    using boost::math::constants::pi;
    using boost::math::constants::half;
    using boost::math::tools::newton_raphson_iterate;

    BOOST_MATH_ASSERT(n >= 0);
    std::vector<T> zeros;
    if (n == 0)
    {
        // There are no zeros of P_0(x) = 1.
        return zeros;
    }
    int k;
    if (n & 1)
    {
        zeros.resize((n-1)/2 + 1, std::numeric_limits<T>::quiet_NaN());
        zeros[0] = 0;
        k = 1;
    }
    else
    {
        zeros.resize(n/2, std::numeric_limits<T>::quiet_NaN());
        k = 0;
    }
    T half_n = ceil(n*half<T>());

    while (k < (int)zeros.size())
    {
        // Bracket the root: Szego:
        // Gabriel Szego, Inequalities for the Zeros of Legendre Polynomials and Related Functions, Transactions of the American Mathematical Society, Vol. 39, No. 1 (1936)
        T theta_nk =  ((half_n - half<T>()*half<T>() - static_cast<T>(k))*pi<T>())/(static_cast<T>(n)+half<T>());
        T lower_bound = cos( (half_n - static_cast<T>(k))*pi<T>()/static_cast<T>(n + 1));
        T cos_nk = cos(theta_nk);
        T upper_bound = cos_nk;
        // First guess follows from:
        //  F. G. Tricomi, Sugli zeri dei polinomi sferici ed ultrasferici, Ann. Mat. Pura Appl., 31 (1950), pp. 93-97;
        T inv_n_sq = 1/static_cast<T>(n*n);
        T sin_nk = sin(theta_nk);
        T x_nk_guess = (1 - inv_n_sq/static_cast<T>(8) + inv_n_sq /static_cast<T>(8*n) - (inv_n_sq*inv_n_sq/384)*(39  - 28 / (sin_nk*sin_nk) ) )*cos_nk;

        std::uintmax_t number_of_iterations = policies::get_max_root_iterations<Policy>();

        legendre_p_zero_func<T, Policy> f(n, pol);

        const T x_nk = newton_raphson_iterate(f, x_nk_guess,
                                              lower_bound, upper_bound,
                                              policies::digits<T, Policy>(),
                                              number_of_iterations);

        BOOST_MATH_ASSERT(lower_bound < x_nk);
        BOOST_MATH_ASSERT(upper_bound > x_nk);
        zeros[k] = x_nk;
        ++k;
    }
    return zeros;
}

} // namespace detail

template <class T, class Policy>
inline typename std::enable_if<policies::is_policy<Policy>::value, typename tools::promote_args<T>::type>::type
   legendre_p(int l, T x, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   static const char* function = "boost::math::legendre_p<%1%>(unsigned, %1%)";
   if(l < 0)
      return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_imp(-l-1, static_cast<value_type>(x), pol, false), function);
   return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_imp(l, static_cast<value_type>(x), pol, false), function);
}


template <class T, class Policy>
inline typename std::enable_if<policies::is_policy<Policy>::value, typename tools::promote_args<T>::type>::type
   legendre_p_prime(int l, T x, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   static const char* function = "boost::math::legendre_p_prime<%1%>(unsigned, %1%)";
   if(l < 0)
      return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_p_prime_imp(-l-1, static_cast<value_type>(x), pol), function);
   return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_p_prime_imp(l, static_cast<value_type>(x), pol), function);
}

template <class T>
inline typename tools::promote_args<T>::type
   legendre_p(int l, T x)
{
   return boost::math::legendre_p(l, x, policies::policy<>());
}

template <class T>
inline typename tools::promote_args<T>::type
   legendre_p_prime(int l, T x)
{
   return boost::math::legendre_p_prime(l, x, policies::policy<>());
}

template <class T, class Policy>
inline std::vector<T> legendre_p_zeros(int l, const Policy& pol)
{
    if(l < 0)
        return detail::legendre_p_zeros_imp<T>(-l-1, pol);

    return detail::legendre_p_zeros_imp<T>(l, pol);
}


template <class T>
inline std::vector<T> legendre_p_zeros(int l)
{
   return boost::math::legendre_p_zeros<T>(l, policies::policy<>());
}

template <class T, class Policy>
inline typename std::enable_if<policies::is_policy<Policy>::value, typename tools::promote_args<T>::type>::type
   legendre_q(unsigned l, T x, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_imp(l, static_cast<value_type>(x), pol, true), "boost::math::legendre_q<%1%>(unsigned, %1%)");
}

template <class T>
inline typename tools::promote_args<T>::type
   legendre_q(unsigned l, T x)
{
   return boost::math::legendre_q(l, x, policies::policy<>());
}

// Recurrence for associated polynomials:
template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type
   legendre_next(unsigned l, unsigned m, T1 x, T2 Pl, T3 Plm1)
{
   typedef typename tools::promote_args<T1, T2, T3>::type result_type;
   return ((2 * l + 1) * result_type(x) * result_type(Pl) - (l + m) * result_type(Plm1)) / (l + 1 - m);
}

namespace detail{
// Legendre P associated polynomial:
template <class T, class Policy>
T legendre_p_imp(int l, int m, T x, T sin_theta_power, const Policy& pol)
{
   BOOST_MATH_STD_USING
   // Error handling:
   if((x < -1) || (x > 1))
      return policies::raise_domain_error<T>(
      "boost::math::legendre_p<%1%>(int, int, %1%)",
         "The associated Legendre Polynomial is defined for"
         " -1 <= x <= 1, but got x = %1%.", x, pol);
   // Handle negative arguments first:
   if(l < 0)
      return legendre_p_imp(-l-1, m, x, sin_theta_power, pol);
   if ((l == 0) && (m == -1))
   {
      return sqrt((1 - x) / (1 + x));
   }
   if ((l == 1) && (m == 0))
   {
      return x;
   }
   if (-m == l)
   {
      return pow((1 - x * x) / 4, T(l) / 2) / boost::math::tgamma(l + 1, pol);
   }
   if(m < 0)
   {
      int sign = (m&1) ? -1 : 1;
      return sign * boost::math::tgamma_ratio(static_cast<T>(l+m+1), static_cast<T>(l+1-m), pol) * legendre_p_imp(l, -m, x, sin_theta_power, pol);
   }
   // Special cases:
   if(m > l)
      return 0;
   if(m == 0)
      return boost::math::legendre_p(l, x, pol);

   T p0 = boost::math::double_factorial<T>(2 * m - 1, pol) * sin_theta_power;

   if(m&1)
      p0 *= -1;
   if(m == l)
      return p0;

   T p1 = x * (2 * m + 1) * p0;

   int n = m + 1;

   while(n < l)
   {
      std::swap(p0, p1);
      p1 = boost::math::legendre_next(n, m, x, p0, p1);
      ++n;
   }
   return p1;
}

template <class T, class Policy>
inline T legendre_p_imp(int l, int m, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   // TODO: we really could use that mythical "pow1p" function here:
   return legendre_p_imp(l, m, x, static_cast<T>(pow(1 - x*x, T(abs(m))/2)), pol);
}

}

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   legendre_p(int l, int m, T x, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::legendre_p_imp(l, m, static_cast<value_type>(x), pol), "boost::math::legendre_p<%1%>(int, int, %1%)");
}

template <class T>
inline typename tools::promote_args<T>::type
   legendre_p(int l, int m, T x)
{
   return boost::math::legendre_p(l, m, x, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SPECIAL_LEGENDRE_HPP


///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 Nikhar Agrawal
//  Copyright 2013 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2013 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_POLYGAMMA_DETAIL_2013_07_30_HPP_
  #define _BOOST_POLYGAMMA_DETAIL_2013_07_30_HPP_

#include <cmath>
#include <limits>
#include <string>
#include <boost/math/policies/policy.hpp>
#include <boost/math/special_functions/bernoulli.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/special_functions/zeta.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/special_functions/cos_pi.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/config.hpp>

#ifdef BOOST_HAS_THREADS
#include <mutex>
#endif

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4702) // Unreachable code (release mode only warning)
#endif

namespace boost { namespace math { namespace detail{

  template<class T, class Policy>
  T polygamma_atinfinityplus(const int n, const T& x, const Policy& pol, const char* function) // for large values of x such as for x> 400
  {
     // See http://functions.wolfram.com/GammaBetaErf/PolyGamma2/06/02/0001/
     BOOST_MATH_STD_USING
     //
     // sum       == current value of accumulated sum.
     // term      == value of current term to be added to sum.
     // part_term == value of current term excluding the Bernoulli number part
     //
     if(n + x == x)
     {
        // x is crazy large, just concentrate on the first part of the expression and use logs:
        if(n == 1) return 1 / x;
        T nlx = n * log(x);
        if((nlx < tools::log_max_value<T>()) && (n < (int)max_factorial<T>::value))
           return ((n & 1) ? 1 : -1) * boost::math::factorial<T>(n - 1, pol) * pow(x, -n);
        else
         return ((n & 1) ? 1 : -1) * exp(boost::math::lgamma(T(n), pol) - n * log(x));
     }
     T term, sum, part_term;
     T x_squared = x * x;
     //
     // Start by setting part_term to:
     //
     // (n-1)! / x^(n+1)
     //
     // which is common to both the first term of the series (with k = 1)
     // and to the leading part.  
     // We can then get to the leading term by:
     //
     // part_term * (n + 2 * x) / 2
     //
     // and to the first term in the series 
     // (excluding the Bernoulli number) by:
     //
     // part_term n * (n + 1) / (2x)
     //
     // If either the factorial would overflow,
     // or the power term underflows, this just gets set to 0 and then we
     // know that we have to use logs for the initial terms:
     //
     part_term = ((n > (int)boost::math::max_factorial<T>::value) && (T(n) * n > tools::log_max_value<T>())) 
        ? T(0) : static_cast<T>(boost::math::factorial<T>(n - 1, pol) * pow(x, -n - 1));
     if(part_term == 0)
     {
        // Either n is very large, or the power term underflows,
        // set the initial values of part_term, term and sum via logs:
        part_term = static_cast<T>(boost::math::lgamma(n, pol) - (n + 1) * log(x));
        sum = exp(part_term + log(n + 2 * x) - boost::math::constants::ln_two<T>());
        part_term += log(T(n) * (n + 1)) - boost::math::constants::ln_two<T>() - log(x);
        part_term = exp(part_term);
     }
     else
     {
        sum = part_term * (n + 2 * x) / 2;
        part_term *= (T(n) * (n + 1)) / 2;
        part_term /= x;
     }
     //
     // If the leading term is 0, so is the result:
     //
     if(sum == 0)
        return sum;

     for(unsigned k = 1;;)
     {
        term = part_term * boost::math::bernoulli_b2n<T>(k, pol);
        sum += term;
        //
        // Normal termination condition:
        //
        if(fabs(term / sum) < tools::epsilon<T>())
           break;
        //
        // Increment our counter, and move part_term on to the next value:
        //
        ++k;
        part_term *= T(n + 2 * k - 2) * (n - 1 + 2 * k);
        part_term /= (2 * k - 1) * 2 * k;
        part_term /= x_squared;
        //
        // Emergency get out termination condition:
        //
        if(k > policies::get_max_series_iterations<Policy>())
        {
           return policies::raise_evaluation_error(function, "Series did not converge, closest value was %1%", sum, pol);
        }
     }
     
     if((n - 1) & 1)
        sum = -sum;

     return sum;
  }

  template<class T, class Policy>
  T polygamma_attransitionplus(const int n, const T& x, const Policy& pol, const char* function)
  {
    // See: http://functions.wolfram.com/GammaBetaErf/PolyGamma2/16/01/01/0017/

    // Use N = (0.4 * digits) + (4 * n) for target value for x:
    BOOST_MATH_STD_USING
    const int d4d  = static_cast<int>(0.4F * policies::digits_base10<T, Policy>());
    const int N = d4d + (4 * n);
    const int m    = n;
    const int iter = N - itrunc(x);

    if(iter > (int)policies::get_max_series_iterations<Policy>())
       return policies::raise_evaluation_error<T>(function, ("Exceeded maximum series evaluations evaluating at n = " + std::to_string(n) + " and x = %1%").c_str(), x, pol);

    const int minus_m_minus_one = -m - 1;

    T z(x);
    T sum0(0);
    T z_plus_k_pow_minus_m_minus_one(0);

    // Forward recursion to larger x, need to check for overflow first though:
    if(log(z + iter) * minus_m_minus_one > -tools::log_max_value<T>())
    {
       for(int k = 1; k <= iter; ++k)
       {
          z_plus_k_pow_minus_m_minus_one = pow(z, minus_m_minus_one);
          sum0 += z_plus_k_pow_minus_m_minus_one;
          z += 1;
       }
       sum0 *= boost::math::factorial<T>(n, pol);
    }
    else
    {
       for(int k = 1; k <= iter; ++k)
       {
          T log_term = log(z) * minus_m_minus_one + boost::math::lgamma(T(n + 1), pol);
          sum0 += exp(log_term);
          z += 1;
       }
    }
    if((n - 1) & 1)
       sum0 = -sum0;

    return sum0 + polygamma_atinfinityplus(n, z, pol, function);
  }

  template <class T, class Policy>
  T polygamma_nearzero(int n, T x, const Policy& pol, const char* function)
  {
     BOOST_MATH_STD_USING
     //
     // If we take this expansion for polygamma: http://functions.wolfram.com/06.15.06.0003.02
     // and substitute in this expression for polygamma(n, 1): http://functions.wolfram.com/06.15.03.0009.01
     // we get an alternating series for polygamma when x is small in terms of zeta functions of
     // integer arguments (which are easy to evaluate, at least when the integer is even).
     //
     // In order to avoid spurious overflow, save the n! term for later, and rescale at the end:
     //
     T scale = boost::math::factorial<T>(n, pol);
     //
     // "factorial_part" contains everything except the zeta function
     // evaluations in each term:
     //
     T factorial_part = 1;
     //
     // "prefix" is what we'll be adding the accumulated sum to, it will
     // be n! / z^(n+1), but since we're scaling by n! it's just 
     // 1 / z^(n+1) for now:
     //
     T prefix = pow(x, n + 1);
     if(prefix == 0)
        return boost::math::policies::raise_overflow_error<T>(function, 0, pol);
     prefix = 1 / prefix;
     //
     // First term in the series is necessarily < zeta(2) < 2, so
     // ignore the sum if it will have no effect on the result anyway:
     //
     if(prefix > 2 / policies::get_epsilon<T, Policy>())
        return ((n & 1) ? 1 : -1) * 
         (tools::max_value<T>() / prefix < scale ? policies::raise_overflow_error<T>(function, 0, pol) : prefix * scale);
     //
     // As this is an alternating series we could accelerate it using 
     // "Convergence Acceleration of Alternating Series",
     // Henri Cohen, Fernando Rodriguez Villegas, and Don Zagier, Experimental Mathematics, 1999.
     // In practice however, it appears not to make any difference to the number of terms
     // required except in some edge cases which are filtered out anyway before we get here.
     //
     T sum = prefix;
     for(unsigned k = 0;;)
     {
        // Get the k'th term:
        T term = factorial_part * boost::math::zeta(T(k + n + 1), pol);
        sum += term;
        // Termination condition:
        if(fabs(term) < fabs(sum * boost::math::policies::get_epsilon<T, Policy>()))
           break;
        //
        // Move on k and factorial_part:
        //
        ++k;
        factorial_part *= (-x * (n + k)) / k;
        //
        // Last chance exit:
        //
        if(k > policies::get_max_series_iterations<Policy>())
           return policies::raise_evaluation_error<T>(function, "Series did not converge, best value is %1%", sum, pol);
     }
     //
     // We need to multiply by the scale, at each stage checking for overflow:
     //
     if(boost::math::tools::max_value<T>() / scale < sum)
        return boost::math::policies::raise_overflow_error<T>(function, 0, pol);
     sum *= scale;
     return n & 1 ? sum : T(-sum);
  }

  //
  // Helper function which figures out which slot our coefficient is in
  // given an angle multiplier for the cosine term of power:
  //
  template <class Table>
  typename Table::value_type::reference dereference_table(Table& table, unsigned row, unsigned power)
  {
     return table[row][power / 2];
  }



  template <class T, class Policy>
  T poly_cot_pi(int n, T x, T xc, const Policy& pol, const char* function)
  {
     BOOST_MATH_STD_USING
     // Return n'th derivative of cot(pi*x) at x, these are simply
     // tabulated for up to n = 9, beyond that it is possible to
     // calculate coefficients as follows:
     //
     // The general form of each derivative is:
     //
     // pi^n * SUM{k=0, n} C[k,n] * cos^k(pi * x) * csc^(n+1)(pi * x)
     //
     // With constant C[0,1] = -1 and all other C[k,n] = 0;
     // Then for each k < n+1:
     // C[k-1, n+1]  -= k * C[k, n];
     // C[k+1, n+1]  += (k-n-1) * C[k, n];
     //
     // Note that there are many different ways of representing this derivative thanks to
     // the many trigonometric identies available.  In particular, the sum of powers of
     // cosines could be replaced by a sum of cosine multiple angles, and indeed if you
     // plug the derivative into Mathematica this is the form it will give.  The two
     // forms are related via the Chebeshev polynomials of the first kind and
     // T_n(cos(x)) = cos(n x).  The polynomial form has the great advantage that
     // all the cosine terms are zero at half integer arguments - right where this
     // function has it's minimum - thus avoiding cancellation error in this region.
     //
     // And finally, since every other term in the polynomials is zero, we can save
     // space by only storing the non-zero terms.  This greatly complexifies
     // subscripting the tables in the calculation, but halves the storage space
     // (and complexity for that matter).
     //
     T s = fabs(x) < fabs(xc) ? boost::math::sin_pi(x, pol) : boost::math::sin_pi(xc, pol);
     T c = boost::math::cos_pi(x, pol);
     switch(n)
     {
     case 1:
        return -constants::pi<T, Policy>() / (s * s);
     case 2:
     {
        return 2 * constants::pi<T, Policy>() * constants::pi<T, Policy>() * c / boost::math::pow<3>(s, pol);
     }
     case 3:
     {
        constexpr int P[] = { -2, -4 };
        return boost::math::pow<3>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<4>(s, pol);
     }
     case 4:
     {
        constexpr int P[] = { 16, 8 };
        return boost::math::pow<4>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<5>(s, pol);
     }
     case 5:
     {
        constexpr int P[] = { -16, -88, -16 };
        return boost::math::pow<5>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<6>(s, pol);
     }
     case 6:
     {
        constexpr int P[] = { 272, 416, 32 };
        return boost::math::pow<6>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<7>(s, pol);
     }
     case 7:
     {
        constexpr int P[] = { -272, -2880, -1824, -64 };
        return boost::math::pow<7>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<8>(s, pol);
     }
     case 8:
     {
        constexpr int P[] = { 7936, 24576, 7680, 128 };
        return boost::math::pow<8>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<9>(s, pol);
     }
     case 9:
     {
        constexpr int P[] = { -7936, -137216, -185856, -31616, -256 };
        return boost::math::pow<9>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<10>(s, pol);
     }
     case 10:
     {
        constexpr int P[] = { 353792, 1841152, 1304832, 128512, 512 };
        return boost::math::pow<10>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<11>(s, pol);
     }
     case 11:
     {
        constexpr int P[] = { -353792, -9061376, -21253376, -8728576, -518656, -1024};
        return boost::math::pow<11>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<12>(s, pol);
     }
     case 12:
     {
        constexpr int P[] = { 22368256, 175627264, 222398464, 56520704, 2084864, 2048 };
        return boost::math::pow<12>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<13>(s, pol);
     }
#ifndef BOOST_NO_LONG_LONG
     case 13:
     {
        constexpr long long P[] = { -22368256LL, -795300864LL, -2868264960LL, -2174832640LL, -357888000LL, -8361984LL, -4096 };
        return boost::math::pow<13>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<14>(s, pol);
     }
     case 14:
     {
        constexpr long long P[] = { 1903757312LL, 21016670208LL, 41731645440LL, 20261765120LL, 2230947840LL, 33497088LL, 8192 };
        return boost::math::pow<14>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<15>(s, pol);
     }
     case 15:
     {
        constexpr long long P[] = { -1903757312LL, -89702612992LL, -460858269696LL, -559148810240LL, -182172651520LL, -13754155008LL, -134094848LL, -16384 };
        return boost::math::pow<15>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<16>(s, pol);
     }
     case 16:
     {
        constexpr long long P[] = { 209865342976LL, 3099269660672LL, 8885192097792LL, 7048869314560LL, 1594922762240LL, 84134068224LL, 536608768LL, 32768 };
        return boost::math::pow<16>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<17>(s, pol);
     }
     case 17:
     {
        constexpr long long P[] = { -209865342976LL, -12655654469632LL, -87815735738368LL, -155964390375424LL, -84842998005760LL, -13684856848384LL, -511780323328LL, -2146926592LL, -65536 };
        return boost::math::pow<17>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<18>(s, pol);
     }
     case 18:
     {
        constexpr long long P[] = { 29088885112832LL, 553753414467584LL, 2165206642589696LL, 2550316668551168LL, 985278548541440LL, 115620218667008LL, 3100738912256LL, 8588754944LL, 131072 };
        return boost::math::pow<18>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<19>(s, pol);
     }
     case 19:
     {
        constexpr long long P[] = { -29088885112832LL, -2184860175433728LL, -19686087844429824LL, -48165109676113920LL, -39471306959486976LL, -11124607890751488LL, -965271355195392LL, -18733264797696LL, -34357248000LL, -262144 };
        return boost::math::pow<19>(constants::pi<T, Policy>(), pol) * tools::evaluate_even_polynomial(P, c) / boost::math::pow<20>(s, pol);
     }
     case 20:
     {
        constexpr long long P[] = { 4951498053124096LL, 118071834535526400LL, 603968063567560704LL, 990081991141490688LL, 584901762421358592LL, 122829335169859584LL, 7984436548730880LL, 112949304754176LL, 137433710592LL, 524288 };
        return boost::math::pow<20>(constants::pi<T, Policy>(), pol) * c * tools::evaluate_even_polynomial(P, c) / boost::math::pow<21>(s, pol);
     }
#endif
     }

     //
     // We'll have to compute the coefficients up to n, 
     // complexity is O(n^2) which we don't worry about for now
     // as the values are computed once and then cached.
     // However, if the final evaluation would have too many
     // terms just bail out right away:
     //
     if((unsigned)n / 2u > policies::get_max_series_iterations<Policy>())
        return policies::raise_evaluation_error<T>(function, "The value of n is so large that we're unable to compute the result in reasonable time, best guess is %1%", 0, pol);
#ifdef BOOST_HAS_THREADS
     static std::mutex m;
     std::lock_guard<std::mutex> l(m);
#endif
     static int digits = tools::digits<T>();
     static std::vector<std::vector<T> > table(1, std::vector<T>(1, T(-1)));

     int current_digits = tools::digits<T>();

     if(digits != current_digits)
     {
        // Oh my... our precision has changed!
        table = std::vector<std::vector<T> >(1, std::vector<T>(1, T(-1)));
        digits = current_digits;
     }

     int index = n - 1;

     if(index >= (int)table.size())
     {
        for(int i = (int)table.size() - 1; i < index; ++i)
        {
           int offset = i & 1; // 1 if the first cos power is 0, otherwise 0.
           int sin_order = i + 2;  // order of the sin term
           int max_cos_order = sin_order - 1;  // largest order of the polynomial of cos terms
           int max_columns = (max_cos_order - offset) / 2;  // How many entries there are in the current row.
           int next_offset = offset ? 0 : 1;
           int next_max_columns = (max_cos_order + 1 - next_offset) / 2;  // How many entries there will be in the next row
           table.push_back(std::vector<T>(next_max_columns + 1, T(0)));

           for(int column = 0; column <= max_columns; ++column)
           {
              int cos_order = 2 * column + offset;  // order of the cosine term in entry "column"
              BOOST_MATH_ASSERT(column < (int)table[i].size());
              BOOST_MATH_ASSERT((cos_order + 1) / 2 < (int)table[i + 1].size());
              table[i + 1][(cos_order + 1) / 2] += ((cos_order - sin_order) * table[i][column]) / (sin_order - 1);
              if(cos_order)
                table[i + 1][(cos_order - 1) / 2] += (-cos_order * table[i][column]) / (sin_order - 1);
           }
        }

     }
     T sum = boost::math::tools::evaluate_even_polynomial(&table[index][0], c, table[index].size());
     if(index & 1)
        sum *= c;  // First coefficient is order 1, and really an odd polynomial.
     if(sum == 0)
        return sum;
     //
     // The remaining terms are computed using logs since the powers and factorials
     // get real large real quick:
     //
     T power_terms = n * log(boost::math::constants::pi<T>());
     if(s == 0)
        return sum * boost::math::policies::raise_overflow_error<T>(function, 0, pol);
     power_terms -= log(fabs(s)) * (n + 1);
     power_terms += boost::math::lgamma(T(n), pol);
     power_terms += log(fabs(sum));

     if(power_terms > boost::math::tools::log_max_value<T>())
        return sum * boost::math::policies::raise_overflow_error<T>(function, 0, pol);

     return exp(power_terms) * ((s < 0) && ((n + 1) & 1) ? -1 : 1) * boost::math::sign(sum);
  }

  template <class T, class Policy>
  struct polygamma_initializer
  {
     struct init
     {
        init()
        {
           // Forces initialization of our table of coefficients and mutex:
           boost::math::polygamma(30, T(-2.5f), Policy());
        }
        void force_instantiate()const{}
     };
     static const init initializer;
     static void force_instantiate()
     {
        initializer.force_instantiate();
     }
  };

  template <class T, class Policy>
  const typename polygamma_initializer<T, Policy>::init polygamma_initializer<T, Policy>::initializer;
  
  template<class T, class Policy>
  inline T polygamma_imp(const int n, T x, const Policy &pol)
  {
    BOOST_MATH_STD_USING
    static const char* function = "boost::math::polygamma<%1%>(int, %1%)";
    polygamma_initializer<T, Policy>::initializer.force_instantiate();
    if(n < 0)
       return policies::raise_domain_error<T>(function, "Order must be >= 0, but got %1%", static_cast<T>(n), pol);
    if(x < 0)
    {
       if(floor(x) == x)
       {
          //
          // Result is infinity if x is odd, and a pole error if x is even.
          //
          if(lltrunc(x) & 1)
             return policies::raise_overflow_error<T>(function, 0, pol);
          else
             return policies::raise_pole_error<T>(function, "Evaluation at negative integer %1%", x, pol);
       }
       T z = 1 - x;
       T result = polygamma_imp(n, z, pol) + constants::pi<T, Policy>() * poly_cot_pi(n, z, x, pol, function);
       return n & 1 ? T(-result) : result;
    }
    //
    // Limit for use of small-x-series is chosen
    // so that the series doesn't go too divergent
    // in the first few terms.  Ordinarily this
    // would mean setting the limit to ~ 1 / n,
    // but we can tolerate a small amount of divergence:
    //
    T small_x_limit = (std::min)(T(T(5) / n), T(0.25f));
    if(x < small_x_limit)
    {
      return polygamma_nearzero(n, x, pol, function);
    }
    else if(x > 0.4F * policies::digits_base10<T, Policy>() + 4.0f * n)
    {
      return polygamma_atinfinityplus(n, x, pol, function);
    }
    else if(x == 1)
    {
       return (n & 1 ? 1 : -1) * boost::math::factorial<T>(n, pol) * boost::math::zeta(T(n + 1), pol);
    }
    else if(x == 0.5f)
    {
       T result = (n & 1 ? 1 : -1) * boost::math::factorial<T>(n, pol) * boost::math::zeta(T(n + 1), pol);
       if(fabs(result) >= ldexp(tools::max_value<T>(), -n - 1))
          return boost::math::sign(result) * policies::raise_overflow_error<T>(function, 0, pol);
       result *= ldexp(T(1), n + 1) - 1;
       return result;
    }
    else
    {
      return polygamma_attransitionplus(n, x, pol, function);
    }
  }

} } } // namespace boost::math::detail

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // _BOOST_POLYGAMMA_DETAIL_2013_07_30_HPP_


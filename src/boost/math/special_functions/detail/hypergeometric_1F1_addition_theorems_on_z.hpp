
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_HYPERGEOMETRIC_1F1_ADDITION_THEOREMS_ON_Z_HPP
#define BOOST_MATH_HYPERGEOMETRIC_1F1_ADDITION_THEOREMS_ON_Z_HPP

#include <boost/math/tools/series.hpp>

//
// This file implements the addition theorems for 1F1 on z, specifically
// each function returns 1F1[a, b, z + k] for some integer k - there's
// no particular reason why k needs to be an integer, but no reason why
// it shouldn't be either.
//
// The functions are named hypergeometric_1f1_recurrence_on_z_[plus|minus|zero]_[plus|minus|zero]
// where a "plus" indicates forward recurrence, minus backwards recurrence, and zero no recurrence.
// So for example hypergeometric_1f1_recurrence_on_z_zero_plus uses forward recurrence on b and
// hypergeometric_1f1_recurrence_on_z_minus_minus uses backwards recurrence on both a and b.
//
// See https://dlmf.nist.gov/13.13
//

  namespace boost { namespace math { namespace detail {

     //
     // This works moderately well for a < 0, but has some very strange behaviour with
     // strings of values of the same sign followed by a sign switch then another
     // series all the same sign and so on.... doesn't converge smoothly either
     // but rises and falls in wave-like behaviour.... very slow to converge...
     //
     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_minus_zero_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_minus_zero_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), b_minus_a_plus_n(b - a), a_(a), b_(b), z_(z), n(0), k(k_)
        {
           BOOST_MATH_STD_USING
           long long scale1(0), scale2(0);
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol, scale1);
           M_next = boost::math::detail::hypergeometric_1F1_imp(T(a - 1), b, z, pol, scale2);
           if (scale1 != scale2)
              M_next *= exp(T(scale2 - scale1));
           if (M > 1e10f)
           {
              // rescale:
              long long rescale = lltrunc(log(fabs(M)));
              M *= exp(T(-rescale));
              M_next *= exp(T(-rescale));
              scale1 += rescale;
           }
           scaling = scale1;
        }
        T operator()()
        {
           T result = term * M;
           term *= b_minus_a_plus_n * k / ((z_ + k) * ++n);
           b_minus_a_plus_n += 1;
           T M2 = -((2 * (a_ - n) - b_ + z_) * M_next - (a_ - n) * M) / (b_ - (a_ - n));
           M = M_next;
           M_next = M2;

           return result;
        }
        long long scale()const { return scaling; }
     private:
        T term, b_minus_a_plus_n, M, M_next, a_, b_, z_;
        int n, k;
        long long scaling;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_minus_zero(const T& a, const T& b, const T& z, int k, const Policy& pol, long long& log_scaling)
     {
        BOOST_MATH_STD_USING
           BOOST_MATH_ASSERT((z + k) / z > 0.5f);
        hypergeometric_1f1_recurrence_on_z_minus_zero_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        log_scaling += s.scale();
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result * exp(T(k)) * pow(z / (z + k), b - a);
     }

#if 0

     //
     // These are commented out as they are currently unused, but may find use in the future:
     //

     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_plus_plus_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_plus_plus_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), a_plus_n(a), b_plus_n(b), z_(z), n(0), k(k_)
        {
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol);
           M_next = boost::math::detail::hypergeometric_1F1_imp(a + 1, b + 1, z, pol);
        }
        T operator()()
        {
           T result = term * M;
           term *= a_plus_n * k / (b_plus_n * ++n);
           a_plus_n += 1;
           b_plus_n += 1;
           // The a_plus_n == 0 case below isn't actually correct, but doesn't matter as that term will be zero
           // anyway, we just need to not divide by zero and end up with a NaN in the result.
           T M2 = (a_plus_n == -1) ? 1 : (a_plus_n == 0) ? 0 : (M_next * b_plus_n * (1 - b_plus_n + z_) + b_plus_n * (b_plus_n - 1) * M) / (a_plus_n * z_);
           M = M_next;
           M_next = M2;
           return result;
        }
        T term, a_plus_n, b_plus_n, M, M_next, z_;
        int n, k;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_plus_plus(const T& a, const T& b, const T& z, int k, const Policy& pol)
     {
        hypergeometric_1f1_recurrence_on_z_plus_plus_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result;
     }

     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_zero_minus_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_zero_minus_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), b_pochhammer(1 - b), x_k_power(-k_ / z), b_minus_n(b), a_(a), z_(z), b_(b), n(0), k(k_)
        {
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol);
           M_next = boost::math::detail::hypergeometric_1F1_imp(a, b - 1, z, pol);
        }
        T operator()()
        {
           BOOST_MATH_STD_USING
           T result = term * M;
           term *= b_pochhammer * x_k_power / ++n;
           b_pochhammer += 1;
           b_minus_n -= 1;
           T M2 = (M_next * b_minus_n * (1 - b_minus_n - z_) + z_ * (b_minus_n - a_) * M) / (-b_minus_n * (b_minus_n - 1));
           M = M_next;
           M_next = M2;
           return result;
        }
        T term, b_pochhammer, x_k_power, M, M_next, b_minus_n, a_, z_, b_;
        int n, k;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_zero_minus(const T& a, const T& b, const T& z, int k, const Policy& pol)
     {
        BOOST_MATH_STD_USING
           BOOST_MATH_ASSERT(abs(k) < fabs(z));
        hypergeometric_1f1_recurrence_on_z_zero_minus_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result * pow((z + k) / z, 1 - b);
     }

     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_plus_zero_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_plus_zero_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), a_pochhammer(a), z_plus_k(z + k_), b_(b), a_(a), z_(z), n(0), k(k_)
        {
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol);
           M_next = boost::math::detail::hypergeometric_1F1_imp(a + 1, b, z, pol);
        }
        T operator()()
        {
           T result = term * M;
           term *= a_pochhammer * k / (++n * z_plus_k);
           a_pochhammer += 1;
           T M2 = (a_pochhammer == -1) ? 1 : (a_pochhammer == 0) ? 0 : (M_next * (2 * a_pochhammer - b_ + z_) + (b_ - a_pochhammer) * M) / a_pochhammer;
           M = M_next;
           M_next = M2;

           return result;
        }
        T term, a_pochhammer, z_plus_k, M, M_next, b_minus_n, a_, b_, z_;
        int n, k;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_plus_zero(const T& a, const T& b, const T& z, int k, const Policy& pol)
     {
        BOOST_MATH_STD_USING
           BOOST_MATH_ASSERT(k / z > -0.5f);
        //BOOST_MATH_ASSERT(floor(a) != a || a > 0);
        hypergeometric_1f1_recurrence_on_z_plus_zero_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result * pow(z / (z + k), a);
     }

     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_zero_plus_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_zero_plus_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), b_minus_a_plus_n(b - a), b_plus_n(b), a_(a), z_(z), n(0), k(k_)
        {
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol);
           M_next = boost::math::detail::hypergeometric_1F1_imp(a, b + 1, z, pol);
        }
        T operator()()
        {
           T result = term * M;
           term *= b_minus_a_plus_n * -k / (b_plus_n * ++n);
           b_minus_a_plus_n += 1;
           b_plus_n += 1;
           T M2 = (b_plus_n * (b_plus_n - 1) * M + b_plus_n * (1 - b_plus_n - z_) * M_next) / (-z_ * b_minus_a_plus_n);
           M = M_next;
           M_next = M2;

           return result;
        }
        T term, b_minus_a_plus_n, M, M_next, b_minus_n, a_, b_plus_n, z_;
        int n, k;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_zero_plus(const T& a, const T& b, const T& z, int k, const Policy& pol)
     {
        BOOST_MATH_STD_USING
           hypergeometric_1f1_recurrence_on_z_zero_plus_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result * exp(T(k));
     }
     //
     // I'm unable to find any situation where this series isn't divergent and therefore 
     // is probably quite useless:
     //
     template <class T, class Policy>
     struct hypergeometric_1f1_recurrence_on_z_minus_minus_series
     {
        typedef T result_type;

        hypergeometric_1f1_recurrence_on_z_minus_minus_series(const T& a, const T& b, const T& z, int k_, const Policy& pol)
           : term(1), one_minus_b_plus_n(1 - b), a_(a), b_(b), z_(z), n(0), k(k_)
        {
           M = boost::math::detail::hypergeometric_1F1_imp(a, b, z, pol);
           M_next = boost::math::detail::hypergeometric_1F1_imp(a - 1, b - 1, z, pol);
        }
        T operator()()
        {
           T result = term * M;
           term *= one_minus_b_plus_n * k / (z_ * ++n);
           one_minus_b_plus_n += 1;
           T M2 = -((b_ - n) * (1 - b_ + n + z_) * M_next - (a_ - n) * z_ * M) / ((b_ - n) * (b_ - n - 1));
           M = M_next;
           M_next = M2;

           return result;
        }
        T term, one_minus_b_plus_n, M, M_next, a_, b_, z_;
        int n, k;
     };

     template <class T, class Policy>
     T hypergeometric_1f1_recurrence_on_z_minus_minus(const T& a, const T& b, const T& z, int k, const Policy& pol)
     {
        BOOST_MATH_STD_USING
           hypergeometric_1f1_recurrence_on_z_minus_minus_series<T, Policy> s(a, b, z, k, pol);
        std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
        T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
        boost::math::policies::check_series_iterations<T>("boost::math::hypergeometric_1f1_recurrence_on_z_plus_plus<%1%>(%1%,%1%,%1%)", max_iter, pol);
        return result * exp(T(k)) * pow((z + k) / z, 1 - b);
     }
#endif

  } } } // namespaces

#endif // BOOST_MATH_HYPERGEOMETRIC_1F1_ADDITION_THEOREMS_ON_Z_HPP

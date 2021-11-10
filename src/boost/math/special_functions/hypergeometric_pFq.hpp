
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_HYPERGEOMETRIC_PFQ_HPP
#define BOOST_MATH_HYPERGEOMETRIC_PFQ_HPP

#include <boost/math/special_functions/detail/hypergeometric_pFq_checked_series.hpp>
#include <chrono>
#include <initializer_list>

namespace boost {
   namespace math {

      namespace detail {

         struct pFq_termination_exception : public std::runtime_error
         {
            pFq_termination_exception(const char* p) : std::runtime_error(p) {}
         };

         struct timed_iteration_terminator
         {
            timed_iteration_terminator(std::uintmax_t i, double t) : max_iter(i), max_time(t), start_time(std::chrono::system_clock::now()) {}

            bool operator()(std::uintmax_t iter)const
            {
               if (iter > max_iter)
                  boost::throw_exception(boost::math::detail::pFq_termination_exception("pFq exceeded maximum permitted iterations."));
               if (std::chrono::duration<double>(std::chrono::system_clock::now() - start_time).count() > max_time)
                  boost::throw_exception(boost::math::detail::pFq_termination_exception("pFq exceeded maximum permitted evaluation time."));
               return false;
            }

            std::uintmax_t max_iter;
            double max_time;
            std::chrono::system_clock::time_point start_time;
         };

      }

      template <class Seq, class Real, class Policy>
      inline typename tools::promote_args<Real, typename Seq::value_type>::type hypergeometric_pFq(const Seq& aj, const Seq& bj, const Real& z, Real* p_abs_error, const Policy& pol)
      {
         typedef typename tools::promote_args<Real, typename Seq::value_type>::type result_type;
         typedef typename policies::evaluation<result_type, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy,
            policies::promote_float<false>,
            policies::promote_double<false>,
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;

         BOOST_MATH_STD_USING

         long long scale = 0;
         std::pair<value_type, value_type> r = boost::math::detail::hypergeometric_pFq_checked_series_impl(aj, bj, value_type(z), pol, boost::math::detail::iteration_terminator(boost::math::policies::get_max_series_iterations<forwarding_policy>()), scale);
         r.first *= exp(Real(scale));
         r.second *= exp(Real(scale));
         if (p_abs_error)
            *p_abs_error = static_cast<Real>(r.second) * boost::math::tools::epsilon<Real>();
         return policies::checked_narrowing_cast<result_type, Policy>(r.first, "boost::math::hypergeometric_pFq<%1%>(%1%,%1%,%1%)");
      }

      template <class Seq, class Real>
      inline typename tools::promote_args<Real, typename Seq::value_type>::type hypergeometric_pFq(const Seq& aj, const Seq& bj, const Real& z, Real* p_abs_error = 0)
      {
         return hypergeometric_pFq(aj, bj, z, p_abs_error, boost::math::policies::policy<>());
      }

      template <class R, class Real, class Policy>
      inline typename tools::promote_args<Real, R>::type hypergeometric_pFq(const std::initializer_list<R>& aj, const std::initializer_list<R>& bj, const Real& z, Real* p_abs_error, const Policy& pol)
      {
         return hypergeometric_pFq<std::initializer_list<R>, Real, Policy>(aj, bj, z, p_abs_error, pol);
      }
      
      template <class R, class Real>
      inline typename tools::promote_args<Real, R>::type  hypergeometric_pFq(const std::initializer_list<R>& aj, const std::initializer_list<R>& bj, const Real& z, Real* p_abs_error = 0)
      {
         return hypergeometric_pFq<std::initializer_list<R>, Real>(aj, bj, z, p_abs_error);
      }

      template <class T>
      struct scoped_precision
      {
         scoped_precision(unsigned p)
         {
            old_p = T::default_precision();
            T::default_precision(p);
         }
         ~scoped_precision()
         {
            T::default_precision(old_p);
         }
         unsigned old_p;
      };

      template <class Seq, class Real, class Policy>
      Real hypergeometric_pFq_precision(const Seq& aj, const Seq& bj, Real z, unsigned digits10, double timeout, const Policy& pol)
      {
         unsigned current_precision = digits10 + 5;

         for (auto ai = aj.begin(); ai != aj.end(); ++ai)
         {
            current_precision = (std::max)(current_precision, ai->precision());
         }
         for (auto bi = bj.begin(); bi != bj.end(); ++bi)
         {
            current_precision = (std::max)(current_precision, bi->precision());
         }
         current_precision = (std::max)(current_precision, z.precision());

         Real r, norm;
         std::vector<Real> aa(aj), bb(bj);
         do
         {
            scoped_precision<Real> p(current_precision);
            for (auto ai = aa.begin(); ai != aa.end(); ++ai)
               ai->precision(current_precision);
            for (auto bi = bb.begin(); bi != bb.end(); ++bi)
               bi->precision(current_precision);
            z.precision(current_precision);
            try
            {
               long long scale = 0;
               std::pair<Real, Real> rp = boost::math::detail::hypergeometric_pFq_checked_series_impl(aa, bb, z, pol, boost::math::detail::timed_iteration_terminator(boost::math::policies::get_max_series_iterations<Policy>(), timeout), scale);
               rp.first *= exp(Real(scale));
               rp.second *= exp(Real(scale));

               r = rp.first;
               norm = rp.second;

               unsigned cancellation;
               try {
                  cancellation = itrunc(log10(abs(norm / r)));
               }
               catch (const boost::math::rounding_error&)
               {
                  // Happens when r is near enough zero:
                  cancellation = UINT_MAX;
               }
               if (cancellation >= current_precision - 1)
               {
                  current_precision *= 2;
                  continue;
               }
               unsigned precision_obtained = current_precision - 1 - cancellation;
               if (precision_obtained < digits10)
               {
                  current_precision += digits10 - precision_obtained + 5;
               }
               else
                  break;
            }
            catch (const boost::math::evaluation_error&)
            {
               current_precision *= 2;
            }
            catch (const detail::pFq_termination_exception& e)
            {
               //
               // Either we have exhausted the number of series iterations, or the timeout.
               // Either way we quit now.
               throw boost::math::evaluation_error(e.what());
            }
         } while (true);

         return r;
      }
      template <class Seq, class Real>
      Real hypergeometric_pFq_precision(const Seq& aj, const Seq& bj, const Real& z, unsigned digits10, double timeout = 0.5)
      {
         return hypergeometric_pFq_precision(aj, bj, z, digits10, timeout, boost::math::policies::policy<>());
      }

      template <class Real, class Policy>
      Real hypergeometric_pFq_precision(const std::initializer_list<Real>& aj, const std::initializer_list<Real>& bj, const Real& z, unsigned digits10, double timeout, const Policy& pol)
      {
         return hypergeometric_pFq_precision< std::initializer_list<Real>, Real>(aj, bj, z, digits10, timeout, pol);
      }
      template <class Real>
      Real hypergeometric_pFq_precision(const std::initializer_list<Real>& aj, const std::initializer_list<Real>& bj, const Real& z, unsigned digits10, double timeout = 0.5)
      {
         return hypergeometric_pFq_precision< std::initializer_list<Real>, Real>(aj, bj, z, digits10, timeout, boost::math::policies::policy<>());
      }

   }
} // namespaces

#endif // BOOST_MATH_BESSEL_ITERATORS_HPP

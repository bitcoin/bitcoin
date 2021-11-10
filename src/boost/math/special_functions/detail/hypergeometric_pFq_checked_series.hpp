///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HYPERGEOMETRIC_PFQ_SERIES_HPP_
#define BOOST_HYPERGEOMETRIC_PFQ_SERIES_HPP_

#ifndef BOOST_MATH_PFQ_MAX_B_TERMS
#  define BOOST_MATH_PFQ_MAX_B_TERMS 5
#endif

#include <array>
#include <cstdint>
#include <boost/math/special_functions/detail/hypergeometric_series.hpp>

  namespace boost { namespace math { namespace detail {

     template <class Seq, class Real>
     unsigned set_crossover_locations(const Seq& aj, const Seq& bj, const Real& z, unsigned int* crossover_locations)
     {
        BOOST_MATH_STD_USING
        unsigned N_terms = 0;

        if(aj.size() == 1 && bj.size() == 1)
        {
           //
           // For 1F1 we can work out where the peaks in the series occur,
           //  which is to say when:
           //
           // (a + k)z / (k(b + k)) == +-1
           //
           // Then we are at either a maxima or a minima in the series, and the
           // last such point must be a maxima since the series is globally convergent.
           // Potentially then we are solving 2 quadratic equations and have up to 4
           // solutions, any solutions which are complex or negative are discarded,
           // leaving us with 4 options:
           //
           // 0 solutions: The series is directly convergent.
           // 1 solution : The series diverges to a maxima before converging.
           // 2 solutions: The series is initially convergent, followed by divergence to a maxima before final convergence.
           // 3 solutions: The series diverges to a maxima, converges to a minima before diverging again to a second maxima before final convergence.
           // 4 solutions: The series converges to a minima before diverging to a maxima, converging to a minima, diverging to a second maxima and then converging.
           //
           // The first 2 situations are adequately handled by direct series evaluation, while the 2,3 and 4 solutions are not.
           //
           Real a = *aj.begin();
           Real b = *bj.begin();
           Real sq = 4 * a * z + b * b - 2 * b * z + z * z;
           if (sq >= 0)
           {
              Real t = (-sqrt(sq) - b + z) / 2;
              if (t >= 0)
              {
                 crossover_locations[N_terms] = itrunc(t);
                 ++N_terms;
              }
              t = (sqrt(sq) - b + z) / 2;
              if (t >= 0)
              {
                 crossover_locations[N_terms] = itrunc(t);
                 ++N_terms;
              }
           }
           sq = -4 * a * z + b * b + 2 * b * z + z * z;
           if (sq >= 0)
           {
              Real t = (-sqrt(sq) - b - z) / 2;
              if (t >= 0)
              {
                 crossover_locations[N_terms] = itrunc(t);
                 ++N_terms;
              }
              t = (sqrt(sq) - b - z) / 2;
              if (t >= 0)
              {
                 crossover_locations[N_terms] = itrunc(t);
                 ++N_terms;
              }
           }
           std::sort(crossover_locations, crossover_locations + N_terms, std::less<Real>());
           //
           // Now we need to discard every other terms, as these are the minima:
           //
           switch (N_terms)
           {
           case 0:
           case 1:
              break;
           case 2:
              crossover_locations[0] = crossover_locations[1];
              --N_terms;
              break;
           case 3:
              crossover_locations[1] = crossover_locations[2];
              --N_terms;
              break;
           case 4:
              crossover_locations[0] = crossover_locations[1];
              crossover_locations[1] = crossover_locations[3];
              N_terms -= 2;
              break;
           }
        }
        else
        {
           unsigned n = 0;
           for (auto bi = bj.begin(); bi != bj.end(); ++bi, ++n)
           {
              crossover_locations[n] = *bi >= 0 ? 0 : itrunc(-*bi) + 1;
           }
           std::sort(crossover_locations, crossover_locations + bj.size(), std::less<Real>());
           N_terms = (unsigned)bj.size();
        }
        return N_terms;
     }

     template <class Seq, class Real, class Policy, class Terminal>
     std::pair<Real, Real> hypergeometric_pFq_checked_series_impl(const Seq& aj, const Seq& bj, const Real& z, const Policy& pol, const Terminal& termination, long long& log_scale)
     {
        BOOST_MATH_STD_USING
        Real result = 1;
        Real abs_result = 1;
        Real term = 1;
        Real term0 = 0;
        Real tol = boost::math::policies::get_epsilon<Real, Policy>();
        std::uintmax_t k = 0;
        Real upper_limit(sqrt(boost::math::tools::max_value<Real>())), diff;
        Real lower_limit(1 / upper_limit);
        long long log_scaling_factor = lltrunc(boost::math::tools::log_max_value<Real>()) - 2;
        Real scaling_factor = exp(Real(log_scaling_factor));
        Real term_m1;
        long long local_scaling = 0;

        if ((aj.size() == 1) && (bj.size() == 0))
        {
           if (fabs(z) > 1)
           {
              if ((z > 0) && (floor(*aj.begin()) != *aj.begin()))
              {
                 Real r = policies::raise_domain_error("boost::math::hypergeometric_pFq", "Got p == 1 and q == 0 and |z| > 1, result is imaginary", z, pol);
                 return std::make_pair(r, r);
              }
              std::pair<Real, Real> r = hypergeometric_pFq_checked_series_impl(aj, bj, Real(1 / z), pol, termination, log_scale);
              Real mul = pow(-z, -*aj.begin());
              r.first *= mul;
              r.second *= mul;
              return r;
           }
        }

        if (aj.size() > bj.size())
        {
           if (aj.size() == bj.size() + 1)
           {
              if (fabs(z) > 1)
              {
                 Real r = policies::raise_domain_error("boost::math::hypergeometric_pFq", "Got p == q+1 and |z| > 1, series does not converge", z, pol);
                 return std::make_pair(r, r);
              }
              if (fabs(z) == 1)
              {
                 Real s = 0;
                 for (auto i = bj.begin(); i != bj.end(); ++i)
                    s += *i;
                 for (auto i = aj.begin(); i != aj.end(); ++i)
                    s -= *i;
                 if ((z == 1) && (s <= 0))
                 {
                    Real r = policies::raise_domain_error("boost::math::hypergeometric_pFq", "Got p == q+1 and |z| == 1, in a situation where the series does not converge", z, pol);
                    return std::make_pair(r, r);
                 }
                 if ((z == -1) && (s <= -1))
                 {
                    Real r = policies::raise_domain_error("boost::math::hypergeometric_pFq", "Got p == q+1 and |z| == 1, in a situation where the series does not converge", z, pol);
                    return std::make_pair(r, r);
                 }
              }
           }
           else
           {
              Real r = policies::raise_domain_error("boost::math::hypergeometric_pFq", "Got p > q+1, series does not converge", z, pol);
              return std::make_pair(r, r);
           }
        }

        while (!termination(k))
        {
           for (auto ai = aj.begin(); ai != aj.end(); ++ai)
           {
              term *= *ai + k;
           }
           if (term == 0)
           {
              // There is a negative integer in the aj's:
              return std::make_pair(result, abs_result);
           }
           for (auto bi = bj.begin(); bi != bj.end(); ++bi)
           {
              if (*bi + k == 0)
              {
                 // The series is undefined:
                 result = boost::math::policies::raise_domain_error("boost::math::hypergeometric_pFq<%1%>", "One of the b values was the negative integer %1%", *bi, pol);
                 return std::make_pair(result, result);
              }
              term /= *bi + k;
           }
           term *= z;
           ++k;
           term /= k;
           //std::cout << k << " " << *bj.begin() + k << " " << result << " " << term << /*" " << term_at_k(*aj.begin(), *bj.begin(), z, k, pol) <<*/ std::endl;
           result += term;
           abs_result += abs(term);
           //std::cout << "k = " << k << " term = " << term * exp(log_scale) << " result = " << result * exp(log_scale) << " abs_result = " << abs_result * exp(log_scale) << std::endl;

           //
           // Rescaling:
           //
           if (fabs(abs_result) >= upper_limit)
           {
              abs_result /= scaling_factor;
              result /= scaling_factor;
              term /= scaling_factor;
              log_scale += log_scaling_factor;
              local_scaling += log_scaling_factor;
           }
           if (fabs(abs_result) < lower_limit)
           {
              abs_result *= scaling_factor;
              result *= scaling_factor;
              term *= scaling_factor;
              log_scale -= log_scaling_factor;
              local_scaling -= log_scaling_factor;
           }

           if ((abs(result * tol) > abs(term)) && (abs(term0) > abs(term)))
              break;
           if (abs_result * tol > abs(result))
           {
              // We have no correct bits in the result... just give up!
              result = boost::math::policies::raise_evaluation_error("boost::math::hypergeometric_pFq<%1%>", "Cancellation is so severe that no bits in the reuslt are correct, last result was %1%", Real(result * exp(Real(log_scale))), pol);
              return std::make_pair(result, result);
           }
           term0 = term;
        }
        //std::cout << "result = " << result << std::endl;
        //std::cout << "local_scaling = " << local_scaling << std::endl;
        //std::cout << "Norm result = " << std::setprecision(35) << boost::multiprecision::mpfr_float_50(result) * exp(boost::multiprecision::mpfr_float_50(local_scaling)) << std::endl;
        //
        // We have to be careful when one of the b's crosses the origin:
        //
        if(bj.size() > BOOST_MATH_PFQ_MAX_B_TERMS)
           policies::raise_domain_error<Real>("boost::math::hypergeometric_pFq<%1%>(Seq, Seq, %1%)", 
              "The number of b terms must be less than the value of BOOST_MATH_PFQ_MAX_B_TERMS (" BOOST_STRINGIZE(BOOST_MATH_PFQ_MAX_B_TERMS)  "), but got %1%.",
              Real(bj.size()), pol);

        unsigned crossover_locations[BOOST_MATH_PFQ_MAX_B_TERMS];

        unsigned N_crossovers = set_crossover_locations(aj, bj, z, crossover_locations);

        bool terminate = false;   // Set to true if one of the a's passes through the origin and terminates the series.

        for (unsigned n = 0; n < N_crossovers; ++n)
        {
           if (k < crossover_locations[n])
           {
              for (auto ai = aj.begin(); ai != aj.end(); ++ai)
              {
                 if ((*ai < 0) && (floor(*ai) == *ai) && (*ai > crossover_locations[n]))
                    return std::make_pair(result, abs_result);  // b's will never cross the origin!
              }
              //
              // local results:
              //
              Real loop_result = 0;
              Real loop_abs_result = 0;
              long long loop_scale = 0;
              //
              // loop_error_scale will be used to increase the size of the error
              // estimate (absolute sum), based on the errors inherent in calculating 
              // the pochhammer symbols.
              //
              Real loop_error_scale = 0;
              //boost::multiprecision::mpfi_float err_est = 0;
              //
              // b hasn't crossed the origin yet and the series may spring back into life at that point
              // so we need to jump forward to that term and then evaluate forwards and backwards from there:
              //
              unsigned s = crossover_locations[n];
              std::uintmax_t backstop = k;
              long long s1(1), s2(1);
              term = 0;
              for (auto ai = aj.begin(); ai != aj.end(); ++ai)
              {
                 if ((floor(*ai) == *ai) && (*ai < 0) && (-*ai <= s))
                 {
                    // One of the a terms has passed through zero and terminated the series:
                    terminate = true;
                    break;
                 }
                 else
                 {
                    int ls = 1;
                    Real p = log_pochhammer(*ai, s, pol, &ls);
                    s1 *= ls;
                    term += p;
                    loop_error_scale = (std::max)(p, loop_error_scale);
                    //err_est += boost::multiprecision::mpfi_float(p);
                 }
              }
              //std::cout << "term = " << term << std::endl;
              if (terminate)
                 break;
              for (auto bi = bj.begin(); bi != bj.end(); ++bi)
              {
                 int ls = 1;
                 Real p = log_pochhammer(*bi, s, pol, &ls);
                 s2 *= ls;
                 term -= p;
                 loop_error_scale = (std::max)(p, loop_error_scale);
                 //err_est -= boost::multiprecision::mpfi_float(p);
              }
              //std::cout << "term = " << term << std::endl;
              Real p = lgamma(Real(s + 1), pol);
              term -= p;
              loop_error_scale = (std::max)(p, loop_error_scale);
              //err_est -= boost::multiprecision::mpfi_float(p);
              p = s * log(fabs(z));
              term += p;
              loop_error_scale = (std::max)(p, loop_error_scale);
              //err_est += boost::multiprecision::mpfi_float(p);
              //err_est = exp(err_est);
              //std::cout << err_est << std::endl;
              //
              // Convert loop_error scale to the absolute error
              // in term after exp is applied:
              //
              loop_error_scale *= tools::epsilon<Real>();
              //
              // Convert to relative error after exp:
              //
              loop_error_scale = fabs(expm1(loop_error_scale, pol));
              //
              // Convert to multiplier for the error term:
              //
              loop_error_scale /= tools::epsilon<Real>();

              if (z < 0)
                 s1 *= (s & 1 ? -1 : 1);

              if (term <= tools::log_min_value<Real>())
              {
                 // rescale if we can:
                 long long scale = lltrunc(floor(term - tools::log_min_value<Real>()) - 2);
                 term -= scale;
                 loop_scale += scale;
              }
               if (term > 10)
               {
                  int scale = itrunc(floor(term));
                  term -= scale;
                  loop_scale += scale;
               }
               //std::cout << "term = " << term << std::endl;
               term = s1 * s2 * exp(term);
               //std::cout << "term = " << term << std::endl;
               //std::cout << "loop_scale = " << loop_scale << std::endl;
               k = s;
               term0 = term;
               long long saved_loop_scale = loop_scale;
               bool terms_are_growing = true;
               bool trivial_small_series_check = false;
               do
               {
                  loop_result += term;
                  loop_abs_result += fabs(term);
                  //std::cout << "k = " << k << " term = " << term * exp(loop_scale) << " result = " << loop_result * exp(loop_scale) << " abs_result = " << loop_abs_result * exp(loop_scale) << std::endl;
                  if (fabs(loop_result) >= upper_limit)
                  {
                     loop_result /= scaling_factor;
                     loop_abs_result /= scaling_factor;
                     term /= scaling_factor;
                     loop_scale += log_scaling_factor;
                  }
                  if (fabs(loop_result) < lower_limit)
                  {
                     loop_result *= scaling_factor;
                     loop_abs_result *= scaling_factor;
                     term *= scaling_factor;
                     loop_scale -= log_scaling_factor;
                  }
                  term_m1 = term;
                  for (auto ai = aj.begin(); ai != aj.end(); ++ai)
                  {
                     term *= *ai + k;
                  }
                  if (term == 0)
                  {
                     // There is a negative integer in the aj's:
                     return std::make_pair(result, abs_result);
                  }
                  for (auto bi = bj.begin(); bi != bj.end(); ++bi)
                  {
                     if (*bi + k == 0)
                     {
                        // The series is undefined:
                        result = boost::math::policies::raise_domain_error("boost::math::hypergeometric_pFq<%1%>", "One of the b values was the negative integer %1%", *bi, pol);
                        return std::make_pair(result, result);
                     }
                     term /= *bi + k;
                  }
                  term *= z / (k + 1);

                  ++k;
                  diff = fabs(term / loop_result);
                  terms_are_growing = fabs(term) > fabs(term_m1);
                  if (!trivial_small_series_check && !terms_are_growing)
                  {
                     //
                     // Now that we have started to converge, check to see if the value of
                     // this local sum is trivially small compared to the result.  If so
                     // abort this part of the series.
                     //
                     trivial_small_series_check = true;
                     Real d; 
                     if (loop_scale > local_scaling)
                     {
                        long long rescale = local_scaling - loop_scale;
                        if (rescale < tools::log_min_value<Real>())
                           d = 1;  // arbitrary value, we want to keep going
                        else
                           d = fabs(term / (result * exp(Real(rescale))));
                     }
                     else
                     {
                        long long rescale = loop_scale - local_scaling;
                        if (rescale < tools::log_min_value<Real>())
                           d = 0;  // terminate this loop
                        else
                           d = fabs(term * exp(Real(rescale)) / result);
                     }
                     if (d < boost::math::policies::get_epsilon<Real, Policy>())
                        break;
                  }
               } while (!termination(k - s) && ((diff > boost::math::policies::get_epsilon<Real, Policy>()) || terms_are_growing));

               //std::cout << "Norm loop result = " << std::setprecision(35) << boost::multiprecision::mpfr_float_50(loop_result)* exp(boost::multiprecision::mpfr_float_50(loop_scale)) << std::endl;
               //
               // We now need to combine the results of the first series summation with whatever
               // local results we have now.  First though, rescale abs_result by loop_error_scale
               // to factor in the error in the pochhammer terms at the start of this block:
               //
               std::uintmax_t next_backstop = k;
               loop_abs_result += loop_error_scale * fabs(loop_result);
               if (loop_scale > local_scaling)
               {
                  //
                  // Need to shrink previous result:
                  //
                  long long rescale = local_scaling - loop_scale;
                  local_scaling = loop_scale;
                  log_scale -= rescale;
                  Real ex = exp(Real(rescale));
                  result *= ex;
                  abs_result *= ex;
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               else if (local_scaling > loop_scale)
               {
                  //
                  // Need to shrink local result:
                  //
                  long long rescale = loop_scale - local_scaling;
                  Real ex = exp(Real(rescale));
                  loop_result *= ex;
                  loop_abs_result *= ex;
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               else
               {
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               //
               // Now go backwards as well:
               //
               k = s;
               term = term0;
               loop_result = 0;
               loop_abs_result = 0;
               loop_scale = saved_loop_scale;
               trivial_small_series_check = false;
               do
               {
                  --k;
                  if (k == backstop)
                     break;
                  term_m1 = term;
                  for (auto ai = aj.begin(); ai != aj.end(); ++ai)
                  {
                     term /= *ai + k;
                  }
                  for (auto bi = bj.begin(); bi != bj.end(); ++bi)
                  {
                     if (*bi + k == 0)
                     {
                        // The series is undefined:
                        result = boost::math::policies::raise_domain_error("boost::math::hypergeometric_pFq<%1%>", "One of the b values was the negative integer %1%", *bi, pol);
                        return std::make_pair(result, result);
                     }
                     term *= *bi + k;
                  }
                  term *= (k + 1) / z;
                  loop_result += term;
                  loop_abs_result += fabs(term);

                  if (!trivial_small_series_check && (fabs(term) < fabs(term_m1)))
                  {
                     //
                     // Now that we have started to converge, check to see if the value of
                     // this local sum is trivially small compared to the result.  If so
                     // abort this part of the series.
                     //
                     trivial_small_series_check = true;
                     Real d;
                     if (loop_scale > local_scaling)
                     {
                        long long rescale = local_scaling - loop_scale;
                        if (rescale < tools::log_min_value<Real>())
                           d = 1;  // keep going
                        else
                           d = fabs(term / (result * exp(Real(rescale))));
                     }
                     else
                     {
                        long long rescale = loop_scale - local_scaling;
                        if (rescale < tools::log_min_value<Real>())
                           d = 0;  // stop, underflow
                        else
                           d = fabs(term * exp(Real(rescale)) / result);
                     }
                     if (d < boost::math::policies::get_epsilon<Real, Policy>())
                        break;
                  }

                  //std::cout << "k = " << k << " result = " << result << " abs_result = " << abs_result << std::endl;
                  if (fabs(loop_result) >= upper_limit)
                  {
                     loop_result /= scaling_factor;
                     loop_abs_result /= scaling_factor;
                     term /= scaling_factor;
                     loop_scale += log_scaling_factor;
                  }
                  if (fabs(loop_result) < lower_limit)
                  {
                     loop_result *= scaling_factor;
                     loop_abs_result *= scaling_factor;
                     term *= scaling_factor;
                     loop_scale -= log_scaling_factor;
                  }
                  diff = fabs(term / loop_result);
               } while (!termination(s - k) && ((diff > boost::math::policies::get_epsilon<Real, Policy>()) || (fabs(term) > fabs(term_m1))));

               //std::cout << "Norm loop result = " << std::setprecision(35) << boost::multiprecision::mpfr_float_50(loop_result)* exp(boost::multiprecision::mpfr_float_50(loop_scale)) << std::endl;
               //
               // We now need to combine the results of the first series summation with whatever
               // local results we have now.  First though, rescale abs_result by loop_error_scale
               // to factor in the error in the pochhammer terms at the start of this block:
               //
               loop_abs_result += loop_error_scale * fabs(loop_result);
               //
               if (loop_scale > local_scaling)
               {
                  //
                  // Need to shrink previous result:
                  //
                  long long rescale = local_scaling - loop_scale;
                  local_scaling = loop_scale;
                  log_scale -= rescale;
                  Real ex = exp(Real(rescale));
                  result *= ex;
                  abs_result *= ex;
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               else if (local_scaling > loop_scale)
               {
                  //
                  // Need to shrink local result:
                  //
                  long long rescale = loop_scale - local_scaling;
                  Real ex = exp(Real(rescale));
                  loop_result *= ex;
                  loop_abs_result *= ex;
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               else
               {
                  result += loop_result;
                  abs_result += loop_abs_result;
               }
               //
               // Reset k to the largest k we reached
               //
               k = next_backstop;
           }
        }

        return std::make_pair(result, abs_result);
     }

     struct iteration_terminator
     {
        iteration_terminator(std::uintmax_t i) : m(i) {}

        bool operator()(std::uintmax_t v) const { return v >= m; }

        std::uintmax_t m;
     };

     template <class Seq, class Real, class Policy>
     Real hypergeometric_pFq_checked_series_impl(const Seq& aj, const Seq& bj, const Real& z, const Policy& pol, long long& log_scale)
     {
        BOOST_MATH_STD_USING
        iteration_terminator term(boost::math::policies::get_max_series_iterations<Policy>());
        std::pair<Real, Real> result = hypergeometric_pFq_checked_series_impl(aj, bj, z, pol, term, log_scale);
        //
        // Check to see how many digits we've lost, if it's more than half, raise an evaluation error -
        // this is an entirely arbitrary cut off, but not unreasonable.
        //
        if (result.second * sqrt(boost::math::policies::get_epsilon<Real, Policy>()) > abs(result.first))
        {
           return boost::math::policies::raise_evaluation_error("boost::math::hypergeometric_pFq<%1%>", "Cancellation is so severe that fewer than half the bits in the result are correct, last result was %1%", Real(result.first * exp(Real(log_scale))), pol);
        }
        return result.first;
     }

     template <class Real, class Policy>
     inline Real hypergeometric_1F1_checked_series_impl(const Real& a, const Real& b, const Real& z, const Policy& pol, long long& log_scale)
     {
        std::array<Real, 1> aj = { a };
        std::array<Real, 1> bj = { b };
        return hypergeometric_pFq_checked_series_impl(aj, bj, z, pol, log_scale);
     }

  } } } // namespaces

#endif // BOOST_HYPERGEOMETRIC_PFQ_SERIES_HPP_

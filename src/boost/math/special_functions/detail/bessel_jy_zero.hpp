//  Copyright (c) 2013 Christopher Kormanyos
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is based on an earlier work:
// "Algorithm 910: A Portable C++ Multiple-Precision System for Special-Function Calculations",
// in ACM TOMS, {VOL 37, ISSUE 4, (February 2011)} (C) ACM, 2011. http://doi.acm.org/10.1145/1916461.1916469
//
// This header contains implementation details for estimating the zeros
// of cylindrical Bessel and Neumann functions on the positive real axis.
// Support is included for both positive as well as negative order.
// Various methods are used to estimate the roots. These include
// empirical curve fitting and McMahon's asymptotic approximation
// for small order, uniform asymptotic expansion for large order,
// and iteration and root interlacing for negative order.
//
#ifndef BOOST_MATH_BESSEL_JY_ZERO_2013_01_18_HPP_
  #define BOOST_MATH_BESSEL_JY_ZERO_2013_01_18_HPP_

  #include <algorithm>
  #include <boost/math/constants/constants.hpp>
  #include <boost/math/special_functions/math_fwd.hpp>
  #include <boost/math/special_functions/cbrt.hpp>
  #include <boost/math/special_functions/detail/airy_ai_bi_zero.hpp>

  namespace boost { namespace math {
  namespace detail
  {
    namespace bessel_zero
    {
      template<class T>
      T equation_nist_10_21_19(const T& v, const T& a)
      {
        // Get the initial estimate of the m'th root of Jv or Yv.
        // This subroutine is used for the order m with m > 1.
        // The order m has been used to create the input parameter a.

        // This is Eq. 10.21.19 in the NIST Handbook.
        const T mu                  = (v * v) * 4U;
        const T mu_minus_one        = mu - T(1);
        const T eight_a_inv         = T(1) / (a * 8U);
        const T eight_a_inv_squared = eight_a_inv * eight_a_inv;

        const T term3 = ((mu_minus_one *  4U) *     ((mu *    7U) -     T(31U) )) / 3U;
        const T term5 = ((mu_minus_one * 32U) *   ((((mu *   83U) -    T(982U) ) * mu) +    T(3779U) )) / 15U;
        const T term7 = ((mu_minus_one * 64U) * ((((((mu * 6949U) - T(153855UL)) * mu) + T(1585743UL)) * mu) - T(6277237UL))) / 105U;

        return a + ((((                      - term7
                       * eight_a_inv_squared - term5)
                       * eight_a_inv_squared - term3)
                       * eight_a_inv_squared - mu_minus_one)
                       * eight_a_inv);
      }

      template<typename T>
      class equation_as_9_3_39_and_its_derivative
      {
      public:
        equation_as_9_3_39_and_its_derivative(const T& zt) : zeta(zt) { }

        boost::math::tuple<T, T> operator()(const T& z) const
        {
          BOOST_MATH_STD_USING // ADL of std names, needed for acos, sqrt.

          // Return the function of zeta that is implicitly defined
          // in A&S Eq. 9.3.39 as a function of z. The function is
          // returned along with its derivative with respect to z.

          const T zsq_minus_one_sqrt = sqrt((z * z) - T(1));

          const T the_function(
              zsq_minus_one_sqrt
            - (  acos(T(1) / z) + ((T(2) / 3U) * (zeta * sqrt(zeta)))));

          const T its_derivative(zsq_minus_one_sqrt / z);

          return boost::math::tuple<T, T>(the_function, its_derivative);
        }

      private:
        const equation_as_9_3_39_and_its_derivative& operator=(const equation_as_9_3_39_and_its_derivative&) = delete;
        const T zeta;
      };

      template<class T, class Policy>
      static T equation_as_9_5_26(const T& v, const T& ai_bi_root, const Policy& pol)
      {
        BOOST_MATH_STD_USING // ADL of std names, needed for log, sqrt.

        // Obtain the estimate of the m'th zero of Jv or Yv.
        // The order m has been used to create the input parameter ai_bi_root.
        // Here, v is larger than about 2.2. The estimate is computed
        // from Abramowitz and Stegun Eqs. 9.5.22 and 9.5.26, page 371.
        //
        // The inversion of z as a function of zeta is mentioned in the text
        // following A&S Eq. 9.5.26. Here, we accomplish the inversion by
        // performing a Taylor expansion of Eq. 9.3.39 for large z to order 2
        // and solving the resulting quadratic equation, thereby taking
        // the positive root of the quadratic.
        // In other words: (2/3)(-zeta)^(3/2) approx = z + 1/(2z) - pi/2.
        // This leads to: z^2 - [(2/3)(-zeta)^(3/2) + pi/2]z + 1/2 = 0.
        //
        // With this initial estimate, Newton-Raphson iteration is used
        // to refine the value of the estimate of the root of z
        // as a function of zeta.

        const T v_pow_third(boost::math::cbrt(v, pol));
        const T v_pow_minus_two_thirds(T(1) / (v_pow_third * v_pow_third));

        // Obtain zeta using the order v combined with the m'th root of
        // an airy function, as shown in  A&S Eq. 9.5.22.
        const T zeta = v_pow_minus_two_thirds * (-ai_bi_root);

        const T zeta_sqrt = sqrt(zeta);

        // Set up a quadratic equation based on the Taylor series
        // expansion mentioned above.
        const T b = -((((zeta * zeta_sqrt) * 2U) / 3U) + boost::math::constants::half_pi<T>());

        // Solve the quadratic equation, taking the positive root.
        const T z_estimate = (-b + sqrt((b * b) - T(2))) / 2U;

        // Establish the range, the digits, and the iteration limit
        // for the upcoming root-finding.
        const T range_zmin = (std::max<T>)(z_estimate - T(1), T(1));
        const T range_zmax = z_estimate + T(1);

        const int my_digits10 = static_cast<int>(static_cast<float>(boost::math::tools::digits<T>() * 0.301F));

        // Select the maximum allowed iterations based on the number
        // of decimal digits in the numeric type T, being at least 12.
        const std::uintmax_t iterations_allowed = static_cast<std::uintmax_t>((std::max)(12, my_digits10 * 2));

        std::uintmax_t iterations_used = iterations_allowed;

        // Calculate the root of z as a function of zeta.
        const T z = boost::math::tools::newton_raphson_iterate(
          boost::math::detail::bessel_zero::equation_as_9_3_39_and_its_derivative<T>(zeta),
          z_estimate,
          range_zmin,
          range_zmax,
          (std::min)(boost::math::tools::digits<T>(), boost::math::tools::digits<float>()),
          iterations_used);

        static_cast<void>(iterations_used);

        // Continue with the implementation of A&S Eq. 9.3.39.
        const T zsq_minus_one      = (z * z) - T(1);
        const T zsq_minus_one_sqrt = sqrt(zsq_minus_one);

        // This is A&S Eq. 9.3.42.
        const T b0_term_5_24 = T(5) / ((zsq_minus_one * zsq_minus_one_sqrt) * 24U);
        const T b0_term_1_8  = T(1) / ( zsq_minus_one_sqrt * 8U);
        const T b0_term_5_48 = T(5) / ((zeta * zeta) * 48U);

        const T b0 = -b0_term_5_48 + ((b0_term_5_24 + b0_term_1_8) / zeta_sqrt);

        // This is the second line of A&S Eq. 9.5.26 for f_k with k = 1.
        const T f1 = ((z * zeta_sqrt) * b0) / zsq_minus_one_sqrt;

        // This is A&S Eq. 9.5.22 expanded to k = 1 (i.e., one term in the series).
        return (v * z) + (f1 / v);
      }

      namespace cyl_bessel_j_zero_detail
      {
        template<class T, class Policy>
        T equation_nist_10_21_40_a(const T& v, const Policy& pol)
        {
          const T v_pow_third(boost::math::cbrt(v, pol));
          const T v_pow_minus_two_thirds(T(1) / (v_pow_third * v_pow_third));

          return v * (((((                         + T(0.043)
                          * v_pow_minus_two_thirds - T(0.0908))
                          * v_pow_minus_two_thirds - T(0.00397))
                          * v_pow_minus_two_thirds + T(1.033150))
                          * v_pow_minus_two_thirds + T(1.8557571))
                          * v_pow_minus_two_thirds + T(1));
        }

        template<class T, class Policy>
        class function_object_jv
        {
        public:
          function_object_jv(const T& v,
                             const Policy& pol) : my_v(v),
                                                  my_pol(pol) { }

          T operator()(const T& x) const
          {
            return boost::math::cyl_bessel_j(my_v, x, my_pol);
          }

        private:
          const T my_v;
          const Policy& my_pol;
          const function_object_jv& operator=(const function_object_jv&) = delete;
        };

        template<class T, class Policy>
        class function_object_jv_and_jv_prime
        {
        public:
          function_object_jv_and_jv_prime(const T& v,
                                          const bool order_is_zero,
                                          const Policy& pol) : my_v(v),
                                                               my_order_is_zero(order_is_zero),
                                                               my_pol(pol) { }

          boost::math::tuple<T, T> operator()(const T& x) const
          {
            // Obtain Jv(x) and Jv'(x).
            // Chris's original code called the Bessel function implementation layer direct, 
            // but that circumvented optimizations for integer-orders.  Call the documented
            // top level functions instead, and let them sort out which implementation to use.
            T j_v;
            T j_v_prime;

            if(my_order_is_zero)
            {
              j_v       =  boost::math::cyl_bessel_j(0, x, my_pol);
              j_v_prime = -boost::math::cyl_bessel_j(1, x, my_pol);
            }
            else
            {
                      j_v       = boost::math::cyl_bessel_j(  my_v,      x, my_pol);
              const T j_v_m1     (boost::math::cyl_bessel_j(T(my_v - 1), x, my_pol));
                      j_v_prime = j_v_m1 - ((my_v * j_v) / x);
            }

            // Return a tuple containing both Jv(x) and Jv'(x).
            return boost::math::make_tuple(j_v, j_v_prime);
          }

        private:
          const T my_v;
          const bool my_order_is_zero;
          const Policy& my_pol;
          const function_object_jv_and_jv_prime& operator=(const function_object_jv_and_jv_prime&) = delete;
        };

        template<class T> bool my_bisection_unreachable_tolerance(const T&, const T&) { return false; }

        template<class T, class Policy>
        T initial_guess(const T& v, const int m, const Policy& pol)
        {
          BOOST_MATH_STD_USING // ADL of std names, needed for floor.

          // Compute an estimate of the m'th root of cyl_bessel_j.

          T guess;

          // There is special handling for negative order.
          if(v < 0)
          {
            if((m == 1) && (v > -0.5F))
            {
              // For small, negative v, use the results of empirical curve fitting.
              // Mathematica(R) session for the coefficients:
              //  Table[{n, BesselJZero[n, 1]}, {n, -(1/2), 0, 1/10}]
              //  N[%, 20]
              //  Fit[%, {n^0, n^1, n^2, n^3, n^4, n^5, n^6}, n]
              guess = (((((    - T(0.2321156900729)
                           * v - T(0.1493247777488))
                           * v - T(0.15205419167239))
                           * v + T(0.07814930561249))
                           * v - T(0.17757573537688))
                           * v + T(1.542805677045663))
                           * v + T(2.40482555769577277);

              return guess;
            }

            // Create the positive order and extract its positive floor integer part.
            const T vv(-v);
            const T vv_floor(floor(vv));

            // The to-be-found root is bracketed by the roots of the
            // Bessel function whose reflected, positive integer order
            // is less than, but nearest to vv.

            T root_hi = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::initial_guess(vv_floor, m, pol);
            T root_lo;

            if(m == 1)
            {
              // The estimate of the first root for negative order is found using
              // an adaptive range-searching algorithm.
              root_lo = T(root_hi - 0.1F);

              const bool hi_end_of_bracket_is_negative = (boost::math::cyl_bessel_j(v, root_hi, pol) < 0);

              while((root_lo > boost::math::tools::epsilon<T>()))
              {
                const bool lo_end_of_bracket_is_negative = (boost::math::cyl_bessel_j(v, root_lo, pol) < 0);

                if(hi_end_of_bracket_is_negative != lo_end_of_bracket_is_negative)
                {
                  break;
                }

                root_hi = root_lo;

                // Decrease the lower end of the bracket using an adaptive algorithm.
                if(root_lo > 0.5F)
                {
                  root_lo -= 0.5F;
                }
                else
                {
                  root_lo *= 0.75F;
                }
              }
            }
            else
            {
              root_lo = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::initial_guess(vv_floor, m - 1, pol);
            }

            // Perform several steps of bisection iteration to refine the guess.
            std::uintmax_t number_of_iterations(12U);

            // Do the bisection iteration.
            const boost::math::tuple<T, T> guess_pair =
               boost::math::tools::bisect(
                  boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::function_object_jv<T, Policy>(v, pol),
                  root_lo,
                  root_hi,
                  boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::my_bisection_unreachable_tolerance<T>,
                  number_of_iterations);

            return (boost::math::get<0>(guess_pair) + boost::math::get<1>(guess_pair)) / 2U;
          }

          if(m == 1U)
          {
            // Get the initial estimate of the first root.

            if(v < 2.2F)
            {
              // For small v, use the results of empirical curve fitting.
              // Mathematica(R) session for the coefficients:
              //  Table[{n, BesselJZero[n, 1]}, {n, 0, 22/10, 1/10}]
              //  N[%, 20]
              //  Fit[%, {n^0, n^1, n^2, n^3, n^4, n^5, n^6}, n]
              guess = (((((    - T(0.0008342379046010)
                           * v + T(0.007590035637410))
                           * v - T(0.030640914772013))
                           * v + T(0.078232088020106))
                           * v - T(0.169668712590620))
                           * v + T(1.542187960073750))
                           * v + T(2.4048359915254634);
            }
            else
            {
              // For larger v, use the first line of Eqs. 10.21.40 in the NIST Handbook.
              guess = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::equation_nist_10_21_40_a(v, pol);
            }
          }
          else
          {
            if(v < 2.2F)
            {
              // Use Eq. 10.21.19 in the NIST Handbook.
              const T a(((v + T(m * 2U)) - T(0.5)) * boost::math::constants::half_pi<T>());

              guess = boost::math::detail::bessel_zero::equation_nist_10_21_19(v, a);
            }
            else
            {
              // Get an estimate of the m'th root of airy_ai.
              const T airy_ai_root(boost::math::detail::airy_zero::airy_ai_zero_detail::initial_guess<T>(m, pol));

              // Use Eq. 9.5.26 in the A&S Handbook.
              guess = boost::math::detail::bessel_zero::equation_as_9_5_26(v, airy_ai_root, pol);
            }
          }

          return guess;
        }
      } // namespace cyl_bessel_j_zero_detail

      namespace cyl_neumann_zero_detail
      {
        template<class T, class Policy>
        T equation_nist_10_21_40_b(const T& v, const Policy& pol)
        {
          const T v_pow_third(boost::math::cbrt(v, pol));
          const T v_pow_minus_two_thirds(T(1) / (v_pow_third * v_pow_third));

          return v * (((((                         - T(0.001)
                          * v_pow_minus_two_thirds - T(0.0060))
                          * v_pow_minus_two_thirds + T(0.01198))
                          * v_pow_minus_two_thirds + T(0.260351))
                          * v_pow_minus_two_thirds + T(0.9315768))
                          * v_pow_minus_two_thirds + T(1));
        }

        template<class T, class Policy>
        class function_object_yv
        {
        public:
          function_object_yv(const T& v,
                             const Policy& pol) : my_v(v),
                                                  my_pol(pol) { }

          T operator()(const T& x) const
          {
            return boost::math::cyl_neumann(my_v, x, my_pol);
          }

        private:
          const T my_v;
          const Policy& my_pol;
          const function_object_yv& operator=(const function_object_yv&) = delete;
        };

        template<class T, class Policy>
        class function_object_yv_and_yv_prime
        {
        public:
          function_object_yv_and_yv_prime(const T& v,
                                          const Policy& pol) : my_v(v),
                                                               my_pol(pol) { }

          boost::math::tuple<T, T> operator()(const T& x) const
          {
            const T half_epsilon(boost::math::tools::epsilon<T>() / 2U);

            const bool order_is_zero = ((my_v > -half_epsilon) && (my_v < +half_epsilon));

            // Obtain Yv(x) and Yv'(x).
            // Chris's original code called the Bessel function implementation layer direct, 
            // but that circumvented optimizations for integer-orders.  Call the documented
            // top level functions instead, and let them sort out which implementation to use.
            T y_v;
            T y_v_prime;

            if(order_is_zero)
            {
              y_v       =  boost::math::cyl_neumann(0, x, my_pol);
              y_v_prime = -boost::math::cyl_neumann(1, x, my_pol);
            }
            else
            {
                      y_v       = boost::math::cyl_neumann(  my_v,      x, my_pol);
              const T y_v_m1     (boost::math::cyl_neumann(T(my_v - 1), x, my_pol));
                      y_v_prime = y_v_m1 - ((my_v * y_v) / x);
            }

            // Return a tuple containing both Yv(x) and Yv'(x).
            return boost::math::make_tuple(y_v, y_v_prime);
          }

        private:
          const T my_v;
          const Policy& my_pol;
          const function_object_yv_and_yv_prime& operator=(const function_object_yv_and_yv_prime&) = delete;
        };

        template<class T> bool my_bisection_unreachable_tolerance(const T&, const T&) { return false; }

        template<class T, class Policy>
        T initial_guess(const T& v, const int m, const Policy& pol)
        {
          BOOST_MATH_STD_USING // ADL of std names, needed for floor.

          // Compute an estimate of the m'th root of cyl_neumann.

          T guess;

          // There is special handling for negative order.
          if(v < 0)
          {
            // Create the positive order and extract its positive floor and ceiling integer parts.
            const T vv(-v);
            const T vv_floor(floor(vv));

            // The to-be-found root is bracketed by the roots of the
            // Bessel function whose reflected, positive integer order
            // is less than, but nearest to vv.

            // The special case of negative, half-integer order uses
            // the relation between Yv and spherical Bessel functions
            // in order to obtain the bracket for the root.
            // In these special cases, cyl_neumann(-n/2, x) = sph_bessel_j(+n/2, x)
            // for v = -n/2.

            T root_hi;
            T root_lo;

            if(m == 1)
            {
              // The estimate of the first root for negative order is found using
              // an adaptive range-searching algorithm.
              // Take special precautions for the discontinuity at negative,
              // half-integer orders and use different brackets above and below these.
              if(T(vv - vv_floor) < 0.5F)
              {
                root_hi = boost::math::detail::bessel_zero::cyl_neumann_zero_detail::initial_guess(vv_floor, m, pol);
              }
              else
              {
                root_hi = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::initial_guess(T(vv_floor + 0.5F), m, pol);
              }

              root_lo = T(root_hi - 0.1F);

              const bool hi_end_of_bracket_is_negative = (boost::math::cyl_neumann(v, root_hi, pol) < 0);

              while((root_lo > boost::math::tools::epsilon<T>()))
              {
                const bool lo_end_of_bracket_is_negative = (boost::math::cyl_neumann(v, root_lo, pol) < 0);

                if(hi_end_of_bracket_is_negative != lo_end_of_bracket_is_negative)
                {
                  break;
                }

                root_hi = root_lo;

                // Decrease the lower end of the bracket using an adaptive algorithm.
                if(root_lo > 0.5F)
                {
                  root_lo -= 0.5F;
                }
                else
                {
                  root_lo *= 0.75F;
                }
              }
            }
            else
            {
              if(T(vv - vv_floor) < 0.5F)
              {
                root_lo  = boost::math::detail::bessel_zero::cyl_neumann_zero_detail::initial_guess(vv_floor, m - 1, pol);
                root_hi = boost::math::detail::bessel_zero::cyl_neumann_zero_detail::initial_guess(vv_floor, m, pol);
                root_lo += 0.01F;
                root_hi += 0.01F;
              }
              else
              {
                root_lo = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::initial_guess(T(vv_floor + 0.5F), m - 1, pol);
                root_hi = boost::math::detail::bessel_zero::cyl_bessel_j_zero_detail::initial_guess(T(vv_floor + 0.5F), m, pol);
                root_lo += 0.01F;
                root_hi += 0.01F;
              }
            }

            // Perform several steps of bisection iteration to refine the guess.
            std::uintmax_t number_of_iterations(12U);

            // Do the bisection iteration.
            const boost::math::tuple<T, T> guess_pair =
               boost::math::tools::bisect(
                  boost::math::detail::bessel_zero::cyl_neumann_zero_detail::function_object_yv<T, Policy>(v, pol),
                  root_lo,
                  root_hi,
                  boost::math::detail::bessel_zero::cyl_neumann_zero_detail::my_bisection_unreachable_tolerance<T>,
                  number_of_iterations);

            return (boost::math::get<0>(guess_pair) + boost::math::get<1>(guess_pair)) / 2U;
          }

          if(m == 1U)
          {
            // Get the initial estimate of the first root.

            if(v < 2.2F)
            {
              // For small v, use the results of empirical curve fitting.
              // Mathematica(R) session for the coefficients:
              //  Table[{n, BesselYZero[n, 1]}, {n, 0, 22/10, 1/10}]
              //  N[%, 20]
              //  Fit[%, {n^0, n^1, n^2, n^3, n^4, n^5, n^6}, n]
              guess = (((((    - T(0.0025095909235652)
                           * v + T(0.021291887049053))
                           * v - T(0.076487785486526))
                           * v + T(0.159110268115362))
                           * v - T(0.241681668765196))
                           * v + T(1.4437846310885244))
                           * v + T(0.89362115190200490);
            }
            else
            {
              // For larger v, use the second line of Eqs. 10.21.40 in the NIST Handbook.
              guess = boost::math::detail::bessel_zero::cyl_neumann_zero_detail::equation_nist_10_21_40_b(v, pol);
            }
          }
          else
          {
            if(v < 2.2F)
            {
              // Use Eq. 10.21.19 in the NIST Handbook.
              const T a(((v + T(m * 2U)) - T(1.5)) * boost::math::constants::half_pi<T>());

              guess = boost::math::detail::bessel_zero::equation_nist_10_21_19(v, a);
            }
            else
            {
              // Get an estimate of the m'th root of airy_bi.
              const T airy_bi_root(boost::math::detail::airy_zero::airy_bi_zero_detail::initial_guess<T>(m, pol));

              // Use Eq. 9.5.26 in the A&S Handbook.
              guess = boost::math::detail::bessel_zero::equation_as_9_5_26(v, airy_bi_root, pol);
            }
          }

          return guess;
        }
      } // namespace cyl_neumann_zero_detail
    } // namespace bessel_zero
  } } } // namespace boost::math::detail

#endif // BOOST_MATH_BESSEL_JY_ZERO_2013_01_18_HPP_

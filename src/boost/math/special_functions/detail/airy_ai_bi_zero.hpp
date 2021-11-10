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
// of the Airy functions airy_ai and airy_bi on the negative real axis.
//
#ifndef BOOST_MATH_AIRY_AI_BI_ZERO_2013_01_20_HPP_
  #define BOOST_MATH_AIRY_AI_BI_ZERO_2013_01_20_HPP_

  #include <boost/math/constants/constants.hpp>
  #include <boost/math/special_functions/cbrt.hpp>

  namespace boost { namespace math {
  namespace detail
  {
    // Forward declarations of the needed Airy function implementations.
    template <class T, class Policy>
    T airy_ai_imp(T x, const Policy& pol);
    template <class T, class Policy>
    T airy_bi_imp(T x, const Policy& pol);
    template <class T, class Policy>
    T airy_ai_prime_imp(T x, const Policy& pol);
    template <class T, class Policy>
    T airy_bi_prime_imp(T x, const Policy& pol);

    namespace airy_zero
    {
      template<class T, class Policy>
      T equation_as_10_4_105(const T& z, const Policy& pol)
      {
        const T one_over_z        (T(1) / z);
        const T one_over_z_squared(one_over_z * one_over_z);

        const T z_pow_third     (boost::math::cbrt(z, pol));
        const T z_pow_two_thirds(z_pow_third * z_pow_third);

        // Implement the top line of Eq. 10.4.105.
        const T fz(z_pow_two_thirds * (((((                     + (T(162375596875.0) / 334430208UL)
                                           * one_over_z_squared - (   T(108056875.0) /   6967296UL))
                                           * one_over_z_squared + (       T(77125UL) /     82944UL))
                                           * one_over_z_squared - (           T(5U)  /        36U))
                                           * one_over_z_squared + (           T(5U)  /        48U))
                                           * one_over_z_squared + (1)));

        return fz;
      }

      namespace airy_ai_zero_detail
      {
        template<class T, class Policy>
        T initial_guess(const int m, const Policy& pol)
        {
          T guess;

          switch(m)
          {
            case  0: { guess = T(0);                       break; }
            case  1: { guess = T(-2.33810741045976703849); break; }
            case  2: { guess = T(-4.08794944413097061664); break; }
            case  3: { guess = T(-5.52055982809555105913); break; }
            case  4: { guess = T(-6.78670809007175899878); break; }
            case  5: { guess = T(-7.94413358712085312314); break; }
            case  6: { guess = T(-9.02265085334098038016); break; }
            case  7: { guess = T(-10.0401743415580859306); break; }
            case  8: { guess = T(-11.0085243037332628932); break; }
            case  9: { guess = T(-11.9360155632362625170); break; }
            case 10:{ guess = T(-12.8287767528657572004); break; }
            default:
            {
              const T t(((boost::math::constants::pi<T>() * 3) * ((T(m) * 4) - 1)) / 8);
              guess = -boost::math::detail::airy_zero::equation_as_10_4_105(t, pol);
              break;
            }
          }

          return guess;
        }

        template<class T, class Policy>
        class function_object_ai_and_ai_prime
        {
        public:
          function_object_ai_and_ai_prime(const Policy& pol) : my_pol(pol) { }

          boost::math::tuple<T, T> operator()(const T& x) const
          {
            // Return a tuple containing both Ai(x) and Ai'(x).
            return boost::math::make_tuple(
              boost::math::detail::airy_ai_imp      (x, my_pol),
              boost::math::detail::airy_ai_prime_imp(x, my_pol));
          }

        private:
          const Policy& my_pol;
          const function_object_ai_and_ai_prime& operator=(const function_object_ai_and_ai_prime&) = delete;
        };
      } // namespace airy_ai_zero_detail

      namespace airy_bi_zero_detail
      {
        template<class T, class Policy>
        T initial_guess(const int m, const Policy& pol)
        {
          T guess;

          switch(m)
          {
            case  0: { guess = T(0);                       break; }
            case  1: { guess = T(-1.17371322270912792492); break; }
            case  2: { guess = T(-3.27109330283635271568); break; }
            case  3: { guess = T(-4.83073784166201593267); break; }
            case  4: { guess = T(-6.16985212831025125983); break; }
            case  5: { guess = T(-7.37676207936776371360); break; }
            case  6: { guess = T(-8.49194884650938801345); break; }
            case  7: { guess = T(-9.53819437934623888663); break; }
            case  8: { guess = T(-10.5299135067053579244); break; }
            case  9: { guess = T(-11.4769535512787794379); break; }
            case 10: { guess = T(-12.3864171385827387456); break; }
            default:
            {
              const T t(((boost::math::constants::pi<T>() * 3) * ((T(m) * 4) - 3)) / 8);
              guess = -boost::math::detail::airy_zero::equation_as_10_4_105(t, pol);
              break;
            }
          }

          return guess;
        }

        template<class T, class Policy>
        class function_object_bi_and_bi_prime
        {
        public:
          function_object_bi_and_bi_prime(const Policy& pol) : my_pol(pol) { }

          boost::math::tuple<T, T> operator()(const T& x) const
          {
            // Return a tuple containing both Bi(x) and Bi'(x).
            return boost::math::make_tuple(
              boost::math::detail::airy_bi_imp      (x, my_pol),
              boost::math::detail::airy_bi_prime_imp(x, my_pol));
          }

        private:
          const Policy& my_pol;
          const function_object_bi_and_bi_prime& operator=(const function_object_bi_and_bi_prime&) = delete;
        };
      } // namespace airy_bi_zero_detail
    } // namespace airy_zero
  } // namespace detail
  } // namespace math
  } // namespaces boost

#endif // BOOST_MATH_AIRY_AI_BI_ZERO_2013_01_20_HPP_

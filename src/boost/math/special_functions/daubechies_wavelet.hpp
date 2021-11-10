/*
 * Copyright Nick Thompson, 2020
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_MATH_SPECIAL_DAUBECHIES_WAVELET_HPP
#define BOOST_MATH_SPECIAL_DAUBECHIES_WAVELET_HPP
#include <vector>
#include <array>
#include <cmath>
#include <thread>
#include <future>
#include <iostream>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/detail/daubechies_scaling_integer_grid.hpp>
#include <boost/math/special_functions/daubechies_scaling.hpp>
#include <boost/math/filters/daubechies.hpp>
#include <boost/math/interpolators/detail/cubic_hermite_detail.hpp>
#include <boost/math/interpolators/detail/quintic_hermite_detail.hpp>
#include <boost/math/interpolators/detail/septic_hermite_detail.hpp>

namespace boost::math {

   template<class Real, int p, int order>
   std::vector<Real> daubechies_wavelet_dyadic_grid(int64_t j_max)
   {
      if (j_max == 0)
      {
         throw std::domain_error("The wavelet dyadic grid is refined from the scaling integer grid, so its minimum amount of data is half integer widths.");
      }
      auto phijk = daubechies_scaling_dyadic_grid<Real, p, order>(j_max - 1);
      //psi_j[l] = psi(-p+1 + l/2^j) = \sum_{k=0}^{2p-1} (-1)^k c_k \phi(1-2p+k + l/2^{j-1})
      //For derivatives just map c_k -> 2^order c_k.
      auto d = boost::math::filters::daubechies_scaling_filter<Real, p>();
      Real scale = boost::math::constants::root_two<Real>() * (1 << order);
      for (size_t i = 0; i < d.size(); ++i)
      {
         d[i] *= scale;
         if (!(i & 1))
         {
            d[i] = -d[i];
         }
      }

      std::vector<Real> v(2 * p + (2 * p - 1) * ((int64_t(1) << j_max) - 1), std::numeric_limits<Real>::quiet_NaN());
      v[0] = 0;
      v[v.size() - 1] = 0;

      for (int64_t l = 1; l < static_cast<int64_t>(v.size() - 1); ++l)
      {
         Real term = 0;
         for (int64_t k = 0; k < static_cast<int64_t>(d.size()); ++k)
         {
            int64_t idx = (int64_t(1) << (j_max - 1)) * (1 - 2 * p + k) + l;
            if (idx < 0 || idx >= static_cast<int64_t>(phijk.size()))
            {
               continue;
            }
            term += d[k] * phijk[idx];
         }
         v[l] = term;
      }

      return v;
   }


   template<class Real, int p>
   class daubechies_wavelet {
      //
      // Some type manipulation so we know the type of the interpolator, and the vector type it requires:
      //
      typedef std::vector < std::array < Real, p < 6 ? 2 : p < 10 ? 3 : 4>> vector_type;
      //
      // List our interpolators:
      //
      typedef std::tuple<
         detail::null_interpolator, detail::matched_holder_aos<vector_type>, detail::linear_interpolation_aos<vector_type>,
         interpolators::detail::cardinal_cubic_hermite_detail_aos<vector_type>, interpolators::detail::cardinal_quintic_hermite_detail_aos<vector_type>,
         interpolators::detail::cardinal_septic_hermite_detail_aos<vector_type> > interpolator_list;
      //
      // Select the one we need:
      //
      typedef std::tuple_element_t<
         p == 1 ? 0 :
         p == 2 ? 1 :
         p == 3 ? 2 :
         p <= 5 ? 3 :
         p <= 9 ? 4 : 5, interpolator_list> interpolator_type;
   public:
      daubechies_wavelet(int grid_refinements = -1)
      {
         static_assert(p < 20, "Daubechies wavelets are only implemented for p < 20.");
         static_assert(p > 0, "Daubechies wavelets must have at least 1 vanishing moment.");
         if (grid_refinements == 0)
         {
            throw std::domain_error("The wavelet requires at least 1 grid refinement.");
         }
         if constexpr (p == 1)
         {
            return;
         }
         else
         {
            if (grid_refinements < 0)
            {
               if (std::is_same_v<Real, float>)
               {
                  if (grid_refinements == -2)
                  {
                     // Control absolute error:
                     //                          p= 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
                     std::array<int, 20> r{ -1, -1, 18, 19, 16, 11,  8,  7,  7,  7,  5,  5,  4,  4,  4,  4,  3,  3,  3,  3 };
                     grid_refinements = r[p];
                  }
                  else
                  {
                     // Control relative error:
                     //                          p= 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
                     std::array<int, 20> r{ -1, -1, 21, 21, 21, 17, 16, 15, 14, 13, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11 };
                     grid_refinements = r[p];
                  }
               }
               else if (std::is_same_v<Real, double>)
               {
                  //                          p= 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
                  std::array<int, 20> r{ -1, -1, 21, 21, 21, 21, 21, 21, 21, 21, 20, 20, 19, 18, 18, 18, 18, 18, 18, 18 };
                  grid_refinements = r[p];
               }
               else
               {
                  grid_refinements = 21;
               }
            }

            // Compute the refined grid:
            // In fact for float precision I know the grid must be computed in double precision and then cast back down, or else parts of the support are systematically inaccurate.
            std::future<std::vector<Real>> t0 = std::async(std::launch::async, [&grid_refinements]() {
               // Computing in higher precision and downcasting is essential for 1ULP evaluation in float precision:
               auto v = daubechies_wavelet_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 0>(grid_refinements);
               return detail::daubechies_eval_type<Real>::vector_cast(v);
               });
            // Compute the derivative of the refined grid:
            std::future<std::vector<Real>> t1 = std::async(std::launch::async, [&grid_refinements]() {
               auto v = daubechies_wavelet_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 1>(grid_refinements);
               return detail::daubechies_eval_type<Real>::vector_cast(v);
               });

            // if necessary, compute the second and third derivative:
            std::vector<Real> d2ydx2;
            std::vector<Real> d3ydx3;
            if constexpr (p >= 6) {
               std::future<std::vector<Real>> t3 = std::async(std::launch::async, [&grid_refinements]() {
                  auto v = daubechies_wavelet_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 2>(grid_refinements);
                  return detail::daubechies_eval_type<Real>::vector_cast(v);
                  });

               if constexpr (p >= 10) {
                  std::future<std::vector<Real>> t4 = std::async(std::launch::async, [&grid_refinements]() {
                     auto v = daubechies_wavelet_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 3>(grid_refinements);
                     return detail::daubechies_eval_type<Real>::vector_cast(v);
                     });
                  d3ydx3 = t4.get();
               }
               d2ydx2 = t3.get();
            }


            auto y = t0.get();
            auto dydx = t1.get();

            if constexpr (p >= 2)
            {
               vector_type data(y.size());
               for (size_t i = 0; i < y.size(); ++i)
               {
                  data[i][0] = y[i];
                  data[i][1] = dydx[i];
                  if constexpr (p >= 6)
                     data[i][2] = d2ydx2[i];
                  if constexpr (p >= 10)
                     data[i][3] = d3ydx3[i];
               }
               if constexpr (p <= 3)
                  m_interpolator = std::make_shared<interpolator_type>(std::move(data), grid_refinements, Real(-p + 1));
               else
                  m_interpolator = std::make_shared<interpolator_type>(std::move(data), Real(-p + 1), Real(1) / (1 << grid_refinements));
            }
            else
               m_interpolator = std::make_shared<detail::null_interpolator>();
         }
      }


      inline Real operator()(Real x) const
      {
         if (x <= -p + 1 || x >= p)
         {
            return 0;
         }
         if constexpr (p == 1)
         {
            if (x < Real(1) / Real(2))
            {
               return 1;
            }
            else if (x == Real(1) / Real(2))
            {
               return 0;
            }
            return -1;
         }
         return (*m_interpolator)(x);
      }

      inline Real prime(Real x) const
      {
         static_assert(p > 2, "The 3-vanishing moment Daubechies wavelet is the first which is continuously differentiable.");
         if (x <= -p + 1 || x >= p)
         {
            return 0;
         }
         return m_interpolator->prime(x);
      }

      inline Real double_prime(Real x) const
      {
         static_assert(p >= 6, "Second derivatives of Daubechies wavelets require at least 6 vanishing moments.");
         if (x <= -p + 1 || x >= p)
         {
            return Real(0);
         }
         return m_interpolator->double_prime(x);
      }

      std::pair<Real, Real> support() const
      {
         return { Real(-p + 1), Real(p) };
      }

      int64_t bytes() const
      {
         return m_interpolator->bytes() + sizeof(*this);
      }

   private:
      std::shared_ptr<interpolator_type> m_interpolator;
   };

}
#endif

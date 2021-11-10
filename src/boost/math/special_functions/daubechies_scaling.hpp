/*
 * Copyright Nick Thompson, John Maddock 2020
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_MATH_SPECIAL_DAUBECHIES_SCALING_HPP
#define BOOST_MATH_SPECIAL_DAUBECHIES_SCALING_HPP
#include <vector>
#include <array>
#include <cmath>
#include <thread>
#include <future>
#include <iostream>
#include <boost/math/special_functions/detail/daubechies_scaling_integer_grid.hpp>
#include <boost/math/filters/daubechies.hpp>
#include <boost/math/interpolators/detail/cubic_hermite_detail.hpp>
#include <boost/math/interpolators/detail/quintic_hermite_detail.hpp>
#include <boost/math/interpolators/detail/septic_hermite_detail.hpp>

namespace boost::math {

template<class Real, int p, int order>
std::vector<Real> daubechies_scaling_dyadic_grid(int64_t j_max)
{
    using std::isnan;
    using std::sqrt;
    auto c = boost::math::filters::daubechies_scaling_filter<Real, p>();
    Real scale = sqrt(static_cast<Real>(2))*(1 << order);
    for (auto & x : c)
    {
        x *= scale;
    }

    auto phik = detail::daubechies_scaling_integer_grid<Real, p, order>();

    // Maximum sensible j for 32 bit floats is j_max = 22:
    if (std::is_same_v<Real, float>)
    {
        if (j_max > 23)
        {
            throw std::logic_error("Requested dyadic grid more dense than number of representables on the interval.");
        }
    }
    std::vector<Real> v(2*p + (2*p-1)*((1<<j_max) -1), std::numeric_limits<Real>::quiet_NaN());
    v[0] = 0;
    v[v.size()-1] = 0;
    for (int64_t i = 0; i < (int64_t) phik.size(); ++i) {
        v[i*(1uLL<<j_max)] = phik[i];
    }

    for (int64_t j = 1; j <= j_max; ++j)
    {
        int64_t k_max = v.size()/(int64_t(1) << (j_max-j));
        for (int64_t k = 1; k < k_max;  k += 2)
        {
            // Where this value will go:
            int64_t delivery_idx = k*(1uLL << (j_max-j));
            // This is a nice check, but we've tested this exhaustively, and it's an expensive check:
            //if (delivery_idx >= (int64_t) v.size()) {
            //    std::cerr << "Delivery index out of range!\n";
            //    continue;
            //}
            Real term = 0;
            for (int64_t l = 0; l < (int64_t) c.size(); ++l)
            {
                int64_t idx = k*(int64_t(1) << (j_max - j + 1)) - l*(int64_t(1) << j_max);
                if (idx < 0)
                {
                    break;
                }
                if (idx < (int64_t) v.size())
                {
                    term += c[l]*v[idx];
                }
            }
            // Again, another nice check:
            //if (!isnan(v[delivery_idx])) {
            //    std::cerr << "Delivery index already populated!, = " << v[delivery_idx] << "\n";
            //    std::cerr << "would overwrite with " << term << "\n";
            //}
            v[delivery_idx] = term;
        }
    }
    return v;
}

namespace detail {

template<class RandomAccessContainer>
class matched_holder {
public:
    using Real = typename RandomAccessContainer::value_type;

    matched_holder(RandomAccessContainer && y, RandomAccessContainer && dydx, int grid_refinements, Real x0) : x0_{x0}, y_{std::move(y)}, dy_{std::move(dydx)}
    {
        inv_h_ = (1 << grid_refinements);
        Real h = 1/inv_h_;
        for (auto & dy : dy_)
        {
            dy *= h;
        }
    }

    inline Real operator()(Real x) const
    {
        using std::floor;
        using std::sqrt;
        // This is the exact Holder exponent, but it's pessimistic almost everywhere!
        // It's only exactly right at dyadic rationals.
        //Real const alpha = 2 - log(1+sqrt(Real(3)))/log(Real(2));
        // We're gonna use alpha = 1/2, rather than 0.5500...
        Real s = (x-x0_)*inv_h_;
        Real ii = floor(s);
        auto i = static_cast<decltype(y_.size())>(ii);
        Real t = s - ii;
        Real dphi = dy_[i+1];
        Real diff = y_[i+1] - y_[i];
        return y_[i] + (2*dphi - diff)*t + 2*sqrt(t)*(diff-dphi);
    }

    int64_t bytes() const
    {
        return 2*y_.size()*sizeof(Real) + sizeof(this);
    }

private:
    Real x0_;
    Real inv_h_;
    RandomAccessContainer y_;
    RandomAccessContainer dy_;
};

template<class RandomAccessContainer>
class matched_holder_aos {
public:
    using Point = typename RandomAccessContainer::value_type;
    using Real = typename Point::value_type;

    matched_holder_aos(RandomAccessContainer && data, int grid_refinements, Real x0) : x0_{x0}, data_{std::move(data)}
    {
        inv_h_ = Real(1uLL << grid_refinements);
        Real h = 1/inv_h_;
        for (auto & datum : data_)
        {
            datum[1] *= h;
        }
    }

    inline Real operator()(Real x) const
    {
        using std::floor;
        using std::sqrt;
        Real s = (x-x0_)*inv_h_;
        Real ii = floor(s);
        auto i = static_cast<decltype(data_.size())>(ii);
        Real t = s - ii;
        Real y0 = data_[i][0];
        Real y1 = data_[i+1][0];
        Real dphi = data_[i+1][1];
        Real diff = y1 - y0;
        return y0 + (2*dphi - diff)*t + 2*sqrt(t)*(diff-dphi);
    }

    int64_t bytes() const
    {
        return data_.size()*data_[0].size()*sizeof(Real) + sizeof(this);
    }

private:
    Real x0_;
    Real inv_h_;
    RandomAccessContainer data_;
};


template<class RandomAccessContainer>
class linear_interpolation {
public:
    using Real = typename RandomAccessContainer::value_type;

    linear_interpolation(RandomAccessContainer && y, RandomAccessContainer && dydx, int grid_refinements) : y_{std::move(y)}, dydx_{std::move(dydx)}
    {
        s_ = (1 << grid_refinements);
    }

    inline Real operator()(Real x) const
    {
        using std::floor;
        Real y = x*s_;
        Real k = floor(y);

        int64_t kk = static_cast<int64_t>(k);
        Real t = y - k;
        return (1-t)*y_[kk] + t*y_[kk+1];
    }

    inline Real prime(Real x) const
    {
        using std::floor;
        Real y = x*s_;
        Real k = floor(y);

        int64_t kk = static_cast<int64_t>(k);
        Real t = y - k;
        return (1-t)*dydx_[kk] + t*dydx_[kk+1];
    }

    int64_t bytes() const
    {
        return (1 + y_.size() + dydx_.size())*sizeof(Real) + sizeof(y_) + sizeof(dydx_);
    }

private:
    Real s_;
    RandomAccessContainer y_;
    RandomAccessContainer dydx_;
};

template<class RandomAccessContainer>
class linear_interpolation_aos {
public:
    using Point = typename RandomAccessContainer::value_type;
    using Real = typename Point::value_type;

    linear_interpolation_aos(RandomAccessContainer && data, int grid_refinements, Real x0) : x0_{x0}, data_{std::move(data)}
    {
        s_ = Real(1uLL << grid_refinements);
    }

    inline Real operator()(Real x) const
    {
        using std::floor;
        Real y = (x-x0_)*s_;
        Real k = floor(y);

        int64_t kk = static_cast<int64_t>(k);
        Real t = y - k;
        return (t != 0) ? (1-t)*data_[kk][0] + t*data_[kk+1][0] : data_[kk][0];
    }

    inline Real prime(Real x) const
    {
        using std::floor;
        Real y = (x-x0_)*s_;
        Real k = floor(y);

        int64_t kk = static_cast<int64_t>(k);
        Real t = y - k;
        return t != 0 ? (1-t)*data_[kk][1] + t*data_[kk+1][1] : data_[kk][1];
    }

    int64_t bytes() const
    {
        return sizeof(this) + data_.size()*data_[0].size()*sizeof(Real);
    }

private:
    Real x0_;
    Real s_;
    RandomAccessContainer data_;
};


template <class T>
struct daubechies_eval_type
{
   typedef T type;

   static const std::vector<T>& vector_cast(const std::vector<T>& v) { return v; }

};
template <>
struct daubechies_eval_type<float>
{
   typedef double type;

   inline static std::vector<float> vector_cast(const std::vector<double>& v)
   {
      std::vector<float> result(v.size());
      for (unsigned i = 0; i < v.size(); ++i)
         result[i] = static_cast<float>(v[i]);
      return result;
   }
};
template <>
struct daubechies_eval_type<double>
{
   typedef long double type;

   inline static std::vector<double> vector_cast(const std::vector<long double>& v)
   {
      std::vector<double> result(v.size());
      for (unsigned i = 0; i < v.size(); ++i)
         result[i] = static_cast<double>(v[i]);
      return result;
   }
};

struct null_interpolator
{
   template <class T>
   T operator()(const T&)
   {
      return 1;
   }
};

} // namespace detail

template<class Real, int p>
class daubechies_scaling {
   //
   // Some type manipulation so we know the type of the interpolator, and the vector type it requires:
   //
   typedef std::vector<std::array<Real, p < 6 ? 2 : p < 10 ? 3 : 4>> vector_type;
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
   daubechies_scaling(int grid_refinements = -1)
   {
      static_assert(p < 20, "Daubechies scaling functions are only implemented for p < 20.");
      static_assert(p > 0, "Daubechies scaling functions must have at least 1 vanishing moment.");
      if constexpr (p == 1)
      {
         return;
      }
      else {
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
               std::array<int, 20> r{ -1, -1, 21, 21, 21, 21, 21, 21, 21, 21, 20, 20, 19, 19, 18, 18, 18, 18, 18, 18 };
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
            auto v = daubechies_scaling_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 0>(grid_refinements);
            return detail::daubechies_eval_type<Real>::vector_cast(v);
            });
         // Compute the derivative of the refined grid:
         std::future<std::vector<Real>> t1 = std::async(std::launch::async, [&grid_refinements]() {
            auto v = daubechies_scaling_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 1>(grid_refinements);
            return detail::daubechies_eval_type<Real>::vector_cast(v);
            });

         // if necessary, compute the second and third derivative:
         std::vector<Real> d2ydx2;
         std::vector<Real> d3ydx3;
         if constexpr (p >= 6) {
            std::future<std::vector<Real>> t3 = std::async(std::launch::async, [&grid_refinements]() {
               auto v = daubechies_scaling_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 2>(grid_refinements);
               return detail::daubechies_eval_type<Real>::vector_cast(v);
               });

            if constexpr (p >= 10) {
               std::future<std::vector<Real>> t4 = std::async(std::launch::async, [&grid_refinements]() {
                  auto v = daubechies_scaling_dyadic_grid<typename detail::daubechies_eval_type<Real>::type, p, 3>(grid_refinements);
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
               m_interpolator = std::make_shared<interpolator_type>(std::move(data), grid_refinements, Real(0));
            else
               m_interpolator = std::make_shared<interpolator_type>(std::move(data), Real(0), Real(1) / (1 << grid_refinements));
         }
         else
            m_interpolator = std::make_shared<detail::null_interpolator>();
      }
   }

    inline Real operator()(Real x) const
    {
        if (x <= 0 || x >= 2*p-1)
        {
            return 0;
        }
        return (*m_interpolator)(x);
    }

    inline Real prime(Real x) const
    {
        static_assert(p > 2, "The 3-vanishing moment Daubechies scaling function is the first which is continuously differentiable.");
        if (x <= 0 || x >= 2*p-1)
        {
            return 0;
        }
        return m_interpolator->prime(x);
    }

    inline Real double_prime(Real x) const
    {
        static_assert(p >= 6, "Second derivatives require at least 6 vanishing moments.");
        if (x <= 0 || x >= 2*p - 1)
        {
            return Real(0);
        }
        return m_interpolator->double_prime(x);
    }

    std::pair<Real, Real> support() const
    {
        return {Real(0), Real(2*p-1)};
    }

    int64_t bytes() const
    {
       return m_interpolator->bytes() + sizeof(m_interpolator);
    }

private:
   std::shared_ptr<interpolator_type> m_interpolator;
};

}
#endif

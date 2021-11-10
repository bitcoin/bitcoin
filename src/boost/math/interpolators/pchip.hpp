// Copyright Nick Thompson, 2020
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_PCHIP_HPP
#define BOOST_MATH_INTERPOLATORS_PCHIP_HPP
#include <memory>
#include <boost/math/interpolators/detail/cubic_hermite_detail.hpp>

namespace boost {
namespace math {
namespace interpolators {

template<class RandomAccessContainer>
class pchip {
public:
    using Real = typename RandomAccessContainer::value_type;

    pchip(RandomAccessContainer && x, RandomAccessContainer && y,
          Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
          Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN())
    {
        using std::isnan;
        if (x.size() < 4)
        {
            throw std::domain_error("Must be at least four data points.");
        }
        RandomAccessContainer s(x.size(), std::numeric_limits<Real>::quiet_NaN());
        if (isnan(left_endpoint_derivative))
        {
            // O(h) finite difference derivative:
            // This, I believe, is the only derivative guaranteed to be monotonic:
            s[0] = (y[1]-y[0])/(x[1]-x[0]);
        }
        else
        {
            s[0] = left_endpoint_derivative;
        }

        for (decltype(s.size()) k = 1; k < s.size()-1; ++k) {
            Real hkm1 = x[k] - x[k-1];
            Real dkm1 = (y[k] - y[k-1])/hkm1;

            Real hk = x[k+1] - x[k];
            Real dk = (y[k+1] - y[k])/hk;
            Real w1 = 2*hk + hkm1;
            Real w2 = hk + 2*hkm1;
            if ( (dk > 0 && dkm1 < 0) || (dk < 0 && dkm1 > 0) || dk == 0 || dkm1 == 0)
            {
                s[k] = 0;
            }
            else
            {
                s[k] = (w1+w2)/(w1/dkm1 + w2/dk);
            }

        }
        // Quadratic extrapolation at the other end:
        auto n = s.size();
        if (isnan(right_endpoint_derivative))
        {
            s[n-1] = (y[n-1]-y[n-2])/(x[n-1] - x[n-2]);
        }
        else
        {
            s[n-1] = right_endpoint_derivative;
        }
        impl_ = std::make_shared<detail::cubic_hermite_detail<RandomAccessContainer>>(std::move(x), std::move(y), std::move(s));
    }

    Real operator()(Real x) const {
        return impl_->operator()(x);
    }

    Real prime(Real x) const {
        return impl_->prime(x);
    }

    friend std::ostream& operator<<(std::ostream & os, const pchip & m)
    {
        os << *m.impl_;
        return os;
    }

    void push_back(Real x, Real y) {
        using std::abs;
        using std::isnan;
        if (x <= impl_->x_.back()) {
             throw std::domain_error("Calling push_back must preserve the monotonicity of the x's");
        }
        impl_->x_.push_back(x);
        impl_->y_.push_back(y);
        impl_->dydx_.push_back(std::numeric_limits<Real>::quiet_NaN());
        auto n = impl_->size();
        impl_->dydx_[n-1] = (impl_->y_[n-1]-impl_->y_[n-2])/(impl_->x_[n-1] - impl_->x_[n-2]);
        // Now fix s_[n-2]:
        auto k = n-2;
        Real hkm1 = impl_->x_[k] - impl_->x_[k-1];
        Real dkm1 = (impl_->y_[k] - impl_->y_[k-1])/hkm1;

        Real hk = impl_->x_[k+1] - impl_->x_[k];
        Real dk = (impl_->y_[k+1] - impl_->y_[k])/hk;
        Real w1 = 2*hk + hkm1;
        Real w2 = hk + 2*hkm1;
        if ( (dk > 0 && dkm1 < 0) || (dk < 0 && dkm1 > 0) || dk == 0 || dkm1 == 0)
        {
            impl_->dydx_[k] = 0;
        }
        else
        {
            impl_->dydx_[k] = (w1+w2)/(w1/dkm1 + w2/dk);
        }
    }

private:
    std::shared_ptr<detail::cubic_hermite_detail<RandomAccessContainer>> impl_;
};

}
}
}
#endif

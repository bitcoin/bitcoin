// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_CARDINAL_QUADRATIC_B_SPLINE_HPP
#define BOOST_MATH_INTERPOLATORS_CARDINAL_QUADRATIC_B_SPLINE_HPP
#include <memory>
#include <boost/math/interpolators/detail/cardinal_quadratic_b_spline_detail.hpp>


namespace boost{ namespace math{ namespace interpolators {

template <class Real>
class cardinal_quadratic_b_spline
{
public:
    // If you don't know the value of the derivative at the endpoints, leave them as nans and the routine will estimate them.
    // y[0] = y(a), y[n - 1] = y(b), step_size = (b - a)/(n -1).
    cardinal_quadratic_b_spline(const Real* const y,
                                size_t n,
                                Real t0 /* initial time, left endpoint */,
                                Real h  /*spacing, stepsize*/,
                                Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
                                Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN())
     : impl_(std::make_shared<detail::cardinal_quadratic_b_spline_detail<Real>>(y, n, t0, h, left_endpoint_derivative, right_endpoint_derivative))
    {}

    // Oh the bizarre error messages if we template this on a RandomAccessContainer:
    cardinal_quadratic_b_spline(std::vector<Real> const & y,
                                Real t0 /* initial time, left endpoint */,
                                Real h  /*spacing, stepsize*/,
                                Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
                                Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN())
     : impl_(std::make_shared<detail::cardinal_quadratic_b_spline_detail<Real>>(y.data(), y.size(), t0, h, left_endpoint_derivative, right_endpoint_derivative))
    {}


    Real operator()(Real t) const {
        return impl_->operator()(t);
    }

    Real prime(Real t) const {
       return impl_->prime(t);
    }

    Real t_max() const {
        return impl_->t_max();
    }

private:
    std::shared_ptr<detail::cardinal_quadratic_b_spline_detail<Real>> impl_;
};

}}}
#endif
